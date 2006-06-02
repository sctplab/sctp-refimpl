#include <rserpool_util.h>
#include <rserpool_lib.h>
#include <rserpool.h>
#include <rserpool_io.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

struct rsp_global_info rsp_pcbinfo;
int rsp_inited = 0;
int rsp_debug = 1;

static
void rsp_timer_thread_run( void * )
{
	void *v;
	if (pthread_mutex_lock(&rsp_pcbinfo.rsp_tmr_mtx)) {
		fprintf(stderr, "Pre-Timer start thread mutex lock fails error:%d -- help\n",
			errno);
		return;
	}
	while (1) {
		if(pthread_cond_wait(&rsp_pcbinfo.rsp_tmr_cnd, 
				     &rsp_pcbinfo.rsp_tmr_mtx)) {
			fprintf(stderr, "Condition wait fails error:%d -- help\n", errno);
			return;
		}
		dlist_reset(rsp_pcbinfo.timer_list);
		v = dlist_get( rsp_pcbinfo.timer_list);
		if(v == NULL) {
			/* none on list.. hmm false wakeup */
			continue;
		}
		rsp_timer_check ((struct rsp_timer_entry *)v);
	}

}


static int
rsp_init()
{
	if (rsp_inited)
		return;


	rsp_pcbinfo.rsp_timers_up = 0;
	rsp_pcbinfo.rsp_number_sd = 0;

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
	if(pthread_mutex_cond_init(&rsp_pcbinfo.rsp_tmr_cnd, NULL)) {
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
			  NULL) ) {
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

int
rsp_socket(int domain, int protocol, uint16_t port)
{
	int sd;
	struct rsp_socket_hash *sdata;
	struct sockaddr *sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	socklen_t addrlen;

	if(protocol != IPPROTO_SCTP) {
		errno = ENOSUPPORT;
		return (-1);
	}
	sd = socket(domain, SOCK_SEQPACKET, protocol);
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
	if(domain == AF_INET) {
		memset(&sin, 0, sizeof(sin));
		sin.sin_port = port;
		sa = (struct sockaddr *)&sin;
		addrlen = sizeof(struct sin);
	}else if (domain == AF_INET6) {
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_port = port;
		sa = (struct sockaddr *)&sin6;
		addrlen = sizeof(struct sin6);
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
		goto out_ofb:
	}

	/* ipadd -> rsp_pool_element */
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
	}

	/* Home ENRP server address list*/
	sdata->enrpAddrList = dlist_create();
	if (sdata->enrpAddrList == NULL) {
		if(rsp_debug) {
			fprintf(stderr, "can't get memory for enrp home addr dlist\n");
		}
/*	out_ofg: to add more start here */
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

	
}

int 
rsp_register(sockfd, name)
{
}

int 
rsp_deregister(sockfd, name)
{
}

int 
rsp_close(int sockfd)
{
}

int 
rsp_connect(sockfd, name)
{
}


struct xxx 
rsp_getPoolInfo(sockfd, xxx )
{
}

int 
rsp_reportfailure(sockfd, xxx)
{
}

ssize_t 
rsp_sendmsg(int sockfd,         /* HA socket descriptor */
            struct msghdr *msg, /* message header struct */
            int flags)         /* Options flags */
{
}

ssize_t 
rsp_rcvmsg(int sockfd,         /* HA socket descriptor */
           struct msghdr *msg, /* msg header struct */
           int flags)         /* Options flags */
{
}


int rsp_forcefailover(sockfd, xxx)
{
}
