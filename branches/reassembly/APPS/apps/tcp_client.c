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
#include <arpa/inet.h>
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

int read_history[100];
int read_hist_at = 0;

int
main(int argc, char **argv)
{
	char *buffer;
	int i,fd,ret,sb;
	socklen_t len;
	u_int16_t port=0;
	int optval;
	socklen_t optlen;
	struct sockaddr_in sendaddr,from;
	char *host_to=NULL;
	int sleep_period = 200000; /* 200 ms */
	struct tm *current_tm;
	time_t currentTime;
#ifdef WIN32
	unsigned short versionReq = MAKEWORD(2,2);
	WSADATA wsaData;
	int status;
#endif /* WIN32 */
	char Send_char = 'A';
	char Expect_char = 'A';

	optlen = sizeof(optval);
	sb = 57000;
	while((i= getopt(argc,argv,"h:p:b:s:E:S:?")) != EOF){
		switch(i){
		case 'E':
			Expect_char = optarg[0];
			break;
		case 'S':
			Send_char = optarg[0];
			break;

		case 'h':
			host_to = optarg;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		case 'b':
			sb = strtol(optarg,NULL,0);
			break;
		case 's':
			sleep_period = strtol(optarg,NULL,0);
			break;
		default:
		case '?':
			printf("Use %s -h ip.addr.of.server -p port-of-server [-b sb-size -s sleep-time -E expect-char -S send-char]\n",
			       argv[0]);
			return(-1);
			break;
		};
	}
	if( (host_to == NULL) || (port == 0) ) {
		printf("Use %s -h host -p port [-b window-size -s sleep-time]\n",
		       argv[0]);
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
	buffer = malloc(sb);
	if(buffer == NULL) {
		printf("no memory\n");
		return(-2);
	}
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	optval = sb + 2;
	optlen = sizeof(optval);
	if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, optlen) != 0) {
		printf("Can't set sendbuf\n");
		return(-1);
	}
	optval = sb;
	if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&optval, optlen) != 0) {
		printf("Can't set rcvbuf\n");
		return(-1);
	}
#if defined(WIN32) || defined(LINUX)
	/* no sin_len member */
#else
	sendaddr.sin_len = sizeof(sendaddr);
#endif /* WIN32 || LINUX */
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(port);
	inet_pton(AF_INET, host_to, (void *) &sendaddr.sin_addr);
	errno = 0;
	if(connect(fd, (struct sockaddr *)&sendaddr, sizeof(sendaddr)) == -1) {
		perror("Failed to connect");
		return(-1);
	}
	len = sizeof(from);
	if(getsockname(fd,(struct sockaddr *)&from,&len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		return(-1);
	}	
	printf("Connection to %x:%d via my addr %x:%d\n",
	       (u_int)ntohl(sendaddr.sin_addr.s_addr),
	       (int)ntohs(sendaddr.sin_port),
	       (u_int)ntohl(from.sin_addr.s_addr),
	       (int)ntohs(from.sin_port));

        time(&currentTime);
	current_tm = localtime(&currentTime);
	printf("%s\n", asctime(current_tm));
	while(1) {
		/* now send a full window of B's to the client */
		for(i=0;i<sb;i++) {
			buffer[i] = Send_char;
		}
		ret = send(fd, buffer , sb, 0 );
		if(ret < 0){
			printf("Gak, error %d on send\n",
			       errno);
			break;
		}
		memset(buffer, 0, sb);
		/* collect a whole sb's worth of data */
		ret = recv(fd,buffer,sb,0);
		if(ret == 0) {
                        printf("connection closes errno:%d\n", errno);
			time(&currentTime);
			current_tm = localtime(&currentTime);
			printf("%s\n", asctime(current_tm));
			return(0);
		}
		read_hist_at = 0;
		read_history[read_hist_at] = ret;
		read_hist_at++;
		while(ret < sb) {
			int new_ret;
			new_ret = recv(fd,&buffer[ret],(sb-ret),0);
			if(read_hist_at < 100) {
				read_history[read_hist_at] = new_ret;
				read_hist_at++;
			}
			if(new_ret == 0) {
			        printf("connection closes errno:%d\n", errno);
  			        time(&currentTime);
				current_tm = localtime(&currentTime);
				printf("%s\n", asctime(current_tm));
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
				for(i=0;i<read_hist_at;i++){
					printf("Read history %d = return %d\n",
					       i, read_history[i]);
				}
				return(0);
			}
			
		}
		/* now wait the sleep period (default is 200ms) */
		usleep(sleep_period);
	}
	close(fd);
#ifdef WIN32
	WSACleanup();
#endif /* WIN32 */
	return(0);
}
