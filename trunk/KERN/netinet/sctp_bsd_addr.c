/*-
 * Copyright (c) 2001-2007, Cisco Systems, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its 
 *    contributors may be used to endorse or promote products derived 
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $KAME: sctp_output.c,v 1.46 2005/03/06 16:04:17 itojun Exp $	 */

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/netinet/sctp_bsd_addr.c,v 1.4 2007/01/18 09:58:42 rrs Exp $");
#endif

#include <netinet/sctp_os.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_bsd_addr.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctp_indata.h>


/* XXX
 * This module needs to be rewritten with an eye towards getting
 * rid of the user of ifa.. and use another list method George
 * as told me of.
 */

#ifdef SCTP_DEBUG
extern uint32_t sctp_debug_on;
#endif

static struct sctp_ifa *
sctp_is_ifa_addr_prefered(struct sctp_ifa *ifa, 
			  uint8_t loopscope, 
			  uint8_t ip_scope, 
			  uint8_t * sin_loop, 
			  uint8_t * sin_local,
			  sa_family_t fam)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	/*
	 * Here we determine if its a prefered address. A prefered address
	 * means it is the same scope or higher scope then the destination.
	 * L = loopback, P = private, G = global
	 * ----------------------------------------- 
         *    src    |  dest | result
         *  ---------------------------------------- 
         *     L     |    L  |    yes
         *  -----------------------------------------
         *     P     |    L  |    yes 
         *  -----------------------------------------
         *     G     |    L  |    yes 
         *  -----------------------------------------
         *     L     |    P  |    no 
         *  -----------------------------------------
         *     P     |    P  |    yes 
         *  -----------------------------------------
         *     G     |    P  |    no
         *   -----------------------------------------
         *     L     |    G  |    no 
         *   -----------------------------------------
         *     P     |    G  |    no 
         *    -----------------------------------------
         *     G     |    G  |    yes 
         *    -----------------------------------------
	 */

	if (ifa->address.sa.sa_family != fam) {
		/* forget mis-matched family */
		return (NULL);
	}
	/* Ok the address may be ok */
	sin = (struct sockaddr_in *)&ifa->address.sin;
	*sin_local = *sin_loop = 0;
	if(fam == AF_INET) {
		if (sin->sin_addr.s_addr == 0) {
			/* TSNH */
			return (NULL);
		}
		if ((ifa->ifn_p->ifn_type == IFT_LOOP) ||
		    (IN4_ISLOOPBACK_ADDRESS(&sin->sin_addr))) {
			*sin_loop = 1;
			*sin_local = 1;
		}
		if ((IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
			*sin_local = 1;
		}
	} else if (fam == AF_INET6) {
		struct in6_ifaddr *ifa6;
		/* ok to use deprecated addresses? */

		/* FIX FIX? */
		ifa6 = (struct in6_ifaddr *)ifa->ifa;
		if (!ip6_use_deprecated) {
			if (IFA6_IS_DEPRECATED(ifa6)) {
				/* can't use this type */
				return (NULL);
			}
		}
		/* are we ok, with the current state of this address? */
		if (ifa6->ia6_flags &
		    (IN6_IFF_DETACHED | IN6_IFF_NOTREADY | IN6_IFF_ANYCAST)) {
			/* Can't use these types */
			return (NULL);
		}
		sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			/* we skip unspecifed addresses */
			return (NULL);
		}
		if ((ifa->ifn_p->ifn_type == IFT_LOOP) ||
		    (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))) {
			*sin_loop = 1;
			*sin_local = 1;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
			*sin_local = 1;
		}
	}
	if (!loopscope && *sin_loop) {
		/* Its a loopback address and we don't have loop scope */
		return (NULL);
	}
	if (!ip_scope && *sin_local) {
		/*
		 * Its a private address, and we don't have private address
		 * scope
		 */
		return (NULL);
	}
	if (((ip_scope == 0) && (loopscope == 0)) && (*sin_local)) {
		/* its a global src and a private dest */
		return (NULL);
	}
	/* its a prefered address */
	return (ifa);
}

static struct sctp_ifa *
sctp_is_ifa_addr_acceptable(struct sctp_ifa *ifa, 
			    uint8_t loopscope, 
			    uint8_t ip_scope, 
			    uint8_t * sin_loop, 
			    uint8_t * sin_local,
			    sa_family_t fam)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	/*
	 * Here we determine if its a acceptable address. A acceptable
	 * address means it is the same scope or higher scope but we can
	 * allow for NAT which means its ok to have a global dest and a
	 * private src.
	 * 
	 * L = loopback, P = private, G = global
	 * -----------------------------------------
         *  src    |  dest | result 
	 * -----------------------------------------
	 *   L     |   L   |    yes
	 *  -----------------------------------------
	 *   P     |   L   |    yes
	 *  -----------------------------------------
	 *   G     |   L   |    yes 
         * -----------------------------------------
	 *   L     |   P   |    no 
	 * -----------------------------------------
	 *   P     |   P   |    yes 
	 * -----------------------------------------
	 *   G     |   P   |    yes - May not work
	 * -----------------------------------------
	 *   L     |   G   |     no 
         * -----------------------------------------
	 *   P     |   G   |     yes - May not work
	 * -----------------------------------------
	 *   G     |   G   |     yes 
	 * -----------------------------------------
	 */

	if (ifa->address.sa.sa_family != fam) {
		/* forget non matching family */
		return (NULL);
	}
	/* Ok the address may be ok */
	*sin_local = *sin_loop = 0;
	if(fam == AF_INET) {
		sin = (struct sockaddr_in *)&ifa->address.sin;
		if (sin->sin_addr.s_addr == 0) {
			return (NULL);
		}
		if ((ifa->ifn_p->ifn_type == IFT_LOOP) ||
		    (IN4_ISLOOPBACK_ADDRESS(&sin->sin_addr))) {
			*sin_loop = 1;
			*sin_local = 1;
		}
		if ((IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
			*sin_local = 1;
		}
	} else {
		struct in6_ifaddr *ifa6;
		/* ok to use deprecated addresses? */

		/* FIX FIX? */
		ifa6 = (struct in6_ifaddr *)ifa->ifa;
		if (!ip6_use_deprecated) {
			if (IFA6_IS_DEPRECATED(ifa6)) {
				/* can't use this type */
				return (NULL);
			}
		}
		/* are we ok, with the current state of this address? */
		if (ifa6->ia6_flags &
		    (IN6_IFF_DETACHED | IN6_IFF_NOTREADY | IN6_IFF_ANYCAST)) {
			/* Can't use these types */
			return (NULL);
		}
		sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			/* we skip unspecifed addresses */
			return (NULL);
		}
		if ((ifa->ifn_p->ifn_type == IFT_LOOP) ||
		    (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))) {
			*sin_loop = 1;
			*sin_local = 1;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
			*sin_local = 1;
		}
	}
	if (!loopscope && *sin_loop) {
		/* Its a loopback address and we don't have loop scope */
		return (NULL);
	}
	/* its an acceptable address */
	return (ifa);
}

int
sctp_is_addr_restricted(struct sctp_tcb *stcb, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;

#ifdef SCTP_DEBUG
	int cnt = 0;

#endif
	if (stcb == NULL) {
		/* There are no restrictions, no TCB :-) */
		return (0);
	}
	LIST_FOREACH(laddr, &stcb->asoc.sctp_restricted_addrs, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_OUTPUT1) {
				printf("Help I have fallen and I can't get up!\n");
			}
#endif
			continue;
		}
		if (laddr->ifa == ifa) {
			/* Yes it is on the list */
			return (1);
		}
	}
	return (0);
}

static int
sctp_is_addr_in_ep(struct sctp_inpcb *inp, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;

	if (ifa == NULL)
		return (0);
	LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_OUTPUT1) {
				printf("Help I have fallen and I can't get up!\n");
			}
#endif
			continue;
		}
		if (laddr->ifa == ifa)
			/* same pointer */
			return (1);
	}
	return (0);
}



static struct sctp_ifa *
sctp_choose_boundspecific_inp(struct sctp_inpcb *inp,
			      struct route *ro,
			      uint32_t vrf_id,
			      uint8_t ip_scope,
			      uint8_t loopscope,
			      sa_family_t fam)
{
	struct sctp_laddr *laddr, *starting_point;
	void *ifn;
	int resettotop=0;
	struct sctp_ifn *sctp_ifn;
	struct sctp_ifa *sctp_ifa, *pass;
	uint8_t sin_loop, sin_local;
	struct sctp_vrf *vrf;
	uint32_t ifn_index;

	vrf = sctp_find_vrf(vrf_id);
	if(vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);
	sctp_ifn = sctp_find_ifn(vrf, ifn, ifn_index);
	/*
	 * first question, is the ifn we will emit on in our list, if so, we
	 * want such an address. Note that we first looked for a
	 * prefered address. If that fails we fall back to a acceptable
	 * address on the emit interface.
	 */
	if (sctp_ifn) {
		/* is a prefered one on the interface we route out? */
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			pass = sctp_is_ifa_addr_prefered(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
			if (pass == NULL)
				continue;
			if (sctp_is_addr_in_ep(inp, pass)) {
				atomic_add_int(&pass->refcount, 1);
				return (pass);
			}
		}
		/* is an acceptable one on the interface we route out? */
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			pass = sctp_is_ifa_addr_acceptable(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
			if (pass == NULL)
				continue;
			if (sctp_is_addr_in_ep(inp, sctp_ifa)) {
				atomic_add_int(&pass->refcount, 1);
				return (pass);
			}
		}
	}
	/* ok, now we now need to find one on the list
	 * of the addresses. We can't get one on the
	 * emitting interface so lets find first
	 * a prefered one. If not that a acceptable
	 * one otherwise... we return NULL.
	 */
	starting_point = inp->next_addr_touse;
 once_again:
	if (inp->next_addr_touse == NULL) {
		inp->next_addr_touse = LIST_FIRST(&inp->sctp_addr_list);
		resettotop = 1;
	}
	for (laddr = inp->next_addr_touse; laddr ; laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			/* address has been removed */
			continue;
		}
		pass = sctp_is_ifa_addr_prefered(laddr->ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		atomic_add_int(&pass->refcount, 1);
		return (pass);
	}
	if (resettotop == 0) {
		inp->next_addr_touse = NULL;
		goto once_again;
	}

	inp->next_addr_touse = starting_point;
	resettotop = 0;
 once_again_too:
	if(inp->next_addr_touse == NULL) {
		inp->next_addr_touse = LIST_FIRST(&inp->sctp_addr_list);
		resettotop = 1;
	}

	/* ok, what about an acceptable address in the inp */
	for (laddr = inp->next_addr_touse; laddr ; laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			/* address has been removed */
			continue;
		}
		pass = sctp_is_ifa_addr_acceptable(laddr->ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		atomic_add_int(&pass->refcount, 1);
		return (pass);
	}
	if (resettotop == 0) {
		inp->next_addr_touse = NULL;
		goto once_again_too;
	}

	/*
	 * no address bound can be a source for the destination we are in
	 * trouble
	 */
	return (NULL);
}



static struct sctp_ifa *
sctp_choose_boundspecific_stcb(struct sctp_inpcb *inp,
			       struct sctp_tcb *stcb,
			       struct sctp_nets *net,
			       struct route *ro,
			       uint32_t vrf_id,
			       uint8_t ip_scope,
			       uint8_t loopscope,
			       int non_asoc_addr_ok,
			       sa_family_t fam)				  
{
	struct sctp_laddr *laddr, *starting_point;
	void *ifn;
	struct sctp_ifn *sctp_ifn;
	struct sctp_ifa *sctp_ifa, *pass;
	uint8_t sin_loop, sin_local, start_at_beginning = 0;
	struct sctp_vrf *vrf;
	uint32_t ifn_index;
	/*
	 * first question, is the ifn we will emit on in our list, if so, we
	 * want that one.
	 */
	vrf = sctp_find_vrf(vrf_id);
	if(vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);
	sctp_ifn = sctp_find_ifn(vrf, ifn, ifn_index);

	if (sctp_is_feature_off(inp, SCTP_PCB_FLAGS_DO_ASCONF)) {
		/* We turn this off and get the effect that
		 * the restricted address list is NOT used.
		 */
		non_asoc_addr_ok = 1;
	}
	/*
	 * first question, is the ifn we will emit on in our list,
	 * if so, we want that one.. First we look for a prefered. Second
	 * we go for an acceptable.
	 */
	if (sctp_ifn) {
		/* first try for an prefered address on the ep */
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if (sctp_is_addr_in_ep(inp, sctp_ifa)) {
				pass = sctp_is_ifa_addr_prefered(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
				if (pass == NULL)
					continue;
				if ((non_asoc_addr_ok == 0) &&
				    (sctp_is_addr_restricted(stcb, pass))) {
					/* on the no-no list */
					continue;
				}
				atomic_add_int(&pass->refcount, 1);
				return (pass);
			}
		}
		/* next try for an acceptable address on the ep */
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if (sctp_is_addr_in_ep(inp, sctp_ifa)) {
				pass= sctp_is_ifa_addr_acceptable(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
				if (pass == NULL)
					continue;
				if ((non_asoc_addr_ok == 0) &&
				    (sctp_is_addr_restricted(stcb, pass))) {
					/* on the no-no list */
					continue;
				}
				atomic_add_int(&pass->refcount, 1);
				return (pass);
			}
		}

	}
	/*
	 * if we can't find one like that then we must look at all
	 * addresses bound to pick one at first prefereable then
	 * secondly acceptable.
	 */
	starting_point = stcb->asoc.last_used_address;
 sctp_from_the_top:
	if (stcb->asoc.last_used_address == NULL) {
		start_at_beginning = 1;
		stcb->asoc.last_used_address = LIST_FIRST(&stcb->asoc.sctp_local_addr_list);
	}
	/* search beginning with the last used address */
	for (laddr = stcb->asoc.last_used_address; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			/* address has been removed */
			continue;
		}
		pass = sctp_is_ifa_addr_prefered(laddr->ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		if ((non_asoc_addr_ok == 0) &&
		    (sctp_is_addr_restricted(stcb, pass))) {
			/* on the no-no list */
			continue;
		}
		stcb->asoc.last_used_address = laddr;
		atomic_add_int(&pass->refcount, 1);
		return (pass);

	}
	if (start_at_beginning == 0) {
		stcb->asoc.last_used_address = NULL;
		goto sctp_from_the_top;
	}
	/* now try for any higher scope than the destination */
	stcb->asoc.last_used_address = starting_point;
	start_at_beginning = 0;
 sctp_from_the_top2:
	if (stcb->asoc.last_used_address == NULL) {
		start_at_beginning = 1;
		stcb->asoc.last_used_address = LIST_FIRST(&stcb->asoc.sctp_local_addr_list);
	}
	/* search beginning with the last used address */
	for (laddr = stcb->asoc.last_used_address; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			/* address has been removed */
			continue;
		}
		pass = sctp_is_ifa_addr_acceptable(laddr->ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		if ((non_asoc_addr_ok == 0) &&
		    (sctp_is_addr_restricted(stcb, pass))) {
			/* on the no-no list */
			continue;
		}
		stcb->asoc.last_used_address = laddr;
		atomic_add_int(&pass->refcount, 1);
		return (pass);
	}
	if (start_at_beginning == 0) {
		stcb->asoc.last_used_address = NULL;
		goto sctp_from_the_top2;
	}
	return (NULL);
}

static struct sctp_ifa *
sctp_select_nth_prefered_addr_from_ifn_boundall(struct sctp_ifn *ifn, 
						struct sctp_tcb *stcb, 
						int non_asoc_addr_ok,
						uint8_t loopscope, 
						uint8_t ip_scope, 
						int addr_wanted,
						sa_family_t fam)
{
	struct sctp_ifa *ifa, *pass;
	uint8_t sin_loop, sin_local;
	int num_eligible_addr = 0;

	LIST_FOREACH(ifa, &ifn->ifalist, next_ifa) {
		pass = sctp_is_ifa_addr_prefered(ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		if (stcb) {
			if ((non_asoc_addr_ok == 0) && sctp_is_addr_restricted(stcb, pass)) {
				/*
				 * It is restricted for some reason..
				 * probably not yet added.
				 */
				continue;
			}
		}
		if (num_eligible_addr >= addr_wanted) {
			return (pass);
		}
		num_eligible_addr++;
	}
	return (NULL);
}


static int
sctp_count_num_prefered_boundall(struct sctp_ifn *ifn, 
				 struct sctp_tcb *stcb, 
				 int non_asoc_addr_ok,
				 uint8_t loopscope, 
				 uint8_t ip_scope,
				 uint8_t *sin_loop, 
				 uint8_t *sin_local,
				 sa_family_t fam)
{
	struct sctp_ifa *ifa, *pass;
	int num_eligible_addr = 0;

	LIST_FOREACH(ifa, &ifn->ifalist, next_ifa) {
		pass = sctp_is_ifa_addr_prefered(ifa, loopscope, ip_scope, sin_loop, sin_local, fam);
		if (pass == NULL)
			continue;
		if (stcb) {
			if ((non_asoc_addr_ok == 0) && sctp_is_addr_restricted(stcb, pass)) {
				/*
				 * It is restricted for some reason..
				 * probably not yet added.
				 */
				continue;
			}
		}
		num_eligible_addr++;
	}
	return (num_eligible_addr);

}

static struct sctp_ifa *
sctp_choose_boundall(struct sctp_inpcb *inp,
		     struct sctp_tcb *stcb,
		     struct sctp_nets *net,
		     struct route *ro,
		     uint32_t vrf_id,
		     uint8_t ip_scope,
		     uint8_t loopscope,
		     int non_asoc_addr_ok,
		     sa_family_t fam)
{
	int cur_addr_num = 0, num_prefered = 0;
	uint8_t sin_loop, sin_local;
	void *ifn;
	struct sctp_ifn *sctp_ifn, *looked_at=NULL;
	struct sctp_ifa *sctp_ifa, *pass;
	uint32_t ifn_index;
	struct sctp_vrf *vrf;
	/*
	 * For boundall  we can use  any address in the association.
	 * If non_asoc_addr_ok is set we can use any address (at least in
	 * theory). So we look for prefered addresses first. If we find one,
	 * we use it. Otherwise we next try to get an address on the
	 * interface, which we should be able to do (unless non_asoc_addr_ok
	 * is false and we are routed out that way). In these cases where we
	 * can't use the address of the interface we go through all the
	 * ifn's looking for an address we can use and fill that in. Punting
	 * means we send back address 0, which will probably cause problems
	 * actually since then IP will fill in the address of the route ifn,
	 * which means we probably already rejected it.. i.e. here comes an
	 * abort :-<.
	 */
	vrf = sctp_find_vrf(vrf_id);
	if(vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);

	looked_at = sctp_ifn = sctp_find_ifn(vrf, ifn, ifn_index);
	if (sctp_ifn == NULL) {
		/* ?? We don't have this guy ?? */
		goto bound_all_plan_c;
	}
	if (net) {
		cur_addr_num = net->indx_of_eligible_next_to_use;
	}
	num_prefered = sctp_count_num_prefered_boundall(sctp_ifn, 
							stcb, 
							non_asoc_addr_ok, 
							loopscope, 
							ip_scope, 
							&sin_loop, 
							&sin_local, fam);
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_OUTPUT1) {
		printf("Found %d prefered source addresses\n", num_prefered);
	}
#endif
	if (num_prefered == 0) {
		/*
		 * no eligible addresses, we must use some other interface
		 * address if we can find one.
		 */
		goto bound_all_plan_b;
	}
	/*
	 * Ok we have num_eligible_addr set with how many we can use, this
	 * may vary from call to call due to addresses being deprecated
	 * etc..
	 */
	if (cur_addr_num >= num_prefered) {
		cur_addr_num = 0;
	}
	/*
	 * select the nth address from the list (where cur_addr_num is the
	 * nth) and 0 is the first one, 1 is the second one etc...
	 */
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_OUTPUT1) {
		printf("cur_addr_num:%d\n", cur_addr_num);
	}
#endif
	sctp_ifa = sctp_select_nth_prefered_addr_from_ifn_boundall(ifn, stcb, non_asoc_addr_ok, loopscope,
								      ip_scope, cur_addr_num, fam);

	/* if sctp_ifa is NULL something changed??, fall to plan b. */
	if (sctp_ifa) {
		atomic_add_int(&sctp_ifa->refcount, 1);
		if (net) {
			/* save off where the next one we will want */
			net->indx_of_eligible_next_to_use = cur_addr_num + 1;
		}
		return (sctp_ifa);
	}
	/*
	 * plan_b: Look at the interface that we emit on and see if we can
	 * find an acceptable address.
	 */
bound_all_plan_b:
	LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
		pass = sctp_is_ifa_addr_acceptable(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
		if (pass == NULL)
			continue;
		if (stcb) {
			if ((non_asoc_addr_ok == 0) && sctp_is_addr_restricted(stcb, pass)) {
				/*
				 * It is restricted for some reason..
				 * probably not yet added.
				 */
				continue;
			}
		}
		atomic_add_int(&pass->refcount, 1);
		return (pass);
	}
	/*
	 * plan_c: Look at all interfaces and find a prefered address. If we
	 * reach here we are in trouble I think.
	 */
bound_all_plan_c:
	LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
		if (loopscope == 0 && sctp_ifn->ifn_type == IFT_LOOP) {
			/* wrong base scope */
			continue;
		}
		if ((sctp_ifn == looked_at) && looked_at)
			/* already looked at this guy */
			continue;
		num_prefered = sctp_count_num_prefered_boundall(sctp_ifn, stcb, non_asoc_addr_ok,
								   loopscope, ip_scope, &sin_loop, &sin_local, fam);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_OUTPUT1) {
			printf("Found ifn:%p %d prefered source addresses\n", ifn, num_prefered);
		}
#endif
		if (num_prefered == 0) {
			/*
			 * None on this interface.
			 */
			continue;
		}
		/*
		 * Ok we have num_eligible_addr set with how many we can
		 * use, this may vary from call to call due to addresses
		 * being deprecated etc..
		 */
		if (cur_addr_num >= num_prefered) {
			cur_addr_num = 0;
		}
		pass = sctp_select_nth_prefered_addr_from_ifn_boundall(ifn, stcb, non_asoc_addr_ok, loopscope,
		    ip_scope, cur_addr_num, fam);
		if (pass == NULL)
			continue;
		if (net) {
			net->indx_of_eligible_next_to_use = cur_addr_num + 1;
		}
		atomic_add_int(&pass->refcount, 1);
		return (pass);

	}

	/*
	 * plan_d: We are in deep trouble. No prefered address on any
	 * interface. And the emit interface does not even have an
	 * acceptable address. Take anything we can get! If this does not
	 * work we are probably going to emit a packet that will illicit an
	 * ABORT, falling through.
	 */

	LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
		if (loopscope == 0 && sctp_ifn->ifn_type == IFT_LOOP) {
			/* wrong base scope */
			continue;
		}

		if ((sctp_ifn == looked_at) && looked_at)
			/* already looked at this guy */
			continue;

		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			pass = sctp_is_ifa_addr_acceptable(sctp_ifa, loopscope, ip_scope, &sin_loop, &sin_local, fam);
			if (pass == NULL)
				continue;
			if (stcb) {
				if ((non_asoc_addr_ok == 0) && sctp_is_addr_restricted(stcb, pass)) {
					/*
					 * It is restricted for some
					 * reason.. probably not yet added.
					 */
					continue;
				}
			}
			atomic_add_int(&pass->refcount, 1);
			return (pass);
		}
	}
	/*
	 * Ok we can find NO address to source from that is not on our
	 * negative list and non_asoc_address is NOT ok, or its on
	 * our negative list. We cant source to it :-(
	 */
	return (NULL);
}



/* tcb may be NULL */
struct sctp_ifa *
sctp_source_address_selection(struct sctp_inpcb *inp,
			      struct sctp_tcb *stcb, 
			      struct route *ro, 
			      struct sctp_nets *net,
			      int non_asoc_addr_ok, uint32_t vrf_id)
{
	
	struct sockaddr_in *to = (struct sockaddr_in *)&ro->ro_dst;
	struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&ro->ro_dst;
	struct sctp_ifa *answer;
	uint8_t loc_scope, loopscope;
	int did_rtalloc=0;
	sa_family_t fam;
	/*
	 * Rules: - Find the route if needed, cache if I can. - Look at
	 * interface address in route, Is it in the bound list. If so we
	 * have the best source. - If not we must rotate amongst the
	 * addresses.
	 * 
	 * Cavets and issues
	 * 
	 * Do we need to pay attention to scope. We can have a private address
	 * or a global address we are sourcing or sending to. So if we draw
	 * it out 
	 * zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
	 * For V4
         *------------------------------------------
	 *      source     *      dest  *  result
	 * -----------------------------------------
	 * <a>  Private    *    Global  *	NAT  
	 * ----------------------------------------- 
	 * <b>  Private    *    Private *  No problem
	 * ----------------------------------------- 
         * <c>  Global     *    Private *  Huh, How will this work?
	 * ----------------------------------------- 
         * <d>  Global     *    Global  *  No Problem 
         *------------------------------------------
	 * zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
	 * For V6
         *------------------------------------------
	 *      source     *      dest  *  result
	 * -----------------------------------------
	 * <a>  Linklocal  *    Global  *	
	 * ----------------------------------------- 
	 * <b>  Linklocal  * Linklocal  *  No problem
	 * ----------------------------------------- 
         * <c>  Global     * Linklocal  *  Huh, How will this work?
	 * ----------------------------------------- 
         * <d>  Global     *    Global  *  No Problem 
         *------------------------------------------
	 * zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
         *
	 * And then we add to that what happens if there are multiple addresses
	 * assigned to an interface. Remember the ifa on a ifn is a linked
	 * list of addresses. So one interface can have more than one IP
	 * address. What happens if we have both a private and a global
	 * address? Do we then use context of destination to sort out which
	 * one is best? And what about NAT's sending P->G may get you a NAT
	 * translation, or should you select the G thats on the interface in
	 * preference.
	 * 
	 * Decisions:
	 * 
	 * - count the number of addresses on the interface. 
         * - if its one, no  problem except case <c>. 
         *   For <a> we will assume a NAT out there.
	 * - if there are more than one, then we need to worry about scope P
	 *   or G. We should prefer G -> G and P -> P if possible. 
	 *   Then as a secondary fall back to mixed types G->P being a last ditch one. 
         * -  The above all works for bound all, but bound specific we need to
	 *    use the same concept but instead only consider the bound
	 *    addresses. If the bound set is NOT assigned to the interface then
	 *    we must use rotation amongst the bound addresses..
	 * 
	 */
	if (ro->ro_rt == NULL) {
		/*
		 * Need a route to cache.
		 * 
		 */
#if defined(__FreeBSD__) || defined(__APPLE__)
		rtalloc_ign(ro, 0UL);
#else
		rtalloc(ro);
#endif
		did_rtalloc=1;
	}
	if (ro->ro_rt == NULL) {
		return (NULL);
	}
	fam = to->sin_family;
	loc_scope = loopscope = 0;
	/* Setup our scopes for the destination */
	if(fam == AF_INET) {
		/* Scope based on outbound address */
		if ((IN4_ISPRIVATE_ADDRESS(&to->sin_addr))) {
			loc_scope = 1;
			loopscope = 0;
		} else if (IN4_ISLOOPBACK_ADDRESS(&to->sin_addr)) {
			loc_scope = 1;
			loopscope = 1;
		} else {
			loc_scope = 0;
			loopscope = 0;
		}
	} else if (fam == AF_INET6) {
		/* Scope based on outbound address */
		if (IN6_IS_ADDR_LOOPBACK(&to6->sin6_addr)) {
			/*
			 * If the route goes to the loopback address OR the address
			 * is a loopback address, we are loopback scope. But
			 * we don't use loc_scope (link local addresses).
			 */
			loc_scope = 0;
			loopscope = 1;
			if (net != NULL) {
				/* mark it as local */
				net->addr_is_local = 1;
			}
		} else if (IN6_IS_ADDR_LINKLOCAL(&to6->sin6_addr)) {
			if (to6->sin6_scope_id)
				loc_scope = to6->sin6_scope_id;
			else {
				loc_scope = 1;
			}
			loopscope = 0;
		}
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		/*
		 * When bound to all if the address list is set it is a
		 * negative list. Addresses being added by asconf.
		 */
		answer = sctp_choose_boundall(inp, stcb, net, ro, vrf_id,
					      loc_scope, 
					      loopscope, 
					      non_asoc_addr_ok,  
					      fam);
		return (answer);
	}
	/*
	 * Three possiblities here:
	 * 
	 * a) stcb is NULL, which means we operate only from the list of
	 * addresses (ifa's) bound to the endpoint and we care not about the
	 * list. 
	 * b) stcb is NOT-NULL, which means we have an assoc structure
	 * and auto-asconf is on. This means that the list of addresses is a
	 * NOT list. We use the list from the inp, but any listed address in
	 * our list is NOT yet added. However if the non_asoc_addr_ok is set
	 * we CAN use an address NOT available (i.e. being added). Its a
	 * negative list. 
	 *c) stcb is NOT-NULL, which means we have an assoc
	 * structure and auto-asconf is off. This means that the list of
	 * addresses is the ONLY addresses I can use.. its positive.
	 * 
	 * Note we collapse b & c into the same function just like in the v6
	 * address selection.
	 */
	if (stcb) {
		answer = sctp_choose_boundspecific_stcb(inp, stcb, net, ro, vrf_id,
							loc_scope, loopscope, non_asoc_addr_ok, fam);

	} else {
		answer = sctp_choose_boundspecific_inp(inp, ro, vrf_id, loc_scope, loopscope, fam);
		
	}
	return (answer);
}

static
int
sctp_is_address_in_scope(struct sctp_ifa *ifa,
    int ipv4_addr_legal,
    int ipv6_addr_legal,
    int loopback_scope,
    int ipv4_local_scope,
    int local_scope,
    int site_scope,
    int do_update)
{
	if ((loopback_scope == 0) &&
	    (ifa->ifn_p) &&
	    (ifa->ifn_p->ifn_type == IFT_LOOP)) {
		/*
		 * skip loopback if not in scope *
		 */
		return (0);
	}
	if ((ifa->address.sa.sa_family == AF_INET) && ipv4_addr_legal) {
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)&ifa->address.sin;
		if (sin->sin_addr.s_addr == 0) {
			/* not in scope , unspecified */
			return (0);
		}
		if ((ipv4_local_scope == 0) &&
		    (IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
			/* private address not in scope */
			return (0);
		}
	} else if ((ifa->address.sa.sa_family == AF_INET6) && ipv6_addr_legal) {
		struct sockaddr_in6 *sin6;
		struct in6_ifaddr *ifa6;

		/* Must update the flags,  bummer, which
		 * means any IFA locks must now be applied HERE <->
		 */
		if(do_update) {
			/* LOCK ?? */
			ifa6 = (struct in6_ifaddr *)ifa->ifa;
			ifa->flags = ifa6->ia6_flags;
			/* UNLOCK ?? */
		}
		/* ok to use deprecated addresses? */
		if (!ip6_use_deprecated) {
			if (ifa->flags &
			    IN6_IFF_DEPRECATED) {
				return (0);
			}
		}
		if (ifa->flags &
		    (IN6_IFF_DETACHED |
		    IN6_IFF_ANYCAST |
		    IN6_IFF_NOTREADY)) {
			return (0);
		}
		sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			/* skip unspecifed addresses */
			return (0);
		}
		if (		/* (local_scope == 0) && */
		    (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))) {
			return (0);
		}
		if ((site_scope == 0) &&
		    (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))) {
			return (0);
		}
	} else {
		return (0);
	}
	return (1);
}

static struct mbuf *
sctp_add_addr_to_mbuf(struct mbuf *m, struct sctp_ifa *ifa)
{
	struct sctp_paramhdr *parmh;
	struct mbuf *mret;
	int len;

	if (ifa->address.sa.sa_family == AF_INET) {
		len = sizeof(struct sctp_ipv4addr_param);
	} else if (ifa->address.sa.sa_family == AF_INET6) {
		len = sizeof(struct sctp_ipv6addr_param);
	} else {
		/* unknown type */
		return (m);
	}
	if (M_TRAILINGSPACE(m) >= len) {
		/* easy side we just drop it on the end */
		parmh = (struct sctp_paramhdr *)(SCTP_BUF_AT(m, SCTP_BUF_LEN(m)));
		mret = m;
	} else {
		/* Need more space */
		mret = m;
		while (SCTP_BUF_NEXT(mret) != NULL) {
			mret = SCTP_BUF_NEXT(mret);
		}
		SCTP_BUF_NEXT(mret) = sctp_get_mbuf_for_msg(len, 0, M_DONTWAIT, 1, MT_DATA);
		if (SCTP_BUF_NEXT(mret) == NULL) {
			/* We are hosed, can't add more addresses */
			return (m);
		}
		mret = SCTP_BUF_NEXT(mret);
		parmh = mtod(mret, struct sctp_paramhdr *);
	}
	/* now add the parameter */
	if (ifa->address.sa.sa_family == AF_INET) {
		struct sctp_ipv4addr_param *ipv4p;
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)&ifa->address.sin;
		ipv4p = (struct sctp_ipv4addr_param *)parmh;
		parmh->param_type = htons(SCTP_IPV4_ADDRESS);
		parmh->param_length = htons(len);
		ipv4p->addr = sin->sin_addr.s_addr;
		SCTP_BUF_LEN(mret) += len;
	} else if (ifa->address.sa.sa_family == AF_INET6) {
		struct sctp_ipv6addr_param *ipv6p;
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
		ipv6p = (struct sctp_ipv6addr_param *)parmh;
		parmh->param_type = htons(SCTP_IPV6_ADDRESS);
		parmh->param_length = htons(len);
		memcpy(ipv6p->addr, &sin6->sin6_addr,
		    sizeof(ipv6p->addr));
		/* clear embedded scope in the address */
		in6_clearscope((struct in6_addr *)ipv6p->addr);
		SCTP_BUF_LEN(mret) += len;
	} else {
		return (m);
	}
	return (mret);
}


struct mbuf *
sctp_add_addresses_to_i_ia(struct sctp_inpcb *inp, struct sctp_scoping *scope, struct mbuf *m_at, int cnt_inits_to)
{
	struct sctp_vrf *vrf = NULL;
	int cnt;
	vrf = sctp_find_vrf(SCTP_DEFAULT_VRFID);
	if(vrf == NULL) {
		printf("Gak, can't find default VRF?\n");
		return(m_at);
	}

	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		struct sctp_ifa *sctp_ifap;
		struct sctp_ifn *sctp_ifnp;

		cnt = cnt_inits_to;
		LIST_FOREACH(sctp_ifnp, &vrf->ifnlist, next_ifn) {
			if ((scope->loopback_scope == 0) &&
			    (sctp_ifnp->ifn_type == IFT_LOOP)) {
				/*
				 * Skip loopback devices if loopback_scope
				 * not set
				 */
				continue;
			}
			LIST_FOREACH(sctp_ifap, &sctp_ifnp->ifalist, next_ifa) {
				if (sctp_is_address_in_scope(sctp_ifap,
							     scope->ipv4_addr_legal,
							     scope->ipv6_addr_legal,
							     scope->loopback_scope,
							     scope->ipv4_local_scope,
							     scope->local_scope,
							     scope->site_scope, 1) == 0) {
					continue;
				}
				cnt++;
			}
		}
		if (cnt > 1) {
			LIST_FOREACH(sctp_ifnp, &vrf->ifnlist, next_ifn) {
				if ((scope->loopback_scope == 0) &&
				    (sctp_ifnp->ifn_type == IFT_LOOP)) {
					/*
					 * Skip loopback devices if
					 * loopback_scope not set
					 */
					continue;
				}
				LIST_FOREACH(sctp_ifap, &sctp_ifnp->ifalist, next_ifa) {
					if (sctp_is_address_in_scope(sctp_ifap,
								     scope->ipv4_addr_legal,
								     scope->ipv6_addr_legal,
								     scope->loopback_scope,
								     scope->ipv4_local_scope,
								     scope->local_scope,
								     scope->site_scope, 0) == 0) {
						continue;
					}
					m_at = sctp_add_addr_to_mbuf(m_at, sctp_ifap);
				}
			}
		}
	} else {
		struct sctp_laddr *laddr;
		int cnt;

		cnt = cnt_inits_to;
		/* First, how many ? */
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa == NULL) {
				continue;
			}
			if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED)
				continue;

			if (sctp_is_address_in_scope(laddr->ifa,
						     scope->ipv4_addr_legal,
						     scope->ipv6_addr_legal,
						     scope->loopback_scope,
						     scope->ipv4_local_scope,
						     scope->local_scope,
						     scope->site_scope, 1) == 0) {
				continue;
			}
			cnt++;
		}
		/*
		 * To get through a NAT we only list addresses if we have
		 * more than one. That way if you just bind a single address
		 * we let the source of the init dictate our address.
		 */
		if (cnt > 1) {
			LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
				if (laddr->ifa == NULL) {
					continue;
				}
				if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED)
					continue;

				if (sctp_is_address_in_scope(laddr->ifa,
							     scope->ipv4_addr_legal,
							     scope->ipv6_addr_legal,
							     scope->loopback_scope,
							     scope->ipv4_local_scope,
							     scope->local_scope,
							     scope->site_scope, 0) == 0) {
					continue;
				}
				m_at = sctp_add_addr_to_mbuf(m_at, laddr->ifa);
			}
		}
	}
	return (m_at);
}


static void
sctp_init_ifns_for_vrf(struct sctp_vrf *vrf)
{
	/* Here we must apply ANY locks needed by the
	 * IFN we access and also make sure we lock
	 * any IFA that exists as we float through the
	 * list of IFA's
	 */
	struct ifnet *ifn;
	struct ifaddr *ifa;
	struct in6_ifaddr *ifa6;
	uint32_t ifa_flags;
	
	TAILQ_FOREACH(ifn, &ifnet, if_list) {
		TAILQ_FOREACH(ifa, &ifn->if_addrlist, ifa_list) {
			if((ifa->ifa_addr->sa_family == AF_INET6) ||
			   (ifa->ifa_addr->sa_family == AF_INET)) {
				if (ifa->ifa_addr->sa_family == AF_INET6) {
					ifa6 = (struct in6_ifaddr *)ifa;
					ifa_flags = ifa6->ia6_flags;
				} else {
					ifa_flags = 0;
				}
				sctp_add_addr_to_vrf(vrf, 
						     (void *)ifn,
						     ifn->if_index, 
						     ifn->if_type,
						     (void *)ifa,
						     ifa->ifa_addr,
						     ifa_flags
						     );
			}
		}
	}
}


void 
sctp_init_vrf_list(int vrfid)
{
	struct sctp_vrf *vrf = NULL;
	struct sctp_vrflist *bucket;

	if(vrfid > SCTP_MAX_VRF_ID)
		/* can't do that */
		return;

	/* First allocate the VRF structure */
	SCTP_MALLOC(vrf, struct sctp_vrf *, sizeof(struct sctp_vrf), "SCTP_VRF");
	if(vrf == NULL) {
		/* No memory */
#ifdef INVARIANTS
		panic("No memory for VRF:%d", vrfid);
#endif
		return;
	}
	/* setup the VRF */
	memset(vrf, 0, sizeof(struct sctp_vrf));
	vrf->vrf_id = vrfid;
	LIST_INIT(&vrf->ifnlist);

	/* Add it to the hash table */
	bucket = &sctppcbinfo.sctp_vrfhash[(vrfid & sctppcbinfo.hashvrfmark)];
	LIST_INSERT_HEAD(bucket, vrf, next_vrf);
	
	/* Now we need to build all the ifn's for this vrf and there addresses*/
	sctp_init_ifns_for_vrf(vrf);
}


void
sctp_addr_change(struct ifaddr *ifa, int cmd)
{
	struct sctp_laddr *wi;
	struct sctp_vrf *vrf;
	struct sctp_ifa *ifap=NULL;
	uint32_t ifa_flags=0;
	/* BSD only has one VRF, if this changes
	 * we will need to hook in the right 
	 * things here to get the id to pass to
	 * the address managment routine.
	 */
	vrf = sctp_find_vrf(SCTP_DEFAULT_VRFID);
	if(vrf == NULL)
		return;

	if(cmd == RTM_ADD) {
		struct in6_ifaddr *ifa6;
		if(ifa->ifa_addr->sa_family == AF_INET6) {
			ifa6 = (struct in6_ifaddr *)ifa;
			ifa_flags = ifa6->ia6_flags;
		}
		ifap = sctp_add_addr_to_vrf(vrf, (void *)ifa->ifa_ifp,
					    ifa->ifa_ifp->if_index, ifa->ifa_ifp->if_type,
					    (void *)ifa, ifa->ifa_addr, ifa_flags);
		/* Bump up the refcount so that when the timer
		 * completes it will drop back down.
		 */
 		if(ifap)
			atomic_add_int(&ifap->refcount, 1);
		
	} else if (cmd == RTM_DELETE) {
		ifap = sctp_del_addr_from_vrf(vrf, ifa->ifa_addr, ifa->ifa_ifp->if_index);
		/* We don't bump refcount here so when it completes
		 * the final delete will happen.
		 */
 	}
	if (ifap == NULL) 
		return;

	wi = SCTP_ZONE_GET(sctppcbinfo.ipi_zone_laddr, struct sctp_laddr);
	if (wi == NULL) {
		/*
		 * Gak, what can we do? We have lost an address change can
		 * you say HOSED?
		 */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_PCB1) {
			printf("Lost and address change ???\n");
		}
#endif				/* SCTP_DEBUG */

		/* Opps, must decrement the count */
		sctp_free_ifa(ifap);
		return;
	}
	SCTP_INCR_LADDR_COUNT();
	bzero(wi, sizeof(*wi));
	wi->ifa = ifap;
	wi->action = cmd;
	SCTP_IPI_ADDR_LOCK();
	/*
	 * Should this really be a tailq? As it is we will process the
	 * newest first :-0
	 */
	LIST_INSERT_HEAD(&sctppcbinfo.addr_wq, wi, sctp_nxt_addr);
	sctp_timer_start(SCTP_TIMER_TYPE_ADDR_WQ,
			 (struct sctp_inpcb *)NULL,
			 (struct sctp_tcb *)NULL,
			 (struct sctp_nets *)NULL);
	SCTP_IPI_ADDR_UNLOCK();
}
