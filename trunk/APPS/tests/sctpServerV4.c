/*** a sample set of SCTP server ***/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/times.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/sctp_constants.h>
#include <netinet/sctp.h>

int
main()
{
	int	fd,ret,newfd,count;
	struct sockaddr_in	s,dest;
	int	clientAddrLen;
	int	readNum;
	char *	buff;
	int	size = 2100; /***1024***/

	/***
	fd=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
	***/
	fd=socket(AF_INET,SOCK_STREAM,IPPROTO_SCTP);
	if(fd==-1){
		printf("socket ERROR.\n");
		exit(0);
	}
	bzero((char *)&s,sizeof(s));

	s.sin_port   = htons(10001);
	s.sin_family = AF_INET;

	ret=bind(fd,(struct sockaddr *)&s,sizeof(s));
	if(ret==-1){
		printf("bind ERROR.\n");
		close(fd);
		exit(-1);
	}
	ret=listen(fd,10);
	if(ret==-1){
		printf("listen ERROR.\n");
		close(fd);
		exit(-1);
	}
	count=0;
	while(1){
		clientAddrLen = sizeof(dest);
		printf("wait accept...\n");
		newfd=accept(fd,(struct sockaddr *)&dest,&clientAddrLen);
		if(newfd < 0) {
			printf("accept ERROR.\n");
			close(fd);
			close(newfd);
			exit(-1);
		}
		printf("Start Service %d : SCTP\n",count++);
		buff=(char *)malloc(size);
		for(;;){
			readNum=read(newfd,buff,size);
			if(readNum<0){
				printf("read ERROR.\n");
				close(fd);
				close(newfd);
				free(buff);
				exit(-1);
			}
			printf("Read. %dbytes ###%c%c%c###\n",readNum,
				buff[0],buff[1],buff[2]);
			if(readNum==0) break;
		}
		close(newfd);
		free(buff);
	}
	close(fd);
	close(newfd);
	printf("SCTP server END.\n");
}

