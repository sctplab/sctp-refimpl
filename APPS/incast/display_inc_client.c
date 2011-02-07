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

int infile_size=sizeof(long);

struct incast_peer_outrec32 {
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	int peerno;
	int state;
};

struct incast_peer_outrec64 {
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	int peerno;
	int state;
};


int
read_peer_rec(struct incast_peer_outrec *rec, FILE *io)
{
	int ret;
	struct incast_peer_outrec32 bit32;
	struct incast_peer_outrec64 bit64;
	if (sizeof(time_t) == infile_size) {
		return (fread(&rec, sizeof(struct incast_peer_outrec), 1, io));
	}
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		rec->start.tv_sec = (time_t)bit32.start_tv_sec;
		rec->start.tv_nsec = (long)bit32.start_tv_nsec;
		rec->end.tv_sec = (time_t)bit32.end_tv_sec;
		rec->end.tv_nsec = (long)bit32.end_tv_nsec;
		rec->peerno = bit32.peerno;
		rec->state = bit32.state;
	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 32 bit */
		rec->start.tv_sec = (time_t)bit64.start_tv_sec;
		rec->start.tv_nsec = (long)bit64.start_tv_nsec;
		rec->end.tv_sec = (time_t)bit64.end_tv_sec;
		rec->end.tv_nsec = (long)bit64.end_tv_nsec;
		rec->peerno = bit64.peerno;
		rec->state = bit64.state;
	}
	return(1);
}

struct incast_lead_hdr32 {
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t connected_tv_sec;
	uint32_t connected_tv_nsec;
	uint32_t sending_tv_sec;
	uint32_t sending_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	uint32_t number_servers;
	uint32_t passcnt;
};

struct incast_lead_hdr64 {
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t connected_tv_sec;
	uint64_t connected_tv_nsec;
	uint64_t sending_tv_sec;
	uint64_t sending_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	uint64_t number_servers;
	uint64_t passcnt;
};


int
read_incast_hdr(struct incast_lead_hdr *hdr, FILE *io)
{
	int ret;
	struct incast_lead_hdr32 bit32;
	struct incast_lead_hdr64 bit64;
	if (sizeof(time_t) == infile_size) {
		return (fread(hdr, sizeof(struct incast_lead_hdr), 1, io));
	}
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		hdr->start.tv_sec = (time_t)bit32.start_tv_sec;
		hdr->start.tv_nsec = (long)bit32.start_tv_nsec;
		hdr->connected.tv_sec = (time_t)bit32.connected_tv_sec;
		hdr->connected.tv_nsec = (long)bit32.connected_tv_nsec;
		hdr->sending.tv_sec = (time_t)bit32.sending_tv_sec;
		hdr->sending.tv_nsec = (long)bit32.sending_tv_nsec;
		hdr->end.tv_sec = (time_t)bit32.end_tv_sec;
		hdr->end.tv_nsec = (long)bit32.end_tv_nsec;
		hdr->number_servers = bit32.number_servers;
		hdr->passcnt = bit32.passcnt;

	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 32 bit */
		hdr->start.tv_sec = (time_t)bit64.start_tv_sec;
		hdr->start.tv_nsec = (long)bit64.start_tv_nsec;
		hdr->connected.tv_sec = (time_t)bit64.connected_tv_sec;
		hdr->connected.tv_nsec = (long)bit64.connected_tv_nsec;
		hdr->sending.tv_sec = (time_t)bit64.sending_tv_sec;
		hdr->sending.tv_nsec = (long)bit64.sending_tv_nsec;
		hdr->end.tv_sec = (time_t)bit64.end_tv_sec;
		hdr->end.tv_nsec = (long)bit64.end_tv_nsec;
		hdr->number_servers = bit64.number_servers;
		hdr->passcnt = bit64.passcnt;
	}
	return (1);
}

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
	while ((i = getopt(argc, argv, "r:a?36")) != EOF) {
		switch (i) {
		case '3':
			/* Reading 32 bit mode */
			infile_size = 4;
			break;
		case '6':
			infile_size = 8;
			break;
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
	while(read_incast_hdr(&hdr, io) > 0) {
		for(i=0; i<hdr.number_servers; i++) {
			if (read_peer_rec(&rec, io) < 1) {
				printf("Error hit end and expected %d svr found %d\n",
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
					    (rec.end.tv_nsec > 300000000)) {
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

