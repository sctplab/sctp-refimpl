#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/sctp_os.h>

void sctp_init(void);
int userspace_bind(struct socket *, struct sockaddr *, int);
int userspace_listen(struct socket *, int );
void userspace_close(struct socket *);
ssize_t userspace_sctp_recvmsg(struct socket *, void *, size_t , struct sockaddr *, socklen_t *, struct sctp_sndrcvinfo *, int *);

#define BUFFER_SIZE (1<<16)
#define PORT 9
#define ADDR "192.168.1.15"

int main(int argc, char *argv[])
{
	struct socket *sock;
	int n, flags;
	struct sockaddr_in addr;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	char buffer[BUFFER_SIZE];
	struct sctp_event_subscribe event;
	
	sctp_init();

	if ((sock = userspace_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == NULL) {
		perror("userspace_socket");
	}

	memset((void *)&event, 1, sizeof(struct sctp_event_subscribe));
#if 0
	if (setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS,
		&event, sizeof(struct sctp_event_subscribe)) < 0) {
		perror("setsockopt");
	}
#endif
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(ADDR);
	if (userspace_bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("userspace_bind");
	}
	if (userspace_listen(sock, 1) < 0) {
		perror("userspace_listen");
	}

	while (1) {
		flags = 0;
		memset((void *)&addr, 0, sizeof(struct sockaddr_in));
		from_len = (socklen_t)sizeof(struct sockaddr_in);
		memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));

		n = userspace_sctp_recvmsg(sock, (void*)buffer, BUFFER_SIZE,
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
	
	/* FIXME: Why does this not return a value ? */  
	userspace_close(sock);
	return (0);
}
