#ifndef __Rserpool_h__
#define __Rserpool_h__


int rsp_socket();

int rsp_close(int sockfd);

int rsp_connect(sockfd, name);

int rsp_register(sockfd, name);

int rsp_deregister(sockfd, name);

struct xxx rsp_getPoolInfo(sockfd, xxx );

int rsp_reportfailure(sockfd, xxx);

size_t rsp_sendmsg(int sockfd,         /* HA socket descriptor */
                   struct msghdr *msg, /* message header struct */
                   int flags);         /* Options flags */

ssize_t rsp_rcvmsg(int sockfd,         /* HA socket descriptor */
                   struct msghdr *msg, /* msg header struct */
                   int flags);         /* Options flags */


int rsp_forcefailover(sockfd, xxx);

#endif
