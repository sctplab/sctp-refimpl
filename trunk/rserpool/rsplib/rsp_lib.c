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

	rsp_timer_check ( );

	return((void *)&done);
}


static int
rsp_init()
{
	int i;
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
	bail_out:
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}
	/* now If we are in good shape for that, setup the poll array for the thread */
	rsp_pcbinfo.watchfds = malloc((sizeof(struct pollfd) * RSP_DEF_POLLARRAY_SZ));
	if(rsp_pcbinfo.watchfds == NULL) {
		goto bail_out;
	}
	memset(rsp_pcbinfo.watchfds, 0, ((sizeof(struct pollfd) * RSP_DEF_POLLARRAY_SZ)));
	rsp_pcbinfo.num_fds = 1;
	rsp_pcbinfo.siz_fds = RSP_DEF_POLLARRAY_SZ;
	rsp_pcbinfo.watchfds[0].fd = rsp_pcbinfo.lsd[RSP_LSD_INTERNAL_READ];
	rsp_pcbinfo.watchfds[0].events = POLLIN;
	rsp_pcbinfo.watchfds[0].revents = 0;

	for(i=1; i<RSP_DEF_POLLARRAY_SZ; i++) {
		/* mark all others unused */
		rsp_pcbinfo.watchfds[i].fd = -1;
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
	memset(es, 0, sizeof(struct rsp_enrp_scope));

	es->sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(es->sd  == -1) {
		fprintf(stderr, "Can't get 1-2-many sctp socket for scope struct err:%d\n", errno);
		free(es);
		return(-1);
	}

	/* cache of names */
	es->cache = HashedTbl_create(RSP_CACHE_HASH_TABLE_NAME, 
					RSP_CACHE_HASH_TBL_SIZE);
	if(es->cache == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of name cache\n");
		}
		goto out_with_error;
	}

        /* assoc id-> pe */
	es->vtagHash = HashedTbl_create(RSP_VTAG_HASH_TABLE_NAME, 
					   RSP_VTAG_HASH_TBL_SIZE);
	if(es->vtagHash == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of vtags\n");
		}
		goto out_with_error;
	}

	/* ipadd -> rsp_pool_ele */
	es->ipaddrPortHash= HashedTbl_create(RSP_IPADDR_HASH_TABLE_NAME, 
						RSP_IPADDR_HASH_TBL_SIZE);
	if (es->ipaddrPortHash == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of ipaddr\n");
		}
		goto out_with_error;
	}

	es->enrpList = dlist_create();
	if(es->enrpList == NULL) {
		fprintf(stderr, "Can't get memory for enrp_scope dlist err:%d\n", errno);
		goto out_with_error;
	}
	/* list of all pools */
	es->allPools = dlist_create();
	if(es->allPools == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for dlist of all pools\n");
		}
		goto out_with_error;
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
	event.sctp_partial_delivery_event = 0;
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
		goto out_with_error;
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

	/* all good, we can add it to the list of scopes */
	ret = dlist_append(rsp_pcbinfo.scopes,(void *)es);
	if (ret) {
		fprintf(stderr, "Can't get memory for enrp_scope dlist err:%d ret:%d\n", errno, ret);
		goto out_with_error;
	}


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
	/* Tell the thread that we are now happy with this enrp scope (the sd) */

	if (write(rsp_pcbinfo.lsd[RSP_LSD_WAKE_WRITE], &es->sd, sizeof(es->sd))  < sizeof(es->sd)) {
		fprintf(stderr,"Lost domain - write of sd to thread fails error:%d\n", errno);
	}
	/* start server hunt procedures */
	rsp_start_enrp_server_hunt(es, 1);

	/* Here we must send off the id to the reading thread 
	 * this will get it to add it to the fd list its watching
	 */
	return (es->scopeId);

 out_with_error:
	close(es->sd);
	if(es->cache)
		HashedTbl_destroy(es->cache);
	
	if(es->vtagHash)
		HashedTbl_destroy(es->vtagHash);

	if (es->ipaddrPortHash)
		HashedTbl_destroy(es->ipaddrPortHash);

	if (es->enrpList) 
		dlist_destroy(es->enrpList);

	if (es->allPools) 
		dlist_destroy(es->allPools);

	free(es);
	return(-1);
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
rsp_start_enrp_server_hunt(struct rsp_enrp_scope *scp, int non_blocking)
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

	tmr = scp->enrp_tmr;
	if ((tmr != NULL) && (scp->state & RSP_SERVER_HUNT_IP)) {
		if(non_blocking == 1) {
			return;
		}
		if (scp->enrp_tmr->timer_type != RSP_T5_SERVERHUNT) {
			fprintf(stderr, "Warning waiting to do server hunt on timer:%d\n",
				scp->enrp_tmr->timer_type);
		}
		/* are we ok now? */
		if(scp->state & RSP_ENRP_HS_FOUND) {
			/* Yep, we are ok */
			return;
		}
	}
	/* start server hunt */
	scp->state |= RSP_SERVER_HUNT_IP;
	dlist_reset(scp->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(scp->enrpList)) != NULL) {
		if (re->state == RSP_NO_ASSOCIATION) { 
			if((sctp_connectx(scp->sd, re->addrList, re->number_of_addresses)) < 0) {
				re->state = RSP_ASSOCIATION_FAILED;
			} else {
				re->state = RSP_START_ASSOCIATION;
				/* try to get assoc id */
				re->asocid = get_asocid(scp->sd, re->addrList);
			}
			cnt++;
		} else if (re->state == RSP_ASSOCIATION_UP) {
			/* we have one, set to this guy */
			scp->homeServer = re;
			scp->state |= RSP_ENRP_HS_FOUND;
			scp->state &= ~RSP_SERVER_HUNT_IP;
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
		dlist_reset(scp->enrpList);
		while((re = (struct rsp_enrp_entry *)dlist_get(scp->enrpList)) != NULL) {
			if (re->state == RSP_ASSOCIATION_FAILED) {
				if((sctp_connectx(scp->sd, re->addrList, re->number_of_addresses)) < 0) {
					re->state = RSP_ASSOCIATION_FAILED;
				} else {
					re->state = RSP_START_ASSOCIATION;
					/* try to get assoc id */
					re->asocid = get_asocid(scp->sd, re->addrList);
				}
				cnt++;
			}
			if(cnt >= ENRP_MAX_SERVER_HUNTS)
				/* As far as we will go */
				break;
		}
	}
	rsp_start_timer(scp, (struct rsp_socket *)NULL, 
			scp->timers[RSP_T5_SERVERHUNT], 
			(struct rsp_enrp_req *)NULL, 
			RSP_T5_SERVERHUNT, &scp->enrp_tmr);
}

static struct rsp_enrp_scope *
rsp_find_scope_with_sd(int fd)
{
	struct rsp_enrp_scope *scp=NULL;

	dlist_reset(rsp_pcbinfo.scopes);
	while((scp = (struct rsp_enrp_scope *)dlist_get(rsp_pcbinfo.scopes)) != NULL) {
		if(scp->sd == fd) {
			break;
		}
	}
	return(scp);
}

static struct rsp_enrp_scope *
rsp_find_scope_with_id(uint32_t op_scope)
{
	struct rsp_enrp_scope *scp = NULL;

	dlist_reset(rsp_pcbinfo.scopes);
	while((scp = (struct rsp_enrp_scope *)dlist_get(rsp_pcbinfo.scopes)) != NULL) {
		if(scp->scopeId == op_scope) {
			break;
		}
	}
	return(scp);
}


void
rsp_process_fd_for_scope (struct rsp_enrp_scope *scp)
{
	/* Some ASAP message is waiting on the sd from
	 * an ENRP server or an event at least :-D
	 *
	 * Read and process it.
	 */
	ssize_t sz;
	struct pe_address from;
	socklen_t from_len;
	struct sctp_sndrcvinfo info;
	int msg_flags=0;
	char readbuf[RSERPOOL_STACK_BUF_SPACE];

	from_len = sizeof(from);
	sz = sctp_recvmsg (scp->sd, 
			   readbuf, 
			   sizeof(readbuf),
			   (struct sockaddr *)&from,
			   &from_len,
			   &info,
			   &msg_flags);
	if(! (msg_flags & MSG_EOR)) {
		fprintf(stderr, "Warning got a partial message size:%d can't re-assemble .. gak, enrp msg lost!\n", sz);
		return;
	}
	if(msg_flags & MSG_NOTIFICATION) {
		handle_enrpserver_notification(scp, readbuf, &info, sz, 
					       (struct sockaddr *)&from, from_len);
	} else {
		/* we call the handle_asapmsg_fromenrp routine here */
		handle_asapmsg_fromenrp(scp, readbuf, &info, sz, 
					(struct sockaddr *)&from, from_len);
	}
}

static int
rsp_process_new_sd()
{
	void *temp_space;
	struct rsp_enrp_scope *scp;
	int sd,i, orig;
	int ret, retcode= -1;

	ret = read(rsp_pcbinfo.lsd[RSP_LSD_INTERNAL_READ], (char *)&sd, sizeof(sd));
	if (ret < sizeof(sd)) {
		fprintf(stderr, "Bad read, not %d but %d errno:%d\n",
			sizeof(sd), ret, errno);
		return(-1);
	}
	/* ok sd now contains our new sd, validate it */
	scp = rsp_find_scope_with_sd(sd);
	if(scp == NULL) {
		fprintf(stderr, "Can not find sd:%d in scope array\n",
			sd);
		return(-1);
	}
	/* insert it in read array */
	for(i=1; i<rsp_pcbinfo.siz_fds; i++) {
		if(rsp_pcbinfo.watchfds[i].fd == -1) {
			/* found a slot */
			rsp_pcbinfo.watchfds[i].fd = sd;
			rsp_pcbinfo.watchfds[i].events = POLLIN; 
			rsp_pcbinfo.watchfds[i].revents = 0;
			if(i > rsp_pcbinfo.num_fds) {
				/* update the count */
				rsp_pcbinfo.num_fds = i;
			}
			retcode = 0;
			goto out_locked;
		}
	}
	/* If we fall out, we are out of descriptors 
	 * and must allocate new space. Double the current
	 * size.
	 */
	orig = (sizeof(struct pollfd) * rsp_pcbinfo.siz_fds);
	i = 2 * orig;

	temp_space = malloc(i);
	if(temp_space == NULL) {
		/* no room */
		fprintf(stderr, "Can't malloc more memory for expansion err:%d\n", errno);
		return (-1);
	}
	memset(temp_space, 0, i);
	rsp_pcbinfo.siz_fds *= 2;
	memcpy(temp_space, rsp_pcbinfo.watchfds, orig);
	for(i=rsp_pcbinfo.num_fds; i<rsp_pcbinfo.siz_fds; i++) {
		rsp_pcbinfo.watchfds[i].fd = -1;
	}
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].fd = sd;
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].events = POLLIN;
	rsp_pcbinfo.num_fds++;
	retcode = 0;
 out_locked:
	return (retcode);
}

void rsp_process_fds(int ret)
{
	/* Look at all sockets we are watching. These
	 * include all 
	 */
	int i=1;
	struct rsp_enrp_scope *scp;

	/* two types of handling, type
	 * one is special, notification of new
	 * sd's. Type two is an actual sd awakens.
	 * For these we read what happened, interpret
	 * any events to enrp.. and process.
	 */

	/* check for new fd's */
	if(rsp_pcbinfo.watchfds[0].revents) {
		/* yep, its my lsd socket */
		ret--;
		if(rsp_process_new_sd()) {
			fprintf(stderr, "Warning - failed to add new sd - a scope is unwatched\n");
		}
		rsp_pcbinfo.watchfds[0].revents = 0;
	} 
	i = 1;
	while(ret > 0) {
		for(; i< rsp_pcbinfo.num_fds; i++) {
			if(rsp_pcbinfo.watchfds[i].revents) {
				/* this one woke up, find the scope */
				scp = rsp_find_scope_with_sd(rsp_pcbinfo.watchfds[i].fd);
				if(scp == NULL) {
					fprintf (stderr, "fd:%d does not belong to any scope? - nulling",
						 rsp_pcbinfo.watchfds[i].fd);
					rsp_pcbinfo.watchfds[i].fd = -1;
					rsp_pcbinfo.watchfds[i].events = 0;
					rsp_pcbinfo.watchfds[i].revents = 0;
					ret--;
				} else {
					rsp_process_fd_for_scope (scp);
					rsp_pcbinfo.watchfds[i].revents = 0;
					ret--;
				}
				break;
			}
		}
	}
}


int
rsp_socket(int domain, int type,  int protocol, uint32_t op_scope)
{
	int sd, ret;
	struct rsp_socket *sdata;

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((type != SOCK_SEQPACKET) &&
	    (type != SOCK_DGRAM)){
		errno = ENOTSUP;
		return (-1);
	}
	sdata = (struct rsp_socket *) sizeof(struct rsp_socket);
	if(sdata == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Can't get memory for rsp_socket\n");
		}
		errno = ENOMEM;
		return(-1);
	}
	memset(sdata, 0, sizeof(struct rsp_socket));
	sdata->scp = rsp_find_scope_with_id(op_scope);
	if(sdata->scp == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "Can't find scope id:%d\n", op_scope);
		}
		free(sdata);
		return (-1);
	}
	/* FIX? If we refcount scp, we need a bump here? */

	sd = socket(domain, type, protocol);
	if(sd == -1) {
		free(sdata);
		return (sd);
	}

	/* setup and bind the port */
	memset(sdata, 0, sizeof(struct rsp_socket));
	sdata->sd = sd;
	sdata->port = 0; /* unbound */
	sdata->type = type;
	sdata->domain = domain;

	/* setup w/addrlist w/ctl&data seperate */
	sdata->address_reg = dlist_create();
	if (sdata->address_reg == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for addr_reg req dlist\n");
		}
		goto error_out;
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
	rsp_pcbinfo.rsp_number_sd++;
	if( (ret = HashedTbl_enterKeyed(rsp_pcbinfo.sd_pool , 	/* table */
					sdata->sd, 		/* key-int */
					(void*)sdata , 		/* dataum */
					(void *)&sdata->sd, 	/* keyp */
					sizeof(sdata->sd))) ) {	/* size of key */
		fprintf(stderr, "Failed to enter into hash table error:%d\n", ret);
		goto error_out;
	}

	return(sd);
 error_out:
	close(sdata->sd);
	if(sdata->address_reg)
		dlist_destroy(sdata->address_reg);
	free(sdata);
	return(-1);
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
	struct rsp_socket *sdata;
	struct rsp_pool *pool;
	struct rsp_enrp_scope *scp;
	struct rsp_timer_entry *tme;

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}
	/* First prioity, if name == NULL its a PE
	 * send and either the to or sinfo->sinfo_asocid
	 * is set.
	 * 
	 */
	/* First find the socket stuff */
	sdata  = (struct rsp_socket *)HashedTbl_lookup(rsp_pcbinfo.sd_pool ,
							    (void *)&sockfd,
							    sizeof(sockfd),
							    NULL);
	if(sdata == NULL) {
		/* sockfd is not one of ours */
		errno = EINVAL;
		return (-1);
	}
	scp = sdata->scp;

	if(name == NULL) {
		/* must be a existing PE */

	} else {
		/* named based hunting, imply's using
		 * load balancing policy after finding name.
		 */
		pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, *namelen, NULL);
		if(pool == NULL) {
			/* need to block and get info, first is
			 * there already a request pending for this.
			 * If so just join sleep, if not create and 
			 * send, then sleep.
			 */
			tme = asap_find_req(scp, name, *namelen, ASAP_REQUEST_RESOLUTION, 1);
			if(tme) {
				/* add our selves to list by blocking here on this resolution */
			} else {
				/* create message, and then send, then block on thie resolution */
				
			}
			pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, *namelen, NULL);
			if(pool == NULL) {
				/* Still no entry have resolution, not found */
				errno = ENOENT;
				return (-1);
			}
		}
		/* If we fall to here we have a pool to send to */

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


uint32_t
rsp_initialize(struct rsp_info *info)
{
	/* First do we do major initialization? */
	uint32_t id;
	if (rsp_inited == 0) {
		/* yep */
		if(rsp_init() != 0) {
			return(0);
		}
	}

	rsp_inited = 1;
	id = rsp_load_config_file(info->rsp_prefix);
	return(id);
}
