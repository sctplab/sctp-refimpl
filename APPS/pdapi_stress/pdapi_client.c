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


struct pdapi_request request;
uint8_t large_buffer[100000];
struct pdapi_request sum;
struct sockaddr_in peer;
int sd;

void
send_a_request()
{
	struct pdapi_request *msg = large_buffer;
	/* We need to send three messages */
	/* 1) The message start with the size */
	/* 2) The data message that has a crc32 run over it */
	/* 3) The end message with the crc32 we caculated */
	request.request = PDAPI_DATA_MESSAGE;
	msg->request = PDAPI_REQUEST_MESSAGE;
	sum.request = PDAPI_END_MESSAGE;

	
}


int
handle_notification(char *buf)
{
	struct sctp_tlv *sn_header;
	sn_header = (struct sctp_tlv *)buf;
	struct sctp_shutdown_event *shut;
	struct sctp_assoc_change  *asoc;

	switch (sn_header->sn_type) {
	case SCTP_ASSOC_CHANGE:
		asoc = (struct sctp_assoc_change  *)sn_header;
		if (asoc->sac_state == SCTP_COMM_UP) {
			break;
		} else if (asoc->sac_state == SCTP_COMM_LOST) {
			printf("Got a comm-lost event, exiting\n");
			return(0);
		}
		break;
	case SCTP_SHUTDOWN_EVENT:
		printf("Got a shutdown event, exiting\n");
		return (0);
		break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	case SCTP_REMOTE_ERROR:
	case SCTP_SEND_FAILED:
	case SCTP_ADAPTATION_INDICATION:
	case SCTP_STREAM_RESET_EVENT:
	case SCTP_PEER_ADDR_CHANGE:
	default:
		break;
	}
	return (1);
}

int
main(int argc char **argv) 
{
	int i, not_done=1;
	unsigned seed=0;
	int flags;
	struct sctp_sndrcvinfo sinfo;
	uint16_t port=0;
	char *host=NULL;
	struct sockaddr_in rcvfrom;
	socklen_t rcvsz;
	char readbuffer[2048];
	struct sctp_event_subscribe event;
	
	memset (&peer, 0, sizeof(peer));
	while((i= getopt(argc,argv,"p:h:s:")) != EOF){
		switch(i){
		case 's':
			seed = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = (uint16_t)(strtol(optarg, NULL,0) & 0x0000ffff);
			break;
		default:
			goto use;
			break;
		};
	}

	if ((host == NULL) || (seed = 0) || (port == 0)) {
	use:
		printf("Use %s -s seed -p port -h dot.host.ip.addr\n",
		       argv[0]);
		printf("Where seed is the random number seed\n");
		printf("Where port is the peer port where pdapi_server is\n");
		printf("Where dot.host.ip.addr is the peer ipv4 address\n");
		return(-1);
	}
	
	if (inet_pton(AF_INET, argv[0], &peer.sin_addr)) {
		peer.sin_family = AF_INET;
		peer.sin_len = sizeof(peer);
		peer.sin_port = htons(port);
	}else{
		printf("can't set IPv4: incorrect address format\n");
		goto use;
	}
	/* setup the random number */
	srand(seed);
	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == NULL) {
		printf("error %d can't open sctp socket\n", errno);
		return (-1);
	}
	memset(event, 0, sizeof(event));
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_shutdown_event = 1;
	if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
		return (-1);
	}
	while(not_done) {
		send_a_request();
		rcvsz = sizeof(rcvfrom);
		flags = MSG_DONTWAIT;
		ret = sctp_recvmsg(sd, read_buffer, sizeof(read_buffer),
				   &rcvfrom, &rcvsz,
				   &sinfo, &flags);
		if ((ret > 0) && (flags & MSG_NOTIFICATION)) {
			not_done = handle_notification(read_buffer);
		}
	}

}
