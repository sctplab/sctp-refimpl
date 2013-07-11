#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

int
sctp_configure_address(union sctp_sockstore *s, char *host, char *port)
{
	uint16_t lport;

	if (port) {
		lport = htons(((uint16_t) strtol(port, NULL, 0)));
	} else {
		lport = 0;
	}
	if (host == NULL) {
		/* Foo bar */
		return (-1);
	}
	if (inet_pton(AF_INET6, host, (void *)&s->sin6.sin6_addr) == 1) {
		s->sin6.sin6_port = lport;
		s->sin6.sin6_len = sizeof(struct sockaddr_in6);
		s->sa.sa_family = AF_INET6;
	} else if (inet_pton(AF_INET, host, (void *)&s->sin.sin_addr) == 1) {
		s->sin.sin_port = lport;
		s->sin.sin_len = sizeof(struct sockaddr_in);
		s->sa.sa_family = AF_INET;
	} else {
		struct hostent *add;

		/* Try and see if its a host name */
		add = gethostbyname2(host, AF_INET);
		if (add == NULL) {
			/* Maybe its V6? */
			add = gethostbyname2(host, AF_INET6);
			if (add) {
				/* Pull a v6 address */
				s->sin6.sin6_port = lport;
				s->sin6.sin6_family = AF_INET6;
				bcopy(add->h_addr_list[0],
				    (char *)&s->sin6.sin6_addr,
				    sizeof(struct in6_addr));
				s->sin6.sin6_len = sizeof(struct sockaddr_in6);
			}
		} else {
			/* Pull a v4 address */
			s->sin.sin_port = lport;
			s->sin.sin_family = AF_INET;
			bcopy(add->h_addr_list[0],
			    (char *)&s->sin.sin_addr.s_addr,
			    sizeof(struct in_addr));
			s->sin.sin_len = sizeof(struct sockaddr_in);
			return (0);
		}
		printf("SCTP:Can't translate address %s\n", host);
		return (-1);
	}
	return (0);
}

int
main(int argc, char **argv)
{
	char buf[65535];
	struct sctp_sndrcvinfo sinfo;
	int i,sd,ret;
	size_t sendbufsiz=0;
	size_t send_siz = 0;
	char *host=NULL;
	char *port=NULL;
	int delay_after_send=60;
	int optlen;
	int connect_first=0;
	int bulk_seen = 0;
	int number = 1;

	union sctp_sockstore s;
	while((i= getopt(argc,argv,"h:p:n:s:S:d:bc?")) != EOF)
	{
		switch(i) {
		case 'b':
			bulk_seen = 1;
			break;
		case 'c':
			connect_first = 1;
			break;
		case 'S':
			sendbufsiz = strtol(optarg, NULL, 0);
			break;
		case 'd':
			delay_after_send = strtol(optarg, NULL, 0);
			break;
		case 's':
			send_siz = strtol(optarg, NULL, 0);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'n':
			number = strtol(optarg, NULL, 0);
			break;
		case '?':
		default:
			goto out_of;
			break;
		};
	}
	if ((host == NULL) ||
	    (port == NULL) ||
	    (sendbufsiz == 0) ||
	    (send_siz == 0)) {
	out_of:
		printf("Use %s -h host -p port -S snd-socket-buf-size -s send-siz [-d delay -c -n num -b]\n",
		       argv[0]);
		return(-1);
	}
	if (sctp_configure_address(&s, host, port)) {
		printf("host:%s port:%s invalid combination\n",
		       host, port);
		return (-1);
	}
	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1) {
		printf("Can't open sctp socket err:%d\n", errno);
		return (-1);
	}
	optlen = sizeof(sendbufsiz);
	if(setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &sendbufsiz, optlen) != 0){
		printf("err:%d could not set sndbuf to %zu\n",errno, sendbufsiz);
		return (-1);
	}
	memset(&sinfo, 0, sizeof(sinfo));
	if (connect_first) {
		printf("Connecting first with a 1 second sleep after\n");
		if (connect(sd, &s.sa, s.sa.sa_len) == -1 ) {
			printf("err:%d connect fails\n", errno);
			return (-1);
		}
		sleep(1);
	}
	/* prepare send buffer of AAAA\n0 */
	memset(buf, 'A', sizeof(buf));
	if (bulk_seen) {
		buf[0] = 'b';
		buf[1] = 'u';
		buf[2] = 'l';
		buf[3] = 'k';
	}
	buf[send_siz-1] = 0;
	buf[send_siz-2] = '\n';
	for (i = 0; i < number; i++) {
		errno = 0;
		ret = sctp_sendx(sd, buf, send_siz, &s.sa, 1, &sinfo, 0);
		printf("send returns:%d, err:%d\n", ret, errno);
	}
	/* sleep not in the loop to inspect blocking */
	printf("now sleeping for %d seconds.\n", delay_after_send);
	sleep(delay_after_send);  
	close(sd);
	return (0);
}
