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

int
main(int argc, char **argv)
{
	uint8_t buf[SCTP_PACKET_LOG_SIZE];
	FILE *io, *out;
	int ret;
	struct pcap_file_header head;
	uint32_t loopback = 0x02000000;
	struct pcap_pkthdr phead;

	int limit, at, wlen, cnt=0;
	struct sctp_packet_log { 
		uint32_t datasize;
		uint32_t timestamp;
		/* we should always have at least 20 bytes */
		char data[20];
	} *header;
	if (argc < 3) {
		printf("Use %s file out\n", argv[0]);
		return (0);
	}
	io = fopen(argv[1], "r");
	if(io == NULL) {
		printf("Can't open %s error:%d\n", argv[1], errno);
		return (0);
	}
	out = fopen(argv[2], "w+");
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
	head.magic = 0x1a2b3c4b;
	head.version_major = PCAP_VERSION_MAJOR;
	head.version_minor = PCAP_VERSION_MINOR;
	head.thiszone = 0;
	head.sigfigs = 0;
	head.snaplen = 0x0000ffff;
	head.linktype = 0;
	if( (ret=fwrite(&head, sizeof(head), 1, out)) < 1) {
		printf("Can't write header ret:%d errno=%d\n",
		       ret,errno);
		return(0);
	}
	header = (struct sctp_packet_log *)buf;
	at = 0;
	while (at < limit) {
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
		phead.len = phead.caplen = header->datasize + sizeof(uint32_t);
	
		if( header->datasize > limit) {
			printf("strange, size > limit:%d\n", limit);
			break;
		}	
		if((ret=fwrite(&phead, sizeof(phead), 1, out)) < 1) {
			printf("Can't write phead ret:%d errno=%d\n",
			       ret,errno);
			fclose(out);
			return(0);
		}
		wlen = header->datasize - (sizeof(uint32_t) * 2);
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

