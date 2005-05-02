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
#include <sys/filio.h>

#define MY_MAX_SIZE 230000

unsigned char mybuf[MY_MAX_SIZE];
unsigned char respbuf[MY_MAX_SIZE];
int num_snds=0;
int num_rcvs=0;
int data_failures=0;
int snd_failed=0;
int rcv_failed=0;
struct sctp_sndrcvinfo sinfo;

static void
init_buffer(int sz)
{
  int *sizep, i;
  char at='A';
  memset(&sinfo, 0 , sizeof(sinfo));
  sizep = (int *)mybuf;
  for(i=sizeof(int); i<sz; i++) {
    mybuf[i] = at;
    at++;
    if(at > 'z'){
      at = 'A';
    }
  }
  *sizep = htonl(sz);

}

static int
init_fd(int fd)
{
  int on=0;
  if(setsockopt(fd,IPPROTO_SCTP,
		SCTP_NODELAY,
		&on, sizeof(on)) != 0) {
    printf("Can't set TCP nodelay %d\n", errno);
    return(-1);
  }
  if(ioctl(fd, FIONBIO, &on) != 0) {
    printf("IOCTL FIONBIO fails error:%d!\n",errno);
    return (-1);
  }
  return (0);
}

static void
do_a_pass( int fd, int msg_size, int *stop)
{
  int not_done = 1;
  int at=0;
  int sen,rec;
  num_snds++;
  struct sockaddr_in from;
  socklen_t len=sizeof(from);
  int flags=0;

  errno = 0;
  if((sen = sctp_send(fd, mybuf, msg_size, &sinfo,  0)) < msg_size) {
    printf("snd failed errno:%d size:%d\n",
	   errno,  sen);
    *stop = 1;
    snd_failed++;
    return;
  }
  errno = 0;
  if((rec = sctp_recvmsg(fd, respbuf, MY_MAX_SIZE,
			    (struct sockaddr *)&from,
			    &len, &sinfo, &flags)) < msg_size) {
    rcv_failed++;
    *stop = 1;
    printf("rcv failed errno:%d size:%d\n",
	   errno,  rec);
    return;
  }
  num_rcvs++;
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
  int sec, usec, stop=0;
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

  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
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
    do_a_pass(fd, msg_size, &stop);
    if(stop)
      break;
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
  printf("Had %d snd_failures and %d rcv_failures\n",
	 snd_failed,
	 rcv_failed);
  return (0);
}
