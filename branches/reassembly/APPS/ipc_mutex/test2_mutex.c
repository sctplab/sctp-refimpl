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
struct ipc_mutex *mtx;

void *
thread_routine(void *arg)
{
	pthread_t p;
	long tid;
	int ret=0;
	p = pthread_self();
	tid = ((struct ipc_pthread_private *)p)->tid;
	printf("I am tid:%x\n", tid);
	printf("Will now get mutex %p\n", mtx);
	ret = ipc_mutex_lock(mtx, NULL, 0);
	if (ret != 0) {
		printf("Ok I returned with %d err:%d - done\n",
		       ret, errno);
		pthread_exit(&ret);
	}
	printf("I:%x now hold the lock, hit return when you want to release it\n",
		tid);
	ret = ipc_mutex_unlock(mtx, NULL);
	printf("Released\n");
	pthread_exit(&ret);
	return (NULL);
}       

int
main(int argc, char **argv)
{
	char buf[100];
	int maxmtx,ret;
	pthread_t thr1, thr2;
	void *retb;
	if (argc < 4) {
	use:
		printf("Use %s 'mutex_shm_name' max-mtx 'mutex_name' \n",
		       argv[0]);
		return(-1);
	}
	maxmtx = strtol(argv[2], NULL, 0);
	if (maxmtx < 1) {
		goto use;
	}
	printf("I will first attach/create the shared mem path:%s max:%d\n",
	       argv[1], maxmtx);
	errno = 0;
	ret = ipc_mutex_sysinit(argv[1], maxmtx, 0);
	printf("ret = %d err:%d with no CREATE\n", ret, errno);
	if (ret == -1) {
		errno = 0;
		ret = ipc_mutex_sysinit(argv[1], maxmtx, IPC_MUTEX_CREATE);
		printf("ret = %d err:%d with CREATE\n", ret, errno);
		if (ret == -1) {
			printf("Can't do more - bye\n");
			return (-1);
		}
	}
	printf("Ok now lets try to get mutex %s\n", argv[3]);
	errno = 0;
	mtx = ipc_mutex_init(argv[3], 0);
	if (mtx == NULL) {
		printf("With no create failed err:%d\n", errno);
		mtx = ipc_mutex_init(argv[3], IPC_MUTEX_CREATE);
		if (mtx == NULL) {
			printf("Still can't get mtx with create err:%d\n",
			       errno);
			printf("Can't do more - bye\n");
			return (-2);
		}
	}
	printf("Ok I have the mtx:%p - lets do threads\n", mtx);
	errno = 0;
	pthread_create(&thr1, NULL, thread_routine, NULL);
	sleep(1);
	pthread_create(&thr2, NULL, thread_routine, NULL);
	pthread_join(thr1, &retb);
	pthread_join(thr2, &retb);
	printf("All threads are done - bye\n");
	ipc_mutex_release();
	return (0);
}
