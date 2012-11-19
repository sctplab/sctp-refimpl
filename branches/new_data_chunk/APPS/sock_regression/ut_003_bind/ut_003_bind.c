/*-
 * Copyright (c) 2001-2007 by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2001-2007, by Michael Tuexen, tuexen@fh-muenster.de. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static int
sctp_bind(int fd, in_addr_t address, in_port_t port)
{
	struct sockaddr_in addr;
	
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = htonl(address);
	
	return (bind(fd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)));
}

static unsigned short
sctp_get_local_port(int fd)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	(void)getsockname(fd, (struct sockaddr *) &addr, &addr_len);
	return ntohs(addr.sin_port);
}

static int
sctp_enable_v6_only(int fd)
{
	const int on = 1;
	socklen_t length;
	
	length = (socklen_t)sizeof(int);
	return (setsockopt(fd, IPPROTO_IPV6, IPV6_BINDV6ONLY, &on, length));
}

/*
 * TEST-TITLE bind/port_s_a_s_p
 * TEST-DESCR: (port specifed adress specfied port )
 * TEST-DESCR: On a 1-1 socket, bind to
 * TEST-DESCR: a specified port and address and
 * TEST-DESCR: validate we get the port.
 */
static void
test_bind_port_s_a_s_p(void)
{
	int fd;
	unsigned short port;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	if (sctp_bind(fd, INADDR_LOOPBACK, 12345) < 0) {
		close(fd);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}
	
	port = sctp_get_local_port(fd);
	close(fd);
	
	if (port != 12345)
		errx(-1, "Wrong port");
}

/*
 * TEST-TITLE bind/v4tov6_s_a_s_p
 * TEST-DESCR: (specifed adress specfied port )
 * TEST-DESCR: On a 1-1 socket v6, bind to
 * TEST-DESCR: a specified port and address (v4) and
 * TEST-DESCR: validate we get the port.
 */
static void
test_bind_v4tov6_s_a_s_p(void)
{
	int fd;
	unsigned short port;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	if (sctp_bind(fd, INADDR_LOOPBACK, 12345) < 0) {
		close(fd);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}
	
	port = sctp_get_local_port(fd);
	close(fd);
	
	if (port != 12345)
		errx(-1, "Wrong port");
}

/*
 * TEST-TITLE bind/v4tov6_w_a_s_p
 * TEST-DESCR: (without adress specfied port )
 * TEST-DESCR: On a 1-1 socket v6, bind to
 * TEST-DESCR: a specified port and address set to v4 INADDR_ANY
 * TEST-DESCR: validate we get the port.
 */
static void
test_bind_v4tov6_w_a_s_p(void)
{
	int fd;
	unsigned short port;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	if (sctp_bind(fd, INADDR_ANY, 12345) < 0) {
		close(fd);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}
	
	port = sctp_get_local_port(fd);
	close(fd);
	
	if (port != 12345)
		errx(-1, "Wrong port");
}

/*
 * TEST-TITLE bind/v4tov6only_w_a
 * TEST-DESCR: (without adress)
 * TEST-DESCR: On a 1-1 socket v6 set for v6 only.
 * TEST-DESCR: Bind a specified port and address set to v4 INADDR_ANY
 * TEST-DESCR: validate we fail.
 */
static void
test_bind_v4tov6only_w_a(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
	
	if (sctp_enable_v6_only(fd) < 0) {
		close(fd);
		errx(-1, "sctp_enable_v6_only: %s", strerror(errno));
	}
	
	result = sctp_bind(fd, INADDR_ANY, 12345);
	close(fd);

	if (result == 0)
		errx(-1, "sctp_bind: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/v4tov6only_s_a
 * TEST-DESCR: (specified adress)
 * TEST-DESCR: On a 1-1 socket v6 set for v6 only.
 * TEST-DESCR: Bind a specified port and address set to v4 LOOPBACK
 * TEST-DESCR: validate we fail.
 */
static void
test_bind_v4tov6only_s_a(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
	
	if (sctp_enable_v6_only(fd) < 0) {
		close(fd);
		errx(-1, "sctp_enable_v6_only: %s", strerror(errno));
	}
	
	result = sctp_bind(fd, INADDR_LOOPBACK, 12345);
	close(fd);

	if (result == 0)
		errx(-1, "sctp_bind: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/same_port_s_a_s_p
 * TEST-DESCR: (specified adress specified port)
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and address. Then
 * TEST-DESCR: attempt to bind the same address on another
 * TEST-DESCR: socket, validate we fail.
 */
static void
test_bind_same_port_s_a_s_p(void)
{
	int fd1, fd2, result;
	
	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	if (sctp_bind(fd1, INADDR_LOOPBACK, 12345) < 0) {
		close(fd1);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}
	
	if ((fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));

	result = sctp_bind(fd2, INADDR_LOOPBACK, 12345);
	
	close(fd1);
	close(fd2);
	
	if (result != -1)
		errx(-1, "error expected.");
}

/*
 * TEST-TITLE bind/duplicate_s_a_s_p
 * TEST-DESCR: (specified adress specified port)
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and address. Then
 * TEST-DESCR: attempt to bind the same address/port
 * TEST-DESCR: on the same socket, validate we fail.
 */
static void
test_bind__duplicate_s_a_s_p(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));

	if (sctp_bind(fd, INADDR_LOOPBACK, 1234) < 0) {
		close(fd);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}
	
	result = sctp_bind(fd, INADDR_LOOPBACK, 1234);
	close(fd);
	
	if (result != -1)
		errx(-1, "error expected.");
}

/*
 * TEST-TITLE bind/refinement
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Bind a specified port and with address INADDR_ANY. 
 * TEST-DESCR: validate that binding the same socket to
 * TEST-DESCR: a more specific address (the loopback) fails.
 */
static void
test_bind_refinement(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));

	if (sctp_bind(fd, INADDR_ANY, 1234) < 0) {
		close(fd);
		errx(-1, "sctp_bind: %s", strerror(errno));
	}

	result = sctp_bind(fd, INADDR_LOOPBACK, 1234);
	close(fd);

	if (result == 0)
		errx(-1, "error expected.");
}

int
main(int argc, char *argv[])
{
	test_bind_port_s_a_s_p();
	test_bind_v4tov6_s_a_s_p();
	test_bind_v4tov6_w_a_s_p();
	test_bind_v4tov6only_w_a();
	test_bind_v4tov6only_s_a();
	test_bind_same_port_s_a_s_p();
	test_bind__duplicate_s_a_s_p();
	test_bind_refinement();

	printf("0");
	return EX_OK;
}
