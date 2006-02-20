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


int
main (int argc, char **argv)
{
	int num=0,i,ports=0,porte=0,nohb=0,j;
	struct sctp_paddrparams sp;
	struct sockaddr_in remote_addr;
	struct sctp_event_subscribe evnts;
	int fd[10000];
	bzero(&remote_addr, sizeof(remote_addr));
	while((i= getopt(argc,argv,"Hs:e:h:n:?")) != EOF) {
		switch(i) {
		case 'n':
			num = strtol(optarg,NULL,0);
			if((num < 0) || (num >= 10000)) {
				printf("Number invalid  0 > %d < 10000\n",
				       num);
				exit(0);
			}
			break;
		case 'h':
			remote_addr.sin_addr.s_addr = inet_addr(optarg);
			break;
		case 's':
			ports = strtol(optarg,NULL,0);
			break;
		case 'e':
			porte = strtol(optarg,NULL,0);
			break;
		case 'H':
			nohb = 1;
			break;
		case '?':
		default:
			goto exit_now;
			break;
		};
	};
	if((num == 0) || (ports == 0) ||
	   (porte == 0) || (remote_addr.sin_addr.s_addr == 0)) {
		exit_now:
		printf("Use %s -h host -s port-start -e port-end -n number [-H]\n",
		       argv[0]);
		printf(" -h arg - host that ports are on\n");
		printf(" -s arg - starting port number\n");
		printf(" -e arg - ending port number\n");
		printf(" -n arg - number of sockets to open\n");
		printf(" -H - disable heartbeats\n");
		exit(0);
	}
	remote_addr.sin_len         = sizeof(remote_addr);
	remote_addr.sin_family      = AF_INET;
	bzero(&evnts, sizeof(evnts));
	memset((caddr_t)&sp,0,sizeof(sp));
	sp.spp_hbinterval = 0;
	sp.spp_flags = SPP_HB_DISABLE;
	for(i=0;i<num;i++){
		if ((fd[i] = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
			perror("socket call");
			break;
		}
		/* no events */
		setsockopt(fd[i],IPPROTO_SCTP, SCTP_EVENTS,
			   &evnts, sizeof(evnts));

		/* no hb's */
		if(nohb) {
			setsockopt(fd[i],IPPROTO_SCTP,
				   SCTP_PEER_ADDR_PARAMS, &sp, sizeof(sp));
		}
		for(j=ports;j<=porte;j++) {
			remote_addr.sin_port        = htons(j);
			connect(fd[i],(struct sockaddr *)&remote_addr,sizeof(struct sockaddr_in));
		}
	}
	sleep(10000);
	return(0);
}
