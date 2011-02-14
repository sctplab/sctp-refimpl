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
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/errno.h>


int
main(int argc, char **argv)
{

	char buffer[1024];
	FILE *io;
	char *file=NULL, *stop, *place="1";
	double time_of, readin;
	uint64_t bytes;
	int txfr, i;
	double bytes_per_sec;

	time_of = 0.0;
	bytes = 0;

	while ((i = getopt(argc, argv, "i:p:?")) != EOF) {
		switch (i) {
		case 'p':
			place = optarg;
			break;
		case 'i':
			file = optarg;
			break;
		case '?':
		default:
		use:
			printf("Use %s -i file-to-sum (-p place)\n",
			       argv[0]);
		exit(-1);
		break;
		};
	};
	if (file == NULL) {
		goto use;
	}

	io = fopen(file, "r");
	if (io == NULL) {
		printf("Can't open '%s' err:%d\n", file, errno);
		return (-1);
	}
	stop = NULL;
	while (fgets(buffer, sizeof(buffer), io) != NULL) {
		readin = strtod(buffer, &stop);
		if (stop == NULL) {
			printf("Huh\n");
			return (-1);
		}
		stop++;
		txfr = strtol(stop, NULL, 0);
		time_of += readin;
		bytes += txfr;
	}
	fclose(io);
	bytes_per_sec = ((double)(bytes * 1.0))/time_of;
	printf("%s %ld\n", place, (unsigned long)bytes_per_sec);
	return (0);
}
