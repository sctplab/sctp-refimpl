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
			ifa6 = (struct in6_ifaddr *)ifa->ifa;
			ifa->flags = ifa6->ia6_flags;
			if (!ip6_use_deprecated) {
				if (ifa->flags &
				    IN6_IFF_DEPRECATED) {
					ifa->localifa_flags |= SCTP_ADDR_IFA_UNUSEABLE;
				} else {
					ifa->localifa_flags &= ~SCTP_ADDR_IFA_UNUSEABLE;
				}
			} else {
				ifa->localifa_flags &= ~SCTP_ADDR_IFA_UNUSEABLE;
			}
			if (ifa->flags &
			    (IN6_IFF_DETACHED |
			     IN6_IFF_ANYCAST |
			     IN6_IFF_NOTREADY)) {
				ifa->localifa_flags |= SCTP_ADDR_IFA_UNUSEABLE;
			} else {
				ifa->localifa_flags &= ~SCTP_ADDR_IFA_UNUSEABLE;
			}

		}
		if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
			return (0);
		}
 		/* ok to use deprecated addresses? */
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
sctp_add_addresses_to_i_ia(struct sctp_inpcb *inp, struct sctp_scoping *scope, 
			   struct mbuf *m_at, int cnt_inits_to)
{
	struct sctp_vrf *vrf = NULL;
	int cnt;
	uint32_t vrf_id;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	vrf_id = SCTP_DEFAULT_VRFID;
#else
	vrf_id = panda_get_vrf_from_call(); /* from socket option call? */
#endif
	vrf = sctp_find_vrf(vrf_id);
	if(vrf == NULL) {
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
#ifdef __APPLE__
				                     ifn->if_name,
#else
				                     ifn->if_xname,
#endif
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
#ifdef __APPLE__
		                            ifa->ifa_ifp->if_name,
#else
		                            ifa->ifa_ifp->if_xname,
#endif
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
	if(cmd == RTM_ADD) {
		wi->action = SCTP_ADD_IP_ADDRESS;
	} else if (cmd == RTM_DELETE) {
		wi->action = SCTP_DEL_IP_ADDRESS;
	}
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
