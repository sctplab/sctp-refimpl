#ifndef __Rserpool_h__
#define __Rserpool_h__


int rsp_socket(int domain, int protocol, uint16_t port, const char *confdomain);



int rsp_close(int sockfd);

int rsp_connect(int sockfd, const char *name, size_t namelen);


int rsp_register(int sockfd, const char *name, size_t namelen, uint32_t policy, uint32_t policy_value );

int rsp_deregister(int sockfd);


struct rsp_info *
rsp_getPoolInfo(int sockfd, char *name, size_t namelen);


int 
rsp_reportfailure(int sockfd, char *name,size_t namelen,  const struct sockaddr *to, const sctp_assoc_t id);

size_t 
rsp_sendmsg(int sockfd, const char *msg, size_t len, struct sockaddr *to,
	    socklen_t *tolen, char *name, size_t *namelen, struct sctp_sndrcvinfo *sinfo,
	    int flags); 


ssize_t 
rsp_rcvmsg(int sockfd, const char *msg, size_t len, char *name,
	   size_t *namelen, struct sockaddr *from, socklen_t *fromlen,
	   struct sctp_sndrcvinfo *sinfo, int flags);

#endif
