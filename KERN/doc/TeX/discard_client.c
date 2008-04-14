#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PORT 9
#define ADDR "127.0.0.1"
#define SIZE_OF_MESSAGE    1000
#define NUMBER_OF_MESSAGES 10000
#define PPID 1234

int main(int argc, char *argv[]) {
	unsigned int i;
	int fd;
	struct sockaddr_in addr;
	char buffer[SIZE_OF_MESSAGE];
	struct sctp_status status;
	struct sctp_initmsg init;
	socklen_t opt_len;
	
	memset((void *)buffer, 'A', SIZE_OF_MESSAGE);
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		perror("socket");
	}

	memset((void *)&init, 0, sizeof(struct sctp_initmsg));
	init.sinit_num_ostreams = 2048;
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG,
	               &init, (socklen_t)sizeof(struct sctp_initmsg)) < 0) {
		perror("setsockopt");
	}
	
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(ADDR);	

	if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("connect");
	}
	
	memset((void *)&status, 0, sizeof(struct sctp_status));
	opt_len = (socklen_t)sizeof(struct sctp_status);
	if (getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, &status, &opt_len) < 0) {
		perror("getsockopt");
	}
	
	for (i = 0; i <  NUMBER_OF_MESSAGES; i++) {
		if (sctp_sendmsg(fd, (const void *)buffer, SIZE_OF_MESSAGE,
		                 NULL, 0,
		                 htonl(PPID), 0, i % status.sstat_outstrms,
		                 0, 0) < 0) {
			perror("send");
		}
	}
	
	if (close(fd) < 0) {
		perror("close");
	}
	return(0);
}
