#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sockio.h>

int
main(int argc, char **argv)
{
	int sockfd,i,t;
	struct linger ling;
	char *host=NULL;
	unsigned short port=0;
	struct sockaddr_in servaddr;
	int tcp=1;
	int sctp=0;

	while((i= getopt(argc,argv,"h:p:st")) != EOF){
		switch(i) {
		case 'h':
			host = optarg;
			break;
		case 'p':
			t = strtol(optarg,NULL,0);
			port = (unsigned short)t;
			break;

		case 's':
#ifdef IPPROTO_SCTP
			sctp = 1;
			tcp = 0;
#endif
			break;

		case 't':
			tcp = 1;
			sctp = 0;
			break;
		case '?':
		out_use:
			printf("Use - %s -h host -p port [-s | -t]\n",
			       argv[0]);
			return(-1);

		};
	};
	if((host == NULL) || (port == 0)) {
		goto out_use;
	}
#ifdef IPPROTO_SCTP
	if(sctp) {
		sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	} else {
#endif
		sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef IPPROTO_SCTP
	}
#endif
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if((i = inet_pton(AF_INET, host, &servaddr.sin_addr)) != 1) {
		printf("sorry address not translated (%d)\n",i);
		return(-1);
	}
	i = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(i) {
		printf("Sorry connect failed err:%d\n",errno);
		return(-1);
	}
	ling.l_onoff = 1;
	ling.l_linger = 0;
	i = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
	if(i == -1) {
		printf("Sorry can't set so_linger err:%d\n",errno);
	} else {
		printf("Set for reset/abort\n");
	}
	exit(0);
}
