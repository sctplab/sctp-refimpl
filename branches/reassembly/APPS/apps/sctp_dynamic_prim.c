#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <poll.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/sctp.h>

int
main(int argc, char **argv)
{
	union sctp_sockstore addr;
	int sd, ret;
	if(argc < 2) {
		printf("Use %s <ip address>\n", argv[0]);
		return(-1);
	}
	memset(&addr, 0, sizeof(addr));
	if(inet_pton(AF_INET6,argv[1], &addr.sin6.sin6_addr)) {
		addr.sin6.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
		addr.sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	} else {
		if (inet_pton(AF_INET, argv[1], &addr.sin.sin_addr) ){
			addr.sin.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
			addr.sin.sin_len = sizeof(struct sockaddr_in);
#endif
		} else {
			printf("Unreconginzed address for string '%s'\n",
			       argv[1]);
			return(-1);
		}
	}
	sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(sd == -1) {
		printf("Can't open socket for sctp err:%d\n", errno);
	}
	errno = 0;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_SET_DYNAMIC_PRIMARY,
			 &addr, sizeof(addr));
	printf("Dynamic set primary complete - returns %d err:%d\n", 
	       ret, errno);
	close(sd);
	return(0);
}
