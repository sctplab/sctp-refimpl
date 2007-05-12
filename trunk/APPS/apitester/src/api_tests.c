#include <stdio.h>
#include <string.h>
#include "api_tests.h"

extern struct test all_tests[];
extern unsigned number_of_tests;

int main(int argc, char *argv[])
{
	unsigned int i, j;
	char *result;
	unsigned int run, passed, failed;
	
	run = passed = failed = 0;
	
	printf("Name                          Verdict  Info\n");
	printf("================================================================================\n");
	for (i = 0; i < number_of_tests; i++) {
		for (j = 1; j < argc; j ++)
			if (strcmp(all_tests[i].suite_name, argv[j]) == 0)
				break;
		if ((argc == 1) || (j < argc)) {
			printf("%-29.29s ", all_tests[i].case_name);
			fflush(stdout);
			result =  all_tests[i].func();
			if (result) {
				printf("failed   %-40.40s\n", result);
				failed++;
			} else {
				printf("passed\n");
				passed++;
			}
			run++;
		}
	}
	printf("===============================================================================\n");
	printf("Summary: Number of tests run:    %3u\n", run);
	printf("         Number of tests passed: %3u\n", passed);
	printf("         Number of tests failed: %3u\n", failed);
	if (failed == 0) {
		printf("Congratulations!\n");
	}
	return 0;
}
