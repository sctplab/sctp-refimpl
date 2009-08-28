/*-
 * Copyright (c) 2009 Michael Tuexen <tuexen@fh-muenster.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: aw.c,v 1.4 2009-08-28 19:08:45 tuexen Exp $
 */

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
