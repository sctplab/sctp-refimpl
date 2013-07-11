#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
typedef unsigned short u_int16_t;
#define IPPROTO_SCTP	132
#define close		closesocket
#define sleep(x)	Sleep(x * 1000)
SYSTEMTIME tmpSystime;
#define gettimeofday(x, y)	{GetSystemTime(&tmpSystime); \
                                 (x)->tv_sec = tmpSystime.wSecond; \
                                 (x)->tv_usec = tmpSystime.wMilliseconds*1000;}
#else
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <errno.h>
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

/* inet_pton() stuff */
#ifndef NS_INADDRSZ
#define NS_INADDRSZ	4
#endif
#ifndef NS_IN6ADDRSZ
#define NS_IN6ADDRSZ	16
#endif
#ifndef NS_INT16SZ
#define NS_INT16SZ	sizeof(unsigned short)
#endif
/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton4(src, dst)
	const char *src;
	u_char *dst;
{
	static const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	u_char tmp[NS_INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr(digits, ch)) != NULL) {
			u_int new = *tp * 10 + (pch - digits);

			if (new > 255)
				return (0);
			*tp = new;
			if (! saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return (0);
	}
	if (octets < 4)
		return (0);
	memcpy(dst, tmp, NS_INADDRSZ);
	return (1);
}

#ifdef INET6
/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton6(src, dst)
	const char *src;
	u_char *dst;
{
	static const char xdigits_l[] = "0123456789abcdef",
			  xdigits_u[] = "0123456789ABCDEF";
	u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *xdigits, *curtok;
	int ch, saw_xdigit;
	u_int val;

	memset((tp = tmp), '\0', NS_IN6ADDRSZ);
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
			pch = strchr((xdigits = xdigits_u), ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return (0);
				colonp = tp;
				continue;
			} else if (*src == '\0') {
				return (0);
			}
			if (tp + NS_INT16SZ > endp)
				return (0);
			*tp++ = (u_char) (val >> 8) & 0xff;
			*tp++ = (u_char) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
		    inet_pton4(curtok, tp) > 0) {
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break;	/* '\0' was seen by inet_pton4(). */
		}
		return (0);
	}
	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			return (0);
		*tp++ = (u_char) (val >> 8) & 0xff;
		*tp++ = (u_char) val & 0xff;
	}
	if (colonp != NULL) {
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		const int n = tp - colonp;
		int i;

		if (tp == endp)
			return (0);
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return (0);
	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return (1);
}
#endif /*INET6*/

/* int
 * inet_pton(af, src, dst)
 *	convert from presentation format (which usually means ASCII printable)
 *	to network format (which is usually some kind of binary format).
 * return:
 *	1 if the address was valid for the specified address family
 *	0 if the address wasn't valid (`dst' is untouched in this case)
 *	-1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *	Paul Vixie, 1996.
 */
int
inet_pton(af, src, dst)
	int af;
	const char *src;
	void *dst;
{
	switch (af) {
	case AF_INET:
		return (inet_pton4(src, dst));
#ifdef INET6
	case AF_INET6:
		return (inet_pton6(src, dst));
#endif 
	default:
		errno = WSAEAFNOSUPPORT;
		return (-1);
	}
	/* NOTREACHED */
}
#endif /* WIN32 */

int
main(int argc, char **argv)
{
	char buffer[200000];
	int i,fd,ret,numblk,blksize,sizetos,cnt;
	int optval;
        socklen_t len, optlen;
	char *addr=NULL;
	u_int16_t port=0;
	unsigned int no_READ=0;
	unsigned int no_READ_times;
	int snd_buf=200;
        int sb;
	struct timeval start,end;
	int sec,usec;
#ifdef WIN32
	unsigned short versionReq = MAKEWORD(2,2);
	WSADATA wsaData;
	int status;
	int protocol_touse = IPPROTO_TCP;
#else
	int protocol_touse = IPPROTO_SCTP;
#endif /* WIN32 */
	struct txfr_request{
		int sizetosend;
		int blksize;
		int snd_window;
		int rcv_window;
		u_int8_t tos_value;
	}buf;
	struct sockaddr_in bindto,got,to;

	sizetos = 1000000;
	blksize = 1000;
        sb = 0;
	buf.sizetosend = htonl(1000000);
	buf.blksize = htonl(1000);
	buf.snd_window = 0;
	buf.rcv_window = 0;
	buf.tos_value = 0;
	while((i= getopt(argc,argv,"tsp:h:S:B:b:N:z:")) != EOF){
		switch(i){
		case 'z':
		    snd_buf = strtol(optarg,NULL,0);
		    printf("Change send buffer to %d * 1024\n",
			   snd_buf);
		    buf.snd_window = htonl(snd_buf);
		    break;
		case 'N':
		    no_READ = 1;
		    no_READ_times = strtol(optarg,NULL,0);
		    if(no_READ_times == 0)
			printf("I will NOT read any data\n");
		    else
			printf("I will NOT read for %d minutes\n",
			       no_READ_times);
		    break;
		case 'S':
			sizetos = strtol(optarg,NULL,0);
			buf.sizetosend = ntohl(sizetos);
			break;
		case 'B':
			blksize = strtol(optarg,NULL,0);
                        printf("block size %d\n",blksize);
			buf.blksize = ntohl(blksize);
			break;
		case 's':
#ifdef WIN32
			printf("windows doesn't support SCTP... ignoring...\n");
#else
			protocol_touse = IPPROTO_SCTP;
#endif /* WIN32 */
			break;
		case 'h':
			addr = optarg;
			break;
		case 't':
			protocol_touse = IPPROTO_TCP;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
                case 'b':
                    sb = strtol(optarg,NULL,0);
		    buf.rcv_window = ntohl(sb);
                    break;
		};
	}
	memset(&to,0,sizeof(to));
	if(addr == NULL){
		printf("Sorry you must specify a to addr -h addr\n");
		return(-1);
	}
	if(inet_pton(AF_INET, addr, (void *) &to.sin_addr)){
#if !defined(WIN32) && !defined(linux)
		to.sin_len = sizeof(to);
#endif /* !WIN32 */
		to.sin_family = AF_INET;
		printf("port selected is %d\n",port);
		printf("addr %x\n",(u_int)ntohl(to.sin_addr.s_addr));
		to.sin_port = htons(port);
	}else{
		printf("Can't translate the address\n");
		return(-1);
	}
#ifdef WIN32
	/* init winsock library */
	status = WSAStartup(versionReq, &wsaData);
	if (status != 0 ) {
		printf("can't init winsock library!\n");
		return(-1);
	}
#endif /* WIN32 */
	if(protocol_touse == IPPROTO_SCTP){
		fd = socket(AF_INET, SOCK_SEQPACKET, protocol_touse);
	}else{
		fd = socket(AF_INET, SOCK_STREAM, protocol_touse);
	}
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
#ifdef WIN32
		WSACleanup();
#endif /* WIN32 */
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	len = sizeof(bindto);
#if !defined(WIN32) && !defined(linux)
	bindto.sin_len = sizeof(bindto);
#endif /* !WIN32 */
	bindto.sin_family = AF_INET;
	bindto.sin_port = 0;
	if(bind(fd,(struct sockaddr *)&bindto, len) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
#ifdef WIN32
		WSACleanup();
#endif /* WIN32 */
		return(-1);
	}
	if(getsockname(fd,(struct sockaddr *)&got,&len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
#ifdef WIN32
		WSACleanup();
#endif /* WIN32 */
		return(-1);
	}	
	if(sb){
	    optlen = 4;
	    optval = snd_buf * 1024;
#ifdef WIN32
	    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, optlen) != 0){
#else
	    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, optlen) != 0){
#endif /* WIN32 */
		printf("err:%d could not set sndbuf to %d\n",errno,
		       optval);
	    }
	    optval = sb * 1024;
#ifdef WIN32
	    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&optval, optlen) != 0){
#else
	    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, optlen) != 0){
#endif /* WIN32 */
		printf("err:%d could not set rcvbuf to %d\n",errno,
		       optval);
	    }
	}
	printf("Client uses port %d\n",ntohs(got.sin_port));
	gettimeofday(&start,NULL);
	if(protocol_touse != IPPROTO_SCTP){
		if(connect(fd,(struct sockaddr *)&to,len) == -1){
			printf("Sorry connect fails %d\n",errno);
			close(fd);
#ifdef WIN32
			WSACleanup();
#endif /* WIN32 */
			return(-1);
		}
	}
	numblk = sizetos/blksize;
	if(protocol_touse == IPPROTO_SCTP){
		ret = sendto(fd,(const char *)&buf,sizeof(buf),0,
			     (struct sockaddr *)&to,sizeof(to));
	}else{
		ret = send(fd,(const char *)&buf,sizeof(buf),0);
	}
	if(ret < sizeof(struct txfr_request)){
		printf("Huh send of txfr fails\n");
	exit_now:
		close(fd);
#ifdef WIN32
		WSACleanup();
#endif /* WIN32 */
		return(-1);
	}
	if(no_READ){
	    unsigned int cnt_noread=0;
	    while(1){
		cnt_noread++;
		sleep(60);
		if(no_READ_times > 0){
		    if(cnt_noread >= no_READ_times)
			break;
		}
	    }
	}
	cnt = 0;
	numblk = 0;
	while(sizetos > cnt){
		ret = recv(fd,buffer,blksize,0);
		if(ret <= 0){
			printf("Gak, error %d on send\n",
			       errno);
			printf("cnt:%d numblk:%d... sleeping\n",
			       cnt,numblk);
			sleep(3600);
			
			goto exit_now;
		}
		cnt+= ret;
		numblk++;
	}
	gettimeofday(&end,NULL);
	printf("Recieved %d bytes in %d blocks\n",
	       sizetos,numblk);
	printf("Start %d.%6.6d\n",
	       (int)start.tv_sec,(int)start.tv_usec);
	printf("End %d.%6.6d\n",
	       (int)end.tv_sec,(int)end.tv_usec);
	sec = end.tv_sec - start.tv_sec;
	if(end.tv_usec >= start.tv_usec){
		usec = end.tv_usec - start.tv_usec;
	}else{
		sec--;
		usec = (end.tv_usec + 1000000) - start.tv_usec;
	}
	printf("Difference %d.%6.6d\n", sec,usec);
	sleep(1);
#ifdef WIN32
	WSACleanup();
#endif /* WIN32 */
	return(0);
}
