#ifndef __Rserpool_h__
#define __Rserpool_h__


int rsp_socket(int domain, int protocol, uint16_t port, const char *confdomain);

int rsp_close(int sockfd);

int rsp_connect(int sockfd, const char *name);

int rsp_register(int sockfd, const char *name);

int rsp_deregister(int sockfd, const char *name);

int rsp_getPoolInfo(int sockfd/*, xxx */);

int rsp_reportfailure(int sockfd/*, xxx*/);

size_t rsp_sendmsg(int sockfd,         /* HA socket descriptor */
                   struct msghdr *msg, /* message header struct */
                   int flags);         /* Options flags */

ssize_t rsp_rcvmsg(int sockfd,         /* HA socket descriptor */
                   struct msghdr *msg, /* msg header struct */
                   int flags);         /* Options flags */


int rsp_forcefailover(int sockfd/*, xxx*/);

#endif
