/*-
 * Copyright (c) 2001-2007 by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2001-2011, by Michael Tuexen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef LINUX
#include <getopt.h>
#endif
#include "api_tests.h"

#define RED(string) "\033[31m"string"\033[0m"
#define GREEN(string) "\033[32m"string"\033[0m"
#define YELLOW(string) "\033[33m"string"\033[0m"
#define BLUE(string) "\033[34m"string"\033[0m"

extern struct test all_tests[];
extern unsigned number_of_tests;

char *usage = "Usage: apitester [-f] [-l] [-r] suitelist\n"
              "       -d seconds Sleeps seconds after each test (default 0).\n"
              "       -f Stop after first failed test.\n"
              "       -l Run all tests in a loop.\n"
              "       -r Run all tests in a random order.\n"
              "suitelist The list of test suites to run.\n"
              "          An empty list means all test suites\n";

static unsigned int run    = 0;
static unsigned int passed = 0;
static unsigned int failed = 0;

static void
print_usage()
{
	printf("%s", usage);
}

static void
run_test(unsigned int i, int delay)
{
	char *result;

	if (all_tests[i].enabled) {
		printf("%-29.29s ", all_tests[i].case_name);
		fflush(stdout);
		result =  all_tests[i].func();
		if (result) {
			printf("%s   %-40.40s\n", RED("FAILED"), result);
			failed++;
		} else {
			printf("%s\n", GREEN("PASSED"));
			passed++;
		}
		run++;
		if (delay > 0) {
			sleep(delay);
		}
	}
}

static void
run_tests_random(int delay, int ignore_failed)
{
	do {
		run_test(random() % number_of_tests, delay);
	} while ((failed == 0) || ignore_failed);
}

static void
run_tests_once(int delay, int ignore_failed)
{
	unsigned int i;

	for (i = 0; i < number_of_tests; i++) {
		run_test(i, delay);
		if ((failed > 0) && !ignore_failed) {
			break;
		}
	}
}

static void
run_tests_loop(int delay, int ignore_failed)
{
	unsigned int i;

	i = 0;
	do {
		run_test(i, delay);
		if (++i == number_of_tests)
			i = 0;
	} while ((failed == 0) || ignore_failed);
}

static void
enable_test(unsigned int i)
{
	all_tests[i].enabled = 1;
}

static void
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
	int c;
	int delay = 0;
	int ignore_failed = 1;
	int test_randomly = 0;
	int test_loop = 0;

	while ((c = getopt(argc, argv, "d:fhlr")) != -1) {
		switch(c) {
			case 'd':
				delay = atoi(optarg);
				break;
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
	}
	enable_tests(argc - optind, argv + optind);
	printf("Name                          Verdict  Info\n");
	printf("===============================================================================\n");
	if (test_randomly) {
		run_tests_random(delay, ignore_failed);
	} else if (test_loop) {
		run_tests_loop(delay, ignore_failed);
	} else {
		run_tests_once(delay, ignore_failed);
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
