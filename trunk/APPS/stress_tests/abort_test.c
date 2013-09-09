/*
 * Copyright (c) 2008-2010 Michael Tuexen, tuexen@freebsd.org
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

#define NUMBER_OF_THREADS 100
#define WAITTIME 10

static int done;
static in_port_t port;

in_port_t
sctp_get_local_port(int fd)
{
	struct sockaddr_in addr;
	socklen_t addr_len;

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	(void)getsockname(fd, (struct sockaddr *) &addr, &addr_len);
	return (in_port_t)ntohs(addr.sin_port);
}

static void *
server(void *arg)
{
	int fd;
	struct sockaddr_in addr;
	char buffer[] = "Tschuess!";

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_len         = sizeof(struct sockaddr_in);
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
	}

	port = sctp_get_local_port(fd);

	if (listen(fd, 1) < 0) {
		perror("listen");
	}

	sleep(WAITTIME);
	
	if (sctp_sendmsg(fd, buffer, strlen(buffer), NULL, 0, 0, SCTP_ABORT|SCTP_SENDALL, 0, 0, 0) < 0) {
		perror("sctp_sendmsg");
	}
	return (NULL);
}

static void *
create_association(void *arg)
{
	struct sockaddr_in remote_addr, local_addr;
	int fd;
	char buffer[1024];

	memset((void *)&remote_addr, 0, sizeof(struct sockaddr_in));
	remote_addr.sin_family      = AF_INET;
	remote_addr.sin_len         = sizeof(struct sockaddr_in);
	remote_addr.sin_port        = htons(port);
	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	memset((void *)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_len         = sizeof(struct sockaddr_in);
	local_addr.sin_family      = AF_INET;
	local_addr.sin_port        = htons(0);
	local_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		perror("socket");
	}
	if (bind(fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
	}
	if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("connect");
	}
	if (recv(fd, buffer, sizeof(buffer), 0) < 0) {
		perror("recv");
	}
	if (close(fd) < 0) {
		perror("close");
	}
	return (NULL);
}

int
main() {
	pthread_t server_tid, client_tid[NUMBER_OF_THREADS];
	unsigned int i;

	pthread_create(&server_tid, NULL, &server, NULL);
	sleep(1);
	for (i = 0; i < NUMBER_OF_THREADS; i++) {
		pthread_create(&client_tid[i], NULL, &create_association, NULL);
	}
	pthread_join(server_tid, NULL);
	for (i = 0; i < NUMBER_OF_THREADS; i++) {
		pthread_join(client_tid[i], NULL);
	}
	return (0);
}
