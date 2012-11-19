/*
 * Copyright (C) 2008 Michael Tuexen, tuexen@fh-muenster.de,
 *
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
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMBER_OF_THREADS 250
#define RUNTIME 60
#define BUFFER_SIZE (1<<16)

static int done;
static unsigned short port;

unsigned short
sctp_get_local_port(int fd)
{
	struct sockaddr_in addr;
	socklen_t addr_len;

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	(void)getsockname(fd, (struct sockaddr *) &addr, &addr_len);
	return ntohs(addr.sin_port);
}

static void *
echo_server(void *arg)
{
	int fd;
	struct sockaddr_in addr;
	char *buffer;
	struct sctp_sndrcvinfo sinfo;
	struct sctp_event_subscribe event;
	int flags;
	ssize_t n;
	char *ids;
	socklen_t len;
	struct sctp_assoc_ids *assoc_ids;
	unsigned int i;
	
	buffer = (char *)malloc(BUFFER_SIZE);
	len = 100000;
	ids = (char *)malloc(len);
	assoc_ids = (struct sctp_assoc_ids *)ids;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket");
	}
	memset((void *)&event, 0, sizeof(struct sctp_event_subscribe));
	event.sctp_data_io_event = 1;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(struct sctp_event_subscribe)) < 0) {
		perror("setsockopt");
	}
	
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_len         = sizeof(struct sockaddr_in);
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
	}

	port = sctp_get_local_port(fd);

	if (listen(fd, 1) < 0) {
		perror("listen");
	}
	
	while (1) {
		flags = 0;
		memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));
		n = sctp_recvmsg(fd, (void *)buffer, BUFFER_SIZE, NULL, 0, &sinfo, &flags);
		//printf("Assoc id: %d.\n", (int)sinfo.sinfo_assoc_id);
		if (n < 0) {
			perror("sctp_recvmsg");
		}
		if (flags & MSG_NOTIFICATION) {
			continue;
		}
		if (n > 0) {
			n = sctp_send(fd, (const void *)buffer, n, (const struct sctp_sndrcvinfo *)&sinfo, 0);
			if (n < 0) {
				printf("SSN = %d, Assoc id: %d.\n", sinfo.sinfo_ssn, (int)sinfo.sinfo_assoc_id);
				getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST, ids, &len);
				for (i = 0; i < len / sizeof(sctp_assoc_t); i++) {
					printf("id = %d\n", assoc_ids->gaids_assoc_id[i]);
				}
			}
		}
	}
	return (NULL);
}

static void *
create_associations(void *arg)
{
	struct sockaddr_in remote_addr;
	int fd;
	struct linger linger;
	unsigned int n, l, i;
	char *buffer;
	ssize_t r;
	
	buffer = (char *)malloc(BUFFER_SIZE);

	remote_addr.sin_family      = AF_INET;
	remote_addr.sin_len         = sizeof(struct sockaddr_in);
	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	remote_addr.sin_port        = htons(port);

	linger.l_onoff  = 1;
	linger.l_linger = 10;
	
	while (!done) {
		if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
			perror("socket");
		}
		if (setsockopt( fd, SOL_SOCKET, SO_LINGER, (const void *)&linger, (socklen_t)sizeof(struct linger)) < 0 ) {
			perror("setsockopt");
		}
		if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
			perror("connect");
		}
		
		n = random() % 1024;
		for (i = 0; i < n; i++) {
			l = random()%1024 + 1;
			if ((r = send(fd, (const void *)buffer, l, 0)) != l) {
				printf("send() returned %d instead of %d.\n", (int)r, l);
			}
			if ((r = recv(fd, (void *)buffer, BUFFER_SIZE, 0)) != l) {
				printf("recv() returned %d instead of %d.\n", (int)r, l);
			}
		}
		if (close(fd) < 0) {
			perror("close");
		}
	}
	return (NULL);
}

int 
main() {
	pthread_t tid, tids[NUMBER_OF_THREADS];
	unsigned int i;
	
	pthread_create(&tid, NULL, &echo_server, (void *)NULL);
	do {
		sleep(1);
	} while (port == 0);
	done = 0;
	for(i = 0; i < NUMBER_OF_THREADS; i++) {
		pthread_create(&tids[i], NULL, &create_associations, (void *)NULL);
	}
	sleep(RUNTIME);
	done = 1;
	for(i = 0; i < NUMBER_OF_THREADS; i++) {
		pthread_join(tids[i], NULL);
	}
	sleep(1);
	return (0);
}
