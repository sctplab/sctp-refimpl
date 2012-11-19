#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#ifdef WIN32
#include <winsock2.h>
#include <time.h>
typedef unsigned short uint16_t;
#define IPPROTO_SCTP	132
#define close		closesocket
#define sleep(x)	Sleep(x * 1000)
DWORD tmpUptime;	/* in milliseconds */
/* using GetTickCount() assumes system has been up for less than 49.7 days...
   so, we're pretty safe if we're running Window :-P */
#define gettimeofday(x, y)	{tmpUptime = GetTickCount(); \
                                 (x)->tv_sec = tmpUptime / 1000; \
                                 (x)->tv_usec = (tmpUptime % 1000) * 1000;}
#else
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <arpa/inet.h>
#ifdef linux
#include <time.h>
#endif
#endif /* WIN32 */
#include <signal.h>
#include <string.h>
#ifdef __NetBSD__
#include <sys/inttypes.h>
#endif

static int imitation_mode = 0;

struct txfr_request {
	int sizetosend;
	int blksize;
	int snd_buf;
	int rcv_buf;
	uint8_t tos_value;
};

struct control_info {
    struct txfr_request req;
    char error_rate[32];
};

char *error_rate;
int blocks_is_error = 0;

static uint8_t tos_value=0;

#ifdef linux
#ifndef MSG_NOTIFICATION
#define MSG_NOTIFICATION	0x10000
#endif
#endif

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

#ifndef WIN32
static int
handle_notification(char *notify_buf)
{
    union sctp_notification *snp;
    struct sctp_assoc_change *sac;
    int retval = 0;
    snp = (union sctp_notification *)notify_buf;
    switch(snp->sn_header.sn_type) {
    case SCTP_ASSOC_CHANGE:
        sac = &snp->sn_assoc_change;
        switch(sac->sac_state) {

        case SCTP_COMM_UP:
            break;
        case SCTP_COMM_LOST:
            printf("Communication LOST\n");
	    printf("Sleeping\n");
	    sleep(3600);
            retval = 1;
            break;
        case SCTP_RESTART:
            break;
        case SCTP_SHUTDOWN_COMP:
            retval = 1;
            break;
        case SCTP_CANT_STR_ASSOC:
            printf("Could not start association\n");
            retval = 1;
            break;
        default:
            break;
        } /* end switch(sac->sac_state) */
        break;
    case SCTP_PEER_ADDR_CHANGE:
        break;
    case SCTP_REMOTE_ERROR:
        break;
    case SCTP_SEND_FAILED:
        break;
    case SCTP_ADAPTION_INDICATION:
        break;
    
    case SCTP_PARTIAL_DELIVERY_EVENT:
        break;

    case SCTP_SHUTDOWN_EVENT:
        retval = 2;
        break;
    default:
        printf("Unknown notification event type=%xh\n", 
               snp->sn_header.sn_type);
	retval = 6;
    } /* end switch(snp->sn_header.sn_type) */
    return(retval);
}
#endif /* !WIN32 */

void
cmp_client_dump_notify(char *buffer, int sz)
{
	int i;
	uint8_t *k;
	k = (uint8_t *)buffer;
	for(i=0; i<sz; i++) {
		printf("%2.2x ",
		       k[i]);
		if(((i+1) % 16)  == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

int
measure_one(struct control_info *req, 
	    struct sockaddr_in *to, 
	    int protocol_touse,
	    struct timeval *start,
	    struct timeval *end,
	    struct sockaddr_in *laddr,
	    int sctp_tcpmode)
{
	int optlen,optval;
	int fd,ret;
	uint32_t sz,cnt;
	int blksize, sinfo_flags=0;
	char buffer[600000];
#ifndef WIN32
	char controlbuf[3000];
	struct sockaddr_in from;
	int notification;
	socklen_t flen;
	struct msghdr msg;
	struct sctp_event_subscribe events;
	struct iovec iov[2];

#endif /* !WIN32 */

	if (protocol_touse == IPPROTO_TCP){
		fd = socket(AF_INET, SOCK_STREAM, protocol_touse);
	} else {
		if(sctp_tcpmode) 
			fd = socket(AF_INET, SOCK_STREAM, protocol_touse);
		else {
			fd = socket(AF_INET, SOCK_SEQPACKET, protocol_touse);
#ifdef SCTP_EOR
			if((fd > 0) && (imitation_mode)) {
				int one = 1;
				sinfo_flags = SCTP_EOR;
				if(setsockopt(fd, protocol_touse, SCTP_EXPLICIT_EOR, &one, sizeof(one)) < 0) {
					printf("m_cmp_client: setsockopt: SCTP_EXPLICIT_EOR failed! errno=%d\n", errno);
				}
				if(setsockopt(fd, protocol_touse, SCTP_PARTIAL_DELIVERY_POINT, &one, sizeof(one)) < 0) {
					printf("m_cmp_client: setsockopt: SCTP_PARTIAL_DELIVERY_POINT failed! errno=%d\n", errno);
				}
			}
#endif
		}

#ifndef WIN32
		memset(&events,0, sizeof(events));
		events.sctp_association_event = 1;
/*		events.sctp_partial_delivery_event = 1;*/
		if (setsockopt(fd, IPPROTO_SCTP, 
			       SCTP_EVENTS, &events, 
			       sizeof(events)) != 0) {
			perror("Can't set SCTP assoc events, we may hang");
		}
#endif
	}
	if(laddr) {
		if (bind(fd, (struct sockaddr *)laddr, sizeof(struct sockaddr_in))) {
			perror("Bind failed?");
		}
	}
	if(fd == -1){
		printf("can't open socket error:%d\n",errno);
		return(-1);
	}
	if(req->req.snd_buf){
		optlen = 4;
		optval = htonl(req->req.snd_buf);
#if defined(__APPLE__) || defined(linux)
		printf("Set send buffer to %d\n", htonl(req->req.snd_buf));
#else
		printf("Set send buffer to %ld\n", (long)htonl(req->req.snd_buf));
#endif
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval,
			       optlen) != 0) {
			printf("err:%d could not set sndbuf to %d\n", errno, optval);
		} else {
			printf("snd buf set\n");
		}
	}
	if(req->req.tos_value) {
		if (protocol_touse == IPPROTO_TCP){
			optval = req->req.tos_value;
		} else {
			optval = req->req.tos_value + 4;
		}
		optlen = 4;
		if (setsockopt(fd, IPPROTO_IP, IP_TOS, (const char *)&optval, optlen) != 0) {
			printf("Can't set tos value to %x :-( err:%d\n", req->req.tos_value, errno);
		} else {
			printf("Set tos to %x\n", req->req.tos_value);
		}
	}
	if (req->req.rcv_buf) {
		optlen = 4;
		optval = ntohl(req->req.rcv_buf);
#if defined(__APPLE__) || defined(linux)
		printf("Set recv buffer to %d\n", htonl(req->req.rcv_buf));
#else
		printf("Set recv buffer to %ld\n", (long)htonl(req->req.rcv_buf));
#endif
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&optval,
			       optlen) != 0) {
			printf("err:%d could not set rcvbuf to %d\n", errno, optval);
		} else {
			printf("rcv buf set\n");
		}
	}

#ifndef WIN32
	iov[0].iov_base = buffer;
	iov[0].iov_len = sizeof(buffer);
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msg.msg_name = (caddr_t)&from;
	msg.msg_namelen = sizeof(from);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlbuf;
	msg.msg_controllen = sizeof(controlbuf);
#endif /* !WIN32 */

	gettimeofday(start, NULL);
	gettimeofday(end, NULL);
	if((protocol_touse == IPPROTO_TCP) || sctp_tcpmode){
		if(connect(fd,(struct sockaddr *)to,sizeof(*to)) == -1){
			printf("Sorry connect fails %d\n",errno);
			close(fd);
			return(-1);
		}
	}
	flen = sizeof(struct sockaddr_in);
	cnt = 0;
	sz = ntohl(req->req.sizetosend);
	blksize = ntohl(req->req.blksize);
	if ((protocol_touse == IPPROTO_SCTP) && (sctp_tcpmode == 0)) {
#ifndef WIN32
		ret = sctp_sendmsg(fd, &req->req, sizeof(req->req),
				   (struct sockaddr *)to, sizeof(*to),
				   0, sinfo_flags, 0, 0, 0);
#endif /* !WIN32 */
	} else {
		ret = send(fd, (const char *)&req->req, sizeof(req->req), 0);
	}
	if(ret <= 0){
		printf("Huh send of txfr fails\n");
	exit_now:
		close(fd);
		return(-1);
	}
	while(sz > cnt){
#ifdef WIN32
		/* win32 doesn't to recvmsg() unless under XP */
		ret += recv(fd, buffer, blksize, 0);
		if (ret <= 0)
			goto exit_now;
#else
		if(protocol_touse == IPPROTO_SCTP) {
			struct sockaddr_storage addr;
			flen = sizeof (addr) ;
			msg.msg_flags = 0;
			ret += sctp_recvmsg (fd, buffer, sizeof(buffer), 	
					    (struct sockaddr *)&addr,
					    &flen, NULL, &msg.msg_flags);
		}
		else
			ret += recvmsg(fd,&msg,0);
		if(ret <= 0){
			goto exit_now;
		}
		if (msg.msg_flags & MSG_NOTIFICATION) {
			if((notification = handle_notification(buffer))){
				if(notification == 1) {
					printf("exit type notification\n rcvd:%d needed %d",cnt, sz);
					goto exit_now;
				} else {
					if(notification == 6) {
						/*cmp_client_dump_notify(buffer, ret);*/
					} else {
						printf("shutdown:%d need:%d > rcvd:%d?\n", notification,
						       sz, cnt);
						printf("Force to end\n");
						cnt = sz;
					}
				}
			}
			continue;
		}
#endif /* WIN32 */
		if(ret >= blksize) {
			while(ret > blksize) {
				cnt++;
				ret -= blksize;
			}
		}
	}
	gettimeofday(end, NULL);
	close(fd);
	return(0);
}

int notdone = 1;
int pass_count=0;

void
signal_handler(int sig)
{
    printf("Got signal %d will complete current pass (pass %d) and stop\n",
	   sig,pass_count);
    notdone = 0;
    return;
}

char *control=NULL;

void
log_measurement(int proto,struct control_info *ctl, struct timeval *start,
		struct timeval *end)
{
    int sec,usec;
    FILE *out_log;
    char filename[256];

    sec = end->tv_sec - start->tv_sec;
    if (end->tv_usec >= start->tv_usec) {
	usec = end->tv_usec - start->tv_usec;
    }else{
	sec--;
	usec = (end->tv_usec + 1000000) - start->tv_usec;
    }
    sprintf(filename,"%s.data",control);
    out_log = fopen(filename,"a+");
    if(out_log == NULL){
	printf("Can't open out_log:%s errno:%d\n",
	       filename,errno);
	printf("time was %d.%d\n",
	       sec,usec);
	exit(0);
    }
    /* We output "Error_rate time */
    if(blocks_is_error) {
	    fprintf(out_log,"%u %d.%6.6d # rb:%d size:%u:%s:%d:%s",
		    (int)htonl(ctl->req.blksize),
		    sec,usec,
		    (int)ntohl(ctl->req.rcv_buf),
		    (int)ntohl(ctl->req.sizetosend),
		    ((proto == IPPROTO_SCTP) ? "sctp" : "tcp"),
		    (int)ntohl(ctl->req.blksize),
		    (imitation_mode) ?  "im" : "ni"
	);
    } else {
	    fprintf(out_log,"%s %d.%6.6d # rb:%d size:%u:%s:%d:%s",
		    error_rate,sec,usec,
		    (int)ntohl(ctl->req.rcv_buf),
		    (int)ntohl(ctl->req.sizetosend),
		    ((proto == IPPROTO_SCTP) ? "sctp" : "tcp"),
		    (int)ntohl(ctl->req.blksize),
		    (imitation_mode) ?  "im" : "ni"
	);
    }
    {
	    double time_of_tran,bw, blks, sizt;
	    char *rate;

	    time_of_tran = (sec * 1000000.0);
	    time_of_tran += (usec * 1.0);
	    time_of_tran /= 1000000.0;
	    sizt = 1.0 * ntohl(ctl->req.sizetosend);
	    blks = ntohl(ctl->req.blksize);

	    bw = (8.0 * sizt * blks) / time_of_tran;
	    if(bw > 1000000000.0) {
		    rate = "Gigabits";
		    bw /= 1000000000.0;
	    } else if( bw > 1000000) {
		    rate = "Megabits";
		    bw /= 1000000.0;
	    } else if( bw > 1000) {
		    rate = "kbits";
		    bw /= 1000.0;
	    } else {
		    rate = "bits";
	    }
	    printf("Transfered via %s %d records at %f %s per second\n",
		   ((proto == IPPROTO_SCTP) ? "sctp" : "tcp"),
		   (int)ntohl(ctl->req.sizetosend),bw,rate);
	    fprintf(out_log, " (%f %s per sec)\n",bw,rate);
    }
    fclose(out_log);
}

struct control_info *
load_control_file(char *control,int *num_ctl)
{
    char linebuf[256],*tok;
    struct control_info *ctl;
    int cnt,line_read,line_len;
    char *colon=":";
    char *newline="\n";

    FILE *readfrom;

    *num_ctl = 0;
	
    readfrom = fopen(control,"r");
    if(readfrom == NULL){
	return(NULL);
    }
    line_read = cnt = 0;
    /* how many are there? */

    while(fgets(linebuf,sizeof(linebuf),readfrom) != NULL){
	line_read++;
	if(linebuf[0] == '#'){
	    /* comment */
	    continue;
	}
	cnt++;
	line_len = strlen(linebuf);
	if(line_len < 10){
	    printf("Line %d syntax error, line to short len:%d - skipping\n",
		   line_read,line_len);
	    cnt--;
	    continue;
	}
	/* need to find 3 ':''s and 1 \n */
	tok = strtok(linebuf,colon);
	if(tok == NULL){
	    printf("Line %d syntax error, line does not have a :, needs 3 - skipping\n",
		   line_read);
	    cnt--;
	    continue;
	}
	tok = strtok(NULL,colon);
	if(tok == NULL){
	    printf("Line %d syntax error, line has only 1 :, needs 3 - skipping\n",
		   line_read);
	    cnt--;
	    continue;
	}
	tok = strtok(NULL,colon);
	if(tok == NULL){
	    printf("Line %d syntax error, line has only 2 :, needs 3 - skipping\n",
		   line_read);
	    cnt--;
	    continue;
	}
	tok = strtok(NULL,newline);
	if(tok == NULL){
	    printf("Line %d syntax error, line has no return? - skipping\n",
		   line_read);
	    cnt--;
	    continue;
	}

    }
    printf("there are %d entries in the control file\n",cnt);
    ctl = calloc(cnt,sizeof(struct control_info));
    if(ctl == NULL){
	printf("Gak, no memory for %d entries.. out of here\n",
	       cnt);
	exit(-1);
    }
    rewind(readfrom);	
    cnt = 0;
/*
  Control file needs
  sizetos:blksize:snd_buf:recv_buf\n
  <or>
  # comment
*/
    while (fgets(linebuf, sizeof(linebuf), readfrom) != NULL) {
	line_read++;
	if(linebuf[0] == '#'){
	    /* comment */
	    continue;
	}
	line_len = strlen(linebuf);
	if(line_len < 10){
	    continue;
	}
	/* need to find 3 ':''s and 1 \n */
	tok = strtok(linebuf,colon);
	if(tok == NULL){
	    continue;
	}
	ctl[cnt].req.tos_value = tos_value;
	ctl[cnt].req.sizetosend = htonl(strtoul(tok,NULL,0));
	tok = strtok(NULL,colon);
	if(tok == NULL){
	    continue;
	}
	ctl[cnt].req.blksize = htonl(strtoul(tok,NULL,0));
	tok = strtok(NULL,colon);
	if(tok == NULL){
	    continue;
	}
	ctl[cnt].req.snd_buf = htonl(strtol(tok,NULL,0) * 1024);
	tok = strtok(NULL,newline);
	if(tok == NULL){
	    continue;
	}
	ctl[cnt].req.rcv_buf = htonl(strtol(tok,NULL,0) * 1024);
	if(blocks_is_error == 0) {
		printf("%d: error_rate:%s transfer:%u in blocks of %u snd_buf:%d rcv_buf %d\n",
		       cnt,
		       error_rate,
		       (int)ntohl(ctl[cnt].req.sizetosend),
		       (int)ntohl(ctl[cnt].req.blksize),
		       (int)ntohl(ctl[cnt].req.snd_buf),
		       (int)ntohl(ctl[cnt].req.rcv_buf));
	} else {
		printf("%d: (block size is error-rate) transfer:%u in blocks of %u snd_buf:%d rcv_buf %d\n",
		       cnt,
		       (int)ntohl(ctl[cnt].req.sizetosend),
		       (int)ntohl(ctl[cnt].req.blksize),
		       (int)ntohl(ctl[cnt].req.snd_buf),
		       (int)ntohl(ctl[cnt].req.rcv_buf));
	}
	cnt++;
    }
    fclose(readfrom);
    *num_ctl = cnt;
    return(ctl);
}

int
main(int argc, char **argv)
{
    struct timeval start,end;
    int i,num_ctl;
    char *addr=NULL;
    uint16_t port = 0, lport = 0;
    struct control_info *ctl;
    struct sockaddr_in to;
    int maxpass = 0;
    int sctp_only=0;
    int tcp_only = 0;
    int tcp_skip=0;
    int sctp_skip=0;
    int sctp_tcpmode=0;
    char *laddr=NULL;
    struct sockaddr_in local_addr,*l2addr;
#ifdef WIN32
    unsigned short versionReq = MAKEWORD(2,2);
    WSADATA wsaData;
    int status;
#endif /* WIN32 */

    error_rate = NULL;
    while((i= getopt(argc,argv,"bp:h:f:M:ste:?B:P:TAQ:i")) != EOF){
	switch(i){
	case 'i':
		imitation_mode = 1;
		break;
	case 'P':
	    lport = (uint16_t)strtol(optarg,NULL,0);
	    break;
	case 'B':
	    laddr = optarg;
	    break;
	case 's':
#ifdef WIN32
	    printf("windows doesn't support SCTP... ignoring...\n");
#else
	    sctp_only = 1;
#endif /* WIN32 */
	    break;
	case 'Q':
		tos_value = (uint8_t)strtol(optarg,NULL,0);
		tos_value &= 0xfc;
		printf("Tos_value set to %x\n",
		       (u_int)tos_value);
		break;
	case 't':
	    tcp_only = 1;
	    break;
	case 'e':
	    error_rate = optarg;
	    break;
	case 'b':
	    blocks_is_error = 1;		
	    break;
	case 'M':
	    maxpass = strtol(optarg,NULL,0);
	    printf("max pass count set to %d\n",maxpass);
	    break;
	case 'f':
	    control = optarg;
	    break;
	case 'h':
	    addr = optarg;
	    break;
	case 'p':
	    port = (uint16_t)strtol(optarg,NULL,0);
	    break;
	case 'T':
	    sctp_tcpmode=1;		
	    break;
	case '?':
	default:
	    printf("use %s {-e err-rate|-b} -f file -h host -p port [-M max-passes -T -t -s]\n",
		   argv[0]);
	    printf("-M max-passes limits the number of passes otherwise infinite until ctrl-c\n");
	    printf("-T force SCTP to use the TCP model\n");
	    printf("-t only do TCP do not run SCTP passes\n");
	    printf("-s only do SCTP do not run TCP passes\n");
	    break;
	};
    }
    if(control == NULL){
	printf("Need a control file\n");
	printf("use %s -e err-rate -f file -h host -p port [-M max-passes]\n",
	       argv[0]);
	return(-1);
    }
    if(addr == NULL){
	printf("Sorry you must specify a to addr -h addr\n");
	printf("use %s -e err-rate -f file -h host -p port [-M max-passes]\n",
	       argv[0]);
	return(-1);
    }
    if((error_rate == NULL) && (blocks_is_error == 0)){
	printf("You must provide a -e error-rate (eg: -e 3.0)\n");
	printf("Or a -b option to vary on block size\n");
	printf("use %s {-e err-rate|-b} -f file -h host -p port [-M max-passes -s]\n",
	       argv[0]);
	printf("-s will restrict to NO TCP passes (SCTP only)\n");
	printf("-t will restrict to NO SCTP passes (TCP only)\n");
	return(-1);
    }
    memset(&to,0,sizeof(to));
    memset(&local_addr,0,sizeof(local_addr));
    if(laddr){ 
	inet_pton(AF_INET, laddr, (void *) &local_addr.sin_addr);
#if defined(HAVE_SA_LEN)
	local_addr.sin_len = sizeof(to);
#endif /* HAVE_SA_LEN */
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(lport);
	l2addr = &local_addr;
    } else {
	l2addr = NULL;
    }
    if(inet_pton(AF_INET, addr, (void *) &to.sin_addr)){
#if defined(HAVE_SA_LEN)
	to.sin_len = sizeof(to);
#endif /* HAVE_SA_LEN */
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
    }else{
	printf("Can't translate the address\n");
	return(-1);
    }
    ctl = load_control_file(control,&num_ctl);
    if(ctl == NULL){
	printf("Can't load control file :<\n");
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

    /* signal handler install */
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);
#ifndef WIN32
    signal(SIGHUP,signal_handler);
#endif /* !WIN32 */

    while(notdone){
	pass_count++;
	for(i=0;i<num_ctl;i++){
#ifndef WIN32
	    if(!tcp_only) {
		if(measure_one(&ctl[i],&to,IPPROTO_SCTP,&start,&end,l2addr,sctp_tcpmode) != 0){
		    sctp_skip++;
		    break;
		}else{
		    log_measurement(IPPROTO_SCTP,&ctl[i],&start,&end);
		}
		sleep(2);
	    } else {
		sctp_skip++;
	    }
#endif /* !WIN32 */

	    if(!sctp_only) {
		if(measure_one(&ctl[i],&to,IPPROTO_TCP,&start,&end,l2addr,sctp_tcpmode) != 0){
		    tcp_skip++;
		    break;
		}else{
		    log_measurement(IPPROTO_TCP,&ctl[i],&start,&end);
		}
		sleep(2);
	    } else {
		tcp_skip++;
	    }
	}
	if(maxpass){
	    if(maxpass == pass_count){
		printf("Pass %d now complete, program terminates\n",
		       pass_count);
		notdone = 0;
	    }
	}
	printf("Pass %d now complete at %s Failed %d TCP, Failed %d SCTP\n",
	       pass_count, ctime((const time_t *)&end.tv_sec), tcp_skip,
	       sctp_skip);
    }
#ifdef WIN32
    WSACleanup();
#endif /* WIN32 */
    return(0);
}
