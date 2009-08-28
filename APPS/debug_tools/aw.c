#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_LENGTH   1024

#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))
#define NEXT_SA(ap) ap = (struct sockaddr *) \
	((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof (u_long)) : sizeof(u_long)))
void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int		i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			NEXT_SA(sa);
		} else
			rti_info[i] = NULL;
	}
}

int main (int argc, const char * argv[])
{
	int fd;
	struct ifa_msghdr *ifa;
	char *buffer;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	char addr[INET6_ADDRSTRLEN];

	if ((fd = socket(AF_ROUTE, SOCK_RAW, 0)) <0 ) {
		perror("socket");
	}
	while (1) {
		buffer = calloc(1, BUFFER_LENGTH);
		ifa = (struct ifa_msghdr *) buffer;

		read(fd, ifa, BUFFER_LENGTH);
		sa = (struct sockaddr *) (ifa + 1);
		get_rtaddrs(ifa->ifam_addrs, sa, rti_info);

		switch(ifa->ifam_type) {
			case RTM_NEWADDR:
				if (rti_info[RTAX_IFA]->sa_family == AF_INET) {
					inet_ntop(AF_INET,
					          &((struct sockaddr_in *)rti_info[RTAX_IFA])->sin_addr,
					          addr, sizeof(addr));
				}
				if (rti_info[RTAX_IFA]->sa_family == AF_INET6) {
					inet_ntop(AF_INET6,
					          &((struct sockaddr_in6 *)rti_info[RTAX_IFA])->sin6_addr,
					          addr, sizeof(addr));
				}
				printf("Address %s added.\n", addr);
				break;
			case RTM_DELADDR:
				if (rti_info[RTAX_IFA]->sa_family == AF_INET) {
					inet_ntop(AF_INET,
					          &((struct sockaddr_in *)rti_info[RTAX_IFA])->sin_addr,
					          addr, sizeof(addr));
				}
				if (rti_info[RTAX_IFA]->sa_family == AF_INET6) {
					inet_ntop(AF_INET6,
					          &((struct sockaddr_in6 *)rti_info[RTAX_IFA])->sin6_addr,
					          addr, sizeof(addr));
				}
				printf("Address %s deleted.\n", addr);
				break;
			default:
				break;
		}
		fflush(stdout);
	}
	return 0;
}
