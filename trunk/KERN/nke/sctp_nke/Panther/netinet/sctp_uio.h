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

/* $KAME: sctp_uio.h,v 1.11 2005/03/06 16:04:18 itojun Exp $	 */
#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD:$");
#endif

#ifndef __sctp_uio_h__
#define __sctp_uio_h__




#if ! defined(_KERNEL)
#include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>

typedef uint32_t sctp_assoc_t;

/* On/Off setup for subscription to events */
struct sctp_event_subscribe {
	uint8_t sctp_data_io_event;
	uint8_t sctp_association_event;
	uint8_t sctp_address_event;
	uint8_t sctp_send_failure_event;
	uint8_t sctp_peer_error_event;
	uint8_t sctp_shutdown_event;
	uint8_t sctp_partial_delivery_event;
	uint8_t sctp_adaptation_layer_event;
	uint8_t sctp_authentication_event;
	uint8_t sctp_stream_reset_events;
};

/* ancillary data types */
#define SCTP_INIT	0x0001
#define SCTP_SNDRCV	0x0002
#define SCTP_EXTRCV	0x0003
/*
 * ancillary data structures
 */
struct sctp_initmsg {
	uint32_t sinit_num_ostreams;
	uint32_t sinit_max_instreams;
	uint16_t sinit_max_attempts;
	uint16_t sinit_max_init_timeo;
};

struct sctp_sndrcvinfo {
	uint16_t sinfo_stream;
	uint16_t sinfo_ssn;
	uint16_t sinfo_flags;
	uint32_t sinfo_ppid;
	uint32_t sinfo_context;
	uint32_t sinfo_timetolive;
	uint32_t sinfo_tsn;
	uint32_t sinfo_cumtsn;
	sctp_assoc_t sinfo_assoc_id;
};

struct sctp_extrcvinfo {
	uint16_t sinfo_stream;
	uint16_t sinfo_ssn;
	uint16_t sinfo_flags;
	uint32_t sinfo_ppid;
	uint32_t sinfo_context;
	uint32_t sinfo_timetolive;
	uint32_t sinfo_tsn;
	uint32_t sinfo_cumtsn;
	sctp_assoc_t sinfo_assoc_id; 
	uint16_t next_flags;
	uint16_t next_stream; 
	uint32_t next_asocid;
	uint32_t next_length;
	uint32_t next_ppid;
};

#define SCTP_NO_NEXT_MSG           0x0000
#define SCTP_NEXT_MSG_AVAIL        0x0001
#define SCTP_NEXT_MSG_ISCOMPLETE   0x0002
#define SCTP_NEXT_MSG_IS_UNORDERED 0x0004

struct sctp_snd_all_completes {
	uint16_t sall_stream;
	uint16_t sall_flags;
	uint32_t sall_ppid;
	uint32_t sall_context;
	uint32_t sall_num_sent;
	uint32_t sall_num_failed;
};

/* Flags that go into the sinfo->sinfo_flags field */
#define SCTP_EOF 	  0x0100/* Start shutdown procedures */
#define SCTP_ABORT	  0x0200/* Send an ABORT to peer */
#define SCTP_UNORDERED 	  0x0400/* Message is un-ordered */
#define SCTP_ADDR_OVER	  0x0800/* Override the primary-address */
#define SCTP_SENDALL      0x1000/* Send this on all associations */
/* for the endpoint */

/* The lower byte is an enumeration of PR-SCTP policies */
#define SCTP_PR_SCTP_TTL  0x0001/* Time based PR-SCTP */
#define SCTP_PR_SCTP_BUF  0x0002/* Buffer based PR-SCTP */
#define SCTP_PR_SCTP_RTX  0x0003/* Number of retransmissions based PR-SCTP */

#define PR_SCTP_POLICY(x)      ((x) & 0xff)
#define PR_SCTP_ENABLED(x)     (PR_SCTP_POLICY(x) != 0)
#define PR_SCTP_TTL_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_TTL)
#define PR_SCTP_BUF_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_BUF)
#define PR_SCTP_RTX_ENABLED(x) (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_RTX)

/* Stat's */
struct sctp_pcbinfo {
	uint32_t ep_count;
	uint32_t asoc_count;
	uint32_t laddr_count;
	uint32_t raddr_count;
	uint32_t chk_count;
	uint32_t readq_count;
	uint32_t mbuf_track;
};

struct sctp_sockstat {
	sctp_assoc_t ss_assoc_id;
	uint32_t ss_total_sndbuf;
	uint32_t ss_total_mbuf_sndbuf;
	uint32_t ss_total_recv_buf;
};

/*
 * notification event structures
 */

/*
 * association change event
 */
struct sctp_assoc_change {
	uint16_t sac_type;
	uint16_t sac_flags;
	uint32_t sac_length;
	uint16_t sac_state;
	uint16_t sac_error;
	uint16_t sac_outbound_streams;
	uint16_t sac_inbound_streams;
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
	uint16_t spc_type;
	uint16_t spc_flags;
	uint32_t spc_length;
	struct sockaddr_storage spc_aaddr;
	uint32_t spc_state;
	uint32_t spc_error;
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
 * CAUTION: these are user exposed SCTP addr reachability states must be
 * compatible with SCTP_ADDR states in sctp_constants.h
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
#define SCTP_UNCONFIRMED	0x0200	/* SCTP_ADDR_UNCONFIRMED */

#ifdef SCTP_NOHEARTBEAT
#undef SCTP_NOHEARTBEAT
#endif
#define SCTP_NOHEARTBEAT	0x0040	/* SCTP_ADDR_NOHB */


/* remote error events */
struct sctp_remote_error {
	uint16_t sre_type;
	uint16_t sre_flags;
	uint32_t sre_length;
	uint16_t sre_error;
	sctp_assoc_t sre_assoc_id;
	uint8_t sre_data[4];
};

/* data send failure event */
struct sctp_send_failed {
	uint16_t ssf_type;
	uint16_t ssf_flags;
	uint32_t ssf_length;
	uint32_t ssf_error;
	struct sctp_sndrcvinfo ssf_info;
	sctp_assoc_t ssf_assoc_id;
	uint8_t ssf_data[4];
};

/* flag that indicates state of data */
#define SCTP_DATA_UNSENT	0x0001	/* inqueue never on wire */
#define SCTP_DATA_SENT		0x0002	/* on wire at failure */

/* shutdown event */
struct sctp_shutdown_event {
	uint16_t sse_type;
	uint16_t sse_flags;
	uint32_t sse_length;
	sctp_assoc_t sse_assoc_id;
};

/* Adaptation layer indication stuff */
struct sctp_adaptation_event {
	uint16_t sai_type;
	uint16_t sai_flags;
	uint32_t sai_length;
	uint32_t sai_adaptation_ind;
	sctp_assoc_t sai_assoc_id;
};

struct sctp_setadaptation {
	uint32_t ssb_adaptation_ind;
};

/* compatable old spelling */
struct sctp_adaption_event {
	uint16_t sai_type;
	uint16_t sai_flags;
	uint32_t sai_length;
	uint32_t sai_adaption_ind;
	sctp_assoc_t sai_assoc_id;
};

struct sctp_setadaption {
	uint32_t ssb_adaption_ind;
};


/*
 * Partial Delivery API event
 */
struct sctp_pdapi_event {
	uint16_t pdapi_type;
	uint16_t pdapi_flags;
	uint32_t pdapi_length;
	uint32_t pdapi_indication;
	sctp_assoc_t pdapi_assoc_id;
};

/* indication values */
#define SCTP_PARTIAL_DELIVERY_ABORTED	0x0001


/*
 * authentication key event
 */
struct sctp_authkey_event {
	uint16_t auth_type;
	uint16_t auth_flags;
	uint32_t auth_length;
	uint16_t auth_keynumber;
	uint16_t auth_altkeynumber;
	uint32_t auth_indication;
	sctp_assoc_t auth_assoc_id;
};

/* indication values */
#define SCTP_AUTH_NEWKEY	0x0001


/*
 * stream reset event
 */
struct sctp_stream_reset_event {
	uint16_t strreset_type;
	uint16_t strreset_flags;
	uint32_t strreset_length;
	sctp_assoc_t strreset_assoc_id;
	uint16_t strreset_list[0];
};

/* flags in strreset_flags field */
#define SCTP_STRRESET_INBOUND_STR  0x0001
#define SCTP_STRRESET_OUTBOUND_STR 0x0002
#define SCTP_STRRESET_ALL_STREAMS  0x0004
#define SCTP_STRRESET_STREAM_LIST  0x0008
#define SCTP_STRRESET_FAILED       0x0010


/* SCTP notification event */
struct sctp_tlv {
	uint16_t sn_type;
	uint16_t sn_flags;
	uint32_t sn_length;
};

union sctp_notification {
	struct sctp_tlv sn_header;
	struct sctp_assoc_change sn_assoc_change;
	struct sctp_paddr_change sn_paddr_change;
	struct sctp_remote_error sn_remote_error;
	struct sctp_send_failed sn_send_failed;
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
	uint32_t spp_hbinterval;
	uint16_t spp_pathmaxrxt;
	uint32_t spp_pathmtu;
	uint32_t spp_sackdelay;
	uint32_t spp_flags;
	uint32_t spp_ipv6_flowlabel;
	uint8_t spp_ipv4_tos;

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
	uint32_t spinfo_cwnd;
	uint32_t spinfo_srtt;
	uint32_t spinfo_rto;
	uint32_t spinfo_mtu;
};

struct sctp_rtoinfo {
	sctp_assoc_t srto_assoc_id;
	uint32_t srto_initial;
	uint32_t srto_max;
	uint32_t srto_min;
};

struct sctp_assocparams {
	sctp_assoc_t sasoc_assoc_id;
	uint16_t sasoc_asocmaxrxt;
	uint16_t sasoc_number_peer_destinations;
	uint32_t sasoc_peer_rwnd;
	uint32_t sasoc_local_rwnd;
	uint32_t sasoc_cookie_life;
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
	uint32_t ssto_timeout;
	uint32_t ssto_streamid_start;
	uint32_t ssto_streamid_end;
};

struct sctp_status {
	sctp_assoc_t sstat_assoc_id;
	int32_t sstat_state;
	uint32_t sstat_rwnd;
	uint16_t sstat_unackdata;
	uint16_t sstat_penddata;
	uint16_t sstat_instrms;
	uint16_t sstat_outstrms;
	uint32_t sstat_fragmentation_point;
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
	uint16_t sca_keynumber;
	uint8_t sca_key[0];
};

/* SCTP_HMAC_IDENT */
struct sctp_hmacalgo {
	uint16_t shmac_idents[0];
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
	uint16_t scact_keynumber;
};

/* SCTP_PEER_AUTH_CHUNKS / SCTP_LOCAL_AUTH_CHUNKS */
struct sctp_authchunks {
	sctp_assoc_t gauth_assoc_id;
	uint8_t gauth_chunks[0];
};

struct sctp_assoc_value {
	sctp_assoc_t assoc_id;
	uint32_t assoc_value;
};

#define MAX_ASOC_IDS_RET 255
struct sctp_assoc_ids {
	uint16_t asls_assoc_start;	/* array of index's start at 0 */
	uint8_t asls_numb_present;
	uint8_t asls_more_to_get;
	sctp_assoc_t asls_assoc_id[MAX_ASOC_IDS_RET];
};

struct sctp_cwnd_args {
	struct sctp_nets *net;	/* network to */
	uint32_t cwnd_new_value;/* cwnd in k */
	uint32_t inflight;	/* flightsize in k */
	uint32_t pseudo_cumack;
	int cwnd_augment;	/* increment to it */
	uint8_t meets_pseudo_cumack;
	uint8_t need_new_pseudo_cumack;
	uint8_t cnt_in_send;
	uint8_t cnt_in_str;
};

struct sctp_blk_args {
	uint32_t onmb;		/* in 1k bytes */
	uint32_t onsb;		/* in 1k bytes */
	uint16_t maxmb;		/* in 1k bytes */
	uint16_t maxsb;		/* in 1k bytes */
	uint16_t send_sent_qcnt;/* chnk cnt */
	uint16_t stream_qcnt;	/* chnk cnt */
	uint16_t chunks_on_oque;/* chunks out */
	uint16_t sndlen;	/* len of send being attempted */

};

/*
 * Max we can reset in one setting, note this is dictated not by the define
 * but the size of a mbuf cluster so don't change this define and think you
 * can specify more. You must do multiple resets if you want to reset more
 * than SCTP_MAX_EXPLICIT_STR_RESET.
 */
#define SCTP_MAX_EXPLICT_STR_RESET   1000

#define SCTP_RESET_LOCAL_RECV  0x0001
#define SCTP_RESET_LOCAL_SEND  0x0002
#define SCTP_RESET_BOTH        0x0003
#define SCTP_RESET_TSN         0x0004

struct sctp_stream_reset {
	sctp_assoc_t strrst_assoc_id;
	uint16_t strrst_flags;
	uint16_t strrst_num_streams;	/* 0 == ALL */
	uint16_t strrst_list[0];/* list if strrst_num_streams is not 0 */
};


struct sctp_get_nonce_values {
	sctp_assoc_t gn_assoc_id;
	uint32_t gn_peers_tag;
	uint32_t gn_local_tag;
};

/* Debugging logs */
struct sctp_str_log {
	uint32_t n_tsn;
	uint32_t e_tsn;
	uint16_t n_sseq;
	uint16_t e_sseq;
};

struct sctp_sb_log {
	uint32_t stcb;
	uint32_t so_sbcc;
	uint32_t stcb_sbcc;
	uint32_t incr;
};

struct sctp_fr_log {
	uint32_t largest_tsn;
	uint32_t largest_new_tsn;
	uint32_t tsn;
};

struct sctp_fr_map {
	uint32_t base;
	uint32_t cum;
	uint32_t high;
};

struct sctp_rwnd_log {
	uint32_t rwnd;
	uint32_t send_size;
	uint32_t overhead;
	uint32_t new_rwnd;
};

struct sctp_mbcnt_log {
	uint32_t total_queue_size;
	uint32_t size_change;
	uint32_t total_queue_mb_size;
	uint32_t mbcnt_change;
};

struct sctp_sack_log {
	uint32_t cumack;
	uint32_t oldcumack;
	uint32_t tsn;
	uint16_t numGaps;
	uint16_t numDups;
};

struct sctp_lock_log {
	uint32_t sock;
	uint32_t inp;
	uint8_t tcb_lock;
	uint8_t inp_lock;
	uint8_t info_lock;
	uint8_t sock_lock;
	uint8_t sockrcvbuf_lock;
	uint8_t socksndbuf_lock;
	uint8_t create_lock;
	uint8_t resv;
};

struct sctp_rto_log {
	uint32_t net;
	uint32_t rtt;
	uint32_t rttvar;
	uint8_t direction;
};

struct sctp_nagle_log {
	uint32_t stcb;
	uint32_t total_flight;
	uint32_t total_in_queue;
	uint16_t count_in_queue;
	uint16_t count_in_flight;
};

struct sctp_sbwake_log {
	uint32_t stcb;
	uint32_t tsn;
	uint32_t wake_cnt;
};

struct sctp_cwnd_log {
	uint32_t time_event;
	uint8_t from;
	uint8_t event_type;
	uint8_t resv[2];
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
	}     x;
};

struct sctp_cwnd_log_req {
	int num_in_log;		/* Number in log */
	int num_ret;		/* Number returned */
	int start_at;		/* start at this one */
	int end_at;		/* end at this one */
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
    int filling_sinfo,
    int *extra);

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
int sctp_peeloff __P((int, sctp_assoc_t));
int sctp_bindx __P((int, struct sockaddr *, int, int));
int sctp_connectx __P((int, struct sockaddr *, int));
int sctp_getaddrlen __P((sa_family_t));
int sctp_getpaddrs __P((int, sctp_assoc_t, struct sockaddr **));
void sctp_freepaddrs __P((struct sockaddr *));
int sctp_getladdrs __P((int, sctp_assoc_t, struct sockaddr **));
void sctp_freeladdrs __P((struct sockaddr *));
int sctp_opt_info __P((int, sctp_assoc_t, int, void *, socklen_t *));

ssize_t sctp_sendmsg
__P((int, const void *, size_t,
    const struct sockaddr *,
    socklen_t, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));

	ssize_t sctp_send __P((int sd, const void *msg, size_t len,
              const struct sctp_sndrcvinfo *sinfo, int flags));

	ssize_t
	sctp_sendx __P((int sd, const void *msg, size_t len,
               struct sockaddr *addrs, int addrcnt,
               struct sctp_sndrcvinfo *sinfo, int flags));
	ssize_t
	sctp_sendmsgx __P((int sd, const void *, size_t,
                  struct sockaddr *, int,
                  uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));

sctp_assoc_t
sctp_getassocid __P((int sd, struct sockaddr *sa));

	ssize_t sctp_recvmsg __P((int, void *, size_t, struct sockaddr *,
                 socklen_t *, struct sctp_sndrcvinfo *, int *));

__END_DECLS

#endif				/* !_KERNEL */
#endif				/* !__sctp_uio_h__ */
