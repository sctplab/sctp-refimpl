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

#define REMOTE_PORT 9
#define REMOTE_ADDR "192.168.1.193"
#define LOCAL_PORT 0
#define LOCAL_ADDR "192.168.1.194"
#define SIZE_OF_MESSAGE    1000
#define NUMBER_OF_MESSAGES 1000

void sctp_init(void);
int userspace_bind(struct socket *, struct sockaddr *, int);
int userspace_connect(struct socket *, struct sockaddr *, int);
ssize_t userspace_sctp_sendmsg(struct socket *, const void *, size_t, struct sockaddr *, socklen_t, u_int32_t, u_int32_t, u_int16_t, u_int32_t, u_int32_t);
void userspace_close(struct socket *);

int main() {
	struct socket *sock;
	unsigned int i;
	struct sockaddr_in addr;
	char buffer[SIZE_OF_MESSAGE];

	sctp_init();
	SCTP_BASE_SYSCTL(sctp_udp_tunneling_for_client_enable) = 0;
	SCTP_BASE_SYSCTL(sctp_debug_on) = 0xffffffff;
	memset((void *)buffer, 'A', SIZE_OF_MESSAGE);

	if ((sock = userspace_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == NULL) {
		perror("userspace_socket");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len = sizeof(struct sockaddr_in);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOCAL_PORT);
	inet_pton(AF_INET, LOCAL_ADDR, &addr.sin_addr);
	if (userspace_bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("userspace_bind");
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
	addr.sin_len = sizeof(struct sockaddr_in);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(REMOTE_PORT);
	inet_pton(AF_INET, REMOTE_ADDR, &addr.sin_addr);

	if (userspace_connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
		perror("userspace_connect");

	for (i = 0; i <  NUMBER_OF_MESSAGES; i++) {
		if (userspace_sctp_sendmsg(sock, (const void *)buffer, SIZE_OF_MESSAGE, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0) < 0) {
			perror("userspace_sctp_sendmsg");
		}
	}
	userspace_close(sock);
	sleep(10);
	return(0);
}
