#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/sctp.h>
#include <errno.h>

int main()
{
   int fd;
   struct sockaddr_in addr;
   socklen_t addr_len;

   addr.sin_family      = AF_INET;
   addr.sin_len         = sizeof(struct sockaddr_in);
   addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   addr.sin_port        = htons(2000);

   if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
     perror("socket");

   if (bind(fd, (const struct sockaddr *) &addr, 
(socklen_t)sizeof(struct sockaddr_in)) < 0)
     perror("bind");

   if (listen(fd, 1) < 0)
     perror("listen");

   memset((void *) &addr, 0, sizeof(struct sockaddr_in));
   addr_len = (socklen_t)sizeof(struct sockaddr_in);
   if (accept(fd, (struct sockaddr *) &addr, &addr_len) < 0) {
     perror("accept");
     printf("errno %d\n",errno);
   }
   if (close(fd) < 0)
     perror("close");
   return 0;
}
