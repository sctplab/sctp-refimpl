#include <ipc_mutex.h>

struct ipc_local_memory *_local_ipc_mem=NULL;

static int
initialize_kinfo(lwpid_t *tid)
{
	/* Get the TID of the main thread */
	int name[4];
	int pid;
	size_t len;
	struct kinfo_proc *kipp;


	if (tid ==  NULL) {
		errno = EINVAL;
		return (-1);
	}
	pid = strtol(argv[1], NULL, 0);

	name[0] = CTL_KERN;
	name[1] = KERN_PROC;
	name[2] = KERN_PROC_PID;
	name[3] = getpid();
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
	*tid = kipp->ki_tid);
	free(kipp);
	return (0);
}


int
ipc_mutex_sysinit(char *pathname, int maxmtx, int flags)
{
	if (pathname == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if (maxmtx == 0) {
		errno = EINVAL;
		return (-1);
	}

	if (_local_ipc_mem != NULL) {
		/* Already init'd */
		errno = EINVAL;
		return (-1);
	}
	_local_ipc_mem = (struct ipc_local_memory *)malloc(sizeof(struct ipc_local_memory));
	if (_local_ipc_mem == NULL)
		/* No memory */
		return (-1);
	memset(_local_ipc_mem, 0, sizeof(struct ipc_local_memory));

	/* get the default tid to use */
	if(initialize_kinfo(&_local_ipc_mem->tid)) {
		/* Can't get ki_tid? */
		int er;
	back_out:
		er = errno;
		free(_local_ipc_mem);
		_local_ipc_mem = NULL;
		errno = er;
		return(-1);
	}
	_local_ipc_mem->pathname = (char *)malloc(strlen(pathname)+1);
	if (_local_ipc_mem->pathname == NULL) {
		got back_out;
	}
	strcpy(_local_ipc_mem->pathname,pathname);
	/* Now attach and setup the shared memory */
	

	/* If we need a hash table, here is where we would load it to _local_ipc_mem */
	/* TODO? */
}


