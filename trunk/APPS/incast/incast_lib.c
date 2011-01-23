#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <net/if.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/queue.h>
#include <incast_fmt.h>

int
translate_ip_address(char *host, struct sockaddr_in *sa)
{
	struct hostent *hp;
	int len, cnt, i;

	if (sa == NULL) {
		return (-1);
	}
	len = strlen(host);
	cnt = 0;
	for (i = 0; i < len; i++) {
		if (host[i] == '.')
			cnt++;
	}
	if (cnt < 3) {
		/* make it treat it like a host name */
		sa->sin_addr.s_addr = 0xffffffff;
	}
	sa->sin_len = sizeof(struct sockaddr_in);
	sa->sin_family = AF_INET;
	if (sa->sin_addr.s_addr == 0xffffffff) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			return (htonl(strtoul(host, NULL, 0)));
		}
		memcpy(&sa->sin_addr, hp->h_addr_list[0], sizeof(sa->sin_addr));
	} else {
		sa->sin_addr.s_addr = htonl(inet_network(host));
	}
	if (sa->sin_addr.s_addr == 0xffffffff) {
		return (-1);
	}
	return (0);
}

void 
incast_run_clients(struct incast_control *ctrl)
{
	return;
}

