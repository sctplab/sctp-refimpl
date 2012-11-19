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
sctp_one2one(unsigned short port, int should_listen, int bindall)
{
	int fd;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) 
		return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_len         = sizeof(struct sockaddr_in);
	addr.sin_port        = htons(port);
	if (bindall) {
		addr.sin_addr.s_addr = 0;
	} else {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

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

static int
sctp_socketpair(int *fds, int bindall)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	/* Get any old port, but listen */
	fd = sctp_one2one(0, 1, bindall);
	if (fd  < 0)
		return -1;

	/* Get any old port, but no listen */
	fds[0] = sctp_one2one(0, 0, bindall);
	if (fds[0] < 0) {
		close(fd);
		return -1;
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	if (bindall)
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (connect(fds[0], (struct sockaddr *) &addr, addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	if ((fds[1] = accept(fd, NULL, 0)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	close(fd);
	return 0;
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_c_a
 * TEST-DESCR: (correct port correct address).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Validate that no error
 * TEST-DESCR: occurs when sending with an address and
 * TEST-DESCR: proper size (tolen).
 */
static void
test_sctp_sendmsg_c_p_c_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);

	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_c_a_over
 * TEST-DESCR: (correct port correct address override).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Validate that no error
 * TEST-DESCR: occurs when sending with an address and
 * TEST-DESCR: proper size (tolen), add the override flag.
 */
static void
test_sctp_sendmsg_c_p_c_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);

	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_c_a
 * TEST-DESCR: (without port correct address).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Validate that no error
 * TEST-DESCR: occurs when sending with an port
 * TEST-DESCR: that is set to zero, but sized correctly.
 */
static void
test_sctp_sendmsg_w_p_c_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);

	addr.sin_port = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "%s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_c_a_over
 * TEST-DESCR: (without port correct address override).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Validate that no error
 * TEST-DESCR: occurs when sending with an address 
 * TEST-DESCR: that is set with a port of zero, but sized correctly and
 * TEST-DESCR: with the override address flag set.
 */
static void
test_sctp_sendmsg_w_p_c_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);

	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_w_a
 * TEST-DESCR: (correct port without address).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. And send to INADDR_ANY
 * TEST-DESCR: with no port set. Validate it succeeds.
 */
static void
test_sctp_sendmsg_c_p_w_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_w_a_over
 * TEST-DESCR: (correct port without address override).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. And send to INADDR_ANY
 * TEST-DESCR: with port set and address override
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_c_p_w_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_w_a
 * TEST-DESCR: (with port without address).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. And send to INADDR_ANY
 * TEST-DESCR: with no port set and no address override
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_w_p_w_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_w_a_over
 * TEST-DESCR: (with port without address and override).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. And send to INADDR_ANY
 * TEST-DESCR: with no port set and address override
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_w_p_w_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "%s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/b_p_c_a
 * TEST-DESCR: (bad port correct address).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: with correct address.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_c_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(1);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/b_p_c_a_over
 * TEST-DESCR: (bad port correct address override).
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: with correct address.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_c_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_port        = htons(1);

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_b_a
 * TEST-DESCR: (correct port bad address)
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a correct port
 * TEST-DESCR: with bad address.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_c_p_b_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/c_p_b_a_over
 * TEST-DESCR: (correct port bad address override) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a correct port
 * TEST-DESCR: with bad address and override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_c_p_b_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");

	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/b_p_b_a
 * TEST-DESCR: (bad port bad address) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: with bad address and override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_b_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/b_p_b_a_over
 * TEST-DESCR: (bad port bad address override) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: with bad address and override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_b_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_b_a
 * TEST-DESCR: (without port bad address) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a no port (0)
 * TEST-DESCR: with bad address.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_w_p_b_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/w_p_b_a_over
 * TEST-DESCR: (without port bad address override) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a no port (0)
 * TEST-DESCR: with bad address and override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_w_p_b_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/b_p_w_a
 * TEST-DESCR: (bad port without address) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: without an  address.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_w_a(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}
/*
 * TEST-TITLE sctp_sendmsg/b_p_w_a
 * TEST-DESCR: (bad port without address override) 
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port
 * TEST-DESCR: without an address with override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_b_p_w_a_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(0);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/non_null_zero
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port/bad address
 * TEST-DESCR: with no override.
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_non_null_zero(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    0, 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/non_null_zero_over
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a bad port/bad address
 * TEST-DESCR: with override.
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_non_null_zero_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)&addr,
	    0, 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/null_zero
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a null address with 0 length
 * TEST-DESCR: without override.
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_null_zero(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL,
	    0, 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "sctp_sendmsg: %s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/null_zero_over
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a null address with 0 length
 * TEST-DESCR: with override.
 * TEST-DESCR: Validate it succeeds.
 */
static void
test_sctp_sendmsg_null_zero_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL,
	    0, 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (n < 0)
		errx(-1, "%s", strerror(errno));
}

/*
 * TEST-TITLE sctp_sendmsg/null_non_zero
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a null address with non-zero length
 * TEST-DESCR: without override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_null_non_zero(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL,
	    sizeof(struct sockaddr_in), 0, 0, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

/*
 * TEST-TITLE sctp_sendmsg/null_non_zero_over
 * TEST-DESCR: On a 1-1 socket, create an
 * TEST-DESCR: association. Send to a null address with non-zero length
 * TEST-DESCR: with override.
 * TEST-DESCR: Validate it fails.
 */
static void
test_sctp_sendmsg_null_non_zero_over(void)
{
	int fd[2], n;
	struct sockaddr_in addr;
	socklen_t size;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "sctp_socketpair: %s", strerror(errno));
	
	size = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&addr, 0, size);
	(void)getsockname(fd[1], (struct sockaddr *)&addr, &size);
	
	addr.sin_addr.s_addr = inet_addr("1.1.1.1");
	addr.sin_port        = htons(1);
	
	n = sctp_sendmsg(fd[0], "Hello", 5, (struct sockaddr *)NULL,
	    sizeof(struct sockaddr_in), 0, SCTP_ADDR_OVER, 0, 0, 0);
	close(fd[0]);
	close(fd[1]);
	
	if (!(n < 0))
		errx(-1, "sctp_sendmsg was successful");
}

int
main(int argc, char *argv[])
{
	test_sctp_sendmsg_c_p_c_a();
	test_sctp_sendmsg_c_p_c_a_over();
	test_sctp_sendmsg_w_p_c_a();
	test_sctp_sendmsg_w_p_c_a_over();
	test_sctp_sendmsg_c_p_w_a();
	test_sctp_sendmsg_c_p_w_a_over();
	test_sctp_sendmsg_w_p_w_a();
	test_sctp_sendmsg_w_p_w_a_over();
	test_sctp_sendmsg_b_p_c_a();
	test_sctp_sendmsg_b_p_c_a_over();
	test_sctp_sendmsg_c_p_b_a();
	test_sctp_sendmsg_c_p_b_a_over();
	test_sctp_sendmsg_b_p_b_a();
	test_sctp_sendmsg_b_p_b_a_over();
	test_sctp_sendmsg_w_p_b_a();
	test_sctp_sendmsg_w_p_b_a_over();
	test_sctp_sendmsg_b_p_w_a();
	test_sctp_sendmsg_b_p_w_a_over();
	test_sctp_sendmsg_non_null_zero();
	test_sctp_sendmsg_non_null_zero_over();
	test_sctp_sendmsg_null_zero();
	test_sctp_sendmsg_null_zero_over();
	test_sctp_sendmsg_null_non_zero();
	test_sctp_sendmsg_null_non_zero_over();

	printf("0");
	return EX_OK;
}
