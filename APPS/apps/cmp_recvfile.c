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
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct txfr_request{
	char filename[256];
}req;

static int
handle_notification(char *notify_buf)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	int retval = 0;
	snp = (union sctp_notification *)notify_buf;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {

		case SCTP_COMM_UP:
			break;
		case SCTP_COMM_LOST:
			printf("Communication LOST\n");
			retval = 1;
			break;
		case SCTP_RESTART:
			break;
		case SCTP_SHUTDOWN_COMP:
			retval = 1;
			break;
		case SCTP_CANT_STR_ASSOC:
			printf("Could not start association\n");
			retval = 1;
			break;
		default:
			break;
		} /* end switch(sac->sac_state) */
		break;
	case SCTP_PEER_ADDR_CHANGE:
		break;
	case SCTP_REMOTE_ERROR:
		break;
	case SCTP_SEND_FAILED:
		break;
	case SCTP_ADAPTION_INDICATION:
		break;
    
	case SCTP_PARTIAL_DELIVERY_EVENT:
		break;

	case SCTP_SHUTDOWN_EVENT:
		retval = 2;
		break;
	default:
		printf("Unknown notification event type=%xh\n", 
		       snp->sn_header.sn_type);
		break;
	} /* end switch(snp->sn_header.sn_type) */
	return(retval);
}



int
main(int argc, char **argv)
{
	char *addr=NULL,*file_wanted=NULL, *out_file=NULL;
	uint16_t port=0;
	struct sockaddr_in to,bindto,got,from;
	struct sctp_event_subscribe event;
	struct sctp_sndrcvinfo sinfo;
	char buffer[200000];
	struct timeval start, end;
	int outfd,ret, ret1, i, fd;
	int amount_in=0, amount_out=0;
	socklen_t len;
	uint32_t sec, usec;
	int flags;

	while((i= getopt(argc,argv,"p:h:f:F:")) != EOF){
		switch(i){
		case 'F':
			out_file = optarg;
			break;
		case 'h':
			addr = optarg;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		case 'f':
			file_wanted = optarg;
			break;
		};
	}
	if((addr == NULL) || (file_wanted == NULL) || (port == 0) || (out_file == NULL)){
		printf("Use: %s -h 10.1.1.1 -p 2222 -f rem-filename -F out-file\n",
		       argv[0]);
		return(-1);
	}
	memset(&to,0,sizeof(to));
	if(inet_pton(AF_INET, addr, (void *) &to.sin_addr)){
		to.sin_len = sizeof(to);
		to.sin_family = AF_INET;
		printf("port selected is %d\n",port);
		printf("addr %x\n",(u_int)ntohl(to.sin_addr.s_addr));
		to.sin_port = htons(port);
	}else{
		printf("Can't translate the address\n");
		return(-1);
	}
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_address_event = 0;
	event.sctp_send_failure_event = 0;
	event.sctp_peer_error_event = 0;
	event.sctp_shutdown_event = 1;
	event.sctp_partial_delivery_event = 0;
	event.sctp_adaptation_layer_event = 0;
	event.sctp_authentication_event = 0;
	event.sctp_stream_reset_event = 0;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
	}
	memset(&bindto,0,sizeof(bindto));
	len = sizeof(bindto);
	bindto.sin_len = sizeof(bindto);
	bindto.sin_family = AF_INET;
	bindto.sin_port = 0;
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
	printf("Client uses port %d\n",ntohs(got.sin_port));
	outfd = open(out_file, ((O_WRONLY|O_CREAT|O_TRUNC)));
	if(outfd == -1) {
		printf("Can't open out file %s err:%d",
		       out_file, errno);
		exit(-1);
	}
	gettimeofday(&start,NULL);
	memset(&req,0, sizeof(req));
	strcpy(req.filename, file_wanted);
	len = sizeof(to);
	ret = sctp_sendmsg (fd,
			    &req, /* msg */
			    sizeof(req),  /* data size */
			    (struct sockaddr *)&to, /* socket to */
			    len, /* socklen */
			    0, /* ppid */
			    SCTP_UNORDERED, /* flags */
			    0, /* stream */
			    0, /* time to live */
			    0); /* context */

	if(ret < sizeof(struct txfr_request)){
		printf("Huh send of txfr fails\n");
	exit_now:
		close(fd);
		close(outfd);
		return(-1);
	}
	len = sizeof(from);
	while(1) {
		flags = 0;
		ret = sctp_recvmsg (fd, buffer, sizeof(buffer), 	
				    (struct sockaddr *)&from,
				    &len, &sinfo, &flags);
		if(ret <= 0){
			printf("Gak, error %d on send\n",
			       errno);
			goto exit_now;
		}
		if(flags & MSG_NOTIFICATION) {
			if(handle_notification(buffer)) {
				printf("Association ends\n");
				break;
			}
		} else {
			amount_in += ret;
			ret1 = write(outfd, buffer, ret);
			if(ret != ret1) {
				printf("Can't write all the data %d vs %d\n",
				       ret, ret1);
			}
			if(ret1 > 0) {
				amount_out += ret1;
				
			}
		}
	}
	fchmod(outfd, 0644);
	close(outfd);
	close(fd);
	gettimeofday(&end,NULL);
	printf("Recieved %d wrote %d bytes\n",
	       amount_in,amount_out);
	printf("Start %d.%6.6d\n",
	       (int)start.tv_sec,(int)start.tv_usec);
	printf("End %d.%6.6d\n",
	       (int)end.tv_sec,(int)end.tv_usec);
	sec = end.tv_sec - start.tv_sec;
	if(end.tv_usec >= start.tv_usec){
		usec = end.tv_usec - start.tv_usec;
	}else{
		sec--;
		usec = (end.tv_usec + 1000000) - start.tv_usec;
	}
	printf("Difference %d.%6.6d\n", sec,usec);
	return(0);
}
