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
	unsigned short port;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	if (sctp_bind(fd, INADDR_LOOPBACK, 12345) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	port = sctp_get_local_port(fd);
	close(fd);
	
	if (port != 12345)
		return "Wrong port";
	else
		return NULL;
}

DEFINE_APITEST(bind, same_port_s_a_s_p)
{
	int fd1, fd2, result;
	
	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	if (sctp_bind(fd1, INADDR_LOOPBACK, 12345) < 0) {
		close(fd1);
		return strerror(errno);
	}
	
	if ((fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_bind(fd2, INADDR_LOOPBACK, 12345);
	
	close(fd1);
	close(fd2);
	
	if (result < 0)
		return NULL;
	else
		return "bind was successful";
}

DEFINE_APITEST(bind, duplicate_s_a_s_p)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	if (sctp_bind(fd, INADDR_LOOPBACK, 1234) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_bind(fd, INADDR_LOOPBACK, 1234);
	
	close(fd);
	
	if (result < 0)
		return NULL;
	else
		return "bind was successful";
}

DEFINE_APITEST(bind, refinement)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	if (sctp_bind(fd, INADDR_ANY, 1234) < 0) {
		close(fd);
		return strerror(errno);
	}

	result = sctp_bind(fd, INADDR_LOOPBACK, 1234);
	close(fd);
	
	if (result == 0)
		return "bind was successful";
	else
		return NULL;
}
