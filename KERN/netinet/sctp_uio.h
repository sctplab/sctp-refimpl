/*	$KAME: sctp_uio.h,v 1.11 2005/03/06 16:04:18 itojun Exp $	*/

#ifndef __sctp_uio_h__
#define __sctp_uio_h__

/*-
 * Copyright (c) 2001-2006 Cisco Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#if ! defined(_KERNEL)
#include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>

typedef u_int32_t sctp_assoc_t;

/* On/Off setup for subscription to events */
struct sctp_event_subscribe {
	u_int8_t sctp_data_io_event;
	u_int8_t sctp_association_event;
	u_int8_t sctp_address_event;
	u_int8_t sctp_send_failure_event;
	u_int8_t sctp_peer_error_event;
	u_int8_t sctp_shutdown_event;
	u_int8_t sctp_partial_delivery_event;
	u_int8_t sctp_adaptation_layer_event;
	u_int8_t sctp_authentication_event;
	u_int8_t sctp_stream_reset_events;
};

/* ancillary data types */
#define SCTP_INIT	0x0001
#define SCTP_SNDRCV	0x0002

/*
 * ancillary data structures
 */
struct sctp_initmsg {
	u_int32_t sinit_num_ostreams;
	u_int32_t sinit_max_instreams;
	u_int16_t sinit_max_attempts;
	u_int16_t sinit_max_init_timeo;
};

struct sctp_sndrcvinfo {
	u_int16_t sinfo_stream;
	u_int16_t sinfo_ssn;
	u_int16_t sinfo_flags;
	u_int32_t sinfo_ppid;
	u_int32_t sinfo_context;
	u_int32_t sinfo_timetolive;
	u_int32_t sinfo_tsn;
	u_int32_t sinfo_cumtsn;
	sctp_assoc_t sinfo_assoc_id;
};

struct sctp_snd_all_completes {
	u_int16_t sall_stream;
	u_int16_t sall_flags;
	u_int32_t sall_ppid;
	u_int32_t sall_context;
	u_int32_t sall_num_sent;
	u_int32_t sall_num_failed;
};

/* Flags that go into the sinfo->sinfo_flags field */
#define SCTP_EOF 	  0x0100	/* Start shutdown procedures */
#define SCTP_ABORT	  0x0200	/* Send an ABORT to peer */
#define SCTP_UNORDERED 	  0x0400	/* Message is un-ordered */
#define SCTP_ADDR_OVER	  0x0800	/* Override the primary-address */
#define SCTP_SENDALL      0x1000	/* Send this on all associations */
					/* for the endpoint */

/* The lower byte is an enumeration of PR-SCTP policies */
#define SCTP_PR_SCTP_TTL  0x0001	/* Time based PR-SCTP */
#define SCTP_PR_SCTP_BUF  0x0002	/* Buffer based PR-SCTP */
#define SCTP_PR_SCTP_RTX  0x0003	/* Number of retransmissions based PR-SCTP */

#define PR_SCTP_POLICY(x)      ((x) & 0xff)
#define PR_SCTP_ENABLED(x)     (PR_SCTP_POLICY(x) != 0)
#define PR_SCTP_TTL_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_TTL)
#define PR_SCTP_BUF_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_BUF)
#define PR_SCTP_RTX_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_RTX)

/* Stat's */
struct sctp_pcbinfo {
	u_int32_t ep_count;
	u_int32_t asoc_count;
	u_int32_t laddr_count;
	u_int32_t raddr_count;
	u_int32_t chk_count;
	u_int32_t readq_count;
	u_int32_t mbuf_track;
};

struct sctp_sockstat {
	sctp_assoc_t ss_assoc_id;
	u_int32_t ss_total_sndbuf;
	u_int32_t ss_total_mbuf_sndbuf;
	u_int32_t ss_total_recv_buf;
};

/*
 * notification event structures
 */

/*
 * association change event
 */
struct sctp_assoc_change {
	u_int16_t sac_type;
	u_int16_t sac_flags;
	u_int32_t sac_length;
	u_int16_t sac_state;
	u_int16_t sac_error;
	u_int16_t sac_outbound_streams;
	u_int16_t sac_inbound_streams;
	sctp_assoc_t sac_assoc_id;
};
/* sac_state values */
#define SCTP_COMM_UP		0x0001
#define SCTP_COMM_LOST		0x0002
#define SCTP_RESTART		0x0003
#define SCTP_SHUTDOWN_COMP	0x0004
#define SCTP_CANT_STR_ASSOC	0x0005


/*
 * Address event
 */
struct sctp_paddr_change {
	u_int16_t spc_type;
	u_int16_t spc_flags;
	u_int32_t spc_length;
	struct sockaddr_storage spc_aaddr;
	u_int32_t spc_state;
	u_int32_t spc_error;
	sctp_assoc_t spc_assoc_id;
};
/* paddr state values */
#define SCTP_ADDR_AVAILABLE	0x0001
#define SCTP_ADDR_UNREACHABLE	0x0002
#define SCTP_ADDR_REMOVED	0x0003
#define SCTP_ADDR_ADDED		0x0004
#define SCTP_ADDR_MADE_PRIM	0x0005
#define SCTP_ADDR_CONFIRMED	0x0006

/*
 * CAUTION: these are user exposed SCTP addr reachability states
 *          must be compatible with SCTP_ADDR states in sctp_constants.h
 */
#ifdef SCTP_ACTIVE
#undef SCTP_ACTIVE
#endif
#define SCTP_ACTIVE		0x0001	/* SCTP_ADDR_REACHABLE */

#ifdef SCTP_INACTIVE
#undef SCTP_INACTIVE
#endif
#define SCTP_INACTIVE		0x0002	/* SCTP_ADDR_NOT_REACHABLE */

#ifdef SCTP_UNCONFIRMED
#undef SCTP_UNCONFIRMED
#endif
#define SCTP_UNCONFIRMED	0x0200  /* SCTP_ADDR_UNCONFIRMED */

#ifdef SCTP_NOHEARTBEAT
#undef SCTP_NOHEARTBEAT
#endif
#define SCTP_NOHEARTBEAT	0x0040 /* SCTP_ADDR_NOHB */


/* remote error events */
struct sctp_remote_error {
	u_int16_t sre_type;
	u_int16_t sre_flags;
	u_int32_t sre_length;
	u_int16_t sre_error;
	sctp_assoc_t sre_assoc_id;
	u_int8_t  sre_data[4];
};

/* data send failure event */
struct sctp_send_failed {
	u_int16_t ssf_type;
	u_int16_t ssf_flags;
	u_int32_t ssf_length;
	u_int32_t ssf_error;
	struct sctp_sndrcvinfo ssf_info;
	sctp_assoc_t ssf_assoc_id;
	u_int8_t ssf_data[4];
};

/* flag that indicates state of data */
#define SCTP_DATA_UNSENT	0x0001	/* inqueue never on wire */
#define SCTP_DATA_SENT		0x0002	/* on wire at failure */

/* shutdown event */
struct sctp_shutdown_event {
	u_int16_t	sse_type;
	u_int16_t	sse_flags;
	u_int32_t	sse_length;
	sctp_assoc_t	sse_assoc_id;
};

/* Adaptation layer indication stuff */
struct sctp_adaptation_event {
	u_int16_t	sai_type;
	u_int16_t	sai_flags;
	u_int32_t	sai_length;
	u_int32_t	sai_adaptation_ind;
	sctp_assoc_t	sai_assoc_id;
};

struct sctp_setadaptation {
	u_int32_t	ssb_adaptation_ind;
};
/* compatable old spelling */
struct sctp_adaption_event {
	u_int16_t	sai_type;
	u_int16_t	sai_flags;
	u_int32_t	sai_length;
	u_int32_t	sai_adaption_ind;
	sctp_assoc_t	sai_assoc_id;
};

struct sctp_setadaption {
	u_int32_t	ssb_adaption_ind;
};


/*
 * Partial Delivery API event
 */
struct sctp_pdapi_event {
	u_int16_t	pdapi_type;
	u_int16_t	pdapi_flags;
	u_int32_t	pdapi_length;
	u_int32_t	pdapi_indication;
	sctp_assoc_t	pdapi_assoc_id;
};
/* indication values */
#define SCTP_PARTIAL_DELIVERY_ABORTED	0x0001


/*
 * authentication key event
 */
struct sctp_authkey_event {
	u_int16_t	auth_type;
	u_int16_t	auth_flags;
	u_int32_t	auth_length;
	u_int32_t	auth_keynumber;
	u_int32_t	auth_altkeynumber;
	u_int32_t	auth_indication;
	sctp_assoc_t	auth_assoc_id;
};
/* indication values */
#define SCTP_AUTH_NEWKEY	0x0001


/*
 * stream reset event
 */
struct sctp_stream_reset_event {
	u_int16_t	strreset_type;
	u_int16_t	strreset_flags;
	u_int32_t	strreset_length;
	sctp_assoc_t    strreset_assoc_id;
	u_int16_t       strreset_list[0];
};

/* flags in strreset_flags field */
#define SCTP_STRRESET_INBOUND_STR  0x0001
#define SCTP_STRRESET_OUTBOUND_STR 0x0002
#define SCTP_STRRESET_ALL_STREAMS  0x0004
#define SCTP_STRRESET_STREAM_LIST  0x0008
#define SCTP_STRRESET_FAILED       0x0010


/* SCTP notification event */
struct sctp_tlv {
	u_int16_t sn_type;
	u_int16_t sn_flags;
	u_int32_t sn_length;
};

union sctp_notification {
	struct sctp_tlv sn_header;
	struct sctp_assoc_change sn_assoc_change;
	struct sctp_paddr_change sn_paddr_change;
	struct sctp_remote_error sn_remote_error;
	struct sctp_send_failed	sn_send_failed;
	struct sctp_shutdown_event sn_shutdown_event;
	struct sctp_adaptation_event sn_adaptation_event;
	/* compatability same as above */
	struct sctp_adaption_event sn_adaption_event;
	struct sctp_pdapi_event sn_pdapi_event;
	struct sctp_authkey_event sn_auth_event;
	struct sctp_stream_reset_event sn_strreset_event;
};
/* notification types */
#define SCTP_ASSOC_CHANGE		0x0001
#define SCTP_PEER_ADDR_CHANGE		0x0002
#define SCTP_REMOTE_ERROR		0x0003
#define SCTP_SEND_FAILED		0x0004
#define SCTP_SHUTDOWN_EVENT		0x0005
#define SCTP_ADAPTATION_INDICATION	0x0006
/* same as above */
#define SCTP_ADAPTION_INDICATION	0x0006
#define SCTP_PARTIAL_DELIVERY_EVENT	0x0007
#define SCTP_AUTHENTICATION_EVENT	0x0008
#define SCTP_STREAM_RESET_EVENT		0x0009


/*
 * socket option structs
 */

struct sctp_paddrparams {
	sctp_assoc_t spp_assoc_id;
	struct sockaddr_storage spp_address;
	u_int32_t spp_hbinterval;
	u_int16_t spp_pathmaxrxt;
	u_int32_t spp_pathmtu;
	u_int32_t spp_sackdelay;
	u_int32_t spp_flags;
	u_int32_t spp_ipv6_flowlabel;
	u_int8_t  spp_ipv4_tos;

};

#define SPP_HB_ENABLE		0x00000001
#define SPP_HB_DISABLE		0x00000002
#define SPP_HB_DEMAND		0x00000004
#define SPP_PMTUD_ENABLE	0x00000008
#define SPP_PMTUD_DISABLE	0x00000010
#define SPP_SACKDELAY_ENABLE	0x00000020
#define SPP_SACKDELAY_DISABLE	0x00000040
#define SPP_HB_TIME_IS_ZERO     0x00000080
#define SPP_IPV6_FLOWLABEL      0x00000100
#define SPP_IPV4_TOS            0x00000200

struct sctp_paddrinfo {
	sctp_assoc_t spinfo_assoc_id;
	struct sockaddr_storage spinfo_address;
	int32_t spinfo_state;
	u_int32_t spinfo_cwnd;
	u_int32_t spinfo_srtt;
	u_int32_t spinfo_rto;
	u_int32_t spinfo_mtu;
};

struct sctp_rtoinfo {
	sctp_assoc_t srto_assoc_id;
	u_int32_t srto_initial;
	u_int32_t srto_max;
	u_int32_t srto_min;
};

struct sctp_assocparams {
	sctp_assoc_t sasoc_assoc_id;
	u_int16_t sasoc_asocmaxrxt;
	u_int16_t sasoc_number_peer_destinations;
	u_int32_t sasoc_peer_rwnd;
	u_int32_t sasoc_local_rwnd;
	u_int32_t sasoc_cookie_life;
};

struct sctp_setprim {
	sctp_assoc_t ssp_assoc_id;
	struct sockaddr_storage ssp_addr;
};

struct sctp_setpeerprim {
	sctp_assoc_t sspp_assoc_id;
	struct sockaddr_storage sspp_addr;
};

struct sctp_getaddresses {
	sctp_assoc_t sget_assoc_id;
	/* addr is filled in for N * sockaddr_storage */
	struct sockaddr addr[1];
};

struct sctp_setstrm_timeout {
	sctp_assoc_t ssto_assoc_id;
	u_int32_t ssto_timeout;
	u_int32_t ssto_streamid_start;
	u_int32_t ssto_streamid_end;
};

struct sctp_status {
	sctp_assoc_t sstat_assoc_id;
	int32_t sstat_state;
	u_int32_t sstat_rwnd;
	u_int16_t sstat_unackdata;
	u_int16_t sstat_penddata;
	u_int16_t sstat_instrms;
	u_int16_t sstat_outstrms;
	u_int32_t sstat_fragmentation_point;
	struct sctp_paddrinfo sstat_primary;
};

/*
 * AUTHENTICATION support
 */
/* SCTP_AUTH_CHUNK */
struct sctp_authchunk {
    uint8_t sauth_chunk;
};

/* SCTP_AUTH_KEY */
struct sctp_authkey {
    sctp_assoc_t sca_assoc_id;
    uint32_t sca_keynumber;
    uint8_t sca_key[0];
};

/* SCTP_HMAC_IDENT */
struct sctp_hmacalgo {
    uint32_t shmac_idents[0];
};
/* AUTH hmac_id */
#define SCTP_AUTH_HMAC_ID_RSVD		0x0000
#define SCTP_AUTH_HMAC_ID_SHA1		0x0001	/* default, mandatory */
#define SCTP_AUTH_HMAC_ID_MD5		0x0002
#define SCTP_AUTH_HMAC_ID_SHA224	0x8001
#define SCTP_AUTH_HMAC_ID_SHA256	0x8002
#define SCTP_AUTH_HMAC_ID_SHA384	0x8003
#define SCTP_AUTH_HMAC_ID_SHA512	0x8004


/* SCTP_AUTH_ACTIVE_KEY / SCTP_AUTH_DELETE_KEY */
struct sctp_authkeyid {
    sctp_assoc_t scact_assoc_id;
    uint32_t scact_keynumber;
};

/* SCTP_PEER_AUTH_CHUNKS / SCTP_LOCAL_AUTH_CHUNKS */
struct sctp_authchunks {
    sctp_assoc_t gauth_assoc_id;
    uint8_t gauth_chunks[0];
};

struct sctp_assoc_value {
	sctp_assoc_t assoc_id;
	u_int32_t assoc_value;
};

#define MAX_ASOC_IDS_RET 255
struct sctp_assoc_ids {
	u_int16_t asls_assoc_start;	/* array of index's start at 0 */
	u_int8_t asls_numb_present;
	u_int8_t asls_more_to_get;
	sctp_assoc_t asls_assoc_id[MAX_ASOC_IDS_RET];
};

struct sctp_cwnd_args {
	struct sctp_nets *net;		/* network to */
	u_int32_t cwnd_new_value;	/* cwnd in k */
	u_int32_t inflight;		/* flightsize in k */
	u_int32_t pseudo_cumack;
	int cwnd_augment;		/* increment to it */
	u_int8_t meets_pseudo_cumack;
	u_int8_t need_new_pseudo_cumack;
	u_int8_t cnt_in_send;
	u_int8_t cnt_in_str;
};

struct sctp_blk_args {
	u_int32_t onmb;			/* in 1k bytes */
	u_int32_t onsb;			/* in 1k bytes */
	u_int16_t maxmb;		/* in 1k bytes */
	u_int16_t maxsb;		/* in 1k bytes */
	u_int16_t send_sent_qcnt;	/* chnk cnt */
	u_int16_t stream_qcnt;		/* chnk cnt */
	u_int16_t chunks_on_oque;       /* chunks out */
	u_int16_t sndlen;		/* len of send being attempted */

};

/*
 * Max we can reset in one setting, note this is dictated not by the
 * define but the size of a mbuf cluster so don't change this define
 * and think you can specify more. You must do multiple resets if you
 * want to reset more than SCTP_MAX_EXPLICIT_STR_RESET.
 */
#define SCTP_MAX_EXPLICT_STR_RESET   1000

#define SCTP_RESET_LOCAL_RECV  0x0001
#define SCTP_RESET_LOCAL_SEND  0x0002
#define SCTP_RESET_BOTH        0x0003
#define SCTP_RESET_TSN         0x0004

struct sctp_stream_reset {
	sctp_assoc_t strrst_assoc_id;
	u_int16_t    strrst_flags;
	u_int16_t    strrst_num_streams;	/* 0 == ALL */
	u_int16_t    strrst_list[0];		/* list if strrst_num_streams is not 0*/
};


struct sctp_get_nonce_values {
	sctp_assoc_t gn_assoc_id;
	u_int32_t    gn_peers_tag;
	u_int32_t    gn_local_tag;
};

/* Debugging logs */
struct sctp_str_log {
	u_int32_t n_tsn;
	u_int32_t e_tsn;
	u_int16_t n_sseq;
	u_int16_t e_sseq;
};

struct sctp_sb_log {
	u_int32_t stcb;
	u_int32_t so_sbcc;
	u_int32_t stcb_sbcc;
	u_int32_t incr;
};

struct sctp_fr_log {
	u_int32_t largest_tsn;
	u_int32_t largest_new_tsn;
	u_int32_t tsn;
};

struct sctp_fr_map {
	u_int32_t base;
	u_int32_t cum;
	u_int32_t high;
};

struct sctp_rwnd_log {
	u_int32_t rwnd;
	u_int32_t send_size;
	u_int32_t overhead;
	u_int32_t new_rwnd;
};

struct sctp_mbcnt_log {
	u_int32_t total_queue_size;
	u_int32_t size_change;
	u_int32_t total_queue_mb_size;
	u_int32_t mbcnt_change;
};

struct sctp_sack_log {
	u_int32_t cumack;
	u_int32_t oldcumack;
	u_int32_t tsn;
	u_int16_t numGaps;
	u_int16_t numDups;
};

struct sctp_lock_log {
	u_int32_t sock;
	u_int32_t inp;
	u_int8_t tcb_lock;
	u_int8_t inp_lock;
	u_int8_t info_lock;
	u_int8_t sock_lock;
	u_int8_t sockrcvbuf_lock;
	u_int8_t socksndbuf_lock;
	u_int8_t create_lock;
	u_int8_t resv;
};

struct sctp_rto_log {
	u_int32_t net;
	u_int32_t rtt;
	u_int32_t rttvar;
	u_int8_t  direction;
};

struct sctp_nagle_log {
	u_int32_t stcb;
	u_int32_t total_flight;
	u_int32_t total_in_queue;
	u_int16_t count_in_queue;
	u_int16_t count_in_flight;
};

struct sctp_sbwake_log {
	u_int32_t stcb;
	u_int32_t tsn;
	u_int32_t wake_cnt;
};

struct sctp_cwnd_log{
	u_int32_t time_event;
	u_int8_t from;
	u_int8_t event_type;
	u_int8_t resv[2];
	union {
		struct sctp_blk_args blk;
		struct sctp_cwnd_args cwnd;
		struct sctp_str_log strlog;
		struct sctp_fr_log fr;
		struct sctp_fr_map map;
		struct sctp_rwnd_log rwnd;
		struct sctp_mbcnt_log mbcnt;
  		struct sctp_sack_log sack;
		struct sctp_lock_log lock;
		struct sctp_rto_log rto;
		struct sctp_sb_log sb;
		struct sctp_nagle_log nagle;
		struct sctp_sbwake_log wake;
	} x;
};

struct sctp_cwnd_log_req{
	int num_in_log;	/* Number in log */
	int num_ret;	/* Number returned */
	int start_at;	/* start at this one */
	int end_at;	/* end at this one */
	struct sctp_cwnd_log log[0];
};


/*
 * Kernel defined for sctp_send
 */
#if defined(_KERNEL)
int
sctp_lower_sosend(struct socket *so,
		  struct sockaddr *addr,
		  struct uio *uio,
		  struct mbuf *top,
		  struct mbuf *control,
		  int flags,
		  int use_rcvinfo,
		  struct sctp_sndrcvinfo *srcv,		  
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
		  struct thread *p
#else
		  struct proc *p
#endif
	);

int
sctp_sorecvmsg(struct socket *so,
	       struct uio *uio,
	       struct mbuf **mp,
	       struct sockaddr *from,
	       int fromlen,
	       int *msg_flags, 
	       struct sctp_sndrcvinfo *sinfo,
	       int filling_sinfo);
#endif

/*
 * API system calls
 */
#if (defined(__APPLE__) && defined(KERNEL))
#ifndef _KERNEL
#define _KERNEL
#endif
#endif

#if !(defined(_KERNEL))

__BEGIN_DECLS
int	sctp_peeloff	__P((int, sctp_assoc_t));
int	sctp_bindx	__P((int, struct sockaddr *, int, int));
int     sctp_connectx   __P((int, struct sockaddr *, int));
int     sctp_getaddrlen __P((sa_family_t));
int	sctp_getpaddrs	__P((int, sctp_assoc_t, struct sockaddr **));
void	sctp_freepaddrs	__P((struct sockaddr *));
int	sctp_getladdrs	__P((int, sctp_assoc_t, struct sockaddr **));
void	sctp_freeladdrs	__P((struct sockaddr *));
int     sctp_opt_info   __P((int, sctp_assoc_t, int, void *, socklen_t *));

ssize_t sctp_sendmsg    __P((int, const void *, size_t,
	const struct sockaddr *,
	socklen_t, u_int32_t, u_int32_t, u_int16_t, u_int32_t, u_int32_t));

ssize_t sctp_send       __P((int sd, const void *msg, size_t len,
	const struct sctp_sndrcvinfo *sinfo,int flags));

ssize_t
sctp_sendx __P((int sd, const void *msg, size_t len,
		struct sockaddr *addrs, int addrcnt,
		struct sctp_sndrcvinfo *sinfo, int flags));
ssize_t
sctp_sendmsgx __P((int sd, const void *, size_t,
		   struct sockaddr *, int,
		   u_int32_t, u_int32_t, u_int16_t, u_int32_t, u_int32_t));

sctp_assoc_t
sctp_getassocid __P((int sd, struct sockaddr *sa));

ssize_t sctp_recvmsg __P((int, void *, size_t, struct sockaddr *,
			  socklen_t *, struct sctp_sndrcvinfo *, int *));

__END_DECLS

#endif /* !_KERNEL */
#endif /* !__sctp_uio_h__ */
