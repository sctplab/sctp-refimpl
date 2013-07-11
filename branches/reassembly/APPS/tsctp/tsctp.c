/*-
 * Copyright (c) 2005 - 2011 Michael Tuexen, tuexen@fh-muenster.de
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
#include <arpa/inet.h>
#include <netinet/sctp.h>
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
#include <errno.h>

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


char Usage[] =
"Usage: tsctp [options] [address]\n"
"Options:\n"
"        -a      set adaptation layer indication\n"
"        -A      chunk type to authenticate \n"
"        -D      turns Nagle off\n"
"        -f      fragmentation point\n"
"        -l      size of send/receive buffer\n"
"        -L      local address\n"
"        -n      number of messages sent (0 means infinite)/received\n"
"        -p      port number\n"
"        -R      socket recv buffer\n"
"        -S      socket send buffer\n"
"        -T      time to send messages\n"
"        -u      use unordered user messages\n"
"        -v      verbose\n"
"        -V      very verbose\n"
"        -4      IPv4 only\n"
"        -6      IPv6 only\n"
;

#define DEFAULT_LENGTH             1024
#define DEFAULT_NUMBER_OF_MESSAGES 1024
#define DEFAULT_PORT               5001
#define BUFFERSIZE                  (1<<16)
#define LINGERTIME                 1
#define MAX_LOCAL_ADDR             10

static int verbose, very_verbose;
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
	unsigned int first_length;
	int flags;
	socklen_t len;

	fd = *(int *) arg;
	free(arg);
	tid = pthread_self();
	pthread_detach(tid);

	buf = malloc(BUFFERSIZE);
	flags = 0;
	len = (socklen_t)0;
	n = sctp_recvmsg(fd, (void*)buf, BUFFERSIZE, NULL, &len, NULL, &flags);
	gettimeofday(&start_time, NULL);
	first_length = 0;
	while (n > 0) {
		recv_calls++;
		if (flags & MSG_NOTIFICATION) {
			notifications++;
		} else {
			sum += n;
			if (flags & MSG_EOR) {
				messages++;
				if (first_length == 0)
					first_length = sum;
			}
		}
		flags = 0;
		len = (socklen_t)0;
		n = sctp_recvmsg(fd, (void*)buf, BUFFERSIZE, NULL, &len, NULL, &flags);
	}
	if (n < 0)
		perror("sctp_recvmsg");
	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &diff_time);
	seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000.0;
	fprintf(stdout, "%u, %lu, %lu, %lu, %llu, %f, %f\n",
	        first_length, messages, recv_calls, notifications, sum, seconds, (double)first_length * (double)messages / seconds);
	fflush(stdout);
	close(fd);
	free(buf);
	return NULL;
}

int main(int argc, char **argv)
{
	int fd, *cfdptr;
	size_t intlen;
	char c, *buffer;
	socklen_t addr_len;
	union sock_union{
		struct sockaddr sa;
		struct sockaddr_in s4;
		struct sockaddr_in6 s6;
	} remote_addr;
	struct sockaddr_storage local_addr[MAX_LOCAL_ADDR];
	char *local_addr_ptr = (char*) local_addr;
	unsigned int nr_local_addr = 0;
	struct timeval start_time, now, diff_time;
	int length, client;
	short local_port, remote_port, port;
	double seconds;
	double throughput;
	const int on = 1;
	const int off = 0;
	int nodelay = 0;
	unsigned long i, number_of_messages;
	pthread_t tid;
	int rcvbufsize=0, sndbufsize=0, myrcvbufsize, mysndbufsize;
	struct linger linger;
	int fragpoint = 0;
	unsigned int runtime = 0;
	struct sctp_setadaptation ind = {0};
#ifdef SCTP_AUTH_CHUNK
	unsigned int number_of_chunks_to_auth = 0;
	unsigned int chunk_number;
	unsigned char chunk[256];
	struct sctp_authchunk sac;
#endif
	struct sctp_assoc_value av;
	int unordered = 0;
	int ipv4only = 0;
	int ipv6only = 0;

	length             = DEFAULT_LENGTH;
	number_of_messages = DEFAULT_NUMBER_OF_MESSAGES;
	port               = DEFAULT_PORT;
	verbose            = 0;
	very_verbose       = 0;

	memset((void *) &remote_addr, 0, sizeof(remote_addr));

#ifdef SCTP_AUTH_CHUNK
	while ((c = getopt(argc, argv, "a:A:Df:l:L:n:p:R:S:T:uvV46")) != -1)
#else
	while ((c = getopt(argc, argv, "a:Df:l:L:n:p:R:S:T:uvV46")) != -1)
#endif
		switch(c) {
			case 'a':
				ind.ssb_adaptation_ind = atoi(optarg);
				break;
#ifdef SCTP_AUTH_CHUNK
			case 'A':
				if (number_of_chunks_to_auth < 256) {
					chunk[number_of_chunks_to_auth++] = (unsigned char)atoi(optarg);
				}
				break;
#endif
			case 'D':
				nodelay = 1;
				break;
			case 'f':
				fragpoint = atoi(optarg);
				break;
			case 'l':
				length = atoi(optarg);
				break;
			case 'L':
				if (nr_local_addr < MAX_LOCAL_ADDR) {
					struct sockaddr_in *s4 = (struct sockaddr_in*) local_addr_ptr;
					struct sockaddr_in6 *s6 = (struct sockaddr_in6*) local_addr_ptr;

					if (inet_pton(AF_INET6, optarg, &s6->sin6_addr)) {
						s6->sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
						s6->sin6_len = sizeof(struct sockaddr_in6);
#endif
						local_addr_ptr += sizeof(struct sockaddr_in6);
						nr_local_addr++;
					} else {
						if (inet_pton(AF_INET, optarg, &s4->sin_addr)) {
							s4->sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
							s4->sin_len = sizeof(struct sockaddr_in);
#endif
							local_addr_ptr += sizeof(struct sockaddr_in);
							nr_local_addr++;
						} else {
							printf("Invalid address\n");
							fprintf(stderr, "%s", Usage);
							exit(1);
						}
					}
				}
				break;
			case 'n':
				number_of_messages = atoi(optarg);
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
			case 'T':
				runtime = atoi(optarg);
				number_of_messages = 0;
				break;
			case 'u':
				unordered = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'V':
				verbose = 1;
				very_verbose = 1;
				break;
			case '4':
				ipv4only = 1;
				if (ipv6only) {
					printf("IPv6 only already\n");
					exit(1);
				}
				break;
			case '6':
				ipv6only = 1;
				if (ipv4only) {
					printf("IPv4 only already\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "%s", Usage);
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

	if (nr_local_addr == 0) {
		memset((void *) local_addr, 0, sizeof(local_addr));
		if (ipv4only) {
			struct sockaddr_in *s4 = (struct sockaddr_in*) local_addr;
			s4->sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
			s4->sin_len         = sizeof(struct sockaddr_in);
#endif
			s4->sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			struct sockaddr_in6 *s6 = (struct sockaddr_in6*) local_addr;
			s6->sin6_family      = AF_INET6;
#ifdef HAVE_SIN_LEN
			s6->sin6_len         = sizeof(struct sockaddr_in6);
#endif
			s6->sin6_addr = in6addr_any;
		}
		nr_local_addr = 1;
	}

	local_addr_ptr = (char*) local_addr;
	for (i = 0; i < nr_local_addr; i++) {
		struct sockaddr_in *s4 = (struct sockaddr_in*) local_addr_ptr;
		struct sockaddr_in6 *s6 = (struct sockaddr_in6*) local_addr_ptr;

		if (s4->sin_family == AF_INET) {
			s4->sin_port = htons(local_port);
			local_addr_ptr += sizeof(struct sockaddr_in);
			if (ipv6only) {
				printf("Can't use IPv4 address when IPv6 only\n");
				exit(1);
			}
		} else if (s6->sin6_family == AF_INET6) {
			s6->sin6_port = htons(local_port);
			local_addr_ptr += sizeof(struct sockaddr_in6);
			if (ipv4only) {
				printf("Can't use IPv6 address when IPv4 only\n");
				exit(1);
			}
		}
	}

	if ((fd = socket((ipv4only ? AF_INET : AF_INET6), SOCK_STREAM, IPPROTO_SCTP)) < 0)
		perror("socket");

	if (!ipv4only) {
		if (ipv6only) {
			if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const void*)&on, (socklen_t)sizeof(on)) < 0)
				perror("ipv6only");
		} else {
			if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const void*)&off, (socklen_t)sizeof(off)) < 0)
				perror("ipv6only");
		}
	}

#ifdef SCTP_AUTH_CHUNK
	for (chunk_number = 0; chunk_number < number_of_chunks_to_auth; chunk_number++) {
		sac.sauth_chunk = chunk[chunk_number];
		if (setsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_CHUNK, &sac, (socklen_t)sizeof(struct sctp_authchunk)) < 0)
			perror("setsockopt");
	}
#endif
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_ADAPTATION_LAYER, (const void*)&ind, (socklen_t)sizeof(struct sctp_setadaptation)) < 0) {
		perror("setsockopt");
	}
	if (!client)
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, (socklen_t)sizeof(on));

	if (nr_local_addr > 0) {
		if (sctp_bindx(fd, (struct sockaddr *)local_addr, nr_local_addr, SCTP_BINDX_ADD_ADDR) != 0)
			perror("bind");
	}

	if (!client) {
		struct sctp_event_subscribe event;

		if (listen(fd, 100) < 0)
			perror("listen");
		if (rcvbufsize)
			if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbufsize, sizeof(int)) < 0)
				perror("setsockopt: rcvbuf");
		if (verbose) {
			intlen = sizeof(int);
			if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &myrcvbufsize, (socklen_t *)&intlen) < 0) {
				perror("setsockopt: rcvbuf");
			} else {
				fprintf(stdout,"Receive buffer size: %d.\n", myrcvbufsize);
			}
		}
		memset(&event, 0, sizeof(event));
		event.sctp_data_io_event=1;
		if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
			perror("set event failed");
		}

		while (1) {
			memset(&remote_addr, 0, sizeof(remote_addr));

			if (ipv4only) {
				addr_len = sizeof(struct sockaddr_in);
			} else {
				addr_len = sizeof(struct sockaddr_in6);
			}

			cfdptr = malloc(sizeof(int));
			if ((*cfdptr = accept(fd, (struct sockaddr *)&remote_addr, &addr_len)) < 0) {
				perror("accept");
				continue;
			}
			if (verbose) {
				char temp[INET6_ADDRSTRLEN];

				if (remote_addr.sa.sa_family == AF_INET) {
					fprintf(stdout,"Connection accepted from %s:%d\n", inet_ntop(AF_INET, &remote_addr.s4.sin_addr, temp, INET_ADDRSTRLEN), ntohs(remote_addr.s4.sin_port));
				} else {
					fprintf(stdout,"Connection accepted from %s:%d\n", inet_ntop(AF_INET6, &remote_addr.s6.sin6_addr, temp, INET6_ADDRSTRLEN), ntohs(remote_addr.s6.sin6_port));
				}
			}
			pthread_create(&tid, NULL, &handle_connection, (void *) cfdptr);
		}
		close(fd);
	} else {
		if (inet_pton(AF_INET6, argv[optind], &remote_addr.s6.sin6_addr)) {
			remote_addr.s6.sin6_family = AF_INET6;
#ifdef HAVE_SIN_LEN
			remote_addr.s6.sin6_len = sizeof(struct sockaddr_in6);
#endif
			remote_addr.s6.sin6_port = htons(remote_port);
			addr_len = sizeof(struct sockaddr_in6);

			if (ipv4only) {
				printf("Can't use IPv6 address when IPv4 only\n");
				exit(1);
			}
		} else {
			if (inet_pton(AF_INET, argv[optind], &remote_addr.s4.sin_addr))
			{
				remote_addr.s4.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
				remote_addr.s4.sin_len = sizeof(struct sockaddr_in);
#endif
				remote_addr.s4.sin_port = htons(remote_port);
				addr_len = sizeof(struct sockaddr_in);

				if (ipv6only) {
					printf("Can't use IPv4 address when IPv6 only\n");
					exit(1);
				}
			} else {
				printf("Invalid address\n");
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}

		if (fragpoint) {
			av.assoc_id = 0;
			av.assoc_value = fragpoint;
			if (setsockopt(fd, IPPROTO_SCTP, SCTP_MAXSEG, &av, sizeof(av)) < 0) {
				perror("setsockopt: SCTP_MAXSEG");
			}
		}

		if (connect(fd, (struct sockaddr *)&remote_addr, addr_len) < 0) {
			perror("connect");
		}

#ifdef SCTP_NODELAY
		/* Explicit settings, because LKSCTP does not enable it by default */
		if (nodelay == 1) {
			if (setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&on, sizeof(on)) < 0) {
				perror("setsockopt: nodelay");
			}
		} else {
			if (setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&off, sizeof(off)) < 0) {
				perror("setsockopt: nodelay");
			}
		}
#endif
		if (sndbufsize)
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbufsize, sizeof(int)) < 0) {
				perror("setsockopt: sndbuf");
			}

		if (verbose) {
			intlen = sizeof(int);
			if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &mysndbufsize, (socklen_t *)&intlen) < 0) {
				perror("setsockopt: sndbuf");
			} else {
				fprintf(stdout,"Send buffer size: %d.\n", mysndbufsize);
			}
		}
		buffer = malloc(length);
		memset(buffer, 'A', length);

		gettimeofday(&start_time, NULL);
		if (verbose) {
			printf("Start sending %ld messages...", (long)number_of_messages);
			fflush(stdout);
		}

		i = 0;
		done = 0;

		if (runtime > 0) {
			signal(SIGALRM, stop_sender);
			alarm(runtime);
		}

		while (!done && ((number_of_messages == 0) || (i < (number_of_messages - 1)))) {
			if (very_verbose) {
				printf("Sending message number %lu.\n", i);
			}
			if (sctp_sendmsg(fd, buffer, length, NULL, 0, 0, unordered?SCTP_UNORDERED:0, 0, 0, 0) < 0) {
				perror("sctp_sendmsg");
				break;
			}
			i++;
		}
		if (sctp_sendmsg(fd, buffer, length, NULL, 0, 0, unordered?SCTP_EOF|SCTP_UNORDERED:SCTP_EOF, 0, 0, 0) < 0) {
			perror("sctp_sendmsg");
		}
		i++;
		if (verbose)
			printf("done.\n");
		linger.l_onoff = 1;
		linger.l_linger = LINGERTIME;
		if (setsockopt(fd, SOL_SOCKET, SO_LINGER,(char*)&linger, sizeof(struct linger)) < 0) {
			perror("setsockopt");
		}
		close(fd);
		gettimeofday(&now, NULL);
		timersub(&now, &start_time, &diff_time);
		seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000;
		fprintf(stdout, "%s of %ld messages of length %u took %f seconds.\n",
		       "Sending", i, length, seconds);
		throughput = (double)i * (double)length / seconds;
		fprintf(stdout, "Throughput was %f Byte/sec.\n", throughput);
	}
	return 0;
}
