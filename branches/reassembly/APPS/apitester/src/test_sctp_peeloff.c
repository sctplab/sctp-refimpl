/*-
 * Copyright (c) 2011 Michael Tuexen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: test_sctp_peeloff.c,v 1.3 2011-10-15 12:14:15 tuexen Exp $
 */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "sctp_utilities.h"
#include "api_tests.h"

DEFINE_APITEST(peeloff, goodcase)
{
	int lfd, cfd, pfd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	sctp_assoc_t assoc_id;

	if ((lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(lfd, INADDR_LOOPBACK, 0) < 0) {
		close(lfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (listen(lfd, 1) < 0) {
		close(lfd);
		RETURN_FAILED_WITH_ERRNO;
	}

	if ((cfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		close(lfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_bind(cfd, INADDR_LOOPBACK, 0) < 0) {
		close(lfd);
		close(cfd);
		RETURN_FAILED_WITH_ERRNO;
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (lfd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(lfd);
		close(cfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (sctp_connectx(cfd, (struct sockaddr *) &addr, 1, &assoc_id) < 0) {
		close(lfd);
		close(cfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	sctp_delay(SCTP_SLEEP_MS);
	if ((pfd = sctp_peeloff(cfd, assoc_id)) < 0) {
		close(lfd);
		close(cfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (close(pfd) < 0) {
		close(cfd);
		close(lfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (close(cfd) < 0) {
		close(lfd);
		RETURN_FAILED_WITH_ERRNO;
	}
	if (close(lfd) < 0) {
		RETURN_FAILED_WITH_ERRNO;
	}
	RETURN_PASSED;
}
