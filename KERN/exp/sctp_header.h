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

/* $KAME: sctp_header.h,v 1.14 2005/03/06 16:04:17 itojun Exp $	 */

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD:$");
#endif

#ifndef __sctp_header_h__
#define __sctp_header_h__


#include <sys/time.h>
#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>

/*
 * Parameter structures
 */
struct sctp_ipv4addr_param {
	struct sctp_paramhdr ph;/* type=SCTP_IPV4_PARAM_TYPE, len=8 */
	uint32_t addr;		/* IPV4 address */
};

struct sctp_ipv6addr_param {
	struct sctp_paramhdr ph;/* type=SCTP_IPV6_PARAM_TYPE, len=20 */
	uint8_t addr[16];	/* IPV6 address */
};

/* Cookie Preservative */
struct sctp_cookie_perserve_param {
	struct sctp_paramhdr ph;/* type=SCTP_COOKIE_PRESERVE, len=8 */
	uint32_t time;		/* time in ms to extend cookie */
};

/* Host Name Address */
struct sctp_host_name_param {
	struct sctp_paramhdr ph;/* type=SCTP_HOSTNAME_ADDRESS */
	char name[1];		/* host name */
};

/* supported address type */
struct sctp_supported_addr_param {
	struct sctp_paramhdr ph;/* type=SCTP_SUPPORTED_ADDRTYPE */
	uint16_t addr_type[1];	/* array of supported address types */
};

/* ECN parameter */
struct sctp_ecn_supported_param {
	struct sctp_paramhdr ph;/* type=SCTP_ECN_CAPABLE */
};


/* heartbeat info parameter */
struct sctp_heartbeat_info_param {
	struct sctp_paramhdr ph;
	uint32_t time_value_1;
	uint32_t time_value_2;
	uint32_t random_value1;
	uint32_t random_value2;
	uint16_t user_req;
	uint8_t addr_family;
	uint8_t addr_len;
	char address[SCTP_ADDRMAX];
};


/* draft-ietf-tsvwg-prsctp */
/* PR-SCTP supported parameter */
struct sctp_prsctp_supported_param {
	struct sctp_paramhdr ph;
};


/* draft-ietf-tsvwg-addip-sctp */
struct sctp_asconf_paramhdr {	/* an ASCONF "parameter" */
	struct sctp_paramhdr ph;/* a SCTP parameter header */
	uint32_t correlation_id;/* correlation id for this param */
};

struct sctp_asconf_addr_param {	/* an ASCONF address parameter */
	struct sctp_asconf_paramhdr aph;	/* asconf "parameter" */
	struct sctp_ipv6addr_param addrp;	/* max storage size */
};

struct sctp_asconf_addrv4_param {	/* an ASCONF address (v4) parameter */
	struct sctp_asconf_paramhdr aph;	/* asconf "parameter" */
	struct sctp_ipv4addr_param addrp;	/* max storage size */
};

struct sctp_supported_chunk_types_param {
	struct sctp_paramhdr ph;/* type = 0x8008  len = x */
	uint8_t chunk_types[0];
};


/* ECN Nonce: draft-ladha-sctp-ecn-nonce */
struct sctp_ecn_nonce_supported_param {
	struct sctp_paramhdr ph;/* type = 0x8001  len = 4 */
};


/*
 * Structures for DATA chunks
 */
struct sctp_data {
	uint32_t tsn;
	uint16_t stream_id;
	uint16_t stream_sequence;
	uint32_t protocol_id;
	/* user data follows */
};

struct sctp_data_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_data dp;
};

/*
 * Structures for the control chunks
 */

/* Initiate (INIT)/Initiate Ack (INIT ACK) */
struct sctp_init {
	uint32_t initiate_tag;	/* initiate tag */
	uint32_t a_rwnd;	/* a_rwnd */
	uint16_t num_outbound_streams;	/* OS */
	uint16_t num_inbound_streams;	/* MIS */
	uint32_t initial_tsn;	/* I-TSN */
	/* optional param's follow */
};

/* state cookie header */
struct sctp_state_cookie {	/* this is our definition... */
	uint8_t identification[16];	/* id of who we are */
	uint32_t cookie_life;	/* life I will award this cookie */
	uint32_t tie_tag_my_vtag;	/* my tag in old association */
	uint32_t tie_tag_peer_vtag;	/* peers tag in old association */
	uint32_t peers_vtag;	/* peers tag in INIT (for quick ref) */
	uint32_t my_vtag;	/* my tag in INIT-ACK (for quick ref) */
	struct timeval time_entered;	/* the time I built cookie */
	uint32_t address[4];	/* 4 ints/128 bits */
	uint32_t addr_type;	/* address type */
	uint32_t laddress[4];	/* my local from address */
	uint32_t laddr_type;	/* my local from address type */
	uint32_t scope_id;	/* v6 scope id for link-locals */
	uint16_t peerport;	/* port address of the peer in the INIT */
	uint16_t myport;	/* my port address used in the INIT */
	uint8_t ipv4_addr_legal;/* Are V4 addr legal? */
	uint8_t ipv6_addr_legal;/* Are V6 addr legal? */
	uint8_t local_scope;	/* IPv6 local scope flag */
	uint8_t site_scope;	/* IPv6 site scope flag */
	uint8_t ipv4_scope;	/* IPv4 private addr scope */
	uint8_t loopback_scope;	/* loopback scope information */
	uint16_t reserved;
	/*
	 * at the end is tacked on the INIT chunk and the INIT-ACK chunk
	 * (minus the cookie).
	 */
};

struct sctp_inv_mandatory_param {
	uint16_t cause;
	uint16_t length;
	uint32_t num_param;
	uint16_t param;
	/*
	 * We include this to 0 it since only a missing cookie will cause
	 * this error.
	 */
	uint16_t resv;
};

struct sctp_unresolv_addr {
	uint16_t cause;
	uint16_t length;
	uint16_t addr_type;
	uint16_t reserved;	/* Only one invalid addr type */
};

/* state cookie parameter */
struct sctp_state_cookie_param {
	struct sctp_paramhdr ph;
	struct sctp_state_cookie cookie;
};

struct sctp_init_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_init init;
};

struct sctp_init_msg {
	struct sctphdr sh;
	struct sctp_init_chunk msg;
};

/* ... used for both INIT and INIT ACK */
#define sctp_init_ack		sctp_init
#define sctp_init_ack_chunk	sctp_init_chunk
#define sctp_init_ack_msg	sctp_init_msg


/* Selective Ack (SACK) */
struct sctp_gap_ack_block {
	uint16_t start;		/* Gap Ack block start */
	uint16_t end;		/* Gap Ack block end */
};

struct sctp_sack {
	uint32_t cum_tsn_ack;	/* cumulative TSN Ack */
	uint32_t a_rwnd;	/* updated a_rwnd of sender */
	uint16_t num_gap_ack_blks;	/* number of Gap Ack blocks */
	uint16_t num_dup_tsns;	/* number of duplicate TSNs */
	/* struct sctp_gap_ack_block's follow */
	/* uint32_t duplicate_tsn's follow */
};

struct sctp_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_sack sack;
};


/* Heartbeat Request (HEARTBEAT) */
struct sctp_heartbeat {
	struct sctp_heartbeat_info_param hb_info;
};

struct sctp_heartbeat_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_heartbeat heartbeat;
};

/* ... used for Heartbeat Ack (HEARTBEAT ACK) */
#define sctp_heartbeat_ack		sctp_heartbeat
#define sctp_heartbeat_ack_chunk	sctp_heartbeat_chunk


/* Abort Asssociation (ABORT) */
struct sctp_abort_chunk {
	struct sctp_chunkhdr ch;
	/* optional error cause may follow */
};

struct sctp_abort_msg {
	struct sctphdr sh;
	struct sctp_abort_chunk msg;
};


/* Shutdown Association (SHUTDOWN) */
struct sctp_shutdown_chunk {
	struct sctp_chunkhdr ch;
	uint32_t cumulative_tsn_ack;
};


/* Shutdown Acknowledgment (SHUTDOWN ACK) */
struct sctp_shutdown_ack_chunk {
	struct sctp_chunkhdr ch;
};


/* Operation Error (ERROR) */
struct sctp_error_chunk {
	struct sctp_chunkhdr ch;
	/* optional error causes follow */
};


/* Cookie Echo (COOKIE ECHO) */
struct sctp_cookie_echo_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_state_cookie cookie;
};

/* Cookie Acknowledgment (COOKIE ACK) */
struct sctp_cookie_ack_chunk {
	struct sctp_chunkhdr ch;
};

/* Explicit Congestion Notification Echo (ECNE) */
struct sctp_ecne_chunk {
	struct sctp_chunkhdr ch;
	uint32_t tsn;
};

/* Congestion Window Reduced (CWR) */
struct sctp_cwr_chunk {
	struct sctp_chunkhdr ch;
	uint32_t tsn;
};

/* Shutdown Complete (SHUTDOWN COMPLETE) */
struct sctp_shutdown_complete_chunk {
	struct sctp_chunkhdr ch;
};

/* Oper error holding a stale cookie */
struct sctp_stale_cookie_msg {
	struct sctp_paramhdr ph;/* really an error cause */
	uint32_t time_usec;
};

struct sctp_adaptation_layer_indication {
	struct sctp_paramhdr ph;
	uint32_t indication;
};

struct sctp_cookie_while_shutting_down {
	struct sctphdr sh;
	struct sctp_chunkhdr ch;
	struct sctp_paramhdr ph;/* really an error cause */
};

struct sctp_shutdown_complete_msg {
	struct sctphdr sh;
	struct sctp_shutdown_complete_chunk shut_cmp;
};

/*
 * draft-ietf-tsvwg-addip-sctp
 */
/* Address/Stream Configuration Change (ASCONF) */
struct sctp_asconf_chunk {
	struct sctp_chunkhdr ch;
	uint32_t serial_number;
	/* lookup address parameter (mandatory) */
	/* asconf parameters follow */
};

/* Address/Stream Configuration Acknowledge (ASCONF ACK) */
struct sctp_asconf_ack_chunk {
	struct sctp_chunkhdr ch;
	uint32_t serial_number;
	/* asconf parameters follow */
};

/* draft-ietf-tsvwg-prsctp */
/* Forward Cumulative TSN (FORWARD TSN) */
struct sctp_forward_tsn_chunk {
	struct sctp_chunkhdr ch;
	uint32_t new_cumulative_tsn;
	/* stream/sequence pairs (sctp_strseq) follow */
};

struct sctp_strseq {
	uint16_t stream;
	uint16_t sequence;
};

struct sctp_forward_tsn_msg {
	struct sctphdr sh;
	struct sctp_forward_tsn_chunk msg;
};

/* should be a multiple of 4 - 1 aka 3/7/11 etc. */

#define SCTP_NUM_DB_TO_VERIFY 3

struct sctp_chunk_desc {
	uint8_t chunk_type;
	uint8_t data_bytes[SCTP_NUM_DB_TO_VERIFY];
	uint32_t tsn_ifany;
};


struct sctp_pktdrop_chunk {
	struct sctp_chunkhdr ch;
	uint32_t bottle_bw;
	uint32_t current_onq;
	uint16_t trunc_len;
	uint16_t reserved;
	uint8_t data[0];
};

/**********STREAM RESET STUFF ******************/

struct sctp_stream_reset_out_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;	/* monotonically increasing seq no */
	uint32_t response_seq;	/* if a response, the resp seq no */
	uint32_t send_reset_at_tsn;	/* last TSN I assigned outbound */
	uint16_t list_of_streams[0];	/* if not all list of streams */
};

struct sctp_stream_reset_in_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;
	uint16_t list_of_streams[0];	/* if not all list of streams */
};


struct sctp_stream_reset_tsn_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;
};

struct sctp_stream_reset_response {
	struct sctp_paramhdr ph;
	uint32_t response_seq;	/* if a response, the resp seq no */
	uint32_t result;
};

struct sctp_stream_reset_response_tsn {
	struct sctp_paramhdr ph;
	uint32_t response_seq;	/* if a response, the resp seq no */
	uint32_t result;
	uint32_t senders_next_tsn;
	uint32_t receivers_next_tsn;
};



#define SCTP_STREAM_RESET_NOTHING   0x00000000	/* Nothing for me to do */
#define SCTP_STREAM_RESET_PERFORMED 0x00000001	/* Did it */
#define SCTP_STREAM_RESET_DENIED    0x00000002	/* refused to do it */
#define SCTP_STREAM_RESET_ERROR_STR 0x00000003	/* bad Stream no */
#define SCTP_STREAM_RESET_TRY_LATER 0x00000004	/* collision, try again */
#define SCTP_STREAM_RESET_BAD_SEQNO 0x00000005	/* bad str-reset seq no */

/*
 * convience structures, note that if you are making a request for specific
 * streams then the request will need to be an overlay structure.
 */

struct sctp_stream_reset_out_req {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_out_request sr_req;
};

struct sctp_stream_reset_in_req {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_in_request sr_req;
};

struct sctp_stream_reset_tsn_req {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_tsn_request sr_req;
};

struct sctp_stream_reset_resp {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_response sr_resp;
};

/* respone only valid with a TSN request */
struct sctp_stream_reset_resp_tsn {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_response_tsn sr_resp;
};

/****************************************************/

/*
 * Authenticated chunks support draft-ietf-tsvwg-sctp-auth
 */
struct sctp_auth_random {
	struct sctp_paramhdr ph;/* type = 0x8002 */
	uint8_t random_data[0];
};

struct sctp_auth_chunk_list {
	struct sctp_paramhdr ph;/* type = 0x8003 */
	uint8_t chunk_types[0];
};

struct sctp_auth_hmac_algo {
	struct sctp_paramhdr ph;/* type = 0x8004 */
	uint16_t hmac_ids[0];
};

struct sctp_auth_chunk {
	struct sctp_chunkhdr ch;
	uint16_t shared_key_id;
	uint16_t hmac_id;
	uint8_t hmac[0];
};

struct sctp_auth_invalid_hmac {
	struct sctp_paramhdr ph;
	uint16_t hmac_id;
	uint16_t padding;
};

/*
 * we pre-reserve enough room for a ECNE or CWR AND a SACK with no missing
 * pieces. If ENCE is missing we could have a couple of blocks. This way we
 * optimize so we MOST likely can bundle a SACK/ECN with the smallest size
 * data chunk I will split into. We could increase throughput slightly by
 * taking out these two but the  24-sack/8-CWR i.e. 32 bytes I pre-reserve I
 * feel is worth it for now.
 */
#ifndef SCTP_MAX_OVERHEAD
#ifdef AF_INET6
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip6_hdr))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip6_hdr))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip6_hdr) + \
			   sizeof(struct sctphdr))

#else
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip) + \
			   sizeof(struct sctphdr))

#endif				/* AF_INET6 */
#endif				/* !SCTP_MAX_OVERHEAD */

#define SCTP_MED_V4_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			      sizeof(struct sctphdr) + \
			      sizeof(struct ip))

#define SCTP_MIN_V4_OVERHEAD (sizeof(struct ip) + \
			      sizeof(struct sctphdr))

#endif				/* !__sctp_header_h__ */
