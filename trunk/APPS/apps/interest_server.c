#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <errno.h>

/* wire test structures */
typedef struct {
    u_char  type;
    u_char  padding;
    u_short dgramLen;
    u_long  dgramID;
}testDgram_t;

#define SCTP_TEST_LOOPREQ   1
#define SCTP_TEST_LOOPRESP  2
#define SCTP_TEST_SIMPLE    3
#define SCTP_TEST_RFTP	    4
#define SCTP_TEST_RFTPRESP  5

/*
int
sendLoopRequest(int fd,int sz, struct sockaddr *to, int flgs, int strm)
{
  char buffer[15000];
  testDgram_t *tt;
  static unsigned int dgramCount=0;
  int sndsz,ret;

  if((sz+sizeof(testDgram_t)) > (sizeof(buffer))){
    sndsz = sizeof(buffer) -  sizeof(testDgram_t);
  }else{
    sndsz = sz +  sizeof(sizeof(testDgram_t));
  }
  memset(buffer,0,sizeof(buffer));
  tt = (testDgram_t *)buffer;
  tt->type = SCTP_TEST_LOOPREQ;
  tt->dgramLen = sndsz - sizeof(testDgram_t);
  tt->dgramID = dgramCount++;
  ret =  sctp_sendmsg(fd,buffer,sz,to,to->sa_len,0,flgs,strm,0,0);
  return(ret);
}
*/


int
main (int argc, char **argv)
{
	char buf[65535];
	int port,len,cnt=0,flags,sleep_opt;
	socklen_t fromlen;
	struct sockaddr_in local_addr;
	struct sockaddr_in from;
	struct sctp_sndrcvinfo info;
	int fd;
	if(argc < 3) {
		printf("Use %s port events\n",argv[0]);
		return(-1);
	}
	if(argc > 3) {
		sleep_opt = strtol(argv[3],NULL,0);
	} else {
		sleep_opt = 0;
	}
	port = strtol(argv[1],NULL,0);
	bzero(&local_addr, sizeof(local_addr));
#ifndef linux
	local_addr.sin_len         = sizeof(local_addr);
#endif
	local_addr.sin_family      = AF_INET;
	local_addr.sin_addr.s_addr = 0;
	local_addr.sin_port        = htons(port);
	printf("Binding port %d\n",
	       ntohs(local_addr.sin_port));
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket call");
		return(-1);
	}
	if (bind(fd, (struct sockaddr *)&local_addr,
		 sizeof(local_addr)) == -1) {
		perror("Bind fails");
		return(-1);
	}
	if (listen(fd, 10) == -1) {
		perror("Listen fails");
		return (-1);
	}
	bzero(&info,sizeof(info));
	while(1) {
		len = sizeof(buf);
		fromlen = sizeof(from);
		len = sctp_recvmsg (fd,buf,len,
				    (struct sockaddr *)&from,&fromlen,
				    &info,&flags);
		cnt++;
		if(((cnt+1) % 1000) == 0) {
			printf("msg cnt to %d\n", cnt);
		}
	}
	return(0);
}
