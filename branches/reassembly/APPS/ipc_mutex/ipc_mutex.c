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

#include <ipc_mutex.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


struct ipc_local_memory *_local_ipc_mem=NULL;

static int
initialize_kinfo(pid_t pid, lwpid_t *tid)
{
	/* Get the TID of the main thread */
	int name[4];
	size_t len;
	struct kinfo_proc *kipp;


	if (tid ==  NULL) {
		errno = EINVAL;
		return (-1);
	}
	pid = getpid();

	name[0] = CTL_KERN;
	name[1] = KERN_PROC;
	name[2] = KERN_PROC_PID;
	name[3] = pid;
	if (sysctl(name, 4, NULL, &len, NULL, 0) < 0) {
		return (-1);
	}

	kipp = (struct kinfo_proc *)malloc(len);
	if (kipp == NULL) {
		return (-1);
	}

	if (sysctl(name, 4, kipp, &len, NULL, 0) < 0) {
		free(kipp);
		return (-1);
	}
	*tid = kipp->ki_tid;
	free(kipp);
	return (0);
}


static void
ipc_sub_my_tid(pid_t pid, lwpid_t tid)
{
	int i;
	struct mutex_proc_map *map;

	if (_local_ipc_mem == NULL) {
		/* TSNH */
		return;
	}

	if (_local_ipc_mem->shm == NULL) {
		/* TSNH */
		return;
	}
	map = _local_ipc_mem->shm->map;
	map[_local_ipc_mem->my_pidentry].pid = 0;
	map[_local_ipc_mem->my_pidentry].tid = 0;
	return;
}

void
ipc_audit_tid_space()
{
	int i;
	struct mutex_proc_map *map;
	lwpid_t tid;

	if (_local_ipc_mem == NULL) {
		return;
	}

	if (_local_ipc_mem->shm == NULL) {
		return;
	}
	umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
	map = _local_ipc_mem->shm->map;
	for (i=0; i<IPC_MUTEX_MAX_MAP; i++) {
		if (map[i].pid == 0) {
			continue;
		}
		if (kill(map[i].pid, 0) == -1) {
			/* process is dead/non-existant free it */
			map[i].pid = 0;
			map[i].tid = 0;
			continue;
		}
		initialize_kinfo(map[i].pid, &tid);
		if (tid != map[i].tid) {
			/* process pid, is NOT the same pid reused */
			map[i].pid = 0;
			map[i].tid = 0;
			continue;
		}
	}
	umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
}


void
ipc_mutex_exits()
{
	int i;
	lwpid_t id, owner;
	struct ipc_mutex *mtx;
	if (_local_ipc_mem == NULL)
		/* TSNH */
		return;
	if (_local_ipc_mem->default_tid == UMTX_UNOWNED)
		/* TSNH */
		return;
	umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
	for(i=0; i<_local_ipc_mem->shm->max_mutex; i++) {
		if(_local_ipc_mem->shm->names[i].mutex_p_tid ==
		   _local_ipc_mem->default_tid) {
			/* Look at the owner */
			mtx = &_local_ipc_mem->mtxs[i];
			owner = umtx_owner(&mtx->mutex);
			if (owner != UMTX_UNOWNED) {
				/* Some how we left with this locked */
				umtx_unlock(&mtx->mutex, owner);
				_local_ipc_mem->shm->names[i].mutex_p_tid = 0;
			}
		}
	}
	ipc_sub_my_tid(getpid(), _local_ipc_mem->default_tid);
	umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
}


static int
ipc_add_my_tid(pid_t pid, lwpid_t tid)
{
	int i;
	struct mutex_proc_map *map;

	if (_local_ipc_mem == NULL) {
		/* TSNH */
		return(-1);
	}

	if (_local_ipc_mem->shm == NULL) {
		/* TSNH */
		return(-1);
	}
	map = _local_ipc_mem->shm->map;
	for (i=0; i<IPC_MUTEX_MAX_MAP; i++) {
		if (map[i].pid == 0) {
			/* Got one to use */
			_local_ipc_mem->my_pidentry = i;
			map[i].pid = pid;
			map[i].tid = tid;
			return (0);
		}
	}
	return (-1);
}

void
ipc_mutex_release()
{
	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return;
	}

	if (_local_ipc_mem->shm == NULL) {
		errno = EINVAL;
		return;
	}
	ipc_mutex_exits();
	munmap(_local_ipc_mem->shm_base_addr, 
	       _local_ipc_mem->size);
	free(_local_ipc_mem);
	_local_ipc_mem = NULL;
}

int 
ipc_mutex_sysdestroy()
{
	
	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}

	ipc_mutex_exits();
	if (shm_unlink(_local_ipc_mem->pathname) == -1) {
		return (-1);
	}
	if (munmap(_local_ipc_mem->shm_base_addr, _local_ipc_mem->size) == -1) {
		return (-1);
	}
	free(_local_ipc_mem);
	_local_ipc_mem = NULL;
	return (0);
}



int
ipc_mutex_sysinit(char *pathname, int maxmtx, int flags)
{
	struct stat sbuf;
	int i,cnt;
	int creator=0;
	int prot, flg;
	size_t size, osiz;
	if (pathname == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if ((maxmtx == 0) && (flags & IPC_MUTEX_CREATE)) {
		errno = EINVAL;
		return (-1);
	}
	if (_local_ipc_mem != NULL) {
		/* Already init'd */
		errno = EINVAL;
		return (-1);
	}
	_local_ipc_mem = (struct ipc_local_memory *)malloc(sizeof(struct ipc_local_memory));
	if (_local_ipc_mem == NULL) {
		/* No memory */
		return (-1);
	}
	memset(_local_ipc_mem, 0, sizeof(struct ipc_local_memory));

	/* get the default tid to use */
	if(initialize_kinfo(getpid(), &_local_ipc_mem->default_tid)) {
		/* Can't get ki_tid? */
		int er;
	back_out:
		er = errno;
		free(_local_ipc_mem);
		_local_ipc_mem = NULL;
		errno = er;
		return(-1);
	}
	size = (((sizeof(struct ipc_mutex_names)) * maxmtx) +
		(sizeof(struct ipc_mutex) * maxmtx) +
		(sizeof(struct ipc_mutex_shm)));
	osiz = size;
	if (size % PAGE_SIZE) {
		size += (PAGE_SIZE - (size % PAGE_SIZE));
	}
	/* Round up to next page size */
	if (osiz < size) {
		/* rethink the max mtx's */
		maxmtx += ((size - osiz)/ (sizeof(struct ipc_mutex_names) +
					   sizeof(struct ipc_mutex)));
	}
	osiz = size;
	/* Now attach and setup the shared memory */
	if (stat(pathname, &sbuf) == -1) {
		char buf[2];
		FILE *io;
		int i;
	creat_it:
		if ((flags & IPC_MUTEX_CREATE) == 0) {
			errno = ENOENT;
			goto back_out;
		}
		creator = 1;
		io = fopen(pathname, "w+");
		if (io == NULL) {
			/* Can't create file we can't attach */
			goto back_out;
		}
		buf[0] = 0;
		/* Pad out the file to the right size */
		for (i=0; i<size; i++) {
			fwrite(buf, 1, 1, io);
		}
		fclose(io);
	} else {
		int saved_size, saved_maxmtx;
		saved_size = size;
		saved_maxmtx = maxmtx;
		size = sbuf.st_size;
		if (size != osiz) {
			osiz -= (sizeof(struct ipc_mutex_shm));
			maxmtx = osiz/ (sizeof(struct ipc_mutex_names) +
					sizeof(struct ipc_mutex));
			if (maxmtx < 1) {
				size = saved_size;
				maxmtx = saved_maxmtx;
				goto creat_it;
			}
		}
	}
	/* Now lets generate the keyid */
	strcpy(_local_ipc_mem->pathname, pathname);
	_local_ipc_mem->shmid = shm_open(pathname, O_RDWR, 0);
	if (_local_ipc_mem->shmid == -1) {
		/* Can't generate the key */
		goto back_out;
	}
	prot = PROT_READ | PROT_WRITE;
	flg = MAP_HASSEMAPHORE | MAP_NOSYNC | MAP_SHARED;
	_local_ipc_mem->size = size;
	_local_ipc_mem->shm_base_addr = mmap(0, size, prot, flg, 
					     _local_ipc_mem->shmid, 0); 
	if (_local_ipc_mem->shm_base_addr == MAP_FAILED) {
		/* Huh? */
		printf("map failed errno:%d\n", errno);
		goto back_out;
	}
	/* Get our first pointer setup */
	_local_ipc_mem->shm = (struct ipc_mutex_shm *)_local_ipc_mem->shm_base_addr;
	if (creator) {
		/* We are the creator so initialize it please */
	do_the_init:
		umtx_init(&_local_ipc_mem->shm->shmlock);
		umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
		_local_ipc_mem->shm->size = size;
		_local_ipc_mem->shm->initialized = IPC_INITIALIZED_PATTERN;
		_local_ipc_mem->shm->max_mutex = maxmtx;
		_local_ipc_mem->shm->num_mutex_used = 0;
		creator = 1;
	} else {
 		if (_local_ipc_mem->shm->initialized != IPC_INITIALIZED_PATTERN) {
			/* Simultaneous init? */
			sleep(1);
		}
		if (_local_ipc_mem->shm->initialized != IPC_INITIALIZED_PATTERN) {
			/* The initializer died without completing? */

			/* now what size is it really ? */
			maxmtx = (size - sizeof(struct ipc_mutex_shm));
			maxmtx /= (sizeof(struct ipc_mutex_names) +
				   sizeof(struct ipc_mutex));
			goto do_the_init;
		}
		umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
		_local_ipc_mem->size =_local_ipc_mem->shm->size;
	}
	/* Up until now we canNOT accuess _local_ipc_mem->mtxs since the
	 * address does not get valid until we calculate it.
	 */
	{
		caddr_t calc;
		calc = _local_ipc_mem->shm_base_addr;
		calc += sizeof(struct ipc_mutex_shm);
		calc += (_local_ipc_mem->shm->max_mutex * sizeof(struct ipc_mutex_names));
		_local_ipc_mem->mtxs = (struct ipc_mutex *)calc;
	}
	if (creator) {
		/* We should have only one creator. 
		 * The lock at the initialization assures
		 * us that all others will sleep while the
		 * creator initializes things. Note that we must
		 * never take more than 1 second to init the lock, lock
		 * it and write the INIT pattern down. If we do we could
		 * have a problem.
		 */
		/* First init the pid <-> tid mapping for audits */
		for (i=0; i<IPC_MUTEX_MAX_MAP; i++) {
			_local_ipc_mem->shm->map[i].pid = 0;
			_local_ipc_mem->shm->map[i].tid = 0;
		}
		/* Now all the locks */
		for (i=0; i<maxmtx; i++) {
			_local_ipc_mem->shm->names[i].mutex_flags = IPC_MUTEX_UNUSED;
			_local_ipc_mem->shm->names[i].mutex_index = i;
			_local_ipc_mem->mtxs[i].mutex_index = i;
			umtx_init(&_local_ipc_mem->mtxs[i].mutex);
		}
	}
	cnt = 0;
retry:
	if(ipc_add_my_tid(getpid(), _local_ipc_mem->default_tid)) {
		/* Can't add me to the pid cache.. its failed */
		umtx_unlock(&_local_ipc_mem->shm->shmlock, 
			    _local_ipc_mem->default_tid);
		if (cnt == 0) {
			/* Try again after an audit */
			ipc_audit_tid_space();
			umtx_lock(&_local_ipc_mem->shm->shmlock, 
				  _local_ipc_mem->default_tid);			
			cnt++;
			goto retry;
		}
		goto back_out;

	}
	/****************************************************************************/
	/* If we add a hash table, here is where we would load it to _local_ipc_mem */
	/****************************************************************************/
	printf("There are a maximum of %d mutexes %d are in use\n",
	       _local_ipc_mem->shm->max_mutex,
	       _local_ipc_mem->shm->num_mutex_used);
	umtx_unlock(&_local_ipc_mem->shm->shmlock, 
		    _local_ipc_mem->default_tid);		

	/* Audit after all init and unlock */
	if (cnt == 0)
		ipc_audit_tid_space();
	atexit(ipc_mutex_exits);
	return (0);
}

static void
ipc_harvest_dead(int *empty_slot, int *empty_seen)
{
	int i;
	for(i=0; i<_local_ipc_mem->shm->max_mutex; i++) {
		if(_local_ipc_mem->shm->names[i].mutex_flags & IPC_MUTEX_DELETED) {
			_local_ipc_mem->shm->names[i].mutex_flags = IPC_MUTEX_UNUSED;
			umtx_init(&_local_ipc_mem->mtxs[i].mutex);			
			_local_ipc_mem->shm->num_mutex_used--;
			if (*empty_seen == 0) {
				*empty_seen = 1;
				*empty_slot = i;
			}
		}
	}
}


struct ipc_mutex *
ipc_mutex_init(char *mutex_name, int flags)
{
	struct ipc_mutex *ret=NULL;
	char mtxname[IPC_MUTEX_NAME];
	int empty_slot, i, empty_seen=0;
	/* We first try to find the mutex */
	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (NULL);
	}

	if (strlen(mutex_name) > (IPC_MUTEX_NAME-1)) {
		strncpy(mtxname, mutex_name, (IPC_MUTEX_NAME-1));
		mtxname[(IPC_MUTEX_NAME-1)] = 0;
	} else {
		strcpy(mtxname, mutex_name);
	}
	umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
	for(i=0; i<_local_ipc_mem->shm->max_mutex; i++) {
		if (_local_ipc_mem->shm->names[i].mutex_flags == IPC_MUTEX_ASSIGNED) {
			/* Is this us? */
			if (strcmp(mtxname, _local_ipc_mem->shm->names[i].mutex_name) == 0) {
				/* found it */
				ret = &_local_ipc_mem->mtxs[i];
				goto out_now;
			}
		} else if ((empty_seen == 0) && 
			   (_local_ipc_mem->shm->names[i].mutex_flags == IPC_MUTEX_UNUSED)) {
			empty_slot = i;
			empty_seen = 1;
		}
	}
	/* Ok we did not find it */
	if (flags & IPC_MUTEX_CREATE) {
		/* Not allowed to create */
		errno = ENOENT;
		umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);	
		return (NULL);
	}
	/* Can we create one? */
	if (_local_ipc_mem->shm->num_mutex_used >= _local_ipc_mem->shm->max_mutex) {
		/* No room? */
		ipc_harvest_dead(&empty_slot, &empty_seen);
		if (_local_ipc_mem->shm->num_mutex_used >= _local_ipc_mem->shm->max_mutex) {
			errno = ENOMEM;
			umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);	
			return (NULL);
		}
	}
	/* Ok if we reach here there is room for one */
	if (empty_seen) {
		/* We have a slot cached */
		strcpy(_local_ipc_mem->shm->names[empty_slot].mutex_name, mtxname);
		_local_ipc_mem->shm->names[empty_slot].mutex_flags = IPC_MUTEX_ASSIGNED;
		ret = &_local_ipc_mem->mtxs[empty_slot];
		_local_ipc_mem->shm->num_mutex_used++;
	}
	/* Ok, no cached seen. We really should not
	 * get here since the dead harvest will return and set
	 * the empty_slot and we would not have stopped our hunt
	 * through the names.. consider this no-memory condition.
	 */
	errno = ENOMEM;
out_now:
	umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);	
	return (ret);
}

int 
ipc_mutex_lock(struct ipc_mutex *mtx, pthread_t thr, int flags)
{
	int ret, slot;
	lwpid_t id;

	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if (thr == NULL) {
		id = _local_ipc_mem->default_tid;	
	} else {
		id = (lwpid_t)(((struct ipc_pthread_private *)thr)->tid);
	}
	slot = mtx->mutex_index;
	if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
		errno = EINVAL;
		return (-1);
	}
	ret = umtx_lock(&mtx->mutex, id);
	if (ret == 0) {
		if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
			/* Its been killed by previous owner */
			errno = EINVAL;
			umtx_unlock(&mtx->mutex, id);
			return (-1);
		}
		_local_ipc_mem->shm->names[slot].mutex_p_tid = _local_ipc_mem->default_tid;
	}
	return (ret);
}

int 
ipc_mutex_lock_timed(struct ipc_mutex *mtx, pthread_t thr, 
		     const struct timespec *ts)
{
	int ret, slot;
	lwpid_t id;

	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if (thr == NULL) {
		id = _local_ipc_mem->default_tid;	
	} else {
		id = (lwpid_t)(((struct ipc_pthread_private *)thr)->tid);
	}
	slot = mtx->mutex_index;
	if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
		errno = EINVAL;
		return (-1);
	}
	ret = umtx_timedlock(&mtx->mutex, id, ts);
	if (ret == 0) {
		if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
			/* Its been killed by previous owner */
			errno = EINVAL;
			umtx_unlock(&mtx->mutex, id);
			return (-1);
		}
		_local_ipc_mem->shm->names[slot].mutex_p_tid = _local_ipc_mem->default_tid;
	}
	return (ret);
}


int 
ipc_mutex_unlock(struct ipc_mutex *mtx, pthread_t thr)
{
	lwpid_t id;
	int slot;

	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if (thr == NULL) {
		id = _local_ipc_mem->default_tid;	
	} else {
		id = (lwpid_t)(((struct ipc_pthread_private *)thr)->tid);
	}
	slot = mtx->mutex_index;
	_local_ipc_mem->shm->names[slot].mutex_p_tid = 0;
	umtx_unlock(&mtx->mutex, id);

}

int
ipc_mutex_trylock(struct ipc_mutex *mtx, pthread_t thr)
{
	int ret, slot;
	lwpid_t id;

	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if (thr == NULL) {
		id = _local_ipc_mem->default_tid;	
	} else {
		id = (lwpid_t)(((struct ipc_pthread_private *)thr)->tid);
	}
	slot = mtx->mutex_index;
	if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
		errno = EINVAL;
		return (-1);
	}
	ret = umtx_trylock(&mtx->mutex, id);
	if (ret == 0) {
		if (_local_ipc_mem->shm->names[slot].mutex_flags & IPC_MUTEX_DELETED) {
			/* Its been killed by previous owner */
			errno = EINVAL;
			umtx_unlock(&mtx->mutex, id);
			return (-1);
		}
		_local_ipc_mem->shm->names[slot].mutex_p_tid = _local_ipc_mem->default_tid;
	}
	return (ret);
}

int
ipc_mutex_destroy(struct ipc_mutex *mtx, pthread_t thr)
{
	lwpid_t id, owner;
	int slot;
	int ret = -1;

	if (_local_ipc_mem == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if (thr == NULL) {
		id = _local_ipc_mem->default_tid;	
	} else {
		id = (lwpid_t)(((struct ipc_pthread_private *)thr)->tid);
	}
	umtx_lock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
	owner = umtx_owner(&mtx->mutex);
	if (owner != id) {
		/* You must own it to destroy it */
		errno = EINVAL;
		goto out_of;
	}
	slot = mtx->mutex_index;
	_local_ipc_mem->shm->names[slot].mutex_flags |= IPC_MUTEX_DELETED;
	_local_ipc_mem->shm->names[slot].mutex_flags &= ~IPC_MUTEX_ASSIGNED;
	ret = 0;
out_of:
	umtx_unlock(&_local_ipc_mem->shm->shmlock, _local_ipc_mem->default_tid);
	return (ret);
}

void
ipc_mutex_show()
{
	int i;
	lwpid_t owner;
	if (_local_ipc_mem == NULL) {
		printf("Can't show anything, mutex system not initialized\n");
		return;
	}

	printf("There are %d mutex's that can be created and %d currently exist\n",
	       _local_ipc_mem->shm->max_mutex,
	       _local_ipc_mem->shm->num_mutex_used);
	for(i=0; i<_local_ipc_mem->shm->max_mutex; i++) {	
		if (_local_ipc_mem->shm->names[i].mutex_flags & IPC_MUTEX_DELETED){
			printf("Mutex %s is deleted at slot:%d %p\n", 
			       _local_ipc_mem->shm->names[i].mutex_name,
			       i, &_local_ipc_mem->mtxs[i]);
		} else if (_local_ipc_mem->shm->names[i].mutex_flags & IPC_MUTEX_ASSIGNED){
			owner = umtx_owner(&_local_ipc_mem->mtxs[i].mutex);
			if (owner != UMTX_UNOWNED) {
			printf("Mutex %s is assigned at slot:%d (%p) owned by %x\n", 
			       _local_ipc_mem->shm->names[i].mutex_name,
			       i, &_local_ipc_mem->mtxs[i], (uint)owner);
			} else {

			printf("Mutex %s is assigned at slot:%d (%p) and is un-owned\n", 
			       _local_ipc_mem->shm->names[i].mutex_name,
			       i, &_local_ipc_mem->mtxs[i]);
			}
		}
	}
}
