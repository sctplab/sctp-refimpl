/*-
 * Copyright (c) 2007, by Cisco Systems, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its 
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef __FreeBSD__
#include <getopt.h>
#endif

#include <netinet/sctp.h>

#define SCTP_STRING_BUF_SZ 256

static void
SCTPPrintAnAddress(struct sockaddr *a)
{
	char stringToPrint[SCTP_STRING_BUF_SZ];
	u_short prt;
	char *srcaddr, *txt;

	if (a == NULL) {
		printf("NULL\n");
		return;
	}
	if (a->sa_family == AF_INET) {
		srcaddr = (char *)&((struct sockaddr_in *)a)->sin_addr;
		txt = "IPv4 Address: ";
		prt = ntohs(((struct sockaddr_in *)a)->sin_port);
	} else if (a->sa_family == AF_INET6) {
		srcaddr = (char *)&((struct sockaddr_in6 *)a)->sin6_addr;
		prt = ntohs(((struct sockaddr_in6 *)a)->sin6_port);
		txt = "IPv6 Address: ";
	} else {
		return;
	}
	if (inet_ntop(a->sa_family, srcaddr, stringToPrint, sizeof(stringToPrint))) {
		if (a->sa_family == AF_INET6) {
			printf("%s%s:%d scope:%d\n",
			    txt, stringToPrint, prt,
			    ((struct sockaddr_in6 *)a)->sin6_scope_id);
		} else {
			printf("%s%s:%d\n", txt, stringToPrint, prt);
		}

	} else {
		printf("%s unprintable?\n", txt);
	}
}


void
useage(char *who)
{
	printf("Usage: %s [ options ]\n", who);
	printf("Current options:\n");
	printf("-D time         - Delay time in seconds between new sockets at assoc failures/terms (def = 1s)\n");
	printf("-m myport       - Set my port to bind to (defaults to 2222)\n");
	printf("-B ip-address   - Bind the specified address (else bindall)\n");
	printf("-s size         - send data at start of an assoc size is data write size (def none)\n");
	printf("-c cnt          - count of data to send at start (default 10)\n");
	printf("-4              - bind v4 socket (default)\n");
	printf("-6              - bind v6 socket\n");
	printf("-l              - don't connect, listen only (default)\n");
	printf("-h host-address - address of host to connect to (required if no -l)\n");
	printf("-p port         - of host to connect to (defaults to 2222)\n");
	printf("-L              - send LOOP requests\n");
	printf("-v              - Verbose please\n");
	printf("-q              - Quiet mode, no prints or stats\n");
	printf("-S              - send SIMPLE data (default)\n");
	printf("-Z              - No heatbeat delay\n");
	printf("-I strms        - Number of allowed in-streams (default 13)\n");
	printf("-O strms        - Number of requesed out-streams (default 13)\n");
	printf("-H or -?        - help\n");
	printf("-f level        - set fragment interleave level (0, 1 or 2) (default 0)\n");
	printf("-P pdapi_point  - set partial delivery point (0 = system default)\n");
}

int no_hb_delay = 0;
int quiet_mode = 0;
union sctp_sockstore addr;
union sctp_sockstore from;
union sctp_sockstore bindto;
int pdapi_point = 0;
int verbose = 0;
int listen_only = 1;
uint16_t remote_port, local_port;
int size_to_send = 0;
int cnt_to_send = 10;
int use_v6=1;
int send_loop_request = 0;
char *conn_string = NULL;
char *bind_string = NULL;
uint16_t strm_in=13;
uint16_t strm_out=13;
uint32_t msg_in_cnt=0;
uint32_t msg_in_cnt_weor=0;
uint32_t notify_in_cnt=0;
uint32_t sends_out = 0;
uint32_t sends_ping_resp = 0;
int fragment_interleave = 0;

unsigned int mydelay;
struct sctp_sndrcvinfo sinfo_in, sinfo_out;

/* Max message sizes */
#define RECV_BUF_SIZE 65535
#define SEND_BUF_SIZE 65535

typedef struct {
    u_char  type;
    u_char  padding;
    u_short dgramLen;
    u_long  dgramID;
} testDgram_t;

#define SCTP_TEST_LOOPREQ   1
#define SCTP_TEST_LOOPRESP  2
#define SCTP_TEST_SIMPLE    3
uint32_t dgram_id_counter = 0;

void
print_stats()
{
	printf("Rcvd data:%d Sent:%d ping resp,  Sent:%d unsol, Notify's:%d msg_w_weor:%d\n",
	       msg_in_cnt, sends_ping_resp, sends_out, notify_in_cnt, msg_in_cnt_weor);
}

void
handle_notification(char *receive_buffer, int *notDone)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
	struct sctp_authkey_event *auth;
	struct sctp_stream_reset_event *strrst;
	int asocDown;
	char *str = NULL;
	char buf[256];
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	sctp_assoc_t id = 0;

	asocDown = 0;

	snp = (union sctp_notification *)receive_buffer;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {

		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			id = sac->sac_assoc_id;
			asocDown = 1;
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			id = sac->sac_assoc_id;
			asocDown = 1;
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CANT START ASSOC";
			id = sac->sac_assoc_id;
			asocDown = 1;
			break;
		default:
			str = "UNKNOWN";
		} /* end switch(sac->sac_state) */
		if(verbose) {
			printf("SCTP_ASSOC_CHANGE: %s, sac_error=0x%x assoc=0x%x\n",
			       str,
			       (uint32_t)sac->sac_error,
			       (uint32_t)sac->sac_assoc_id);
		}
		break;
	case SCTP_PEER_ADDR_CHANGE:
		spc = &snp->sn_paddr_change;
		switch(spc->spc_state) {
		case SCTP_ADDR_AVAILABLE:
			str = "ADDRESS AVAILABLE";
			break;
		case SCTP_ADDR_UNREACHABLE:
			str = "ADDRESS UNAVAILABLE";
			break;
		case SCTP_ADDR_REMOVED:
			str = "ADDRESS REMOVED";
			break;
		case SCTP_ADDR_ADDED:
			str = "ADDRESS ADDED";
			break;
		case SCTP_ADDR_MADE_PRIM:
			str = "ADDRESS MADE PRIMARY";
			break;
		case SCTP_ADDR_CONFIRMED:
			str = "ADDRESS CONFIRMED";
			break;
		default:
			str = "UNKNOWN";
		} /* end switch */
		sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
		if (sin6->sin6_family == AF_INET6) {
			char scope_str[16];
			snprintf(scope_str, sizeof(scope_str)-1, " scope %u",
				 sin6->sin6_scope_id);
			(void)inet_ntop(AF_INET6, (char*)&sin6->sin6_addr, buf, sizeof(buf));
			strncat(buf, scope_str, sizeof(scope_str));
		} else {
			sin = (struct sockaddr_in *)&spc->spc_aaddr;
			(void)inet_ntop(AF_INET, (char*)&sin->sin_addr, buf, sizeof(buf));
		}
		if(verbose) {
			printf("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=0x%x\n",
			       str, buf, (uint32_t)spc->spc_assoc_id);
		}
		break;
	case SCTP_REMOTE_ERROR:
		sre = &snp->sn_remote_error;
		if(verbose) {
			printf("SCTP_REMOTE_ERROR: assoc=0x%x\n",
			       (uint32_t)sre->sre_assoc_id);
		}
		break;

	case SCTP_AUTHENTICATION_EVENT:
	{
		auth = (struct sctp_authkey_event *)&snp->sn_auth_event;
		if(verbose) {
			printf("SCTP_AUTHKEY_EVENT: assoc=0x%x - ",
			       (uint32_t)auth->auth_assoc_id);
		}
		switch(auth->auth_indication) {
		case SCTP_AUTH_NEWKEY:
			if(verbose) {
				printf("AUTH_NEWKEY");
			}
			break;
		default:
			if(verbose) {
				printf("Indication 0x%x", auth->auth_indication);
			}
		}
		if(verbose) {
			printf(" key %u, alt_key %u\n", auth->auth_keynumber,
			       auth->auth_altkeynumber);
		}
		break;
	}
	case SCTP_STREAM_RESET_EVENT:
	{
		int len;
		char *strscope="unknown";
		strrst = (struct sctp_stream_reset_event *)&snp->sn_strreset_event;
		if(verbose) {
			printf("SCTP_STREAM_RESET_EVENT: assoc=0x%x\n",
			       (uint32_t)strrst->strreset_assoc_id);
		}
		if (strrst->strreset_flags & SCTP_STRRESET_INBOUND_STR) {
			strscope = "inbound";
		} else if (strrst->strreset_flags & SCTP_STRRESET_OUTBOUND_STR) {
			strscope = "outbound";
		}
		if(strrst->strreset_flags & SCTP_STRRESET_ALL_STREAMS) {
			if(verbose) {
				printf("All %s streams have been reset\n",
				       strscope);
			}
		} else {
			int i,cnt=0;
			len = ((strrst->strreset_length - sizeof(struct sctp_stream_reset_event))/sizeof(uint16_t));
			if(verbose) {
				printf("Streams ");
				for ( i=0; i<len; i++){
					cnt++;
					printf("%d",strrst->strreset_list[i]);
					if ((cnt % 16) == 0) {
						printf("\n");
					} else {
						printf(",");
					}
				}
				if((cnt % 16) == 0) {
					/* just put out a cr */
					printf("Have been reset %s\n",strscope);
				} else {
					printf(" have been reset %s\n",strscope);
				}
			}
		}
		
	}
	break;
	case SCTP_SEND_FAILED:
	{
		char *msg;
		static char msgbuf[200];
		ssf = &snp->sn_send_failed;
		if(ssf->ssf_flags == SCTP_DATA_UNSENT)
			msg = "data unsent";
		else if(ssf->ssf_flags == SCTP_DATA_SENT)
			msg = "data sent";
		else{
			snprintf(msgbuf, sizeof(msgbuf), "unknown flags:%d", ssf->ssf_flags);
			msg = msgbuf;
		}
		if(verbose) {
			printf("SCTP_SEND_FAILED: assoc=0x%x flag indicate:%s\n",
			       (uint32_t)ssf->ssf_assoc_id,msg);
		}

	}

	break;
	case SCTP_ADAPTION_INDICATION:
	{
		struct sctp_adaption_event *ae;
		ae = &snp->sn_adaption_event;
		if(verbose) {
			printf("SCTP_ADAPTION_INDICATION: assoc=0x%x - indication:0x%x\n",
			       (uint32_t)ae->sai_assoc_id, (uint32_t)ae->sai_adaption_ind);
		}
	}
	break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	{
		struct sctp_pdapi_event *pdapi;
		pdapi = &snp->sn_pdapi_event;
		if(verbose) {
			printf("SCTP_PD-API event:%u\n",
			       pdapi->pdapi_indication);
		}
	}
	break;

	case SCTP_SHUTDOWN_EVENT:
                sse = &snp->sn_shutdown_event;
		if(verbose) {
			printf("SCTP_SHUTDOWN_EVENT: assoc=0x%x\n",
			       (uint32_t)sse->sse_assoc_id);
		}
		str = "SHUTDOWN RCVD";
		id = sse->sse_assoc_id;
		asocDown = 1;
		break;
	default:
		if(verbose) {
			printf("Unknown notification event type=0x%x\n", 
			       snp->sn_header.sn_type);
		}
	} /* end switch(snp->sn_header.sn_type) */
	if (asocDown) {
		if(quiet_mode == 0) {
			print_stats();
			printf("Associd:0x%x Terminates (reason=%s)\n", 
			       id, ((str == NULL) ? "None" : str));
		}
		*notDone = 0;
	}
}

int
setup_a_socket()
{
	int sd;
	socklen_t siz, bindlen;
	int on_off;
	struct sctp_rtoinfo rto;
	struct sctp_initmsg init;
	struct linger linger;
	struct sctp_event_subscribe events;
	if(use_v6) {
		sd  = socket(AF_INET6,SOCK_SEQPACKET, IPPROTO_SCTP);
		bindlen = sizeof(struct sockaddr_in6);
	} else {
		sd  = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		bindlen = sizeof(struct sockaddr_in);
	}
	if (sd == -1) {
		printf("Can't open socket error:%d - exiting\n", errno);
		exit (-1);
	}
	memset(&rto, 0, sizeof(rto));
	siz = sizeof(rto);
	rto.srto_min = 1000;
        rto.srto_max = 5000;
        rto.srto_initial = 1000;
	if (no_hb_delay) {
		struct sctp_paddrparams param;
		socklen_t len;
		int result;
		memset(&param, 0, sizeof(param));
		param.spp_flags = SPP_HB_ENABLE | SPP_HB_TIME_IS_ZERO;
		len = sizeof(param);
		result = setsockopt(sd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
				    &param, len);
		if(result != 0) {
			printf("Can't turn HB delay to zero error:%d -exiting\n", errno);
			exit (-1);
		}
	}
	if(setsockopt(sd, IPPROTO_SCTP, SCTP_RTOINFO, &rto, siz) != 0) {
		printf("Can't set RTO information error:%d - exiting\n", errno);
		exit (-1);
	}
	if (pdapi_point) {
		siz = sizeof(pdapi_point);
		if(setsockopt(sd, IPPROTO_SCTP, SCTP_PARTIAL_DELIVERY_POINT, &pdapi_point, siz) != 0) {
			printf("Can't set PDAPI-Point error:%d - exiting\n", errno);
			exit (-1);
		}
	}
	siz = sizeof(fragment_interleave);
	printf("Set fragment interleave level %d\n", fragment_interleave);
	if(setsockopt(sd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE, &fragment_interleave, siz) != 0) {
		printf("Can't set fragment interleave error:%d - exiting\n", errno);
		exit (-1);
	}
	on_off = 1;
	siz = sizeof(on_off);
	/* set no mydelay */
	if(setsockopt(sd,IPPROTO_SCTP, SCTP_NODELAY, &on_off, siz) != 0) {
		printf("Can't set SCTP_NODELAY to on error:%d - exiting\n", errno);
		exit (-1);
	}
	linger.l_onoff = 1;
	linger.l_linger = 0;
	if (setsockopt(sd, SOL_SOCKET, SO_LINGER,(char*)&linger, sizeof(struct linger))<0) {
		printf("Warning:Can't set SOL_SOCKET-SO_LINGER error:%d - tests may hang\n", errno);
	}
	memset(&init, 0, sizeof(init));
	init.sinit_num_ostreams = strm_out;
	init.sinit_max_instreams = strm_in;
	init.sinit_max_attempts = 8;
	init.sinit_max_init_timeo = 1001;
	siz = sizeof(init);
	if(setsockopt(sd,IPPROTO_SCTP, SCTP_INITMSG, &init, siz) != 0) {
		printf("Can't set SCTP_INIT to on error:%d - exiting\n", errno);
		exit (-1);
	}
	memset(&events, 0, sizeof(events));
	events.sctp_data_io_event = 1;
	events.sctp_association_event = 1;
	events.sctp_address_event = 1;
	events.sctp_shutdown_event = 1;
	events.sctp_adaptation_layer_event = 1;
	events.sctp_stream_reset_event = 1;
	siz = sizeof(events);
	if(setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &events, siz) != 0) {
		printf("Can't set SCTP_EVENTS to on error:%d - exiting\n", errno);
		exit (-1);
	}
	if(verbose) {
		printf("Attempt to bind:");
		SCTPPrintAnAddress(&bindto.sa);
		printf("\n");
	}
	if(bind(sd, (struct sockaddr *)&bindto, bindlen) < 0){
		printf("bind failed error:%d - exiting\n",errno);
		exit(-1);
	}
	if (listen(sd, 10) < 0) {
		printf("listen failed error:%d - exiting\n",
		       errno);
		exit(-1);
	}
	if (verbose) {
		printf("return socket %d\n", sd);
	}
	return (sd);
}

int do_not_respond = 0;

struct msg_of {
	struct msg_of *next;
	sctp_assoc_t id;
	uint16_t notification;
	uint16_t done;
	int stream;
	int length;
	char *buffer;
};

struct msg_of *base=NULL;

struct msg_of *
find_the_msg(struct sctp_sndrcvinfo *sinfo, int flags)
{
	struct msg_of *at;
	if(verbose) {
		printf("Looking for assoc_id:%x stream:%d\n",
		       sinfo->sinfo_assoc_id, sinfo->sinfo_stream);
	}
	at = base;
	while(at) {
		if (flags & MSG_NOTIFICATION) {
			if ((at->id == sinfo->sinfo_assoc_id) &&
			    (at->notification)) {
				if(verbose) {
					printf("Found notification\n");
				}
				return (at);
			}
		} else {
			if ((at->id == sinfo->sinfo_assoc_id) &&
			    (at->stream == sinfo->sinfo_stream) &&
				(at->notification == 0)) {
				if(verbose) {
					printf("Found the message\n");
				}
				return (at);
			}
		}
		at = at->next;
	}	
	return(NULL);
}


char *
sctp_msg_recv(char *rbuf, int *in_len, struct sctp_sndrcvinfo *sinfo_in,  int flags)
{
	/* Need to assemble the incoming messages into
	 * complete messages. assoc_id and stream number are
	 * the keys to uniqueness. For NOTIFICATIONS,
	 * in theory we should only get
	 * one of these at a time, but we write the code to
	 * handle more than one at a time. For these cases (notifications)
	 * we load them all as stream 0, so we can use the same logic
	 * for getting them.
	 */
	struct msg_of *it;
	int len;

	len = *in_len;
	it = find_the_msg(sinfo_in, flags);
	if(it == NULL) {
		it = (struct msg_of *)malloc(sizeof(struct msg_of));
                if (it == NULL) {
                    return (NULL);
                }
		memset(it, 0, sizeof(struct msg_of));
		if (flags & MSG_NOTIFICATION) {
			it->notification = 1;
		} else {
			it->notification = 0;
		}
		if(verbose) {
			printf("New msg stream:%d for assoc:%x notification:%d\n", sinfo_in->sinfo_stream,
			       sinfo_in->sinfo_assoc_id, it->notification);
		}
		it->id = sinfo_in->sinfo_assoc_id;
		it->stream = sinfo_in->sinfo_stream;
		it->length = len;
		it->buffer = malloc(len);
		memcpy(it->buffer, rbuf, len);
		it->next = base;
		base = it;
	} else {
		char *temp;
		if(verbose) {
			printf("Found length is %d new len is %d\n",
			       it->length, len);
		}
		temp = malloc(len + it->length);
		memcpy(temp, it->buffer, it->length);
		memcpy(&temp[it->length], rbuf, len);
		free(it->buffer);
		it->buffer = temp;
		it->length += len;
		if(verbose) {
			printf("New buffer is %d bytes\n", it->length);
		}
	}
	if(flags & MSG_EOR) {
		it->done = 1;
	}
	if (it->done) {
		if(verbose) {
			printf("msg complete now\n");
		}
		*in_len = it->length;
		return(it->buffer);

	} else {
		return (NULL);
	}
}


void
sctp_msg_cleanup()
{
	struct msg_of *it;
	if(base->done) {
		it = base;
		base = it->next;
		free(it->buffer);
		free(it);
	}
}


void
handle_read_event(int *notDone, int sd)
{
	testDgram_t *rcv;
	char receive_buffer[RECV_BUF_SIZE], *buf;
	int ret, respmsg;
	socklen_t flen;
	int msg_flags=0;

	flen = sizeof(from);
	ret = sctp_recvmsg(sd, receive_buffer, sizeof(receive_buffer),
			   &from.sa, &flen, &sinfo_in, &msg_flags);
	if (ret < 0) {
		print_stats();
		if(quiet_mode == 0) {
			printf("Got error on sctp_recvmsg:%d - next socket\n", errno);
		}
		*notDone=0;
		return;
	}
	if (msg_flags & MSG_EOR)
		msg_in_cnt_weor++;

	buf = sctp_msg_recv(receive_buffer, &ret, &sinfo_in,  msg_flags);
	if(buf) {
		rcv = (testDgram_t *)buf;
	} else {
		return;
	}
	if(msg_flags & MSG_NOTIFICATION) {
		notify_in_cnt++;
		handle_notification(receive_buffer, notDone);
	} else {
		msg_in_cnt++;
		if(verbose) {
			if ((msg_in_cnt % 100) == 0) {
				printf("msg count now %d\n", msg_in_cnt);
			}
		}
		if ((rcv->type == SCTP_TEST_LOOPREQ) && (do_not_respond == 0)) {
			if(verbose) {
				printf("Got loop request id:%x - send resp\n",sinfo_in.sinfo_assoc_id);
			}
			       
			rcv->type = SCTP_TEST_LOOPRESP;
			if (verbose) {
				printf("Send loop response of size %d\n", ret);
			}
			sends_ping_resp++;
			respmsg = sctp_send(sd, buf, ret, &sinfo_in, 0);
			if(respmsg < 0) {
				if(quiet_mode == 0) {
					print_stats();
					printf("sctp_send error:%d - enter silent mode until comm-lost\n", errno);
				}
				do_not_respond = 1;
				return;
			}
		} 
	}
	sctp_msg_cleanup();
}

int
main (int argc, char **argv)
{
	testDgram_t *snd;
	char send_buffer[SEND_BUF_SIZE];
	socklen_t salen;
	int i, sd, notDone = 1, at;
	int send_out=0;
	uint8_t dest_addr_set = 0, dest_port_set=0;
	remote_port = local_port = htons(2222);
	memset(&addr, 0, sizeof(addr));
	memset(&bindto, 0, sizeof(bindto));
	mydelay = 1;
	bindto.sa.sa_family = AF_INET;
	bindto.sa.sa_len = sizeof(struct sockaddr_in);
	addr.sa.sa_family = AF_INET;
	addr.sa.sa_len = sizeof(struct sockaddr_in);
	use_v6 = 0;
	while((i= getopt(argc, argv,"lSLs:c:46m:p:vB:h:D:?HZqf:P:")) != EOF) {
		switch(i) {
		case 'D':
			mydelay = strtoul(optarg, NULL, 0);
			if (mydelay == 0) {
				printf("Warning, test app may not function/fail without some delay\n");
			}
			break;
		case 'P':
			pdapi_point = strtol(optarg, NULL, 0);
			break;
		case 'Z':
			no_hb_delay = 1;
			break;
		case 'l':
			listen_only = 1;
			break;
		case 'S':
			send_loop_request = 0;
			break;
		case 'L':
			send_loop_request = 1;
			break;

		case 's':
			size_to_send = strtol(optarg, NULL, 0);
			if(size_to_send > SEND_BUF_SIZE) {
				printf("Sorry %d is to big, shrinking to my max:%d\n",
				       size_to_send, SEND_BUF_SIZE);
				size_to_send = SEND_BUF_SIZE;
			} else if ((size_to_send != 0) && (size_to_send < sizeof(testDgram_t))) {
				printf("Sorry need minimum size of %u bytes - overriding to minimum value\n",
				       (uint32_t)sizeof(testDgram_t));
				size_to_send = sizeof(testDgram_t);
			}
			break;
		case 'c':
			cnt_to_send = strtol(optarg, NULL, 0);
			break;
		case '4':
			bindto.sa.sa_family = AF_INET;
			bindto.sa.sa_len = sizeof(struct sockaddr_in);
			addr.sa.sa_family = AF_INET;
			addr.sa.sa_len = sizeof(struct sockaddr_in);
			use_v6 = 0;
			break;
		case '6':
			bindto.sa.sa_family = AF_INET6;
			bindto.sa.sa_len = sizeof(struct sockaddr_in6);
			addr.sa.sa_family = AF_INET6;
			addr.sa.sa_len = sizeof(struct sockaddr_in6);
			use_v6 = 1;
			break;
		case 'f':
			fragment_interleave = strtol(optarg, NULL, 0);
			if ((fragment_interleave < 0) || (fragment_interleave > 2)) {
				printf("Fragment intern-leave defaults to 0, must be 0, 1 or 2\n");
				fragment_interleave = 0;
			}
			break;
		case 'q':
			verbose = 0;
			quiet_mode = 1;
			break;
		case 'I':
			strm_in = (uint16_t)strtol(optarg, NULL, 0);
			break;
		case 'O':
			strm_out = (uint16_t)strtol(optarg, NULL, 0);
			break;

		case 'm':
			local_port = htons(((uint16_t)strtol(optarg, NULL, 0)));
			break;
		case 'p':
			remote_port = htons(((uint16_t)strtol(optarg, NULL, 0)));
			if(remote_port)
				dest_port_set=1;
			break;

		case 'v':
			verbose = 1;
			quiet_mode = 0;
			break;
		case 'B':
		{
			bind_string = optarg;
			if (inet_pton(AF_INET6, optarg, (void *)&bindto.sin6.sin6_addr) != 1) {
				if (inet_pton(AF_INET, optarg, (void *)&bindto.sin.sin_addr) != 1) {
					printf("-B Un-parsable address not AF_INET6/or/AF_INET4 %s\n",
					       optarg);
					printf("Default to bind-all\n");
					memset(&bindto, 0, sizeof(bindto));
					if (use_v6) {
						bindto.sa.sa_family = AF_INET6;
						bindto.sa.sa_len = sizeof(struct sockaddr_in6);
					} else {
						bindto.sa.sa_family = AF_INET;
						bindto.sa.sa_len = sizeof(struct sockaddr_in);
					}
					bind_string = NULL;
				}
			}
		}
		break;
		case 'h':
		{
			conn_string = optarg;
			if (inet_pton(AF_INET6, optarg, (void *)&addr.sin6.sin6_addr) != 1) {
				if (inet_pton(AF_INET, optarg, (void *)&addr.sin.sin_addr) != 1) {
					printf("-h Un-parsable address not AF_INET6/or/AF_INET4 %s\n",
					       optarg);
					listen_only = 1;
					printf("Will listen only\n");
				} else {
					addr.sa.sa_family = AF_INET;
					addr.sa.sa_len = sizeof(struct sockaddr_in);
					listen_only = 0;
					dest_addr_set = 1;
				}
			} else {
				addr.sa.sa_family = AF_INET6;
				addr.sa.sa_len = sizeof(struct sockaddr_in6);
				listen_only = 0;
				dest_addr_set = 1;
			}
		}
		break;
		default:
		case '?':
		case 'H':
			useage(argv[0]);
			return(0);
			break;
		}
	}
	if(verbose) {
		if(listen_only) {
			printf("I will listen to port %d and not associate\n", ntohs(remote_port));
		} else {
			printf("I will try to keep a association up to address:%s port:%d until ctl-c\n",
			       conn_string, ntohs(remote_port));
		}
		if(bind_string == NULL) {
			printf("I will bind %s to addr:INADDR_ANY port:%d\n", 
			       ((use_v6) ? "IPv6" : "IPv4"),
			       ntohs(local_port));
		} else {
			printf("I will bind %s to addr:%s port:%d\n", 
			       ((use_v6) ? "IPv6" : "IPv4"),
			       bind_string, ntohs(local_port));
		}
		if(size_to_send) {
			printf("When an association begins, I will send %d %s records of %d bytes\n",
			       cnt_to_send, (send_loop_request ? "loop request" : "simple"  ),size_to_send);
		} else {
			printf("I will NOT send any data at association startup\n");
		}
		printf("I will mydelay %d. seconds between any assoc failure to restart attempt\n",
		       mydelay);
	}
	if ((listen_only == 0) && ((dest_addr_set == 0)  || (dest_port_set == 0))) {
		printf("You must set -h and -p if you do not use -l\n");
		useage(argv[0]);
		exit(0);
	}
	at = 0;
	for(i=0; i<sizeof(send_buffer); i++) {
		send_buffer[i] = at + ' ';
		at++;
		if (at > 93)
			at = 0;
	}
	snd = (testDgram_t *)send_buffer;
	if (send_loop_request) {
		snd->type = SCTP_TEST_LOOPREQ;
	} else {
		snd->type = SCTP_TEST_SIMPLE;		
	}
	memset(&sinfo_out, 0, sizeof(sinfo_out));
	memset(&sinfo_in, 0, sizeof(sinfo_in));	

	snd->dgramLen = size_to_send - sizeof(testDgram_t);
	snd->dgramID = dgram_id_counter++;
	bindto.sin.sin_port = local_port;
	if(listen_only == 0) {
		printf("set remote port to %d\n", ntohs(remote_port));
		addr.sin.sin_port = remote_port;		
	}
	sd = setup_a_socket();
	if(sd == -1) {
		printf("huh?\n");
		exit(0);
	}
	while (1) {
		if (addr.sa.sa_family == AF_INET) {
			salen = sizeof(struct sockaddr_in);
		} else {
			salen = sizeof(struct sockaddr_in6);
		}
		if (size_to_send) {
			send_out = cnt_to_send;
		}
		if (listen_only == 0) {
			if (send_out) {
				if(sctp_sendx(sd, send_buffer, size_to_send, &addr.sa,
					      1, &sinfo_out, 0) < 0) {
					printf("Send failed error:%d\n", errno);
				} else {
					snd->dgramID = dgram_id_counter++;
					send_out--;
					sends_out++;
					printf("Associd:0x%x - Implicit send to:", sinfo_out.sinfo_assoc_id);
					SCTPPrintAnAddress(&addr.sa);
				}
			} else {
				if(sctp_connectx(sd, &addr.sa, 1, &sinfo_out.sinfo_assoc_id) < 0) {
					if (errno == EALREADY) {
						printf("Connect already running\n");
						goto start_loop;
					}
					printf("Connect fails error:%d - next loop\n", errno);
					goto next_loop;
				}
				printf("Associd:0x%x - Attempt to connect to:", sinfo_out.sinfo_assoc_id);
				SCTPPrintAnAddress(&addr.sa);

			}
		} 
	start_loop:
		while (notDone) {
			if ((listen_only == 0) && send_out) {
				/* queue up the rest */
				while (send_out > 0) {
					(void)sctp_sendx(sd, send_buffer, size_to_send, &addr.sa,
						   1, &sinfo_out, 0);
					sends_out++;
					send_out--;
				}
			}
			handle_read_event(&notDone, sd);
		}
	next_loop:
		if(mydelay)
			sleep(mydelay);
		notDone = 1;
		do_not_respond = 0;
		msg_in_cnt = msg_in_cnt_weor = notify_in_cnt = sends_out = sends_ping_resp = 0;
	}
}
