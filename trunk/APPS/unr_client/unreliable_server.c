#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PORT 30002
#define SIZE_OF_MESSAGE (512 * 1024)
int cnt=0;
int pcnt=0, icnt=0, bcnt=0;

void
handle_notification(char *buffer)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_shutdown_event *sse;
	char *str, *timemark;
	time_t now;

	now = time(NULL);
	timemark = localtime(&now);

	snp = (union sctp_notification *)buffer;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {
		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			cnt = pcnt = bcnt = icnt = 0;
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			printf("\n");
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			printf("\n");
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			printf("\n");
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CANT START ASSOC";
			break;
		default:
			str = "UNKNOWN";
		} /* end switch(sac->sac_state) */
		
		printf("SCTP_ASSOC_CHANGE: %s, assoc=0x%x - %s",
		       str,
		       (uint32_t)sac->sac_assoc_id, timemark);
		break;
	case SCTP_PEER_ADDR_CHANGE:
		break;
	case SCTP_REMOTE_ERROR:
		break;
	case SCTP_AUTHENTICATION_EVENT:
		break;
	case SCTP_SENDER_DRY_EVENT:
		break;
	case SCTP_STREAM_RESET_EVENT:
		break;
	case SCTP_SEND_FAILED:
		break;
	case SCTP_ADAPTION_INDICATION:
		break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
		break;

	case SCTP_SHUTDOWN_EVENT:
                sse = &snp->sn_shutdown_event;
		printf("\nSCTP_SHUTDOWN_EVENT: assoc=0x%x - %s",
		       (uint32_t)sse->sse_assoc_id, timemark);
		break;
	default:
		break;
	} /* end switch(snp->sn_header.sn_type) */
}


int main() 
{
	int fd;
	struct sockaddr_in saddr_in;
	char buffer[SIZE_OF_MESSAGE];
	int saddrlen;
	int msg_flags=0;
	int received;
	struct sctp_event_subscribe event;
	struct sctp_sndrcvinfo sri;
    	struct sctp_initmsg initparams;

    	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
        	perror("Couldn't create socket");

    	bzero(&(saddr_in), sizeof(struct sockaddr_in));
    	saddr_in.sin_family = AF_INET;
    	saddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    	saddr_in.sin_port = htons(PORT);

    	bzero(&initparams, sizeof(initparams));
    	if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &initparams, sizeof(initparams))<0)
        	perror("SCTP_NODELAY");

        if (bind(fd, (struct sockaddr *)&(saddr_in),sizeof(struct sockaddr_in)) < 0) 
            	perror("bind");

        if(listen(fd, 5)<0)
            	perror("listen");
	memset(&event, 0, sizeof(event));
	event.sctp_association_event = 1;
	event.sctp_shutdown_event = 1;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		perror("Can't do SET_EVENTS socket option!");
	}
	
	bzero(&saddr_in, sizeof(struct sockaddr_in));
	bzero(&sri, sizeof(struct sctp_sndrcvinfo));
	saddrlen=sizeof(struct sockaddr_in);
	while(1){
		received=sctp_recvmsg(fd,                // socket descriptor
			buffer,                        // message received
	                SIZE_OF_MESSAGE,                             // bytes read
	                (struct sockaddr *) &saddr_in,       // remote address
	                (socklen_t *)&saddrlen,             // value-result parameter (remote address len)
	                &sri, &msg_flags              // struct sctp_sndrcvinfo and flags received
		       );
		cnt++;
		if (msg_flags & MSG_NOTIFICATION) {
			handle_notification(buffer);
			continue;
		}
		if (buffer[0] == 'I') {
			icnt++;
		} else if (buffer[0] == 'P') {
			pcnt++;
		} else if (buffer[0] == 'B') {
			bcnt++;
		}
		printf("%d I:%d P:%d B:%d\r",
		       cnt, icnt, pcnt, bcnt);
		fflush(stdout);
		if (received <= 0)
			break;
	}
	printf("\n");

	if (close(fd) < 0)
		perror("close");

    	return 0;
}
