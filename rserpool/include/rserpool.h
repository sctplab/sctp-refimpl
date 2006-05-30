#ifndef __Rserpool_h__
#define __Rserpool_h__


ssize_t rsp_socket();

ssize_t rsp_close(int sockfd);

ssize_t rsp_connect(sockfd, name);

ssize_t rsp_register(sockfd, name);

ssize_t rsp_deregister(sockfd, name);

struct xxx rsp_getPoolInfo(sockfd, xxx );

ssize_t rsp_reportfailure(sockfd, xxx);

ssize_t rsp_sendmsg(int sockfd,         /* HA socket descriptor */
                    struct msghdr *msg, /* message header struct */
                    int flags);         /* Options flags */

ssize_t rsp_rcvmsg(int sockfd,         /* HA socket descriptor */
                   struct msghdr *msg, /* msg header struct */
                   int flags);         /* Options flags */


ssize_t rsp_forcefailover(sockfd, xxx);

#endif
