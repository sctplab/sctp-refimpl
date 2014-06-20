/*-
 * Copyright (c) 2001-2009 by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2001-2009, by Michael Tuexen, tuexen@fh-muenster.de. All rights reserved.
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

DEFINE_APITEST(listen, ipv4_1to1)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd, 1) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	close(fd);
	RETURN_PASSED;
}

DEFINE_APITEST(listen, ipv4_1toM)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd, 1) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	close(fd);
	RETURN_PASSED;
}

DEFINE_APITEST(listen, ipv6_1to1)
{
	int fd;

	if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd, 1) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	close(fd);
	RETURN_PASSED;
}

DEFINE_APITEST(listen, ipv6_1toM)
{
	int fd;

	if ((fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd, 1) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	close(fd);
	RETURN_PASSED;
}

DEFINE_APITEST(listen, ipv4_1to1_two_wildcard)
{
	int fd1, fd2;

	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_enable_reuse_port(fd1) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(fd1, INADDR_ANY, 2000) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if ((fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_enable_reuse_port(fd2) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(fd2, INADDR_ANY, 2000) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd1, 1) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd2, 1) == 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED("listen() was successful");
	}
	close(fd1);
	close(fd2);
	RETURN_PASSED;
}

DEFINE_APITEST(listen, ipv4_1to1_two_specific)
{
	int fd1, fd2;

	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_enable_reuse_port(fd1) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(fd1, INADDR_LOOPBACK, 2000) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if ((fd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fd1);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_enable_reuse_port(fd2) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(fd2, INADDR_LOOPBACK, 2000) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd1, 1) < 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(fd2, 1) == 0) {
		close(fd1);
		close(fd2);
		RETURN_FAILED("listen() was successful");
	}
	close(fd1);
	close(fd2);
	RETURN_PASSED;
}
