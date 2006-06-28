#include <sys/types.h>
#include <stdio.h>
#include <rserpool_lib.h>
#include <rserpool.h>
#include <rserpool_io.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <rserpool_util.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

struct rsp_timer_entry *
asap_find_req(struct rsp_enrp_scope *scp,
	      char *name, 
	      int name_len, 
	      int type)
{
	struct rsp_timer_entry *tme=NULL;
	int failed_lock =0;
	/*
	 * For now we look through the running timers. In
	 * theory if we have multiple scopes in a process this
	 * is a bit in-efficent and we should have a per scope
	 * list of requests. But since this is more rare IMO
	 * we will leave it with just the master timer list 
	 * looking for a entry. This can be changed later
	 * if we need to.
	 */
	if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
		fprintf(stderr, "Unsafe access %d can't lock timer to find entry\n", errno);
		failed_lock = 1;
	}
	dlist_reset(rsp_pcbinfo.timer_list);
	while ((tme = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
		if(tme->req == NULL) {
			/* not a request for us */
			continue;
		}
		if((tme->req->request_type == type) &&
		   (name_len == tme->req->namelen)) {
			/* its possible, lets copmare */
			if(memcmp(tme->req->name, name, name_len) == 0) {
				/* yep, we found it */
				break;
			}
		}
	}
	if(!failed_lock) {
		if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access,  unlock failed for rsp_tmr_mtx:%d during find entry\n", errno);
		}
	}
	return(tme);
}


static struct rsp_paramhdr *
get_next_parameter (uint8_t *buf, 
		    uint8_t *limit, 
		    uint8_t **nxt, 
		    uint16_t *type)
{
	struct rsp_paramhdr *ph;
	uint16_t len;

	*type = RSP_PARAM_RSVD;
	if( ((caddr_t)limit - (caddr_t)buf)  < (sizeof(struct rsp_paramhdr))) {
		/* not even enough space for a param. */
		return(NULL);
	}
	ph = (struct rsp_paramhdr *)buf;
	len = ntohs(ph->param_length);
	if((buf + len) >= limit) {
		/* to long */
		return(NULL);
	}
	/* save back the len in local byte order */
	ph->param_length = len;
	*type = ntohs(ph->param_type);

	/* save back the type in local byte order */
	ph->param_type = *type;
	if(nxt) {
		*nxt = (uint8_t *)(((caddr_t)ph) + len);
	}
	return(ph);
}

struct sockaddr *
asap_pull_and_alloc_an_address(struct sockaddr **loc, 
			       int ttype, 
			       union rsp_address_union *address, 
			       uint16_t port, 
			       size_t *len )
{
	struct sockaddr *to;
	if((loc == NULL) || (*loc == NULL)){
		/* New, get new memory */
 		if(ttype == RSP_PARAM_IPV4_ADDR) {
			to = (struct sockaddr *)malloc(sizeof(struct sockaddr_in));
			*len = sizeof(struct sockaddr_in);
		} else if (ttype == RSP_PARAM_IPV6_ADDR) {
			to = (struct sockaddr *)malloc(sizeof(struct sockaddr_in6));
			*len = sizeof(struct sockaddr_in6);
		} else {
			*len = 0;
			if(loc == NULL)
				return(NULL);
			else
				return(*loc);
		}
		if(to == NULL) {
			*len = 0;
			if(loc == NULL)
				return (NULL);
			else
				return(*loc);
		}
		if(loc)
			*loc = to;
	} else {
		/* do realloc thing, *len is old size */
		size_t old_siz, new_siz;
		void *old, *new;
		old_siz = *len;
		old = (void *)*loc;
		if(ttype == RSP_PARAM_IPV4_ADDR) {
			new_siz = old_siz + sizeof(struct sockaddr_in); 
			new = realloc(old, new_siz);
			if(new == NULL) {
				return(*loc);
			}
			to = (struct sockaddr *)((caddr_t)new + old_siz);
			memset(to, 0, sizeof(struct sockaddr_in));
		} else if (ttype == RSP_PARAM_IPV6_ADDR) {
			new_siz = old_siz + sizeof(struct sockaddr_in6); 
			new = realloc(old, new_siz);
			if(new == NULL) {
				return(*loc);
			}
			to = (struct sockaddr *)((caddr_t)new + old_siz);
			memset(to, 0, sizeof(struct sockaddr_in6));
		} else {
			/* *len is untouched */
			return (*loc);
		}
	}
	if(ttype == RSP_PARAM_IPV4_ADDR) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)to;
		memset(sin, 0, sizeof(struct sockaddr_in));
		sin->sin_family = AF_INET;
		sin->sin_port = port;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_addr = address->ipv4.in;
	} else if (ttype == RSP_PARAM_IPV6_ADDR) {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)to;

		memset(sin6, 0, sizeof(struct sockaddr_in6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = port;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		sin6->sin6_addr = address->ipv6.in6;
	}
	if(loc)
		return(*loc);
	else
		return(to);
}

void
asap_destroy_pe(struct rsp_pool_ele *pes, int remove_from_cache)
{
	void *v;
	struct sockaddr *sa;
	struct rsp_pool *pool;
	struct rsp_pool_ele *ele;
	struct rsp_enrp_scope *scp; 
	int i;
	/* First un-hash the old SCTP addresses if relevant */

	pool = pes->pool;
	scp = pool->scp;

	if((pes->protocol_type == RSP_PARAM_SCTP_TRANSPORT) &&
	   (remove_from_cache)){
		sa = pes->addrList;
		for (i=0; i<pes->number_of_addr; i++) {
			v = HashedTbl_remove(scp->ipaddrPortHash, sa, sa->sa_len, NULL);
			sa = (struct sockaddr *)((char *)sa + sa->sa_len);
		}
	}
	pes->pool->refcnt--;
	if(pes->addrList)
		free(pes->addrList);
	pes->addrList = NULL;
	pes->len = 0;
	pes->number_of_addr = 0;
	while ((v = dlist_getNext(pes->failover_list)) != NULL) {
		;
	}
	dlist_destroy(pes->failover_list);
	pes->failover_list = NULL;

	/* now lets find and remove from the pool master list */
	dlist_reset(pool->peList);
	while ((v = dlist_get(pool->peList)) != NULL) {
		/* here is another pes, is it me? */
		if (v == (void *)pes) {
			/* yep remove it */
			dlist_getThis(pool->peList);
			break;
		}
	}

	/* now lets make sure there are NO secondary references to us */
	dlist_reset(pool->peList);
	while ((v = dlist_get(pool->peList)) != NULL) {
		ele = (struct rsp_pool_ele *)v;
		dlist_reset(ele->failover_list); 
		while ((v = dlist_get(ele->failover_list)) != NULL) {
			if(v == (void *)pes) {
				/* found me ref'ed by a peer */
				dlist_getThis(ele->failover_list);
				break;
			}
		}
	}
	/* now purge the memory, and we are done. */
	free(pes);
}


struct rsp_pool_ele *
asap_build_pool_element(struct rsp_enrp_scope *scp, 
			struct rsp_pool *pool,
			struct rsp_pool_element *pe,
			uint8_t* limit,
			uint32_t id)
{
	/* This takes a PE from the wire and makes a
	 * NEW internal representation, note it does NOT
	 * append to the pool in the dlist but does
	 * increment the pool use count. 
	 */
	uint16_t transport, ttype, ptype;
	uint8_t *calc, *at, *nxt ;
	int siz,i;
	struct sockaddr *sa;
	struct rsp_paramhdr *ph;
	struct rsp_pool_ele *pes;
	struct rsp_sctp_transport *sctp;
	struct rsp_tcp_transport *tcp;
	struct rsp_udp_transport *udp;
	struct rsp_select_policy  *policy;

	/* if we reach here we are at step 2 and this is a new name */
	pes = malloc(sizeof(struct rsp_pool_ele));
	if(pes == NULL) {
		/* no memory ? */
		return(NULL);
	}
	memset(pes, 0, sizeof(*pes));
	pes->failover_list = dlist_create();
	if(pes->failover_list == NULL) {
		/* no memory */
		free(pes);
		return(NULL);
	}
	pes->pool = pool;
	pes->state = RSP_PE_STATE_ACTIVE;
	pes->pe_identifer = id;
	pes->reglife = htonl(pe->registration_life);
	/* pull/check transport */
	at = (uint8_t *)&pe->user_transport;
	
	siz = ntohs(pe->user_transport.param_length);
	calc = (uint8_t *)pe;
	calc += siz;
	if(calc >= limit) {
		/* The parameter does not fit */
		fprintf(stderr, "Mis-sized transport parameter %d size:%d limit %x start:%x - skipped\n",
			transport,
			ntohs(pe->user_transport.param_length), 
			(u_int)limit,
			(u_int)&pe->user_transport);
		goto destroy_it;
	}

	ph = get_next_parameter (at, limit, &nxt, &transport);
	if(ph == NULL) {
		fprintf(stderr, "Can't get a parameter out of the user transport\n");
		goto destroy_it;
	}
	pes->protocol_type = transport;
	switch(transport) {
	case RSP_PARAM_SCTP_TRANSPORT:
	{
		uint8_t *local_at, *local_nxt, *local_limit;
		uint16_t local_type;
		size_t len;
		sctp = (struct rsp_sctp_transport *)&pe->user_transport;
		/* sanity check */
		pes->transport_use = ntohs(sctp->transport_use);
		pes->port = udp->port;
		/* special handling needed for SCTP */
		pes->addrList = NULL;
		pes->port = sctp->port;
		/* we set the local limit to nxt, the next param */
		local_limit = nxt;
		/* set the local_at to first transport address */
		local_at = (uint8_t *)&sctp->address[0];
		pes->number_of_addr = 0;
		ph = get_next_parameter (local_at, local_limit, &local_nxt, &local_type);
		while(ph) {
			len = pes->len;
			pes->addrList = asap_pull_and_alloc_an_address(&pes->addrList, 
								       local_type,
								       (union rsp_address_union *)ph, 
								       pes->port, &pes->len);
			if(pes->len > len)
				pes->number_of_addr++;
			local_at = local_nxt;
			ph = get_next_parameter (local_at, local_limit, &local_nxt, &local_type);			
		}
	
	}
	break;
	case RSP_PARAM_TCP_TRANSPORT:
		tcp = (struct rsp_tcp_transport *)&pe->user_transport;
		pes->transport_use = ntohs(tcp->transport_use);
		pes->port = tcp->port;
		pes->number_of_addr = 1;
		ttype = ntohs(tcp->address.ph.param_type);
		pes->addrList = asap_pull_and_alloc_an_address(NULL, ttype, &tcp->address, pes->port, &pes->len);
		if(pes->addrList == NULL) {
			fprintf(stderr,"Unknown TCP transport address type %d - ignoring PE\n",
				ttype);
			goto destroy_it;
		}
		/* enter it in the hash */
		
		break;
	case RSP_PARAM_UDP_TRANSPORT:
		udp = (struct rsp_udp_transport *)&pe->user_transport;
		pes->transport_use = 0x0000;
		pes->number_of_addr = 1;
		pes->port = udp->port;
		pes->addrList = NULL;
		ttype = ntohs(udp->address.ph.param_type);
		pes->addrList = asap_pull_and_alloc_an_address(NULL, ttype, &udp->address, pes->port, &pes->len);
		if (pes->addrList == NULL) {
			fprintf(stderr,"Unknown UDP transport address type %d - ignoring PE\n",
				ttype);
			goto destroy_it;
		}
		/* enter it in the hash */

		break;
	default:
		fprintf(stderr, "Unknown transport parameter type %d? -- ignoring PE\n", transport);
	destroy_it:
		if(pes->addrList)
			free(pes->addrList);
		dlist_destroy(pes->failover_list);
		free(pes);
		return(NULL);
	};
        /* nxt points to member selection policy */
	at = nxt;
	ph = get_next_parameter (at, limit, &nxt, &ptype);
	if ((ph == NULL) || (ptype != RSP_PARAM_SELECT_POLICY)) {
		fprintf(stderr, "Expected a Member Selection policy and found %d? - ignoring PE\n",
			ptype);
		goto destroy_it;
	}
	/* Now we must get the policy.
	 */
	policy = (struct rsp_select_policy  *)ph;
	switch(policy->policy_type) {
	case RSP_POLICY_ROUND_ROBIN:
		pes->policy_value = 0;
		pes->policy_actvalue = 0;
		break;
	case RSP_POLICY_LEAST_USED:
	case RSP_POLICY_LEAST_USED_DEGRADES:
	case RSP_POLICY_WEIGHTED_ROUND_ROBIN:
		pes->policy_value = ntohl(((struct rsp_select_policy_value *)policy)->policy_value);
		pes->policy_actvalue = pes->policy_value;
		break;
	default:
		pes->policy_value = 0;
		pes->policy_actvalue = 0;
		fprintf(stderr, "Unknown policy type %d - set to default values for RR\n", policy->policy_type);
		break;
	}
	
	/* 
	 * Now we must get the actual ASAP transport out too. But
	 * I don't see the value, so I won't for now. FIX ME FIX ME!
	 * maybe when I do enrp I will get this out.
	 */
	pool->refcnt++;
	/* Now if we need to place it in sctp address cache to pe 
	 * this is used for a reverse PE lookup if we are the PE
	 * and receive a message from a PU that is also a PE, we
	 * can find the guy with this, also we can associate
	 * responses with the right guy.
	 */
	if(pes->protocol_type == RSP_PARAM_SCTP_TRANSPORT) {
		sa = pes->addrList;
		for (i=0; i<pes->number_of_addr; i++) {
			if(HashedTbl_enter(scp->ipaddrPortHash, sa, pes, sa->sa_len)) {
				fprintf(stderr, "Cross ref in hash ipadd/port to pe fails to enter\n");
			}
			sa = (struct sockaddr *)((char *)sa + sa->sa_len);
		}
	}
	return(pes);
}

void
asap_decode_pe_entry_and_update(struct rsp_enrp_scope *scp, 
				struct rsp_pool *pool,
				struct rsp_pool_element *pe,
				uint8_t *limit,
				struct rsp_pool_ele *pes
				)
{
	/* Ok, here we have a pes. The easiest
	 * way to do this is to build a new one,
	 * and then save off the important things from the
	 * old.. and then remove the old.
	 */
	struct sockaddr *sa;
	struct rsp_pool_ele *new_pes;
	void *v;
	int i;

	/* First un-hash the old SCTP addresses if relevant */
	if(pes->protocol_type == RSP_PARAM_SCTP_TRANSPORT) {
		sa = pes->addrList;
		for (i=0; i<pes->number_of_addr; i++) {
			v = HashedTbl_remove(scp->ipaddrPortHash, sa, sa->sa_len, NULL);
			sa = (struct sockaddr *)((char *)sa + sa->sa_len);
		}
	}
	new_pes = asap_build_pool_element(scp, pool, pe, limit, pes->pe_identifer);
	if(new_pes == NULL) {
		/* do nothing since we can't build a new one */
		return;
	}
	new_pes->policy_actvalue = pes->policy_actvalue;
	new_pes->asocid = pes->asocid;
	dlist_reset(pes->failover_list);
	while ((v = dlist_getNext(pes->failover_list)) != NULL) {
		dlist_append(new_pes->failover_list, v);
	}
	asap_destroy_pe(pes, 0);
}

void
asap_decode_pe_entry_and_add(struct rsp_enrp_scope *scp, 
			     struct rsp_pool *pool,
			     struct rsp_pool_element *pe,
			     uint8_t *limit)
{
	struct rsp_pool_ele *pes;
	uint32_t id;	
	/*
	 * 1. See if pe is already in, if so just mark it to present.
	 * 2. If not in, build a new one from this.
	 */

	id = ntohl(pe->id);

	/* Note: pe-identifer unique to pool <OR>
	 *       unique to pool + home server?
	 *       Maybe homeserver id should always be 0 from ENRP to PU/PE?
	 */
	dlist_reset(pool->peList);
	while((pes = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
		if(pes->pe_identifer == id) {
			pes->state &= ~RSP_PE_STATE_BEING_DEL;
			/* Call the update routine to get new entries etc */
			asap_decode_pe_entry_and_update(scp, pool, pe, limit, pes);
			return;
		}
	}
	pes = asap_build_pool_element(scp, pool, pe, limit, id);
	if (pes) {
		/* add it */
		dlist_append(pool->peList, pes);
	}
}



void
asap_handle_name_resolution_response(struct rsp_enrp_scope *scp, 
				     struct rsp_enrp_entry *enrp, 
				     uint8_t *buf, 
				     ssize_t sz, 
				     struct sctp_sndrcvinfo *sinfo)
{
	struct rsp_pool_handle *ph;
	struct rsp_select_policy *sp;
	struct rsp_pool *pool;
	struct rsp_pool_element *pe;
	struct rsp_pool_ele *pes;
	uint8_t *limit, *at, new_ent=0;
	struct rsp_enrp_req *req;
	struct rsp_timer_entry *tme;
	uint16_t this_param;
	int pool_nm_len;

	/* at all times our pointer must be less than limit */
	limit = (buf + sz);
	at = buf + sizeof(struct asap_message);

	/* Now we must first we get the pool handle parameter */
	ph  = (struct rsp_pool_handle *)get_next_parameter(at, limit, &at, &this_param);
	if ((this_param != RSP_PARAM_POOL_HANDLE) || (ph == NULL)) {
		fprintf(stderr, "Did not find first req param, pool handle in msg found %d\n",
			this_param);
		return;
	}

	/* Now we get the selection policy */
	sp = (struct rsp_select_policy *)get_next_parameter(at, limit, &at, &this_param);
	if ((sp == NULL) || (this_param != RSP_PARAM_SELECT_POLICY)) {
		fprintf(stderr, "Did not find second req param, selection policy in msg found %d\n",
			this_param);

		return;
	}
	/* Now we must:
	 * 1: find the pool.	   
	 * 2: if it does not exist build it.
	 * 3: once we have the pool, go through all
	 *    the pe's and load up the data structures, pruning
	 *    out guys that were removed (if we had a previous ds)..
	 * 4: If there is a blocking hanger(s) on this pool, wake it/them up.
	 */
	pool_nm_len = ph->ph.param_length - sizeof(struct rsp_paramhdr);

	pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,ph->pool_handle, pool_nm_len, NULL);
	if(pool == NULL) {
		/* new entry - point 2*/
		new_ent = 1;
		pool = (struct rsp_pool *)malloc(sizeof(struct rsp_pool));
		if(pool == NULL) {
			/* no memory */
			return;
		}
		memset(pool, 0, sizeof(struct rsp_pool));
		pool->scp = scp;
		pool->name_len = pool_nm_len;
		pool->name = malloc(pool_nm_len + 1);
		if(pool->name == NULL) {
			/* no memory */
			free(pool);
			return;
		}
		memset(pool->name, 0, (pool_nm_len + 1));
		memcpy(pool->name, ph->pool_handle, pool->name_len);
		pool->peList = dlist_create();
		if(pool->peList == NULL) {
			/* no memory */
		failed:
			free(pool->name);
			free(pool);
			return;
		}
		/* The cookie stuff is inited to NULL/0 len */
		pool->refcnt = 1;

		/* we don't worry about the value, it only
		 * has meaning on the individual PE's
		 */
		pool->regType = sp->policy_type;
		if(HashedTbl_enter(scp->cache, pool->name, pool, pool->name_len) != 0) {
			fprintf(stderr, "Can't add entry to global name cache?\n");
			dlist_destroy(pool->peList);
			goto failed;
		}
	}
	/* First lets mark any PE in the list currently to being deleted */
	dlist_reset(pool->peList);
	while((pes = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
		pes->state |= RSP_PE_STATE_BEING_DEL;
	}
	/* From here on down we have a pool, we are to point 3. */
	pe = (struct rsp_pool_element *)get_next_parameter(at, limit, &at, &this_param);
	while (pe != NULL) {
		if (this_param !=  RSP_PARAM_POOL_ELEMENT) {
			fprintf(stderr,"Wanted PE-Handle found type:%d - corrupt/malformed msg??\n",
				this_param);
		}
		/* note here that at becomes the new limit with in this message */
		asap_decode_pe_entry_and_add(scp, pool, pe, at);
		pe = (struct rsp_pool_element *)get_next_parameter(at, limit, &at, &this_param);
	}

	/* now we must stop any timers and wake any sleepers */
	tme = asap_find_req(scp, pool->name, pool->name_len, ASAP_REQUEST_RESOLUTION);
	if(tme == NULL) {
		fprintf (stderr, "Error, can't find resolve request for pool\n");
		return;
	}
	req = tme->req;
	/* stop timer, which wakes everyone */
	rsp_stop_timer(tme);

	/* free the request */
	rsp_free_req(req);
}


struct rsp_enrp_entry *
rsp_find_enrp_entry_by_asocid(struct rsp_enrp_scope *scp, sctp_assoc_t asoc_id)
{
	struct rsp_enrp_entry *re = NULL;
	if(asoc_id == 0) {
		return (re);
	}
	dlist_reset(scp->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(scp->enrpList)) != NULL) {
		if(re->asocid == asoc_id) 
			break;
	}
	return(re);
}

struct rsp_enrp_entry *
rsp_find_enrp_entry_by_addr(struct rsp_enrp_scope *scp, 
			    struct sockaddr *from, 
			    socklen_t  from_len)
{
	struct rsp_enrp_entry *re = NULL;
	struct sockaddr *at;
	int i;

	dlist_reset(scp->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(scp->enrpList)) != NULL) {
		at = re->addrList;
		for(i=0; i<re->number_of_addresses; i++) {
			if(at->sa_family == from->sa_family) {
				/* potential */
				if(at->sa_family == AF_INET) {
					if((((struct sockaddr_in *)from)->sin_addr.s_addr) ==
					   (((struct sockaddr_in *)at)->sin_addr.s_addr))
						/* found */
						return(re);
				} 
				else if(at->sa_family == AF_INET6) {
					if(IN6_ARE_ADDR_EQUAL(&((struct sockaddr_in6 *)from)->sin6_addr,
							      &((struct sockaddr_in6 *)at)->sin6_addr)) {
						/* found */
						return(re);
					}
				}
			}
			/* Move to next address */
			at = (struct sockaddr *)(((caddr_t)at) + from->sa_len);
		}
	}
	return(re);

}

void
handle_enrpserver_notification (struct rsp_enrp_scope *scp, char *buf, 
				struct sctp_sndrcvinfo *sinfo, ssize_t sz,
				struct sockaddr *from, socklen_t from_len)
{
	union sctp_notification *notify;
	struct sctp_send_failed  *sf;
	struct sctp_assoc_change *ac;
	struct sctp_shutdown_event *sh;
	struct rsp_enrp_entry *enrp;

	notify = (union sctp_notification *)buf;
	switch(notify->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		ac = &notify->sn_assoc_change;
		if(sz < sizeof(*ac)) {
			fprintf(stderr, "Event ac size:%d got:%d -- to small\n",
				sizeof(*ac), sz);
			return;
		}
		switch(ac->sac_state) {
		case SCTP_RESTART:
		case SCTP_COMM_UP:
			/* Find this guy and mark it up */
			enrp = rsp_find_enrp_entry_by_asocid(scp, ac->sac_assoc_id);
			if (enrp == NULL) {
				enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
			}
			if(enrp == NULL) {
				/* huh? */
				return;
			}
			enrp->state = RSP_ASSOCIATION_UP;
			if(scp->homeServer == NULL) {
				/* we have a home server */
				scp->homeServer = enrp;
				scp->state &= ~RSP_SERVER_HUNT_IP;
				scp->state |= RSP_ENRP_HS_FOUND;
				rsp_stop_timer(scp->enrp_tmr);
			}
			break;
		case SCTP_COMM_LOST:
		case SCTP_CANT_STR_ASSOC:
		case SCTP_SHUTDOWN_COMP:
			/* Find this guy and mark it down */
			enrp = rsp_find_enrp_entry_by_asocid(scp, ac->sac_assoc_id);
			if (enrp == NULL) {
				enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
			}
			if(enrp == NULL) {
				/* huh? */
				return;
			}
			enrp->state = RSP_ASSOCIATION_FAILED;
			if(scp->homeServer == enrp) {
				scp->homeServer = NULL;
				if(scp->state & RSP_SERVER_HUNT_IP) {
					/* we must clear this state if present, it
					 * really should not be.
					 */
					scp->state &= ~RSP_SERVER_HUNT_IP;
				}
				rsp_start_enrp_server_hunt(scp, 1);
			}
			break;
		default:
			fprintf( stderr, "Unknown assoc state %d\n", ac->sac_state);
			break;

		break;
		};
	case SCTP_SHUTDOWN_EVENT:
		sh = &notify->sn_shutdown_event;
		if(sz < sizeof(*sh)) {
			fprintf(stderr, "Event sh size:%d got:%d -- to small\n",
				sizeof(*sh), sz);
			return;
		}
		enrp = rsp_find_enrp_entry_by_asocid(scp, sh->sse_assoc_id);
		if (enrp == NULL) {
			enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
		}
		if(enrp == NULL) {
			/* huh? */
			return;
		}
		enrp->state = RSP_ASSOCIATION_FAILED;
		if(scp->homeServer == enrp) {
			scp->homeServer = NULL;
			if(scp->state & RSP_SERVER_HUNT_IP) {
				/* we must clear this state if present, it
				 * really should not be.
				 */
				scp->state &= ~RSP_SERVER_HUNT_IP;
			}
			rsp_start_enrp_server_hunt(scp, 1);
		}
		break;
	case SCTP_SEND_FAILED:
		sf = &notify->sn_send_failed;
		if(sz < sizeof(*sf)) {
			fprintf(stderr, "Event sf size:%d got:%d -- to small\n",
				sizeof(*sf), sz);
			return;
		}
		enrp = rsp_find_enrp_entry_by_asocid(scp, sf->ssf_assoc_id);
		if(enrp == NULL) {
			enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
		}
		if(enrp == NULL) {
			/* huh? */
			return;
		}
		enrp->state = RSP_ASSOCIATION_FAILED;
		if(scp->homeServer == enrp) {
			scp->homeServer = NULL;
			if(scp->state & RSP_SERVER_HUNT_IP) {
				/* we must clear this state if present, it
				 * really should not be.
				 */
				scp->state &= ~RSP_SERVER_HUNT_IP;
			}
			rsp_start_enrp_server_hunt(scp, 1);
		}
		/* FIX ME -- Need to add stuff here to
		 * worry about retran after server hunt begins.
		 */
		break;
	default:
		fprintf(stderr, "Got event type %d I did not subscibe for?\n",
			notify->sn_header.sn_type);
		break;
	};
}

void
handle_asapmsg_fromenrp (struct rsp_enrp_scope *scp, char *buf, 
			 struct sctp_sndrcvinfo *sinfo, ssize_t sz,
			 struct sockaddr *from, socklen_t from_len)
{
	struct rsp_enrp_entry *enrp;	
	struct asap_message *msg;
	uint16_t len;
	uint16_t failed_lock = 0;

	if(sz < sizeof(struct asap_message)) {
		/* bad message */
		return;
	}

	/* Get ENRP server we are talking to please */
	enrp = rsp_find_enrp_entry_by_asocid(scp, sinfo->sinfo_assoc_id);
	if (enrp == NULL) {
		enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
	}
	if(enrp == NULL) {
		/* huh? */
		return;
	}
	msg = (struct asap_message *)buf;
	len = ntohs(msg->asap_length);
	if(len > sz) {
		/* message problem */
		fprintf(stderr, "length of message is supposed to be %d but we read %d bytes\n",
			len, sz);
		return;
	}
	/* Now what is the message it is sending us? */
	switch (msg->asap_type) {
	case ASAP_REGISTRATION_RESPONSE:
		/*
		 * Ok, now we need to figure out if we
		 * got registered ok. If yes, then we must
		 * wake our sleeper. If no, then we must
		 * setup the error condition and wake our
		 * sleeper so it can unblock and realize it
		 * failed.
		 */
 		break;
	case ASAP_DEREGISTRATION_RESPONSE:
		/* Here we can check our de-reg response, it
		 * should be ok since the ENRP server should
		 * not refuse to let us deregister. 
		 */
 		break;
	case ASAP_HANDLE_RESOLUTION_RESPONSE:
		/* Ok we must find the pending response
		 * that we have asked for and then digest the
		 * name space PE list into the cache. If we
		 * have someone blocked on this action (aka first
		 * send to a name) then we must wake the sleeper's
		 * afterwards.
		 */
		if (pthread_mutex_lock(&scp->scp_mtx)) {
			fprintf (stderr, "failed lock error, with address resolution resp:%d\n", errno);
			failed_lock = 1;
		}
		asap_handle_name_resolution_response(scp, enrp, buf, sz, sinfo);
		if(!failed_lock) {
			if (pthread_mutex_unlock(&scp->scp_mtx)) {
				fprintf (stderr,"Gak, failed ot unlock on address resolution resp:%d\n", errno);
			}
		}
		break;
	case ASAP_ENDPOINT_KEEP_ALIVE:
		/* its a keep-alive, we must declare
		 * this to be our new home server if it is not already 
		 * and send back an ASAP_ENDPOINT_KEEP_ALIVE_ACK.
		 */
		break;
	case ASAP_SERVER_ANNOUNCE:
		/* Server announce, this is usually a multi-cast
		 * msg to add to our server pool... we do not
		 * support this currently since we are configured.
		 */
		break;
	case ASAP_ERROR:
		/* Hmm, some sort of error.
		 */
 		break;
	case ASAP_ENDPOINT_UNREACHABLE:
	case ASAP_ENDPOINT_KEEP_ALIVE_ACK:
	case ASAP_REGISTRATION:
	case ASAP_DEREGISTRATION:
	case ASAP_HANDLE_RESOLUTION:
		fprintf(stderr, "Huh, some PE thinks I am a ENRP server, got msg:%d\n",
			msg->asap_type);
		break;

	case ASAP_COOKIE:
	case ASAP_COOKIE_ECHO:
	case ASAP_BUSINESS_CARD:
		fprintf(stderr, "Huh, some ENRP server is sending me a PE <-> PU message (%d)\n",
			msg->asap_type);
		break;
	default:
		fprintf(stderr, "Unknown message type %d\n", msg->asap_type);
		break;
	};
}
