/*** a sample set of SCTP client ***/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/***hchiba***/
#include <netinet/sctp_constants.h>
#include <netinet/sctp.h>

int
main()
{
	const char	hostname[64]="10.1.1.11";
	const char	servname[16]="10001";
	struct addrinfo	hints,*res,*res0;
	int s,count,ii;
	char hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];
	int error;
	int	writeNum;
	int	size = 2100;/*** 128 ***/
	char *	buff;
	char *	buf2;

#ifdef __FreeBSD__
	printf("hchiba-> FreeBSD version = %d\n",__FreeBSD__);
#endif /* __FreeBSD__ */

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	/****
	hints.ai_socktype = SOCK_DGRAM;
	*** hchiba ****/
	hints.ai_protocol = IPPROTO_TCP;

	hints.ai_flags = AI_NUMERICHOST;

	error = getaddrinfo(hostname,servname,&hints,&res0);
	/****
	error = getaddrinfo(NULL,servname,&hints,&res0);
	*** hchiba ****/

	if (error) {
		fprintf(stderr,"%s %s: %s\n",hostname,servname,
						gai_strerror(error));
		exit(1);
	}

	for(res = res0; res; res = res->ai_next){
		error = getnameinfo(res->ai_addr, res->ai_addrlen,
			hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
		if (error) {
			fprintf(stderr,"#2 %s %s: %s\n",hostname,servname,
						gai_strerror(error));
			continue;
		}
		fprintf(stderr,"trying %s port %s\n",hbuf,sbuf);

		/***hchiba***/
		/***res->ai_protocol = 132;***/
		printf("fami=%d,sktype=%d,proto=%d\n",
			res->ai_family,res->ai_socktype,res->ai_protocol);
		s = socket(res->ai_family,res->ai_socktype, IPPROTO_SCTP);
		if(s<0){
			fprintf(stderr,"socket ERROR.\n");
			continue;
		}

		if(connect(s,res->ai_addr,res->ai_addrlen) < 0){
			fprintf(stderr,"Failed to connect.\n");
			close(s);
			s = -1;
			continue;
		}

		buff=(char *)malloc(size);
		buf2=(char *)malloc(size);
		buff[0]='T'; buff[1]='h'; buff[2]='x'; buff[3]=0;
		for(count=0;count<5;++count){
			/*** send/recv ***/
			writeNum=write(s,buff,size);
			if(writeNum<0){
				fprintf(stderr,"write error.\n");
				close(s);
				free(buff);
				s = -1;
				continue;
			}
			buff[2]=(0x30 + count + 1);
			printf("Connected. Written %dbytes. [%d]:SCTP\n",
					writeNum,count);
			/*******************************/
			if(count>1){
				for(ii=5;ii<100000000;++ii){
				  if(    ii%10000000==0)printf("xxx%dxxx\n",ii);
				}
			}
		}
		/*************************************/
		for(ii=5;ii<1000000000;++ii){
		  if(    ii%100000000==0) printf("xxx%dxxx\n",ii);
		}
		/*************************************/

		close(s);
		free(buff);
		free(buf2);
		printf("SCTP client END.\n");
		exit(0);
	}

	fprintf(stderr, "no destination to connect to.\n");
	exit(1);

}
