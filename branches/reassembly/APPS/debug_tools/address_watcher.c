/*-
 * Copyright (C) 2002-20012 Michael Tuexen, tuexen@fh-muenster.de
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
	((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof (uint32_t)) : sizeof(uint32_t)))

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

const char *
satostr(struct sockaddr *sa, char *dst, socklen_t size)
{
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	void *src;

	if ((sa->sa_family) == AF_INET) {
		sin = (struct sockaddr_in *)sa;
		src = (void *)&sin->sin_addr.s_addr;
	} else {
		sin6 = (struct sockaddr_in6 *)sa;
		src = (void *)&sin6->sin6_addr;
	}
	return inet_ntop(sa->sa_family, src, dst, size);
}

char *name[] = {"Unknown",
               "RTM_ADD",
	       "RTM_DELETE",
	       "RTM_CHANGE",
	       "RTM_GET",
	       "RTM_LOSING",
	       "RTM_REDIRECT",
	       "RTM_MISS",
	       "RTM_LOCK",
	       "RTM_OLDADD",
	       "RTM_OLDDEL",
	       "RTM_RESOLVE",
	       "RTM_NEWADDR",
	       "RTM_DELADDR",
	       "RTM_IFINFO",
	       "RTM_NEWMADDR",
	       "RTM_DELMADDR",
	       "RTM_IFINFO2",
	       "RTM_NEWMADDR2",
	       "RTM_GET2" };

int 
main (int argc, const char * argv[])
{
	int fd;
	struct ifa_msghdr *ifa;
	char *buffer;
	char str[100];
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	ssize_t n;

	if ((fd = socket(AF_ROUTE, SOCK_RAW, 0)) < 0) {
		perror("socket");
	}
	while (1) {
		buffer = calloc(1, BUFFER_LENGTH);
		ifa = (struct ifa_msghdr *) buffer;

		n = read(fd, ifa, BUFFER_LENGTH);
		printf("read() returned %ld, msglen = %d.\n", n, ifa->ifam_msglen);
		sa = (struct sockaddr *) (ifa + 1);
		get_rtaddrs(ifa->ifam_addrs, sa, rti_info);

		switch(ifa->ifam_type) {
		case RTM_NEWADDR:
			printf("Address %s added.\n", satostr(rti_info[RTAX_IFA], str, sizeof(str)));
			break;
		case RTM_DELADDR:
			printf("Address %s deleted.\n", satostr(rti_info[RTAX_IFA], str, sizeof(str)));
			break;
		default:
			printf("Routing message of type %s.\n", name[ifa->ifam_type]);
			break;
		}
		fflush(stdout);
	}
	return 0;
}
