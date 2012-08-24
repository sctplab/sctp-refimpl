/*-
 * Copyright (c) 2007, by Weongyo Jeong. All rights reserved.
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

#include <machine/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <sysexits.h>
#include <unistd.h>

#define TEST_1_DEFAULT_PORT	65531

static char *
test_1_shm_client(void)
{
	char *sm;
	int shmid;

	if ((shmid = shmget((key_t)1234, sizeof(char), 0)) == -1)
		errx(-1, "c shmget error");

	sm = shmat(shmid, (void *)0, 0666|IPC_CREAT);
	if (sm == (void *)-1)
		errx(-1, "c shmat error");

	return sm;
}

static void
test_1_client(void)
{
	int fd, sv, rv;
	char buf[0x10], *sm;
	struct sockaddr_in sin;

	sm = test_1_shm_client();
	for (;sm[0] != 'D';) {
		sleep(1);
	}

	sin.sin_port = ntohs(TEST_1_DEFAULT_PORT);
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);

	if (inet_pton(AF_INET, "127.0.0.1", (void *)&sin.sin_addr.s_addr) != 1)
		errx(-1, "inet_pton error");

	if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1)
		errx(-1, "socket error");

	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)))
		errx(-1, "connect error");

	if ((sv = send(fd, "ABCDEFGH", 8, 0)) != 8)
		errx(-1, "send error");

	if ((rv = recv(fd, &buf, 8, 0)) != 8)
		errx(-1, "recv, error");

	close(fd);
}

static char *
test_1_shm_server(void)
{
	char *sm;
	int shmid;

	if ((shmid = shmget((key_t)1234, sizeof(char), 0666|IPC_CREAT)) == -1)
		errx(-1, "shmget error");
	sm = shmat(shmid, (void *)0, 0);
	if (sm == (void *)-1)
		errx(-1, "shmat error");

	bzero(sm, 1);

	return sm;
}

static void
test_1_server(char *sm)
{
	int afd, fd, on = 1, rv, sv;
	socklen_t fromlen;
	char buf[0x10];
	struct sockaddr_in sin, from;

	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = ntohs(TEST_1_DEFAULT_PORT);
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);
	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd == -1)
		errx(-1, "socket error");

	if (setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &on, sizeof(on)) == -1)
		errx(-1, "setsockopt error");

	on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (int *)&on, sizeof(on));

	if (bind(fd, (struct sockaddr *)&sin, sin.sin_len) == -1)
		errx(-1, "bind error");

	if (listen(fd, 1) == -1)
		errx(-1, "listen error");

	sm[0] = 'D';

	fromlen = sizeof(from);
	if ((afd = accept(fd, (struct sockaddr *)&from, &fromlen)) == -1)
		errx(-1, "accept error");
	
	if ((rv = recv(afd, &buf, 8, 0)) == -1)
		errx(-1, "recv error");
	if (rv != 8)
		errx(-1, "recv error");

	if ((sv = send(afd, buf, 8, 0)) == -1)
		errx(-1, "send error");
	if (sv != 8)
		errx(-1, "send error");

	close(afd);
	close(fd);
}

static void
sigchld_handler(int x)
{
	int nWaiter;
	pid_t pid;
	
	for (;;) {
#ifdef POSIX
		pid = waitpid(-1, &nWaiter, WNOHANG);
#else
		pid = wait3(&nWaiter, WNOHANG, (struct rusage *)0);
#endif
		switch (pid) {
		case -1:
			return;
		case 0:
			return;
		default:
			break;
        }
    }
}

int
main(int argc, char *argv[])
{
	char *sm;
	struct sigaction action;

	sm = test_1_shm_server();

	/* install signal handler */
	bzero(&action, sizeof(action));
	action.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &action, NULL);

	switch(fork()) {
	case -1:
		errx(-1, "fork error");
		break;
	case 0:
		test_1_client();
		_exit(EX_OK);
	default:
		test_1_server(sm);
	}

	printf("0");
	return EX_OK;
}
