#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define DISCARD_PORT 30002
#define ADDR "10.1.1.3"
#define SIZE_OF_MESSAGE    34000
#define NUMBER_OF_MESSAGES 1000000
#define HAVE_SIN_LEN

int main(int argc, char **argv) 
{
	unsigned int i;
	int fd, ret;
	struct sockaddr_in addr;
	char buffer[SIZE_OF_MESSAGE];
	int flags;
	char *addrnm = ADDR;
	memset((void *)buffer, 'A', SIZE_OF_MESSAGE);
	
	if (argc > 1) {
		addrnm = argv[1];
	}
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		perror("socket");
	
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len    = sizeof(struct sockaddr_in);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(DISCARD_PORT);
	inet_pton(AF_INET, addrnm, &addr.sin_addr);
	
	for (i = 0; i <  NUMBER_OF_MESSAGES; i++) {
		flags=SCTP_PR_SCTP_RTX;
		printf("i=%d, flags=%d\r",i,flags);
		fflush(stdout);
		errno = 0;
		if ((ret = sctp_sendmsg(fd,
		                 (const void *)buffer, SIZE_OF_MESSAGE,
	                         (struct sockaddr *)&addr, sizeof(struct sockaddr_in),
		                 htonl(333),       /* PPID */
		                 flags,            /* flags */
		                 1,                /* stream identifier */
		                 0,                /* Max number of rtx */
		                 0                 /* context */
			     )) <= 0) {
			perror("sctp_sendmsg");
			printf("ret=%d errno:%d\n", ret, errno);
			break;
		}
	}
	printf("\n");
	if (close(fd) < 0)
		perror("close");
	
	return(0);
}
