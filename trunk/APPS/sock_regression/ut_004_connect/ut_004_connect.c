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

int
sctp_one2one(unsigned short port, int should_listen, int bindall)
{
	int fd;
	struct sockaddr_in addr;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) 
		return -1;
	
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(port);
	if (bindall)
		addr.sin_addr.s_addr = 0;
	else
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}
	if(should_listen) {
		if (listen(fd, 1) < 0) {
			close(fd);
			return -1;
		}
	}
	return(fd);
}

/*
 * TEST-TITLE connect/non_listen
 * TEST-DESCR: On a 1-1 socket, get two sockets.
 * TEST-DESCR: Neither should listen, attempt to
 * TEST-DESCR: connect one to the other. This should fail.
 */
static void
test_connect_non_listen(void)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;

	fds = sctp_one2one(0, 0, 0);
	if (fds < 0)
		errx(-1, "sctp_one2one: %s", strerror(errno));

	addr_len = (socklen_t)sizeof(struct sockaddr_in);		
	if (getsockname(fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		errx(-1, "getsockname: %s", strerror(errno));
	}
	
	fdc = sctp_one2one(0, 0, 0);
	if (fdc  < 0) {
		close(fds);
		errx(-1, "sctp_one2one: %s", strerror(errno));
	}
	n = connect(fdc, (const struct sockaddr *)&addr, addr_len);

	close(fds);
	close(fdc);
	
	if (!(n < 0))
		errx(-1, "connect: connect was successful");
}

/*
 * TEST-TITLE connect/non_listen
 * TEST-DESCR: On a 1-1 socket, get two sockets.
 * TEST-DESCR: One should listen, attempt to
 * TEST-DESCR: connect one to the other. This should work..
 */
static void
test_connect_listen(void)
{
	int fdc, fds, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fds = sctp_one2one(0, 1, 0);
	if (fds  < 0)
		errx(-1, "sctp_one2on2: %s", strerror(errno));
		
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname(fds, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fds);
		errx(-1, "getsockname: %s", strerror(errno));
	}
	
	fdc = sctp_one2one(0, 0, 0);
	if (fdc  < 0) {
		close(fds);
		errx(-1, "sctp_one2one: %s", strerror(errno));
	}
	
	n = connect(fdc, (const struct sockaddr *)&addr, addr_len);
	close(fds);
	close(fdc);
	if (n < 0)
		errx(-1, "connect: %s", strerror(errno));
}

/*
 * TEST-TITLE connect/self_non_listen
 * TEST-DESCR: On a 1-1 socket, get a socket, no listen.
 * TEST-DESCR: Attempt to connect to itself. 
 * TEST-DESCR: This should fail, since we are not listening.
 */
static void
test_connect_self_non_listen(void)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fd = sctp_one2one(0, 0, 0);
	if (fd  < 0)
		errx(-1, "sctp_one2one: %s", strerror(errno));

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		errx(-1, "getsockname: %s", strerror(errno));
	}
	
	n = connect(fd, (const struct sockaddr *)&addr, addr_len);
	close(fd);

	/*  This really depends on when connect() returns. On the
	 *  reception of the INIT-ACK or the COOKIE-ACK
	 */
	if (!(n < 0))
		errx(-1, "connect was successful");


}
/*
 * TEST-TITLE connect/self_listen
 * TEST-DESCR: On a 1-1 socket, get a socket, and listen.
 * TEST-DESCR: Attempt to connect to itself. 
 * TEST-DESCR: This should fail, since we are not allowed to
 * TEST-DESCR: connect when listening.
 */
static void
test_connect_self_listen(void)
{
	int fd, n;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	fd = sctp_one2one(0, 1, 0);
	if (fd  < 0)
		errx(-1, "sctp_one2one: %s", strerror(errno));
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname(fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		errx(-1, "getsockname: %s", strerror(errno));
	}
	
	n = connect(fd, (const struct sockaddr *)&addr, addr_len);
	close(fd);
	if (!(n < 0))
		errx(-1, "connect: connect was successful");
}

int
main(int argc, char *argv[])
{
	test_connect_non_listen();
	test_connect_listen();
	test_connect_self_non_listen();
	test_connect_self_listen();

	printf("0");
	return EX_OK;
}
