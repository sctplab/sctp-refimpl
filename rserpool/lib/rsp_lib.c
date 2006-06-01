#include <rserpool_lib.h>
#include <rserpool.h>
#include <rserpool_io.h>

struct rsp_global_info rsp_pcbinfo;
int rsp_inited = 0;

static int
rsp_init()
{
	if (rsp_inited)
		return;


	rsp_pcbinfo.rsp_timers_up = 0;
	rsp_pcbinfo.rsp_number_sd = 0;

	rsp_pcbinfo.sd_pool = HashedTbl_create(RSP_SD_HASH_TABLE_NAME, 
					       RSP_SD_HASH_TBL_SIZE);
	if(rsp_pcbinfo.sd_pool == NULL) {
		return (-1);
	}
	rsp_pcbinfo.timer_list = dlist_create();
	if (rsp_pcbinfo.timer_list == NULL) {
		HashedTbl_destroy(rsp_pcbinfo.sd_pool);
		return (-1);
	}

pthread_mutex_t		sd_pool_mtx;
pthread_cond_t		rsp_tmr_cnd;
pthread_mutex_t		rsp_tmr_mtx;
	
pthread 		tmr_thread; 	
	/* set the flag that we init'ed */
	rsp_inited = 1;
}

int
rsp_socket(int domain, int protocol, uint16_t port)
{
	
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
