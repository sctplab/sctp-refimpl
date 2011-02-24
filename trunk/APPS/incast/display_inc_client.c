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
int milli_second = 0;
int 
main(int argc, char **argv)
{
	int i;
	FILE *io;
	FILE *output = stdout;
	char *dir=NULL;
	char *conf=NULL;
	char *prefix=NULL;
	uint64_t above_ms = 0;
	char infile[1024];
	struct incast_lead_hdr hdr;
	struct incast_peer_outrec rec;
	struct incast_control ctrl;
	struct incast_peer *peer;
	struct timespec highest;
	
	while ((i = getopt(argc, argv, "d:c:p:?ma:")) != EOF) {
		switch (i) {
		case 'm':
			milli_second = 1;
			break;
		case 'a':
			above_ms = ((uint64_t)strtol(optarg, NULL, 0) * 
				    1000000);
			break;
		case 'd':
			dir = optarg;
			break;
		case 'c':
			conf = optarg;
			break;
		case 'p':
			prefix = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -d directory -c config [-p prefix -a abovems]\n", argv[0]);
			exit(0);
			break;
		};
	};
	if (dir == NULL) {
		goto use;
	}
	if (conf == NULL) {
		goto use;
	}
	parse_config_file(&ctrl, conf, DEFAULT_SVR_PORT);
	if (ctrl.hostname == NULL) {
		printf("Sorry no hostname found in loaded config\n");
		return (-1);
	}	
	if (ctrl.number_server == 0) {
		printf("Control file has no peers\n");
		return (-1);


	}
	LIST_FOREACH(peer, &ctrl.master_list, next) {
		sprintf(infile, "%s/incast_%s.out",
			dir,
			peer->peer_name); 
		io = fopen(infile, "r");
		if (io == NULL) {
			printf("Can't open file %s - err:%d\n", infile, errno);
			continue;
		}
		if (prefix) {
			sprintf(infile, "%s%s_incast.graph", 
				prefix,
				peer->peer_name);
			output = fopen(infile, "w+");
			if (output == NULL) {
				printf("Can't open '%s' to write err:%d\n", 
				       infile, errno);
				return (-1);
			}
			
		}
		while(read_incast_hdr(&hdr, io, peer->long_size) > 0) {
			highest.tv_sec = 0;
			highest.tv_nsec = 0;
			for(i=0; i<hdr.number_servers; i++) {
				if (read_peer_rec(&rec, io, peer->long_size) < 1) {
					fprintf(output, 
						"Error hit end and expected %d svr found %d\n",
						hdr.number_servers, i);
					return (-1);
				}
				if (rec.state == SRV_STATE_COMPLETE) {
					timespecsub(&rec.end, &rec.start);
					if (rec.end.tv_sec > highest.tv_sec) {
						highest = rec.end;
					} else if (rec.end.tv_sec == highest.tv_sec) {
						if (rec.end.tv_nsec > highest.tv_nsec) {
							highest = rec.end;
						}
					}
				}
			}
			if (above_ms) {
				if (highest.tv_sec ||
				    (highest.tv_nsec > above_ms)) {
					goto print_it;
				}
			} else {
			print_it:
				if (milli_second) {
					fprintf(output, "%d %ld\n",
						hdr.passcnt,
						(unsigned long)((highest.tv_sec * 1000) +
								(highest.tv_nsec/1000000)));
				} else {
					fprintf(output, "%d %ld.%9.9ld\n",
						hdr.passcnt,
						(unsigned long)highest.tv_sec,
						(unsigned long)highest.tv_nsec);
				}
			}

		}
		fclose(io);
	}
	return (0);
}

