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
struct data_block {
  struct data_block *next;
  struct sctp_sndrcvinfo info;
  unsigned char data[PDAPI_DATA_BLOCK_SIZE];
};


struct requests {
  sctp_assoc_t assoc_id;
  struct sockaddr_in who;
  struct sctp_requests *next;
  struct data_block *first;
};


struct requests *base=NULL;

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
    } else if (asoc->sac_state == SCTP_COMM_LOST) {
    }
    break;
  case SCTP_PARTIAL_DELIVERY_EVENT:
    pdapi = (struct sctp_pdapi_event *)sn_header;
    break;
  case SCTP_SHUTDOWN_EVENT:
    shut = (struct sctp_shutdown_event *)sn_header;
    
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
    len = sctp_recvmsg(fd, buffer, (size_t)PDAPI_DATA_BLOCK_SIZE, 
		       (struct sockaddr *)&from, &fromlen,
		       &sndrcv, &flags);
    if(len) {
      pdapi_process_msg(buffer, len, sndrcv, from, flags);
    }
  }
}

