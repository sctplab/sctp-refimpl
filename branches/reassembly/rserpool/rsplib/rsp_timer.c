#include <rserpool_util.h>
#include <rserpool_lib.h>
#include <rserpool.h>
#include <rserpool_io.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <sys/errno.h>
#include <poll.h>

void
rsp_free_req(struct rsp_enrp_req *req)
{
	if(req->req)
		free(req->req);
	if(req->name)
		free(req->name);
	if (req->cause.olen > 0)
	    free(req->cause.ocause);
	free(req);
}

struct rsp_enrp_req *
rsp_aloc_req(const char *name, int namelen, void *msg, int msglen, int type)
{
	struct rsp_enrp_req *r;
	r = malloc(sizeof(struct rsp_enrp_req));
	if(r == NULL)
		return(r);

	r->req = msg;
	r->len = msglen;
	r->request_type = type;
	r->namelen = namelen;
	r->name = malloc(namelen);
	r->resolved = 0;
	if(r->name == NULL) {
		free(r);
		return(NULL);
	}
	memcpy(r->name, name, namelen);
	return (r);
}

void
handle_t1_enrp_timer(struct rsp_timer_entry *entry)
{
	/* ok, we must start server hunt */
	struct rsp_timer_entry *te;
	struct rsp_enrp_scope *scp;

	entry->chained_next = NULL;
	scp = entry->scp;
	scp->homeServer = NULL;
	rsp_start_enrp_server_hunt(scp);
	if(scp->homeServer == NULL) {
		/* need to chain on, SH has begun */
		if(scp->enrp_tmr->chained_next) {
			te = scp->enrp_tmr->chained_next;
			while(te->chained_next != NULL)
				te = te->chained_next;
			/* add myself to the list */
			te->chained_next = entry;
		} else {
			scp->enrp_tmr->chained_next = entry;
		}
	}
}

void
handle_t2_reg_timer(struct rsp_timer_entry *entry)
{
}

void
handle_t3_dereg_timer(struct rsp_timer_entry *entry)
{
}

void
handle_t4_rereg_timer(struct rsp_timer_entry *entry)
{
}

void
handle_t5_hunt_timer(struct rsp_timer_entry *entry)
{
	rsp_start_enrp_server_hunt(entry->scp);
}

void
handle_t6_announce_timer(struct rsp_timer_entry *entry)
{
}

void
handle_t7_enrpoutdate_timer(struct rsp_timer_entry *entry)
{
}

void rsp_expire_timer(struct rsp_timer_entry *entry)
{
	void *v;
	/* First pull it off */
	v = dlist_getThis(rsp_pcbinfo.timer_list);
	if(v != (void *)entry) {
		printf("List inconsistency .. hmm\n");
	}
	/* Now unlock the list before we do the timeout */
	switch(entry->timer_type) {
	case RSP_T1_ENRP_REQUEST:
		handle_t1_enrp_timer(entry);
		break;
	case RSP_T2_REGISTRATION:
		handle_t2_reg_timer(entry);
		break;
	case RSP_T3_DEREGISTRATION:
		handle_t3_dereg_timer(entry);
		break;
	case RSP_T4_REREGISTRATION:
		handle_t4_rereg_timer(entry);
		break;
	case RSP_T5_SERVERHUNT:
		handle_t5_hunt_timer(entry);
		break;
	case RSP_T6_SERVERANNOUNCE:
		handle_t6_announce_timer(entry);
		break;
	case RSP_T7_ENRPOUTDATE:
		handle_t7_enrpoutdate_timer(entry);
		break;
	default:
		fprintf(stderr,"Unknown timer type %d??\n", entry->timer_type);
		if(entry->req) {
			rsp_free_req(entry->req);
		}
		break;
	};
}

/* Start, or restart a timer */
int
rsp_start_timer(struct rsp_enrp_scope *scp,
		struct rsp_socket	*sdata, 
		uint32_t time_out_ms, 
		struct rsp_enrp_req *msg,
		int type, 
		struct rsp_timer_entry **ote)
{
	int sec, usec, ret;
	struct rsp_timer_entry *te, *ete;

	if(*ote == NULL) {
		te = malloc(sizeof(struct rsp_timer_entry));
		if(te == NULL) {
			fprintf(stderr, "No memory for timer err:%d\n", errno);
			return(-1);
		}
		te->chained_next = NULL;
	} else {
		te = *ote;
		/* sanity check */
		dlist_reset(rsp_pcbinfo.timer_list);
		while ((ete = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
			if(ete == te) {
				/* already running */
				errno = EALREADY;
				return(-1);
			}
		}
	}

	if (gettimeofday(&te->started , NULL) ) {
		fprintf(stderr, "Gak, system error can't get time of day?? -- failed:%d\n", errno);
		free(te);
		errno = EFAULT;
		return(-1);
	}
	/* translate to sec and usec */
	sec = time_out_ms/1000;
	usec = (time_out_ms % 1000) * 1000;

	/* add it to start to get expire */
	te->expireTime.tv_sec = te->started.tv_sec + sec;
	te->expireTime.tv_usec = te->started.tv_usec + usec;

	/* if we have a carry, do it */
	if(te->expireTime.tv_usec > 1000000){
		te->expireTime.tv_sec += 1;
		te->expireTime.tv_usec -= 1000000;
	}
	if(*ote == NULL) {
		te->scp = scp;
		te->sdata = sdata;
		te->req = msg;
		te->timer_type = type;
	}
	/* Now add it to timer queue */
	dlist_reset(rsp_pcbinfo.timer_list);
	while ((ete = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
		if(ete->expireTime.tv_sec > te->expireTime.tv_sec) {
		insert_where_at:
			if((ret = dlist_insertHere(rsp_pcbinfo.timer_list, te))) {
			failed_insert:
				fprintf(stderr, "Can't insert entry on the timer list -- failed:%d\n", ret);
				free(te);
				return(-1);
			}
			break;
		} else if ((ete->expireTime.tv_sec == te->expireTime.tv_sec) && 
			   (ete->expireTime.tv_usec > te->expireTime.tv_usec)) {
			goto insert_where_at;
		}
	}
	if(ete == NULL) {
		/* We did not insert before but ran to end of list */
		if((ret = dlist_append(rsp_pcbinfo.timer_list, te))) {
			goto failed_insert;
		}
	}
	/* give back the running entry */
	if(*ote == NULL)
		*ote = te;
	return (0);
}

int
rsp_stop_timer(struct rsp_timer_entry *te)
{
	/* lock mutex */
	struct rsp_timer_entry *ent;
	int ret = -1;

	/* The dis-advantage to this dlist object is that
	 * in effect we cannot help but walk the whole list
	 * looking for our element. If we had the TAILQ_ from
	 * kernel land you can pull one off.. or get the next
	 * one simply by having a pointer to it. For this
	 * enrp timer in this particular instance its not
	 * to bad. BUT if I were to make this general purpose
	 * I would have to change it so that instead we use
	 * either a kernel copied TAILQ or some such. Cut
	 * down on list walks (especially for large lists)...
	 */
	dlist_reset(rsp_pcbinfo.timer_list);	
	while ((ent = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
		if(ent == te) {
			/* found him. */
			ent = dlist_getThis(rsp_pcbinfo.timer_list);
			free(ent);
			ret = 0;
		}
	}
	return(ret);
}
