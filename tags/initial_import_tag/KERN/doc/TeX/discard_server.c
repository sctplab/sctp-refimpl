#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE (1<<16)
#define PORT 9
#define ADDR "0.0.0.0"

int main(int argc, char *argv[]) {
	int fd, n, flags;
	struct sockaddr_in addr;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	char buffer[BUFFER_SIZE];
	struct sctp_event_subscribe event;
      
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		perror("socket");
	}
	memset((void *)&event, 1, sizeof(struct sctp_event_subscribe));
	if (setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS,
		&event, sizeof(struct sctp_event_subscribe)) < 0) {
		perror("setsockopt");
	}
  
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(ADDR);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
    	}
	if (listen(fd, 1) < 0) {
		perror("listen");
	}
	
	while (1) {
		flags = 0;
		memset((void *)&addr, 0, sizeof(struct sockaddr_in));
		from_len = (socklen_t)sizeof(struct sockaddr_in);
		memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));
    
		n = sctp_recvmsg(fd, (void*)buffer, BUFFER_SIZE,
		                 (struct sockaddr *)&addr, &from_len,
	                         &sinfo, &flags);
                     
		if (flags & MSG_NOTIFICATION) {
			printf("Notification received.\n");
		} else {
			printf("Msg of length %d received from %s:%u on stream %d, PPID %d.\n",
			       n, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),
			       sinfo.sinfo_stream, ntohl(sinfo.sinfo_ppid));
		}
	}
  
	if (close(fd) < 0) {
		perror("close");
	}
  
	return (0);
}
