/*-
 * Copyright (c) 2011, by Randall Stewart.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of the authors nor the names of its 
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
#include <incast_fmt.h>
#include <sys/signal.h>

struct incast_control ctrl;

extern int no_cc_change;

int
main(int argc, char **argv)
{
	int i;
	char *configfile=NULL;
	char *storeFile = NULL;
	memset(&ctrl, 0, sizeof(ctrl));
	
	while ((i = getopt(argc, argv, "Nc:vS:w:?")) != EOF) {
		switch (i) {
		case 'N':
			no_cc_change = 1;
			break;
		case 'w':
			storeFile = optarg;
			break;
		case 'S':
			ctrl.nap_time = strtol(optarg, NULL, 0);
			if (ctrl.nap_time < 0) 
				ctrl.nap_time = 0;
			if (ctrl.nap_time >= 1000000000) {
				/* 1 ns shy of 1 sec */
				ctrl.nap_time = 999999999;
			}
			break;
		case 'v':
			ctrl.verbose = 1;
			break;
		case 'c':
			configfile = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -c config-file [-w outfile -v -S nap]\n", argv[0]);
			return (-1);
			break;
		};
			       
	}
	if (configfile == NULL) {
		goto use;
	}
	/* Setup our list and init things */
	signal(SIGPIPE, SIG_IGN);
	ctrl.file_to_store_results = storeFile;

	/* Now parse the configuration file */
	parse_config_file(&ctrl, configfile, DEFAULT_SVR_PORT);
	if (ctrl.bind_addr.sin_family != AF_INET) {
		printf("Fatal error bind address not set by config file\n");
		return (-1);
	}
	if (ctrl.number_server == 0) {
		printf("Fatal error number of servers still 0\n");
		return (-1);
	}
	incast_run_clients(&ctrl);
	return (0);
}
