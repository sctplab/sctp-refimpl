/*
 * Copyright (C) 2005 Michael Tuexen, tuexen@fh-muenster.de,
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
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef LINUX
#include <getopt.h>
#endif
#include <netinet/sctp.h>

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif


char Usage[] ="\
Usage: tsctp\n\
Options:\n\
        -v      verbose\n\
        -V      very verbose\n\
        -L      local address\n\
        -l      size of send/receive buffer\n\
        -n      number of messages sent (0 means infinite)/received\n\
        -D      turns Nagle off\n\
";

#define DEFAULT_LENGTH             1024
#define DEFAULT_NUMBER_OF_MESSAGES 1024
#define DEFAULT_PORT               5001
#define BUFFERSIZE                  (1<<16)
#define LINGERTIME                 1000

static void* handle_connection(void *arg)
{
	ssize_t n;
	unsigned long long sum = 0;
	char *buf;
	pthread_t tid;
	int fd, length;
	struct timeval start_time, now, diff_time;
	double seconds;
	unsigned long messages = 0;
	
	buf = malloc(BUFFERSIZE);
	fd = *(int *) arg;
	free(arg);
	tid = pthread_self();
	pthread_detach(tid);
	gettimeofday(&start_time, NULL);
	
	n = recv(fd, (void*)buf, BUFFERSIZE, 0);
	/*
	printf("Received %u bytes\n", n);
	fflush(stdout);
	*/
	length = n;
	while (n > 0) {
		sum += n;
		messages++;
		n = recv(fd, (void*)buf, BUFFERSIZE, 0);
		/*
		printf("Received %u bytes\n", n);
		fflush(stdout);
		*/
	}
	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &diff_time);
	seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000.0;
	fprintf(stdout, "%u, %lu, %f, %f, %f, %f\n", 
	       length, (long)messages, start_time.tv_sec + (double)start_time.tv_usec/1000000, now.tv_sec+(double)now.tv_usec/1000000, seconds, sum / seconds / 1024.0);
	fflush(stdout);
	printf("\nwaiting for close");
	close(fd);
	printf("\nwaiting for close");

	return NULL;
}


int main(int argc, char **argv)
{
	int fd, *cfdptr;
	char c, *buffer;
	socklen_t addr_len;
	struct sockaddr_in local_addr, remote_addr;
	struct timeval start_time, now, diff_time;
	int length, verbose, very_verbose, client;
	short local_port, remote_port, port;
	double seconds;
	double throughput;
	int one = 1;
	int nodelay = 0;
	unsigned long i, number_of_messages;
	pthread_t tid;
	const int on = 1;
	int rcvbufsize=0, sndbufsize=0;
	int setFP = 0;
	int SMALL_MAXSEG = 1500;

	struct linger linger;

	length             = DEFAULT_LENGTH;
	number_of_messages = DEFAULT_NUMBER_OF_MESSAGES;
	port               = DEFAULT_PORT;
	verbose            = 0;
	very_verbose       = 0;

	memset((void *) &local_addr,  0, sizeof(local_addr));
	memset((void *) &remote_addr, 0, sizeof(remote_addr));
	local_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	local_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	while ((c = getopt(argc, argv, "p:l:f:L:n:R:S:vVD")) != -1)
		switch(c) {
			case 'l':
				length = atoi(optarg);
				break;
			case 'L':
				local_addr.sin_addr.s_addr = inet_addr(optarg);
				break;
		        case 'n':
				number_of_messages = atoi(optarg);
				break;
			case 'f':
				setFP = 1;
				SMALL_MAXSEG = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'R':
				rcvbufsize = atoi(optarg);
				break;
			case 'S':
				sndbufsize = atoi(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'V':
				verbose = 1;
				very_verbose = 1;
				break;
			case 'D':
				nodelay = 1;
				break;
			default:
				fprintf(stderr, Usage);
				exit(1);
		}
	
	if (optind == argc) {
		client      = 0;
		local_port  = port;
		remote_port = 0;
	} else {
		client	    = 1;
		local_port  = 0;
		remote_port = port;
	}
	local_addr.sin_port        = htons(local_port);

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		perror("socket");
		
	if (!client)
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, (socklen_t)sizeof(on));

	if (bind(fd, (const struct sockaddr *) &local_addr, sizeof(local_addr)) != 0)
		perror("bind");

	if (!client) {
		if (listen(fd, 1) < 0)
			perror("listen");
		while (1) {
			memset(&remote_addr, 0, sizeof(remote_addr));
			addr_len = sizeof(struct sockaddr_in);
			cfdptr = malloc(sizeof(int));
			if ((*cfdptr = accept(fd, (struct sockaddr *)&remote_addr, &addr_len)) < 0)
				perror("accept");
			if (verbose)
				fprintf(stdout,"Connection accepted from %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
			if (rcvbufsize)
				if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbufsize, sizeof(int)) < 0)
					perror("setsockopt: rcvbuf");

			pthread_create(&tid, NULL, &handle_connection, (void *) cfdptr);
		}
		close(fd);
	} else {
		remote_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
		remote_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
		
		remote_addr.sin_addr.s_addr = inet_addr(argv[optind]);
		remote_addr.sin_port        = htons(remote_port);
	
		if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0)
			perror("connect");
#ifdef SCTP_NODELAY	
		if (nodelay == 1) {
			if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&one, sizeof(one)) < 0)
				perror("setsockopt: nodelay");
		}
#endif
		if(setFP == 1){
			if(setsockopt(fd,IPPROTO_SCTP ,SCTP_MAXSEG, &SMALL_MAXSEG, sizeof(SMALL_MAXSEG))< 0)
				perror("setsockopt: SCTP_MAXSEG");
		}	
		if (sndbufsize)
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int)) < 0)
				perror("setsockopt: sndbuf");
			
		buffer = malloc(length);
		gettimeofday(&start_time, NULL);
		if (verbose) {
			printf("Start sending %ld messages...", (long)number_of_messages);
			fflush(stdout);
		}
		
		i = 0;
		while ((number_of_messages == 0) || (i < number_of_messages)) {
			i++;
			if (very_verbose)
				printf("Sending message number %lu.\n", i);
			send(fd, buffer, length, 0);
		}
		if (verbose)
			printf("done.\n");
		linger.l_onoff = 1;
		linger.l_linger = LINGERTIME;
		if (setsockopt(fd, SOL_SOCKET, SO_LINGER,(char*)&linger, sizeof(struct linger))<0)
			perror("setsockopt");
		close(fd);
		gettimeofday(&now, NULL);
		timersub(&now, &start_time, &diff_time);
		seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000;
		fprintf(stdout, "%s of %ld messages of length %u took %f seconds.\n", 
		       "Sending", (long)number_of_messages, length, seconds);
		throughput = (double)(number_of_messages * length) / seconds / 1024.0;
		fprintf(stdout, "Throughput was %f KB/sec.\n", throughput);
	}
	return 0;
}
