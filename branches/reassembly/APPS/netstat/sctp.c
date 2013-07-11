/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
static const char rcsid[] = "$NetBSD: inet.c,v 1.51 2002/02/27 02:33:51 lukem Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/if.h>
#if defined(__FreeBSD__)
#include <net/if_var.h>
#endif
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

#ifdef SCTP
#if !defined(__OpenBSD__)
struct pool {
};  /* XXX pool defined only KERNEL */
#endif
#include <netinet/sctp_pcb.h>

#if !(defined(__OpenBSD__) || defined(__APPLE__))
#define nflag	numeric_port
#endif

static int width;

void	inet46print	__P((struct sockaddr *, u_int16_t, int));

void
#if defined(__FreeBSD__) || defined(__APPLE__)
sctp_protopr(u_long off, const char *name, int af)
#else /* NetBSD and OpenBSD */
sctp_protopr(off, name)
	u_long off;
	char *name;
#endif
{
#if !(defined(__FreeBSD__) || defined(__APPLE__))
	int af;
#endif
	struct sctp_epinfo epinfo;
	struct sctp_inpcb *inp_next, *inp_prev;
	struct sctp_inpcb inp;
	struct sctp_tcb *tcb_next, *tcb_prev;
	struct sctp_tcb tcb;
	struct ifaddrs *ifap0, *ifap;
	struct sctp_laddr *laddr_next;
	struct sctp_laddr laddr;
	struct ifaddr ifa;
	struct sockaddr_storage ss;
	int ss_len;
	struct sctp_nets *nets_next;
	struct sctp_nets nets;
	struct sockaddr *sa;
	struct sockaddr_in *sin, *sin_r, sin_any;
	struct sockaddr_in6 *sin6, *sin6_r;
	int ipv4_addr_legal, ipv6_addr_legal;
	struct socket sockb;
	int first;

	if (off == 0)
		return;
	if(kread(off, (char *)&epinfo, sizeof(epinfo)) == -1) {
		printf("sctp_proto:Can't read epinfo:%x\n", (u_int)off);
		return;
	}
	inp_prev = inp_next = epinfo.listhead.lh_first;
#if defined(__FreeBSD__) || defined(__APPLE__)
	if (Aflag && !Wflag)
		width = 18;
	else if (Wflag)
#else
	if (vflag)
#endif
		width = 45;
	else if (Aflag)
#if defined(__OpenBSD__)
		width = 18;
#else
		width = 21;
#endif
	else 
		width = 22;

	printf("Active Stream Conrol Transmission Protocol associations");
	if (aflag)
		printf(" (including servers)");
	putchar('\n');
	if (Aflag)
		printf("%-8.8s ",
#if defined(__FreeBSD__) || defined(__APPLE__)
			"Socket");
#else
			"PCB");
#endif
#if defined(__FreeBSD__) || defined(__APPLE__)
	printf("%-6.6s %-6.6s %-6.6s%s %-*.*s %-*.*s %s\n",
#else
	printf("%-5.5s %-6.6s %-6.6s%s %-*.*s %-*.*s %s\n",
#endif
	    "Proto", "Recv-Q", "Send-Q",
#if defined(__FreeBSD__) || defined(__APPLE__)
	    " ",
#else
	    Aflag ? "" : " ",
#endif
	    width, width, "Local Address",
	    width, width, "Foreign Address",
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
	    "(state)");
#else
	    "State");
#endif

	if (getifaddrs(&ifap0) < 0)
		return;

	memset(&sin_any, 0, sizeof(sin_any));
	sin_any.sin_len = sizeof(sin_any);
	sin_any.sin_family = AF_INET;

	while (inp_next != NULL) {
		if(kread((u_long)inp_next, (char *)&inp, sizeof(inp)) == -1) {
			printf("Error reading address %x terminated\n",
			       (u_int)inp_next);
			inp_next = NULL;
			continue;
		}
		inp_prev = inp_next;
		inp_next = inp.sctp_list.le_next;
		
		if(kread((u_long)inp.sctp_socket, (char *)&sockb, sizeof(sockb)) == -1) {
			printf("Error reading sctp_socket for inp %x terminated (addr:%x)\n",
			       (u_int)inp_prev, (u_int)inp.sctp_socket);
			inp_next = NULL;
			continue;
		}

		ipv4_addr_legal = ipv6_addr_legal = 0;
		if (inp.sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
			ipv6_addr_legal = 1;
			if (
#if defined(__OpenBSD__)
			(0) /* we always do dual bind */
#elif defined (__NetBSD__)
			(((struct in6pcb *)&inp)->in6p_flags & IN6P_IPV6_V6ONLY)
#else
			(((struct in6pcb *)&inp)->inp_flags & IN6P_IPV6_V6ONLY)
#endif
			== 0) {
				ipv4_addr_legal = 1;
			}
		} else {
			ipv4_addr_legal = 1;
		}

		if (aflag && (inp.sctp_flags & SCTP_PCB_FLAGS_ACCEPTING)) {
			if ((inp.sctp_flags & SCTP_PCB_FLAGS_BOUNDALL)) {
				ifap = ifap0;
				laddr_next = NULL;
			} else {
				ifap = NULL;
				laddr_next = inp.sctp_addr_list.lh_first;
			}
			first = 1;
			while (ifap != NULL || laddr_next != NULL) {
				sin = NULL;
				sin6 = NULL;
				if (ifap != NULL) {
					while (ifap != NULL) {
						if (ifap->ifa_addr->sa_family == AF_INET &&
							ipv4_addr_legal) {
							sin = (struct sockaddr_in *)ifap->ifa_addr;
							break;
						} else if (ifap->ifa_addr->sa_family == AF_INET6 &&
							ipv6_addr_legal) {
							sin6 = (struct sockaddr_in6 *)ifap->ifa_addr;
							break;
						}
						ifap = ifap->ifa_next;
					}
					if (ifap != NULL) 
						ifap = ifap->ifa_next;
				} else if (laddr_next != NULL) {
					do {
						if(kread((u_long)laddr_next,
						    (char *)&laddr,
							 sizeof(laddr)) == -1){
							break;
						}
						laddr_next = laddr.sctp_nxt_addr.le_next;
						if (laddr.ifa == NULL)
							break;
						if(kread((u_long)laddr.ifa,
						    (char *)&ifa,
							 sizeof(ifa)) == -1) {
							break;
						}
						if (ifa.ifa_addr == NULL)
							break;
						if(kread((u_long)ifa.ifa_addr,
						    (char *)&ss,
							 sizeof(struct sockaddr)) == -1) {
							break;
						}
						ss_len = ss.ss_len;
						if(kread((u_long)ifa.ifa_addr,
						    (char *)&ss,
							 ss_len) == -1) {
							break;
						}
						if  (ss.ss_family == AF_INET) {
							sin = (struct sockaddr_in *)&ss;
						} else if (ss.ss_family == AF_INET6) {
							sin6 = (struct sockaddr_in6 *)&ss;
						}
					} while (0);
				}
				if (sin == NULL && sin6 == NULL)
					continue;
				if (first) {
					if (Aflag)
						printf("%8lx ", (u_long)inp_prev);
#if defined(__FreeBSD__) || defined(__APPLE__)
					printf("%-6.6s %6ld %6ld%s ",
#else
					printf("%-5.5s %6ld %6ld%s ",
#endif
					    name,
					    (u_long)sockb.so_rcv.sb_cc,
					    (u_long)sockb.so_snd.sb_cc,
#if defined(__FreeBSD__) || defined(__APPLE__)
					    " ");
#else
					    Aflag ? "" : " ");
#endif
				} else {
#if defined(__FreeBSD__) || defined(__APPLE__)
					if (Aflag)
						printf("%31s"," ");
					else 
						printf("%22s"," ");
#else
					if (Aflag)
						printf("%29s"," ");
					else 
						printf("%21s"," ");
#endif
				}

				if (sin != NULL) {
					inet46print((struct sockaddr *)sin,
					    inp.sctp_lport, nflag);
				} else if (sin6 != NULL) {
					inet46print((struct sockaddr *)sin6,
					    inp.sctp_lport, nflag);
				}
				inet46print((struct sockaddr *)&sin_any,
				    0, nflag);
				if (first) {
					printf("%s", "LISTEN");
					first = 0;
				}
				putchar('\n');
			}
		}

		tcb_next = inp.sctp_asoc_list.lh_first;
		while (tcb_next != NULL) {
			if(kread((u_long)tcb_next, (char *)&tcb, sizeof(tcb)) == -1) {
				printf("Can't read tcp_next:%x\n", 
				       (u_int)tcb_next);
				tcb_next = NULL;
				continue;
			}
			tcb_prev = tcb_next;
			tcb_next = tcb.sctp_tcblist.le_next;
			nets_next = tcb.asoc.nets.tqh_first;
			if ((inp.sctp_flags & SCTP_PCB_FLAGS_BOUNDALL)) {
				ifap = ifap0;
				laddr_next = NULL;
			} else {
				ifap = NULL;
				laddr_next = inp.sctp_addr_list.lh_first;
			}

			first = 1;
			while (ifap != NULL ||
				laddr_next != NULL ||
				nets_next != NULL) {
				sin = sin_r = NULL;
				sin6 = sin6_r =  NULL;

				if (ifap != NULL) {
					while (ifap != NULL) {
						if (ifap->ifa_addr->sa_family == AF_INET &&
							ipv4_addr_legal) {
							sin = (struct sockaddr_in *)ifap->ifa_addr;
							break;
						} else if (ifap->ifa_addr->sa_family == AF_INET6 &&
							ipv6_addr_legal) {
							sin6 = (struct sockaddr_in6 *)ifap->ifa_addr;
							break;
						}
						ifap = ifap->ifa_next;
					}
					if (ifap != NULL) 
						ifap = ifap->ifa_next;
				} else if (laddr_next != NULL) {
					do {
						if(kread((u_long)laddr_next,
						    (char *)&laddr,
							 sizeof(laddr)) == -1) {
							break;
						}
						laddr_next = laddr.sctp_nxt_addr.le_next;
						if (laddr.ifa == NULL)
							break;
						if(kread((u_long)laddr.ifa,
						    (char *)&ifa,
							 sizeof(ifa)) == -1) {
							break;
						}
						if (ifa.ifa_addr == NULL)
							break;
						if(kread((u_long)ifa.ifa_addr,
						    (char *)&ss,
						    sizeof(struct sockaddr)) == -1) {
							break;
						}
						ss_len = ss.ss_len;
						if(kread((u_long)ifa.ifa_addr,
						    (char *)&ss,
						    ss_len) == -1) {
							break;
						}
						if  (ss.ss_family == AF_INET) {
							sin = (struct sockaddr_in *)&ss;
						} else if (ss.ss_family == AF_INET6) {
							sin6 = (struct sockaddr_in6 *)&ss;
						}
					} while (0);
				}

				if (nets_next != NULL) {
					if(kread((u_long)nets_next, (char *)&nets,
					    sizeof(nets)) == -1) {
						break;
					}
					nets_next = nets.sctp_next.tqe_next;
					sa = (struct sockaddr *)&nets.ro._l_addr;
					if (sa->sa_family == AF_INET) {
						sin_r = (struct sockaddr_in *)sa;
					} else if (sa->sa_family == AF_INET6) {
						sin6_r = (struct sockaddr_in6 *)sa;
					} 
				}

				if (sin == NULL && sin6 == NULL &&
				    sin_r == NULL && sin6_r == NULL)
					continue;
				if (first) {
					if (Aflag)
						printf("%8lx ", (u_long)inp_prev);
#if defined(__FreeBSD__) || defined(__APPLE__)
					printf("%-6.6s %6ld %6ld%s ",
#else
					printf("%-5.5s %6ld %6ld%s ",
#endif
					    name,
					    (u_long)sockb.so_rcv.sb_cc,
					    (u_long)sockb.so_snd.sb_cc,
#if defined(__FreeBSD__) || defined(__APPLE__)
					    " ");
#else
					    Aflag ? "" : " ");
#endif
						
				} else {
#if defined(__FreeBSD__) || defined(__APPLE__)
					if (Aflag)
						printf("%31s"," ");
					else 
						printf("%22s"," ");
#else
					if (Aflag)
						printf("%29s"," ");
					else 
						printf("%21s"," ");
#endif
				}
					
				if (sin != NULL) {
					inet46print((struct sockaddr *)sin,
					    inp.sctp_lport, nflag);
				} else if (sin6 != NULL) {
					inet46print((struct sockaddr *)sin6,
					    inp.sctp_lport, nflag);
				} else {
					printf("N/A%*s", width - 3 + 1, " ");
				}
				if (sin_r != NULL) {
					inet46print((struct sockaddr *)sin_r,
					    tcb.rport, nflag);
				} else if (sin6_r != NULL) {
					inet46print((struct sockaddr *)sin6_r,
					    tcb.rport, nflag);
				} else {
					printf("N/A%*s", width - 3 + 1, " ");
				}
				if (first) {
					switch (tcb.asoc.state) {
					case SCTP_STATE_COOKIE_WAIT:
						printf("COOKIE_WAIT");
						break;
					case SCTP_STATE_COOKIE_ECHOED:
						printf("COOKIE_ECHOED");
						break;
					case SCTP_STATE_OPEN:
						printf("ESTABLISHED");
						break;
					case SCTP_STATE_SHUTDOWN_SENT:
						printf("SHUTDOWN-SENT");
						break;
					case SCTP_STATE_SHUTDOWN_RECEIVED:
						printf("SHUTDOWN-RECEIVED");
						break;
					case SCTP_STATE_SHUTDOWN_ACK_SENT:
						printf("SHUTDOWN-ACK-SENT");
						break;
					case SCTP_STATE_SHUTDOWN_PENDING:
						printf("SHUTDOWN-PENDING");
						break;
					case SCTP_STATE_EMPTY:
					case SCTP_STATE_INUSE:
					case SCTP_STATE_CLOSED_SOCKET:
					default:
						printf("CLOSED");
						break;
					}
					first = 0;
				}
				putchar('\n');
			}
		}
	}
	freeifaddrs(ifap0);
}

void
inet46print(sa, port, numeric)
	struct sockaddr *sa;
	u_int16_t port;
	int numeric;
{
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	u_int16_t scope_id;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	int flags = 0;
	int n, addrwidth;

	addrwidth = width - 6;

	if (sa->sa_family == AF_INET) {
		sin = *(struct sockaddr_in *)sa;
		sin.sin_port = port;
		sa = (struct sockaddr *)&sin;
	} else if (sa->sa_family == AF_INET6) {
		sin6 = *(struct sockaddr_in6 *)sa;
		if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) {
			bcopy(&sin6.sin6_addr.s6_addr[2], &scope_id,
			    sizeof(u_int16_t));
			sin6.sin6_scope_id = ntohs(scope_id);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
		sin6.sin6_port = port;
		sa = (struct sockaddr *)&sin6;
	} else
		return;
	
	if (numeric)
		flags |= NI_NUMERICHOST | NI_NUMERICSERV;

	if (getnameinfo(sa, sa->sa_len, hbuf, sizeof(hbuf),
		sbuf, sizeof(sbuf), flags)) {
		printf("%*s ", width, "???");
		return;
	}
	n = 0;
	if ((sa->sa_family == AF_INET &&
	     sin.sin_addr.s_addr == INADDR_ANY) ||
	    (sa->sa_family == AF_INET6 &&
	     IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)))
		n = printf("%.*s.", addrwidth, "*");
	else
		n = printf("%.*s.", addrwidth, hbuf);

	if (port == 0)
		n += printf("%-5s ", "*") - 1;
	else
		n += printf("%-5s ", sbuf) - 1;
		
	if (n < width)
		printf("%*s", width - n, " ");
}

static char *pegs_name[SCTP_NUMBER_OF_PEGS] = {
/* SCTP_PEG_SACKS_SEEN	 0 */ "SACKs received",
/* SCTP_PEG_SACKS_SENT	 1 */ "SACKs sent",
/* SCTP_PEG_TSNS_SENT	 2 */ "TSNs sent",
/* SCTP_PEG_TSNS_RCVD	 3 */ "non-duplicate TSNs received",
/* SCTP_DATAGRAMS_SENT	 4 */ "SCTP packets sent",
/* SCTP_DATAGRAMS_RCVD	 5 */ "valid SCTP packets received",
/* SCTP_RETRANTSN_SENT	 6 */ "TSNs retransmitted",
/* SCTP_DUPTSN_RECVD	 7 */ "duplicate TSNs received",
/* SCTP_HB_RECV		 8 */ "HEARTBEATs received",
/* SCTP_HB_ACK_RECV	 9 */ "HEARTBEAT-ACKs received",
/* SCTP_HB_SENT		10 */ "HEARTBEATs sent",
/* SCTP_WINDOW_PROBES	11 */ "window probe TSNs sent",
/* SCTP_DATA_DG_RECV	12 */ "SCTP packets received with DATA",
/* SCTP_TMIT_TIMER	13 */ "T3 retransmission timeouts",
/* SCTP_RECV_TIMER	14 */ "delayed SACK timeouts",
/* SCTP_HB_TIMER	15 */ "heartbeat timeouts",
/* SCTP_FAST_RETRAN	16 */ "packets fast retransmitted",
/* SCTP_TIMERS_EXP	17 */ "timers expired/handled",
/* SCTP_FR_INAWINDOW	18 */ "multiple fast retransmissions in an RTT",
/* SCTP_RWND_BLOCKED	19 */ "attempts blocked by peer's receive window",
/* SCTP_CWND_BLOCKED	20 */ "attempts blocked by destination cwnd",
/* SCTP_RWND_DROPS	21 */ "receive window overruns by peer",
/* SCTP_BAD_STRMNO	22 */ "DATA received with invalid stream number",
/* SCTP_BAD_SSN_WRAP	23 */ "DATA received with SSN less than expected",
/* SCTP_DROP_NOMEMORY	24 */ "DATA dropped due to memory shortage",
/* SCTP_DROP_FRAG	25 */ "drpfragm",
/* SCTP_BAD_VTAGS	26 */ "packets received with invalid VTAG",
/* SCTP_BAD_CSUM	27 */ "packets received with invalid checksum",
/* SCTP_INPKTS		28 */ "total packets received",
/* SCTP_IN_MCAST	29 */ "multicast packets received (dropped)",
/* SCTP_HDR_DROPS	30 */ "packets received with header errors",
/* SCTP_NOPORTS		31 */ "packets received for non-listening ports",
/* SCTP_CWND_NOFILL	32 */ "cwnd_nf",
/* SCTP_CALLS_TO_CO	33 */ "calls to sctp_chunk_output",
/* SCTP_CO_NODATASNT	34 */ "calls to sctp_chunk_output but no data sent",
/* SCTP_CWND_NOUSE_SS	35 */ "cw_nu_ss",
/* SCTP_MAX_BURST_APL	36 */ "max burst limited output",
/* SCTP_EXPRESS_ROUTE	37 */ "express deliveries",
/* SCTP_NO_COPY_IN	38 */ "co_cp_in",
/* SCTP_CACHED_SRC	39 */ "packets sent with optimized copying",
/* SCTP_CWND_NOCUM	40 */ "cw_nocum",
/* SCTP_CWND_SS		41 */ "cwnd increases in slow start",
/* SCTP_CWND_CA		42 */ "cwnd increases in congestion avoidance",
/* SCTP_CWND_SKIP	43 */ "loss recoveries skipped cwnd adjustment",
/* SCTP_CWND_NOUSE_CA	44 */ "cw_nu_ca",
/* SCTP_MAX_CWND	45 */ ": max cwnd on any destination",
/* SCTP_CWND_DIFF_CA	46 */ "diff_ca ",
/* SCTP_CWND_DIFF_SA	47 */ "diff_ss ",
/* SCTP_OQS_AT_SS	48 */ "tqs @ ss",
/* SCTP_SQQ_AT_SS	49 */ "sqs @ ss",
/* SCTP_OQS_AT_CA	50 */ "tqs @ ca",
/* SCTP_SQQ_AT_CA	51 */ "sqs @ ca",
/* SCTP_MOVED_MTU	52 */ ": largest MTU requested",
/* SCTP_MOVED_QMAX	53 */ ": largest number of chunks at once",
/* SCTP_SQC_AT_SS	54 */ "sndqctss",
/* SCTP_SQC_AT_CA	55 */ "sndqctca",
/* SCTP_MOVED_MAX	56 */ "calls to sctp_fill_outqueue with data moved",
/* SCTP_MOVED_NLEF	57 */ "calls to sctp_fill_outqueue with no data left",
/* SCTP_NAGLE_NOQ	58 */ "Nagle limited output",
/* SCTP_NAGLE_OFF	59 */ "nagle_of",
/* SCTP_OUTPUT_FRM_SND	60 */ "user send calls sctp_chunk_output",
/* SCTP_SOS_NOSNT	61 */ "sostrnos",
/* SCTP_NOS_NOSNT	62 */ "nostrnos",
/* SCTP_SOSE_NOSNT	63 */ "sosnqnos",
/* SCTP_NOSE_NOSNT	64 */ "nosnqnos",
/* SCTP_DATA_OUT_ERR	65 */ "intoperr",
/* SCTP_DUP_SSN_RCVD	66 */ "duplicate SSN received",
/* SCTP_DUP_FR		67 */ "multi-fr",
/* SCTP_VTAG_EXPR	68 */ "TCB located by VTAG",
/* SCTP_VTAG_BOGUS	69 */ "VTAG and address mismatches",
/* SCTP_T3_SAFEGRD	70 */ "T3 safegaurd timers started",
/* SCTP_PDRP_FMBOX	71 */ "PACKET-DROP reports received from middleboxes",
/* SCTP_PDRP_FEHOS	72 */ "PACKET-DROP reports received from end hosts",
/* SCTP_PDRP_MB_DA	73 */ "PACKET-DROPs from middleboxes reporting lost data chunks ",
/* SCTP_PDRP_MB_CT	74 */ "PACKET-DROPs from middleboxes reporting lost control chunks",
/* SCTP_PDRP_BWRPT	75 */ "PACKET-DROPs received with only Bandwidth reports",
/* SCTP_PDRP_CRUPT	76 */ "pd_corup",
/* SCTP_PDRP_NEDAT	77 */ "PACKET-DROPs received with inconclusive DATA",
/* SCTP_PDRP_PDBRK	78 */ "PACKET-DROPs received and not processed due to error",
/* SCTP_PDRP_TSNNF	79 */ "PACKET-DROPs received with a TSN that could not be found",
/* SCTP_PDRP_DNFND	80 */ "PACKET-DROPs received requiring exhuastive DATA chunk search",
/* SCTP_PDRP_DIWNP	81 */ "PACKET-DROPs from endhost about a window probe TSN",
/* SCTP_PDRP_DIZRW	82 */ "PACKET-DROPs from middleboxes about a window probe TSN",
/* SCTP_PDRP_BADD	83 */ "PACKET-DROPs received with invalid DATA TSN",
/* SCTP_PDRP_MARK	84 */ "PACKET-DROPs causing a valid fast retransmission",
/* SCTP_ECNE_RCVD	85 */ "ECN-Echo chunks received",
/* SCTP_CWR_PERFO	86 */ "congestion window reductions performed",
/* SCTP_ECNE_SENT	87 */ "ECN-Echo chunks sent",
/* SCTP_MSGC_DROP	88 */ "chunks on queue dropped due to peer total chunk limit",
/* SCTP_SEND_QUEUE_POP	89 */ "queue_pop",
/* SCTP_ERROUT_FRM_USR	90 */ "errout_from_usr",
/* SCTP_SENDTO_FULL_CWND91 */ "sendto_with_full_cwnd",
/* SCTP_QUEONLY_BURSTLMT 92*/ "queue_only_burst_limit",
/* SCTP_IFP_QUEUE_FULL   93*/ "ifp_queue_is_full",
/* SCTP_NO_TCB_IN_RCV    94*/ "no_tcb_in_recv_function",
/* SCTP_HAD_TCB_IN_RCV   95*/ "had_tcb_in_recv_function",
/* SCTP_PDAPI_UP_IN_RCV  96*/ "pdapi_up_in_recv_function",
/* SCTP_PDAPI_HAD_TOWAIT_RCV97*/ "pdapi_had_to_wait_in_recv",
/* SCTP_PDAPI_HAD_TORCVR_RCV 98*/ "pdap_had_torcv_rcv",
/* SCTP_PDAPI_NOSTCB_ATC     99*/ "pdapi_no_stcb_atc",
/* SCTP_ENTER_SCTPSORCV     100*/ "enter_sctp_so_rcv",
/* SCTP_REACHED_FR_MARK     101*/ "reached_fr_mark",
/* SCTP_BY_ASSOCID          102*/ "find_assoc_by_asoc_id",
/* SCTP_ASID_BOGUS          103*/ "find_assoc_bogus_asoc_id",
/* SCTP_EARLYFR_STRT_TMR    104*/ "early_fr_timer_start",
/* SCTP_EARLYFR_STOP_TMR    105*/ "early_fr_timer_stop",
/* SCTP_EARLYFR_EXPI_TMR    106*/ "early_fr_timer_expire",
/* SCTP_EARLYFR_MRK_RETR    107*/ "early_fr_timer_mark_for_retran",
/* SCTP_T3_AT_WINPROBE      108*/ "t3_window_probe",
/* SCTP_RENEGED_ALL         109*/ "reneged_all_data",
/* SCTP_IRENEGED_ON_ASOC    110*/ "i_reneged_on_asoc",
/* SCTP_EARLYFR_STP_OUT     111*/ "early_fr_stp_out",
/* SCTP_EARLYFR_STP_ID_SCK1 112*/ "earlY_fr_stp_id_sack1",
/* SCTP_EARLYFR_STP_ID_SCK2 113*/ "earlY_fr_stp_id_sack2",
/* SCTP_EARLYFR_STP_ID_SCK3 114*/ "earlY_fr_stp_id_sack3",
/* SCTP_EARLYFR_STP_ID_SCK4 115*/ "earlY_fr_stp_id_sack4",
/* SCTP_EARLYFR_STR_ID      116*/ "earlY_fr_stp_id_strid",
/* SCTP_EARLYFR_STR_OUT     117*/ "earlY_fr_stp_id_strout",
/* SCTP_EARLYFR_STR_TMR     118*/ "earlY_fr_stp_id_strtmr",
/*SCTP_T3_MARKED_TSNS      119*/  "t3_tsns_marked_for_retran",
		NULL,
};
		
void
#if defined(__FreeBSD__) || defined(__APPLE__)
sctp_stats(u_long off, const char *name, int af)
#else /* NetBSD and OpenBSD */
sctp_stats(off, name)
        u_long off;
        char *name;
#endif
{
#if !(defined(__FreeBSD__) || defined(__APPLE__))
        int af;
#endif
	int i;
	u_int32_t pegs[SCTP_NUMBER_OF_PEGS];

	if (off == 0)
		return;

	if(kread(off, (char *)pegs, sizeof(pegs)) == -1) {
		printf("Error reading SCTP pegs\n");
		return;
	}
	
	printf("sctp:\n");
	for (i = 0; i < SCTP_NUMBER_OF_PEGS; i++) {
		if (pegs_name[i])
			printf("\t%10u %s\n", pegs[i], pegs_name[i]);
	}

	/* clear stats, if requested */
#if !defined(__APPLE__)
	if (zflag) {
		int fd;
		int option = 0;

		fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		if (fd < 0)
			return;
		setsockopt(fd, IPPROTO_SCTP, SCTP_RESET_PEGS, &option, sizeof(option));
	}
#endif
}
#endif /* SCTP */
