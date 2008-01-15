#ifndef __Rserpool_h__
#define __Rserpool_h__

#define RSP_PREFIX_SIZE 128

struct rsp_info {
	char rsp_prefix[RSP_PREFIX_SIZE];
};


int32_t rsp_initialize(struct rsp_info *);

int rsp_socket(int domain, int type,  int protocol, uint32_t op_scope);

int rsp_close(int sockfd);

int rsp_connect(int sockfd, const char *name, size_t namelen);

int
rsp_register_sctp(int sockfd, const char *name, size_t namelen, uint32_t policy, uint32_t pvalue);

int
rsp_register(int sockfd, const char *name, size_t namelen, struct rsp_register_params *params,
        struct asap_error_cause *cause);

/*
int
rsp_register(int sockfd, const char *name, size_t namelen,
        uint16_t ptype, uint16_t tuse, const struct sockaddr *taddr, socklen_t len, uint32_t cnt_of_addr,
        uint32_t policy, uint32_t policy_value, struct asap_error_cause *cause);
*/

int
rsp_deregister(int sockfd, const char *name, size_t namelen, uint32_t pe_identifier,
        struct asap_error_cause *cause);

int
rsp_lookup(int sockfd, const char *name, size_t namelen, int *protocol_type, struct sockaddr *addr);

int 
rsp_getPoolInfo(int sockfd, char *name, size_t namelen);


int 
rsp_reportfailure(int sockfd, char *name,size_t namelen,  const struct sockaddr *to, const sctp_assoc_t id);

size_t 
rsp_sendmsg(int sockfd, const char *msg, size_t len, struct sockaddr *to,
	    socklen_t tolen, const char *name, size_t namelen, struct sctp_sndrcvinfo *sinfo,
	    int flags); 


ssize_t 
rsp_rcvmsg(int sockfd, char *msg, size_t len, char *name,
	   size_t *namelen, struct sockaddr *from, socklen_t *fromlen,
	   struct sctp_sndrcvinfo *sinfo, int flags);

int
rsp_poll ( struct pollfd fds[], nfds_t nfds, int timeout);

int
rsp_select(int nfds, 
	   fd_set *readfds,
	   fd_set *writefds,
	   fd_set *exceptfds,
	   struct timeval *timeout);

#endif
