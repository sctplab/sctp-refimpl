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

#define DEFAULT_SVR_PORT 9902       /* Where mice/rats are */
#define DEFAULT_ELEPHANT_PORT 10902 /* Where elephants are */

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
	char *peer_name;
	int long_size;
	int exclude;
};

/* 32bit:24 bytes 64bit:40 bytes */
struct incast_peer_outrec {
	struct timespec start;
	struct timespec end;
	int peerno;
	int state;
};

/* 32bit:16 bytes 64bit:24 bytes */
struct peer_record {
	struct timespec timeof;
	int byte_cnt;
	int msg_cnt;
};

LIST_HEAD(incast_list, incast_peer);

struct incast_control {
	char *file_to_store_results;
	struct sockaddr_in bind_addr;
	struct incast_list master_list;
	char *hostname;
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
	int long_size;
};

/* 32bit:40 bytes 64bit:72 bytes */
struct incast_lead_hdr {
	struct timespec start;
	struct timespec connected;
	struct timespec sending;
	struct timespec end;
	int number_servers;
	int passcnt;
};

/* 32bit:24 bytes 64bit:40  */
struct elephant_lead_hdr {
	struct timespec start;
	struct timespec end;
	uint32_t number_of_bytes;
	int number_servers;
};

/* 32bit:52 bytes 64bit:88 bytes */
struct elephant_sink_rec {
	struct sockaddr_in from;
	struct timespec start;
	struct timespec end;
	struct timespec mono_start;
	struct timespec mono_end;
	int number_bytes;
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

#ifndef timespecadd
#define timespecadd(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec += (uvp)->tv_sec;				\
		(vvp)->tv_nsec += (uvp)->tv_nsec;			\
		if ((vvp)->tv_nsec >= 1000000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_nsec -= 1000000000;			\
		}							\
	} while (0)
#endif

int translate_ip_address(char *host, struct sockaddr_in *sa);
void incast_run_clients(struct incast_control *ctrl);
void  elephant_run_clients(struct incast_control *ctrl, int first_size);
void print_an_address(struct sockaddr *a, int cr);
void parse_config_file(struct incast_control *ctrl, char *configfile,
	uint16_t def_port);

/* Magic read utilities */
int read_ele_hdr(struct elephant_lead_hdr *hdr, FILE *io, int mac_long_size);
int read_peer_rec(struct incast_peer_outrec *rec, FILE *io, int mac_long_size);
int read_incast_hdr(struct incast_lead_hdr *hdr, FILE *io, int mac_long_size);
int  read_a_sink_rec(struct elephant_sink_rec *sink, FILE *io, int infile_size);

#endif
