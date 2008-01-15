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
	rsp_pcbinfo.num_fds = 0;
	rsp_pcbinfo.siz_fds = RSP_DEF_POLLARRAY_SZ;
	for(i=0; i<RSP_DEF_POLLARRAY_SZ; i++) {
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

static void
rsp_expand_watchfd_array()
{
	struct pollfd *pol;

	int new_sz, cp_siz, i;

	new_sz = rsp_pcbinfo.siz_fds + RSP_WATCHFD_INCR;
	pol = (struct pollfd *)malloc((new_sz * sizeof(struct pollfd)));
	if(pol == NULL) {
		fprintf(stderr, "Out of memory in expanding pollfd from %d to %d, core soon\n",
			rsp_pcbinfo.siz_fds, new_sz);
	}
	cp_siz = rsp_pcbinfo.siz_fds * sizeof(struct pollfd);
	memcpy(pol, rsp_pcbinfo.watchfds, cp_siz);
	for(i=rsp_pcbinfo.siz_fds; i<new_sz; i++) {
		pol[i].fd = -1;
		pol[i].events = 0;
		pol[i].revents = 0;
	}
	rsp_pcbinfo.siz_fds += RSP_WATCHFD_INCR;
	free(rsp_pcbinfo.watchfds);
	rsp_pcbinfo.watchfds = pol;
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
	es->asocidHash = HashedTbl_create(RSP_VTAG_HASH_TABLE_NAME, 
					   RSP_VTAG_HASH_TBL_SIZE);
	if(es->asocidHash == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for hashtable of asocids\n");
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
		if(p3 == NULL) {
			fprintf(stderr,"config file %s line %d can't find a ':' seperating val/port from address/nullcr\n",
			       file, line);
			continue;
		}
		p4 = strtok(NULL, "\n");
		if(p4 == NULL) {
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
			if(inet_pton(AF_INET, p4 , &sin.sin_addr ) == 1) {
				sin.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
				sin.sin_len = sizeof(sin);
#endif			
				sin.sin_port = port;
				rsp_add_enrp_server(es, enrpid, (struct sockaddr *)&sin);
				continue;
			}
			fprintf(stderr,"config file %s line %d can't translate the address %s?\n", file, line, p4);
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
	if ((rsp_pcbinfo.siz_fds - rsp_pcbinfo.num_fds) < 1) {
		/* expand array */
		rsp_expand_watchfd_array();
	}
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].fd = es->sd;
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].events = POLLIN;
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].revents = 0;
	rsp_pcbinfo.num_fds++;

	/* start server hunt procedures */
	rsp_start_enrp_server_hunt(es);

	/* Here we must send off the id to the reading thread 
	 * this will get it to add it to the fd list its watching
	 */
	return (es->scopeId);

 out_with_error:
	close(es->sd);
	if(es->cache)
		HashedTbl_destroy(es->cache);

	if(es->asocidHash)
		HashedTbl_destroy(es->asocidHash);

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
	sprintf(file, "%s/.enrp.conf", prefix);
	if ((io = fopen(file, "r")) != NULL) {
		id = rsp_load_file(io, file);
		fclose(io);
		return(id);
	}
	sprintf(file, "~/.enrp.conf");
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
rsp_start_enrp_server_hunt(struct rsp_enrp_scope *scp)
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
	int cnt = 0, ret;
	struct rsp_enrp_entry *re;
	struct rsp_timer_entry *tmr;

	/* start server hunt */
	tmr = scp->enrp_tmr;
	scp->state |= RSP_SERVER_HUNT_IP;
	if(dlist_getCnt(scp->enrpList) == 0) {
		printf("Error, no valid servers to hunt to :-<\n");
		return;
	}
	dlist_reset(scp->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(scp->enrpList)) != NULL) {
		if (re->state == RSP_NO_ASSOCIATION) { 
			if(((ret = sctp_connectx(scp->sd, re->addrList, re->number_of_addresses, &re->asocid))) < 0) {
			  printf("connectx to this re:%x one fails %d\n",(u_int)re, ret);
			  re->state = RSP_ASSOCIATION_FAILED;
			} else {
			  re->state = RSP_START_ASSOCIATION;
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
			  if((sctp_connectx(scp->sd, re->addrList, re->number_of_addresses, &re->asocid)) < 0) {
				re->state = RSP_ASSOCIATION_FAILED;
			  } else {
				re->state = RSP_START_ASSOCIATION;
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
	/* empty we are just starting */
	if(scp->enrp_tmr)
		scp->enrp_tmr->chained_next = NULL;
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

static struct rsp_socket *
rsp_find_socket_with_sd(int sd)
{
	struct rsp_socket *sock;

	sock = (struct rsp_socket *)HashedTbl_lookupKeyed(rsp_pcbinfo.sd_pool,
							  sd,
							  &sd,
							  sizeof(sd),
							  NULL);
	return(sock);
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
		printf("Handle ENRP notification\n");
		handle_enrpserver_notification(scp, readbuf, &info, sz, 
					       (struct sockaddr *)&from, from_len);
		printf("Done\n");
	} else {
		/* we call the handle_asapmsg_fromenrp routine here */
		printf("Handle asap msg from ENRP\n");
		handle_asapmsg_fromenrp(scp, readbuf, &info, sz, 
					(struct sockaddr *)&from, from_len);
		printf("Done\n");
	}
}

int rsp_process_fds(int ret)
{
	/* Look at all sockets we are watching. These
	 * include all 
	 */
	int i=1;
	int count_processed=0;
	struct rsp_enrp_scope *scp;
	int howmany=0;
	/* 
	 * For sd's we read what happened, interpret
	 * any events to enrp.. and process.
	 */

	/* check for new fd's */
	for(i=0; i< rsp_pcbinfo.num_fds; i++) {
		if(rsp_pcbinfo.watchfds[i].revents) {
			/* this one woke up, find the scope */
			howmany++;
			scp = rsp_find_scope_with_sd(rsp_pcbinfo.watchfds[i].fd);
			printf("Find scope:%x for fd:%d events:%d\n", (u_int)scp,
			       rsp_pcbinfo.watchfds[i].fd, rsp_pcbinfo.watchfds[i].revents);
			if(scp == NULL) {
				fprintf (stderr, "fd:%d does not belong to any scope? - nulling",
					 rsp_pcbinfo.watchfds[i].fd);
				rsp_pcbinfo.watchfds[i].fd = -1;
				rsp_pcbinfo.watchfds[i].events = 0;
				rsp_pcbinfo.watchfds[i].revents = 0;
				ret--;
			} else {
				printf("Processing this sd:%d\n", scp->sd);
				count_processed++;
				rsp_process_fd_for_scope (scp);
				rsp_pcbinfo.watchfds[i].revents = 0;
				ret--;
			}
		}
		if(ret == howmany) {
			break;
		}
	}
	printf("Return %d\n", howmany);
	return(howmany);
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
	sdata = (struct rsp_socket *) malloc(sizeof(struct rsp_socket));
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
	printf("Added sdata:%x scope:%x\n", (u_int)sdata, (u_int)sdata->scp);
	/* FIX? If we refcount scp, we need a bump here? */

	sd = socket(domain, type, protocol);
	if(sd == -1) {
		free(sdata);
		return (sd);
	}
	if((type == SOCK_SEQPACKET) || (protocol == IPPROTO_SCTP)) {
		struct sctp_event_subscribe event;
		socklen_t len;
		len = sizeof(event);
		if (getsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, &len) != 0) {
			fprintf(stderr, "Can't do GET_EVENTS socket option! err:%d\n", errno);
			goto skip_it;
		}
		/* We want these events, not sure how to pass them
		 * to the app if they want them.... maybe we could
		 * hold internal flags that could then key us
		 * to pass the event on after we use it :-D FIX FIX?
		 */
		event.sctp_data_io_event = 1;
		event.sctp_association_event = 1;
		event.sctp_send_failure_event = 1;
		event.sctp_shutdown_event = 1;
		if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
			fprintf(stderr, "Can't do SET_EVENTS socket option! err:%d\n", errno);
		}
	}
 skip_it:
	/* setup and bind the port */
	sdata->sd = sd;
	sdata->port = 0; /* unbound */
	sdata->type = type;
	sdata->domain = domain;
	sdata->protocol = protocol;

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
									sdata->sd, 		        /* key-int */
									(void*)sdata , 		    /* dataum */
									(void *)&sdata->sd, 	/* keyp */
									sizeof(sdata->sd))) ) {	/* size of key */
		fprintf(stderr, "Failed to enter into hash table error:%d\n", ret);
		goto error_out;
	}
	printf("Returning sd:%d\n", sd);
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
   */
  struct rsp_pool *pool;
  struct rsp_socket *sd;
  struct rsp_enrp_scope *scp;

  /* steps:
   *
   * 1) see if we have a hs, if not start blocking
   *    server hunt procedures.
   * 2) once we have a hs, lookup the name -> cache
   * 3) If cache hit, request non-blocking update.
   * 4) If cache miss, request blocking update.
   *
   */

  if (rsp_inited == 0) {
  out:
	errno = EINVAL;
	return (-1);
  }

  sd = rsp_find_socket_with_sd(sockfd);
  if(sd == NULL) {
	goto out;
  }
  scp = sd->scp;

  /* now we have a socket, lets look for the name */
  pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache , name, namelen, NULL);
  if ((pool == NULL) ||
	  (pool->state == RSP_POOL_STATE_REQUESTED) ||
	  (pool->state == RSP_POOL_STATE_NOTFOUND)
	  ) {
	/* we need to get the name first */
	rsp_enrp_make_name_request(sd, pool, name, namelen, 0);
  } else if ((pool->auto_update == 0) &&
			 (pool->state != RSP_POOL_STATE_TIMEDOUT)){
	/* check to see if its aged and needs update */
	struct timeval now;
	gettimeofday(&now, NULL);
	if ((now.tv_sec - pool->received.tv_sec) >= (sd->timers[RSP_T7_ENRPOUTDATE]/1000)) {
	  pool->state = RSP_POOL_STATE_TIMEDOUT;
	  rsp_enrp_make_name_request(sd, pool, name, namelen, MSG_DONTWAIT);
	}
  }
  /* If we wanted to pre-setup an assoc, we would do it
   * here. Right now I don't want to and I think its ok
   * just to get the caceh sync'd.. the first send will
   * implictly setup the assoc getting a piggyback CE+Data.
   */
  return (0);
}

int
rsp_register_sctp(int sockfd, const char *name, size_t namelen,
        uint32_t policy, uint32_t pvalue)
{
    struct rsp_socket *sdata;
    struct rsp_enrp_scope *scp;
    struct rsp_pool_ele *pes;
    struct rsp_register_params params;
    struct asap_error_cause cause;
    socklen_t siz = sizeof(socklen_t);
    uint16_t port;

    if (rsp_inited == 0) {
        errno = EINVAL;
        return (-1);
    }

    sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
    if(sdata == NULL) {
        /* sockfd is not one of ours */
        errno = EINVAL;
        return (-1);
    }
    printf("sdata ret:%x scp:%x\n",
           (u_int)sdata, (u_int)sdata->scp);

    scp = sdata->scp;
    if(scp == NULL) {
        errno = EFAULT;
        return (-1);
    }

    /* Connect to an enrp server if not connected*/
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

    bzero(&params, sizeof(struct rsp_register_params));
    params.transport_use = 1;
    params.protocol_type = RSP_PARAM_SCTP_TRANSPORT;
    params.policy = policy;
    params.policy_value = pvalue;
    params.reglife = -1; /* FIX read it from configfile */

    if (getsockopt(scp->sd, IPPROTO_SCTP, SCTP_GET_LOCAL_ADDR_SIZE,  &params.socklen, &siz) < 0) {
        perror("Failed to get addr size");
        return (-1);
    }

    /* Get the laddrs from sctp socket */
    params.cnt_of_addr = sctp_getladdrs(scp->sd, scp->homeServer->asocid, &(params.taddr));
    if (params.cnt_of_addr < 0) {
        perror("Failed to get laddrs");
        return(-1);
    }

    printf("SCTP listen port: %d\n", port);

    pes = asap_create_pool_ele(scp, name, namelen, &params);
    if (pes == NULL) {
        errno = EFAULT;
        return (-1);
    }

    rsp_enrp_make_register_request(sdata, pes->pool, pes, 0, &cause);

    listen(scp->sd, 10);

    return (pes->pe_identifer);
}

int
rsp_register(int sockfd, const char *name, size_t namelen,
        struct rsp_register_params *params,
        struct asap_error_cause *cause)
{
    struct rsp_socket *sdata;
    struct rsp_enrp_scope *scp;
    struct rsp_pool_ele *pes;

    if (rsp_inited == 0) {
        errno = EINVAL;
        return (-1);
    }

    if (params == NULL) {
        errno = EINVAL;
        return(-1);
    }

    sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
    if(sdata == NULL) {
        /* sockfd is not one of ours */
        errno = EINVAL;
        return (-1);
    }
    printf("sdata ret:%x scp:%x\n",
           (u_int)sdata, (u_int)sdata->scp);

    scp = sdata->scp;
    if(scp == NULL) {
        errno = EFAULT;
        return (-1);
    }

    /* Connect to an enrp server if not connected*/

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
    pes = asap_create_pool_ele(scp, name, namelen, params);
    if (pes == NULL) {
        errno = EFAULT;
        return (-1);
    }

    rsp_enrp_make_register_request(sdata, pes->pool, pes, 0, cause);

    return (pes->pe_identifer);
}

int
rsp_deregister(int sockfd, const char *name, size_t namelen, uint32_t pe_identifier,
               struct asap_error_cause *cause)
{
    struct rsp_socket *sdata;
    struct rsp_enrp_scope *scp;
    struct rsp_pool_ele *pes;
    struct rsp_pool *pool;

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}

    sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
    if(sdata == NULL) {
        /* sockfd is not one of ours */
        errno = EINVAL;
        return (-1);
    }
    printf("sdata ret:%x scp:%x\n",
           (u_int)sdata, (u_int)sdata->scp);

    scp = sdata->scp;
    if(scp == NULL) {
        errno = EINVAL;
        return (-1);
    }
    pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache, name, namelen, NULL);
    if (pool == NULL) {
        errno = EINVAL;
        return (-1);
    }

    pes = asap_find_pe(pool, pe_identifier);
    if (pes == NULL) {
        errno = ENOENT;
        return -1;
    }

    rsp_enrp_make_deregister_request(sdata, pool, pes, 0, cause);
	return (0);
}

int
rsp_lookup(int sockfd, const char *name, size_t namelen, int *protocol_type,
        struct sockaddr *addr)
{
    struct rsp_socket *sdata;
    struct rsp_enrp_scope *scp;
    struct rsp_pool_ele *pe;
    struct rsp_pool *pool;

    if (rsp_inited == 0) {
        errno = EINVAL;
        return (-1);
    }

    sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
    if(sdata == NULL) {
        /* sockfd is not one of ours */
        errno = EINVAL;
        return (-1);
    }
    printf("sdata ret:%x scp:%x\n",
           (u_int)sdata, (u_int)sdata->scp);

    scp = sdata->scp;
    if(scp == NULL) {
        errno = EINVAL;
        return (-1);
    }

    /* named based hunting, imply's using
     * load balancing policy after finding name.
     */
    pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
    if(pool == NULL) {
        /* need to block and get info, first is
         * there already a request pending for this.
         * If so just join sleep, if not create and
         * send, then sleep.
         */
        rsp_connect(sockfd, name, namelen);
        printf("Connect returns, lookup pool\n");
        pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
    }
    /* If we fall to here we have a pool to send to */
    if ((pool == NULL) || (pool->state == RSP_POOL_STATE_NOTFOUND)) {
        errno = ENOENT;
        return (-1);
    }
    pe = rsp_server_select(pool);
    printf("Pick a pe\n");

    if (pe == NULL) {
        errno = ENOENT;
        return(-1);
    }
    /* Collect the address info and return to the called */
    *protocol_type = pe->protocol_type;
    addr = malloc(pe->number_of_addr * pe->len);
    if (addr == NULL) {
        errno = EINVAL; // FIX
        return (-1);
    }
    memcpy(addr, pe->addrList, pe->number_of_addr * pe->len);
    return (0);

}
int
rsp_getPoolInfo(int sockfd, char *name, size_t namelen)
{
  struct rsp_socket *sd;
  struct rsp_pool *pool;
  struct rsp_enrp_scope *scp;


  if (rsp_inited == 0) {
	errno = EINVAL;
	return (-1);
  }
  if (name == NULL) {
	errno = EINVAL;
	return (-1);
  }

  sd = rsp_find_socket_with_sd(sockfd);
  if(sd == NULL) {
	errno = EFAULT;
	goto out;
  }
  scp = sd->scp;

  pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
  if (pool == NULL) {
	rsp_enrp_make_name_request(sd, pool, name, namelen, 0);
	pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
	if (pool == NULL) {
	  goto no_entries;
	}
  }
  if (pool->state == RSP_POOL_STATE_NOTFOUND) {
  no_entries:
	return (0);
  } else {
	return(pool->refcnt);
  }
 out:
  return (-1);
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
	    socklen_t tolen,
	    const char *name,
	    size_t namelen,
	    struct sctp_sndrcvinfo *sinfo,
	    int flags)         /* Options flags */
{
	struct rsp_socket *sdata;
	struct rsp_pool *pool;
	struct rsp_enrp_scope *scp;
	struct rsp_pool_ele  *pe;
	int found_asocid=0;
	int ret;

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

	sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
	if(sdata == NULL) {
		/* sockfd is not one of ours */
		errno = EINVAL;
		return (-1);
	}
	printf("sdata ret:%x scp:%x\n",
	       (u_int)sdata, (u_int)sdata->scp);

	scp = sdata->scp;
	if(scp == NULL) {
		errno = EFAULT;
		return (-1);
	}
	if(name == NULL) {
		/* must be a existing PE, check ipaddr and assoc-id */
		if ((sinfo) && sinfo->sinfo_assoc_id) {
			pe = (struct rsp_pool_ele  *)HashedTbl_lookupKeyed(scp->asocidHash, 
									   sinfo->sinfo_assoc_id, 
									   &sinfo->sinfo_assoc_id, 
									   sizeof(sctp_assoc_t), 
									   NULL);
			if(pe)
				found_asocid = 1;
		}
		if ((pe == NULL) && to)
			pe = (struct rsp_pool_ele  *)HashedTbl_lookup(scp->ipaddrPortHash, to, to->sa_len, NULL);
		if(pe == NULL) {
			if(to) {
				goto send_no_pe;
			}
			goto no_entry;
		}
		pool = pe->pool;
	} else {
		/* named based hunting, imply's using
		 * load balancing policy after finding name.
		 */
		pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
		if(pool == NULL) {
			/* need to block and get info, first is
			 * there already a request pending for this.
			 * If so just join sleep, if not create and 
			 * send, then sleep.
			 */
			rsp_connect(sockfd, name, namelen);
			printf("Connect returns, lookup pool\n");
			pool = (struct rsp_pool *)HashedTbl_lookup(scp->cache ,name, namelen, NULL);
		}
		/* If we fall to here we have a pool to send to */
		if ((pool == NULL) || (pool->state == RSP_POOL_STATE_NOTFOUND)) {
		no_entry:
			errno = ENOENT;
			return (-1);
		}
		pe = rsp_server_select(pool);
	}
	printf("Pick a pe\n");
	pool->lastUsed = pe;
	/* Ok, we can send to our peer or new peer */
	ret = 0;
	if(pe->protocol_type != RSP_PARAM_SCTP_TRANSPORT) {
		return(sendto(sdata->sd, msg, len, flags, pe->addrList, pe->addrList[0].sa_len));
	}
	if(sinfo) {
		if(pe->asocid) {
			sinfo->sinfo_assoc_id = pe->asocid;
			ret = sctp_send(sdata->sd, msg, len, sinfo, flags);
			if(found_asocid == 0) {
				goto update_asocid;
			}
		} else {
			goto use_addrs_collect_associd;
		}
	} else {
		struct sockaddr *sa;
		int cnt;

	use_addrs_collect_associd:
	    cnt = pe->number_of_addr;
		sa = pe->addrList;
		while(sa->sa_family == AF_INET6) {
			sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
			cnt--;
			printf("Skip v6,on to next:%d cnt:%d\n",
			       sa->sa_family, cnt);
			if(cnt <= 0) {
				printf("Sorry no V4 addresses\n");
				return(-1);
			}
		}
		ret = sctp_sendx (sdata->sd, msg, len, sa, cnt,
				  sinfo, flags);
/*
		ret = sctp_sendx (sdata->sd, msg, len, pe->addrList, pe->number_of_addr,
				  sinfo, flags);
*/
	update_asocid:
		pe->asocid = get_asocid(sdata->sd, pe->addrList);
		/* now enter it into the db */
		HashedTbl_enterKeyed(scp->asocidHash, pe->asocid, pe, &pe->asocid, sizeof(sctp_assoc_t));
		if(sinfo) {
			sinfo->sinfo_assoc_id = pe->asocid;
		}
	}
	return (ret);
 send_no_pe:	
	return(sendto(sdata->sd, msg, len, flags, to, tolen));
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
rsp_internal_poll(nfds_t nfds, int timeout, int ret_from_enrp)
{
	struct rsp_timer_entry *entry;
	struct timeval now;
	int min_timeout, poll_ret, ret;
	int rem_to = 0;

	if (gettimeofday(&now , NULL) ) {
		fprintf(stderr, "Gak, system error can't get time of day?? -- failed:%d\n", errno);
		return(-1);
	}
	if(nfds < rsp_pcbinfo.num_fds) {
		printf("Warning, nfds:%d can't be smaller than %d\n",
		       nfds,
		       rsp_pcbinfo.num_fds);
		nfds = rsp_pcbinfo.num_fds;
	}
	while (1) {
		/* Deal with any timers */
		dlist_reset(rsp_pcbinfo.timer_list);
		entry = (struct rsp_timer_entry *)dlist_get( rsp_pcbinfo.timer_list);
		if(entry == NULL) {
			/* none left, we are done */
			goto do_poll;
		}
		/* Has it expired? */
		if ((now.tv_sec > entry->expireTime.tv_sec) ||
		    ((entry->expireTime.tv_sec ==  now.tv_sec) &&
		     (now.tv_sec >= entry->expireTime.tv_usec))) {
			/* Yep, this one has expired */
			printf("Expire entry %x\n", (u_int)entry);
			rsp_expire_timer(entry);
			/* Go get the next one, this works because
			 * rsp_expire_timer removes the head of the list
			 * aka the one that just expired. So we come
			 * back to the while(1) and reset, and get the
			 * next one. We consume all timers ready to go until
			 * we reach a point where one with time is left, or none
			 * our left.
			 */
			continue;
		}
		/* ok, at this point entry points to an un-expired timer and
		 * the next one to expire at that... so adjust its time.
		 */
		min_timeout = rsp_pcbinfo.minimumTimerQuantum;
		if(entry) {
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
		}
		if(min_timeout < 1) {
			min_timeout = rsp_pcbinfo.minimumTimerQuantum;
		}
		if(min_timeout > rsp_pcbinfo.minimumTimerQuantum)
			min_timeout = rsp_pcbinfo.minimumTimerQuantum;

	do_poll:
		/* delay min_timeout */

		if ((timeout > 0) && (timeout < min_timeout)) {
			rem_to = 1;
			min_timeout = timeout;
		} else {
			rem_to = 0;
		}
		poll_ret = poll(rsp_pcbinfo.watchfds, nfds , min_timeout);
		if(poll_ret)
			printf("Poll returns %d\n", poll_ret);
		if(poll_ret > 0) {
			/* we have some to deal with */
			printf("call process_fds\n");
			ret = rsp_process_fds(poll_ret);
			printf("ret is %d\n", ret);
			if ((poll_ret - ret) > 0) {
				printf("ret %d\n", (poll_ret - ret));
				return(poll_ret - ret);
			} else if(ret_from_enrp) {
				printf("ret_from_enrp:%d\n", ret_from_enrp);
				return (0);
			}
		}
		if(poll_ret < 0) {
			/* we have an error to deal with? */
			/*fprintf(stderr, "Error in poll?? errno:%d\n", errno);*/
		}

		if ((poll_ret == 0) && rem_to) {
			printf("poll ret 0\n");
			return (0);
		}

		if (gettimeofday(&now , NULL) ) {
			fprintf(stderr, "Gak, system error can't get time of day?? -- failed:%d\n", errno);
		}
	}
}



ssize_t 
rsp_rcvmsg(int sockfd,		/* HA socket descriptor */
	   char *msg,
	   size_t len,
	   char *name, 		/* in-out/limit */
	   size_t *namelen,
	   struct sockaddr *from,
	   socklen_t *fromlen,	/* in-out/limit */
	   struct sctp_sndrcvinfo *sinfo,
	   int flags)		/* Options flags */
{
	int ret;
	struct rsp_socket *sdata;
	struct rsp_enrp_scope *scp;
	struct rsp_pool_ele  *pe;
	struct sctp_sndrcvinfo lsinfo;

	if (rsp_inited == 0) {
		errno = EINVAL;
		return (-1);
	}
	/* Game plan for receive 
	 * 1) We do a select, which will cause our
	 *    enrp messages to be processed like they should.
	 *    Note:check to see if we handle the keepalive and
	 *         send a keepalive response. NOPE, FIX FIX
	 * 2) When we have something on our sockfd, then read
	 *    it in.
	 * 3) If its a notification then pass it to the handler for
	 *    our notifications. Note: do we need to keep shadow
	 *    state for notifications we want, or do we just make
	 *    the user loose them?
	 * 3a) If its a ppid of ASAP, then we need to 
	 *     process it has a cookie or bc
	 * 4) If a message, then send it back to the user but
	 *    fill in the name if possible to go with the address
	 *    and associd.
	 */
	/* First find the socket stuff */
	sdata  = (struct rsp_socket *)rsp_find_socket_with_sd(sockfd);
	if(sdata == NULL) {
		/* sockfd is not one of ours */
		errno = EINVAL;
		return (-1);
	}
	if ((sdata->protocol == IPPROTO_SCTP) && (sinfo == NULL)) {
		errno = EINVAL;
		return (-1);
	}

	scp = sdata->scp;

	if ((rsp_pcbinfo.num_fds+1) > rsp_pcbinfo.siz_fds) {
		/* grow it big enough */
		rsp_expand_watchfd_array();
	}
	if ((rsp_pcbinfo.num_fds+1) > rsp_pcbinfo.siz_fds) {
		errno = EFAULT;
		return(-1);
	}
 try_again:
    flags = 0; /* flags from previous recvmsg might mess up subsequent recvmsg calls */
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].fd = sdata->sd;
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].events = POLLIN;
	rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].revents = 0;
	printf("Looking for data on rsp_pcbinfo.num_fds:%d sd:%d\n",
	       rsp_pcbinfo.num_fds, sdata->sd);

	ret = rsp_internal_poll(rsp_pcbinfo.num_fds+1, INFTIM, 0);
	if(ret < 1) {
		if(ret < 0) {
			if(errno == EAGAIN)
				goto try_again;

			fprintf(stderr, "Got error:%d on poll\n", errno);
			return(-1);
		}
		printf("Try again\n");
		goto try_again;
	}
	if (rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].revents == 0) {
		/* not me? */
		printf("Not mine? %d\n", rsp_pcbinfo.watchfds[rsp_pcbinfo.num_fds].revents);
		goto try_again;
	}
	/* at this point we have an event on the fd we want! 
	 * and are at 2)
	 */
	printf("Got an event\n");
	if(sdata->protocol == IPPROTO_SCTP) {
		/* SCTP */
		if(sinfo == NULL) {
			sinfo = &lsinfo;
		}
		ret = sctp_recvmsg(sdata->sd, msg, len,
				   from, fromlen, sinfo, &flags);
		printf("ret:%d sinfo->sinfo_flags:%x flags:%x ppid:%x msg:%s\n",
		       ret,
		       sinfo->sinfo_flags,
		       flags,
		       ntohl(sinfo->sinfo_ppid),
		       msg);

		if (flags & MSG_NOTIFICATION) {
			printf("Its a notification\n");
			handle_asap_sctp_notification(sdata, msg, ret, from, *fromlen, sinfo, flags);
			printf("Try again\n");
			goto try_again;
		} else if(sinfo->sinfo_ppid == htonl(RSERPOOL_ASAP_PPID)) {
			printf("Its a asap mag\n");
			handle_asap_message(sdata, msg, ret, from, *fromlen, sinfo, flags);
			printf("Try again\n");
			goto try_again;
		} 
	} else {
		/* UDP */
		return(recvfrom(sdata->sd, msg, len, flags,
				from, fromlen));

	}
	if(name) {
		pe = (struct rsp_pool_ele  *)HashedTbl_lookupKeyed(scp->asocidHash, 
								   sinfo->sinfo_assoc_id, 
								   &sinfo->sinfo_assoc_id, 
								   sizeof(sctp_assoc_t), 
								   NULL);
		if(pe) {
			int copy_max;
			if(*namelen < pe->pool->name_len)
				copy_max = *namelen;
			else {
				copy_max = pe->pool->name_len;
				*namelen = copy_max;
			}
			memcpy(name, pe->pool->name, copy_max);
		} else if (name) {
			*namelen = 0;
		}
	}
	return (ret);
}


int
rsp_select(int nfds, 
	   fd_set *readfds,
	   fd_set *writefds,
	   fd_set *exceptfds,
	   struct timeval *timeout
	   )
{
	/* convert the rsp_select into the under-lying
	 * rsp_poll that it is.
	 */
	int ms;
	int at,nat, incr_needed, added_fds;
	int ret,nret, didone;

	if(timeout == NULL) {
		/* Infinity */
		ms = INFTIM;
	} else if ((timeout->tv_sec == 0) && (timeout->tv_usec == 0)) {
		/* immediate return */
		ms = 0;
	} else {
		/* some time */
		ms = (timeout->tv_sec * 1000) + (timeout->tv_sec/1000);
	}
	at = rsp_pcbinfo.num_fds;
	nat = 0;
	added_fds = 0;
	while(nat <= nfds) {
		if(at >= rsp_pcbinfo.siz_fds) {
			rsp_expand_watchfd_array();
		}
		incr_needed = 0;
		if(readfds) {
			if(FD_ISSET(nat, readfds)) {
				incr_needed = 1;
				rsp_pcbinfo.watchfds[at].fd = nat;
				rsp_pcbinfo.watchfds[at].events |= POLLIN;
				rsp_pcbinfo.watchfds[at].revents = 0;
			}
		}
		if(writefds) {
			if (FD_ISSET(nat, writefds)) {
				incr_needed = 1;
				rsp_pcbinfo.watchfds[at].fd = nat;
				rsp_pcbinfo.watchfds[at].events |= POLLOUT;
				rsp_pcbinfo.watchfds[at].revents = 0;
			} 
		}
		if(exceptfds) {
			if (FD_ISSET(nat, exceptfds)) {
				incr_needed = 1;
				rsp_pcbinfo.watchfds[at].fd = nat;
				rsp_pcbinfo.watchfds[at].events |= POLLPRI;
				rsp_pcbinfo.watchfds[at].revents = 0;
			} 
		}
		if(incr_needed) {
			at++;
			added_fds++;
		}
		nat++;
	}
	ret = rsp_internal_poll((nfds_t)(added_fds+rsp_pcbinfo.num_fds), ms, 0);
	nret = ret;

	if(readfds) {
		FD_ZERO(readfds);
	}
	if(writefds) {
		FD_ZERO(writefds);
	}
	if(exceptfds) {
		FD_ZERO(exceptfds);
	}
	at = rsp_pcbinfo.num_fds;
	while(nret > 0) {
		if(at >= rsp_pcbinfo.siz_fds)
			break;
		didone = 0;
		if (rsp_pcbinfo.watchfds[at].revents & POLLIN) {
			didone = 1;
			if(readfds) {
				FD_SET(rsp_pcbinfo.watchfds[at].fd, readfds);
			}
		}
		if (rsp_pcbinfo.watchfds[at].revents & POLLOUT) {
			didone = 1;
			if(writefds) {
				FD_SET(rsp_pcbinfo.watchfds[at].fd, writefds);
			}
		}
		if (rsp_pcbinfo.watchfds[at].revents & POLLPRI) {
			didone = 1;
			if(exceptfds) {
				FD_SET(rsp_pcbinfo.watchfds[at].fd, exceptfds);
			}
		}
		if(didone)
			nret--;
		at++;
	}
	return(ret);
}

int
rsp_poll ( struct pollfd fds[], nfds_t nfds, int timeout)
{
	int ret, i;
	i=0;
	while ((nfds + rsp_pcbinfo.num_fds) > rsp_pcbinfo.siz_fds) {
		/* grow it big enough */
		i++;
		rsp_expand_watchfd_array();
		if(i > RSP_GROW_LIMIT) {
			errno = EFAULT;
			return(-1);
		}
	}
	for(i=0; i<nfds; i++) {
		rsp_pcbinfo.watchfds[(i+rsp_pcbinfo.num_fds)] = fds[i];
	}
	ret = rsp_internal_poll((nfds_t)(nfds+rsp_pcbinfo.num_fds), timeout, 0);
	for(i=0; i<nfds; i++) {
		fds[i] = rsp_pcbinfo.watchfds[(i+rsp_pcbinfo.num_fds)];
	}
	return(ret);
}


int32_t
rsp_initialize(struct rsp_info *info)
{
	/* First do we do major initialization? */
	int32_t id;
	if (rsp_inited == 0) {
		/* yep */
		if(rsp_init() != 0) {
			return(0);
		}
		rsp_inited = 1;
	}
	id = rsp_load_config_file(info->rsp_prefix);
	return(id);
}
