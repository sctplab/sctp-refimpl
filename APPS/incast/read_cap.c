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
#include <sys/types.h>
#include <stdio.h>
#include <pcap/pcap.h>
#include <sys/errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/cdefs.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/sctp.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_constants.h>
#include <sys/time.h>

#define PKT_ARRAY_SIZE 4096

struct pkt_track {
	uint32_t tsn;
	uint32_t cnt_out;
	struct timeval sent;
	int inuse;
};

int beginning=0;
int cnt_outstanding=0;
int time_in_micro = 0;
int ecn_print = 0;

struct pkt_track pkt_arry[PKT_ARRAY_SIZE];

/************************************/
/* shamelessly stolen from head     */
static void
timevalfix(struct timeval *t1)
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

void
timevaladd(struct timeval *t1, const struct timeval *t2)
{

	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}


void
timevalsub(struct timeval *t1, const struct timeval *t2)
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

struct pkt_track *
find_next_entry()
{
	int i;
	for(i=beginning; i<PKT_ARRAY_SIZE; i++) {
		if (pkt_arry[i].inuse == 0) {
			beginning = i;
			pkt_arry[i].inuse = 1;
			return (&pkt_arry[i]);
		}
	}
	for(i=0; i<beginning; i++) {
		if (pkt_arry[i].inuse == 0) {
			beginning = i;
			pkt_arry[i].inuse = 1;
			return (&pkt_arry[i]);
		}
	}
	return (NULL);
}

void
process_data(struct pcap_pkthdr *phdr, 
	     struct sctphdr *sctp, struct sctp_data_chunk *dc)
{
	struct pkt_track *tslot;

	tslot = find_next_entry();
	if (tslot == NULL) {
		printf("Out of space -- eek\n");
		exit(-1);
	}
	tslot->tsn = ntohl(dc->dp.tsn);
	cnt_outstanding++;
	tslot->cnt_out = cnt_outstanding;
	tslot->sent = phdr->ts;
}

void
process_sack(struct pcap_pkthdr *phdr,
	     struct sctphdr *sctp, struct sctp_sack_chunk *sc,
	     int ecnseen, uint32_t ecn_tsn)
{
	int i;
	uint32_t cumtsn;
	struct timeval tv;
	cumtsn = ntohl(sc->sack.cum_tsn_ack);
	for(i=0;i<PKT_ARRAY_SIZE;i++) {
		if (pkt_arry[i].inuse == 0)
			continue;
		if (SCTP_TSN_GE(cumtsn, pkt_arry[i].tsn)) {
			cnt_outstanding--;
			tv = phdr->ts;
			timevalsub(&tv, &pkt_arry[i].sent);
			if (ecn_print) {
				if (ecnseen) {
					if (time_in_micro) {
						uint64_t tim;
						tim = ((tv.tv_sec* 1000000) +  tv.tv_usec);
						printf("%ld.%6.6ld %d %ld\n",
						       (unsigned long)phdr->ts.tv_sec,
						       (unsigned long)phdr->ts.tv_usec,
						       pkt_arry[i].cnt_out,
						       (unsigned long)tim);
					} else {
						printf("%ld.%6.6ld %d %ld.%6.6ld\n",
						       (unsigned long)phdr->ts.tv_sec,
						       (unsigned long)phdr->ts.tv_usec,
						       pkt_arry[i].cnt_out,
						       tv.tv_sec, tv.tv_usec);
					}
				}
			} else {
				if (time_in_micro) {
					uint64_t tim;
					tim = ((tv.tv_sec* 1000000) +  tv.tv_usec);
					printf("%ld.%6.6ld %d %ld\n",
					       (unsigned long)phdr->ts.tv_sec,
					       (unsigned long)phdr->ts.tv_usec,
					       pkt_arry[i].cnt_out,
					       (unsigned long)tim);
				} else {
					printf("%ld.%6.6ld %d %ld.%6.6ld\n",
					       (unsigned long)phdr->ts.tv_sec,
					       (unsigned long)phdr->ts.tv_usec,
					       pkt_arry[i].cnt_out,
					       tv.tv_sec, tv.tv_usec);
				}
			}
			pkt_arry[i].inuse = 0;
		}
	}
}

void
analyze_packet(struct pcap_pkthdr *phdr, uint32_t len, const u_char *p)
{
	struct ether_header *pkt;
	struct ip *iph;
	struct sctphdr *sctp;
	struct sctp_chunkhdr *ch;
	int lenat;
	int ecnseen=0;
	uint32_t ecn_tsn=0;

	pkt = __DECONST(struct ether_header *, p);
	if (ntohs(pkt->ether_type) != ETHERTYPE_IP) {
		return;
	}
	iph = (struct ip *)(pkt + 1);
	if (iph->ip_p != IPPROTO_SCTP) {
		return;
	}
	lenat = sizeof(struct ether_header) + (iph->ip_hl << 2) + sizeof(struct sctphdr);
	sctp = (struct sctphdr *)((char *)iph + (iph->ip_hl << 2));
	ch = (struct sctp_chunkhdr *)(sctp + 1);
retry:
	if (ch->chunk_type == SCTP_DATA) {
		process_data(phdr, sctp, (struct sctp_data_chunk *)ch);
	} else if (ch->chunk_type == SCTP_SELECTIVE_ACK) {
		process_sack(phdr, sctp, (struct sctp_sack_chunk *)ch,
			     ecnseen, ecn_tsn);
	} else if (ch->chunk_type == SCTP_ECN_ECHO) {
		/* move forward past ECN-Echo */
		ecnseen = 1;
		struct sctp_ecne_chunk *ecn;
	
		lenat += ntohs(ch->chunk_length);
		if (lenat > len) {
			return;
		}
		ecn = (struct sctp_ecne_chunk *) ch;
		ecn_tsn = ntohl(ecn->tsn);
		ch = (struct sctp_chunkhdr *)((char *)ch + ntohs(ch->chunk_length));
		goto retry;
	} 

}
int
main(int argc, char **argv)
{
	const u_char *pkt;
	struct pcap_pkthdr *phdr;
	char error_message[PCAP_ERRBUF_SIZE];
	pcap_t *pd;
	int i;
	char *capture=NULL;

	memset(pkt_arry, 0, sizeof(pkt_arry));

	while ((i = getopt(argc, argv, "emc:?")) != EOF) {
		switch (i) {
		case 'e':
			ecn_print = 1;
			break;
		case 'm':
			time_in_micro = 1;
			break;
		case 'c':
			capture = optarg;
			break;
		case '?':
		default:
		use:
			printf("Use %s -c capture-file [-e -m]\n", argv[0]);
			printf("  -e - only sacks with ECNE\n");
			printf("  -m - time of rrt in microseconds\n");
			return (-1);
			break;
		};
	};

	if (capture == NULL) {
		goto use;
	}
	pd = pcap_open_offline(capture, error_message);
	if (pd == NULL) {
		printf("Pcap claims '%s'\n", error_message);
		return (-1);
	}
	while(pcap_next_ex(pd, &phdr, &pkt) > 0) {
		if (phdr->caplen != phdr->len) {
			continue;
		}
		analyze_packet(phdr, phdr->len, pkt);
	}
	pcap_close(pd);
	return (0);
}
