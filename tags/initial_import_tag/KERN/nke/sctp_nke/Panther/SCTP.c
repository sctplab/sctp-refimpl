/*
 * Copyright (C) 2004, 2005, 2006 Michael Tuexen, tuexen@fh-muenster.de,
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
#include <sys/systm.h>
#include <mach/mach_types.h>
#endif

#include <sys/sysctl.h>
#include <netinet/in.h>
#include <sys/protosw.h>
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
#include <netinet/ip.h>
#include <sys/lock.h>
#include <sys/domain.h>
#else
#include <sys/mbuf.h>
#endif

#include <net/route.h>
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
#include <sys/socketvar.h>
#endif
#include <netinet/in_pcb.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_timer.h>
#include <netinet6/sctp6_var.h>

SYSCTL_DECL(_net_inet);
SYSCTL_DECL(_net_inet6);
SYSCTL_NODE(_net_inet, IPPROTO_SCTP,    sctp,   CTLFLAG_RW, 0,  "SCTP");
SYSCTL_NODE(_net_inet6, IPPROTO_SCTP,   sctp6,  CTLFLAG_RW, 0,  "SCTP6");

extern struct sysctl_oid sysctl__net_inet_sctp_sendspace;
extern struct sysctl_oid sysctl__net_inet_sctp_recvspace;
/*
extern struct sysctl_oid sysctl__net_inet_sctp_auto_asconf;
*/
extern struct sysctl_oid sysctl__net_inet_sctp_ecn_enable;
extern struct sysctl_oid sysctl__net_inet_sctp_ecn_nonce;
extern struct sysctl_oid sysctl__net_inet_sctp_strict_sacks;
extern struct sysctl_oid sysctl__net_inet_sctp_loopback_nocsum;
extern struct sysctl_oid sysctl__net_inet_sctp_strict_init;
extern struct sysctl_oid sysctl__net_inet_sctp_peer_chkoh;
extern struct sysctl_oid sysctl__net_inet_sctp_maxburst;
extern struct sysctl_oid sysctl__net_inet_sctp_maxchunks;
extern struct sysctl_oid sysctl__net_inet_sctp_delayed_sack_time;
extern struct sysctl_oid sysctl__net_inet_sctp_heartbeat_interval;
extern struct sysctl_oid sysctl__net_inet_sctp_pmtu_raise_time;
extern struct sysctl_oid sysctl__net_inet_sctp_shutdown_guard_time;
extern struct sysctl_oid sysctl__net_inet_sctp_secret_lifetime;
extern struct sysctl_oid sysctl__net_inet_sctp_rto_max;
extern struct sysctl_oid sysctl__net_inet_sctp_rto_min;
extern struct sysctl_oid sysctl__net_inet_sctp_rto_initial;
extern struct sysctl_oid sysctl__net_inet_sctp_init_rto_max;
extern struct sysctl_oid sysctl__net_inet_sctp_valid_cookie_life;
extern struct sysctl_oid sysctl__net_inet_sctp_init_rtx_max;
extern struct sysctl_oid sysctl__net_inet_sctp_assoc_rtx_max;
extern struct sysctl_oid sysctl__net_inet_sctp_path_rtx_max;
extern struct sysctl_oid sysctl__net_inet_sctp_nr_outgoing_streams;
extern struct sysctl_oid sysctl__net_inet_sctp_cmt_on_off;
extern struct sysctl_oid sysctl__net_inet_sctp_cwnd_maxburst;
extern struct sysctl_oid sysctl__net_inet_sctp_early_fast_retran;
extern struct sysctl_oid sysctl__net_inet_sctp_use_rttvar_congctrl;
extern struct sysctl_oid sysctl__net_inet_sctp_deadlock_detect;
extern struct sysctl_oid sysctl__net_inet_sctp_early_fast_retran_msec;
extern struct sysctl_oid sysctl__net_inet_sctp_asconf_auth_nochk;
extern struct sysctl_oid sysctl__net_inet_sctp_auth_disable;
extern struct sysctl_oid sysctl__net_inet_sctp_auth_random_len;
extern struct sysctl_oid sysctl__net_inet_sctp_auth_hmac_id;
extern struct sysctl_oid sysctl__net_inet_sctp_abc_l_var;
extern struct sysctl_oid sysctl__net_inet_sctp_max_chained_mbufs;
extern struct sysctl_oid sysctl__net_inet_sctp_cmt_use_dac;
extern struct sysctl_oid sysctl__net_inet_sctp_do_sctp_drain;
extern struct sysctl_oid sysctl__net_inet_sctp_warm_crc_table;
extern struct sysctl_oid sysctl__net_inet_sctp_abort_at_limit;
extern struct sysctl_oid sysctl__net_inet_sctp_strict_data_order;
extern struct sysctl_oid sysctl__net_inet_sctp_stats;
#ifdef SCTP_DEBUG
extern struct sysctl_oid sysctl__net_inet_sctp_debug;
#endif

extern struct domain inetdomain;
extern struct domain inet6domain;
extern struct protosw *ip_protox[];
extern struct protosw *ip6_protox[];

extern struct sctp_epinfo sctppcinfo;

struct protosw sctp4_dgram;
struct protosw sctp4_seqpacket;
struct protosw sctp4_stream;
struct protosw sctp6_dgram;
struct protosw sctp6_seqpacket;
struct protosw sctp6_stream;

struct protosw *old_pr4, *old_pr6;

kern_return_t SCTP_start (kmod_info_t * ki, void * d) {
	
	int err;
#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
	int s;
	int funnel_state;
#endif

	old_pr4  = ip_protox [IPPROTO_SCTP];
	old_pr6  = ip6_protox[IPPROTO_SCTP];
	
	bzero(&sctp4_dgram,     sizeof(struct protosw));
	bzero(&sctp4_seqpacket, sizeof(struct protosw));
	bzero(&sctp4_stream,    sizeof(struct protosw));
	bzero(&sctp6_dgram,     sizeof(struct protosw));
	bzero(&sctp6_seqpacket, sizeof(struct protosw));
	bzero(&sctp6_stream,    sizeof(struct protosw));

	sctp4_dgram.pr_type          = SOCK_DGRAM;
	sctp4_dgram.pr_domain        = &inetdomain;
	sctp4_dgram.pr_protocol      = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_dgram.pr_flags         = PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp4_dgram.pr_flags         = PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp4_dgram.pr_input         = sctp_input;
	sctp4_dgram.pr_output        = 0;
	sctp4_dgram.pr_ctlinput      = sctp_ctlinput;
	sctp4_dgram.pr_ctloutput     = sctp_ctloutput;
	sctp4_dgram.pr_ousrreq       = 0;
	sctp4_dgram.pr_init          = sctp_init;
	sctp4_dgram.pr_fasttimo      = 0;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_dgram.pr_slowtimo      = sctp_slowtimo;
#else
	sctp4_dgram.pr_slowtimo      = 0;
#endif
	sctp4_dgram.pr_drain         = sctp_drain;
	sctp4_dgram.pr_sysctl        = 0;
	sctp4_dgram.pr_usrreqs       = &sctp_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_dgram.pr_lock          = sctp_lock;
	sctp4_dgram.pr_unlock        = sctp_unlock;
	sctp4_dgram.pr_getlock       = sctp_getlock;
#endif

	sctp4_seqpacket.pr_type	     = SOCK_SEQPACKET;
	sctp4_seqpacket.pr_domain    = &inetdomain;
	sctp4_seqpacket.pr_protocol  = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_seqpacket.pr_flags     = PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp4_seqpacket.pr_flags     = PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp4_seqpacket.pr_input     = sctp_input;
	sctp4_seqpacket.pr_output    = 0;
	sctp4_seqpacket.pr_ctlinput  = sctp_ctlinput;
	sctp4_seqpacket.pr_ctloutput = sctp_ctloutput;
	sctp4_seqpacket.pr_ousrreq   = 0;
	sctp4_seqpacket.pr_init      = 0;
	sctp4_seqpacket.pr_fasttimo  = 0;
	sctp4_seqpacket.pr_slowtimo  = 0;
	sctp4_seqpacket.pr_drain     = sctp_drain;
	sctp4_seqpacket.pr_sysctl    = 0;
	sctp4_seqpacket.pr_usrreqs   = &sctp_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_seqpacket.pr_lock      = sctp_lock;
	sctp4_seqpacket.pr_unlock    = sctp_unlock;
	sctp4_seqpacket.pr_getlock   = sctp_getlock;
#endif

	sctp4_stream.pr_type         = SOCK_STREAM;
	sctp4_stream.pr_domain       = &inetdomain;
	sctp4_stream.pr_protocol     = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_stream.pr_flags        = PR_CONNREQUIRED|PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp4_stream.pr_flags        = PR_CONNREQUIRED|PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp4_stream.pr_input        = sctp_input;
	sctp4_stream.pr_output       = 0;
	sctp4_stream.pr_ctlinput     = sctp_ctlinput;
	sctp4_stream.pr_ctloutput    = sctp_ctloutput;
	sctp4_stream.pr_ousrreq      = 0;
	sctp4_stream.pr_init         = 0;
	sctp4_stream.pr_fasttimo     = 0;
	sctp4_stream.pr_slowtimo     = 0;
	sctp4_stream.pr_drain        = sctp_drain;
	sctp4_stream.pr_sysctl       = 0;
	sctp4_stream.pr_usrreqs      = &sctp_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp4_stream.pr_lock         = sctp_lock;
	sctp4_stream.pr_unlock       = sctp_unlock;
	sctp4_stream.pr_getlock      = sctp_getlock;
#endif

	sctp6_dgram.pr_type          = SOCK_DGRAM;
	sctp6_dgram.pr_domain        = &inet6domain;
	sctp6_dgram.pr_protocol      = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_dgram.pr_flags         = PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp6_dgram.pr_flags         = PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp6_dgram.pr_input         = (void (*) (struct mbuf *, int)) sctp6_input;
	sctp6_dgram.pr_output        = 0;
	sctp6_dgram.pr_ctlinput      = sctp6_ctlinput;
	sctp6_dgram.pr_ctloutput     = sctp_ctloutput;
	sctp6_dgram.pr_ousrreq       = 0;
	sctp6_dgram.pr_init          = 0;
	sctp6_dgram.pr_fasttimo      = 0;
	sctp6_dgram.pr_slowtimo      = 0;
	sctp6_dgram.pr_drain         = sctp_drain;
	sctp6_dgram.pr_sysctl        = 0;
	sctp6_dgram.pr_usrreqs       = &sctp6_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_dgram.pr_lock          = sctp_lock;
	sctp6_dgram.pr_unlock        = sctp_unlock;
	sctp6_dgram.pr_getlock       = sctp_getlock;
#endif

	sctp6_seqpacket.pr_type      = SOCK_SEQPACKET;
	sctp6_seqpacket.pr_domain    = &inet6domain;
	sctp6_seqpacket.pr_protocol  = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_seqpacket.pr_flags     = PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp6_seqpacket.pr_flags     = PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp6_seqpacket.pr_input     = (void (*) (struct mbuf *, int)) sctp6_input;
	sctp6_seqpacket.pr_output    = 0;
	sctp6_seqpacket.pr_ctlinput  = sctp6_ctlinput;
	sctp6_seqpacket.pr_ctloutput = sctp_ctloutput;
	sctp6_seqpacket.pr_ousrreq   = 0;
	sctp6_seqpacket.pr_init      = 0;
	sctp6_seqpacket.pr_fasttimo  = 0;
	sctp6_seqpacket.pr_slowtimo  = 0;
	sctp6_seqpacket.pr_drain     = sctp_drain;
	sctp6_seqpacket.pr_sysctl    = 0;
	sctp6_seqpacket.pr_usrreqs   = &sctp6_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_seqpacket.pr_lock      = sctp_lock;
	sctp6_seqpacket.pr_unlock    = sctp_unlock;
	sctp6_seqpacket.pr_getlock   = sctp_getlock;
#endif

	sctp6_stream.pr_type         = SOCK_STREAM;
	sctp6_stream.pr_domain       = &inet6domain;
	sctp6_stream.pr_protocol     = IPPROTO_SCTP;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_stream.pr_flags        = PR_CONNREQUIRED|PR_WANTRCVD|PR_PCBLOCK|PR_PROTOLOCK;
#else
	sctp6_seqpacket.pr_flags     = PR_ADDR_OPT|PR_WANTRCVD;
#endif
	sctp6_stream.pr_input        = (void (*) (struct mbuf *, int)) sctp6_input;
	sctp6_stream.pr_output       = 0;
	sctp6_stream.pr_ctlinput     = sctp6_ctlinput;
	sctp6_stream.pr_ctloutput    = sctp_ctloutput;
	sctp6_stream.pr_ousrreq      = 0;
	sctp6_stream.pr_init         = 0;
	sctp6_stream.pr_fasttimo     = 0;
	sctp6_stream.pr_slowtimo     = 0;
	sctp6_stream.pr_drain        = sctp_drain;
	sctp6_stream.pr_sysctl       = 0;
	sctp6_stream.pr_usrreqs      = &sctp6_usrreqs;
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp6_stream.pr_lock         = sctp_lock;
	sctp6_stream.pr_unlock       = sctp_unlock;
	sctp6_stream.pr_getlock      = sctp_getlock;
#endif

#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	lck_mtx_lock(inetdomain.dom_mtx);
	lck_mtx_lock(inet6domain.dom_mtx);
#else
	funnel_state = thread_funnel_set(network_flock, TRUE);
	s            = splnet();
#endif

	err  = net_add_proto(&sctp4_dgram,     &inetdomain);
	err |= net_add_proto(&sctp4_seqpacket, &inetdomain);
	err |= net_add_proto(&sctp4_stream,    &inetdomain);
	err |= net_add_proto(&sctp6_dgram,     &inet6domain);
	err |= net_add_proto(&sctp6_seqpacket, &inet6domain);
	err |= net_add_proto(&sctp6_stream,    &inet6domain);

	if (err) {
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
		lck_mtx_unlock(inet6domain.dom_mtx);
		lck_mtx_unlock(inetdomain.dom_mtx);
#else
		splx(s);
		thread_funnel_set(network_flock, funnel_state);
#endif
		printf("SCTP NKE: Not all protocol handlers could be installed.\n");
		return KERN_FAILURE;
	}

	ip_protox[IPPROTO_SCTP]  = &sctp4_dgram;
	ip6_protox[IPPROTO_SCTP] = &sctp6_dgram;

#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	lck_mtx_unlock(inet6domain.dom_mtx);
	lck_mtx_unlock(inetdomain.dom_mtx);
#endif
	
	sysctl_register_oid(&sysctl__net_inet_sctp);
	sysctl_register_oid(&sysctl__net_inet_sctp_sendspace);
	sysctl_register_oid(&sysctl__net_inet_sctp_recvspace);
/*
	sysctl_register_oid(&sysctl__net_inet_sctp_auto_asconf);
*/
	sysctl_register_oid(&sysctl__net_inet_sctp_ecn_enable);
	sysctl_register_oid(&sysctl__net_inet_sctp_ecn_nonce);
	sysctl_register_oid(&sysctl__net_inet_sctp_strict_sacks);
	sysctl_register_oid(&sysctl__net_inet_sctp_loopback_nocsum);
	sysctl_register_oid(&sysctl__net_inet_sctp_strict_init);
	sysctl_register_oid(&sysctl__net_inet_sctp_peer_chkoh);
	sysctl_register_oid(&sysctl__net_inet_sctp_maxburst);
	sysctl_register_oid(&sysctl__net_inet_sctp_maxchunks);
	sysctl_register_oid(&sysctl__net_inet_sctp_delayed_sack_time);
	sysctl_register_oid(&sysctl__net_inet_sctp_heartbeat_interval);
	sysctl_register_oid(&sysctl__net_inet_sctp_pmtu_raise_time);
	sysctl_register_oid(&sysctl__net_inet_sctp_shutdown_guard_time);
	sysctl_register_oid(&sysctl__net_inet_sctp_secret_lifetime);
	sysctl_register_oid(&sysctl__net_inet_sctp_rto_max);
	sysctl_register_oid(&sysctl__net_inet_sctp_rto_min);
	sysctl_register_oid(&sysctl__net_inet_sctp_rto_initial);
	sysctl_register_oid(&sysctl__net_inet_sctp_init_rto_max);
	sysctl_register_oid(&sysctl__net_inet_sctp_valid_cookie_life);
	sysctl_register_oid(&sysctl__net_inet_sctp_init_rtx_max);
	sysctl_register_oid(&sysctl__net_inet_sctp_assoc_rtx_max);
	sysctl_register_oid(&sysctl__net_inet_sctp_path_rtx_max);
	sysctl_register_oid(&sysctl__net_inet_sctp_nr_outgoing_streams);
	sysctl_register_oid(&sysctl__net_inet_sctp_cmt_on_off);
	sysctl_register_oid(&sysctl__net_inet_sctp_cwnd_maxburst);
	sysctl_register_oid(&sysctl__net_inet_sctp_early_fast_retran);
	sysctl_register_oid(&sysctl__net_inet_sctp_use_rttvar_congctrl);
	sysctl_register_oid(&sysctl__net_inet_sctp_deadlock_detect);
	sysctl_register_oid(&sysctl__net_inet_sctp_early_fast_retran_msec);
	sysctl_register_oid(&sysctl__net_inet_sctp_asconf_auth_nochk);
	sysctl_register_oid(&sysctl__net_inet_sctp_auth_disable);
	sysctl_register_oid(&sysctl__net_inet_sctp_auth_random_len);
	sysctl_register_oid(&sysctl__net_inet_sctp_auth_hmac_id);
	sysctl_register_oid(&sysctl__net_inet_sctp_abc_l_var);
	sysctl_register_oid(&sysctl__net_inet_sctp_max_chained_mbufs);
	sysctl_register_oid(&sysctl__net_inet_sctp_cmt_use_dac);
	sysctl_register_oid(&sysctl__net_inet_sctp_do_sctp_drain);
	sysctl_register_oid(&sysctl__net_inet_sctp_warm_crc_table);
	sysctl_register_oid(&sysctl__net_inet_sctp_abort_at_limit);
	sysctl_register_oid(&sysctl__net_inet_sctp_strict_data_order);
	sysctl_register_oid(&sysctl__net_inet_sctp_stats);

#ifdef SCTP_DEBUG
	sysctl_register_oid(&sysctl__net_inet_sctp_debug);
#endif

#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
	splx(s);
	thread_funnel_set(network_flock, funnel_state);
#endif

	printf("SCTP NKE: NKE loaded.\n");
	return KERN_SUCCESS;
}


kern_return_t SCTP_stop (kmod_info_t * ki, void * d) {

#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
	int s;
	int funnel_state;
#endif
	int err;
	
#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
	funnel_state = thread_funnel_set(network_flock, TRUE);
	s = splnet();
#endif

#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	lck_rw_lock_exclusive(sctppcbinfo.ipi_ep_mtx);
#endif
	if (!LIST_EMPTY(&sctppcbinfo.listhead)) {
		printf("SCTP NKE: There are still SCTP enpoints. NKE not unloaded\n");
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
		lck_rw_done(sctppcbinfo.ipi_ep_mtx);
#else
		splx(s);
		(void)thread_funnel_set(network_flock, funnel_state);
#endif
		return KERN_FAILURE;
	}
#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	lck_rw_done(sctppcbinfo.ipi_ep_mtx);
#endif

	sysctl_unregister_oid(&sysctl__net_inet_sctp_sendspace);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_recvspace);
/*
	sysctl_unregister_oid(&sysctl__net_inet_sctp_auto_asconf);
*/
	sysctl_unregister_oid(&sysctl__net_inet_sctp_ecn_enable);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_ecn_nonce);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_strict_sacks);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_loopback_nocsum);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_strict_init);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_peer_chkoh);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_maxburst);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_maxchunks);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_delayed_sack_time);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_heartbeat_interval);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_pmtu_raise_time);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_shutdown_guard_time);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_secret_lifetime);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_rto_max);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_rto_min);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_rto_initial);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_init_rto_max);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_valid_cookie_life);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_init_rtx_max);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_assoc_rtx_max);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_path_rtx_max);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_nr_outgoing_streams);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_cmt_on_off);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_cwnd_maxburst);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_early_fast_retran);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_use_rttvar_congctrl);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_deadlock_detect);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_early_fast_retran_msec);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_asconf_auth_nochk);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_auth_disable);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_auth_random_len);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_auth_hmac_id);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_abc_l_var);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_max_chained_mbufs);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_cmt_use_dac);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_do_sctp_drain);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_warm_crc_table);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_abort_at_limit);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_strict_data_order);
	sysctl_unregister_oid(&sysctl__net_inet_sctp_stats);
#ifdef SCTP_DEBUG
	sysctl_unregister_oid(&sysctl__net_inet_sctp_debug);
#endif
	sysctl_unregister_oid(&sysctl__net_inet_sctp);

#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	lck_mtx_lock(inetdomain.dom_mtx);
	lck_mtx_lock(inet6domain.dom_mtx);
#endif
	ip_protox[IPPROTO_SCTP]  = old_pr4;
	ip6_protox[IPPROTO_SCTP] = old_pr6;

	err  = net_del_proto(sctp4_dgram.pr_type,     sctp4_dgram.pr_protocol,     &inetdomain);
	err |= net_del_proto(sctp4_seqpacket.pr_type, sctp4_seqpacket.pr_protocol, &inetdomain);
	err |= net_del_proto(sctp4_stream.pr_type,    sctp4_stream.pr_protocol,    &inetdomain);
	err |= net_del_proto(sctp6_dgram.pr_type,     sctp6_dgram.pr_protocol,     &inet6domain);
	err |= net_del_proto(sctp6_seqpacket.pr_type, sctp6_seqpacket.pr_protocol, &inet6domain);
	err |= net_del_proto(sctp6_stream.pr_type,    sctp6_stream.pr_protocol,    &inet6domain);

#ifdef SCTP_APPLE_FINE_GRAINED_LOCKING
	sctp_finish();
	lck_mtx_unlock(inet6domain.dom_mtx);
	lck_mtx_unlock(inetdomain.dom_mtx);
#else
	splx(s);
	thread_funnel_set(network_flock, funnel_state);
#endif

	if (err) {
		printf("SCTP NKE: Not all protocol handlers could be removed.\n");
		return KERN_FAILURE;
	} else {
		printf("SCTP NKE: NKE unloaded.\n");
		return KERN_SUCCESS;
	}
}
