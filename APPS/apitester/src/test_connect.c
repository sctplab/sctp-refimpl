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
 * TEST-TITLE connect/non_listen
 * TEST-DESCR: On a 1-1 socket, get two sockets.
 * TEST-DESCR: Neither should listen, attempt to
 * TEST-DESCR: connect one to the other. This should fail.
 */
DEFINE_APITEST(connect, non_listen)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;

	fds = sctp_one2one(0, 0, 0);
	if (fds  < 0)
		return strerror(errno);

	addr_len = (socklen_t)sizeof(struct sockaddr_in);		
	if (getsockname (fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	fdc = sctp_one2one(0, 0, 0);
	if (fdc  < 0) {
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

/*
 * TEST-TITLE connect/non_listen
 * TEST-DESCR: On a 1-1 socket, get two sockets.
 * TEST-DESCR: One should listen, attempt to
 * TEST-DESCR: connect one to the other. This should work..
 */
DEFINE_APITEST(connect, listen)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fds = sctp_one2one(0, 1, 0);
	if (fds  < 0)
		return strerror(errno);
		
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		return strerror(errno);
	}
	
	fdc = sctp_one2one(0, 0, 0);
	if (fdc  < 0) {
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

/*
 * TEST-TITLE connect/self_non_listen
 * TEST-DESCR: On a 1-1 socket, get a socket, no listen.
 * TEST-DESCR: Attempt to connect to itself. 
 * TEST-DESCR: This should fail, since we are not listening.
 */
DEFINE_APITEST(connect, self_non_listen)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fd = sctp_one2one(0, 0, 0);
	if (fd  < 0)
		return strerror(errno);

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
		return NULL;
	else
		return "connect was successful";


}
/*
 * TEST-TITLE connect/self_listen
 * TEST-DESCR: On a 1-1 socket, get a socket, and listen.
 * TEST-DESCR: Attempt to connect to itself. 
 * TEST-DESCR: This should fail, since we are not allowed to
 * TEST-DESCR: connect when listening.
 */
DEFINE_APITEST(connect, self_listen)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fd = sctp_one2one(0, 1, 0);
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
