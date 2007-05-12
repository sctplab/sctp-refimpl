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

DEFINE_APITEST(connect, non_listen)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	if ((fds = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fds, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	if ((fdc = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fds);
		return strerror(errno);
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fdc, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fdc);
		close(fds);
		return strerror(errno);
	}
	
	n = connect(fdc, (const struct sockaddr *)&addr, addr_len);

	close(fds);
	close(fdc);
	
	if (n < 0)
		return NULL;
	else
		return "connect was successful";
}

DEFINE_APITEST(connect, listen)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	if ((fds = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fds, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	if (listen(fds, 1) < 0) {
		close(fds);
		return strerror(errno);
	}
		
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	if ((fdc = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	n = connect(fdc, (const struct sockaddr *)&addr, addr_len);
	close(fds);
	close(fdc);
	if (n < 0)
		return strerror(errno);
	else
		return NULL;
}


DEFINE_APITEST(connect, self_non_listen)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
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
	
	n = connect(fd, (const struct sockaddr *)&addr, addr_len);
	close(fd);

	/*  This really depends on when connect() returns. On the
	 *  reception of the INIT-ACK or the COOKIE-ACK
	 */
	if (n < 0)
		return strerror(errno);
	else
		return NULL;

}

DEFINE_APITEST(connect, self_listen)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	if (listen(fd, 1) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname(fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	n = connect(fd, (const struct sockaddr *)&addr, addr_len);
	close(fd);
	if (n < 0)
		return NULL;
	else
		return "connect was successful";

}
