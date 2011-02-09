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
	struct incast_control ctrl;
	char loadfile[1024];
	char *directory=NULL;
	char *config=NULL;
	struct elephant_lead_hdr hdr;
	struct incast_peer_outrec rec;
	struct timespec tmp;
	while ((i = getopt(argc, argv, "c:d:")) != EOF) {
		switch (i) {
		case 'c':
			config = optarg;
			break;
		case 'd':
			directory = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -c config -d directory\n", argv[0]);
			exit(0);
			break;
		};
	};
	if (config == NULL) {
		printf("No -c config\n");
		goto use;
	}
	if (directory == NULL) {
		printf("No -d directory\n");
		goto use;
	}
	parse_config_file(&ctrl, config);
	if (ctrl.hostname == NULL) {
		printf("Sorry no hostname found in loaded config\n");
		return (-1);
	}
	sprintf(loadfile, "%s/%s_ele_src.out", directory, ctrl.hostname);
	printf("Loading file %s\n", loadfile);
	io = fopen(loadfile, "r");
	if (io == NULL) {
		printf("Can't open file %s - err:%d\n", loadfile, errno);
		return (-1);
	}
	while(read_ele_hdr(&hdr, io, ctrl.long_size) > 0) {
		printf("Started %ld.%9.9ld %d cnt:%d bytes\n",
		       (unsigned long)hdr.start.tv_sec, (unsigned long)hdr.start.tv_nsec,
		       hdr.number_servers, hdr.number_of_bytes);
		for(i=0; i<hdr.number_servers; i++) {
			if (read_peer_rec(&rec, io, ctrl.long_size) < 1) {
				printf("Error hit end and expected %d svr found %d\n",
				       hdr.number_servers, i);
				return (-1);
			}
			timespecsub(&rec.end, &rec.start);
			printf("%d:%ld.%9.9ld\n",
			       rec.peerno,
			       (unsigned long)rec.end.tv_sec,
			       (unsigned long)rec.end.tv_nsec);
		}
		tmp = hdr.end;
		timespecsub(&tmp, &hdr.start);
		printf("Ended  %ld.%9.9ld (%ld.%9.9ld)\n", 
		       (unsigned long)hdr.end.tv_sec, 
		       (unsigned long)hdr.end.tv_nsec,
		       (unsigned long)tmp.tv_sec,
		       (unsigned long)tmp.tv_nsec);
	}
	fclose(io);
	return (0);
}

