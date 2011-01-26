#include <incast_fmt.h>
#include <pthread.h>

void
process_a_child(int sd, struct sockaddr_in *sin, int use_sctp)
{
	uint64_t cnt, sz;
	int ret;
	char buffer[MAX_SINGLE_MSG];
	struct timespec tvs, tve;
	int no_clock_s=1, no_clock_e=1;
	socklen_t optlen;
	int optval;
	if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tvs))
		no_clock_s = 1;
	else 
		no_clock_s = 0;
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
	/* Read data until there is no more */
	cnt = sz = 0;
	do {
		ret = recv(sd, buffer, MAX_SINGLE_MSG, 0);
		if (ret > 0) {
			cnt++;
			sz += ret;
		}
	} while (ret > 0);

	if(clock_gettime(CLOCK_MONOTONIC_PRECISE, &tve))
		no_clock_e = 1;
	else 
		no_clock_e = 0;
	if ((no_clock_e == 0) && (no_clock_s == 0)) {
		timespecsub(&tve, &tvs);
		double bw, timeof;
		timeof = (tve.tv_sec * 1.0) + (1.0 / (tve.tv_nsec * 1.0));
		bw = (sz * 1.0)/timeof;
		printf("%ld:%ld.%9.9ld:%f:",
		       (long int)sz,
		       (long int)tve.tv_sec, tve.tv_nsec, bw);
		print_an_address((struct sockaddr *)sin, 1);
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
	while ((i = getopt(argc, argv, "B:b:tsp:?T:")) != EOF) {
		switch (i) {
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
			printf("Use %s -b bind_address[-p port -t -s -B backlog]\n", 
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

