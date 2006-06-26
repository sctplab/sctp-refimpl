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

static struct rsp_paramhdr *
get_next_parameter (uint8_t *buf, uint8_t *limit, uint8_t **nxt, uint16_t *type)
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

void
asap_decode_pe_entry_and_add(struct rsp_pool *pool,
			     struct rsp_pool_element *pe,
			     uint8_t *limit)
{
	struct rsp_pool_ele *pes;
	/*
	 * 1. See if pe is already in, if so just mark it to present.
	 * 2. If not in, build a new one from this.
	 */
	uint32_t id;

	id = ntohl(pe->id);

	/* Note: pe-identifer unique to pool <OR>
	 *       unique to pool + home server?
	 *       Maybe homeserver id should always be 0 from ENRP to PU/PE?
	 */
	dlist_reset(pool->peList);
	while((pes = (struct rsp_pool_ele *)dlist_get(pool->peList)) != NULL) {
		if(pes->pe_identifer == id) {
			pes->state &= ~RSP_PE_STATE_BEING_DEL;
			return;
		}
	}
	/* if we reach here we are at step 2. */
	pes = malloc(sizeof(struct rsp_pool_ele));
	if(pes == NULL) {
		/* no memory ? */
		return;
	}
	memset(pes, 0, sizeof(*pes));
	pes->pool = pool;
	pool->refcnt++;

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
		asap_decode_pe_entry_and_add(pool, pe, at);
		pe = (struct rsp_pool_element *)get_next_parameter(at, limit, &at, &this_param);
	}
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
handle_asapmsg_fromenrp (struct rsp_enrp_scope *scp, char *buf, struct sctp_sndrcvinfo *sinfo, ssize_t sz,
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
		asap_handle_name_resolution_response(scp, enrp, buf, sz, sinfo);
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
