#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/sctp.h>
#include "sctp_utilities.h"
#include "api_tests.h"

DEFINE_APITEST(bind, port_s_a_s_p)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return strerror(errno);
	}

	close(fd);
	
	if (ntohs(addr.sin_port) != 12345)
		return "Wrong port";
	else
		return NULL;
}

DEFINE_APITEST(bind, same_port_s_a_s_p)
{
	int fd1, fd2, n;
	struct sockaddr_in addr;
	
	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd1, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd1);
		return strerror(errno);
	}
	
	if ((fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	n = bind(fd2, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in));
	
	close(fd1);
	close(fd2);
	
	if (n < 0)
		return NULL;
	else
		return "bind was successful";
}
