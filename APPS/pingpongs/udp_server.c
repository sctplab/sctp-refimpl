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
#include <netinet/tcp.h>

#define MY_MAX_SIZE 8000

unsigned char respbuf[MY_MAX_SIZE];
int verbose = 0;
int num_snds=0;
int num_rcvs=0;
int data_failures=0;
int num_rcv_failed=0;
int num_snd_failed=0;

static int
init_fd(struct sockaddr_in *sin, int fd)
{
  int on=1;
  if(bind(fd,(struct sockaddr *)sin, sin->sin_len) < 0){
    printf("Can't bind port %d error:%d\n",ntohs(sin->sin_port), errno);
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
  struct sockaddr_in from;
  socklen_t len;
  at = 0;
  size_p = (int *)respbuf;
  len = sizeof(struct sockaddr_in);
  while(1) {
    memset(respbuf, 0, sizeof(respbuf));
    len = sizeof(struct sockaddr_in);
    rec = recvfrom(fd, respbuf, MY_MAX_SIZE, 0,
		   (struct sockaddr *)&from, &len);
    if(verbose) {
      printf("Received %d bytes from %x:%d\n",
	     rec, ntohl(from.sin_addr.s_addr),
	     ntohs(from.sin_port));
    }
    num_rcvs++;
    if(rec < sizeof(int)) {
      continue;
    }
    msg_size = ntohl(*size_p);
    sen = sendto(fd, respbuf, sizeof(int), 0, 
		 (struct sockaddr *)&from, len);
    if(sen < sizeof(int)) {
      printf("Error sent %d not %d?\n", sen, msg_size);
      num_snd_failed++;
    }
    num_snds++;
    len = sizeof(struct sockaddr_in);
  }
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

  while((i= getopt(argc,argv,"h:p:v?")) != EOF) {
    switch (i) {
    case 'v':
      verbose = 1;
      break;
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

  fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
  handle_fd(fd);
  return (0);
}
