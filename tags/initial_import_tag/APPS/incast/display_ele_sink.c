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


int 
main(int argc, char **argv)
{
	int i;
	FILE *io;
	char *infile=NULL;
	int infile_size=sizeof(long);
	struct elephant_sink_rec sink;
	while ((i = getopt(argc, argv, "r:?36")) != EOF) {
		switch (i) {
		case '3':
			/* Reading 32 bit mode */
			infile_size = 4;
			break;
		case '6':
			infile_size = 8;
			break;
		case 'r':
			infile = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -r infile\n", argv[0]);
			exit(0);
			break;
		};
	};
	if (infile == NULL) {
		goto use;
	}
	io = fopen(infile, "r");
	if (io == NULL) {
		printf("Can't open file %s - err:%d\n", infile, errno);
		return (-1);
	}
	while(read_a_sink_rec(&sink, io, infile_size) > 0) {
		print_an_address((struct sockaddr *)&sink.from, 0);
		timespecsub(&sink.mono_end, &sink.mono_start);
		printf(" - bytes:%d time:%ld.%9.9ld\n",
		       sink.number_bytes,
		       (unsigned long)sink.mono_end.tv_sec, 
		       (unsigned long)sink.mono_end.tv_nsec);
	}
	fclose(io);
	return (0);
}

