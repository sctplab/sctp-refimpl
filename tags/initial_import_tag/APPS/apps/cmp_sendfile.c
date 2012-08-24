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
#include <sys/uio.h>
#include <fcntl.h>

struct txfr_request{
	char filename[256];
};

int
main(int argc, char **argv)
{
	int in_fd,i,ret;
	char buffer[1024];
	uint16_t port=0;
	struct txfr_request *req;
	struct sockaddr_in bindto,got,from;
	char *addr_to_bind=NULL;
	struct sctp_sndrcvinfo sinfo;
	socklen_t sa_len;
	int fd, failed_cnt=0, newfd;
	int flags;
#if defined(__APPLE__)
	off_t len=0;
#endif

	while((i= getopt(argc,argv,"B:p::")) != EOF){
		switch(i){
		case 'B':
			addr_to_bind = optarg;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
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
	bindto.sin_len = sizeof(bindto);
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
 again:
	sa_len = sizeof(from);
	newfd = accept(fd,(struct sockaddr *)&from,&sa_len);
	if(newfd == -1){
		failed_cnt++;
		printf("Accept fails for fd:%d err:%d\n",fd,errno);
		if(failed_cnt > 20)
			return(-1);
		goto again;
	}
	failed_cnt = 0;
	printf("Got a connection from %x:%d fd:%d newfd:%d\n",
	       (u_int)ntohl(from.sin_addr.s_addr),
	       (int)ntohs(from.sin_port),
	       fd, newfd);
	sa_len = sizeof(from);
	ret = sctp_recvmsg (newfd, buffer, sizeof(buffer), 	
			    (struct sockaddr *)&from,
			    &sa_len, &sinfo, &flags);

	if(ret <= 0){
		printf("rcvmsg:Gak got %d bytes errno:%d\n",
		       ret,errno);
		close(newfd);
		goto again;
	}
	if(ret < sizeof(struct txfr_request)){
		printf("Huh not the right request\n");
		close(newfd);
		goto again;
	}
	req = (struct txfr_request *)buffer;
	
	in_fd = open(req->filename, O_RDONLY);
	if(in_fd == -1) {
		printf("Can't open file %s\n",req->filename);
		close(newfd);
		goto again;
	}
#if defined(__APPLE__)
	ret = sendfile(in_fd, newfd, 0, &len, NULL, 0);
#else
	ret = sendfile(in_fd, newfd, 0, 0, NULL, NULL, 0);
#endif
	printf("ret from sendfile is %d\n", ret);
	close(newfd);
	close(in_fd);
	printf("\nclosed and done with client\n");
	goto again;
	return(0);
}

