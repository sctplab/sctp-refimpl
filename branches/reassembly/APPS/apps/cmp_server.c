#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
typedef unsigned short uint16_t;
#define IPPROTO_SCTP	132
#define close		closesocket
#else
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#endif /* WIN32 */
#ifdef __NetBSD__
#include <sys/inttypes.h>
#endif

#ifdef WIN32
/* getopt() stuff... */
int	opterr = 1,		/* if error message should be printed */
	optind = 1,		/* index into parent argv vector */
	optopt,			/* character checked for validity */
	optreset;		/* reset getopt */
char	*optarg;		/* argument associated with option */

#define	BADCH	(int)'?'
#define	BADARG	(int)':'
#define	EMSG	""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int
getopt(nargc, nargv, ostr)
	int nargc;
	char * const *nargv;
	const char *ostr;
{
	static char *place = EMSG;		/* option letter processing */
	char *oli;				/* option letter list index */

	if (optreset || !*place) {		/* update scanning pointer */
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return (EOF);
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++optind;
			place = EMSG;
			return (EOF);
		}
	}      					/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' ||
	    !(oli = strchr(ostr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (optopt == (int)'-')
			return (EOF);
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			(void)fprintf(stderr,
			    "%s: illegal option -- %c\n", __FILE__, optopt);
		return (BADCH);
	}
	if (*++oli != ':') {			/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {					/* need an argument */
		if (*place)			/* no white space */
			optarg = place;
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    __FILE__, optopt);
			return (BADCH);
		}
	 	else				/* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt);			/* dump back option letter */
}
#endif /* WIN32 */

static int immitation_mode = 0;


struct txfr_request{
	int sizetosend;
	int blksize;
	int snd_window;
	int rcv_window;
	uint8_t tos_value;
};

int
main(int argc, char **argv)
{
	struct txfr_request *req;
	char buffer[600000];
	int i,fd,newfd,ret;
	uint32_t sizetosend,blksize,numblk,sb;
	uint16_t port=0;
	int optval,optlen;
	socklen_t sa_len;
	int snd_buf=200;
	char *addr_to_bind=NULL;
	int maxburst=0;
#ifdef WIN32
	unsigned short versionReq = MAKEWORD(2,2);
	WSADATA wsaData;
	int status;
	int protocol_touse = IPPROTO_TCP;
#else
	int protocol_touse = IPPROTO_SCTP;
#endif /* WIN32 */
	struct sockaddr_in bindto,got,from;
	struct sctp_sndrcvinfo s_info,*sinfo=NULL;;
	int cnt_written=0;
	int maxseg=0;
	int print_before_write=0;
	char name[64];

	memset(&s_info, 0, 0);
	optlen = sizeof(optval);
	sb = 0;
	while((i= getopt(argc,argv,"itsp:b:Pz:S:m:B:")) != EOF){
		switch(i){
		case 'i':
#ifdef SCTP_EOR
			immitation_mode = 1;
			s_info.sinfo_flags = SCTP_EOR;
#endif
			break;
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
		case 's':
#ifdef WIN32
			printf("windows doesn't support SCTP... ignoring...\n");
#else
			protocol_touse = IPPROTO_SCTP;
#endif /* WIN32 */
			break;
		case 't':
			protocol_touse = IPPROTO_TCP;
			break;
		case 'p':
			port = (uint16_t)strtol(optarg,NULL,0);
			break;
		case 'b':
			sb = strtol(optarg,NULL,0);
			break;
		};
	}
#ifdef WIN32
	/* init winsock library */
	status = WSAStartup( versionReq, &wsaData );
	if (status != 0 ) {
		printf("can't init winsock library!\n");
		return(-1);
	}

#else
	signal(SIGPIPE,SIG_IGN);
#endif /* !WIN32 */
	fd = socket(AF_INET, SOCK_STREAM, protocol_touse);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	if ((protocol_touse == IPPROTO_SCTP) && immitation_mode) {
		int one = 1;
		if(setsockopt(fd, protocol_touse, SCTP_EXPLICIT_EOR, &one, sizeof(one)) < 0) {
			printf("cmp_server: setsockopt: SCTP_EXPLICIT_EOR failed! errno=%d\n", errno);
		}
		if(setsockopt(fd, protocol_touse, SCTP_PARTIAL_DELIVERY_POINT, &one, sizeof(one)) < 0) {
			printf("cmp_server: setsockopt: SCTP_PARTIAL_DELIVERY_POINT failed! errno=%d\n", errno);
		}
	}
	sprintf(name,"./out_log_oferr_tcp.txt");
#if defined (SCTP_MAXBURST) && !defined(WIN32) && !defined(linux)
	if((protocol_touse == IPPROTO_SCTP) && maxburst){
		int sz;
		sz = sizeof(maxburst);
		printf("Setting max burst to %d\n",maxburst);
		if(setsockopt(fd,IPPROTO_SCTP,
			      SCTP_MAXBURST,
			      &maxburst, sz) != 0) {
			perror("Failed maxburst set");
		}
		sprintf(name,"./out_log_oferr_sctp.txt");

	}
#endif /* !WIN32 && !linux */

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
#ifdef WIN32
		printf("windoze doesn't support TCP_MAXSEG... ignorning...\n");
#else
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
#endif /* WIN32 */
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
	sizetosend = (uint32_t)ntohl(req->sizetosend);
	blksize =  (uint32_t)ntohl(req->blksize);
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
	numblk = sizetosend;
	numblk++;
	printf("Client would like %u msgs of %u byte, I will send:%d (sndbuf:%d)\n",
	       sizetosend,blksize,numblk,optval);
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
		if (protocol_touse == IPPROTO_SCTP){
			if(immitation_mode && (numblk == 1)) {
				sinfo = &s_info;
			}
			ret = sctp_send(newfd, 
					buffer, 
					blksize,
					sinfo,
					0);
		} else {
			ret = sendto(newfd,buffer,blksize,0,
				     (struct sockaddr *)&from,
				     sizeof(from));
		}
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
#ifdef WIN32
	WSACleanup();
#endif /* WIN32 */
	return(0);
}

