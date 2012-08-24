#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#define NUMBER_RESOURCES 10

struct resource_lit {
	int limit;
	const char *name;
} limits[NUMBER_RESOURCES] = {
	{ RLIMIT_AS, "virtual memory max" },
	{ RLIMIT_CORE, "largest core file size" },
	{ RLIMIT_CPU, "max CPU in seconds" },
	{ RLIMIT_DATA, "max data segment size" },
	{ RLIMIT_FSIZE, "largest file size" },
	{ RLIMIT_MEMLOCK, "max locked memory" },
	{ RLIMIT_NOFILE, "max number of open files" },
	{ RLIMIT_NPROC, "max number of simultaneous processes" },
	{ RLIMIT_RSS, "max resident set size (RSS)" },
	{ RLIMIT_SBSIZE, "max socket buffer size" }
};

void
show_rlimit(int i)
{
	struct rlimit rl, *rlp;
	char max[100], *maxp;
	char cur[100], *curp;
	rlp = &rl;
	getrlimit(limits[i].limit, rlp);
	if (rlp->rlim_max == RLIM_INFINITY) {
		maxp = "Infinity";
	} else {
		sprintf(max, "%d", rlp->rlim_max);
		maxp = max;
	}
	if (rlp->rlim_cur == RLIM_INFINITY) {
		curp = "Infinity";
	} else {
		sprintf(cur, "%d", rlp->rlim_cur);
		curp = cur;
	}


	printf("Limit for %s max:%s current:%s\n",
	       limits[i].name, 
	       maxp,
	       curp);
}

int
main(int argc, char **argv)
{

	int i;
	if(argc == 1) {
		goto do_all;
	} else {
		i = strtol(argv[1], NULL, 0);
		if(i >= NUMBER_RESOURCES) {
			printf("Error resource must be between 0 - %d\n",
			       (NUMBER_RESOURCES-1));
			return(-1);
		}
		show_rlimit(i);
		goto out;
	}

 do_all:
	for (i=0;i<NUMBER_RESOURCES;i++) {
		show_rlimit(i);
	}
 out:
	return (0);
}
