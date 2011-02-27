/*-
 * Copyright (c) 2011, by Randall Stewart.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of the authors nor the names of its 
 *    contributors may be used to endorse or promote products derived 
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <incast_fmt.h>
#include <pthread.h>

char *outfile=NULL;
extern int no_cc_change;

void
process_a_child(int sd, struct sockaddr_in *sin, int use_sctp)
{
	double bw, timeof;
	uint64_t cnt, sz;
	int ret;
	char buffer[MAX_SINGLE_MSG];
	struct timespec tvs, tve;
	socklen_t optlen;
	int optval;
	struct elephant_sink_rec sink;

	memset(&sink, 0, sizeof(sink));
	optval = 1;
	optlen = sizeof(optval);
	if (use_sctp) {
		if(setsockopt(sd, IPPROTO_SCTP, SCTP_NODELAY, &optval, optlen)) {
			printf("Warning - can't turn on nodelay for sctp err:%d\n",
			       errno);
		}
	} else {
		if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &optval, optlen)) {
			printf("Warning - can't turn on nodelay for tcp err:%d\n",
			       errno);
		}
	}
	if (clock_gettime(CLOCK_REALTIME_PRECISE, &sink.start)) {
		printf("Can't get clock, errno:%d\n", errno);
		exit(-1);
	}
	if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tvs)) {
		printf("Can't get clock err:%d\n", errno);
		exit(-1);
	}
	/* Read data until there is no more */
	cnt = sz = 0;
	do {
		ret = recv(sd, buffer, MAX_SINGLE_MSG, 0);
		if (ret > 0) {
			cnt++;
			sz += ret;
		}
	} while (ret > 0);

	if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tve)) {
		printf("Can't get clock err:%d\n", errno);
		exit(-1);
	}
	if (clock_gettime(CLOCK_REALTIME_PRECISE, &sink.end)) {
		printf("Can't get clock, errno:%d\n", errno);
		exit(-1);
	}
	sink.mono_start = tvs;
	sink.mono_end = tve;
	timespecsub(&tve, &tvs);
	if (outfile == NULL) {
		timeof = (tve.tv_sec * 1.0) + (1.0 / (tve.tv_nsec * 1.0));
		bw = (sz * 1.0)/timeof;
		printf("%ld:%ld.%9.9ld:%f:",
		       (long int)sz,
		       (long int)tve.tv_sec, tve.tv_nsec, bw);
		print_an_address((struct sockaddr *)sin, 1);
	} else {
		/* save result */
		FILE *io;
		sink.from = *sin;
		sink.number_bytes = sz;
		io = fopen(outfile, "a+");
		if (io == NULL) {
			printf("Can't open outfile '%s' for append err:%d\n",
			       outfile, errno);
			exit(-1);
		}
		if (fwrite(&sink, sizeof(sink), 1, io) != 1) {
			printf("Can't write outfile '%s' err:%d\n",
			       outfile, errno);
			exit(-1);
		}
		fclose(io);
	}
	close(sd);
}

int use_sctp=0;
int sd;
void *
execute_forever(void *notused)
{
	struct sockaddr_in sin;
	socklen_t slen;
	int nsd;
	while(1) {
		slen = sizeof(sin);
		nsd = accept(sd, (struct sockaddr *)&sin, &slen);
		process_a_child(nsd, &sin, use_sctp);
	}
}

int
main(int argc, char **argv)
{
	struct sockaddr_in lsin, bsin;
	pthread_t *thread_list;
	int thrd_cnt=2;
	int i, temp;
	uint16_t port = htons(DEFAULT_ELEPHANT_PORT);
	int backlog=4;
	socklen_t slen;
	char *bindto = NULL;
	while ((i = getopt(argc, argv, "B:b:tsp:?T:w:N")) != EOF) {
		switch (i) {
		case 'N':
			no_cc_change = 1;
			break;
		case 'w':
			outfile = optarg;
			break;
		case 'T':
			thrd_cnt = strtol(optarg, NULL, 0);
			break;
		case 'B':
			backlog = strtol(optarg, NULL, 0);
			if (backlog < 1) {
				printf("Sorry backlog must be 1 or more - using default\n");
				backlog = 4;
			}
			break;
		case 'b':
			bindto = optarg;
			break;
		case 'p':
			temp = strtol(optarg, NULL, 0);
			if ((temp < 1) || (temp > 65535)) {
				printf("Error port %d invalid - using default\n",
					temp);
			} else {
				port = htons((uint16_t)temp);
			}
			break;
		case 's':
			use_sctp = 1;
			break;
		case 't':
			use_sctp = 0;
			break;
		default:
		case '?':
		use:
			printf("Use %s -b bind_address[-p port -t -s -B backlog -N ]\n", 
			       argv[0]);
			exit(-1);
			break;
		};
	}
	/* Did they forget the bind address? */
	if (bindto == NULL) {
		goto use;
	}
	/* setup bind address */
	memset(&lsin, 0, sizeof(lsin));
	if(translate_ip_address(bindto, &lsin)) {
		printf("bind to address is invalid - can't translate '%s'\n",
		       bindto);
		exit(-1);
	}
	lsin.sin_port = port;

	/* Which protocol? */
	if (use_sctp) {
		sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
		if ((sd >= 0) && (no_cc_change == 0)) {
			struct sctp_assoc_value av;
			socklen_t optlen;
			av.assoc_id = 0;
			av.assoc_value = SCTP_CC_RTCC;
			optlen = sizeof(av);
			printf("Setting on RTCC congestion control\n");
			if (setsockopt(sd, IPPROTO_SCTP,  SCTP_PLUGGABLE_CC,
			       &av, optlen) == -1) {
				printf("Can't turn on RTCC cc err:%d\n",
				       errno);
				exit(-1);
			}
		}
	} else {
		sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

	/* Now lets bind it if we can */
	slen = sizeof(lsin);
	if (bind(sd, (struct sockaddr *)&lsin, slen)) {
		printf("Bind fails errno:%d\n", errno);
		close(sd);
		exit(-1);
	}

	/* Validate that we got our address */
	slen = sizeof(bsin);
	if (getsockname(sd, (struct sockaddr *)&bsin, &slen)) {
		printf("Get socket name fails errno:%d\n", errno);
		close(sd);
		exit(-1);
	}
	if (bsin.sin_port != lsin.sin_port) {
		printf("Bound port is incorrect bound:%d wanted:%d\n",
		       ntohs(bsin.sin_port), ntohs(lsin.sin_port));
		close(sd);
		exit(-1);
	}

	/* now lets listen */
	if (listen(sd, backlog)) {
		printf("Listen fails err:%d\n", errno);
		close(sd);
		exit(-1);
	}
	/* Loop forever processing connections */
	if (thrd_cnt) {
		thread_list = malloc((sizeof(pthread_t) * thrd_cnt));
		if (thread_list == NULL) {
			printf("Can't malloc %d pthread_t array\n",
			       thrd_cnt);
			exit(0);
		}
	}
	for (i=0; i<thrd_cnt; i++) {
		if (pthread_create(&thread_list[i], NULL, 
				   execute_forever, (void *)NULL)) {
			printf("Can't start thread %d\n", i);
			exit(0);
		}
	}
	execute_forever(NULL);
	return (0);
}

