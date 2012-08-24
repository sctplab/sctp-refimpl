/*-
 * Copyright (c) 2001-2007, by Weongyo Jeong. All rights reserved.
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
#include <netinet/sctp.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static void
test_sctp_getaddrlen(void)
{
	int ret;

	ret = sctp_getaddrlen(AF_UNSPEC);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_getaddrlen: EINVAL expected");

	ret = sctp_getaddrlen(AF_INET);
	if (!(ret == sizeof(struct sockaddr_in)))
		errx(-1,
		    "sctp_getaddrlen: sizeof(struct sockaddr_in) expected");

	ret = sctp_getaddrlen(AF_INET6);
	if (!(ret == sizeof(struct sockaddr_in6)))
		errx(-1,
		    "sctp_getaddrlen: sizeof(struct sockaddr_in6) expected");
}

static void
test_sctp_bindx(void)
{
	char *addrs0, *addrs;
	struct addrinfo hints, *res, *res0;
	int sd, sz = 0, ret, addrs0_count = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(NULL, "http", &hints, &res0);
	if (ret)
		errx(1, "getaddrinfo: %s", gai_strerror(ret));

	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		sz += res->ai_addr->sa_len;
	}

	addrs0 = calloc(1, sz);
	if (addrs0 == NULL)
		errx(-1, "calloc: %s", strerror(errno));

	addrs = addrs0;
	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		memcpy(addrs, res->ai_addr, res->ai_addr->sa_len);
		addrs = (char *)((caddr_t)addrs + res->ai_addr->sa_len);
		addrs0_count++;
	}

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_bindx(-1, (struct sockaddr *)addrs0, addrs0_count,
	    SCTP_BINDX_ADD_ADDR);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_bindx: EBADF expected");

	ret = sctp_bindx(sd, (struct sockaddr *)NULL, addrs0_count,
	    SCTP_BINDX_ADD_ADDR);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_bindx: EINVAL expected");

	ret = sctp_bindx(sd, (struct sockaddr *)addrs0, 999,
	    SCTP_BINDX_ADD_ADDR);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_bindx: EINVAL expected");

	ret = sctp_bindx(sd, (struct sockaddr *)addrs0, addrs0_count, -1);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_bindx: EFAULT expected");

	close(sd);
	freeaddrinfo(res0);
	free(addrs0);
}

static void
test_sctp_connectx(void)
{
	char *addrs0, *addrs;
	struct addrinfo hints, *res, *res0;
	int sd, sz = 0, ret, addrs0_count = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(NULL, "http", &hints, &res0);
	if (ret)
		errx(1, "getaddrinfo: %s", gai_strerror(ret));

	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		sz += res->ai_addr->sa_len;
	}

	addrs0 = calloc(1, sz);
	if (addrs0 == NULL)
		errx(-1, "calloc: %s", strerror(errno));

	addrs = addrs0;
	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		memcpy(addrs, res->ai_addr, res->ai_addr->sa_len);
		addrs = (char *)((caddr_t)addrs + res->ai_addr->sa_len);
		addrs0_count++;
	}

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_connectx(-1, (struct sockaddr *)addrs0, addrs0_count, NULL);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_connectx: EBADF expected");

	ret = sctp_connectx(sd, (struct sockaddr *)NULL, addrs0_count, NULL);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");
	
	ret = sctp_connectx(sd, (struct sockaddr *)addrs0, -1, NULL);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	ret = sctp_connectx(sd, (struct sockaddr *)addrs0, addrs0_count, NULL);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	close(sd);
	freeaddrinfo(res0);
	free(addrs0);
}

static void
test_sctp_opt_info(void)
{
	int sd, ret;
	sctp_assoc_t tmp;

        sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (sd == -1)
                errx(-1, "socket: %s", strerror(errno));

	ret = sctp_opt_info(-1, 9999, 9999, NULL, NULL);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	ret = sctp_opt_info(-1, 9999, 9999, &tmp, NULL);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_connectx: EFAULT expected");

	ret = sctp_opt_info(sd, 9999, 9999, NULL, &tmp);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_opt_info: EINVAL expected");

	close(sd);
}

static void
test_sctp_getpaddrs_freepaddrs(void)
{
	struct sockaddr *raddrs = NULL;
        int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (sd == -1)
                errx(-1, "socket: %s", strerror(errno));

	ret = sctp_getpaddrs(-1, 0, NULL);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_getpaddrs: EFAULT expected");

	ret = sctp_getpaddrs(sd, 0, NULL);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_getpaddrs: EFAULT expected");

	ret = sctp_getpaddrs(sd, 0, &raddrs);
	if (!(ret == -1 && errno == ENOTCONN))
		errx(-1, "sctp_getpaddrs: ENOTCONN expected");

	close(sd);
}

static void
test_sctp_getladdrs_freeladdrs(void)
{
	struct sockaddr *raddrs = NULL;
        int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (sd == -1)
                errx(-1, "socket: %s", strerror(errno));

	ret = sctp_getladdrs(-1, 0, NULL);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_getpaddrs: EFAULT expected");

	ret = sctp_getladdrs(sd, 0, NULL);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_getpaddrs: EFAULT expected");

	ret = sctp_getladdrs(sd, 0, &raddrs);
	if (!(ret == -1 && errno == ENOTCONN))
		errx(-1, "sctp_getpaddrs: ENOTCONN expected");

	close(sd);
}

static void
test_sctp_sendmsg(void)
{
	struct addrinfo hints, *res0;
	char *msg = "Hello SCTP!";
	int sd, ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(NULL, "http", &hints, &res0);
	if (ret)
		errx(1, "getaddrinfo: %s", gai_strerror(ret));

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (sd == -1)
                errx(-1, "socket: %s", strerror(errno));

	ret = sctp_sendmsg(-1, NULL, 0, NULL, 0, 0, 0, 0, 0, 0);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_getpaddrs: EBADF expected");

	ret = sctp_sendmsg(sd, NULL, 20, res0->ai_addr, res0->ai_addr->sa_len,
	    -1, SCTP_EOF, 1, 5000, 4);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_opt_info: EINVAL expected");

	ret = sctp_sendmsg(sd, msg, strlen(msg), NULL, res0->ai_addr->sa_len,
	    -1, SCTP_EOF, 1, 5000, 4);
	if (!(ret == -1 && errno == EFAULT))
		errx(-1, "sctp_getpaddrs: EFAULT expected");

	ret = sctp_sendmsg(sd, msg, strlen(msg),
	    res0->ai_addr, -1, -1, SCTP_EOF, 1, 5000, 4);
	if (!(ret == -1 && errno == ENAMETOOLONG))
		errx(-1, "sctp_getpaddrs: ENAMETOOLONG expected");
	
	close(sd);
	freeaddrinfo(res0);
}

static void
test_sctp_sendmsgx(void)
{
	char *addrs0, *addrs;
	struct addrinfo hints, *res, *res0;
	//char *msg = "Hello SCTP!";
	int sd, sz = 0, ret, addrs0_count = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(NULL, "http", &hints, &res0);
	if (ret)
		errx(1, "getaddrinfo: %s", gai_strerror(ret));

	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		sz += res->ai_addr->sa_len;
	}

	addrs0 = calloc(1, sz);
	if (addrs0 == NULL)
		errx(-1, "calloc: %s", strerror(errno));

	addrs = addrs0;
	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		memcpy(addrs, res->ai_addr, res->ai_addr->sa_len);
		addrs = (char *)((caddr_t)addrs + res->ai_addr->sa_len);
		addrs0_count++;
	}

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_sendmsgx(-1, NULL, 0, NULL, 0, 0, 0, 0, 0, 0);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_getpaddrs: EINVAL expected");

	ret = sctp_sendmsgx(sd, NULL, 0, NULL, 0, 0, 0, 0, 0, 0);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_opt_info: EINVAL expected");

	ret = sctp_sendmsgx(sd, NULL, 0,
	    (struct sockaddr *)addrs0, addrs0_count,
	    0, 0, 0, 0, 0);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_opt_info: EINVAL expected");
	ret = sctp_sendmsgx(sd, NULL, 0, (struct sockaddr *)addrs0, 9999,
	    0, 0, 0, 0, 0);
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_opt_info: EINVAL expected");

	close(sd);
	freeaddrinfo(res0);
	free(addrs0);
}

static void
test_sctp_send(void)
{
	int sd, ret;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_send(-1, NULL, 0, NULL, 0);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_connectx: EBADF expected");

	ret = sctp_send(sd, NULL, 0, NULL, 0);
	if (!(ret == -1 && errno == ENOENT))
		errx(-1, "sctp_connectx: ENOENT expected");

	close(sd);
}

static void
test_sctp_sendx(void)
{
	char *addrs0, *addrs;
	struct addrinfo hints, *res, *res0;
	//char *msg = "Hello SCTP!";
	int sd, sz = 0, ret, addrs0_count = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(NULL, "http", &hints, &res0);
	if (ret)
		errx(1, "getaddrinfo: %s", gai_strerror(ret));

	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		sz += res->ai_addr->sa_len;
	}

	addrs0 = calloc(1, sz);
	if (addrs0 == NULL)
		errx(-1, "calloc: %s", strerror(errno));

	addrs = addrs0;
	for (res = res0; res; res = res->ai_next) {
		if (!(res->ai_addr->sa_family == AF_INET ||
			res->ai_addr->sa_family == AF_INET6))
			continue;

		memcpy(addrs, res->ai_addr, res->ai_addr->sa_len);
		addrs = (char *)((caddr_t)addrs + res->ai_addr->sa_len);
		addrs0_count++;
	}

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_sendx(-1, NULL, 0, NULL, 0, NULL, 0);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_connectx: EBADF expected");

	close(sd);
	freeaddrinfo(res0);
	free(addrs0);
}

static void
test_sctp_recvmsg(void)
{
	int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = sctp_recvmsg(-1, NULL, 0, NULL, NULL, NULL, NULL);
	if (!(ret == -1 && errno == EBADF))
		errx(-1, "sctp_connectx: EBADF expected");

	ret = sctp_recvmsg(sd, NULL, 0, NULL, NULL, NULL, NULL);
	if (!(ret == -1 && errno == ENOTCONN))
		errx(-1, "sctp_getpaddrs: ENOTCONN expected");

	close(sd);
}

int
main(int argc, char *argv[])
{
	test_sctp_getaddrlen();
        test_sctp_bindx();
        test_sctp_connectx();
        test_sctp_opt_info();
        test_sctp_getpaddrs_freepaddrs();
	test_sctp_getladdrs_freeladdrs();
        test_sctp_sendmsg();
        test_sctp_sendmsgx();
        test_sctp_send();
        test_sctp_sendx();
        test_sctp_recvmsg();

	printf("0");
	return EX_OK;
}
