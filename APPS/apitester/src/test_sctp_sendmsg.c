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

DEFINE_APITEST(sctp_sendmsg, c_p_c_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);

	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, c_p_c_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);

	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_c_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);

	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_c_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);

	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, c_p_w_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, c_p_w_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_w_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_w_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_c_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(1);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_c_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(1);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, c_p_b_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, c_p_b_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_b_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_b_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_b_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, w_p_b_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_w_a)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, b_p_w_a_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, non_null_zero)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, 0, 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, non_null_zero_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr, 0, 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, null_zero)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL, 0, 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, null_zero_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL, 0, 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return strerror(errno);
	} else {
		return NULL;
	}
}

DEFINE_APITEST(sctp_sendmsg, null_non_zero)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL, sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}

DEFINE_APITEST(sctp_sendmsg, null_non_zero_over)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		return strerror(errno);
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL, sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0) {
		return NULL;
	} else {
		return "sctp_sendmsg was successful";
	}
}
