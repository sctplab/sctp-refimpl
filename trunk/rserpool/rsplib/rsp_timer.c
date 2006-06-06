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
	free(req);
}

struct rsp_enrp_req *
rsp_aloc_req(char *name, int namelen, void *msg, int msglen)
{
	struct rsp_enrp_req *r;
	r = malloc(sizeof(struct rsp_enrp_req));
	if(r == NULL)
		return(r);

	r->req = msg;
	r->len = msglen;
	r->request_type = 0;
	r->name = malloc(namelen);
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
	rsp_start_enrp_server_hunt(entry->sd, entry, 1);
}

void
handle_t6_announce_timer(struct rsp_timer_entry *entry)
{
}

void
handle_t7_enrpoutdate_timer(struct rsp_timer_entry *entry)
{
}


static
void rsp_expire_timer(struct rsp_timer_entry *entry)
{
	void *v;
	/* First pull it off */
	v = dlist_getThis(rsp_pcbinfo.timer_list);
	if(v != (void *)entry) {
		printf("List inconsistency .. hmm\n");
	}
	/* Now unlock the list before we do the timeout */
	if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
		fprintf(stderr, "Unsafe access, thread unlock failed for rsp_tmr_mtx:%d\n", errno);
	}
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
		if ((entry->cond_awake) && (entry->sleeper_count > 0)) {
			pthread_cond_broadcast(&entry->rsp_sleeper);
	
		} 
		if(entry->cond_awake) 
			pthread_cond_destroy(&entry->rsp_sleeper);

		if(entry->req) {
			rsp_free_req(entry->req);
		}
		break;
	};
	if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
		fprintf(stderr, "Unsafe access, thread lock failed for rsp_tmr_mtx:%d\n", errno);
	}
}

void rsp_timer_check ( void )
{
	/* we enter with the timer mutex asserted */
	struct rsp_timer_entry *entry;
	struct timeval now;
	int not_done = 1;
	int min_timeout;
	struct pollfd pfd[1];

	/* setup empty poll array */
	pfd[0].fd = -1;
	pfd[0].events = 0;

	if (gettimeofday(&now , NULL) ) {
		fprintf(stderr, "Gak, system error can't get time of day?? -- failed:%d\n", errno);
		return;
	}
	while(not_done) {
		dlist_reset(rsp_pcbinfo.timer_list);
		entry = (struct rsp_timer_entry *)dlist_get( rsp_pcbinfo.timer_list);
		if(entry == NULL) {
			/* none left, we are done */
			not_done = 0;
			continue;
		}
		/* Has it expired? */
		if ((now.tv_sec > entry->expireTime.tv_sec) ||
		    ((entry->expireTime.tv_sec ==  now.tv_sec) &&
		     (now.tv_sec >= entry->expireTime.tv_usec))) {
			/* Yep, this one has expired */
			rsp_expire_timer(entry);
			/* go get the next one */
			continue;
		}
		/* ok, at this point entry points to an un-expired timer and
		 * the next one to expire at that.
		 */
		if(now.tv_sec > entry->expireTime.tv_sec) {
			min_timeout = (now.tv_sec - entry->expireTime.tv_sec) * 1000;
			if(now.tv_usec >= entry->expireTime.tv_usec) {
				min_timeout += (now.tv_usec - entry->expireTime.tv_usec)/1000;
			} else {
				/* borrow a second */
				min_timeout -= 1000;
				/* add it to now */
				now.tv_usec += 1000000;
				min_timeout += (now.tv_usec - entry->expireTime.tv_usec)/1000;
			}
		} else if (now.tv_sec == entry->expireTime.tv_sec) {
			min_timeout = (now.tv_usec - entry->expireTime.tv_usec)/1000;
		} else {
			/* wait a ms and reprocess */
			min_timeout = 0;
		}
		if(min_timeout < 1) {
			min_timeout = 1;
		}
		if(min_timeout > rsp_pcbinfo.minimumTimerQuantum)
			min_timeout = rsp_pcbinfo.minimumTimerQuantum;

		if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access, thread unlock failed for rsp_tmr_mtx:%d\n", errno);
		}
		/* delay min_timeout */
		if(poll(pfd, 0, min_timeout)) {
			fprintf(stderr, "hmm. poll returns non-zero errno:%d\n", errno);
		}

		if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access, thread lock failed for rsp_tmr_mtx:%d\n", errno);
		}
		if (gettimeofday(&now , NULL) ) {
			fprintf(stderr, "Gak, system error can't get time of day?? -- failed:%d\n", errno);
			not_done = 0;
		}
	}
	/* we leave with the timer mutex asserted */
}


/* Start, or restart a timer */
int
rsp_start_timer(struct rsp_enrp_scope *sd,
		struct rsp_socket_hash 	*sdata, 
		uint32_t time_out_ms, 
		struct rsp_enrp_req *msg,
		int type, 
		uint8_t want_cond, 
		uint16_t sleeper_cnt,
		struct rsp_timer_entry *ote)
{
	int sec, usec, ret;
	struct rsp_timer_entry *te, *ete;

	if(ote == NULL) {
		te = malloc(sizeof(struct rsp_timer_entry));
		if(te == NULL) {
			fprintf(stderr, "No memory for timer err:%d\n", errno);
			return(-1);
		}
	} else {
		te = ote;
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
	if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
		fprintf(stderr, "Unsafe access, thread lock failed for rsp_tmr_mtx:%d - refusing\n", errno);
		free(te);
		return(-1);
	}

	if(ote == NULL) {
		te->sd = sd;
		te->sdata = sdata;
		te->req = msg;
		te->timer_type = type;
		if(want_cond) {
			if(pthread_cond_init(&te->rsp_sleeper, NULL)) {
				fprintf(stderr, "Warning timer can't gen cond variable error:%d\n", errno);
			} else {
				te->cond_awake = 1;
			}
		}
		te->sleeper_count = sleeper_cnt;
	}

	/* Now add it to timer queue */
	dlist_reset(rsp_pcbinfo.timer_list);
	while ((ete = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
		if(ete->expireTime.tv_sec > te->expireTime.tv_sec) {
		insert_where_at:
			if((ret = dlist_insertHere(rsp_pcbinfo.timer_list, te))) {
			failed_insert:
				fprintf(stderr, "Can't insert entry on the timer list -- failed:%d\n", ret);
				if(want_cond) {
					pthread_cond_destroy(&te->rsp_sleeper);
				}
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
	/* Now wakeup timer thread, if it is sleeping */
	if ((ote == NULL) && (dlist_getCnt(rsp_pcbinfo.timer_list) == 1)) {
		/* we were the only ones on the list, we must
		 * wake up the sleeping thread.
		 */
		if(pthread_cond_signal(&rsp_pcbinfo.rsp_tmr_cnd)) {
			fprintf(stderr, "Can't wake timeout thread? error:%d\n",
				errno);
		}
	}
	/* unlock, your done */
	if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
		fprintf(stderr, "Unsafe access, thread unlock failed for timer start rsp_tmr_mtx:%d\n", errno);
	}
	return (0);
}

