#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#define PORT 30002
#define SIZE_OF_MESSAGE (512 * 1024)
int cnt=0;
int pcnt=0, icnt=0, bcnt=0;

time_t up=0, down=0;
void
handle_notification(char *buffer)
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_shutdown_event *sse;
	char *str, *timemark;
	time_t now;
	struct tm *tmm;

	now = time(NULL);
	tmm = localtime(&now);
	timemark = asctime(tmm);
	snp = (union sctp_notification *)buffer;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {
		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			down = up = now;
			cnt = pcnt = bcnt = icnt = 0;
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			down = now;
			printf("\n");
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			printf("\n");
			up = now;
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
		if (up != down) {
			printf("%d seconds\n", (down - up));
		}
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
		down = now;
		printf("\nSCTP_SHUTDOWN_EVENT: assoc=0x%x - %s",
		       (uint32_t)sse->sse_assoc_id, timemark);
		printf("%d seconds\n", (down - up));
		break;
	default:
		break;
	} /* end switch(snp->sn_header.sn_type) */
}


int main(int argc, char **argv) 
{
	int fd, i;
	struct sockaddr_in saddr_in;
	char buffer[SIZE_OF_MESSAGE];
	int saddrlen;
	int msg_flags=0;
	int received;
	struct sockaddr_in bindaddr[10];
	char *baddr[10];
	int bndcnt=0;
	uint16_t port= htons(PORT);

	struct sctp_event_subscribe event;
	struct sctp_sndrcvinfo sri;
	while((i= getopt(argc,argv,"P:I:B:b:h:p:m:?")) != EOF)
	{
		switch(i) {
		case 'p':
			port = htons(strtol(optarg, NULL, 0));
			break;

		case 'b':
			if (bndcnt < 10) {
				baddr[bndcnt] = optarg;
				bndcnt++;
			}
			break;
		case '?':
		default:
			printf("Use %s [-p port -b bind -b bind]\n",
			       argv[0]);
			exit(-1);
			break;
		};
	}
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket");
		exit(-1);
	}

	if (bndcnt == 0) {
		saddr_in.sin_family = AF_INET;
		saddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		saddr_in.sin_port = port;
		if (bind(fd, (struct sockaddr *)&(saddr_in),sizeof(struct sockaddr_in)) < 0)  {
			perror("bind");
			exit (-1);
		}
	} else {
		memset(bindaddr, 0, sizeof(bindaddr));
		for(i=0; i<bndcnt; i++) {
			bindaddr[i].sin_len    = sizeof(struct sockaddr_in);
			bindaddr[i].sin_family = AF_INET;
			bindaddr[i].sin_port   = port;
			inet_pton(AF_INET, baddr[i], &bindaddr[i].sin_addr);
		}
		/* bindx */
		if (sctp_bindx(fd, (struct sockaddr *)bindaddr,
			       bndcnt, SCTP_BINDX_ADD_ADDR) != 0) {
			perror("bind failed");
			exit(0);
		}
	}

        if(listen(fd, 5)<0) {
            	perror("listen");
		exit (-1);
	}
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
