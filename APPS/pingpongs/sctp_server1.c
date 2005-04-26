#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <poll.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#define MY_MAX_SIZE 230000

unsigned char respbuf[MY_MAX_SIZE];
int num_snds=0;
int num_rcvs=0;
int data_failures=0;
int num_rcv_failed=0;

static int
init_fd(struct sockaddr_in *sin, int fd)
{
  int on=1;
  if(setsockopt(fd,IPPROTO_SCTP,
		SCTP_NODELAY,
		&on, sizeof(on)) != 0) {
    printf("Can't set TCP nodelay %d\n", errno);
    return(-1);
  }
  if(bind(fd,(struct sockaddr *)sin, sin->sin_len) < 0){
    printf("Can't bind port %d error:%d\n",ntohs(sin->sin_port), errno);
    return(-1);
  }
  if(listen(fd, 1) == -1) {
    return(-1);
  }
  return (0);
}

static void
handle_fd(int fd)
{
  int at=0;
  int msg_size;
  int sen,rec;
  int *size_p;
  at = 0;
  size_p = (int *)respbuf;
  while((rec = recv(fd, &respbuf[at], sizeof(int) , 0)) == sizeof(int)) {
    num_rcvs++;
    msg_size = ntohl(*size_p);
    if(msg_size > MY_MAX_SIZE) {
      printf("Gak, wants more than my max %d\n", msg_size);
      num_rcv_failed++;
      return;
    }
    at += rec;
    rec = recv(fd, &respbuf[at], (msg_size-at) , 0);
    if((rec + at) != msg_size) {
      printf("Gak, did not get enough data got %d supposed to get %d\n",
	     (rec+at),
	     msg_size);
      num_rcv_failed++;
      return;
    }
    num_rcvs++;
    sen = send(fd, respbuf, msg_size, 0);
    if(sen < msg_size) {
      printf("Error sent %d not %d?\n", sen, msg_size);
      return;
    }
    num_snds++;
    at = 0;
  }
  printf("sen:%d recv:%d\n", num_snds, num_rcvs);
  num_snds = 0;
  num_rcvs = 0;
  return;
}

int
main(int argc, char **argv)
{
  struct timeval start, end;
  int port=0;
  char *addr=NULL;
  int msg_size = 0;
  int fd=-1, i, newfd;
  int sec, usec;
  int number_iterations=0;
  socklen_t len;

  struct sockaddr_in sin;
  struct sockaddr_in from;

  while((i= getopt(argc,argv,"h:p:?")) != EOF) {
    switch (i) {
    case 'h':
      addr = optarg;
      break;
    case 'p':
      port = strtol(optarg, NULL, 0);
      break;
    case '?':
    default:
      printf ("Use %s -h host -p port -i iterations -s msg_size \n",
	      argv[0]);
      return (-1);
    };
  }
  if (port == 0) {
      printf ("Use %s -p port [-h host] \n",
	      argv[0]);
      return (-1);
  }
  if(addr) {
    if (inet_pton(AF_INET, addr, (void *)&sin.sin_addr.s_addr) != 1) {
      printf("Address not parseable\n");
      return(-1);
    }
  } else {
    sin.sin_addr.s_addr = INADDR_ANY;
  }
  sin.sin_port = ntohs(port);
  sin.sin_family = AF_INET;
  sin.sin_len = sizeof(struct sockaddr_in);

  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
  if( fd == -1) {
    printf("Can't open socket %d\n", errno);
    return(-1);
  }

  errno = 0;
  if(init_fd(&sin, fd)) {
    printf("Can't init fd errno:%d\n", errno);
    return (-1);
  }
  len = sizeof(from);
  while((newfd = accept(fd, (struct sockaddr *)&from, &len)) != -1 ){
    handle_fd(newfd);
    close(newfd);
  }
  return (0);
}
