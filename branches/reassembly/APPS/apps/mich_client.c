#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/sctp.h>

int main()
{
   int fd;
   struct sockaddr_in addr;
   const char message = 'A';

   memset((void *)&addr, 0 ,sizeof(struct sockaddr_in));
   addr.sin_family      = AF_INET;
   addr.sin_len         = sizeof(struct sockaddr_in);
   addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   addr.sin_port        = htons(2000);

   if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
     perror("socket");

   if (sctp_sendmsg(fd,
                    (const void *)&message, 1,
                    (struct sockaddr *) &addr, (socklen_t) sizeof 
(struct sockaddr_in),
                    0, MSG_EOF, 0, 0, 0) < 0)
     perror("sctp_sendmsg");

   sleep(1);

   if (close(fd) < 0)
     perror("close");
   return 0;
}
