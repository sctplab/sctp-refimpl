/*-
 * Copyright (c) 2009 Michael Tuexen tuexen@fh-muenster.de
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
 * $Id: sut.c,v 1.1 2009-09-03 12:22:51 tuexen Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define TIMEOUT      1
#define BUFFERSIZE   (1<<16)
#define DISCARD_PORT 9

char Usage[] =
"Usage: sut [options] [address port]\n"
"Options:\n"
"        -i      number of inbound streams\n"
"        -L      local address (Default: 0.0.0.0)\n"
"        -N      maximum number of INIT retransmissions\n"
"        -o      number of inbound streams\n"
"        -p      port (Default: 0)\n"
"        -T      RTO_Max for INITs in seconds\n";

static void
start_server(in_addr_t local_addr,
             uint16_t istreams, uint16_t ostreams) {
	int fd;
	struct sockaddr_in addr;
	struct sctp_initmsg initmsg;
	char *buffer;
	ssize_t n;

	buffer = (char *)malloc(BUFFERSIZE);
	if (buffer == NULL) {
		return;
	}

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0 ) {
		perror("socket");
		return;
	}

	initmsg.sinit_num_ostreams   = ostreams;
	initmsg.sinit_max_instreams  = istreams;
	initmsg.sinit_max_attempts   = 0;
	initmsg.sinit_max_init_timeo = 0;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG, (const void *)&initmsg, (socklen_t)sizeof(struct sctp_initmsg)) < 0) {
		perror("setsockopt");
		close(fd);
		return;
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_len         = sizeof(struct sockaddr_in);
	addr.sin_port        = htons(DISCARD_PORT);
	addr.sin_addr.s_addr = local_addr;
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		close (fd);
		return;
	}

	if (listen(fd, 1) < 0) {
		perror("listen");
		close(fd);
		return;
	}

	do {
		n = recv(fd, buffer, BUFFERSIZE, 0);
	} while (n >= 0);
	perror("recv");
	return;
}

static void
start_client(in_addr_t local_addr, uint16_t local_port,
             in_addr_t remote_addr, uint16_t remote_port,
             uint16_t istreams, uint16_t ostreams,
             uint16_t max_attemps, uint16_t max_timeout) {
	int fd;
	struct sockaddr_in addr;
	struct sctp_initmsg initmsg;

	while (1) {
		
		if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0 ) {
			perror("socket");
			return;
		}

		initmsg.sinit_num_ostreams   = ostreams;
		initmsg.sinit_max_instreams  = istreams;
		initmsg.sinit_max_attempts   = max_attemps;
		initmsg.sinit_max_init_timeo = max_timeout;
		if (setsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG, (const void *)&initmsg, (socklen_t)sizeof(struct sctp_initmsg)) < 0) {
			perror("setsockopt");
			close(fd);
			return;
		}

		memset((void *)&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family      = AF_INET;
		addr.sin_len         = sizeof(struct sockaddr_in);
		addr.sin_port        = htons(local_port);
		addr.sin_addr.s_addr = local_addr;
		if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
			perror("bind");
			close (fd);
			return;
		}

		memset((void *)&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family      = AF_INET;
		addr.sin_len         = sizeof(struct sockaddr_in);
		addr.sin_port        = htons(remote_port);
		addr.sin_addr.s_addr = remote_addr;
		connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

		if (close(fd) < 0) {
			perror("close");
			return;
		}

		sleep(TIMEOUT);
	}
}

int
main(int argc, char **argv) {
	in_addr_t local_addr;
	uint16_t port;
	uint16_t istreams, ostreams;
	uint16_t max_attemps, max_timeout;
	int c;

	local_addr = INADDR_ANY;
	port = 0;
	istreams = 0;
	ostreams = 0;
	max_attemps = 0;
	max_timeout = 0;

	while ((c = getopt(argc, argv, "hL:i:o:N:p:T:")) != -1) {
		switch(c) {
			case 'h':
				fprintf(stderr, "%s", Usage);
				return 0;
				break;
			case 'i':
				istreams = atoi(optarg);
				break;
			case 'L':
				local_addr = inet_addr(optarg);
				break;
			case 'N':
				max_attemps = atoi(optarg);
				break;
			case 'o':
				ostreams = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'T':
				max_timeout = atoi(optarg) * 1000;
				break;
			default:
				fprintf(stderr, "%s", Usage);
				return -1;
		}
	}

	if (optind == argc) {
		start_server(local_addr, istreams, ostreams);
	} else {
		start_client(local_addr, port, inet_addr(argv[optind]), atoi(argv[optind+1]),
		             istreams, ostreams, max_attemps, max_timeout);
	}

	return 0;
}