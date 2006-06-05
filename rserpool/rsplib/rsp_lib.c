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

struct rsp_global_info rsp_pcbinfo;



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

	/* create mutex to lock sd pool */
	if (pthread_mutex_init(&rsp_pcbinfo.sd_pool_mtx, NULL)) {
		if(rsp_debug) {
			fprintf(stderr, "Could not init sd_pool mtx\n");
		}
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
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
		pthread_cond_destroy(&rsp_pcbinfo.rsp_tmr_cnd);
		pthread_mutex_destroy(&rsp_pcbinfo.sd_pool_mtx);
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		dlist_destroy(rsp_pcbinfo.timer_list);
		return (-1);
	}
	
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
rsp_add_enrp_server(struct rsp_socket_hash *sdata, uint32_t enrpid, struct sockaddr *sa)
{
	struct rsp_enrp_entry *re;

	dlist_reset(sdata->enrpList);
	while((re = (struct rsp_enrp_entry *)dlist_get(sdata->enrpList)) != NULL) {
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
	re->asocid = 0;
	re->state = RSP_NO_ASSOCIATION;
	rsp_add_addr_to_enrp_entry(re, sa);
	dlist_append(sdata->enrpList, (void *)re);
}

static void
rsp_load_file(struct rsp_socket_hash *sdata, FILE *io, char *file)
{
	/* load the enrp control file opened at io */
	int line=0;
	uint32_t enrpid;
	uint16_t port;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;

	/* Format is
	 * id:port:address
	 *
	 * if port is 0 it goes to the default port for
	 * ENRP.
	 */
	char string[256], *p1, *p2, *p3;
	while(fgets(string, sizeof(string), io) != NULL) {
		line++;
		if(string[0] == '#')
			continue;
		if(string[0] == ';')
			continue;

		p1 = strtok(string,":" );
		if(p1 == NULL) {
			printf("config file %s line %d can't find a ':' seperating id from port\n", file, line);
			continue;
		}
		p2 = strtok(NULL, ":");
		if(p2 == NULL) {
			printf("config file %s line %d can't find a ':' seperating port from address\n", file, line);
			continue;
		}
		p3 = strtok(NULL, "\n");
		if(p2 == NULL) {
			printf("config file %s line %d can't find a terminating char after address?\n", file, line);
			continue;
		}
		/* ok p1 points to a int. p2 points to an address */
		enrpid = strtoul(p1, NULL, 0);
		if(enrpid == 0) {
			printf("config file %s line %d reserved enrpid found aka 0?\n", file, line);
		}
		port = htons((uint16_t)strtol(p2, NULL, 0));
		if(port == 0) {
			port = htons(ENRP_DEFAULT_PORT_FOR_ASAP);
		}
		memset(&sin6, 0, sizeof(sin6));
		if(inet_pton(AF_INET6, p3 , &sin6.sin6_addr ) == 1) {
			sin6.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
			sin6.sin6_len = sizeof(sin6);
#endif			
			sin6.sin6_port = port;
			rsp_add_enrp_server(sdata, enrpid, (struct sockaddr *)&sin6);
			continue;
		}
		memset(&sin, 0, sizeof(sin));
		if(inet_pton(AF_INET, p3 , &sin.sin_addr ) == 1) {
			sin.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
			sin.sin_len = sizeof(sin);
#endif			
			sin.sin_port = port;
			rsp_add_enrp_server(sdata, enrpid, (struct sockaddr *)&sin);
			continue;
		}
		printf("config file %s line %d can't translate the address?\n", file, line);
		
	}
}

static void
rsp_load_config_file(struct rsp_socket_hash *sdata, const char *confprefix)
{
	char file[256];
	char prefix[100];
	FILE *io;

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
		rsp_load_file(sdata, io, file);
		fclose(io);
		return;
	}
	sprintf(file, "/usr/local/etc/.%senrp.conf", prefix);
	if ((io = fopen(file, "r")) != NULL) {
		rsp_load_file(sdata, io, file);
		fclose(io);
		return;
	}
	sprintf(file,"/usr/local/etc/enrp.conf");
	if ((io = fopen(file, "r")) != NULL) {
		rsp_load_file(sdata, io, file);
		fclose(io);
		return;
	}
	sprintf(file, "/etc/enrp.conf");
	if ((io = fopen(file, "r")) != NULL) {
		rsp_load_file(sdata, io, file);
		fclose(io);
		return;
	}
}

void
rsp_start_enrp_server_hunt(struct rsp_socket_hash *sd, struct rsp_timer_entry *te)
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

	if ((te == NULL) && (sd->state & RSP_SERVER_HUNT_IP)) {
		struct rsp_timer_entry *ete;
		int found = 0;

		if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access %d can't look up timed server hunt in progress, \n", errno);
			return;
		}
		dlist_reset(rsp_pcbinfo.timer_list);
		while ((ete = (struct rsp_timer_entry *)dlist_get(rsp_pcbinfo.timer_list)) != NULL) {
			if (ete->timer_type == RSP_T5_SERVERHUNT) {
				found = 1;
				ete->sleeper_count++;
				if(pthread_cond_wait(&rsp_pcbinfo.rsp_tmr_cnd, &rsp_pcbinfo.rsp_tmr_mtx)) {
					fprintf(stderr, "Cond wait for s-h fails error:%d\n", errno);
				}
				break;
			}
		}
		if (pthread_mutex_unlock(&rsp_pcbinfo.rsp_tmr_mtx) ) {
			fprintf(stderr, "Unsafe access, thread unlock failed for s-h rsp_tmr_mtx:%d\n", errno);
		}
		if(found == 0) {
			/* hmm, this should not happen */
			fprintf(stderr, "In server hunt state, but cannot find the timer?\n");
		} else {
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
			if(te) {
				if ((te->cond_awake) && (te->sleeper_count > 0)) {
					pthread_cond_broadcast(&te->rsp_sleeper);
				} 
				pthread_cond_destroy(&te->rsp_sleeper);
				free(te);
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
	rsp_start_timer(sd, sd->timers[RSP_T5_SERVERHUNT], (struct rsp_enrp_req *)NULL, RSP_T5_SERVERHUNT, 1, 0, te);
}

int
rsp_socket(int domain, int protocol, uint16_t port, const char *confprefix)
{
	int sd, ret;
	struct rsp_socket_hash *sdata;
	struct sockaddr *sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	socklen_t addrlen;
	struct sctp_event_subscribe event;

	if(protocol != IPPROTO_SCTP) {
		errno = ENOTSUP;
		return (-1);
	}
	sd = socket(domain, SOCK_SEQPACKET, protocol);
	if(sd == -1) {
		return (sd);
	}
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
	if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		fprintf(stderr, "Can't do SET_EVENTS socket option! err:%d\n", errno);
		close(sd);
		errno = EFAULT;
		return(-1);
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
	if(domain == AF_INET) {
		memset(&sin, 0, sizeof(sin));
		sin.sin_port = port;
		sa = (struct sockaddr *)&sin;
		addrlen = sizeof(sin);
	}else if (domain == AF_INET6) {
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_port = port;
		sa = (struct sockaddr *)&sin6;
		addrlen = sizeof(sin6);
	} else {
		if(rsp_debug) {
			fprintf(stderr, "unknown protocol:%d\n", domain);
		}
		close(sd);
		free(sdata);
		errno = ESOCKTNOSUPPORT;
		return (-1);
	}
	sa->sa_len = addrlen;
	if(bind(sd, sa, addrlen) == -1) {
		if(rsp_debug) {
			fprintf(stderr, "bind of socket failed errno:%d\n", errno);
		}
	out_of_here:
		close(sd);
		free(sdata);
		return (-1);
	} 
	if(getsockname(sd, sa, &addrlen) == -1) {
		if(rsp_debug) {
			fprintf(stderr, "can't do getsockname():%d\n", errno);
		}
		goto out_of_here;
	}
	sdata->port = ((struct sockaddr_in *)sa)->sin_port;

	if (rsp_inited == 0) {
		if(rsp_init() != 0) {
			errno = EFAULT;
			free(sdata);
			close(sd);
			return(-1);
		}
	}
	/* now init the rest of the structures */

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
/*	out_ofg: to add more start here */
		dlist_destroy(sdata->address_reg);
		goto out_off;
	}
	sdata->homeServer = NULL;
	sdata->state = RSP_NO_ENRP_SERVER;

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
	rsp_load_config_file(sdata, confprefix);

	if (pthread_mutex_unlock(&rsp_pcbinfo.sd_pool_mtx) ) {
		fprintf(stderr, "Unsafe access, thread unlock failed for sd_pool_mtx:%d\n", errno);
	}


	/* We need a home ENRP server, start
	 * server hunt procedures.
	 */
	rsp_start_enrp_server_hunt(sdata, (struct rsp_timer_entry *)NULL);
	return(sd);
}

int 
rsp_close(int sockfd)
{
	return (0);
}

int 
rsp_connect(int sockfd, const char *name)
{
	return (0);
}

int 
rsp_register(int sockfd, const char *name)
{
	return (0);	
}

int
rsp_deregister(int sockfd, const char *name)
{
	return (0);
}

int 
rsp_getPoolInfo(int sockfd/*, xxx */)
{
	return (0);
}

int 
rsp_reportfailure(int sockfd/*, xxx*/)
{
	return (0);
}

size_t 
rsp_sendmsg(int sockfd,         /* HA socket descriptor */
	    struct msghdr *msg, /* message header struct */
	    int flags)         /* Options flags */
{
	return (0);
}

ssize_t 
rsp_rcvmsg(int sockfd,         /* HA socket descriptor */
	   struct msghdr *msg, /* msg header struct */
	   int flags)         /* Options flags */
{
	return (0);
}


int 
rsp_forcefailover(int sockfd/*, xxx*/)
{
	return (0);
}


