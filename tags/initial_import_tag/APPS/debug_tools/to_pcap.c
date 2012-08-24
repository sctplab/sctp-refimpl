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
#include <pcap.h>
struct part_ip {
	uint8_t ver;
	uint8_t tos;
	uint16_t len;
	uint16_t ip_id;
	uint16_t ip_off;
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
	FILE *io, *out;
	int ret, i;
	struct pcap_file_header head;
	uint32_t loopback = 0x00000002;
	struct pcap_pkthdr phead;
	struct part_ip *ip;
	struct part_ip6 *ip6;
	uint16_t len;
	char *infile=NULL, *outfile=NULL;
	int byte_order_rev=0;

	int limit, at, wlen, cnt=0;
	struct sctp_packet_log { 
		uint32_t datasize;
		uint32_t prev;
		uint32_t timestamp;
		/* we should always have at least 20 bytes */
		char data[20];
	} *header;

	while ((i = getopt(argc,argv,"i:o:e?")) != EOF) {
		switch (i) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'e':
			byte_order_rev = 1;
			break;
		default:
		case '?':
			goto error_out;
		}
	}
	if((infile == NULL) || (outfile == NULL) ) {
	error_out:
		printf("Use %s -i input-file -o output-file [-e]\n", argv[0]);
		printf(" -e is for dumps generated on a non intel byte order machine\n");
		return (-1);
	}
	io = fopen(infile, "r");
	if(io == NULL) {
		printf("Can't open %s error:%d\n", argv[1], errno);
		return (0);
	}
	out = fopen(outfile, "w+");
	if (out == NULL) {
		printf("Can't open %s error:%d\n", argv[2], errno);
		return (0);
	}
	limit = fread(buf, 1, SCTP_PACKET_LOG_SIZE, io);
	if(limit < 0) {
		printf("Could not read error %d\n", errno);
		fclose(io);
		return (0);
	}
	printf("Read in %d bytes\n", limit);
	/* build pcap header and write */
	if (byte_order_rev) {
		head.magic = htonl(0xa1b2c3d4);
		head.version_major = htons(PCAP_VERSION_MAJOR);
		head.version_minor = htons(PCAP_VERSION_MINOR);
		head.snaplen = htonl(0x0000ffff);
	} else {
		head.magic = 0xa1b2c3d4;
		head.version_major = PCAP_VERSION_MAJOR;
		head.version_minor = PCAP_VERSION_MINOR;
		head.snaplen = 0x0000ffff;
	}
	head.thiszone = 0;
	head.sigfigs = 0;


	head.linktype = 0;
	if( (ret=fwrite(&head, sizeof(head), 1, out)) < 1) {
		printf("Can't write header ret:%d errno=%d\n",
		       ret,errno);
		return(0);
	}
	header = (struct sctp_packet_log *)buf;
	at = 0;
	while (at < limit) {
		if(byte_order_rev) {
			int x;
			x = ntohl(header->datasize);
			header->datasize = x;
			x = ntohl(header->timestamp);
			header->timestamp = x;
		}
		printf("%d - %d bytes (including pad), ts:%x ipversion:%x\n",
		       cnt, 
		       header->datasize, 
		       header->timestamp, 
		       ((header->data[0] >> 4) & 0x0f));
		cnt++;
		phead.ts.tv_sec = header->timestamp/1000000;
		if(header->timestamp > 1000000) {
			phead.ts.tv_usec = header->timestamp % 1000000;
		}
		if( header->datasize > limit) {
			printf("strange, size > limit:%d\n", limit);
			break;
		}	
		if(((header->data[0] & 0xf0) >> 4) == 4) {
			uint16_t tmp;
			ip = (struct part_ip *) header->data;
			if(byte_order_rev == 0) {
				len = ip->len;
				tmp = htons(ip->ip_off);
				ip->ip_off = tmp;
				ip->len = htons(header->datasize-16);
			} else {
				len = ntohs(ip->len);

			}
			len = wlen = header->datasize-16;
			wlen = ((((len)+3) >> 2) << 2);
		} else if (((header->data[0] & 0xf0) >> 4) == 6) {
			ip6 = (struct part_ip6 *)header->data;
			if(byte_order_rev) 
				len = ntohs(ip6->len);
			else
				len = ip6->len;
			wlen = header->datasize - 12;
		} else {
			printf("Not v6 or v4?\n");
			break;
		}
		if(byte_order_rev) 
			phead.len = phead.caplen = htonl(wlen + 4);
		else
			phead.len = phead.caplen = wlen + 4;

		if((ret=fwrite(&phead, sizeof(phead), 1, out)) < 1) {
			printf("Can't write phead ret:%d errno=%d\n",
			       ret,errno);
			fclose(out);
			return(0);
		}
		if ((ret=fwrite(&loopback, sizeof(loopback), 1, out)) < 1) {
			printf("Can't write encaps ret:%d errno=%d\n",
			       ret,errno);
			fclose(out);
			return(0);
		}
		if ((ret=fwrite(header->data, wlen, 1, out)) < 1) {
			printf("Can't write body ret:%d errno=%d\n",
			       ret,errno);
			fclose(out);
			return(0);
		}
		at += header->datasize;
		header = (struct sctp_packet_log *)(&buf[at]);
	}
	printf("pcap file complete\n");
	fclose(out);
	fclose(io);
	return (0);
}

