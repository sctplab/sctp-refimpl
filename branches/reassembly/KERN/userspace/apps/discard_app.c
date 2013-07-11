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
#include <netinet/sctp_pcb.h>

void sctp_init(void);
int userspace_connect(struct socket *, struct sockaddr *, int);
int userspace_bind(struct socket *, struct sockaddr *, int);
int userspace_listen(struct socket *, int );
void userspace_close(struct socket *);
int userspace_setsockopt(struct socket *, int, int, const void *, socklen_t);

ssize_t userspace_sctp_sendmsg(struct socket *, const void *, size_t, struct sockaddr *, socklen_t, u_int32_t, u_int32_t, u_int16_t, u_int32_t, u_int32_t);
ssize_t userspace_sctp_recvmsg(struct socket *, void *, size_t , struct sockaddr *, socklen_t *, struct sctp_sndrcvinfo *, int *);

#define BUFFER_SIZE (1024)
#define DISCARD_PORT 9
#define NUMBER_OF_MESSAGES 10

static void *
discard_server(void *arg)
{
	struct socket *sock;
	int n, flags;
	struct sockaddr_in addr;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	char buffer[BUFFER_SIZE];
	struct sctp_event_subscribe event;
	
	if ((sock = userspace_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == NULL) {
		perror("userspace_socket");
	}

	memset((void *)&event, 1, sizeof(struct sctp_event_subscribe));
	if (userspace_setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(struct sctp_event_subscribe)) < 0) {
		perror("setsockopt");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(DISCARD_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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
	return NULL;
}

int main(int argc, char *argv[])
{

	pthread_t tid;
	struct socket *sock;
	unsigned int i;
	struct sockaddr_in addr;
	char buffer[BUFFER_SIZE];

	sctp_init();

	pthread_create(&tid, NULL, &discard_server, (void *)NULL);
	sleep(1);
	
	memset((void *)buffer, 'A', BUFFER_SIZE);
	
	if ((sock = userspace_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == NULL) {
		perror("userspace_socket");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (userspace_bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("userspace_bind");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(DISCARD_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (userspace_connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
		perror("userspace_connect");

	for (i = 0; i <  NUMBER_OF_MESSAGES; i++) {
		if (userspace_sctp_sendmsg(sock, (const void *)buffer, BUFFER_SIZE, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), htonl(3), 0, 2, 0, 0) < 0) {
			perror("userspace_sctp_sendmsg");
		}
	}
	userspace_close(sock);
	sleep(1);
	return(0);
}
