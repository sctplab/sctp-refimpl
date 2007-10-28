#include <stdio.h>
#include <sys/types.h>

int
main(int argc, char **argv)
{
	uint32_t val;
	uint32_t mask;
	uint32_t hash;

	if(argc < 3) {
		printf("Use %s val mask\n", argv[0]);
		return(-1);
	}
	val = strtoul(argv[1], 0, NULL);
	mask = strtoul(argv[2], 0, NULL);
	hash = val & mask;
	printf("val:%x mask:%x = Hash is 0x%x/%d\n",
	       val, mask,
	       hash, hash);
	return(0);
}
