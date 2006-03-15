#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <sys/signal.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "cmp_msg.h"


int
main(int argc, char **argv)
{
	struct txfr_request *req;
	char buffer[200000];
	int i,fd,newfd,ret,sizetosend,blksize,numblk,sb;
	u_int16_t port=0;
	int optval,optlen;
	socklen_t sa_len;
	int snd_buf=200;
	char *addr_to_bind=NULL;
	int maxburst=0;
	struct sockaddr_in bindto,got,from;
	int cnt_written=0;
	int maxseg=0;
	int print_before_write=0;
	FILE *outlog;

	optlen = sizeof(optval);
	sb = 0;
	while((i= getopt(argc,argv,"p:b:Pz:S:m:B:")) != EOF){
		switch(i){
		case 'B':
			addr_to_bind = optarg;
			break;
		case 'm':
			maxburst = strtol(optarg,NULL,0);
			break;
		case 'S':
			maxseg = (int)strtol(optarg,NULL,0);
			printf("maxseg set to %d\n",maxseg);
			break;
		case 'z':
			snd_buf = strtol(optarg,NULL,0);
			printf("snd_buf now %d * 1024\n",
			       snd_buf);
			break;
		case 'P':
			print_before_write = 1;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		case 'b':
			sb = strtol(optarg,NULL,0);
			break;
		};
	}
	signal(SIGPIPE,SIG_IGN);
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	sa_len = sizeof(bindto);
#if defined (HAVE_SA_LEN)
	bindto.sin_len = sizeof(bindto);
#endif /* HAVE_SA_LEN */
	bindto.sin_family = AF_INET;
	bindto.sin_port = htons(port);
	if(addr_to_bind) {
		inet_pton(AF_INET, addr_to_bind, (void *) &bindto.sin_addr);
	}
	if(bind(fd,(struct sockaddr *)&bindto, sa_len) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
		return(-1);
	}
	sa_len = sizeof(got);
	if(getsockname(fd,(struct sockaddr *)&got,&sa_len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		return(-1);
	}	
	if(port){
		if(got.sin_port != bindto.sin_port){
			printf("Warning:could not get your port :<\n");
		}
	}
	printf("Server listens on port %d\n",
	       ntohs(got.sin_port));
	errno = 0;
	newfd = listen(fd,1);
	printf("Listen returns %d errno:%d\n",
	       newfd,errno);
 again:
	sa_len = sizeof(from);
	newfd = accept(fd,(struct sockaddr *)&from,&sa_len);
	if(newfd == -1){
		printf("Accept fails for fd:%d err:%d\n",fd,errno);
		return(-1);
	}
	if(maxseg){
		optval = maxseg;	
		if(protocol_touse == IPPROTO_SCTP){
			if(setsockopt(newfd, IPPROTO_SCTP, SCTP_MAXSEG, &optval, optlen) != 0){
				printf("err:%d could not set maxseg to %d\n",errno,optval);
			}
		}else if(protocol_touse == IPPROTO_TCP){
			if(setsockopt(newfd, IPPROTO_TCP, TCP_MAXSEG, &optval, optlen) != 0){
				printf("err:%d could not set maxseg to %d\n",errno,optval);
			}
		}
	}
	printf("Got a connection from %x:%d fd:%d\n",
	       (u_int)ntohl(from.sin_addr.s_addr),
	       (int)ntohs(from.sin_port),
	       fd);
	ret = recv(newfd,buffer,sizeof(struct txfr_request),0);
	if(ret <= 0){
		printf("Gak got %d bytes errno:%d\n",
		       ret,errno);
		close(newfd);
		goto again;
	}
	if(ret < sizeof(struct txfr_request)){
		printf("Huh not the right request\n");
	exit_now:
		close(newfd);
		goto again;
	}
	req = (struct txfr_request *)buffer;
	sizetosend = (int)ntohl(req->sizetosend);
	blksize =  (int)ntohl(req->blksize);
	if(req->snd_window) {
		optval = (int)ntohl(req->snd_window);
		if(setsockopt(newfd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, optlen) != 0) {
			printf("err:%d could not set sndbuf to clients %d\n",errno,optval);
		}
	}
	if(req->tos_value) {
		if (protocol_touse == IPPROTO_TCP){
			optval = req->tos_value;
		} else {
			optval = req->tos_value + 0x4;
		}
		optlen = 4;
		if (setsockopt(newfd, IPPROTO_IP, IP_TOS, (const char *)&optval, optlen) != 0) {
			printf("Can't set tos value to %x :-( err:%d\n", req->tos_value, errno);
		} else {
			printf("Set tos to %x\n", req->tos_value);
		}
	}
	numblk = sizetosend/blksize;
	if((sizetosend % blksize) > 0){
		numblk++;
	}
	if(blksize > sizeof(buffer))
		blksize = sizeof(buffer);

	memset(buffer,0,blksize);
	strcpy(buffer,"inbetween");

	while(numblk > 0){
		if(print_before_write){
			printf("Writting block %8.8d  - %d\r",cnt_written,
			       numblk);
			fflush(stdout);
		}
		cnt_written++;
		sprintf(buffer,"%6.6d",numblk);
		ret = sendto(newfd,buffer,blksize,0,
			     (struct sockaddr *)&from,
			     sizeof(from));
		if(ret < 0){
			printf("Gak, error %d on send\n",
			       errno);
			printf("Left to Send %d before exit\n",numblk);
			goto exit_now;
		}
		numblk--;
	}
	close(newfd);
	printf("\nclosed and done with client\n");
	goto again;
	return(0);
}

