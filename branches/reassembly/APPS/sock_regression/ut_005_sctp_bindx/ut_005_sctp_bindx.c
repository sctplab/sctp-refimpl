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

/*
 * TEST-TITLE bind/port_w_a_w_p
 * TEST-DESCR: (port without adress without port )
 * TEST-DESCR: On a 1-1 socket, bindx to a single
 * TEST-DESCR: address (INADDR_ANY) and 0 port.
 * TEST-DESCR: We expect success.
 */
static void
test_bindx_port_w_a_w_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(0);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
	
	if (result)
		errx(-1, "sctp_bindx: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/port_s_a_w_p
 * TEST-DESCR: (port specified adress without port )
 * TEST-DESCR: On a 1-1 socket, bindx to a single
 * TEST-DESCR: address (LOOPBACK) and 0 port.
 * TEST-DESCR: We expect success.
 */
static void
test_bindx_port_s_a_w_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(0);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		errx(-1, "sctp_bindx: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/port_w_a_s_p
 * TEST-DESCR: (port without address specifed port )
 * TEST-DESCR: On a 1-1 socket, bindx to a single
 * TEST-DESCR: address (ANY) and a specified port.
 * TEST-DESCR: We expect success and we got that port.
 */
static void
test_bindx_port_w_a_s_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		errx(-1, "sctp_bindx: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/port_s_a_s_p
 * TEST-DESCR: (port specified address specifed port )
 * TEST-DESCR: On a 1-1 socket, bindx to a single
 * TEST-DESCR: address (LOOPBACK) and a specified port.
 * TEST-DESCR: We expect success and we got that port.
 */
static void
test_bindx_port_s_a_s_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		errx(-1, "sctp_bindx: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/zero_flag
 * TEST-DESCR: On a 1-1 socket, bindx to a single
 * TEST-DESCR: address (LOOPBACK) and a specified port with
 * TEST-DESCR: no flags, we expect failure.
 */
static void
test_bindx_zero_flag(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, 0);
	close(fd);
		
	if (!result)
		errx(-1, "sctp_bindx() was successful");
}

/*
 * TEST-TITLE bind/add_zero_addresses
 * TEST-DESCR: On a 1-1 socket, bindx add to a single
 * TEST-DESCR: address (LOOPBACK) and a specified port with
 * TEST-DESCR: but address count is 0, we expect failure.
 */
static void
test_bindx_add_zero_addresses(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 0,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
	
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "%s", strerror(errno));
	else
		return errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/rem_zero_addresses
 * TEST-DESCR: On a 1-1 socket, bindx remove to a single
 * TEST-DESCR: address (LOOPBACK) and a specified port with
 * TEST-DESCR: but address count is 0, we expect failure.
 */
static void
test_bindx_rem_zero_addresses(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 0,
	    SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "sctp_bindx: %s", strerror(errno));
	else
		return errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/add_zero_addresses_NULL
 * TEST-DESCR: On a 1-1 socket, bindx add no addresses
 * TEST-DESCR: and NULL pointer, we expect a error.
 */
static void
test_bindx_add_zero_addresses_NULL(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	result = sctp_bindx(fd, NULL, 0, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "sctp_bindx: %s", strerror(errno));
	else
		errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/rem_zero_addresses_NULL
 * TEST-DESCR: On a 1-1 socket, bindx remove no addresses
 * TEST-DESCR: and NULL pointer, we expect a error.
 */
static void
test_bindx_rem_zero_addresses_NULL(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	result = sctp_bindx(fd, NULL, 0, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "%s", strerror(errno));
	else
		errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/add_null_addresses
 * TEST-DESCR: On a 1-1 socket, bindx add addresses
 * TEST-DESCR: with a NULL pointer, we expect a error.
 */
static void
test_bindx_add_null_addresses(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	result = sctp_bindx(fd, NULL, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "sctp_bindx: %s", strerror(errno));
	else
		errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/rem_null_addresses
 * TEST-DESCR: On a 1-1 socket, bindx remove addresses
 * TEST-DESCR: with NULL pointer, we expect a error.
 */
static void
test_bindx_rem_null_addresses(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	result = sctp_bindx(fd, NULL, 1, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		if (errno == EINVAL)
			return;
		else
			errx(-1, "%s", strerror(errno));
	else
		errx(-1, "sctp_bindx() succeeded");
}

/*
 * TEST-TITLE bind/dup_add_s_a_s_p
 * TEST-DESCR: (duplicate add specified addres specified port)
 * TEST-DESCR: On a 1-1 socket, bindx add an address (loopback/1234)
 * TEST-DESCR: and then do it a second time and look for failure
 * TEST-DESCR: of the second attempt.
 */
static void
test_bindx_dup_add_s_a_s_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1,
		SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		errx(-1, "sctp_bindx: %s", strerror(errno));
	}
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		errx(-1, "sctp_bindx: %s", strerror(errno));
}

/*
 * TEST-TITLE bind/rem_last_s_a_s_p
 * TEST-DESCR: (remove last specified address specified port)
 * TEST-DESCR: On a 1-1 socket, bindx add an address (loopback/1234)
 * TEST-DESCR: and then do it a second time with a remove.
 * TEST-DESCR: This should fail, since you can't remove your last address.
 */
static void
test_bindx_rem_last_s_a_s_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1,
		SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		errx(-1, "sctp_bindx: %s", strerror(errno));
	}
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (!result)
		errx(-1, "Can remove last address");
}

/*
 * TEST-TITLE bind/rem_s_a_s_p
 * TEST-DESCR: (remove specified address specified port)
 * TEST-DESCR: On a 1-1 socket, bindx add an address (inaddr_any/1234)
 * TEST-DESCR: and then do it a second time with a remove of the loopback.
 * TEST-DESCR: This should fail, since you can't downgrade a bound-all
 * TEST-DESCR: socket to bound specific.
 */
static void
test_bindx_rem_s_a_s_p(void)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "socket: %s", strerror(errno));
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1,
		SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		errx(-1, "sctp_bindx: %s", strerror(errno));
	}

	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1,
	    SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (!result)
		errx(-1, "Allowed to remove a boundall address");
}

int
main(int argc, char *argv[])
{
	test_bindx_port_w_a_w_p();
	test_bindx_port_s_a_w_p();
	test_bindx_port_w_a_s_p();
	test_bindx_port_s_a_s_p();
	test_bindx_zero_flag();
	test_bindx_add_zero_addresses();
	test_bindx_rem_zero_addresses();
	test_bindx_add_zero_addresses_NULL();
	test_bindx_rem_zero_addresses_NULL();
	test_bindx_add_null_addresses();
	test_bindx_rem_null_addresses();
	test_bindx_dup_add_s_a_s_p();
	test_bindx_rem_last_s_a_s_p();
	test_bindx_rem_s_a_s_p();

	printf("0");
	return EX_OK;
}
