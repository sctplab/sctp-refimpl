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
	      int type,
	      int leave_locked)
{
	struct rsp_timer_entry *tme=NULL;

	/*
	 * For now we look through the running timers. In
	 * theory if we have multiple scopes in a process this
	 * is a bit in-efficent and we should have a per scope
	 * list of requests. But since this is more rare IMO
	 * we will leave it with just the master timer list 
	 * looking for a entry. This can be changed later
	 * if we need to.
	 */
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
		printf("limit is %x buf is %x left %d -- DONE\n",
		       (u_int)limit, (u_int)buf, (int)((caddr_t)limit - (caddr_t)buf));
		return(NULL);
	}
	ph = (struct rsp_paramhdr *)buf;
	len = ntohs(ph->param_length);
	if((buf + len) > limit) {
		/* to long */
		return(NULL);
	}
	/* save back the len in local byte order */
	ph->param_length = len;
	*type = ntohs(ph->param_type);

	/* save back the type in local byte order */
	ph->param_type = *type;
	if(nxt) {
		*nxt = (uint8_t *)(((caddr_t)ph) + RSP_SIZE32(len));
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
			*len += sizeof(struct sockaddr_in);
		} else if (ttype == RSP_PARAM_IPV6_ADDR) {
			new_siz = old_siz + sizeof(struct sockaddr_in6); 
			new = realloc(old, new_siz);
			if(new == NULL) {
				return(*loc);
			}
			to = (struct sockaddr *)((caddr_t)new + old_siz);
			memset(to, 0, sizeof(struct sockaddr_in6));
			*len += sizeof(struct sockaddr_in6);
		} else {
			/* *len is untouched */
			return (*loc);
		}
		if(*loc) {
			*loc = (struct sockaddr *)new;
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
	/* what about assoc id hash ? */


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
asap_find_pe(struct rsp_pool *pool, uint32_t id) {
    struct rsp_pool_ele *ele ;

    dlist_reset(pool->peList);
    while ((ele = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
        if (ele->pe_identifer == id) {
            return ele;
        }
    }
    return NULL;
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
	if(calc > limit) {
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
			printf("Entry sa:%x sa->sa_len:%d family:%d\n",
			       (u_int)sa, sa->sa_len, sa->sa_family);
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
	/* what about assoc id hash ? */

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


static struct rsp_pool *
build_a_pool(struct rsp_enrp_scope *scp, const char *name, int namelen, uint32_t policy_type)
{
	struct rsp_pool *pool;

	pool = (struct rsp_pool *)malloc(sizeof(struct rsp_pool));
	if(pool == NULL) {
		/* no memory */
		return(NULL);
	}
	memset(pool, 0, sizeof(struct rsp_pool));
	pool->scp = scp;
	pool->name_len = namelen;
	pool->name = malloc(namelen + 1);
	if(pool->name == NULL) {
		/* no memory */
		free(pool);
		return(NULL);
	}
	memset(pool->name, 0, (namelen + 1));
	memcpy(pool->name, name, namelen);
	pool->peList = dlist_create();
	if(pool->peList == NULL) {
		/* no memory */
	failed:
		free(pool->name);
		free(pool);
		return(NULL);
	}

	if (pool->next_peid == 0) {
	    /* initialize it with a random number */
	    srandomdev();
	    pool->next_peid = random();
	}

	/* The cookie stuff is inited to NULL/0 len */
	pool->refcnt = 1;

	/* we don't worry about the value, it only
	 * has meaning on the individual PE's
	 */
	pool->state = RSP_POOL_STATE_RESPONDED;
	pool->regType = policy_type;
	if(HashedTbl_enter(scp->cache, pool->name, pool, pool->name_len) != 0) {
		fprintf(stderr, "Can't add entry to global name cache?\n");
		dlist_destroy(pool->peList);
		goto failed;
	}
	return(pool);
}

struct rsp_pool_ele *
asap_create_pool_ele (struct rsp_enrp_scope *scp, const char *name, size_t namelen,
                      struct rsp_register_params *params)
{

    struct rsp_pool_ele *pes;
    struct rsp_pool *pool;

    pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache, name, namelen, NULL);
    if (pool == NULL) {
        printf("Building the pool for %s\n", name);
        pool = build_a_pool(scp, name, namelen, params->policy);
        if (pool == NULL) {
            return NULL;
        }
    }
    printf("Pool is %x\n", (u_int)pool);


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
    pes->state = RSP_PE_STATE_INACTIVE;
    pes->pe_identifer = ++(pool->next_peid);
    pes->protocol_type = params->protocol_type;
    pes->reglife = params->reglife; // FIX

    /* Simply Copy the addressess */
    pes->addrList = malloc(params->cnt_of_addr * params->socklen);
    if (pes->addrList == NULL) {
        dlist_destroy(pes->failover_list);
        free(pes);
        return(NULL);
    }
    pes->number_of_addr = params->cnt_of_addr;
    pes->len = params->socklen;
    memcpy(pes->addrList, params->taddr, params->cnt_of_addr * params->socklen);
    if (pes->addrList->sa_family == AF_INET) {
        pes->port = ((struct sockaddr_in*)pes->addrList)->sin_port;
    } else if(pes->addrList[0].sa_family == AF_INET6) {
        pes->port = ((struct sockaddr_in6*)pes->addrList)->sin6_port;
    } else {
        dlist_destroy(pes->failover_list);
        free(pes);
        printf("\nUnknown address family");
        return(NULL);
    }

    pes->transport_use = params->transport_use;

    /* policy FIX*/
    pes->policy_value = params->policy;
    pes->policy_actvalue = params->policy_value;

    /* add it */
    dlist_append(pool->peList, pes);

    return pes;
}

void
handle_response_op_errorx(struct rsp_enrp_scope *scp,
        struct rsp_timer_entry *tme,
        struct rsp_pool *pool,
        struct rsp_operational_error *op_error) {
    int len;
    struct rsp_enrp_req *req = tme->req;

    /* update the req with error cause code and string */
    req->cause.ccode = ntohs(op_error->cause[0].code);
    len = ntohs(op_error->cause[0].length);

    /*
     * the cause code and optional cause TLV needs to be returned to the caller
     * Store it in the req structure. retrieve it after the rsp_internal_poll from the req
     * and store it in the user supplied data structure.
     */
    if (len > 4) {
        /* Has an optional cause variable length string */
        req->cause.olen = (len-4); /* -cause header + 1 byte to store \0 */
        req->cause.ocause = malloc(req->cause.olen);
        memcpy(req->cause.ocause, op_error->cause[0].ocause, req->cause.olen);
    } else {
        /* No optional cause header */
        req->cause.olen = 0;
    }

    /* now we must stop any timers and wake any sleepers */
    /* stop timer, which wakes everyone */
    rsp_stop_timer(tme);
    return; /* basically returns to the caller waiting for response */

}
void
handle_response_op_error(struct rsp_enrp_scope *scp,
			 struct rsp_enrp_entry *enrp,
			 struct rsp_pool_handle *ph,
			 struct rsp_operational_error *op)
{
	int pool_nm_len;
	struct rsp_pool *pool;

	pool_nm_len = ph->ph.param_length - sizeof(struct rsp_paramhdr);

	pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,ph->pool_handle, pool_nm_len, NULL);
	if(pool == NULL) {
		/* no one waiting for it? */
		return;
	}
	pool->state = RSP_POOL_STATE_NOTFOUND;

}

void
asap_handle_registration_response(struct rsp_enrp_scope *scp,
        struct rsp_enrp_entry *enrp,
        uint8_t *buf,
        ssize_t sz,
        struct sctp_sndrcvinfo *sinfo)
{
    struct rsp_pool_handle *ph;
    struct rsp_pe_identifier *pe_id;
    struct rsp_operational_error *op_error;

    struct rsp_pool *pool;
    uint8_t *limit, *at, *bu;
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
    bu = at;

    pool_nm_len = ph->ph.param_length - sizeof(struct rsp_paramhdr);

    pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache, ph->pool_handle, pool_nm_len, NULL);
    if (pool == NULL) {
        fprintf(stderr, "pool %s not found\n", ph->pool_handle);
        return;
    }
    printf("Pool is %x\n", (u_int)pool);

    /* mark time of update */
    gettimeofday(&pool->received, NULL);
    pool->state = RSP_POOL_STATE_RESPONDED;

    /* Now we must first we get the pe identifier parameter */
    pe_id  = (struct rsp_pe_identifier *)get_next_parameter(at, limit, &at, &this_param);
    if ((this_param != RSP_PARAM_PE_IDENTIFIER) || (pe_id == NULL)) {
        fprintf(stderr, "Did not find required param, pool identifier in msg found %d\n",
                this_param);
        return;
    }

    tme = asap_find_req(scp, pool->name, pool->name_len, ASAP_REQUEST_REGISTRATION, 0);
    if(tme == NULL) {
        fprintf (stderr, "Error, can't find resolve request for pool\n");
        return;
    }
    req = tme->req;

    /* Check if operation error is present */
    if (at < limit) {
        op_error = (struct rsp_operational_error*)get_next_parameter(at, limit, &at, &this_param);
        if ((this_param != RSP_PARAM_OPERATION_ERROR) || op_error == NULL) {
            fprintf(stderr, "Did not find required param, pool identifier in msg found %d\n",
                    this_param);
        }
        handle_response_op_errorx(scp, tme, pool, op_error);
        return;
    } else {
        /* No error. Registered ok */

    }


    /* now we must stop any timers and wake any sleepers */
    /* stop timer, which wakes everyone */
    rsp_stop_timer(tme);
    req->resolved = 1;
    printf("Registered now\n");

}

void
asap_handle_deregistration_response(struct rsp_enrp_scope *scp,
        struct rsp_enrp_entry *enrp,
        uint8_t *buf,
        ssize_t sz,
        struct sctp_sndrcvinfo *sinfo)
{
    struct rsp_pool_handle *ph;
    struct rsp_pe_identifier *pe_id;
    struct rsp_operational_error *op_error;

    struct rsp_pool *pool;
    uint8_t *limit, *at, *bu;
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
    bu = at;

    pool_nm_len = ph->ph.param_length - sizeof(struct rsp_paramhdr);

    pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache, ph->pool_handle, pool_nm_len, NULL);
    if (pool == NULL) {
        fprintf(stderr, "pool %s not found\n", ph->pool_handle);
        return;
    }
    printf("Pool is %x\n", (u_int)pool);

    /* mark time of update */
    gettimeofday(&pool->received, NULL);
    pool->state = RSP_POOL_STATE_RESPONDED;

    /* Now we must first we get the pe identifier parameter */
    pe_id  = (struct rsp_pe_identifier *)get_next_parameter(at, limit, &at, &this_param);
    if ((this_param != RSP_PARAM_PE_IDENTIFIER) || (pe_id == NULL)) {
        fprintf(stderr, "Did not find required param, pool identifier in msg found %d\n",
                this_param);
        return;
    }

    /* now we must stop any timers and wake any sleepers */
    tme = asap_find_req(scp, pool->name, pool->name_len, ASAP_REQUEST_DEREGISTRATION, 0);
    if(tme == NULL) {
        fprintf (stderr, "Error, can't find resolve request for pool\n");
        return;
    }
    req = tme->req;

    /* Check if operation error is present */
    if (at < limit) {
        op_error = (struct rsp_operational_error*)get_next_parameter(at, limit, &at, &this_param);
        if ((this_param != RSP_PARAM_OPERATION_ERROR) || op_error == NULL) {
            fprintf(stderr, "Did not find required param, pool identifier in msg found %d\n",
                    this_param);
        }
        handle_response_op_errorx(scp, tme, pool, op_error);
        return;
    } else {
        /* No error. DeRegistered ok */
        /* Remove the pe from the pool FIX*/
        struct rsp_pool_ele *pes;
        pes = asap_find_pe(pool, ntohl(pe_id->id));
        if (pes)
            asap_destroy_pe(pes, 1);
    }



    /* stop timer, which wakes everyone */
    rsp_stop_timer(tme);
    req->resolved = 1;
    printf("DeRegistered now\n");

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
	struct rsp_pool_ele *pes;
	struct rsp_pool_element *pe;
	int pool_nm_len;
	uint8_t *limit, *at, *bu, new_ent=0;
	struct rsp_enrp_req *req;
	struct rsp_timer_entry *tme;
	uint16_t this_param;
	uint8_t regType;
	uint8_t overall_sp = 1;

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
	bu = at;
	sp = (struct rsp_select_policy *)get_next_parameter(at, limit, &at, &this_param);
	if (sp && (this_param == RSP_PARAM_SELECT_POLICY)) {

		regType = sp->policy_type;
	}else if(sp && (this_param == RSP_PARAM_OPERATION_ERROR)) {
			handle_response_op_error(scp, enrp, ph, (struct rsp_operational_error *)sp); 
			return;
	} else {
		/* reselect */
		regType = RSP_POLICY_ROUND_ROBIN;

		/* the Overall Selection Policy is an optional parameter */
	    overall_sp = 0;
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

	pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache, ph->pool_handle, pool_nm_len, NULL);
	if(pool == NULL) {
		/* new entry - point 2*/
		new_ent = 1;
		printf("Building the pool for %s\n", ph->pool_handle);
		pool = build_a_pool (scp, (char *)ph->pool_handle, pool_nm_len, (uint32_t)sp->policy_type);
		if(pool == NULL)
			return;
	}
	printf("Pool is %x\n", (u_int)pool);
	pool->regType = regType;
	/* mark time of update */
	gettimeofday(&pool->received, NULL);
	pool->state = RSP_POOL_STATE_RESPONDED;
	/* First lets mark any PE in the list currently to being deleted */
	dlist_reset(pool->peList);
	while((pes = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
		pes->state |= RSP_PE_STATE_BEING_DEL;
	}
	/* From here on down we have a pool, we are to point 3. */
	if (overall_sp) {
	    pe = (struct rsp_pool_element *)get_next_parameter(at, limit, &at, &this_param);
	} else {
		/* Since the Overall Selection Policy parameter was not sent, 
		 * sp points to the next parameter which is pe */
		pe = (struct rsp_pool_element *)sp;
	}
	while (pe != NULL) {
		if (this_param !=  RSP_PARAM_POOL_ELEMENT) {
			fprintf(stderr,"Wanted PE-Handle found type:%d - corrupt/malformed msg??\n",
				this_param);
		}
		/* note here that at becomes the new limit with in this message */
		printf("adding pe:%x id:%x\n", (u_int)pe, ntohl(pe->id));
		asap_decode_pe_entry_and_add(scp, pool, pe, at);
		pe = (struct rsp_pool_element *)get_next_parameter(at, limit, &at, &this_param);
	}
	/* now we must stop any timers and wake any sleepers */
	tme = asap_find_req(scp, pool->name, pool->name_len, ASAP_REQUEST_RESOLUTION, 0);
	if(tme == NULL) {
		fprintf (stderr, "Error, can't find resolve request for pool\n");
		return;
	}
	req = tme->req;
	/* stop timer, which wakes everyone */
	rsp_stop_timer(tme);
	req->resolved = 1;
	printf("Resolved now\n");
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
		if(re->asocid == asoc_id) {
			printf("found him\n");
			break;
		}
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
rsp_retry_after_sh(struct rsp_timer_entry *te)
{
	switch(te->timer_type) {
	case RSP_T1_ENRP_REQUEST:
		/* resend the ENRP request and
		 * restart timer 
		 */
		rsp_send_enrp_req(te->sdata, te->req);
		rsp_start_timer(te->scp, te->sdata, te->sdata->timers[RSP_T1_ENRP_REQUEST], 
				te->req, RSP_T1_ENRP_REQUEST, &te);
		break;
	case RSP_T2_REGISTRATION:
		/* resend the reg request and
		 * restart timer FIX ME (yet to do)
		 */
		break;
	case RSP_T3_DEREGISTRATION:
		/* resend the dereg request and
		 * restart timer FIX ME (yet to do)
		 */
		break;
	case RSP_T4_REREGISTRATION:
		/* resend the reg request and
		 * restart any timer. FIX ME (yet to do)
		 */
		break;
	case RSP_T5_SERVERHUNT:
		/* should not happen */
		fprintf(stderr, "Saw on retry a SH?\n");
		break;
	case RSP_T6_SERVERANNOUNCE:
		fprintf(stderr, "Saw on retry a server announce?\n");
		break;
	case RSP_T7_ENRPOUTDATE:
		/* don't care if cache is out of date */
		break;
	default:
		fprintf(stderr, "Saw on retry an unknown type %d?\n",
			te->timer_type);
		break;
	}
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
	printf("Handle enrp_server notify %d\n",
	       notify->sn_header.sn_type);
	switch(notify->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		printf("Assoc change\n");
		ac = &notify->sn_assoc_change;
		if(sz < sizeof(*ac)) {
			fprintf(stderr, "Event ac size:%d got:%d -- to small\n",
				sizeof(*ac), sz);
			return;
		}
		printf("what type:%d\n", ac->sac_state);
		switch(ac->sac_state) {
		case SCTP_RESTART:
		case SCTP_COMM_UP:
			/* Find this guy and mark it up */
			printf("commup/restart id:%x\n", ac->sac_assoc_id);
			enrp = rsp_find_enrp_entry_by_asocid(scp, ac->sac_assoc_id);
			if (enrp == NULL) {
				printf("Try by addr\n");
				enrp = rsp_find_enrp_entry_by_addr(scp, from, from_len);
			}
			if(enrp == NULL) {
				/* huh? */
				printf("Hmm no enrp id found\n");
				return;
			}
			printf("Found enrp:%x setting up\n", (u_int)enrp);
			enrp->state = RSP_ASSOCIATION_UP;
			if(scp->homeServer == NULL) {
				/* we have a home server */
				struct rsp_timer_entry *te;
				printf("Setting home server to this one for scp:%x\n", (u_int)scp);
				scp->homeServer = enrp;
				scp->state &= ~RSP_SERVER_HUNT_IP;
				scp->state |= RSP_ENRP_HS_FOUND;
				rsp_stop_timer(scp->enrp_tmr);
				te = scp->enrp_tmr->chained_next;
				while(te) {
					/* unwind and restart any guys waiting on this to finish */
					rsp_retry_after_sh(te);
					te = te->chained_next;
				}
			}
			printf("Finshed with msg\n");
			break;
		case SCTP_COMM_LOST:
		case SCTP_CANT_STR_ASSOC:
		case SCTP_SHUTDOWN_COMP:
			/* Find this guy and mark it down */
			printf("comm lost\n");
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
				rsp_start_enrp_server_hunt(scp);
			}
			break;
		default:
			fprintf( stderr, "Unknown assoc state %d\n", ac->sac_state);
			break;

		};
		printf("At break\n");
		break;
	case SCTP_SHUTDOWN_EVENT:
		printf("Shutdown?\n");
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
			rsp_start_enrp_server_hunt(scp);
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
			rsp_start_enrp_server_hunt(scp);
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
	printf("Done with ENRP server notify process\n");
}

void
rsp_send_enrp_req(struct rsp_socket *sd,
		  struct rsp_enrp_req *req)
{
	/* Pick the home ENRP server and send the mesg */
	struct rsp_enrp_entry *hs;
	struct sctp_sndrcvinfo sinfo;
	struct rsp_enrp_scope *scp;

	int ret, cnt=0;

	scp = sd->scp;
	printf("scp:%x need to send a request hs:%x\n", (u_int)scp,
	       (u_int)scp->homeServer);
	if(scp->homeServer == NULL) {
		while(scp->homeServer == NULL) {
			cnt++;
			ret = rsp_internal_poll((nfds_t)(rsp_pcbinfo.num_fds), 1000, 1);
		}
		return;
	}
	hs = scp->homeServer;
	printf("hs is %x\n",(u_int)hs);
	if (hs->state != RSP_ASSOCIATION_UP) {
		printf("No HS up?\n");
		return;
	}

	if (hs->asocid == 0) {
		printf("HS has no asoc id?\n");
		return;
	}
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.sinfo_assoc_id = hs->asocid;
	sinfo.sinfo_flags = SCTP_UNORDERED;
	sinfo.sinfo_ppid = htonl(RSERPOOL_ASAP_PPID);
	printf("Sending to scp->sd:%d id:%x\n", scp->sd, (u_int)sinfo.sinfo_assoc_id);
	ret = sctp_send(scp->sd, req->req, req->len, &sinfo, 0);
	printf("ret is %d errno:%d\n", ret, errno);
}


void
handle_asapmsg_fromenrp (struct rsp_enrp_scope *scp, char *buf, 
			 struct sctp_sndrcvinfo *sinfo, ssize_t sz,
			 struct sockaddr *from, socklen_t from_len)
{
	struct rsp_enrp_entry *enrp;	
	struct asap_message *msg;
	uint16_t len;

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
	    asap_handle_registration_response(scp, enrp, (uint8_t *)buf, sz, sinfo);
 		break;
	case ASAP_DEREGISTRATION_RESPONSE:
		/* Here we can check our de-reg response, it
		 * should be ok since the ENRP server should
		 * not refuse to let us deregister.
		 */
	    asap_handle_deregistration_response(scp, enrp, (uint8_t *)buf, sz, sinfo);
 		break;
	case ASAP_HANDLE_RESOLUTION_RESPONSE:
		/* Ok we must find the pending response
		 * that we have asked for and then digest the
		 * name space PE list into the cache. If we
		 * have someone blocked on this action (aka first
		 * send to a name) then we must wake the sleeper's
		 * afterwards.
		 */
	  asap_handle_name_resolution_response(scp, enrp, (uint8_t *)buf, sz, sinfo);
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


int
rsp_enrp_make_name_request(struct rsp_socket *sd,
						   struct rsp_pool *pool,
						   const char *name,
						   int32_t namelen,
						   uint32_t flags
						   )

{
	int len;
	struct rsp_enrp_req *req;
	struct asap_message *msg;
	struct rsp_enrp_scope *scp;
	struct rsp_timer_entry *ote=NULL;
	struct rsp_pool_handle  *ph;

	scp = sd->scp;
	if(pool == NULL) {
		/* need to build a pool element */
		pool = build_a_pool (scp, name, namelen, 0);
		if(pool == NULL) {
			return(-1);
		}
		printf("Build a pool here (%x)\n", (u_int)pool);
	}
	if(pool->state != RSP_POOL_STATE_TIMEDOUT)
		pool->state = RSP_POOL_STATE_REQUESTED;

	len = RSP_SIZE32((sizeof(struct asap_message) + namelen + sizeof(struct rsp_pool_handle)));
	req = rsp_aloc_req(name, namelen, (void *)NULL, 0, ASAP_REQUEST_RESOLUTION);
	if(req == NULL) {
		return(-1);
	}
	req->req = malloc(len);
	if(req->req == NULL) {
		rsp_free_req(req);
		return(-1);
	}
	req->len = len;
	msg = (struct asap_message *)req->req;
	msg->asap_type = ASAP_HANDLE_RESOLUTION;
	msg->asap_length = htons(req->len);

	ph = (struct rsp_pool_handle *)((caddr_t)msg + sizeof(*msg));
	ph->ph.param_type = htons(RSP_PARAM_POOL_HANDLE);
	ph->ph.param_length = htons(namelen+sizeof(struct rsp_pool_handle));
	memcpy(ph->pool_handle, name, namelen);

	/* Now we have a fully formed req, we do
	 * one of two things.
	 *
	 * A) If there is a H-ENRP-S, start timer
	 *    and send off the request.
	 *
	 * B) If no H-ENRP-S, find serverhunt, if happening,
	 *    and then chain to end of that.
	 *
	 * In either case we will then run the select loop
	 * setting it up to return when a message is received
	 * from ENRP as well.
	 */
	if(scp->state & RSP_ENRP_HS_FOUND) {
		/* we have a HS */
		;
	} else {
		/* Server Hunt may be IP */
		if((scp->state & RSP_SERVER_HUNT_IP) == 0)
			rsp_start_enrp_server_hunt(scp);

		/* Block getting a server */
		while((scp->state & RSP_ENRP_HS_FOUND) == 0) {
			rsp_internal_poll(rsp_pcbinfo.num_fds, INFTIM, 1);
		}
	}
	/* Now we must send off the request */
	rsp_send_enrp_req(sd, req);
	rsp_start_timer(scp,sd, sd->timers[RSP_T1_ENRP_REQUEST], req, RSP_T1_ENRP_REQUEST, &ote);
	if(ote == NULL) {
		fprintf(stderr, "Timer fails to start, enrp-request!\n");
	}

	/* For a thread safe library we probably
	 * need to change INFTIM into a set amount.
	 * Then check every so often to see if another
	 * thread has gotten my response and thus we
	 * would be done.
	 */
	if((flags & MSG_DONTWAIT) == 0) {
	  while(pool->state == RSP_POOL_STATE_REQUESTED) {
		rsp_internal_poll(rsp_pcbinfo.num_fds, INFTIM, 1);
	  }
	}
	free(req->req);
	req->req = NULL;
	rsp_free_req(req);
	if(ote) {
		rsp_stop_timer(ote);
		free(ote);
	}
	printf("returning to caller, we have the pool now\n");
	return(0);
}

void
rsp_create_pool_handle_param(void *req, char *name, int32_t namelen, uint32_t *nlen)
{
    struct rsp_pool_handle *phandle;
    phandle = (struct rsp_pool_handle *)req;
    phandle->ph.param_type = htons(RSP_PARAM_POOL_HANDLE);
    memcpy(phandle->pool_handle, name, namelen);
    *nlen = namelen + sizeof(struct rsp_pool_handle);
    phandle->ph.param_length = htons(*nlen);
}

void
rsp_create_pe_identifier_param(void *req, uint32_t peid, uint32_t *nlen)
{
    struct rsp_pe_identifier *pe_identifier;
    pe_identifier = (struct rsp_pe_identifier *)req;
    pe_identifier->ph.param_type = htons(RSP_PARAM_PE_IDENTIFIER);
    pe_identifier->id = htonl(peid);
    *nlen = sizeof(struct rsp_pe_identifier);
    pe_identifier->ph.param_length = htons(*nlen);
}

int
rsp_create_pool_element_param(void *req, struct rsp_pool_ele *pes, uint32_t *nlen)
{
    struct rsp_pool_element *pe;

    uint16_t tlen;

    pe = (struct rsp_pool_element *)req;
    pe->ph.param_type = htons(RSP_PARAM_POOL_ELEMENT);
    pe->id = htonl(pes->pe_identifer);
    pe->home_enrp_id = htonl(0); // FIX
    pe->registration_life = htonl(pes->reglife);
    switch (pes->protocol_type ) {
        case RSP_PARAM_SCTP_TRANSPORT: {
            struct rsp_sctp_transport *sctp;
            int i=0;
            tlen = sizeof(struct rsp_sctp_transport);
            sctp = (struct rsp_sctp_transport *)&pe->user_transport;
            sctp->ph.param_type = htons(RSP_PARAM_SCTP_TRANSPORT);
            sctp->port = pes->port; /* Already in network byte order */
            sctp->transport_use = htons(pes->transport_use);

            for (i=0;i<pes->number_of_addr;++i) {
                uint16_t atype = (pes->addrList[i].sa_family == AF_INET)?RSP_PARAM_IPV4_ADDR:RSP_PARAM_IPV6_ADDR;
                if (atype == RSP_PARAM_IPV4_ADDR) {
                    struct rsp_ipv4_address *ipv4 = (struct rsp_ipv4_address *)
                        ((void *)sctp->address + (i*sizeof(struct rsp_ipv4_address)));
                    ipv4->ph.param_type = htons(atype);
                    ipv4->ph.param_length = htons(8);
                    ipv4->in = ((struct sockaddr_in *)&pes->addrList[i])->sin_addr;
                    tlen += sizeof(struct rsp_ipv4_address);
                } else {
                    struct rsp_ipv6_address *ipv6 = (struct rsp_ipv6_address *)
                        ((void *)sctp->address + (i*sizeof(struct rsp_ipv6_address)));
                    ipv6->ph.param_type = htons(atype);
                    ipv6->ph.param_length = htons(16);
                    ipv6->in6 = ((struct sockaddr_in6 *)&pes->addrList[i])->sin6_addr;
                    tlen += sizeof(struct rsp_ipv6_address);
                }
            }
            sctp->ph.param_length = htons(tlen);
        }
        break;
        case RSP_PARAM_TCP_TRANSPORT: {
            struct rsp_tcp_transport *tcp;
            tlen = sizeof(struct rsp_tcp_transport);
            tcp = (struct rsp_tcp_transport *)&pe->user_transport;
            tcp->ph.param_type = htons(RSP_PARAM_TCP_TRANSPORT);
            tcp->port = htons(pes->port);
            tcp->transport_use = 0; /* Not significant */

            uint16_t atype = (pes->addrList->sa_family == AF_INET)?RSP_PARAM_IPV4_ADDR:RSP_PARAM_IPV6_ADDR;
            if (atype == RSP_PARAM_IPV4_ADDR) {
                tcp->address.ipv4.ph.param_type = htons(atype);
                tcp->address.ipv4.ph.param_length = htons(8);
                tcp->address.ipv4.in = ((struct sockaddr_in *)&pes->addrList)->sin_addr;
                tlen += sizeof(struct rsp_ipv4_address);
            } else {
                tcp->address.ipv6.ph.param_type = htons(atype);
                tcp->address.ipv6.ph.param_length = htons(16);
                tcp->address.ipv6.in6 = ((struct sockaddr_in6 *)pes->addrList)->sin6_addr;
                tlen += sizeof(struct rsp_ipv6_address);
            }
            tcp->ph.param_length = htons(tlen);
        }
        break;
        default:
            return (-1);
    }
    /* Add selection policy param */
    if (pes->policy_value == RSP_POLICY_ROUND_ROBIN) {
        struct rsp_select_policy *spolicy;
        spolicy = (struct rsp_select_policy*)((void *)&pe->user_transport + tlen);
        spolicy->ph.param_type = htons(RSP_PARAM_SELECT_POLICY);
        spolicy->ph.param_length = htons(8);
        spolicy->policy_type = htonl(pes->policy_value);
        *nlen = (sizeof(struct rsp_pool_element) - 4) + tlen + sizeof(struct rsp_select_policy);
    } else {
        struct rsp_select_policy_value *spolicy;
        spolicy = (struct rsp_select_policy_value *)((void *)&pe->user_transport + tlen);
        spolicy->ph.param_type = htons(RSP_PARAM_SELECT_POLICY);
        spolicy->ph.param_length = htons(12);
        spolicy->policy_type = htonl(pes->policy_value);
        spolicy->policy_value = htonl(pes->policy_actvalue);
        *nlen = (sizeof(struct rsp_pool_element) - 4) + tlen + sizeof(struct rsp_select_policy_value);
    }
    pe->ph.param_length = htons(*nlen);
    return (0);
}

uint32_t pe_paramlen(struct rsp_pool_ele *pes)
{
    uint32_t tlen = 0, pelen=0;
    int i=0;

    switch (pes->protocol_type ) {
        case RSP_PARAM_SCTP_TRANSPORT:
            tlen = sizeof(struct rsp_sctp_transport);
            for (i=0;i<pes->number_of_addr;++i) {
                uint16_t atype = (pes->addrList->sa_family == AF_INET)?RSP_PARAM_IPV4_ADDR:RSP_PARAM_IPV6_ADDR;
                if (atype == RSP_PARAM_IPV4_ADDR) {
                    tlen += sizeof(struct rsp_ipv4_address);
                } else {
                    tlen += sizeof(struct rsp_ipv6_address);
                }
            }
        break;
        case RSP_PARAM_TCP_TRANSPORT: {
            tlen = sizeof(struct rsp_tcp_transport);
            uint16_t atype = (pes->addrList->sa_family == AF_INET)?RSP_PARAM_IPV4_ADDR:RSP_PARAM_IPV6_ADDR;
            if (atype == RSP_PARAM_IPV4_ADDR) {
                tlen += sizeof(struct rsp_ipv4_address);
            } else {
                tlen += sizeof(struct rsp_ipv6_address);
            }
        }
        break;
    }
    if (pes->policy_value == RSP_POLICY_ROUND_ROBIN) {
        pelen = (sizeof(struct rsp_pool_element) - 4 ) + tlen + sizeof(struct rsp_select_policy);
    } else {
        pelen = (sizeof(struct rsp_pool_element) - 4 ) + tlen + sizeof(struct rsp_select_policy_value);
    }
    return pelen;
}

int
rsp_enrp_make_register_request(struct rsp_socket *sd, struct rsp_pool *pool,
        struct rsp_pool_ele *pes, uint32_t flags, struct asap_error_cause *cause)

{
    uint32_t len=0, nlen=0;
    struct rsp_enrp_req *req;
    struct asap_message *msg;
    struct rsp_enrp_scope *scp;
    struct rsp_timer_entry *ote=NULL;
    void *at = NULL;

    scp = sd->scp;
    if (pool == NULL || pes == NULL) {
        return(-1);
    }

    /* FIX */
    if (pool->state != RSP_POOL_STATE_TIMEDOUT)
        pool->state = RSP_POOL_STATE_REQUESTED;


    req = rsp_aloc_req(pool->name, pool->name_len, (void *)NULL, 0, ASAP_REQUEST_REGISTRATION);
    if (req == NULL) {
        return(-1);
    }

    /* FIX calculating pelen */

    /* asap msg + pool handle param*/
    len = RSP_SIZE32((sizeof(struct asap_message) + pool->name_len + sizeof(struct rsp_pool_handle)));
    len += pe_paramlen(pes);

    req->req = malloc(len);
    if(req->req == NULL) {
        rsp_free_req(req);
        return(-1);
    }
    bzero(req->req, len);
    req->len = len;
    req->cause.ccode = -1; /* NO ERROR */
    msg = (struct asap_message *)req->req;
    msg->asap_type = ASAP_REGISTRATION;
    msg->asap_length = htons(req->len);

    /* Add a pool handle parameter */
    at = ((caddr_t)msg + sizeof(*msg));
    rsp_create_pool_handle_param(at, pool->name, pool->name_len, &nlen);

    /* Add a pool element parameter */
    rsp_create_pool_element_param(at+nlen, pes, &nlen);

    /* Now we have fully formed registration request */

    /* Now we must send off the request */
    rsp_send_enrp_req(sd, req);

    rsp_start_timer(scp,sd, sd->timers[RSP_T1_ENRP_REQUEST], req, RSP_T1_ENRP_REQUEST, &ote);
    if(ote == NULL) {
        fprintf(stderr, "Timer fails to start, enrp-request!\n");
    }

    /* For a thread safe library we probably
     * need to change INFTIM into a set amount.
     * Then check every so often to see if another
     * thread has gotten my response and thus we
     * would be done.
     */
    if ((flags & MSG_DONTWAIT) == 0) {
        while(pool->state == RSP_POOL_STATE_REQUESTED) {
            rsp_internal_poll(rsp_pcbinfo.num_fds, INFTIM, 1);
        }
    }


    /* Check if there was an error.
     * retrieve the information from req and store it in the out parameter provided by the user
     */
    if (req->cause.ccode >= 0) {
        cause->ccode = req->cause.ccode;
        cause->olen = req->cause.olen;
        if (cause->olen > 0) {
            memcpy(cause->ocause, req->cause.ocause, cause->olen);
        }
    }

    rsp_free_req(req);
    if (ote) {
        rsp_stop_timer(ote);
        free(ote);
    }
    printf("returning to caller, registered\n");
    return (0);
}

int
rsp_enrp_make_deregister_request(struct rsp_socket *sd, struct rsp_pool *pool,
        struct rsp_pool_ele *pes, uint32_t flags, struct asap_error_cause *cause)
{
    uint32_t len=0, nlen=0;
    struct rsp_enrp_req *req;
    struct asap_message *msg;
    struct rsp_enrp_scope *scp;
    struct rsp_timer_entry *ote=NULL;
    void *at = NULL;

    scp = sd->scp;
    if (pool == NULL || pes == NULL) {
        return(-1);
    }

    /* FIX */
    if (pool->state != RSP_POOL_STATE_TIMEDOUT)
        pool->state = RSP_POOL_STATE_REQUESTED;

    req = rsp_aloc_req(pool->name, pool->name_len, (void *)NULL, 0, ASAP_REQUEST_DEREGISTRATION);
    if (req == NULL) {
        return(-1);
    }

    /* asap msg + pool handle param + pe identifier param*/
    len = RSP_SIZE32((sizeof(struct asap_message) + pool->name_len +
            sizeof(struct rsp_pool_handle)) + sizeof(struct rsp_pe_identifier));
    req->req = malloc(len);
    if(req->req == NULL) {
        rsp_free_req(req);
        return(-1);
    }
    bzero(req->req, len);
    req->len = len;
    msg = (struct asap_message *)req->req;
    msg->asap_type = ASAP_DEREGISTRATION;
    msg->asap_length = htons(req->len);

    /* Add a pool handle parameter */
    at = ((caddr_t)msg + sizeof(*msg));
    rsp_create_pool_handle_param(at, pool->name, pool->name_len, &nlen);

    /* Add a pool identifier param */
    rsp_create_pe_identifier_param(at+nlen, pes->pe_identifer, &nlen);

    /* Now we have fully formed registration request */
    if(scp->state & RSP_ENRP_HS_FOUND) {
        /* we have a HS */
        ;
    } else {
        /* Server Hunt may be IP */
        if((scp->state & RSP_SERVER_HUNT_IP) == 0)
            rsp_start_enrp_server_hunt(scp);

        /* Block getting a server */
        while((scp->state & RSP_ENRP_HS_FOUND) == 0) {
            rsp_internal_poll(rsp_pcbinfo.num_fds, INFTIM, 1);
        }
    }
    /* Now we must send off the request */
    rsp_send_enrp_req(sd, req);

    rsp_start_timer(scp,sd, sd->timers[RSP_T1_ENRP_REQUEST], req, RSP_T1_ENRP_REQUEST, &ote);
    if(ote == NULL) {
        fprintf(stderr, "Timer fails to start, enrp-request!\n");
    }

    /* For a thread safe library we probably
     * need to change INFTIM into a set amount.
     * Then check every so often to see if another
     * thread has gotten my response and thus we
     * would be done.
     */
    if ((flags & MSG_DONTWAIT) == 0) {
        while(pool->state == RSP_POOL_STATE_REQUESTED) {
            rsp_internal_poll(rsp_pcbinfo.num_fds, INFTIM, 1);
        }
    }

    /* Check if there was an error.
     * retrieve the information from req and store it in the out parameter provided by the user
     */

    if (req->cause.ccode >= 0) {
        cause->ccode = req->cause.ccode;
        cause->olen = req->cause.olen;
        if (cause->olen > 0) {
            memcpy(cause->ocause, req->cause.ocause, cause->olen);
        }
    }

    rsp_free_req(req);
    if (ote) {
        rsp_stop_timer(ote);
        /* free(ote); FIX uncomment and fix crash */
    }
    printf("returning to caller, deregistered\n");
    return (0);
}
struct rsp_pool_ele *
rsp_server_select(struct rsp_pool *pool)
{
	struct rsp_pool_ele *pe = NULL, *low=NULL;
	uint32_t curval;
	int cnt=0;

	/* do the server selection policy */
	switch (pool->regType) {
	default:
		fprintf(stderr, "Unknown pool policy %d, use rr\n",
			pool->regType);
	case RSP_POLICY_ROUND_ROBIN:
		pe = (struct rsp_pool_ele *)dlist_get(pool->peList);
		if(pe == NULL) {
			dlist_reset(pool->peList);
			pe = (struct rsp_pool_ele *)dlist_get(pool->peList);
		}
		break;
	case RSP_POLICY_LEAST_USED:
		dlist_reset(pool->peList);
		low = (struct rsp_pool_ele *)dlist_get(pool->peList);
		curval = low->policy_value;
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(curval > pe->policy_value) {
				curval = pe->policy_value;
			}
		}
		/* Now we have the lowest value, lets find
		 * the cnt of these.
		 */
		dlist_reset(pool->peList);
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(curval == pe->policy_value) {
				cnt++;
				if(cnt == 1) {
					/* remember first one */
					low = pe;
				}
			}
		}
		if(cnt == 1) {
			pe = low;
			break;
		}
		if(pool->lastUsed == NULL) {
			/* never given out yet */
			pe = low;
			break;
		}
		/* give out next one beyond last given out */
		dlist_reset(pool->peList);
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(pe == pool->lastUsed) {
				pe = (struct rsp_pool_ele *)dlist_get(pool->peList);
				if(pe == NULL) {
					/* recall first one */
					pe = low;
				}
				break;
			}
		}
		break;
	case RSP_POLICY_LEAST_USED_DEGRADES:
		dlist_reset(pool->peList);
		low = (struct rsp_pool_ele *)dlist_get(pool->peList);
		curval = low->policy_value;
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(curval > pe->policy_value) {
				curval = pe->policy_value;
			}
		}
		/* Now we have the lowest value, lets find
		 * the cnt of these.
		 */
		dlist_reset(pool->peList);
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(curval == pe->policy_value) {
				cnt++;
				if(cnt == 1) {
					/* remember first one */
					low = pe;
				}
			}
		}
		if(cnt == 1) {
			pe = low;
			pe->policy_value++;
			break;
		}
		if(pool->lastUsed == NULL) {
			/* never given out yet */
			pe = low;
			pe->policy_value++;
			break;
		}
		/* give out next one beyond last given out */
		dlist_reset(pool->peList);
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(pe == pool->lastUsed) {
				pe = (struct rsp_pool_ele *)dlist_get(pool->peList);
				if(pe == NULL) {
					/* recall first one */
					pe = low;
				}
				pe->policy_value++;
				break;
			}
		}
		break;
	case RSP_POLICY_WEIGHTED_ROUND_ROBIN:
		pe = NULL;
		cnt = 0;
		if(pool->lastUsed == NULL) {
		prep_it:
			dlist_reset(pool->peList);
			while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
				pe->policy_actvalue = pe->policy_value;
			}
			dlist_reset(pool->peList);
		}
	try_again:
		while((pe = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
			if(pe->policy_actvalue == 0)
				continue;
			pe->policy_actvalue--;
			break;
		}

		if ((pe == NULL) && (cnt == 0)) {
			/* go back to the beginning. */
			cnt++;
			dlist_reset(pool->peList);
			goto try_again;
		}
		if ((pe == NULL) && (cnt == 1)) {
			/* If we reach here, we have an all
			 * zero set of policy values. Reset
			 * all the values.
			 */
			cnt++;
			goto prep_it;
		}
		if ((pe == NULL) && (cnt == 2)) {
			/* no possible pe */
			pe = NULL;
		}
		break;

	}
	return (pe);
}

