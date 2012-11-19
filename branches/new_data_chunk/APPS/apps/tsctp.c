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
#include <signal.h>
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
        -R      recv buffer\n\
        -S      send buffer\n\
        -N      number of clients\n\
        -s      number of streams\n\
        -T      time to send messages\n\
";

#define DEFAULT_LENGTH             1024
#define DEFAULT_NUMBER_OF_MESSAGES 1024
#define DEFAULT_PORT               5001
#define BUFFERSIZE                  (1<<16)
#define LINGERTIME                 1000

static unsigned int done;

void stop_sender(int sig)
{
	done = 1; 
}

static void* handle_connection(void *arg)
{
	ssize_t n;
	unsigned long long sum = 0;
	char *buf;
	pthread_t tid;
	int fd;
	struct timeval start_time, now, diff_time;
	double seconds;
	unsigned long messages = 0;
	unsigned long recv_calls = 0;
	unsigned long notifications = 0;
	struct sctp_sndrcvinfo sinfo;
	unsigned int first_length;
	int flags;
	
	fd = *(int *) arg;
	free(arg);
	tid = pthread_self();
	pthread_detach(tid);
	
	buf = malloc(BUFFERSIZE);
	n = sctp_recvmsg(fd, (void*)buf, BUFFERSIZE, NULL, NULL, &sinfo, &flags);
	gettimeofday(&start_time, NULL);
	first_length = n;
	while (n > 0) {
		recv_calls++;
		if (flags & MSG_NOTIFICATION) {
			notifications++;
		} else {
			sum += n;
			if (flags & MSG_EOR)
				messages++;
		}
		flags = 0;
		n = sctp_recvmsg(fd, (void*)buf, BUFFERSIZE, NULL, NULL, &sinfo, &flags);
	}
	if (n < 0)
		perror("sctp_recvmsg");
	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &diff_time);
	seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000.0;
	fprintf(stdout, "%u, %lu, %lu, %lu, %llu, %f, %f\n", 
	       first_length, messages, recv_calls, notifications, sum, seconds, (double)first_length * (double)messages / seconds / 1024.0);
	fflush(stdout);
	close(fd);
	free(buf);
	return NULL;
}

void handle_signal(int sig)
{
	printf("Caught signal %d.\n", sig); 
}



int main(int argc, char **argv)
{
	int fd, *cfdptr;
	size_t intlen;
	char c, *buffer;
	socklen_t addr_len;
	struct sockaddr_in local_addr, remote_addr;
	struct timeval start_time, now, diff_time;
	int length, verbose, very_verbose, client;
	short local_port, remote_port, port;
	double seconds;
	double throughput;
	const int on = 1;
	const int off = 0;
	int nodelay = 0;
	unsigned long i, number_of_messages;
	pthread_t tid;
	int rcvbufsize=0, sndbufsize=0, myrcvbufsize, mysndbufsize;
	struct msghdr msghdr;
	struct iovec iov[1024];
	struct linger linger;
	int fragpoint = 0;
	unsigned int runtime = 0;
	int n;
	
	signal(SIGHUP, handle_signal);

	memset(iov, 0, sizeof(iov));
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

	while ((c = getopt(argc, argv, "p:l:f:L:n:R:S:T:vVD")) != -1)
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
			case 'p':
				port = atoi(optarg);
				break;
			case 'f':
				fragpoint = atoi(optarg);
				break;
			case 'R':
				rcvbufsize = atoi(optarg);
				break;
			case 'S':
				sndbufsize = atoi(optarg);
				break;
			case 'T':
				runtime = atoi(optarg);
				number_of_messages = 0;
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
		client      = 1;
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
			if (verbose) {
				intlen = sizeof(int);
				if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &myrcvbufsize, (socklen_t *)&intlen) < 0)
					perror("setsockopt: rcvbuf");
				else
					fprintf(stdout,"Receive buffer size: %d.\n", myrcvbufsize);
			}
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
		/* Explicit settings, because LKSCTP does not enable it by default */
		if (nodelay == 1) {
			if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&on, sizeof(on)) < 0)
				perror("setsockopt: nodelay");
		} else {
			if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&off, sizeof(off)) < 0)
				perror("setsockopt: nodelay");
		}
#endif
		if (fragpoint){
			if(setsockopt(fd,IPPROTO_SCTP, SCTP_MAXSEG, &fragpoint, sizeof(fragpoint))< 0)
				perror("setsockopt: SCTP_MAXSEG");
		}	
		if (sndbufsize)
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int)) < 0)
				perror("setsockopt: sndbuf");
		
		if (verbose) {
			intlen = sizeof(int);
			if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &mysndbufsize, (socklen_t *)&intlen) < 0)
				perror("setsockopt: sndbuf");
			else
				fprintf(stdout,"Send buffer size: %d.\n", mysndbufsize);
		}
		buffer = malloc(length);
		iov[0].iov_base = buffer;
		iov[0].iov_len  = length;

		gettimeofday(&start_time, NULL);
		if (verbose) {
			printf("Start sending %ld messages...", (long)number_of_messages);
			fflush(stdout);
		}
		
		msghdr.msg_name       = NULL;
		msghdr.msg_namelen    = 0;
		msghdr.msg_iov        = iov;
		msghdr.msg_iovlen     = 1;
		msghdr.msg_control    = NULL;
		msghdr.msg_controllen = 0;
		msghdr.msg_flags      = 0;
		
		i = 0;
		done = 0;
		
		if (runtime > 0) {
			signal(SIGALRM, stop_sender);
			alarm(runtime);
		}
			
		while (!done && ((number_of_messages == 0) || (i < number_of_messages))) {
			i++;
			if (very_verbose)
				printf("Sending message number %lu.\n", i);
			/*
			if (sendmsg(fd, &msghdr, 0) < 0)
				perror("sendmsg");
			*/
			
			if ((n = send(fd, buffer, length, 0)) < 0)
				perror("send");
			if (n != length)
				printf("send returned %d instead of %d.\n", n, length);
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
		       "Sending", i, length, seconds);
		throughput = (double)i * (double)length / seconds / 1024.0;
		fprintf(stdout, "Throughput was %f KB/sec.\n", throughput);
	}
	return 0;
}
