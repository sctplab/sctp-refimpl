#ifndef __incast_fmt_h__
#define __incast_fmt_h__
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <unistd.h>
#include <stdlib.h>
#include <net/if.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/queue.h>

#define MAX_SINGLE_MSG 9000 /* Largest 1 mtu normally 1448*/
#define DEFAULT_MSG_SIZE 1448 /* TCP header and TS and IP 
hdr - 1500 */
#define DEFAULT_NUMBER_SENDS 3 /* default to TCP IW */

struct incast_msg_req {
	int number_of_packets;
	int size;
};

#define DEFAULT_SVR_PORT 9902

#define SRV_STATE_NEW       1
#define SRV_STATE_REQ_SENT  2
#define SRV_STATE_READING   3
#define SRV_STATE_COMPLETE  4
#define SRV_STATE_ERROR     5

struct incast_peer {
	LIST_ENTRY(incast_peer) next; /* Next list element */
	int sd;
	int state;
	struct timespec start; /* first read/kqueue wake */
	struct timespec end;   /* When last byte read */
	struct sockaddr_in addr;
	int msg_cnt; /* Cnt rcvd */
	int byte_cnt; /* byte cnt rcvd */
};

LIST_HEAD(incast_list, incast_peer);

struct incast_control {
	struct sockaddr_in bind_addr;
	struct incast_list master_list;
	int number_server;
	int sctp_on;
	int cnt_of_times;  /* number of passes */
	int decrement_amm; /* pass count decrement */
	int size;    /* size of each send */
	int cnt_req; /* number of size ele requested */
	int byte_cnt_req; /* how much total */
	int completed_server_cnt; /* How many servers are done with pass */
	int verbose;
	int nap_time;
};

#ifndef timespecsub
#define timespecsub(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec -= (uvp)->tv_sec;				\
		(vvp)->tv_nsec -= (uvp)->tv_nsec;			\
		if ((vvp)->tv_nsec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_nsec += 1000000000;			\
		}							\
	} while (0)
#endif

int translate_ip_address(char *host, struct sockaddr_in *sa);
void incast_run_clients(struct incast_control *ctrl);
void print_an_address(struct sockaddr *a, int cr);
#endif
