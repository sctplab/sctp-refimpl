/*******************************************************************
 *      
 *   Description: Test application for SCTP
 *
 *   Date:   06/15/99
 *
 */
/******************************************************************
*	Test application for SCTP
******************************************************************/

/******************************************************************
* EXTERNAL REFERENCES
*******************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <poll.h>
#include <errno.h>
#include <setjmp.h>
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
	uint8_t buf[SCTP_PACKET_LOG_SIZE];
	FILE *io;
	int limit, at, cnt=0;
	struct sctp_packet_log { 
		uint32_t datasize;
		uint32_t timestamp;
		/* we should always have at least 20 bytes */
		char data[20];
	} *header;
	if (argc < 2) {
		printf("Use %s file\n", argv[0]);
		return (0);
	}
	io = fopen(argv[1], "r");
	if(io == NULL) {
		printf("Can't open %s error:%d\n", argv[1], errno);
		return (0);
	}
	limit = fread(buf, 1, SCTP_PACKET_LOG_SIZE, io);
	if(limit < 0) {
		printf("Could not read error %d\n", errno);
		fclose(io);
		return (0);
	}
	printf("Read in %d bytes\n", limit);
	header = (struct sctp_packet_log *)buf;
	at = 0;
	while (at < limit) {
		printf("header:%p | %d - %d bytes (including pad), ts:%x ipversion:%x\n",
		       header,
		       cnt, 
		       header->datasize, 
		       header->timestamp, 
		       ((header->data[0] >> 4) & 0x0f));
		cnt++;
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
		at += header->datasize;
		header = (struct sctp_packet_log *)(&buf[at]);
	}
	printf("Done\n");
	return (0);
}

