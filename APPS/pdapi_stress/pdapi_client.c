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

FILE *sum_out;
struct pdapi_request request;
uint8_t large_buffer[0x40000];
struct pdapi_request sum;
struct sockaddr_in peer;
int sd;
uint16_t seq=0;
int max_size = 0x3ffff;

int chars_out=0;
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
send_a_request()
{
	int i, *p, ret;
	size_t sz;
	struct sctp_sndrcvinfo sinfo;
	struct pdapi_request *msg;
	uint32_t base_crc = 0xffffffff;
	/* We need to send three messages */
	/* 1) The message start with the size */
	/* 2) The data message that has a crc32 run over it */
	/* 3) The end message with the crc32 we caculated */
	memset(large_buffer,0, sizeof(large_buffer));
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.sinfo_flags = SCTP_EOR;
	msg = (struct pdapi_request *)&large_buffer[0];
	request.request = PDAPI_REQUEST_MESSAGE;
	msg->request = PDAPI_DATA_MESSAGE;
	sum.request = PDAPI_END_MESSAGE;
	
	/* First we must generate the size of
	 * the message. 
	 */
 again:
	sz = rand() & 0x0003ffff;
	if ((sz < 4) || (sz > max_size)) {
		goto again;
	}
	/* round it down to even word boundary */
	sz &= ~0x03;
	p = (int *)msg->msg.data;
	for(i=0;i<sz/4;i+=sizeof(int)) {
		*p = rand();
		p++;
	}
	/* now we need to csum it */
	if (sum_out)
		sum_it_out(msg->msg.data, sz);
	base_crc = update_crc32(base_crc, msg->msg.data, sz);
	sum.msg.checksum = sctp_csum_finalize(base_crc);
	if(sum_out) {
		fprintf(sum_out, "\n");
	}
	/* now our messages are formed. Send them */

	/* network-ify the size 
	 */
	request.msg.size = htonl(sz);

	/* Increase it to include the header */
	sz += sizeof(request) - sizeof(int);

	request.seq = ntohs(seq);
	seq++;
	/* Note for now we just send in stream 0 */
	ret = sctp_sendx(sd, &request, sizeof(request), 
			 (struct sockaddr *)&peer, 1,
			 &sinfo, 0);
	if (ret < sizeof(request)) {
		printf("error in sending request %d errno:%d\n",
		       ret, errno);
		return(0);
	}
	msg->seq = ntohs(seq);
	seq++;
	ret = sctp_sendx(sd, large_buffer, sz,
			 (struct sockaddr *)&peer, 1,
			 &sinfo, 0);
	if (ret == -1) {
		printf("error in sending message %d errno:%d\n",
		       ret, errno);
		return(0);
	
	} else if (ret < sz) {
		printf("Msg send incomplete, wanted to send %d  actual:%d\n",
		       (int)sz, ret);

	}
	sum.seq = ntohs(seq);
	seq++;
	ret = sctp_sendx(sd, &sum, sizeof(sum), 
			 (struct sockaddr *)&peer, 1,
			 &sinfo, 0);
	if (ret < sizeof(request)) {
		printf("error in sending sum %d errno:%d\n",
		       ret, errno);
		return(0);
	}
	return (1);
}

int
handle_notification(char *buf)
{
	struct sctp_tlv *sn_header;
	sn_header = (struct sctp_tlv *)buf;
	struct sctp_assoc_change  *asoc;

	switch (sn_header->sn_type) {
	case SCTP_ASSOC_CHANGE:
		asoc = (struct sctp_assoc_change  *)sn_header;
		if (asoc->sac_state == SCTP_COMM_UP) {
			break;
		} else if (asoc->sac_state == SCTP_COMM_LOST) {
			printf("Got a comm-lost event, exiting\n");
			return(0);
		} else if (asoc->sac_state == SCTP_CANT_STR_ASSOC) {
			printf("Can't start association!\n");
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
main(int argc, char **argv) 
{
	int i, not_done=1, ret;
	unsigned seed=0;
	int flags;
	int cnt=0;
	int limit = 0;
	struct sctp_sndrcvinfo sinfo;
	uint16_t port=0;
	char *host=NULL;
	struct sockaddr_in rcvfrom;
	socklen_t rcvsz;
	char read_buffer[2048];
	struct sctp_event_subscribe event;
	
	memset (&peer, 0, sizeof(peer));
	while((i= getopt(argc,argv,"p:h:s:l:S:")) != EOF){
		switch(i){
		case 'S':
			sum_out = fopen(optarg, "w+");
			printf("Putting sum log out to %s\n", optarg);
			break;
		case 's':
			seed = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = (uint16_t)(strtol(optarg, NULL,0) & 0x0000ffff);
			break;
		case 'l':
			limit = strtol(optarg, NULL, 0);
			break;
		default:
			goto use;
			break;
		};
	}

	if ((host == NULL) || (seed == 0) || (port == 0)) {
	use:
		printf("Use %s -s seed -p port -h dot.host.ip.addr [-l limit]\n",
		       argv[0]);
		printf("Where seed is the random number seed\n");
		printf("Where port is the peer port where pdapi_server is\n");
		printf("Where dot.host.ip.addr is the peer ipv4 address\n");
		printf("Where limit is the number of msg sets to send (0 is unlimited)\n");
		return(-1);
	}
	
	if (inet_pton(AF_INET, host, &peer.sin_addr)) {
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
	if (sd == -1) {
		printf("error %d can't open sctp socket\n", errno);
		return (-1);
	}
#ifdef SCTP_EOR
	if(sd > 0) {
		int one = 1;
		if(setsockopt(sd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR, &one, sizeof(one)) < 0) {
			printf("pdapi_server: setsockopt: SCTP_EXPLICIT_EOR failed! errno=%d\n", errno);
		}
	}
#else
        {
		int optlen = sizeof(maxsz);
		if(getsockopt(sd, SOL_SOCKET, SO_SNDBUF, &maxsz, &optlen) != 0)	
			exit(-1);
		else
			printf("Max msg size shrinks from 0x3ffff to %x due to missing Explicit-EOR mode\n", maxsz)
	}

#endif

	memset(&event, 0, sizeof(event));
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_shutdown_event = 1;
	if (setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
		return (-1);
	}
	while(not_done) {
		not_done = send_a_request();
		if(not_done == 0) {
			printf("Process ends\n");
			continue;
		}
		cnt++;
		rcvsz = sizeof(rcvfrom);
		flags = MSG_DONTWAIT;
		ret = sctp_recvmsg(sd, read_buffer, sizeof(read_buffer),
				   (struct sockaddr *)&rcvfrom, &rcvsz,
				   &sinfo, &flags);
		if ((ret > 0) && (flags & MSG_NOTIFICATION)) {
			not_done = handle_notification(read_buffer);
		}
		if(limit && (cnt >= limit)) {
			printf("Sends complete\n");
			not_done = 0;
		}
	}
	if(sum_out)
		fclose(sum_out);
	return (0);
}
