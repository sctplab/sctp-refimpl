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
	int port,num,i;
	struct sockaddr_in remote_addr;
	int fd;
	if(argc < 4) {
		printf("Use %s host port num\n",argv[0]);
		return(-1);
	}
	num = strtol(argv[3],NULL,0);
	port = strtol(argv[2],NULL,0);
	bzero(&remote_addr, sizeof(remote_addr));
#ifndef linux
	remote_addr.sin_len         = sizeof(remote_addr);
#endif
	remote_addr.sin_family      = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr(argv[1]);
	remote_addr.sin_port        = htons(port);

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket call");
		return(-1);
	}
	for(i=0;i<num;i++){
		sendto(fd,"hi",2,0,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr_in));
		printf("%d\r",(i+1));
		fflush(stdout);
	}
	sleep(10000);
	return(0);
}
