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

struct incast_peer **peers;
struct timespec begin;
time_t cutoff_time=0;
int divsor = 1024;
static int
retrieve_ele_sink_record(struct incast_control *ctrl,
			 struct elephant_sink_rec *sink,
			 struct incast_peer *peer, 
			 char *dir, 
			 int number_of_bytes)
{
	FILE *io;
	struct elephant_sink_rec lsink;
	char loadfile[1024];
	int target_size;

	target_size = (number_of_bytes/ctrl->size + 1) * ctrl->size;

	sprintf(loadfile, "%s/%s_ele.out", dir, peer->peer_name);
	io = fopen(loadfile, "r");
	if (io == NULL) {
		printf("Can't open file %s - err:%d\n", loadfile, errno);
		return (-1);
	}
	while(read_a_sink_rec(&lsink, io, peer->long_size) > 0) {
		if (lsink.number_bytes == target_size) {
			fclose(io);
			memcpy(sink, &lsink, sizeof(lsink));
			return (0);
		}
	}
	fclose(io);
	return (-1);
}

FILE *out;
int raw_bytes=0;
int one_print_per = 0;

void
display_an_entry(struct incast_peer *peer, 
		 struct incast_peer_outrec *rec, 
		 struct elephant_sink_rec *sink,
		 struct elephant_lead_hdr *hdr)
{
	double bps, tim, byt;
	long i;
	if (peer->exclude) 
		return;

	if (cutoff_time && (hdr->start.tv_sec > cutoff_time)) { 
		return;
	}

	bps = 0;
	tim = ((1.0 * sink->mono_end.tv_sec) + ((sink->mono_end.tv_nsec * 1.0)/1000000000));
	if (tim > 0.0) {
		byt = 1.0 * sink->number_bytes;
		bps =  byt/tim;
	} else {
		printf("Invalid time\n");
	}
	if (raw_bytes) {
		fprintf(out, "%f %ld\n",
			tim,
			(unsigned long)sink->number_bytes);

	} else if (one_print_per) {
			fprintf(out, "%f %ld %ld\n",
				tim,
				(unsigned long)sink->number_bytes,
				(unsigned long)hdr->start.tv_sec
				);

	} else {
		for(i=0; i<sink->mono_end.tv_sec; i++) {

			fprintf(out, "%ld %ld\n",
				(unsigned long)(hdr->start.tv_sec - begin.tv_sec + i),
				(unsigned long)(bps/(divsor * 1.0)));
		}
	}
	timespecadd(&hdr->start, &sink->mono_end);
}

#define EXCLUDE_MAX 10
int 
main(int argc, char **argv)
{
	int i, j;
	FILE *io;
	struct incast_control ctrl;
	char loadfile[1024];
	char *directory=NULL;
	char *config=NULL;
	struct incast_peer *peer;
	int siz, secs=0;
	struct elephant_lead_hdr hdr;
	struct incast_peer_outrec rec;
	struct elephant_sink_rec sink;
	char *exclude_list[EXCLUDE_MAX];
	int excl_cnt=0;

	out = stdout;
	memset(exclude_list, 0, sizeof(exclude_list));
	begin.tv_sec = 0;
	begin.tv_nsec = 0;
	while ((i = getopt(argc, argv, "c:d:s:b:x:w:rC:OD:")) != EOF) {
		switch (i) {
		case 'D':
			divsor = strtol(optarg, NULL, 0);
			if (divsor <= 0) {
				printf("Error divsor must be 1 or > \n");
				divsor = 1024;
			}
			break;
		case 'O':
			one_print_per = 1;
			break;
		case 'C':
			cutoff_time = strtol(optarg, NULL, 0);
			break;
		case 'r':
			raw_bytes = 1;
			break;
		case 'w':
			out = fopen(optarg, "w+");
			if (out == NULL) {
				printf("Can't open %s for output - sorry err:%d\n",
				       optarg, errno);
				out = stdout;
			}
			break;
		case 'x':
			if (excl_cnt < EXCLUDE_MAX ) {
				exclude_list[excl_cnt]= optarg;
				excl_cnt++;
			} else {
				printf("Sorry - Max exclude %d reached\n", 
				       excl_cnt);
			}
			break;
		case 'b':
			begin.tv_sec = (time_t)strtoul(optarg, NULL, 0);
			break;
		case 's':
			secs = strtol(optarg, NULL, 0);
			break;
		case 'c':
			config = optarg;
			break;
		case 'd':
			directory = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -c config -d directory [ -C cutoff -O -w out -s sec -b begin -x exclude -r  -D divsor]\n", argv[0]);
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
	parse_config_file(&ctrl, config, DEFAULT_ELEPHANT_PORT);
	if (ctrl.hostname == NULL) {
		printf("Sorry no hostname found in loaded config\n");
		return (-1);
	}

	siz = sizeof(struct incast_peer *) * ctrl.number_server;
	peers = malloc(siz);
	memset(peers, 0, siz);
	i = 0;
	LIST_FOREACH(peer, &ctrl.master_list, next) {
		peers[i] = peer;
		i++;
		peer->exclude = 0;
		/* See if its in the exclude list */
		if (excl_cnt) {
			for(j=0; j<excl_cnt; j++) {
				if (strcmp(exclude_list[j], 
					   peer->peer_name) == 0) {
					peer->exclude = 1;
				}
			}
		}
	}

	sprintf(loadfile, "%s/%s_ele_src.out", directory, ctrl.hostname);
	io = fopen(loadfile, "r");
	if (io == NULL) {
		printf("Can't open file %s - err:%d\n", loadfile, errno);
		return (-1);
	}
	while(read_ele_hdr(&hdr, io, ctrl.long_size) > 0) {
		/* Add time compensation if any */
		hdr.start.tv_sec += secs;
		for(i=0; i<hdr.number_servers; i++) {
			if (read_peer_rec(&rec, io, ctrl.long_size) < 1) {
				printf("Error hit end and expected %d svr found %d\n",
				       hdr.number_servers, i);
				return (-1);
			}
			timespecsub(&rec.end, &rec.start);
			if (rec.peerno >= ctrl.number_server) {
				printf("peer exceeds bounds %d >= %d\n",
				       rec.peerno, ctrl.number_server);
				       
			} else {
				if(retrieve_ele_sink_record(&ctrl, &sink, 
							 peers[rec.peerno], 
							 directory, 
							    hdr.number_of_bytes)) {
					printf("Could not retrieve sink record for peer %s record\n", 
					       peers[rec.peerno]->peer_name);
					continue;
				}
				timespecsub(&sink.mono_end, &sink.mono_start);
				display_an_entry(peers[rec.peerno], &rec, &sink, &hdr);

			}
		}
	}
	fclose(io);
	return (0);
}

