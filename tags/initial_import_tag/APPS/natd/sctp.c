/*
 * natd - Network Address Translation Daemon for FreeBSD.
 *
 * This software is provided free of charge, with no 
 * warranty of any kind, either expressed or implied.
 * Use at your own risk.
 * 
 * You may copy, modify and distribute this software (icmp.c) freely.
 *
 * Michael Tuexen <tuexen@fh-muenster.de>
 *
 * $FreeBSD: src/sbin/natd/icmp.c,v 1.7 2004/07/04 12:53:54 phk Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

#include <netdb.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <machine/in_cksum.h>
#include <netinet/sctp_header.h>

#include <alias.h>

#include "natd.h"

int SendSctpAbort (int sock, struct ip* SctpPacket)
{
	struct sctphdr *sctphdr;
	struct sctp_chunkhdr *sctp_chunkhdr;
	int wrote, bytes;
	struct sockaddr_in	addr;
	char sctpBuf[IP_MAXPACKET];

	if(SctpPacket->ip_p != IPPROTO_SCTP)
		return 0;

	/* FIXME: Handle the case of no common header or no chunk */
	/* FIXME: Look not only at the first chunk? */
	sctphdr = (struct sctphdr *) ((char*)SctpPacket + (SctpPacket->ip_hl << 2));
	sctp_chunkhdr = (struct sctp_chunkhdr*)((char*) sctphdr + sizeof(struct sctp_chunkhdr));
	
	switch (sctp_chunkhdr->chunk_type) {
	
	case SCTP_INITIATION:
	{
		struct ip *ip_abort;
		struct sctphdr *sctphdr_abort;
		struct sctp_chunkhdr *sctp_chunkhdr_abort;
		struct sctp_init *sctp_init;
		
		sctp_init = (struct sctp_init*)((char*)sctphdr + sizeof(*sctphdr) + sizeof(struct sctp_chunkhdr*));

		ip_abort = (struct ip*)sctpBuf;

		ip_abort->ip_v = 4;
		ip_abort->ip_hl = 5;
		ip_abort->ip_tos = 0;
		ip_abort->ip_len = htons(36);
		ip_abort->ip_id = 0;
		ip_abort->ip_off = 0;
		ip_abort->ip_ttl = 255;
		ip_abort->ip_p = IPPROTO_SCTP;
		ip_abort->ip_dst = SctpPacket->ip_src;
		ip_abort->ip_src = SctpPacket->ip_dst;
		ip_abort->ip_sum = 0;

		sctphdr_abort = (struct sctphdr*) ((char*)ip_abort + sizeof(*ip_abort));


		sctphdr_abort->src_port = sctphdr->dest_port;
		sctphdr_abort->dest_port = sctphdr->src_port;
		sctphdr_abort->v_tag = sctp_init->initiate_tag;
		sctphdr_abort->checksum = 0; /* FIXME compute CRC32C */
		
		sctp_chunkhdr_abort = (struct sctp_chunkhdr*)((char*) sctphdr_abort + sizeof(*sctphdr_abort));
		
		sctp_chunkhdr_abort->chunk_type = SCTP_ABORT_ASSOCIATION;
		sctp_chunkhdr_abort->chunk_flags = 0; /* M-Bit */
		sctp_chunkhdr_abort->chunk_length = htons(sizeof(struct sctp_abort_chunk));

		addr.sin_family	= AF_INET;
		addr.sin_len    = sizeof(struct sockaddr_in);
		addr.sin_addr	= SctpPacket->ip_dst;
		addr.sin_port	= 0;
		bytes = ntohs (ip_abort->ip_len);
	}
	default:
		/* nothing */
		break;
	}
	wrote = sendto (sock, 
		sctpBuf,
		bytes,
		0,
		(struct sockaddr*) &addr,
		sizeof(struct sockaddr_in));
	if (wrote != bytes)
		Warn ("Cannot send ICMP message.");
}

