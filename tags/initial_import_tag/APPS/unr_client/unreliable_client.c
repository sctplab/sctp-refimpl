#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#define SIZE_OF_MESSAGE (512 * 1024)
#define SIZE_OF_I_FRAME (51 * 1024)
#define SIZE_OF_P_FRAME (9 * 1024)
#define SIZE_OF_B_FRAME (3 * 1024)

#define DISCARD_PORT 30002
int fd;

int iframe_rel=0;
int pframe_rel=0;
int bframe_rel=0;
int added_flags=0;
struct sockaddr_in addr;
char buffer[SIZE_OF_MESSAGE];
int icount=0, bcount=0, pcount=0;
int
send_iframe()
{
	int ret;
	buffer[0] = 'I';
	if ((ret = sctp_sendmsg(fd,
				(const void *)buffer, SIZE_OF_I_FRAME,
				(struct sockaddr *)&addr, sizeof(struct sockaddr_in),
				htonl(73),       /* PPID */
				(added_flags |SCTP_PR_SCTP_RTX), /* flags */
				1,                /* stream identifier */
				iframe_rel,                /* Max number of rtx */
				0                 /* context */
		     )) < 0) {
		perror("sctp_sendmsg - I");
	} else {
		icount++;
	}
	printf("I:%d\r", icount);
	fflush(stdout);
	return(ret);
}

int
send_pframe()
{
	int ret;
	buffer[0] = 'P';
 	if ((ret = sctp_sendmsg(fd,
				(const void *)buffer, SIZE_OF_P_FRAME,
				(struct sockaddr *)&addr, sizeof(struct sockaddr_in),
				htonl(80),       /* PPID */
				(added_flags |SCTP_PR_SCTP_RTX), /* flags */
				1,                /* stream identifier */
				pframe_rel,                /* Max number of rtx */
				0                 /* context */
		     )) < 0) {
		perror("sctp_sendmsg - P");
	} else {
		pcount++;
	}
	printf("I:%d P:%d\r", icount, pcount);
	fflush(stdout);
	return(ret);
}

int
send_bframe()
{
	int ret;
	buffer[0] = 'B';
	if ((ret = sctp_sendmsg(fd,
				(const void *)buffer, SIZE_OF_P_FRAME,
				(struct sockaddr *)&addr, sizeof(struct sockaddr_in),
				htonl(66),       /* PPID */
				(added_flags |SCTP_PR_SCTP_RTX), /* flags */
				1,                /* stream identifier */
				bframe_rel,                /* Max number of rtx */
				0                 /* context */
		     )) < 0) {
		perror("sctp_sendmsg - B");
	} else {
		bcount++;
	}
	printf("I:%d P:%d B:%d\r", icount, pcount, bcount);
	fflush(stdout);
	return(ret);
}



int main(int argc, char **argv) 
{
	unsigned int i, j, k, ret;
	int num_messages=24;
	struct sockaddr_in bindaddr[10];
	struct sockaddr_in saddr_in;
	struct sctp_sndrcvinfo sri;
	int msg_flags;
	socklen_t saddrlen;
	char *baddr[10];
	struct sctp_event_subscribe event;
	int bndcnt=0;
	char *toaddr = NULL;
	int port = htons(DISCARD_PORT);

	while((i= getopt(argc,argv,"P:I:B:b:h:p:m:s?")) != EOF)
	{
		switch(i) {
		case 's':
			added_flags = SCTP_SACK_IMMEDIATELY;
			break;
		case 'p':
			port = htons(strtol(optarg, NULL, 0));
			break;
		case 'P':
			pframe_rel = strtol(optarg, NULL, 0);
			break;
		case 'I':
			iframe_rel = strtol(optarg, NULL, 0);
			break;
		case 'B':
			bframe_rel = strtol(optarg, NULL, 0);
			break;


		case 'b':
			if (bndcnt < 10) {
				baddr[bndcnt] = optarg;
				bndcnt++;
			}
			break;
		case 'h':
			toaddr = optarg;
			break;
		case 'm':
			num_messages = strtol(optarg, NULL, 0);
			break;
		case '?':
		default:
			goto use;
			break;
		};
	}
	if (toaddr == NULL) {
	use:
		printf("use %s -h to-addr-in-dot-notation [-m message-cnt -b bindaddr]\n",
		       argv[0]);
		printf("                    [-P rel -I rel -B rel]\n");
		exit(0);
	}
	memset((void *)buffer, 'A', SIZE_OF_MESSAGE);
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		perror("socket");
	

	i = 1;
    	if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &i, sizeof(int))<0) {
        	perror("SCTP_NODELAY");
	}
	if (bndcnt) {
		/* prepare */
		memset(bindaddr, 0, sizeof(bindaddr));
		for(i=0; i<bndcnt; i++) {
			bindaddr[i].sin_len    = sizeof(struct sockaddr_in);
			bindaddr[i].sin_family = AF_INET;
			bindaddr[i].sin_port   = 0;
			inet_pton(AF_INET, baddr[i], &bindaddr[i].sin_addr);
		}
		/* bindx */
		if (sctp_bindx(fd, (struct sockaddr *)bindaddr,
			       bndcnt, SCTP_BINDX_ADD_ADDR) != 0) {
			perror("bind failed");
			exit(0);
		}
	}
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_len    = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	addr.sin_port   = port;
	inet_pton(AF_INET, toaddr, &addr.sin_addr);
	for (i = 0; i <  num_messages; i++) {
		if (send_iframe() < 0) {
			exit(-4);
		}
		if (send_pframe() < 0) {
			exit(-3);
		}
		for (j=0; j<5; j++) {
			if (send_pframe() < 0) {
				exit(-2);
			}
			for (k=0; k<6; k++) {
				if (send_bframe() < 0) {
					exit(-1);
				}
			}
		}
	}
	/* Ok now we want to know when shutting down */
	memset(&event, 0, sizeof(event));
	event.sctp_association_event = 1;
	event.sctp_shutdown_event = 1;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		perror("Can't do SET_EVENTS socket option!");
	}

	buffer[0] = 'E';
again:
	msg_flags = 0;
	ret = sctp_sendmsg(fd,
			   (const void *)buffer, 1,
			   (struct sockaddr *)&addr, sizeof(struct sockaddr_in),
			   0,       /* PPID */
			   (SCTP_EOF|SCTP_PR_SCTP_RTX), /* flags */
			   1,                /* stream identifier */
			   0,                /* Max number of rtx */
			   0                 /* context */
		);
	saddrlen = sizeof(saddr_in);
	ret = sctp_recvmsg(fd,                // socket descriptor
			   buffer,                        // message received
			   SIZE_OF_MESSAGE,                             // bytes read
			   (struct sockaddr *) &saddr_in,       // remote address
			   (socklen_t *)&saddrlen,             // value-result parameter (remote address len)
			   &sri, &msg_flags              // struct sctp_sndrcvinfo and flags received
		);
	if (msg_flags & MSG_NOTIFICATION) {
		union sctp_notification *snp;
		snp = (union sctp_notification *)buffer;
		if (snp->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
			struct sctp_assoc_change *sac;
			sac = &snp->sn_assoc_change;
			if (sac->sac_state == SCTP_SHUTDOWN_COMP) {
				goto done;
			} else {
				printf("assoc event was %d not shutdown\n", sac->sac_state);
				goto again;
			}
		}else if (snp->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
			goto done;
		} else {
			printf("Got notification %d not shutdown event continue\n");
			goto again;
		}
	} else {
		printf("Got a non-notification message . ignore and re-read ret:%d\n", ret);
		goto again;
	}
done:
	if (close(fd) < 0)
		perror("close");
	printf("\n");
	return(0);
}
