/*
 * Copyright (C) 2007 Michael Tuexen, tuexen@fh-muenster.de,
 *
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

#define NUMBER_OF_THREADS 250
#define RUNTIME 60
#define ADDR "127.0.0.1"
#define PORT 5001

static void* create_associations(void *arg)
{
	struct sockaddr_in remote_addr;
	int fd;
	
	remote_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	remote_addr.sin_len         = sizeof(struct sockaddr_in);
#endif	
	remote_addr.sin_addr.s_addr = inet_addr(ADDR);
	remote_addr.sin_port        = htons(PORT);

	while (1) {
		if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
			perror("socket");
		}		
		if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
			perror("connect");
		}
		if (close(fd) < 0) {
			perror("close");
		}
	}
}		
		
int main() {
	pthread_t tid;
	unsigned int i;
	
	for(i = 0; i < NUMBER_OF_THREADS; i++) {
		pthread_create(&tid, NULL, &create_associations, (void *) NULL);
	}
	sleep(RUNTIME);
	return (0);
}
