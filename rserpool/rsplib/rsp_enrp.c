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
