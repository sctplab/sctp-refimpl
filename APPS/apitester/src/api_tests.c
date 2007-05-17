#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef LINUX
#include <getopt.h>
#endif
#include "api_tests.h"

extern struct test all_tests[];
extern unsigned number_of_tests;

char *usage = "Usage: apitester [-f] [-l] [-r] suitelist\n"
              "       -f Stop after first failed test.\n"
              "       -l Run all tests in a loop.\n"
              "       -r Run all tests in a random order.\n"
              "suitelist The list of test suites to run.\n"
              "          An empty list means all test suites\n";

static unsigned int run    = 0;
static unsigned int passed = 0;
static unsigned int failed = 0;

void print_usage()
{
	printf("%s", usage);
}

void
run_test(unsigned int i)
{
	char *result;

	if (all_tests[i].enabled) {
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

void
run_tests_random(int ignore_failed)
{	
	do {
		run_test(random() % number_of_tests);
	} while ((failed == 0) || ignore_failed);
}

void
run_tests_once(int ignore_failed)
{
	unsigned int i;
	
	for (i = 0; i < number_of_tests; i++) {
		run_test(i);
		if ((failed > 0) && !ignore_failed)
			break;
	}
}

void
run_tests_loop(int ignore_failed)
{
	unsigned int i;
	
	i = 0;
	do {
		run_test(i);
		if (++i == number_of_tests)
			i = 0;
	} while ((failed == 0) || ignore_failed);
}

void
enable_test(unsigned int i)
{
	all_tests[i].enabled = 1;
}

void
enable_tests(int number_of_suites, char *suite_name[])
{
	unsigned int i, j;

	for (i = 0; i < number_of_tests; i++) {
		for (j = 0; j < number_of_suites; j ++)
			if (strcmp(all_tests[i].suite_name, suite_name[j]) == 0)
				break;
		if ((number_of_suites == 0) || (j < number_of_suites)) {
			enable_test(i);
		}
	}
}

int 
main(int argc, char *argv[])
{
	char c;
	int ignore_failed = 1;
	int test_randomly = 0;
	int test_loop = 0;
	
	while ((c = getopt(argc, argv, "fhlr")) != -1)
		switch(c) {
			case 'f':
				ignore_failed = 0;
				break;
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'l':
				test_loop = 1;
				break;
			case 'r':
				test_randomly = 1;
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}

	enable_tests(argc - optind, argv + optind);
	printf("Name                          Verdict  Info\n");
	printf("================================================================================\n");
	if (test_randomly)
		run_tests_random(ignore_failed);
	else if (test_loop)
		run_tests_loop(ignore_failed);
	else
		run_tests_once(ignore_failed);		
	printf("===============================================================================\n");
	printf("Summary: Number of tests run:    %3u\n", run);
	printf("         Number of tests passed: %3u\n", passed);
	printf("         Number of tests failed: %3u\n", failed);
	if (failed == 0) {
		printf("Congratulations!\n");
	}
	return 0;
}
