
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <net/if.h>
#include <string.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>

#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <poll.h>
#include <sys/uio.h>
#include <netdb.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_constants.h>

void sctpstr_cli(int sock)
{
	struct sockaddr_in peeraddr;
	struct sctp_sndrcvinfo sri;
	char recvline[2000];
	socklen_t len;
	int rd_sz,wr_sz = 0,goAhead = 1;
	int msg_flags;
	FILE* file = NULL;

	bzero(&sri,sizeof(sri));
	bzero(recvline,sizeof(recvline));
	len = sizeof(struct sockaddr_in);

	if((rd_sz = sctp_recvmsg(sock,recvline,sizeof(recvline),(struct sockaddr*)&peeraddr,&len,&sri,&msg_flags)) < 0)
	{
		perror("read filename from socket\n");
		exit(-1);
	}
	printf("creating file %s\n",recvline);
	if((file = fopen(recvline,"w")) == NULL)
	{
		printf("could not create file\n");
		exit(-1);
	}


	while(goAhead)
	{
		bzero(recvline,sizeof(recvline));

		if((rd_sz = sctp_recvmsg(sock,recvline,sizeof(recvline),(struct sockaddr*)&peeraddr,&len,&sri,&msg_flags)) < 0)
		{
			perror("read from socket\n");
			exit(-1);
		}
		if(msg_flags & MSG_NOTIFICATION)
		{
			if(((int*)recvline)[0] & SCTP_SHUTDOWN_EVENT)
			{
				printf("file transferred\n");
				goAhead = 0;
			}
		}
		else
		{
			printf("ppid: %d\n",sri.sinfo_ppid);
			if(fwrite(recvline,sizeof(char),rd_sz,file) < rd_sz)
			{
				perror("fwite failed!\n");
				exit(-1);
			}
			fflush(file);
			wr_sz += rd_sz;
		}
	}

	fclose(file);

	printf("wrote %d bytes successfully\n",wr_sz);
}

int main(int argc, char** argv)
{
	int sock;
	struct sockaddr_in servaddr;
	struct sctp_event_subscribe evnts;
	struct hostent *addr;
	//-------------------------------------------------------
	struct sctp_initmsg initmsg;

	if(argc < 2)
	{
		printf("Missing argument - use '%s host'\n",argv[0]);
		exit(-1);
	}

	sock = socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);
	bzero(&servaddr,sizeof(servaddr));

	if(atoi(argv[1]) > 0)
	{
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	}
	else
	{
		if ((addr=gethostbyname(argv[1])) == NULL)
		{ //Adress not found
			perror("bad hostname");
			exit(-1);
		}
		//Set address family
		servaddr.sin_family = addr->h_addrtype;

		//Copy and set adress
		bcopy(addr->h_addr_list[0],(char*)&servaddr.sin_addr.s_addr,  sizeof(struct in_addr));
	}
	servaddr.sin_port = htons(2960);

	if(bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
		perror("bind");

	//inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

	bzero(&evnts,sizeof(evnts));
	evnts.sctp_data_io_event = 1;
	evnts.sctp_shutdown_event = 1;

	setsockopt(sock,IPPROTO_SCTP,SCTP_EVENTS,&evnts,sizeof(evnts));

	bzero(&initmsg,sizeof(initmsg));
	initmsg.sinit_num_ostreams = 1;
    initmsg.sinit_max_instreams = 1;
    initmsg.sinit_max_attempts = 0;
    initmsg.sinit_max_init_timeo  = 0;

    //len = sizeof(initmsg);
    if (setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0) {
      perror("setsockopt SCTP_INITMSG");
      close(sock);
      return -1;
    }



	listen(sock,10);

	sctpstr_cli(sock);
	//if(echo_to_all == 0)
	//{
		//sctpstr_cli(stdin,sock,(struct sockaddr*) &servaddr, sizeof(servaddr));
	//}
	//else
	//{
		//sctpstr_cli_echoall(stdin,sock,(struct sockaddr*) &servaddr, sizeof(servaddr));
	//}
	close(sock);

	return 0;
}
