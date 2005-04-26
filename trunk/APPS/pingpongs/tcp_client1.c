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

#define MY_MAX_SIZE 230000

unsigned char mybuf[MY_MAX_SIZE];
unsigned char respbuf[MY_MAX_SIZE];
int num_snds=0;
int num_rcvs=0;
int data_failures=0;

static void
init_buffer(int sz)
{
  int *sizep, i;
  char at='A';
  sizep = (int *)mybuf;
  for(i=sizeof(int); i<sz; i++) {
    mybuf[i] = at;
    at++;
    if(at > 'z'){
      at = 'A';
    }
  }
  printf("Place %d in buffer\n", sz);
  *sizep = htonl(sz);
}

static int
init_fd(int fd)
{
  int on=1;
  if(setsockopt(fd,IPPROTO_TCP,
		TCP_NODELAY,
		&on, sizeof(on)) != 0) {
    printf("Can't set TCP nodelay %d\n", errno);
    return(-1);
  }
  return (0);
}

static void
do_a_pass( int fd, int msg_size)
{
  int not_done = 1;
  int at=0;
  int sen,rec;
  while (at < msg_size) {
    sen = send(fd, &mybuf[at], (msg_size-at), 0);
    if(sen <= 0) {
      printf("Error connect reset err:%d?\n", errno);
      exit(-1);
    }
    num_snds++;
    at += sen;
  }
  at = 0;
  while (at < msg_size) {
    rec = recv(fd, &respbuf[at], (msg_size-at), 0);
    if(rec <= 0) {
      printf("Error connection reset at rcv err:%d\n", errno);
      exit(-1);
    }
    num_rcvs++;
    at += rec;
  }
  if(memcmp(mybuf, respbuf, msg_size) != 0) {
    data_failures++;
  }
  return;
}

int
main(int argc, char **argv)
{
  struct timeval start, end;
  int port=0;
  char *addr=NULL;
  int msg_size = 0;
  int fd=-1, i;
  int sec, usec;
  int number_iterations=0;
  struct sockaddr_in sin;

  while((i= getopt(argc,argv,"h:p:?s:i:")) != EOF) {
    switch (i) {
    case 'h':
      addr = optarg;
      break;
    case 'i':
      number_iterations = strtol(optarg, NULL, 0);
      break;
    case 's':
      msg_size = strtol(optarg, NULL, 0);
      if(msg_size > MY_MAX_SIZE) {
	printf("Sorry max is %d setting to max\n", MY_MAX_SIZE);
	msg_size = MY_MAX_SIZE;
      }
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
  if ( (addr == NULL) ||
       (msg_size == 0) ||
       (number_iterations == 0) ||
       (port == 0)) {
      printf ("Use %s -h host -p port -i iterations -s msg_size \n",
	      argv[0]);
      return (-1);
  }
  if (inet_pton(AF_INET, addr, (void *)&sin.sin_addr.s_addr) != 1) {
    printf("Address not parseable\n");
    return(-1);
  }
  sin.sin_port = ntohs(port);
  sin.sin_family = AF_INET;
  sin.sin_len = sizeof(struct sockaddr_in);

  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if( fd == -1) {
    printf("Can't open socket %d\n", errno);
    return(-1);
  }
  init_buffer(msg_size);
  errno = 0;
  if(init_fd(fd)) {
    printf("Can't init fd errno:%d\n", errno);
    return (-1);
  }
  gettimeofday(&start, NULL);
 
  if(connect(fd, (struct sockaddr *)&sin, sizeof(sin))) {
    printf("connect fails %d\n", errno);
    return (-1);
  }
  for (i=0; i<number_iterations; i++) {
    do_a_pass(fd, msg_size);
  }
  gettimeofday(&end, NULL);
  
  sec = end.tv_sec - start.tv_sec;
  if(sec) {
    if(end.tv_usec > start.tv_usec) {
      usec = end.tv_usec - start.tv_usec;
    } else {
      usec = 1000000 - start.tv_usec;
      usec += end.tv_usec;
      sec--;
    }
  } else {
    usec = end.tv_usec - start.tv_usec;
  }
  printf("%d snds and %d recvs had %d failures in %d.%6.6d\n",
	 num_snds,
	 num_rcvs,
	 data_failures,
	 sec, usec);
  return (0);
}
