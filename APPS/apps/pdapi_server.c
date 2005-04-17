#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <sys/signal.h>
#include "pdapi_req.h"

#define PDAPI_DATA_BLOCK_SIZE 16000

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

void
clean_up_broken_msg(struct requests *who)
{
  struct data_block *blk,*nxt;	
  struct pdapi_request *msg;
  ushort ssn;
  int killed=0;
  printf("Failed to process a message, cleaning\n");
 try_again:
  if(who->first == NULL) {
    printf("msg queue now empty\n");
    return;
  }
  msg = (struct pdapi_request *)who->first->data;
  if(msg->request == PDAPI_REQUEST_MESSAGE) {
    /* this is correct */
    printf("First msg is correct\n");
    ssn = who->first->info.sinfo_ssn;
  } else if (msg->request == PDAPI_END_MESSAGE) {
    printf("First msg is a End?? -- unexpected, freeing\n");
    blk = who->first;
    who->first = blk->next;
    blk->next = NULL;
    if(blk == who->tail) {
      tail = NULL;
    }
    free(blk);
    killed++;
    goto try_again;
  } else if (msg->request == PDAPI_DATA_MESSAGE) {
    printf("First msg is a Data?? -- unexpected freeing\n");
    blk = who->first;
    who->first = blk->next;
    blk->next = NULL;
    if(blk == who->tail) {
      tail = NULL;
    }
    free(blk);
    killed++;
    goto try_again;
  } else {
    printf("First msg is a partial message of data -- unexpected freeing\n");
    blk = who->first;
    who->first = blk->next;
    blk->next = NULL;
    if(blk == who->tail) {
      tail = NULL;
    }
    free(blk);
    killed++;
    goto try_again;
  }
  printf("Ok, we killed %d msgs\n", killed);
  if(killed) {
    return;
  }
  printf("This requires further analysis req-ssn:%d\n",ssn);
  abort();
}

int
audit_a_msg (struct requests *who)
{
  struct data_block *blk,*nxt, *end_blk;	
  struct pdapi_request *msg, *end, *datafirst=NULL;
  int cnt_data=0, cnt_end=0, tot_size, calc_size=0;
  uint32_t base_crc = 0xffffffff, passed_sum;

  msg = (struct pdapi_request *)who->first->data;
  ushort ssn_req, ssn_data, ssn_end;
  if(msg->request != PDAPI_REQUEST_MESSAGE) {
    /* not a request at the head? */
    return(0);
  } else {
    tot_size = msg->msg.size;
  }
  ssn_req = who->first->info.sinfo_ssn;
  ssn_data = ssn_req + 1;
  ssn_end = ssn_data + 1;
  /* now do we have all the ssn's */
  blk = who->first->next;
  while(blk) {
    if(blk->info.sinfo_ssn == ssn_data) {
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
    printf("We have %d data blocks and %d ends\n", 
	   cnt_data, cnt_end);

    /* get rid of request */
    blk = who->first;
    who->first = blk->next;
    if(blk == who->tail) {
      /* should not happen */
      who->tail = NULL;
    }
    free(blk);
    blk = who->first;
    if(blk->info.sinfo_ssn != ssn_data) {
      printf("Out of order\n");
      return(0);
    }
    msg = (struct pdapi_request *)blk->data;
    if(msg->request != PDAPI_DATA_MESSAGE) {
      printf("Data garbled -- not request data type\n");
      return (0);
    }
    /* csum the first msg */
    calc_size = blk->sz - 1;
    base_crc = update_crc32(base_crc, msg->msg.data, calc_size);
    who->first = blk->next;
    if(blk == who->tail) {
      /* should not happen */
      who->tail = NULL;
    }
    free(blk);
    blk = who->first;
    while(blk && (blk != end_blk)) {
      base_crc = update_crc32(base_crc, blk->data, blk->sz);
      calc_size += blk->sz;
      who->first = blk->next;
      if(blk == who->tail) {
	/* should not happen */
	who->tail = NULL;
      }
      free(blk);
      blk = who->first;
    }
    if(who->first == end_blk) {
      printf("Ate all data and saw %d bytes\n", calc_size);
      base_crc = sctp_csum_finalize(base_crc);
      who->first = end_blk->next;
      if(who->tail == end_blk) {
	/* may happen */
	who->tail = NULL;
      }
      free(end_blk);
    } else {
      printf("corrupt chain\n");
      return(0);
    }
    if(calc_size != tot_size) {
      printf("Message size was supposed to be %d but saw %d\n",
	     tot_size, calc_size);
    }
    if(passed_sum != base_crc) {
      printf("Checksum mis-match should be %x but is %x\n",
	     (u_int)passed_sum, (u_int)base_crc);
    }
    return(1);
  }
  printf("Not a complete msg yet\n");
  return (1);
}

void
audit_all_msg(struct requests *who)
{
  struct data_block *blk,*nxt;	
  int cnt=0,ret;
  ushort ssn;
  int notset=1, notdone=1;

  blk = who->first;
  while(blk) {
    nxt = blk->next;
    if(notset) {
      notset = 0;
      ssn = blk->info.sinfo_ssn;
      cnt++;
    } else {
      if(blk->info.sinfo_ssn == ssn) {
	goto nxt_msg;
      } else {
	cnt++;
	if(blk->info.sinfo_ssn < ssn) {
	  printf("Found ssn:%d before finding ssn:%d??\n",
		 ssn, blk->info.sinfo_ssn);
	}
	ssn = blk->info.sinfo_ssn;
      }
    }
  nxt_msg:
    blk = nxt;      
  }
  printf("assoc:%x has %d messages %s",
	 who->assoc_id,
	 cnt,
	 (((cnt % 3) == 0) ? "which is normal" :
	  "Which is incorrect"));
  while(not_done) {
    if(who->first == NULL) {
      /* all gone */
      not_done = 0;
      continue;
    }
    ret = audit_a_msg (who);
    if(ret == 0) {
      /* we did NOT consume a message */
      clean_up_broken_msg(who);
    }
  }
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
  who->assoc_id = asoc->sac_assoc_id;
  who->who = *from;
  who->prev = who->next = NULL;
  who->first = NULL;
  if(base == NULL) {
    base = who;
  } else {
    who->next = base;
    who->base->prev = who;
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
    blk = next;
    cnt++;
  }
  return (cnt);
}

void
pdapi_delasoc( struct sctp_assoc_change *asoc)
{
  struct requests *who;
  for(who=base; who; who=who->next) {
    if(who->assoc_id == asoc->sac_assoc_id) {
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
      audit_all_msg(who);
      pdapi_clean_all(who);
      free(who);
      if(who->first) {
	printf("Association fails incompletely free:%d msgs\n",
	       pdapi_clean_all(who));	
      }
    }
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
  who = base;
  while(who) {
    if(who->assoc_id == sinfo->sinfo_assoc_id){
      break;
    }
    who = who->next;
  }
  if(who == NULL) {
    printf("Huh, can't find asoc %x\n", (u_int)sinfo->sinfo_assoc_id);
    abort();
  }
  blk = malloc(sizeof(struct data_block));
  blk->next = NULL;
  blk->info = *sinfo;
  blk->sz = len;
  if(who->tail) {
    who->tail->next = blk;
  } else {
    who->first = blk;
    who->tail = blk;
  }
  if(flags & MSG_EOR) {
    (void)audit_a_msg (who);
  }
}


void
pdapi_notification(unsigned char *buffer, 
		   ssize_t len, 
		   struct sctp_sndrcvinfo *sinfo, 
		   struct sockaddr_in *from)
{
  struct sctp_tlv *sn_header;
  sn_header = (struct sctp_tlv *)buffer;
  struct sctp_assoc_change  *asoc;
  struct sctp_shutdown_event *shut;
  struct sctp_pdapi_event *pdapi;

  switch (sn_header->sn_type) {
  case SCTP_ASSOC_CHANGE:
    asoc = (struct sctp_assoc_change  *)sn_header;
    if (asoc->sac_state == SCTP_COMM_UP) {
      pdapi_addasoc( from, asoc );
    } else if (asoc->sac_state == SCTP_COMM_LOST) {
      pdapi_delasoc( from, asoc );
    }
    break;
  case SCTP_PARTIAL_DELIVERY_EVENT:
    pdapi = (struct sctp_pdapi_event *)sn_header;
    pdapi_abortrecption(pdapi);
    break;
  case SCTP_SHUTDOWN_EVENT:
    break;
  case SCTP_REMOTE_ERROR:
  case SCTP_SEND_FAILED:
  case SCTP_ADAPTION:
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



int
main(int argc, char **argv)
{
  char buffer[PDAPI_DATA_BLOCK_SIZE]
    u_int16_t port=0;
  ssize_t len;
  socklen_t fromlen;
  int flag;
  struct sctp_sndrcvinfo sndrcv;
  struct sockaddr_in bindto,got,from;
  struct sctp_event_subscribe event;
	
  optlen = sizeof(optval);
  sb = 0;
  while((i= getopt(argc,argv,"p:v")) != EOF){
    switch(i){
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
  memset(&bindto,0,sizeof(bindto));
  len = sizeof(bindto);
  bindto.sin_len = sizeof(bindto);
  bindto.sin_family = AF_INET;
  bindto.sin_port = htons(port);
  if(bind(fd,(struct sockaddr *)&bindto, len) < 0){
    printf("can't bind a socket:%d\n",errno);
    close(fd);
    return(-1);
  }
  if(getsockname(fd,(struct sockaddr *)&got,&len) < 0){
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
  event.sctp_adaption_layer_event = 0;
  event.sctp_stream_reset_events = 0;

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
    len = sctp_recvmsg(fd, buffer, (size_t)PDAPI_DATA_BLOCK_SIZE, 
		       (struct sockaddr *)&from, &fromlen,
		       &sndrcv, &flags);
    rdlog[rdlog_at].sz = len;
    rdlog[rdlog_at].flags = flags;
    rdlog[rdlog_at].assoc_id = sinfo->sinfo_assoc_id;
    rdlog_at++;
    if(rdlog_at >= READ_LOG_SIZE) {
      rdlog_at = 0;
      rdlog_wrap++;
    }
    if(len) {
      pdapi_process_msg(buffer, len, sndrcv, from, flags);
    }
  }
}

