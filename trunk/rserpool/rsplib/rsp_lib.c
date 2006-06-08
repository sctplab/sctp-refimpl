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

int rsp_inited = 0;
int rsp_debug = 1;
int rsp_scope_counter = 0;

struct rsp_global_info rsp_pcbinfo;

/*
static void
rsp_free_enrp_ent(struct rsp_enrp_entry *re)
{
	if(re->refcount == 1) {
		free(re->addrList);
		free(re);
	} else {
		re->refcount--;
	}
}
*/


static sctp_assoc_t
get_asocid(int sd, struct sockaddr *sa)
{
	struct sctp_paddrinfo sp;
	socklen_t siz;
	socklen_t sa_len;

	/* First get the assoc id */
	siz = sizeof(sp);
	memset(&sp,0,sizeof(sp));

#ifdef HAVE_SA_LEN
	sa_len = sa->sa_len;
#else
	if(sa->sa_family == AF_INET) {
		sa_len = sizeof(struct sockaddr_in);
	} else if (sa->sa_family == AF_INET6) {
		sa_len = sizeof(struct sockaddr_in6);
	}
#endif
	memcpy((caddr_t)&sp.spinfo_address, sa, sa_len);
	errno = 0;
	if(getsockopt(sd, IPPROTO_SCTP, SCTP_GET_PEER_ADDR_INFO,
		      &sp, &siz) != 0) {
		fprintf(stderr, "Failed to get assoc_id with GET_PEER_ADDR_INFO, errno:%d\n", errno);
		return (0);
	}
	/* BSD: We depend on the fact that 0 can never be returned */
	return (sp.spinfo_assoc_id);
}


void *rsp_timer_thread_run( void *arg )
{
	static int done = 0;
	if(arg != NULL)
		return((void *)&done);

	if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx)) {
		fprintf(stderr, "Pre-Timer start thread mutex lock fails error:%d -- help\n",
			errno);
		return((void *)&done);
	}
	while (1) {
		if(pthread_cond_wait(&rsp_pcbinfo.rsp_tmr_cnd, 
				     &rsp_pcbinfo.rsp_tmr_mtx)) {
			fprintf(stderr, "Condition wait fails error:%d -- help\n", errno);
			return((void *)&done);
		}
		rsp_timer_check ( );
	}
	return((void *)&done);
}


static int
rsp_init()
{
	if (rsp_inited)
		return(0);


	/* Init so we have true random number from random() */
	srandomdev();

	/* number of sd's */
	rsp_pcbinfo.rsp_number_sd = 0;

	/* shortest wait time used by timer thread */
	rsp_pcbinfo.minimumTimerQuantum = DEF_MINIMUM_TIMER_QUANTUM;
	/* create socket descriptor hash table */
	rsp_pcbinfo.sd_pool = HashedTbl_create(RSP_SD_HASH_TABLE_NAME, 
					       RSP_SD_HASH_TBL_SIZE);
	if(rsp_pcbinfo.sd_pool == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Could not alloc sd hash table\n");
		}
		return (-1);
	}

	/* create timer list */
	rsp_pcbinfo.timer_list = dlist_create();
	if (rsp_pcbinfo.timer_list == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Could not alloc tmr dlist\n");
		}
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		return (-1);
	}

	rsp_pcbinfo.scopes = dlist_create();
	if(rsp_pcbinfo.scopes == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Could not alloc tmr dlist\n");
		}
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}

	/* create mutex to lock sd pool */
	if (pthread_mutex_init(&rsp_pcbinfo.sd_pool_mtx, NULL)) {
		if(rsp_debug) {
			fprintf(stderr, "Could not init sd_pool mtx\n");
		}
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
		dlist_destroy(rsp_pcbinfo.scopes);
		return (-1);
	}

	/* create condition variable to have timer thread sleep on */
	if(pthread_cond_init(&rsp_pcbinfo.rsp_tmr_cnd, NULL)) {
		if(rsp_debug) {
			fprintf(stderr, "Could not init tmr cond var\n");
		}
		pthread_mutex_destroy(&rsp_pcbinfo.sd_pool_mtx);
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}

	/* create mutex for timer list */
	if(pthread_mutex_init(&rsp_pcbinfo.rsp_tmr_mtx, NULL)) {
		if(rsp_debug) {
			fprintf(stderr, "Could not init tmr mtx\n");
		}
	bail_out:
		pthread_cond_destroy(&rsp_pcbinfo.rsp_tmr_cnd);
		pthread_mutex_destroy(&rsp_pcbinfo.sd_pool_mtx);
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.scopes);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}
	/* now If we are in good shape for that, setup the poll array for the thread */
	rsp_pcbinfo.watchfds = malloc((sizeof(struct pollfd) * RSP_DEF_POLLARRAY_SZ));
	if(rsp_pcbinfo.watchfds == NULL) {
		goto bail_out;
	}
	/* now the socket pair for our reader thread */
	if(socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, rsp_pcbinfo.lsd) ) {
		free(rsp_pcbinfo.watchfds);
		fprintf( stderr, "Could not initialize socket pair err:%d\n", errno);
		goto bail_out;
	}
	rsp_pcbinfo.num_fds = 1;
	rsp_pcbinfo.siz_fds = RSP_DEF_POLLARRAY_SZ;
	rsp_pcbinfo.watchfds[0].fd = rsp_pcbinfo.lsd[0];
	rsp_pcbinfo.watchfds[0].events = POLLIN;
	rsp_pcbinfo.watchfds[0].revents = 0;

	/* now we must create the timer thread */
	if(pthread_create(&rsp_pcbinfo.tmr_thread,
			  NULL,
			  rsp_timer_thread_run,
			  (void *)NULL) ) {
		if(rsp_debug) {
			fprintf(stderr, "Could not start tmr thread\n");
		}
		pthread_mutex_destroy(&rsp_pcbinfo.rsp_tmr_mtx);
		pthread_cond_destroy(&rsp_pcbinfo.rsp_tmr_cnd);
		pthread_mutex_destroy(&rsp_pcbinfo.sd_pool_mtx);
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.scopes);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}
	/* set the flag that we init'ed */
	rsp_inited = 1;
	return (0);
}



static void
rsp_add_addr_to_enrp_entry(struct rsp_enrp_entry *re, struct sockaddr *sa)
{
	char *al;
	int len = 0;
#ifdef HAVE_SA_LEN
	len = sa->sa_len;;
#else
	if(sa->sa_family == AF_INET) {
		len = sizeof(struct sockaddr_in);
	} else if (sa->sa_family == AF_INET6) {
		len = sizeof(struct sockaddr_in6);
	}
#endif
	al = malloc((re->size_of_addrList + len));
	if(al == NULL) {
		fprintf(stderr, "error can't get allocate memory %d \n", errno);
		return;
	}
	if(re->addrList) {
		memcpy(al, re->addrList, re->size_of_addrList);
		free(re->addrList);
	}
	memcpy(&al[re->size_of_addrList], (caddr_t)sa, len);
	re->size_of_addrList += len;
	re->number_of_addresses++;
	re->addrList = (struct sockaddr *)al;
}

static void
rsp_add_enrp_server(struct rsp_enrp_scope *es, uint32_t enrpid, struct sockaddr *sa)
{
	struct rsp_enrp_entry *re;

	dlist_reset(es->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(es->enrpList)) != NULL) {
		if(enrpid == re->enrpId) {
			rsp_add_addr_to_enrp_entry(re, sa);
			return;
		}
	}
	re = (struct rsp_enrp_entry *)malloc(sizeof(struct rsp_enrp_entry));
	if(re == NULL) {
		fprintf(stderr,"Can't get memory for rsp_enrp_entry - error:%d\n", errno);
		return;
	}
	re->enrpId = enrpid;
	re->number_of_addresses = 0;
	re->size_of_addrList = 0;
	re->refcount = 1;
	re->asocid = 0;
	re->state = RSP_NO_ASSOCIATION;
	rsp_add_addr_to_enrp_entry(re, sa);
	dlist_append(es->enrpList, (void *)re);
}

static int
rsp_load_file(FILE *io, char *file)
{
	/* load the enrp control file opened at io */
	int line=0, val, ret;
	uint32_t enrpid;
	uint16_t port;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	char string[256], *p1, *p2, *p3, *p4;
	struct rsp_enrp_scope *es;
	struct sctp_event_subscribe event;


	/* Format is
	 * ENRP:id:port:address
	 * <or>
	 * TIMER:TIMERNAME:value:\n
	 * TIMERNAME = T1 - T7
	 * if port is 0 it goes to the default port for
	 * ENRP.
	 */
	es = malloc(sizeof(struct rsp_enrp_scope));
	if (es == NULL) {
		fprintf(stderr, "Can't get memory for enrp_scope err:%d\n", errno);
		return(-1);
	}
	ret = dlist_append(rsp_pcbinfo.scopes,(void *)es);
	if (ret) {
		fprintf(stderr, "Can't get memory for enrp_scope dlist err:%d ret:%d\n", errno, ret);
		free(es);
		return(-1);
	}

	es->sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(es->sd  == -1) {
		fprintf(stderr, "Can't get 1-2-many sctp socket for scope struct err:%d\n", errno);
		free(es);
		return(-1);
	}

	es->enrpList = dlist_create();
	if(es->enrpList == NULL) {
		fprintf(stderr, "Can't get memory for enrp_scope dlist err:%d\n", errno);
		close(es->sd);
		free(es);
		return(-1);
	}

	es->scopeId =  rsp_scope_counter;
	rsp_scope_counter++;

	es->refcount = 0;
	/* enable selected event notifications */
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_address_event = 0;
	event.sctp_send_failure_event = 1;
	event.sctp_peer_error_event = 0;
	event.sctp_shutdown_event = 1;
	event.sctp_partial_delivery_event = 1;
#if defined(__FreeBSD__)
	event.sctp_adaptation_layer_event = 0;
#else
	event.sctp_adaption_layer_event = 0;
#endif
#if defined(__FreeBSD__)
	event.sctp_authentication_event = 0;
	event.sctp_stream_reset_events = 0;
#endif
	if (setsockopt(es->sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		fprintf(stderr, "Can't do SET_EVENTS socket option! err:%d\n", errno);
		errno = EFAULT;
		close(es->sd);
		dlist_destroy(es->enrpList);
		free(es);
		return(-1);
	}
	es->timers[RSP_T1_ENRP_REQUEST] = DEF_RSP_T1_ENRP_REQUEST;
	es->timers[RSP_T2_REGISTRATION] = DEF_RSP_T2_REGISTRATION;
	es->timers[RSP_T3_DEREGISTRATION] = DEF_RSP_T3_DEREGISTRATION;
	es->timers[RSP_T4_REREGISTRATION] = DEF_RSP_T4_REREGISTRATION;
	es->timers[RSP_T5_SERVERHUNT] = DEF_RSP_T5_SERVERHUNT;
	es->timers[RSP_T6_SERVERANNOUNCE] = DEF_RSP_T6_SERVERANNOUNCE;
	es->timers[RSP_T7_ENRPOUTDATE] = DEF_RSP_T7_ENRPOUTDATE;
	es->homeServer = NULL;
	es->enrp_tmr = NULL;
	es->state = RSP_NO_ENRP_SERVER;

	while(fgets(string, sizeof(string), io) != NULL) {
		line++;
		if(string[0] == '#')
			continue;
		if(string[0] == ';')
			continue;


		p1 = strtok(string,":" );
		if(p1 == NULL) {
			fprintf(stderr,"config file %s line %d can't find a ':' seperating type from id/time\n", file, line);
			continue;
		}
		p2 = strtok(NULL, ":");
		if(p2 == NULL) {
			fprintf(stderr,"config file %s line %d can't find a ':' seperating id/time from val/port\n", file, line);
			continue;
		}
		p3 = strtok(NULL, ":");
		if(p2 == NULL) {
			fprintf(stderr,"config file %s line %d can't find a ':' seperating val/port from address/nullcr\n",
			       file, line);
			continue;
		}
		p4 = strtok(NULL, ":");
		if(p2 == NULL) {
			fprintf(stderr,"config file %s line %d can't find a terminating char after address?\n", file, line);
			continue;
		}
		/* ok p1 points to a int. p2 points to an address */
		if ((strncmp(p1, "ENRP", 4) == 0) ||
		   (strncmp(p1, "enrp", 4) == 0)) {
			enrpid = strtoul(p2, NULL, 0);
			if(enrpid == 0) {
				fprintf(stderr,"config file %s line %d reserved enrpid found aka 0?\n", file, line);
			}
			port = htons((uint16_t)strtol(p3, NULL, 0));
			if(port == 0) {
				port = htons(ENRP_DEFAULT_PORT_FOR_ASAP);
			}
			memset(&sin6, 0, sizeof(sin6));
			if(inet_pton(AF_INET6, p4 , &sin6.sin6_addr ) == 1) {
				sin6.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
				sin6.sin6_len = sizeof(sin6);
#endif			
				sin6.sin6_port = port;
				rsp_add_enrp_server(es, enrpid, (struct sockaddr *)&sin6);
				continue;
			}
			memset(&sin, 0, sizeof(sin));
			if(inet_pton(AF_INET, p3 , &sin.sin_addr ) == 1) {
				sin.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
				sin.sin_len = sizeof(sin);
#endif			
				sin.sin_port = port;
				rsp_add_enrp_server(es, enrpid, (struct sockaddr *)&sin);
				continue;
			}
			fprintf(stderr,"config file %s line %d can't translate the address?\n", file, line);
		} else if ((strncmp(p1, "TIMER", 2) == 0) ||
			   (strncmp(p1, "timer", 2) == 0)) {
			if (strncmp(p2, "T1", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T1_ENRP_REQUEST] = val;
				}

			} else if (strncmp(p2, "T2", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T2_REGISTRATION] = val;
				}
			} else if (strncmp(p2, "T3", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T3_DEREGISTRATION] = val;
				}
			} else if (strncmp(p2, "T4", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T4_REREGISTRATION] = val;
				}
			} else if (strncmp(p2, "T5", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T5_SERVERHUNT] =  val;
				}
			} else if (strncmp(p2, "T6", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T6_SERVERANNOUNCE] = val;
				}
			} else if (strncmp(p2, "T7", 2) == 0) {
				val = strtol(p3, NULL, 0);
				if(val < 1) {
					printf("config file error, Timer %s line at %d can't be less than 1 ms\n",
					       p2, line);
				} else {
					es->timers[RSP_T7_ENRPOUTDATE] = val;
				}
			} else {
				fprintf(stderr,"config file %s line %d TIMER unknown type %s\n", file, line , p2);
			}
		}
	}
	/* start server hunt procedures */
	rsp_start_enrp_server_hunt(es, 1);

	/* Here we must send off the id to the reading thread 
	 * this will get it to add it to the fd list its watching
	 */
	return (es->scopeId);
}

static int
rsp_load_config_file(const char *confprefix)
{
	char file[256];
	char prefix[100];
	FILE *io;
	int id= -1;

	if(confprefix) {
		int len;
		len = strlen(confprefix);
		if(len > 99)
			len = 99;
		prefix[len] = 0;
		memcpy(prefix, confprefix, len);
	} else {
		prefix[0] = 0;
	}
	sprintf(file, "~/.%senrp.conf", prefix);
	if ((io = fopen(file, "r")) != NULL) {
		id = rsp_load_file(io, file);
		fclose(io);
		return(id);
	}
	sprintf(file, "/usr/local/etc/.%senrp.conf", prefix);
	if ((io = fopen(file, "r")) != NULL) {
		id = rsp_load_file(io, file);
		fclose(io);
		return(id);
	}
	sprintf(file,"/usr/local/etc/enrp.conf");
	if ((io = fopen(file, "r")) != NULL) {
		id = rsp_load_file(io, file);
		fclose(io);
		return(id);
	}
	sprintf(file, "/etc/enrp.conf");
	if ((io = fopen(file, "r")) != NULL) {
		id = rsp_load_file(io, file);
		fclose(io);
		return(id);
	}
	return(id);
}

void
rsp_start_enrp_server_hunt(struct rsp_enrp_scope *sd, int non_blocking)
{
	/* 
	 * Formulate and set up an association to a
	 * max of 3 enrp servers. Once we get an association
	 * up we will choose the first one of them has the home ENRP
	 * server. Use sctp_connectx() to use multi-homed startup.
	 *
	 * Note this routine is used by both users requesting a SH and
	 * the time-out routine. For the t-o te will be set to non-NULL.
	 * For new users requests (from failures and such) te will be NULL.
	 * Thus if we see a te of NULL and we are in the SERVER_HUNT state, we
	 * don't continue, its already happening we just add to the sleeper
	 * count.
	 */
	int cnt = 0;
	struct rsp_enrp_entry *re;
	struct rsp_timer_entry *tmr;

	tmr = sd->enrp_tmr;
	if ((tmr != NULL) && (sd->state & RSP_SERVER_HUNT_IP)) {
		struct rsp_timer_entry *ete;

		if(non_blocking == 1) {
			return;
		}
		if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access %d can't look up timed server hunt in progress, \n", errno);
			return;
		}
		if (sd->enrp_tmr->timer_type != RSP_T5_SERVERHUNT) {
			fprintf(stderr, "Warning waiting to do server hunt on timer:%d\n",
				sd->enrp_tmr->timer_type);
		}
		ete->sleeper_count++;
		if(pthread_cond_wait(&rsp_pcbinfo.rsp_tmr_cnd, &rsp_pcbinfo.rsp_tmr_mtx)) {
			fprintf(stderr, "Cond wait for s-h fails error:%d\n", errno);
		}
		if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access, thread unlock failed for s-h rsp_tmr_mtx:%d\n", errno);
		}
		/* are we ok now? */
		if(sd->state & RSP_ENRP_HS_FOUND) {
			/* Yep, we are ok */
			return;
		}
	}

	/* start server hunt */
	sd->state |= RSP_SERVER_HUNT_IP;
	dlist_reset(sd->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(sd->enrpList)) != NULL) {
		if (re->state == RSP_NO_ASSOCIATION) { 
			if((sctp_connectx(sd->sd, re->addrList, re->number_of_addresses)) < 0) {
				re->state = RSP_ASSOCIATION_FAILED;
			} else {
				re->state = RSP_START_ASSOCIATION;
				/* try to get assoc id */
				re->asocid = get_asocid(sd->sd, re->addrList);
			}
			cnt++;
		} else if (re->state == RSP_ASSOCIATION_UP) {
			/* we have one, set to this guy */
			sd->homeServer = re;
			sd->state |= RSP_ENRP_HS_FOUND;
			sd->state &= ~RSP_SERVER_HUNT_IP;
			if ((tmr->cond_awake) && (tmr->sleeper_count > 0)) {
				pthread_cond_broadcast(&tmr->rsp_sleeper);
			}
			return;
		}
		if(cnt >= ENRP_MAX_SERVER_HUNTS)
			/* As far as we will go */
			break;
	}

	/* If we did not get our max, try looking for
	 * some that failed to start.
	 */
	if(cnt < ENRP_MAX_SERVER_HUNTS) {
		dlist_reset(sd->enrpList);
		while((re = (struct rsp_enrp_entry *)dlist_get(sd->enrpList)) != NULL) {
			if (re->state == RSP_ASSOCIATION_FAILED) {
				if((sctp_connectx(sd->sd, re->addrList, re->number_of_addresses)) < 0) {
					re->state = RSP_ASSOCIATION_FAILED;
				} else {
					re->state = RSP_START_ASSOCIATION;
					/* try to get assoc id */
					re->asocid = get_asocid(sd->sd, re->addrList);
				}
				cnt++;
			}
			if(cnt >= ENRP_MAX_SERVER_HUNTS)
				/* As far as we will go */
				break;
		}
	}
	rsp_start_timer(sd, (struct rsp_socket_hash *)NULL, 
			sd->timers[RSP_T5_SERVERHUNT], 
			(struct rsp_enrp_req *)NULL, 
			RSP_T5_SERVERHUNT, 1, 0, &sd->enrp_tmr);
}

int
rsp_socket(int domain, int type,  int protocol, int op_scope)
{
	int sd, ret;
	struct rsp_socket_hash *sdata;

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((type != SOCK_SEQPACKET) &&
	    (type != SOCK_DGRAM)){
		errno = ENOTSUP;
		return (-1);
	}
	sd = socket(domain, type, protocol);
	if(sd == -1) {
		return (sd);
	}

	sdata = (struct rsp_socket_hash *) sizeof(struct rsp_socket_hash);
	if(sdata == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Can't get memory for rsp_socket_hash\n");
		}
		errno = ENOMEM;
		close(sd);
		return(-1);
	}
	/* setup and bind the port */
	memset(sdata, 0, sizeof(struct rsp_socket_hash));
	sdata->sd = sd;
	sdata->port = 0; /* unbound */
	sdata->type = type;
	sdata->domain = domain;

	/* list of all pools */
	sdata->allPools = dlist_create();
	if(sdata->allPools == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for dlist of all pools\n");
		}
	out_ofa:
		free(sdata);
		close(sd);
		errno = ENOMEM;
		return (-1);
	}

	/* cache of names */
	sdata->cache = HashedTbl_create(RSP_CACHE_HASH_TABLE_NAME, 
					RSP_CACHE_HASH_TBL_SIZE);
	if(sdata->cache == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of name cache\n");
		}
	out_ofb:
		dlist_destroy(sdata->allPools);
		goto out_ofa;
	}

        /* assoc id-> pe */
	sdata->vtagHash = HashedTbl_create(RSP_VTAG_HASH_TABLE_NAME, 
					   RSP_VTAG_HASH_TBL_SIZE);
	if(sdata->vtagHash == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of vtags\n");
		}
	out_ofc:
		HashedTbl_destroy(sdata->cache);
		goto out_ofb;
	}

	/* ipadd -> rsp_pool_ele */
	sdata->ipaddrPortHash= HashedTbl_create(RSP_IPADDR_HASH_TABLE_NAME, 
						RSP_IPADDR_HASH_TBL_SIZE);
	if (sdata->ipaddrPortHash == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of ipaddr\n");
		}
	out_ofd:
		HashedTbl_destroy(sdata->vtagHash);
		goto out_ofc;
	}
        /* ENRP requests outstanding */
	sdata->enrp_reqs = dlist_create();
	if (sdata->enrp_reqs == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for enrp req dlist\n");
		}
	out_ofe:
		HashedTbl_destroy(sdata->ipaddrPortHash);
		goto out_ofd;
	}
	/* setup w/addrlist w/ctl&data seperate */
	sdata->address_reg = dlist_create();
	if (sdata->address_reg == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for addr_reg req dlist\n");
		}
	out_off:
		dlist_destroy(sdata->enrp_reqs);		
		goto out_ofe;
	}

	/* ENRP server  list */
	sdata->enrpList = dlist_create();
	if (sdata->enrpList == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for enrp home addr dlist\n");
		}
		dlist_destroy(sdata->address_reg);
		goto out_off;
	}
	/* number of names in use */
	sdata->refcnt = 0;

	/* unknown until I do the server hunt */
	sdata->enrpID = 0;

	/* unknown until we register */
	sdata->registeredName = NULL;
	/* sd default timers */
	sdata->timers[RSP_T1_ENRP_REQUEST] = DEF_RSP_T1_ENRP_REQUEST;
	sdata->timers[RSP_T2_REGISTRATION] = DEF_RSP_T2_REGISTRATION;
	sdata->timers[RSP_T3_DEREGISTRATION] = DEF_RSP_T3_DEREGISTRATION;
	sdata->timers[RSP_T4_REREGISTRATION] = DEF_RSP_T4_REREGISTRATION;
	sdata->timers[RSP_T5_SERVERHUNT] = DEF_RSP_T5_SERVERHUNT;
	sdata->timers[RSP_T6_SERVERANNOUNCE] = DEF_RSP_T6_SERVERANNOUNCE;
	sdata->timers[RSP_T7_ENRPOUTDATE] = DEF_RSP_T7_ENRPOUTDATE;

	/* my 32 bit PE id */
	/* unknown until registered */
	sdata->myPEid = 0;

	/* how long my reg is good for */
	/* unknown until registered */
	sdata->reglifetime = 0;

        /* policy I am registered to */
	/* unknown until registered */
	sdata->myPolicy = 0;

	/* times I have attempted to reg */
	sdata->registration_count = 0;

	/* threshold where I fail reg */
	sdata->registration_threshold = DEF_MAX_REG_ATTEMPT;

        /* max request retransmit value */
	sdata->max_request_retransmit = DEF_MAX_REQUEST_RETRANSMIT; 

	/* how long until the cache goes stale */
	sdata->stale_cache_ms = DEF_STALE_CACHE_VALUE;

	/* boolean flag if we are reg'd */
	sdata->registered = 0;

	/* flag say's if sd is data channel */
	/* unknown until registered */
	sdata->useThisSd = 0 ;

	if(pthread_mutex_init(&sdata->rsp_sd_mtx, NULL) ) {
		fprintf(stderr, "Warning - Mutex init fails for socket? errno%d\n", errno);

	}

	if( pthread_mutex_lock(&rsp_pcbinfo.sd_pool_mtx) ) {
		fprintf(stderr, "Unsafe access, thread lock failed for sd_pool_mtx:%d\n", errno);
	}
	rsp_pcbinfo.rsp_number_sd++;
	if( (ret = HashedTbl_enterKeyed(rsp_pcbinfo.sd_pool , 	/* table */
					sdata->sd, 		/* key-int */
					(void*)sdata , 		/* dataum */
					(void *)&sdata->sd, 	/* keyp */
					sizeof(sdata->sd))) ) {	/* size of key */
		fprintf(stderr, "Failed to enter into hash table error:%d\n", ret);
	}

	if (pthread_mutex_unlock(&rsp_pcbinfo.sd_pool_mtx) ) {
		fprintf(stderr, "Unsafe access, thread unlock failed for sd_pool_mtx:%d\n", errno);
	}

	return(sd);
}

int 
rsp_close(int sockfd)
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}
	return (0);
}

int 
rsp_connect(int sockfd, const char *name, size_t namelen)
{
	/* lookup a name, if you have
	 * already pre-loaded the cache, your
	 * done. If not, do the pre-load. 
	 *
	 * Note: we don't block since its possible
	 * to get some other user message which would
	 * get in the way of us reading and if we
	 * read and hold we have an issue with select/poll
	 * since we let the user do his/her own call to
	 * the base select/poll not an rsp_ version.
	 */

	
	/* steps:
	 *
	 * 1) see if we have a hs, if not start blocking
	 *    server hunt procedures.
	 * 2) once we have a hs, lookup the name -> cache
	 * 3) If cache hit, request update and return.
	 * 4) If cache miss, request update and return.
	 *
	 */

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}

int 
rsp_register(int sockfd, const char *name, size_t namelen, uint32_t policy, uint32_t policy_value )
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);	
}

int
rsp_deregister(int sockfd)
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}

struct rsp_info_found *
rsp_getPoolInfo(int sockfd, char *name, size_t namelen)
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (NULL);
	}

	return (NULL);
}

int 
rsp_reportfailure(int sockfd, char *name,size_t namelen,  const struct sockaddr *to, const sctp_assoc_t id)
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}

size_t 
rsp_sendmsg(int sockfd,         /* HA socket descriptor */
	    const char *msg,
	    size_t len,
	    struct sockaddr *to,
	    socklen_t *tolen,
	    char *name,
	    size_t *namelen,
	    struct sctp_sndrcvinfo *sinfo,
	    int flags)         /* Options flags */
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}

ssize_t 
rsp_rcvmsg(int sockfd,		/* HA socket descriptor */
	   const char *msg,
	   size_t len,
	   char *name, 		/* in-out/limit */
	   size_t *namelen,
	   struct sockaddr *from,
	   socklen_t *fromlen,	/* in-out/limit */
	   struct sctp_sndrcvinfo *sinfo,
	   int flags)		/* Options flags */
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}


int 
rsp_forcefailover(int sockfd, 
		  char *name, 
		  size_t namelen,
		  const struct sockaddr *to, 
		  const socklen_t tolen,	  
		  const sctp_assoc_t id)
{
	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (0);
}


int
rsp_initialize(struct rsp_info *info)
{
	/* First do we do major initialization? */
	int id;
	if (rsp_inited == 0) {
		/* yep */
		if(rsp_init() != 0) {
			return(-1);
		}
	}

	rsp_inited = 1;
	if( pthread_mutex_lock(&rsp_pcbinfo.sd_pool_mtx) ) {
		fprintf(stderr, "Unsafe access, thread lock failed for sd_pool_mtx:%d - rsp_initialize\n", errno);
	}
	id = rsp_load_config_file(info->rsp_prefix);
	if (pthread_mutex_unlock(&rsp_pcbinfo.sd_pool_mtx) ) {
		fprintf(stderr, "Unsafe access, thread unlock failed for sd_pool_mtx:%d - rsp_initialize\n", errno);
	}
	return(id);
}
