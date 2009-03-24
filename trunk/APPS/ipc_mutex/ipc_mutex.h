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
#ifndef __ipc_mutex_h__
#define __ipc_mutex_h__
#include <machine/param.h>
#include <sys/types.h>
#include <machine/atomic.h>
#include <sys/mman.h>

#include <sys/user.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/umtx.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/sysctl.h>

/*
 * Shared memory is attached at 
 * a specific address. Once attached
 * the shared memory segment looks like:
 *
 * *************************
 * *    ipc_mutex_shm i.e:
 * *************************
 * *    array of ipc_mutex[]
 * *************************
 *
 * In practicality this means you end up
 * with:
 *  *******
 *   max
 *   used
 *   process_map[]
 *   names[0] ---+
 *   names[1]    |
 *   names[2]    |
 *    ...        | Indirect pointer to.
 *   names[N]    |
 *  *******      |
 *   mutex[0]<---+
 *   mutex[1]
 *   mutex[2]
 *    ...
 *   mutex[N]
 *  ********
 *
 * The shared memory is mapped between processes
 * using a common file as access. We may in future
 * make this file become an actual hash table that
 * can be loaded by the init process for quicker
 * lookup of the actual mutex's. A process or thread
 * needs to have the ipc_mutex pointer to do the actual
 * lock()/unlock(). 
 *
 * When you lock you can provide an pthread id of 0, or
 * you can give a pthread value. We take the pid and the
 * thread id (bottom bits) in combination to generate an
 * id. If you are using a non-threaded app all you need
 * to do is set pthread to NULL.
 *
 *
 */
#define IPC_MUTEX_NAME 24
#define IPC_INITIALIZED_PATTERN 0x61eabe11
#define MAX_PATHNAME 512
#define IPC_MUTEX_MAX_MAP 512 /* Max number of processes that will attach */

struct ipc_mutex {
	int mutex_index;
	struct umtx mutex;
};

#define IPC_MUTEX_UNUSED   0x00000001
#define IPC_MUTEX_ASSIGNED 0x00000002
#define IPC_MUTEX_DELETED  0x00000004

struct ipc_mutex_names {
	char      mutex_name[IPC_MUTEX_NAME];
	uint32_t  mutex_flags;
	uint32_t  mutex_index;
	lwpid_t   mutex_p_tid;
};

struct mutex_proc_map {
	pid_t     pid;
	lwpid_t   tid;
};

struct ipc_mutex_shm {
	int initialized;
	struct umtx shmlock;
	size_t size;
	struct mutex_proc_map map[IPC_MUTEX_MAX_MAP];
	int max_mutex;
	int num_mutex_used;
	struct ipc_mutex_names names[0];
};

/* Local memory stuff */
struct ipc_local_memory {
	lwpid_t default_tid; /* TID to use when no pthread is passed */
	int   my_pidentry;
	int   shmid;
	size_t size;
	char pathname[MAX_PATHNAME];
	void *shm_base_addr;
	struct ipc_mutex_shm *shm;
	struct ipc_mutex *mtxs;
};


/* This is a HACK until we integrate into libthr and can
 * include thr_private.h
 */
struct ipc_pthread_private {
	/* Kernel thread id. */
	long			tid;
};

#define IPC_SHM_FTOK_ID  0x0001  /* We alays use the same ID for ftok */


/* Flags for create */
#define IPC_MUTEX_CREATE 0x0001	 /* Create if not there */

/* Attach to the memory, if it does not exist and
 * IPC_MUTEX_CREATE is not in the flags, then you will
 * fail with an error. 
 */
int ipc_mutex_sysinit(char *pathname, int maxmtx, int flags);

/* Gracefully release the shm leaving it around for
 * others to use.
 */
void ipc_mutex_release();

/* Gracefully release the shm, but also wipe it out
 * so that at the end, it is not available to others.
 * Note that probably current pid's will still have
 * access to it until they exit.
 */
int ipc_mutex_sysdestroy();

/* Audit TID space to look for dead processes */
void ipc_audit_tid_space();



/* Finds the lock, if you don't have the IPC_MUTEX_CREATE
 * flag in place it will not create it if it does not exist.
 */
struct ipc_mutex *ipc_mutex_init(char *mutex_name, int flags);

/* You must own the lock to destroy it */
int ipc_mutex_destroy(struct ipc_mutex *mtx, pthread_t thr);


int ipc_mutex_lock(struct ipc_mutex *mtx, pthread_t thr, int flags);
int ipc_mutex_lock_timed(struct ipc_mutex *mtx, pthread_t thr, 
			 const struct timespec *tv);
int ipc_mutex_trylock(struct ipc_mutex *mtx, pthread_t thr); 

int ipc_mutex_unlock(struct ipc_mutex *mtx, pthread_t thr);

void ipc_mutex_show();

#endif
