#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef LINUX
#include <time.h>
#include <string.h>
#endif /* LINUX */

#ifdef WIN32
#include <winsock2.h>
#include <time.h>
typedef unsigned short u_int16_t;
#define close		closesocket
#define usleep(x)	Sleep(x/1000)
#else
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <netinet/tcp.h>
#endif /* WIN32 */

#ifdef WIN32
/* getopt() stuff... */
int opterr = 1,		/* if error message should be printed */
    optind = 1,		/* index into parent argv vector */
    optopt,		/* character checked for validity */
    optreset;		/* reset getopt */
char *optarg;		/* argument associated with option */

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
	char *buffer;
	int i,fd,newfd,ret,sb;
	socklen_t len;
	int numblk = 0;
	u_int16_t port=0;
	int optval;
	socklen_t optlen;
	int protocol_touse = IPPROTO_TCP;
	struct sockaddr_in bindto,got,from;
	int sleep_period = 200000; /* 200 ms */
	struct tm *current_tm;
	time_t currentTime;
	char Send_char = 'A';
	char Expect_char = 'A';
#ifdef WIN32
	unsigned short versionReq = MAKEWORD(2,2);
	WSADATA wsaData;
	int status;
#endif /* WIN32 */
	
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
#ifdef WIN32
    /* init winsock library */
    status = WSAStartup(versionReq, &wsaData);
    if (status != 0 ) {
	printf("can't init winsock library!\n");
	return(-1);
    }
#endif /* WIN32 */
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
	optval = sb + 2;
	optlen = sizeof(optval);
	if(setsockopt(newfd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, optlen) != 0) {
		printf("Can't set sendbuf\n");
		return(-1);
	}
	optval = sb;
	if(setsockopt(newfd, SOL_SOCKET, SO_RCVBUF, (const char *)&optval, optlen) != 0) {
		printf("Can't set rcvbuf\n");
		return(-1);
	}
	close(fd);
	buffer = malloc(sb);
	if(buffer == NULL) {
		printf("no memory\n");
		return(-2);
	}
	time(&currentTime);
	current_tm = localtime(&currentTime);
	printf("%s\n", asctime(current_tm));
	while(1) {
		/* collect a whole sb's worth of data */
		ret = recv(newfd,buffer,sb,0);
		if(ret == 0){
			printf("connection closes\n");
			return(0);
		}
		while(ret < sb) {
			int new_ret;
			new_ret = recv(newfd,&buffer[ret],(sb-ret),0);
			if(new_ret == 0) {
				printf("connection closes\n");
				return(0);
			}
			ret += new_ret;
		}
		for(i=0;i<sb;i++) {
			if(buffer[i] != Expect_char){
				time(&currentTime);
				current_tm = localtime(&currentTime);
				printf("%s\n", asctime(current_tm));
				printf("Ack, intrusion detected now byte %d is hex:%x\n",
				       i, (u_int)buffer[i]);
				return(0);
			}
			
		}
		/* now send a full window of B's to the client */
		for(i=0;i<sb;i++) {
			buffer[i] = Send_char;
		}
		ret = send(newfd, buffer, sb, 0 );

		if(ret < 0){
			printf("Gak, error %d on send\n",
			       errno);
			printf("Left to Send %d before exit\n",numblk);
			break;
		}
		memset(buffer, 0, sb);
		/* now wait the sleep period (default is 200ms) */
		usleep(sleep_period);
	}
	close(newfd);
#ifdef WIN32
	WSACleanup();
#endif /* WIN32 */
	return(0);
}

