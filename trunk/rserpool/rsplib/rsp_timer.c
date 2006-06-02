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
		if(entry->cond_awake) {
			pthread_cond_broadcast(&entry->rsp_sleeper);
			pthread_cond_destroy(&entry->rsp_sleeper);
		} 
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

