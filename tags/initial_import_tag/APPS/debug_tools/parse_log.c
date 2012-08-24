/*-
 * Copyright (c) 2007, by Cisco Systems, Inc. All rights reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

struct part_ip {
	uint8_t   ver;
	uint8_t   tos;
	uint16_t  len;
	uint16_t  id;
        uint16_t  off;
        uint8_t   ttl;
        uint8_t   p;
        uint16_t  sum;
	uint32_t  src;
        uint32_t  dst;
};

struct part_ip6 {
	uint32_t ver;
	uint16_t len;
	uint16_t rest;
};

int
main(int argc, char **argv)
{
	/* 
	 * Buffer layout.
	 * -sizeof this entry (total_len)
	 * -previous end      (value)
	 * -ticks of log      (ticks)
	 * o -ip packet
	 * o -as logged
	 * - where this started (thisbegin)
	 * x <--end points here 
	 * First 4 bytes of buf will have the pkt_end_value.
	 */

	uint8_t buf[SCTP_PACKET_LOG_SIZE+4], *bufp;
	FILE *io;
	int sd;
	int *point;
	int limit, at, cnt=0, end_at, notdone, last_at, ret;
	socklen_t len;
	struct sctp_packet_log { 
		uint32_t datasize;
		uint32_t prev_end;
		uint32_t timestamp;
		/* we should always have at least 20 bytes */
		char data[20];
	} *header;

	if (argc < 2) {
		printf("Use %s file-to-write\n", argv[0]);
		return (0);
	}
	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(sd == -1) {
		/* Can't open */
		printf("Can't open SCTP socket err:%d\n", errno);
		exit(-1);
	}
	len = sizeof(buf);
	ret = getsockopt(sd, IPPROTO_SCTP, SCTP_GET_PACKET_LOG, &buf, &len);
	if(ret < 0) {
		printf("Could not get packet log error:%d\n", errno);
		return (0);
	}
	at = 4;
	limit = SCTP_PACKET_LOG_SIZE+4;
	/* First we back through the buffer, to
	 * find the first entry. Then we write each
	 * one out in order.
	 */

	/* get end out */
	point = (int *)buf;
	end_at = *point;
	bufp = &buf[4];
	printf("end is at %d\n", end_at);
	if((end_at > sizeof(buf) || (end_at < 4))) {
		printf("nothing to print, end_at is %d\n", end_at);
		return(0);
	}
	point = (int *)&bufp[end_at];
	point--;
	notdone = 1;
	last_at = 0;
	while(notdone) {
		at = *point;
		if ((at > sizeof(buf)) || (at < 4) || (at == end_at)) {
			notdone = 0;
			printf("Found %d records and we start at %d\n", cnt,  last_at);
			continue;
		}
		cnt++;
		header = (struct sctp_packet_log *)&bufp[at];
		last_at = at;
		at = header->prev_end;
		if ((at > sizeof(buf)) || (at < 4) || (at == end_at)) {
			notdone = 0;
			printf("Found %d records and we start at %d - 2\n", cnt, last_at);
			continue;
		}
		/* Go to where the header says and decrement to
		 * get to end of packet (and our start mark).
		 */
		point = (int *)&bufp[at];
		point--;
	}
	at = last_at;
	printf("We have %d records at:%d to write out\n", cnt, at);
	if (cnt <= 0) {
		return(0);
	}
	io = fopen(argv[1], "w+");
	if (io == NULL) {
		printf("Can't open file %s for output error:%d\n", argv[1], 
		       errno);
		return (0);
	}

	while (cnt > 0) {
		header = (struct sctp_packet_log *)&bufp[at];
		printf("header:%p | %d - %d bytes (including pad), ts:%x ipversion:%x\n",
		       header,
		       cnt, 
		       header->datasize, 
		       header->timestamp, 
		       ((header->data[0] >> 4) & 0x0f));
		if((((header->data[0] >> 4) & 0x0f)) == 4) {
			struct part_ip *ip;
			ip = (struct part_ip *)header->data;
			printf("ver:%2.2x tos:%2.2x len:%d\n",
			       ip->ver,
			       ip->tos,
			       ip->len);
			printf("id:%4.4d off:%4.4x ttl:%x proto:%d\n",
			       ip->id, ip->off, ip->ttl, ip->p);
			printf("sum:%d src:%x dest:%x\n",
			       ip->sum,
			       ip->src,
			       ip->dst);
		}
		if( header->datasize > limit) {
			printf("strange, size > limit:%d\n", limit);
			break;
		}
		fwrite(&bufp[at], header->datasize, 1, io);
		at += header->datasize;
		cnt--;
	}
	printf("Done\n");
	return (0);
}

