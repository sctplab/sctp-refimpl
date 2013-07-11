/*-
 * Copyright (c) 2009-2011, by Michael Tuexen. All rights reserved.
 * Copyright (c) 2009-2011, by Randall Stewart. All rights reserved.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sysctl.h>

int main(int argc, char *argv[])
{
#define ADDRSTRLEN 46
	caddr_t buf;
	unsigned int offset;
	struct xsctp_inpcb *xinp;
	struct xsctp_tcb *xstcb;
	struct xsctp_laddr *xladdr;
	struct xsctp_raddr *xraddr;
	char buffer[ADDRSTRLEN];
	char giant_buffer[20480];
	sa_family_t family;
	void *addr;
	struct sctpstat stat;
	size_t len = sizeof(giant_buffer);
	unsigned int cnt = 0;
	int no_stats=0;
	union sctp_sockstore primary_addr;
	int is_primary_addr;

	if (argc > 1) {
		if (strncmp(argv[1], "-n", 2) == 0) {
			no_stats = 1;
		}
	}

	if (sysctlbyname("net.inet.sctp.stats", giant_buffer, 
			 &len, NULL, 0) < 0) {
		printf("Error %d (%s) could not get the stat\n", errno, strerror(errno));
		return(0);
	}
	memcpy(&stat, giant_buffer, sizeof(stat));
	if (sizeof(stat) != len) {
		printf("Warning - mis-aligned? retlen:%ld statlen:%ld\n",
		       (unsigned long)len, (unsigned long)sizeof(stat));
	}

#define p(f, n)                                                        \
	printf("%-10.10s=%08lX%s", n, stat.f, ((cnt++&3)==3)?"\n":" ")
#define nl(n)                                                          \
	do {                                                           \
		printf("%s%s\n", ((cnt&3)==0)?"":"\n", n);             \
		cnt = 0;                                               \
	} while(0)

	if (no_stats == 0) {
		nl("MIB");
		p(sctps_currestab,           "currentestab");
		p(sctps_activeestab,         "activeestab");
		p(sctps_restartestab,        "restartestab");
		p(sctps_collisionestab,      "collisionestab");
		p(sctps_passiveestab,        "passiveestab");
		p(sctps_aborted,             "aborted");
		p(sctps_shutdown,            "shutdown");
		p(sctps_outoftheblue,        "outoftheblue");
		p(sctps_checksumerrors,      "checksumerrors");
		p(sctps_outcontrolchunks,    "outcontrolchunks");
		p(sctps_outorderchunks,      "outorderchunks");
		p(sctps_outunorderchunks,    "outunorderchunks");
		p(sctps_incontrolchunks,     "incontrolchunks");
		p(sctps_inorderchunks,       "inorderchunks");
		p(sctps_inunorderchunks,     "inunorderchunks");
		p(sctps_fragusrmsgs,         "fragusrmsg");
		p(sctps_reasmusrmsgs,        "reasmusrmsg");
		p(sctps_outpackets,          "outpackets");
		p(sctps_inpackets,           "inpackets");
		nl("RECV");
		p(sctps_recvpackets,         "packets");
		p(sctps_recvdatagrams,       "datagrams");
		p(sctps_recvpktwithdata,     "pktwithdata");
		p(sctps_recvsacks,           "sacks");
		p(sctps_recvdata,            "data");
		p(sctps_recvdupdata,         "dupdata");
		p(sctps_recvheartbeat,       "heartbeat");
		p(sctps_recvheartbeatack,    "heartbeatack");
		p(sctps_recvecne,            "ecne");
		p(sctps_recvauth,            "auth");
		p(sctps_recvauthmissing,     "authmissing");
		p(sctps_recvivalhmacid,      "ivalhmacid");
		p(sctps_recvivalkeyid,       "ivalkeyid");
		p(sctps_recvauthfailed,      "authfailed");
		p(sctps_recvexpress,         "expressd");
		p(sctps_recvexpressm,        "expressdm");
		p(sctps_recvnocrc,           "nocrc");
		p(sctps_recvhwcrc,           "hwcrc");
		p(sctps_recvswcrc,           "swcrc");
		nl("SEND");
		p(sctps_sendpackets,         "packets");
		p(sctps_sendsacks,           "sacks");
		p(sctps_senddata,            "data");
		p(sctps_sendretransdata,     "retransdata");
		p(sctps_sendfastretrans,     "fastretrans");
		p(sctps_sendmultfastretrans, "multfastretrans");
		p(sctps_sendheartbeat,       "heartbeat");
		p(sctps_sendecne,            "ecne");
		p(sctps_sendauth,            "auth");
		p(sctps_senderrors,           "ifp:io_errors");
		p(sctps_sendnocrc,           "nocrc");
		p(sctps_sendhwcrc,           "hwcrc");
		p(sctps_sendswcrc,           "swcrc");
		nl("PDRP");
		p(sctps_pdrpfmbox,           "fmbox");
		p(sctps_pdrpfehos,           "fehos");
		p(sctps_pdrpmbda,            "mbda");
		p(sctps_pdrpmbct,            "mbct");
		p(sctps_pdrpbwrpt,           "bwrpt");
		p(sctps_pdrpcrupt,           "crupt");
		p(sctps_pdrpnedat,           "nedat");
		p(sctps_pdrppdbrk,           "pdbrk");
		p(sctps_pdrptsnnf,           "tsnnf");
		p(sctps_pdrpdnfnd,           "dnfnd");
		p(sctps_pdrpdiwnp,           "diwnp");
		p(sctps_pdrpdizrw,           "dizrw");
		p(sctps_pdrpbadd,            "badd");
		p(sctps_pdrpmark,            "mark");
		nl("TIMEOUT");
		p(sctps_timoiterator,        "iterator");
		p(sctps_timodata,            "data");
		p(sctps_timowindowprobe,     "windowprobe");
		p(sctps_timoinit,            "init");
		p(sctps_timosack,            "sack");
		p(sctps_timoshutdown,        "shutdown");
		p(sctps_timoheartbeat,       "heartbeat");
		p(sctps_timocookie,          "cookie");
		p(sctps_timosecret,          "secret");
		p(sctps_timopathmtu,         "pathmtu");
		p(sctps_timoshutdownack,     "shutdownack");
		p(sctps_timoshutdownguard,   "shutdownguard");
		p(sctps_timostrmrst,         "strmrst");
		p(sctps_timoearlyfr,         "earlyfr");
		p(sctps_timoasconf,          "asconf");
		p(sctps_timoautoclose,       "autoclose");
		p(sctps_timoassockill,       "assockill");
		p(sctps_timoinpkill,         "inpkill");
		nl("OTHER");
		p(sctps_hdrops,              "hdrops");
		p(sctps_badsum,              "badsum");
		p(sctps_noport,              "noport");
		p(sctps_badvtag,             "badvtag");
		p(sctps_badsid,              "badsid");
		p(sctps_nomem,               "nomem");
		p(sctps_fastretransinrtt,    "fastretransinrtt");
		p(sctps_markedretrans,       "markedretrans");
		p(sctps_naglesent,           "naglesent");
		p(sctps_naglequeued,         "naglequeued");
		p(sctps_maxburstqueued,      "maxburstqueued");
		p(sctps_ifnomemqueued,       "ifnomemqueued");
		p(sctps_windowprobed,        "windowprobed");
		p(sctps_lowlevelerr,         "lowlevelerr");
		p(sctps_lowlevelerrusr,      "lowlevelerrusr");
		p(sctps_datadropchklmt,      "datadropchklmt");
		p(sctps_datadroprwnd,        "datadroprwnd");
		p(sctps_ecnereducedcwnd,     "ecnereducedcwnd");
		p(sctps_vtagexpress,         "vtagexpress");
		p(sctps_vtagbogus,           "vtagbogus");
		p(sctps_primary_randry,      "pri.randry");
		p(sctps_cmt_randry,          "cmt.randry");
		p(sctps_slowpath_sack,       "slow_sacks");
		p(sctps_wu_sacks_sent,       "wup_sack_s");
		p(sctps_sends_with_flags,     "w_sinfo_fl");
		p(sctps_sends_with_unord,     "w_unorder");
		p(sctps_sends_with_eof,       "w_eof");
		p(sctps_sends_with_abort,     "w_abort");
		p(sctps_read_peeks,           "peek_reads");
		p(sctps_cached_chk,           "free_chk_u");
		p(sctps_cached_strmoq,        "free_rqs_u");
		p(sctps_left_abandon,         "abandoned");
		p(sctps_send_burst_avoid,     "snd_bua");
		p(sctps_send_cwnd_avoid,      "snd_cwa");
		nl("");
#undef p
#undef nl
	}
	len = 0;
	if (sysctlbyname("net.inet.sctp.assoclist", 0, &len, 0, 0) < 0) {
		printf("Error %d (%s) could not get the assoclist\n", errno, strerror(errno));
		return(0);
	}
	if ((buf = (caddr_t)malloc(len)) == 0) {
		printf("malloc %lu bytes failed.\n", (long unsigned)len);
		return(0);
	}
	if (sysctlbyname("net.inet.sctp.assoclist", buf, &len, 0, 0) < 0) {
		printf("Error %d (%s) could not get the assoclist\n", errno, strerror(errno));
		free(buf);
		return(0);
	}
	offset = 0;
	xinp = (struct xsctp_inpcb *)(buf + offset);
	while (xinp->last == 0) {
		printf("\nEndpoint with port=%d, flags=%x, features=%x, FragPoint=%u, Msgs(R/S/SF)=%u/%u/%u, Queue(Len/Max)=%u/%u\n",
		       xinp->local_port, xinp->flags, xinp->features, xinp->fragmentation_point, xinp->total_recvs, xinp->total_sends, xinp->total_nospaces, xinp->qlen, xinp->maxqlen);
		offset += sizeof(struct xsctp_inpcb);
		printf("\tLocal addresses:");
		xladdr = (struct xsctp_laddr *)(buf + offset);
		while (xladdr->last == 0) {
			family = xladdr->address.sin.sin_family;
			switch (family) {
			case AF_INET:
				addr = (void *)&xladdr->address.sin.sin_addr;
				break;
			case AF_INET6:
				addr = (void *)&xladdr->address.sin6.sin6_addr;
				break;
			default:
				printf("Unknown family: %d.\n", family);
				break;
			}
			printf(" %s", inet_ntop(family, addr, buffer, ADDRSTRLEN));
			offset += sizeof(struct xsctp_laddr);
			xladdr = (struct xsctp_laddr *)(buf + offset);
		}
		offset += sizeof(struct xsctp_laddr);
		printf(".\n");

		xstcb = (struct xsctp_tcb *)(buf + offset);
		while (xstcb->last == 0) {
			xstcb = (struct xsctp_tcb *)(buf + offset);
			printf("\tAssociation towards port=%d, state=%x, Streams(I/O)=(%u/%u), HBInterval=%u, MTU=%u, Msgs(R/S)=(%u/%u), refcnt=%u\n\t\tTSN(init/high/cum/cumack)=(%x/%x/%x/%x),\n\t\tTag(L/R)=(%x/%x), peer_rwnd=%u.\n",
			       xstcb->remote_port, xstcb->state, xstcb->in_streams, xstcb->out_streams, xstcb->heartbeat_interval, xstcb->mtu, xstcb->total_recvs, xstcb->total_sends, xstcb->refcnt, xstcb->initial_tsn, xstcb->highest_tsn, xstcb->cumulative_tsn, xstcb->cumulative_tsn_ack, xstcb->local_tag, xstcb->remote_tag, xstcb->peers_rwnd);
			offset += sizeof(struct xsctp_tcb);

			printf("\t\tLocal addresses:");
			xladdr = (struct xsctp_laddr *)(buf + offset);
			while (xladdr->last == 0) {
				family = xladdr->address.sin.sin_family;
				switch (family) {
				case AF_INET:
					addr = (void *)&xladdr->address.sin.sin_addr;
					break;
				case AF_INET6:
					addr = (void *)&xladdr->address.sin6.sin6_addr;
					break;
				default:
					printf("Unknown family: %d.\n", family);
					break;
				}
				printf(" %s", inet_ntop(family, addr, buffer, ADDRSTRLEN));
				offset += sizeof(struct xsctp_laddr);
				xladdr = (struct xsctp_laddr *)(buf + offset);
			}
			offset += sizeof(struct xsctp_laddr);
			printf(".\n");

			xraddr = (struct xsctp_raddr *)(buf + offset);
			primary_addr = xstcb->primary_addr;
			while (xraddr->last == 0) {
				is_primary_addr = 0;
				family = xraddr->address.sin.sin_family;
				switch (family) {
				case AF_INET:
					addr = (void *)&xraddr->address.sin.sin_addr;
					is_primary_addr = ((primary_addr.sa.sa_family == AF_INET) && !memcmp(&primary_addr.sin.sin_addr, &xraddr->address.sin.sin_addr, sizeof(struct in_addr)));
					break;
				case AF_INET6:
					addr = (void *)&xraddr->address.sin6.sin6_addr;
					is_primary_addr = ((primary_addr.sa.sa_family == AF_INET6) && !memcmp(&primary_addr.sin6.sin6_addr, &xraddr->address.sin6.sin6_addr, sizeof(struct in6_addr)));
					break;
				}
				printf("\t\tPath towards %s, %sActive=%d, Confirmed=%d, PF=%d, MTU=%u, HBEnabled=%u, RTO=%u, RTT=%u, CWND=%u, Flightsize=%u, ErrorCounter=%u.\n",
				       inet_ntop(family, addr, buffer, ADDRSTRLEN), is_primary_addr?"Primary, " : "", xraddr->active, xraddr->confirmed, xraddr->potentially_failed, xraddr->mtu, xraddr->heartbeat_enabled, xraddr->rto, xraddr->rtt, xraddr->cwnd, xraddr->flight_size, xraddr->error_counter);
				offset += sizeof(struct xsctp_raddr);
				xraddr = (struct xsctp_raddr *)(buf + offset);
			}
			offset += sizeof(struct xsctp_raddr);
			xstcb = (struct xsctp_tcb *)(buf + offset);
		}
		offset += sizeof(struct xsctp_tcb);
		xinp = (struct xsctp_inpcb *)(buf + offset);
	}
	free((void *)buf);
	return(0);
}

