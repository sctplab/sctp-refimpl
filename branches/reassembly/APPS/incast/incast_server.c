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
int verbose=0;
int nap_time=0;
extern int no_cc_change;
void
process_a_child(int sd, struct sockaddr_in *sin, int use_sctp)
{
	int cnt, sz;
	int *p;
	char buffer[MAX_SINGLE_MSG];
	struct timespec tvs, tve;
	struct incast_msg_req inrec;
	int no_clock_s=1, no_clock_e=1, i;
	socklen_t optlen;
	int optval;
	ssize_t readin, sendout, tot_out=0;
	if (verbose) {
		if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tvs))
			no_clock_s = 1;
		else 
			no_clock_s = 0;
	}
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
	/* Find out how much we must send */
	errno = 0;
	readin = recv(sd, &inrec, sizeof(inrec), 0);
	if(readin != sizeof(inrec)) {
		if (readin) 
			printf("Did not get %ld bytes got:%ld err:%d\n",
			       (long int)sizeof(inrec), (long int)readin, errno);
		goto out;
	}
	cnt = ntohl(inrec.number_of_packets);
	sz = ntohl(inrec.size);
	/* How big must the socket buffer be? */
	optlen = sizeof(optval);
	optval = (cnt * sz) + 1;
	if(setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &optval, optlen) != 0){
		printf("Warning - could not grow sock buf to %d will block err:%d\n",
		       optval,
		       errno);
	}
	/* protect against bad buffer sizes */
	if (sz > MAX_SINGLE_MSG) {
		cnt = (optval / MAX_SINGLE_MSG) + 1;
		sz = MAX_SINGLE_MSG;
	}
	memset(buffer, 0, sizeof(buffer));
	p = (int *)&buffer;
	*p = 1;
	for(i=0; i<cnt; i++) {
		sendout = send(sd, buffer, sz, 0);
		if (sendout < sz) {
			printf("Error sending %d sz:%d sendout:%ld\n", 
			       errno, sz, (long int)sendout);	
			goto out;
		}
		tot_out += sendout;
		*p = *p + 1;
	}
	if (verbose) {
		if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tve))
			no_clock_e = 1;
		else 
			no_clock_e = 0;
		if ((no_clock_e == 0) && (no_clock_s == 0)) {
			timespecsub(&tve, &tvs);
			printf("%d rec of %d in %ld.%9.9ld (tot_out:%ld)\n",
			       cnt, sz, (long int)tve.tv_sec, 
			       tve.tv_nsec, (long int)tot_out);
		}
	} else if (tot_out < (cnt * sz)) {
		printf("--tot_out was %ld but cnt:%d * sz:%d == %d\n",
		       (long int)tot_out, cnt, sz, (cnt * sz));
	}
out:
	if (nap_time) {
		struct timespec nap;
		nap.tv_sec = 0;
		nap.tv_nsec = nap_time;
		nanosleep(&nap, NULL);
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

void byebye()
{
	printf("All done?? errno:%d\n", errno);
}

int
main(int argc, char **argv)
{
	struct sockaddr_in lsin, bsin;
	pthread_t *thread_list;
	int thrd_cnt=3;
	int i, temp;
	uint16_t port = htons(DEFAULT_SVR_PORT);
	int backlog=4;
	socklen_t slen;
	char *bindto = NULL;
	while ((i = getopt(argc, argv, "S:B:b:tsp:?vT:N")) != EOF) {
		switch (i) {
		case 'N':
			no_cc_change = 1;
			break;
		case 'S':
			nap_time = strtol(optarg, NULL, 0);
			if (nap_time < 0) 
				nap_time = 0;
			if (nap_time >= 1000000000) {
				/* 1 ns shy of 1 sec */
				nap_time = 999999999;
			}

			break;
		case 'T':
			thrd_cnt = strtol(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
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
			printf("Use %s -b bind_address[-p port -t -s -B backlog -N]\n", 
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
	atexit(byebye);
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

