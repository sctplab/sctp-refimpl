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

static char *
rsp_read_rest(struct rsp_socket *sdata, void *mag, int len,
	      struct sinfo_sndrcvinfo *sinfo, int *totlen)
{
	struct sinfo_sndrcvinfo lsinfo;
	char *mnew=NULL, *mold=NULL;
	int llen, flags=0;
	int fromlen;
	struct pe_address sa;

	llen = len + RSP_READ_ADDITIONAL_SZ;
	mnew = (char *)malloc(llen);
	if(mnew == NULL) {
		if(mold)
			free(mold);
		return(NULL);
	}
	memcpy(mnew, msg, len);
	lsinfo = *sinfo;
	
	while((lsinfo->sinfo_flags & SCTP_EOF) == 0) {
		/* while there is more */
		fromlen = sizeof(struct pe_address);
		ret = sctp_recvmsg(sdata->sd, &mnew[len], (llen-len),
				   (struct sockaddr *)pa, fromlen, lsinfo, &flags);
		if(ret < 0) {
			/* an error */
			fprintf(stderr, "Read returns %d errno:%d\n",
				ret, errno);
		out_of:
 			free(mnew);
			if(mold){
				free(mold);
			}
			return(NULL);
		}
		len += ret;
		if(lsinfo.sinfo_assoc_id != sinfo->sinfo_assoc_id) {
			fprintf(stderr, "Read from kernel unexpected asocid:%x vs prev:%x\n",
				(u_int)lsinfo.sinfo_assoc_id,
				(u_int)sinfo->sinfo_assoc_id);
			goto out_of;
		}
		if((lsinfo->sinfo_flags & SCTP_EOF) == 0) {
			if(len < llen) {
				/* still room for more? */
				fprintf(stderr, "Strange, no EOF, but did not fill all len:%d < llen:%d\n",
					len, llen);
				continue;
			}
			/* need more space */
			llen = len + RSP_READ_ADDITIONAL_SZ;
			mold = mnew;
			mnew = (char *)malloc(len);
			if(mnew == NULL) {
				if(mold)
					free(mold);
				return(NULL);
			}
			memcpy(mnew, mold, len);
			free(mold);
			mold = NULL;
		}
	}
	*totlen = len;
	return(mnew);
}

void
handle_asap_message(struct rsp_socket *sdata,
		    void *msg, 
		    int len,
		    struct sockaddr *from, 
		    int fromlen, 
		    struct sinfo_sndrcvinfo *sinfo, 
		    int flags)
{
	char *owned_buf;
	int totlen;
	struct rsp_pool_ele  *pe;
	struct rsp_pool *pool;
	struct asap_message *asm;

	/* first do we have the entire message? */
	if((sinfo->sinfo_flags & SCTP_EOF) == 0) {
		owned_buf = rsp_read_rest(sdata, msg, len, sinfo, &totlen);
		if(owned_buf == NULL)
			return;
	} else {
		/* yep, transfer to locally owned buffer */
		owned_buf = malloc(len);
		if(owned_buf == NULL) {
			fprintf(stderr,"Dropping ASAP message, out of memory\n");
			return;
		}
		totlen = len;
		memcpy(owned_buf, msg, len);
	}
	/* Now we have an owned_buf 
	 * there are only two asap messages we
	 * can get from a peer. A cookie or BC.
	 *
	 * If its a cookie, it goes against the pool.
	 *
	 * If its a BC, it goes against the pe.
	 */

	/* first locate the pe */
	asm = (struct asap_message *)owned_buf;
	pe = (struct rsp_pool_ele  *)HashedTbl_lookupKeyed(scp->asocidHash, 
							   sinfo->sinfo_assoc_id, 
							   &sinfo->sinfo_assoc_id, 
							   sizeof(sctp_assoc_t), 
							   NULL);
	if(pe == NULL) {
		/* Try by address */
		pe = (struct rsp_pool_ele  *)HashedTbl_lookup(scp->ipaddrPortHash, to, to->sa_len, NULL);
		if(pe) {
			/* enter the assoc id please */
			pe->asocid = sinfo->sinfo_assoc_id;
			HashedTbl_enterKeyed(scp->asocidHash, pe->asocid, pe, &pe->asocid, sizeof(sctp_assoc_t));
		}
	}
	if ((pe == NULL) && (asm->asap_type == ASAP_BUSINESS_CARD)) {
		/* this is a special case, we COULD be a PE
		 * and received the BC as part of startup with
		 * a client. Lets lookup the pool, and then if it comes
		 * back decoded, we are set.
		 */
		
	}
	if(pe == NULL) {
		fprintf(stderr, "Got ASAP message from a non-pool peer?\n");
		free(owned_buf);
		return;
	}
	pool = pe->pool;
	/* now what to do */
	if(asm->asap_type == ASAP_COOKIE) {
		/* if old cookie exists, purge it */
		if(pool->lastCookie)
			free(pool->lastCookie);
		pool->lastCookie = owned_buf;
		pool->cookieSize = totlen;
		/* convert into cookie-echo so its ready to send */
		asm->asap_type = ASAP_COOKIE_ECHO;
	} else if (asm->asap_type == ASAP_BUSINESS_CARD) {

	} else {
		fprintf(stderr, "Un-handled ASAP message from PE, type %d\n", asm->asap_type);
		free(owned_buf);
	}
	
}

void
handle_asap_sctp_notification(struct rsp_socket *sdata,
			      void *msg, 
			      int len,
			      struct sockaddr *from, 
			      int fromlen, 
			      struct sinfo_sndrcvinfo *sinfo, 
			      int flags)
{

}
