#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#define PORT 5001
#define ADDR "127.0.0.1"
#define BUFFER_SIZE ((1<<16)+1024)

int sctp_enable_non_blocking(int fd)
{
        int flags;
        
        flags = fcntl(fd, F_GETFL, 0);
        return fcntl(fd, F_SETFL, flags  | O_NONBLOCK);
}

int main(int argc, char **argv) {
  int fd, n;
  struct sockaddr_in to;
  char *buffer;
  unsigned int size;
  int non_blocking = 0;
  int i;
  int port = PORT;
  
  while((i= getopt(argc,argv,"np")) != EOF) {
	switch(i) {
	case 'n':
	  non_blocking = 1;
	  break;
	case 'p':
	  port = atoi(optarg);
	default:
	  break;
	}
  }
  buffer = (char *)malloc(BUFFER_SIZE);
  memset((void *)buffer, 0, BUFFER_SIZE);

  memset((void *)&to, 0, sizeof(struct sockaddr_in));
  to.sin_len         = sizeof(struct sockaddr_in);
  to.sin_family      = AF_INET;
  to.sin_port        = htons(port);
  to.sin_addr.s_addr = inet_addr(argv[optind]);

  if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
	perror("socket");
  }

  if (connect(fd, (struct sockaddr *)&to, sizeof(struct sockaddr_in)) < 0) {
	perror("connect");
  }
	
  if(non_blocking) {
	printf("Setting to non-blocking\n");
	sctp_enable_non_blocking(fd);
  } else {
	printf("leaving in blocking mode\n");
  }

  do {
	size = random() % BUFFER_SIZE + 1;
	do {
	  //n = send(fd, (const void *)buffer, size, 0);
	  n = sctp_sendmsg(fd, (const void *)buffer, size, NULL, 0, 0, 0, 0, 0, 0);
	} while ((n == -1) && (errno == EWOULDBLOCK));
  } while (n == size);

  if (n == -1) {
	printf("errno = %d (%s).\n", errno, strerror(errno));
  } else {
	printf("sctp_sendmsg() returned %d instead of %d.\n", n, size);
  }
	
  if (close(fd) < 0) {
	perror("close");
  }
	
  return(0);
}
