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

/*
 * TEST-TITLE bind/port_s_a_s_p
 * TEST-DESCR: (port specifed adress specfied port )
 * TEST-DESCR: On a 1-1 socket, bind to
 * TEST-DESCR: a specified port and address and
 * TEST-DESCR: validate we get the port.
 */
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

/*
 * TEST-TITLE bind/v4tov6_s_a_s_p
 * TEST-DESCR: (specifed adress specfied port )
 * TEST-DESCR: On a 1-1 socket v6, bind to
 * TEST-DESCR: a specified port and address (v4) and
 * TEST-DESCR: validate we get the port.
 */
DEFINE_APITEST(bind, v4tov6_s_a_s_p)
{
	int fd;
	unsigned short port;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
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

/*
 * TEST-TITLE bind/v4tov6_w_a_s_p
 * TEST-DESCR: (without adress specfied port )
 * TEST-DESCR: On a 1-1 socket v6, bind to
 * TEST-DESCR: a specified port and address set to v4 INADDR_ANY
 * TEST-DESCR: validate we get the port.
 */
DEFINE_APITEST(bind, v4tov6_w_a_s_p)
{
	int fd;
	unsigned short port;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	if (sctp_bind(fd, INADDR_ANY, 12345) < 0) {
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

/*
 * TEST-TITLE bind/v4tov6only_w_a
 * TEST-DESCR: (without adress)
 * TEST-DESCR: On a 1-1 socket v6 set for v6 only.
 * TEST-DESCR: Bind a specified port and address set to v4 INADDR_ANY
 * TEST-DESCR: validate we fail.
 */
DEFINE_APITEST(bind, v4tov6only_w_a)
{
	int fd, result;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	if (sctp_enable_v6_only(fd)	< 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_bind(fd, INADDR_ANY, 12345);
	close(fd);
		
	if (result)
		return NULL;
	else
		return "bind() was successful";
}

/*
 * TEST-TITLE bind/v4tov6only_s_a
 * TEST-DESCR: (specified adress)
 * TEST-DESCR: On a 1-1 socket v6 set for v6 only.
 * TEST-DESCR: Bind a specified port and address set to v4 LOOPBACK
 * TEST-DESCR: validate we fail.
 */
DEFINE_APITEST(bind, v4tov6only_s_a)
{
	int fd, result;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	if (sctp_enable_v6_only(fd)	< 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_bind(fd, INADDR_LOOPBACK, 12345);
	close(fd);
		
	if (result)
		return NULL;
	else
		return "bind() was successful";
}

/*
 * TEST-TITLE bind/same_port_s_a_s_p
 * TEST-DESCR: (specified adress specified port)
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and address. Then
 * TEST-DESCR: attempt to bind the same address on another
 * TEST-DESCR: socket, validate we fail.
 */
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

/*
 * TEST-TITLE bind/duplicate_s_a_s_p
 * TEST-DESCR: (specified adress specified port)
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and address. Then
 * TEST-DESCR: attempt to bind the same address/port
 * TEST-DESCR: on the same socket, validate we fail.
 */
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

/*
 * TEST-TITLE bind/refinement
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and with address INADDR_ANY. 
 * TEST-DESCR: validate that binding the same socket to
 * TEST-DESCR: a more specific address (the loopback) fails.
 */
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
