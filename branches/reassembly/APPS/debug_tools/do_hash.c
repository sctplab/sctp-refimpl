#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
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
	val = strtoul(argv[1], NULL, 0);
	mask = strtoul(argv[2], NULL, 0);
	hash = val & mask;
	printf("val:%x mask:%x = Hash is 0x%x/%d\n",
	       val, mask,
	       hash, hash);
	hash = htonl(val) & mask;
	printf("htonl(val):%x mask:%x = Hash is 0x%x/%d\n",
	       htonl(val), mask,
	       hash, hash);
	return(0);
}
