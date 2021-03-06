/*
 * Copyright (C) 2009 Randall R. Stewart
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
#include <stdio.h>
#include <ipc_mutex.h>
#include <errno.h>

int
main(int argc, char **argv)
{
	char buf[100];
	int ret;
	struct ipc_mutex *mtx;
	if (argc <  4) {
	use:
		printf("Use %s [system|mutex] 'mutex_shm_name' 'mutex' \n",
		       argv[0]);
		return(-1);
	}
	ret = ipc_mutex_sysinit(argv[2], 0, 0);
	printf("ret = %d err:%d with no CREATE\n", ret);
	if (ret == -1) {
		printf("Can't do more - sys mem does not exist bye\n");
		return (-1);
	}
	if (strcmp(argv[1], "system") == 0) {
		printf("Destroying system memory '%s'\n", argv[2]);
		errno = 0;
		ret = ipc_mutex_sysdestroy();
		printf("Return is %d err:%d\n", ret, errno);
		if (ret == 0) {
			printf("When the last process at exits it will delete\n");
			return (0);
		}
		return (-1);
	}
	printf("Ok now lets try to find the mutex %s\n", argv[3]);
	errno = 0;
	mtx = ipc_mutex_init(argv[3], 0);
	if (mtx == NULL) {
		printf("Mutex '%s' does not exist\n", argv[3]);
		return (0);
	}
	printf("Ok I have the mtx:%p - lets lock it.\n", mtx);
	errno = 0;
	ret = ipc_mutex_lock(mtx, NULL, 0);
	if (ret != 0) {
		printf("Hmm I returned with %d err:%d - cant lock to destroy\n",
		       ret, errno);
		return (-3);
	}
	printf("I now hold the lock, hit return when you want to release it\n");
	ret = ipc_mutex_destroy(mtx, NULL);
	printf("return is %d errno:%d\n", ret, errno);
	return (0);
}
