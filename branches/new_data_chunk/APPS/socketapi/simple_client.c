#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>

#define LIMIT 0xfffffff

int main(int argc, char **argv)
{
	int fd, n;
	struct sockaddr_in server_addr, client_addr;
	int len;
	char buf[1<<16];
	unsigned long data_read = 0;
    time_t now;
	
#ifdef USE_SCTP	
	fd  = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
#else
	fd  = socket(AF_INET, SOCK_STREAM, 0);
#endif

	memset(&client_addr, 0, sizeof(client_addr));
#ifdef HAVE_SIN_LEN
	client_addr.sin_len         = sizeof(client_addr);
#endif
	client_addr.sin_family      = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port        = htons(0);
	
	if (bind(fd, (const struct sockaddr *) &client_addr, sizeof(client_addr)) != 0)
		perror("bind");
	
	memset(&server_addr, 0, sizeof(server_addr));
#ifdef HAVE_SIN_LEN
	server_addr.sin_len         = sizeof(server_addr);
#endif
	server_addr.sin_family      = AF_INET;
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
	server_addr.sin_port        = htons(19);
	
	if (connect(fd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) != 0)
		perror("connect");
	
    time(&now);
    printf("It is now %s", ctime(&now));
	while (data_read < LIMIT) {
		n = read(fd, (void *) buf, sizeof(buf));
		data_read = data_read + n;
	}
    time(&now);
    printf("It is now %s", ctime(&now));
	if (close(fd) < 0)
		perror("close");
}
