#ifndef __ipc_mutex_h__
#define __ipc_mutex_h__
#include <sys/types.h>
#include <stdio.h>
#include <sys/umtx.h>
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



struct ipc_mutex {
	struct umtx mutex;
};

struct ipc_mutex_names {
	char      mutex_name[IPC_MUTEX_NAME];
	uint32_t  mutex_flags;
	uint32_t  mutex_offset;
};

struct ipc_mutex_shm {
	int max_mutex;
	int num_mutex_used;
	struct ipc_mutex_names mutexs[0];
};



#endif
