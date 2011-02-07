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
	int incomplete=0;
	struct incast_lead_hdr hdr;
	struct incast_peer_outrec rec;
	int a=0;
	while ((i = getopt(argc, argv, "r:a?")) != EOF) {
		switch (i) {
		case 'a':
			a = 1;
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
	while(fread(&hdr, sizeof(hdr), 1, io) > 0) {
		for(i=0; i<hdr.number_servers; i++) {
			if (fread(&rec, sizeof(rec), 1, io) < 1) {
				printf("Error hit end and expeced %d svr found %d\n",
				       hdr.number_servers, i);
				return (-1);
			}
			if (rec.state == SRV_STATE_COMPLETE) {
				timespecsub(&rec.end, &rec.start);
				if (a == 0) {
				print_it:
					printf("%d:%ld.%9.9ld\n",
					       hdr.passcnt,
					       (unsigned long)rec.end.tv_sec,
					       (unsigned long)rec.end.tv_nsec);
				} else {
					if ((rec.end.tv_sec) ||
					    (rec.end.tv_nsec > 30000000)) {
						goto print_it;
					}
				}
				       
			} else {
				incomplete++;
			}
		}
	}
	fclose(io);
	return (0);
}

