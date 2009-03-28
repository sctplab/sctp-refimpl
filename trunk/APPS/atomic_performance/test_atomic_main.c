#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include "test_atomic.h"


int
main (int argc, char **argv)
{
	int number, i;
	uint32_t val1=0, val2=0;
	if (argc < 2) {
		printf("Sorry use %s num-of-times\n", argv[0]);
		return (-1);
	}
	number = strtol(argv[1], NULL, 0);
	for (i=0; i<number; i++) {
		add_one(&val1);
		add_one_atomic(&val2);
	}
	printf("Val 1 is %u Val 2 is %u\n", val1, val2);
	return (0);

}




