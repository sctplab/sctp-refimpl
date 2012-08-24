#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/signal.h>
#include "pdapi_req.h"

FILE *sum_out=NULL;

int verbose = 0;

struct requests *base=NULL;

struct asoc_read_log {
  sctp_assoc_t assoc_id;
  int sz;
  int flags;
};
#define READ_LOG_SIZE 50000
struct asoc_read_log rdlog[READ_LOG_SIZE];
static int rdlog_at=0;
static int rdlog_wrap=0;
static unsigned int total_msgs=0;


int chars_out=0;


struct requests *
find_assoc(sctp_assoc_t id)
{
	struct requests *who;
	who = base;
	while(who) {
		if(who->assoc_id == id){
			break;
		}
		who = who->next;
	}
	return(who);
}


void
sum_it_out(uint8_t *data, int size)
{
	int i;
	for(i=0; i<size; i++) {
		fprintf(sum_out, "%2.2x ", data[i]);
		chars_out++;
		if(chars_out == 16){
			fprintf(sum_out, "\n");
			chars_out = 0;
		}
	}
}


int
audit_a_msg (struct requests *who)
{
	struct data_block *blk, *end_blk, *dat=NULL, *req=NULL;	
	struct pdapi_request *msg, *end; 
	int cnt_data=0, cnt_end=0, tot_size, calc_size=0;
	uint32_t base_crc = 0xffffffff, passed_sum, final_sum;
	ushort ssn_req, ssn_data, ssn_end;
	int ret=1;

	if(who->first == NULL) {
		return (1);
	}
	msg = (struct pdapi_request *)who->first->data;
	if(msg->request != PDAPI_REQUEST_MESSAGE) {
		/* not a request at the head? */
		ret = 0;
		goto clean_it;
	} else {
		req = who->first;;
		tot_size = ntohl(msg->msg.size);
	}
	ssn_req = who->first->info.sinfo_ssn;
	ssn_data = ssn_req + 1;
	ssn_end = ssn_data + 1;
	/* now do we have all the ssn's */
	blk = who->first->next;
	while(blk) {
		if(blk->info.sinfo_ssn == ssn_data) {
			if(dat == NULL) {
				dat = blk;
			}
			cnt_data++;
		} else if (blk->info.sinfo_ssn == ssn_end) {
			cnt_end++;
			end = (struct pdapi_request *)blk->data;
			passed_sum = end->msg.checksum;
			end_blk = blk;
			break;
		}
		blk = blk->next;
	}
	if(cnt_data && cnt_end) {
		/* we have at least ONE complete message */
		/* get rid of request */
		blk = dat;
		if(blk->info.sinfo_ssn != ssn_data) {
			printf("Out of order\n");
			ret = 0;
			goto clean_it;
		}
		msg = (struct pdapi_request *)blk->data;
		if(msg->request != PDAPI_DATA_MESSAGE) {
			printf("Data garbled -- not request data type\n");
			ret = 0;
			goto clean_it;
		}
		/* csum the first msg */
		calc_size = blk->sz - sizeof(struct pdapi_request) + sizeof(int);
		if(sum_out) {
			sum_it_out(msg->msg.data, calc_size);
		}
		base_crc = update_crc32(base_crc, msg->msg.data, calc_size);
		blk = blk->next;
		while(blk && (blk != end_blk)) {
			base_crc = update_crc32(base_crc, blk->data, blk->sz);
			if(sum_out) {
				sum_it_out(blk->data, blk->sz);
			}
			calc_size += blk->sz;
			blk = blk->next;
		}
		final_sum = sctp_csum_finalize(base_crc);
		if(sum_out) {
			fprintf(sum_out, "\n");
			fflush(sum_out);
		}
		msg = (struct pdapi_request *)end_blk->data;
		if (msg->request != PDAPI_END_MESSAGE) {
			printf("Last msg not END?\n");
			passed_sum = 0;
		} else {
			passed_sum = msg->msg.checksum;
		}
		if(calc_size != tot_size) {
			printf("Message size was supposed to be %d but saw %d\n",
			       tot_size, calc_size);
		}
		if(passed_sum != final_sum) {
			printf("Checksum mis-match should be %x but is %x\n",
			       (u_int)passed_sum, (u_int)final_sum);
		}
		total_msgs++;
		if ((total_msgs % 10000) == 0) {
			printf("Processed %d messages\n", total_msgs);
		}
		/* clean up time */
	clean_it:
		who->msg_cnt -= 3;
		while ((who->first != end_blk) && (who->first != NULL)) {
			blk = who->first;
			who->first = blk->next;
			free(blk);
		}
		if(who->first == end_blk) {
			who->first = end_blk->next;
			free(end_blk);
		}
		if(who->first == NULL) 
			who->tail = NULL;
	}
	return (ret);
}

void
pdapi_addasoc( struct sockaddr_in *from, struct sctp_assoc_change *asoc)
{
	struct requests *who;
	who = malloc(sizeof(struct requests));
	if(who == NULL) {
		perror("out of memory");
		abort();
	}
	memset(who, 0, sizeof(struct requests));
	printf("adding assocation %x\n", asoc->sac_assoc_id);
	who->assoc_id = asoc->sac_assoc_id;
	who->msg_cnt = 0;
	who->who = *from;
	who->prev = who->next = NULL;
	who->first = NULL;
	who->tail = NULL;
	if(base == NULL) {
		base = who;
	} else {
		who->next = base;
		who->next->prev = who;
		base = who;
	}
}

int
pdapi_clean_all(struct requests *who)
{
	struct data_block *blk,*nxt;	
	int cnt=0;
	blk = who->first;
	while(blk) {
		nxt = blk->next;
		blk->next = NULL;
		free(blk);
		blk = nxt;
		cnt++;
	}
	return (cnt);
}

void
pdapi_delasoc(sctp_assoc_t id)
{
	struct requests *who;
	who = find_assoc(id);
	if(who) {
		if(who->next) {
			who->next->prev = who->prev;
		}
		if(who->prev) {
			who->prev->next = who->next;
		} else {
			base = who->next;
		}
		who->next = NULL;
		who->prev = NULL;
		audit_a_msg(who);
		pdapi_clean_all(who);
		who->first = who->tail = NULL;
		free(who);
	}
}

void
pdapi_process_data(unsigned char *buffer, 
		   ssize_t len, 
		   struct sctp_sndrcvinfo *sinfo, 
		   struct sockaddr_in *from, 
		   int flags)
{
	/* find the assoc */
	/* add the data to the list */
	/* if MSG_EOR then call the audit function */
	struct data_block *blk;
	struct requests *who;
	if(sinfo->sinfo_assoc_id == 0) {
		printf("Zero'ed assoc id\n");
		abort();
	}
	who = find_assoc(sinfo->sinfo_assoc_id);
	if(who == NULL) {
		printf("Huh, can't find asoc %x\n", (u_int)sinfo->sinfo_assoc_id);
		abort();
	}
	blk = malloc(sizeof(struct data_block) + len);
	if (blk == NULL) {
		printf("Can't allocate a block of size %d + %d\n", (int)len, (int)sizeof(struct data_block));
		abort();
	}
	memset(blk, 0, sizeof(struct data_block));
	blk->next = NULL;
	memcpy(&blk->info, sinfo, sizeof(struct sctp_sndrcvinfo));
	blk->sz = len;
	memcpy(blk->data, buffer, len);
	if(who->first != NULL) {
		if(who->tail) {
			who->tail->next = blk;
			who->tail = blk;
		} else {
			/* huh? */
			abort();
		}
	} else {
		who->first = blk;
		who->tail = blk;
	}
	if(flags & MSG_EOR) {
		who->msg_cnt++;
		blk->last = 1;
		if ((who->msg_cnt % 3) == 0) {
			(void)audit_a_msg (who);
		}
	}
}

static void
pdapi_abortrecption(struct sctp_pdapi_event *pdapi)
{
	/* What do we do here? */
	struct requests *who;
	who = find_assoc(pdapi->pdapi_assoc_id);
	if(who == NULL)
		return;
	pdapi_clean_all(who);
	who->first = who->tail = NULL;
	who->msg_cnt = 0;
}

void
pdapi_notification(unsigned char *buffer, 
		   ssize_t len, 
		   struct sctp_sndrcvinfo *sinfo, 
		   struct sockaddr_in *from)
{
	struct sctp_tlv *sn_header;
	sn_header = (struct sctp_tlv *)buffer;
	struct sctp_shutdown_event *shut;
	struct sctp_assoc_change  *asoc;
	struct sctp_pdapi_event *pdapi;

	switch (sn_header->sn_type) {
	case SCTP_ASSOC_CHANGE:
		asoc = (struct sctp_assoc_change  *)sn_header;
		if (asoc->sac_state == SCTP_COMM_UP) {
			pdapi_addasoc(from, asoc);
		} else if (asoc->sac_state == SCTP_COMM_LOST) {
			printf("Comm lost id:%x del assoc\n",
			       asoc->sac_assoc_id);
			pdapi_delasoc(asoc->sac_assoc_id);
		}
		break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
		pdapi = (struct sctp_pdapi_event *)sn_header;
		pdapi_abortrecption(pdapi);
		break;
	case SCTP_SHUTDOWN_EVENT:
		shut = (struct sctp_shutdown_event *)sn_header;
		printf("Shutdown assoc id:%x del assoc\n",
		       shut->sse_assoc_id);
		pdapi_delasoc(shut->sse_assoc_id);
		break;
	case SCTP_REMOTE_ERROR:
	case SCTP_SEND_FAILED:
	case SCTP_ADAPTATION_INDICATION:
	case SCTP_STREAM_RESET_EVENT:
	case SCTP_PEER_ADDR_CHANGE:
	default:
		printf("Un-handled event type %d?\n",
		       sn_header->sn_type);
		break;
	}
}

void
pdapi_process_msg(unsigned char *buffer, 
		  ssize_t len, 
		  struct sctp_sndrcvinfo *sinfo, 
		  struct sockaddr_in *from, 
		  int flags)
{
	if(verbose){
		printf("Process a message of %d bytes, assoc:%x flags:%x\n",
		       (int)len, (u_int)sinfo->sinfo_assoc_id, (u_int)flags);
	}
	if(flags & MSG_NOTIFICATION) {
		pdapi_notification(buffer, len, sinfo, from);
	} else {
		pdapi_process_data(buffer, len, sinfo, from, flags);
	}
}

uint8_t buffer[PDAPI_DATA_BLOCK_SIZE];

int
main(int argc, char **argv)
{

	int i, fd, flags=0;
	u_int16_t port=0;
	int level=SCTP_FRAG_LEVEL_1;
	int val;
	size_t len;
	socklen_t slen;
	socklen_t fromlen;
	struct sctp_sndrcvinfo sndrcv;
	struct sockaddr_in bindto,got,from;
	struct sctp_event_subscribe event;
	
	memset(&bindto,0,sizeof(bindto));
	bindto.sin_family = AF_INET;
	while((i= getopt(argc,argv,"p:vl:S:B:")) != EOF){
		switch(i){
		case 'S':
			sum_out = fopen(optarg, "w+");
			printf("Putting sum log out to %s\n", optarg);
			break;
		case 'l':
			val = strtol(optarg, NULL, 0);
			if ((val < SCTP_FRAG_LEVEL_0) ||
			    (val > SCTP_FRAG_LEVEL_2)) {
				printf("Sorry level must be %d >= %d <= %d\n",
				       SCTP_FRAG_LEVEL_0,
				       val,
				       SCTP_FRAG_LEVEL_2);
				return (-1);
			}
			break;
		case 'B':
			if (inet_pton(AF_INET, optarg, &bindto.sin_addr)) {
				bindto.sin_family = AF_INET;
				bindto.sin_len = sizeof(bindto);
			} else {
				printf("Cannot decode bindto address - using INADDR_ANY\n");
				memset(&bindto,0,sizeof(bindto));
				bindto.sin_family = AF_INET;
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		default:
			break;
		};
	}
	signal(SIGPIPE,SIG_IGN);
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	if (setsockopt(fd, IPPROTO_SCTP, 
		       SCTP_FRAGMENT_INTERLEAVE, 
		       &level, sizeof(level)) != 0) {
		printf("Can't set FRAGMENT_INTERLEAVE socket option! err:%d\n", errno);
		return (-1);
	}
	slen = sizeof(bindto);
	bindto.sin_len = sizeof(bindto);
	bindto.sin_port = htons(port);
	if(bind(fd,(struct sockaddr *)&bindto, slen) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
		return(-1);
	}
	slen = sizeof(got);
	if(getsockname(fd, (struct sockaddr *)&got, &slen) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		return(-1);
	}	
	if(port){
		if(bindto.sin_port && (got.sin_port != bindto.sin_port)){
			printf("Warning:could not get your port got %d instead\n", 
			       ntohs(got.sin_port));
		}
	}
	printf("Server listens on port %d\n",
	       ntohs(got.sin_port));
	errno = 0;
	/* enable all event notifications */
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_address_event = 0;
	event.sctp_send_failure_event = 0;
	event.sctp_peer_error_event = 0;
	event.sctp_shutdown_event = 1;
	event.sctp_partial_delivery_event = 1;
	event.sctp_adaptation_layer_event = 0;
	event.sctp_stream_reset_event = 0;

	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
		return (-1);
	}
	if(listen(fd,1) == -1){
		printf("Can't listen err:%d\n",errno);
		return (-1);
	}
	while(1) {
		fromlen = sizeof(from);
		memset(&sndrcv, 0, sizeof(sndrcv));
		flags = 0;
		len = sctp_recvmsg(fd, buffer, (size_t)PDAPI_DATA_BLOCK_SIZE, 
				   (struct sockaddr *)&from, &fromlen,
				   &sndrcv, &flags);
		rdlog[rdlog_at].sz = len;
		rdlog[rdlog_at].flags = flags;
		rdlog[rdlog_at].assoc_id = sndrcv.sinfo_assoc_id;
		rdlog_at++;
		if(rdlog_at >= READ_LOG_SIZE) {
			rdlog_at = 0;
			rdlog_wrap++;
		}
		if(len > 0) {
			pdapi_process_msg(buffer, len, &sndrcv, &from, flags);
		}
	}
}

