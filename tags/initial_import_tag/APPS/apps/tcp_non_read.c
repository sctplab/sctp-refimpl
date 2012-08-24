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
#include <netinet/tcp.h>


int
main(int argc, char **argv)
{
	int i,fd,newfd,sb;
	socklen_t len;
	u_int16_t port=0;
	int optval;
	socklen_t optlen;
	int protocol_touse = IPPROTO_SCTP;
	struct sockaddr_in bindto,got,from;
	int sleep_period = 60;
	
	optlen = sizeof(optval);
	sb = 57000;
	while((i= getopt(argc,argv,"p:b:s:S:E:")) != EOF){
		switch(i){
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		case 'b':
			sb = strtol(optarg,NULL,0);
			break;
		case 's':
			sleep_period = strtol(optarg,NULL,0);
			break;
		case 'h':
		case '?':
		useage:
			printf("Use %s -p port [-b window -s sleep-time -S Send-char -E Expect-char ]\n",
			       argv[0]);
			return(-1);
			break;
		};
	}
	if(port == 0){
		goto useage;
	}
	fd = socket(AF_INET, SOCK_STREAM, protocol_touse);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	len = sizeof(bindto);
#if defined(WIN32) || defined(LINUX)
	/* no sin_len member */
#else
	bindto.sin_len = sizeof(bindto);
#endif /* WIN32 || LINUX */
	bindto.sin_family = AF_INET;
	bindto.sin_port = htons(port);
	if(bind(fd,(struct sockaddr *)&bindto, len) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
		return(-1);
	}
	if(getsockname(fd,(struct sockaddr *)&got,&len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		return(-1);
	}	
	if(port){
		if(got.sin_port != bindto.sin_port){
			printf("Warning:could not get your port :<\n");
			return(-1);
		}
	}
	printf("Server listens on port %d\n", ntohs(got.sin_port));
	errno = 0;
	newfd = listen(fd,1);
	newfd = accept(fd,(struct sockaddr *)&from,&len);
	if(newfd == -1){
		printf("Accept fails for fd:%d err:%d\n",fd,errno);
		return(-1);
	}
	printf("Got a connection from %x:%d fd:%d\n",
	       (u_int)ntohl(from.sin_addr.s_addr),
	       (int)ntohs(from.sin_port),
	       fd);
	printf("Sleeping\n");
	sleep(sleep_period);
	printf("call shutdown\n");
	fflush(stdout);
	shutdown(newfd, SHUT_RDWR);
	sleep(sleep_period);
	printf("Done\n");
	return(0);
}

