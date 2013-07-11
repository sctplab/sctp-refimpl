
//#include <iostream>
#include <string.h>

//----------------------------------------------------------
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

int main(int argc, char** argv)
{
	int sock;//, msg_flags;
	char readbuf[2000];

	struct sockaddr_in servaddr;
	struct sctp_sndrcvinfo sri;
	struct sctp_event_subscribe evnts;

	struct hostent *addr;
	struct sctp_initmsg initmsg;
	char inFile[2048];//NULL;
	FILE* file = NULL;

	int sz,sent = 0;
	socklen_t len;
	size_t rd_sz;


	if(argc < 3)
	{
		printf("Missing argument - use '%s peer file'\n",argv[0]);
		exit(-1);
	}
	strcpy(inFile,argv[2]);

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

	if((file = fopen(argv[2],"r")) == NULL)
	{
		printf("file not found: %s\n", argv[2]);
		exit(-1);
	}

	servaddr.sin_port = htons(2960);

	sock = socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);

	bzero(&evnts,sizeof(evnts));
	evnts.sctp_data_io_event = 1;
	if(setsockopt(sock,IPPROTO_SCTP,SCTP_EVENTS,&evnts,sizeof(evnts))<0)
		perror("setsockopt evnts");

	bzero(&initmsg,sizeof(initmsg));
	initmsg.sinit_num_ostreams = 2;
    initmsg.sinit_max_instreams = 2;
    initmsg.sinit_max_attempts = 0;
    initmsg.sinit_max_init_timeo  = 0;

    len = sizeof(initmsg);
   	if (setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0) {
      perror("setsockopt SCTP_INITMSG");
      close(sock);
      exit(-1);
    }

	sri.sinfo_stream = 0;
	printf("SENDING FILENAME: %s\n",inFile);

	len = sizeof(servaddr);
	if((sz = sctp_sendmsg(sock,inFile,strlen(inFile),(struct sockaddr*)&servaddr,len,sri.sinfo_ppid,sri.sinfo_flags,sri.sinfo_stream,0,0))
			< strlen(argv[2]))
	{
		printf("could not send file name\n");
		close(sock);
		exit(-1);
	}
	else
	{
		printf("filename sent\n");
	}
	sri.sinfo_ppid = 0;
	while(!feof(file) && !ferror(file))
	{
		sri.sinfo_ppid++;
		sri.sinfo_ppid = sri.sinfo_ppid % 10;
		printf("using ppid %d\n", sri.sinfo_ppid);

		if((rd_sz = fread(readbuf,sizeof(char),512,file))>=0)
		{
			if((sz = sctp_sendmsg(sock,readbuf,rd_sz,(struct sockaddr*)&servaddr,len,sri.sinfo_ppid,sri.sinfo_flags,sri.sinfo_stream,0,0))<0)
				perror("send error\n");
			else
				sent += sz;
		}
		else{
			perror("file read error\n");
		}
	}
	if(ferror(file))
	{
		perror("file error\n");
	}
	bzero(readbuf,sizeof(readbuf));
	sri.sinfo_flags = MSG_EOF;
	if(sctp_sendmsg(sock,readbuf,0,(struct sockaddr*)&servaddr,len,sri.sinfo_ppid,sri.sinfo_flags,sri.sinfo_stream,0,0) < 0)
		perror("sending EOF\n");

	printf("%d bytes sent successfully\n",sent);
	fclose(file);
	shutdown(sock,SHUT_RDWR);

	return 0;
}










