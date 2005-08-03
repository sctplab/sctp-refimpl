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

#define MY_MAX_SIZE 8000
#define MY_TRAIN_SIZE 9

unsigned char mybuf[MY_TRAIN_SIZE][MY_MAX_SIZE];
unsigned char respbuf[MY_TRAIN_SIZE][MY_MAX_SIZE];
struct timeval rcvtimes[MY_TRAIN_SIZE];

int snd_failed=0;
int num_rcvs=0;
int rcv_failed=0;
int verbose=0;

struct snddata {
  int seq;
  int size_of;
  char data[0];
};

static void
init_buffer(char *buf, int seq, int sz)
{
  int i;
  char at='A';
  struct snddata *snd;

  if(sz < (sizeof(int) *2)) {
    printf("Bad init size\n");
    exit(-1);
  }
  for(i=0; i<sz; i++) {
    buf[i] = at;
    at++;
    if(at > 'z'){
      at = 'A';
    }
  }
  snd = (struct snddata *)buf;
  snd->seq = seq;
  snd->size_of = sz;
}

static int
init_fd(int fd)
{
  return (0);
}

static void
do_send_pass( int fd, struct sockaddr_in *sin, 
	      struct timeval *start)
{
  socklen_t len=sizeof(struct sockaddr_in);
  struct snddata *snd;
  int i,sen;

  gettimeofday(start, NULL);
  errno = 0;
  for(i=0; i<MY_TRAIN_SIZE; i++) {
    snd = (struct snddata *)mybuf[i];
    if((sen = sendto(fd, mybuf[i], snd->size_of, 0, (struct sockaddr *)sin, len) < snd->size_of)) {
      printf("snd failed errno:%d size:%d\n",
	     errno,  sen);
      snd_failed++;
      return;
    }
  }
  return;
}

static void
do_recv_pass( int fd, struct sockaddr_in *sin) 
{
 int recv,i;
 struct sockaddr_in from;
 struct snddata *snd;
 socklen_t len=sizeof(struct sockaddr_in);

  for(i=0; i<MY_TRAIN_SIZE; i++) {
    len = sizeof(struct sockaddr_in);
    snd = (struct snddata *)respbuf[i];
    recv = recvfrom(fd, respbuf[i], MY_MAX_SIZE, 0,
		    (struct sockaddr *)&from, &len);
    gettimeofday(&rcvtimes[i], NULL);
    if(verbose) {
      printf("Received %d bytes from %x:%d\n",
	     recv, ntohl(from.sin_addr.s_addr),
	     ntohs(from.sin_port));
    }
    num_rcvs++;
    if(recv < sizeof(int)){
      i--;
      rcv_failed++;
      continue;
    }
    if(from.sin_addr.s_addr != sin->sin_addr.s_addr) {
      i--;
      rcv_failed++;
      continue;
    }
    if(from.sin_port != from.sin_port) {
      i--;
      rcv_failed++;
      continue;
    }
    snd->size_of = recv;
  }
}


int
main(int argc, char **argv)
{
  struct timeval start, o_start, end;
  int port=0;
  char *addr=NULL;
  int msg_size_s = 0;
  struct snddata *snd,*rec;
  int msg_size_l = 0;
  int fd=-1, i;
  int sec, usec, stop=0;
  struct sockaddr_in sin;

  while((i= getopt(argc,argv,"h:p:?s:S:v")) != EOF) {
    switch (i) {
    case 'v':
      verbose = 1;
      break;
    case 'h':
      addr = optarg;
      break;
    case 's':
      msg_size_s = strtol(optarg, NULL, 0);
      if(msg_size_s > MY_MAX_SIZE) {
	printf("Sorry max is %d setting to max\n", MY_MAX_SIZE);
	msg_size_s = MY_MAX_SIZE;
      }
    case 'S':
      msg_size_l = strtol(optarg, NULL, 0);
      if(msg_size_l > MY_MAX_SIZE) {
	printf("Sorry max is %d setting to max\n", MY_MAX_SIZE);
	msg_size_l = MY_MAX_SIZE;
      }

      break;
    case 'p':
      port = strtol(optarg, NULL, 0);
      break;
    case '?':
    default:
      printf ("Use %s -h host -p port -s msg_size_small -S msg_size_large\n",
	      argv[0]);
      return (-1);
    };
  }
  if ( (addr == NULL) ||
       (msg_size_s == 0) ||
       (msg_size_l == 0) ||
       (port == 0)) {
      printf ("Use %s -h host -p port -s msg_size \n",
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

  fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if( fd == -1) {
    printf("Can't open socket %d\n", errno);
    return(-1);
  }

  for(i=0; i<MY_TRAIN_SIZE; i++) {
    if((i % 2) == 0) {
      init_buffer(mybuf[i], i, msg_size_s);
    } else {
      init_buffer(mybuf[i], i, msg_size_l);
    }
  }
  errno = 0;
  if(init_fd(fd)) {
    printf("Can't init fd errno:%d\n", errno);
    return (-1);
  }
  do_send_pass(fd, &sin, &start);
  if(snd_failed) {
    printf("Could not send errno %d aborted test\n", snd_failed);
    exit(0);
  }
  do_recv_pass(fd, &sin);
  printf("Time train was sent %d.%6.6d\n",start.tv_sec, start.tv_usec);
  o_start = start;
  for(i=0; i<MY_TRAIN_SIZE; i++) {
    rec = (struct snddata *)respbuf[i];    
    if(rec->seq < MY_TRAIN_SIZE) {
      snd = (struct snddata *)mybuf[rec->seq];
    } else {
      printf("Gak, rec->seq:%d unreal -- skip \n",rec->seq);
      continue;
    }
    printf("Packet size %d Time:%d.%6.6d seq:%d ",
	   snd->size_of,
	   rcvtimes[i].tv_sec,
	   rcvtimes[i].tv_usec,
	   rec->seq
	   );
    if(start.tv_sec == rcvtimes[i].tv_sec) {
      end.tv_sec = 0;
      end.tv_usec = rcvtimes[i].tv_usec - start.tv_usec;
    } else if (rcvtimes[i].tv_sec > start.tv_sec) {
      end.tv_sec = rcvtimes[i].tv_sec - start.tv_sec;
      if(start.tv_usec > rcvtimes[i].tv_usec) {
	end.tv_sec--;
	end.tv_usec = rcvtimes[i].tv_usec + (1000000 - start.tv_usec);
      } else {
	end.tv_usec = rcvtimes[i].tv_usec  - start.tv_usec;
      }
    } else {
      printf("Impossible time\n");
      continue;
    }
    printf("IAT:%d.%6.6d ",end.tv_sec, end.tv_usec);
    if(o_start.tv_sec == rcvtimes[i].tv_sec) {
      end.tv_sec = 0;
      end.tv_usec = rcvtimes[i].tv_usec - o_start.tv_usec;
    } else if (rcvtimes[i].tv_sec > o_start.tv_sec) {
      end.tv_sec = rcvtimes[i].tv_sec - o_start.tv_sec;
      if(o_start.tv_usec > rcvtimes[i].tv_usec) {
	end.tv_sec--;
	end.tv_usec = rcvtimes[i].tv_usec + (1000000 - o_start.tv_usec);
      } else {
	end.tv_usec = rcvtimes[i].tv_usec  - o_start.tv_usec;
      }
    } else {
      printf("Impossible time\n");
      continue;
    }

    printf("RTT:%d.%6.6d\n",end.tv_sec, end.tv_usec);
    start = rcvtimes[i];
  }
  return (0);
}
