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

/* $KAME: sctp_pcb.h,v 1.21 2005/07/16 01:18:47 suz Exp $	 */

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD:$");
#endif

#ifndef __sctp_pcb_h__
#define __sctp_pcb_h__



/*
 * We must have V6 so the size of the proto can be calculated. Otherwise we
 * would not allocate enough for Net/Open BSD :-<
 */

#if defined(__FreeBSD__) && __FreeBSD_version > 500000
#if defined(_KERNEL)
#include <net/pfil.h>
#endif
#endif

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <net/if_types.h>
#if defined( __FreeBSD__) || defined(__APPLE__)
#include <net/if_var.h>
#endif
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6protosw.h>
#include <netinet6/in6_var.h>
#if defined(__OpenBSD__) || defined(__APPLE__)
#include <netinet/in_pcb.h>
#else
#include <netinet6/in6_pcb.h>
#endif

#if defined(__OpenBSD__) || defined(__FreeBSD__)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#endif

#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>

LIST_HEAD(sctppcbhead, sctp_inpcb);
LIST_HEAD(sctpasochead, sctp_tcb);
LIST_HEAD(sctpladdr, sctp_laddr);
LIST_HEAD(sctpvtaghead, sctp_tagblock);
TAILQ_HEAD(sctp_readhead, sctp_queued_to_read);
TAILQ_HEAD(sctp_streamhead, sctp_stream_queue_pending);

#include <netinet/sctp_structs.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_auth.h>

/*
 * PCB flags (in sctp_flags bitmask)
 */
#define SCTP_PCB_FLAGS_UDPTYPE		0x00000001
#define SCTP_PCB_FLAGS_TCPTYPE		0x00000002
#define SCTP_PCB_FLAGS_BOUNDALL		0x00000004
#define SCTP_PCB_FLAGS_ACCEPTING	0x00000008
#define SCTP_PCB_FLAGS_UNBOUND		0x00000010
/* TCP model support */
#define SCTP_PCB_FLAGS_CONNECTED	0x00200000
#define SCTP_PCB_FLAGS_IN_TCPPOOL	0x00400000
#define SCTP_PCB_FLAGS_DONT_WAKE	0x00800000
#define SCTP_PCB_FLAGS_WAKEOUTPUT	0x01000000
#define SCTP_PCB_FLAGS_WAKEINPUT	0x02000000
#define SCTP_PCB_FLAGS_BOUND_V6		0x04000000
#define SCTP_PCB_FLAGS_NEEDS_MAPPED_V4	0x08000000
#define SCTP_PCB_FLAGS_BLOCKING_IO	0x10000000
#define SCTP_PCB_FLAGS_SOCKET_GONE	0x20000000
#define SCTP_PCB_FLAGS_SOCKET_ALLGONE	0x40000000
/* flags to copy to new PCB */
#define SCTP_PCB_COPY_FLAGS		0x0e000004


/*
 * PCB Features (in sctp_features bitmask)
 */
#define SCTP_PCB_FLAGS_EXT_RCVINFO      0x00000004
#define SCTP_PCB_FLAGS_DONOT_HEARTBEAT  0x00000008
#define SCTP_PCB_FLAGS_FRAG_INTERLEAVE  0x00000010
#define SCTP_PCB_FLAGS_DO_ASCONF	0x00000020
#define SCTP_PCB_FLAGS_AUTO_ASCONF	0x00000040
/* socket options */
#define SCTP_PCB_FLAGS_NODELAY		0x00000100
#define SCTP_PCB_FLAGS_AUTOCLOSE	0x00000200
#define SCTP_PCB_FLAGS_RECVDATAIOEVNT	0x00000400
#define SCTP_PCB_FLAGS_RECVASSOCEVNT	0x00000800
#define SCTP_PCB_FLAGS_RECVPADDREVNT	0x00001000
#define SCTP_PCB_FLAGS_RECVPEERERR	0x00002000
#define SCTP_PCB_FLAGS_RECVSENDFAILEVNT	0x00004000
#define SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT	0x00008000
#define SCTP_PCB_FLAGS_ADAPTATIONEVNT	0x00010000
#define SCTP_PCB_FLAGS_PDAPIEVNT	0x00020000
#define SCTP_PCB_FLAGS_AUTHEVNT		0x00040000
#define SCTP_PCB_FLAGS_STREAM_RESETEVNT 0x00080000
#define SCTP_PCB_FLAGS_NO_FRAGMENT	0x00100000
#define SCTP_PCB_FLAGS_EXPLICIT_EOR     0x00200000


#define SCTP_PCBHASH_ALLADDR(port, mask) (port & mask)
#define SCTP_PCBHASH_ASOC(tag, mask) (tag & mask)

struct sctp_laddr {
	LIST_ENTRY(sctp_laddr) sctp_nxt_addr;	/* next in list */
	struct ifaddr *ifa;
	int action;		/* Only used in delayed asconf stuff */
};

struct sctp_block_entry {
	int error;
};

struct sctp_timewait {
	uint32_t tv_sec_at_expire;	/* the seconds from boot to expire */
	uint32_t v_tag;		/* the vtag that can not be reused */
};

struct sctp_tagblock {
	LIST_ENTRY(sctp_tagblock) sctp_nxt_tagblock;
	struct sctp_timewait vtag_block[SCTP_NUMBER_IN_VTAG_BLOCK];
};


struct sctp_epinfo {
	struct sctpasochead *sctp_asochash;
	u_long hashasocmark;

	struct sctppcbhead *sctp_ephash;
	u_long hashmark;

	struct sctpasochead *sctp_restarthash;
	u_long hashrestartmark;
	/*
	 * The TCP model represents a substantial overhead in that we get an
	 * additional hash table to keep explicit connections in. The
	 * listening TCP endpoint will exist in the usual ephash above and
	 * accept only INIT's. It will be incapable of sending off an INIT.
	 * When a dg arrives we must look in the normal ephash. If we find a
	 * TCP endpoint that will tell us to go to the specific endpoint
	 * hash and re-hash to find the right assoc/socket. If we find a UDP
	 * model socket we then must complete the lookup. If this fails,
	 * i.e. no association can be found then we must continue to see if
	 * a sctp_peeloff()'d socket is in the tcpephash (a spun off socket
	 * acts like a TCP model connected socket).
	 */
	struct sctppcbhead *sctp_tcpephash;
	u_long hashtcpmark;
	uint32_t hashtblsize;

	struct sctppcbhead listhead;
	struct sctpladdr addr_wq;

#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	struct inpcbhead inplisthead;
#endif
	struct sctpiterators iteratorhead;

	/* ep zone info */
#if defined(__FreeBSD__) || defined(__APPLE__)
#if __FreeBSD_version >= 500000
	struct uma_zone *ipi_zone_ep;
	struct uma_zone *ipi_zone_asoc;
	struct uma_zone *ipi_zone_laddr;
	struct uma_zone *ipi_zone_net;
	struct uma_zone *ipi_zone_chunk;
	struct uma_zone *ipi_zone_readq;
	struct uma_zone *ipi_zone_strmoq;
#else
	struct vm_zone *ipi_zone_ep;
	struct vm_zone *ipi_zone_asoc;
	struct vm_zone *ipi_zone_laddr;
	struct vm_zone *ipi_zone_net;
	struct vm_zone *ipi_zone_chunk;
	struct vm_zone *ipi_zone_readq;
	struct vm_zone *ipi_zone_strmoq;
#endif
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct pool ipi_zone_ep;
	struct pool ipi_zone_asoc;
	struct pool ipi_zone_laddr;
	struct pool ipi_zone_net;
	struct pool ipi_zone_chunk;
	struct pool ipi_zone_readq;
	struct pool ipi_zone_strmoq;
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx logging_mtx;
	struct mtx ipi_ep_mtx;
	struct mtx it_mtx;
	struct mtx ipi_count_mtx;
	struct mtx ipi_addr_mtx;
#elif defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
#ifdef _KERN_LOCKS_H_
	lck_grp_attr_t *mtx_grp_attr;
	lck_grp_t *mtx_grp;
	lck_attr_t *mtx_attr;
	lck_rw_t *ipi_ep_mtx;
	lck_mtx_t *it_mtx;
	lck_mtx_t *ipi_count_mtx;
	lck_mtx_t *logging_mtx;
#else
	void *mtx_grp_attr;
	void *mtx_grp;
	void *mtx_attr;
	void *ipi_ep_mtx;
	void *it_mtx;
	void *ipi_count_mtx;
	void *logging_mtx;
#endif				/* _KERN_LOCKS_H_ */
#endif
	uint32_t ipi_count_ep;

	/* assoc/tcb zone info */
	uint32_t ipi_count_asoc;

	/* local addrlist zone info */
	uint32_t ipi_count_laddr;

	/* remote addrlist zone info */
	uint32_t ipi_count_raddr;

	/* chunk structure list for output */
	uint32_t ipi_count_chunk;

	/* socket queue zone info */
	uint32_t ipi_count_readq;

	/* socket queue zone info */
	uint32_t ipi_count_strmoq;


	struct sctpvtaghead vtag_timewait[SCTP_STACK_VTAG_HASH_SIZE];

#ifdef _SCTP_NEEDS_CALLOUT_
	struct calloutlist callqueue;
#endif				/* _SCTP_NEEDS_CALLOUT_ */

	struct sctp_timer addr_wq_timer;
	uint32_t mbuf_track;

	/* for port allocations */
	uint16_t lastport;
	uint16_t lastlow;
	uint16_t lasthi;

};

extern struct sctpstat sctpstat;
/*
 * Here we have all the relevant information for each SCTP entity created. We
 * will need to modify this as approprate. We also need to figure out how to
 * access /dev/random.
 */
struct sctp_pcb {
	unsigned int time_of_secret_change;	/* number of seconds from
						 * timeval.tv_sec */
	uint32_t secret_key[SCTP_HOW_MANY_SECRETS][SCTP_NUMBER_OF_SECRETS];
	unsigned int size_of_a_cookie;

	unsigned int sctp_timeoutticks[SCTP_NUM_TMRS];
	unsigned int sctp_minrto;
	unsigned int sctp_maxrto;
	unsigned int initial_rto;

	int initial_init_rto_max;

	uint32_t sctp_sws_sender;
	uint32_t sctp_sws_receiver;

	/* authentication related fields */
	struct sctp_keyhead shared_keys;
	sctp_auth_chklist_t *local_auth_chunks;
	sctp_hmaclist_t *local_hmacs;
	uint16_t default_keyid;

	/* various thresholds */
	/* Max times I will init at a guy */
	uint16_t max_init_times;

	/* Max times I will send before we consider someone dead */
	uint16_t max_send_times;

	uint16_t def_net_failure;

	/* number of streams to pre-open on a association */
	uint16_t pre_open_stream_count;
	uint16_t max_open_streams_intome;

	/* random number generator */
	uint32_t random_counter;
	uint8_t random_numbers[SCTP_SIGNATURE_ALOC_SIZE];
	uint8_t random_store[SCTP_SIGNATURE_ALOC_SIZE];

	/*
	 * This timer is kept running per endpoint.  When it fires it will
	 * change the secret key.  The default is once a hour
	 */
	struct sctp_timer signature_change;
	int def_cookie_life;
	/* defaults to 0 */
	int auto_close_time;
	uint32_t initial_sequence_debug;
	uint32_t adaptation_layer_indicator;
	char store_at;
	uint8_t max_burst;
	char current_secret_number;
	char last_secret_number;
};

#ifndef SCTP_ALIGNMENT
#define SCTP_ALIGNMENT 32
#endif

#ifndef SCTP_ALIGNM1
#define SCTP_ALIGNM1 (SCTP_ALIGNMENT-1)
#endif

#define sctp_lport ip_inp.inp.inp_lport

struct sctp_inpcb {
	/*
	 * put an inpcb in front of it all, kind of a waste but we need to
	 * for compatability with all the other stuff.
	 */
	union {
		struct inpcb inp;
		char align[(sizeof(struct in6pcb) + SCTP_ALIGNM1) &
		        ~SCTP_ALIGNM1];
	}     ip_inp;

#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	/* leave some space in case i386 inpcb is bigger than ppc */
	uint8_t		padding[128];
#endif

	/* Socket buffer lock protects read_queue and of course sb_cc */
	struct sctp_readhead read_queue;

        LIST_ENTRY(sctp_inpcb) sctp_list;	/* lists all endpoints */
        /* hash of all endpoints for model */
	LIST_ENTRY(sctp_inpcb) sctp_hash;
	/* count of local addresses bound, 0 if bound all */
	int laddr_count;
	/* list of addrs in use by the EP */
	struct sctpladdr sctp_addr_list;
	/* used for source address selection rotation */
	struct sctp_laddr *next_addr_touse;
	struct ifnet *next_ifn_touse;
	/* back pointer to our socket */
	struct socket *sctp_socket;
	uint32_t sctp_flags;	/* INP state flag set */
	uint32_t sctp_features;	/* Feature flags */
	struct sctp_pcb sctp_ep;/* SCTP ep data */
	/* head of the hash of all associations */
	struct sctpasochead *sctp_tcbhash;
	u_long sctp_hashmark;
	/* head of the list of all associations */
	struct sctpasochead sctp_asoc_list;
	struct sctp_iterator *inp_starting_point_for_iterator;
	uint32_t sctp_frag_point;
	uint32_t partial_delivery_point;
	uint32_t sctp_context;
	struct sctp_sndrcvinfo def_send;
	/* These three are here for the sosend_dgram
	 * (pkt, pkt_last and control).
	 * routine. However, I don't think anyone in
	 * the current FreeBSD kernel calls this. So
	 * they are candidates with sctp_sendm for
	 * de-supporting.
	 */
	struct mbuf *pkt, *pkt_last;
	struct mbuf *control;
#if !(defined(__FreeBSD__) || defined(__APPLE__))
#ifndef INP_IPV6
#define INP_IPV6	0x1
#endif
#ifndef INP_IPV4
#define INP_IPV4	0x2
#endif
	u_char inp_vflag;
	u_char inp_ip_ttl;
	u_char inp_ip_tos;
	u_char inp_ip_resv;
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx inp_mtx;
	struct mtx inp_create_mtx;
	int32_t refcount;
#endif
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	int32_t refcount;

	uint32_t lock_caller1;
	uint32_t lock_caller2;
	uint32_t lock_caller3;
	uint32_t unlock_caller1;
	uint32_t unlock_caller2;
	uint32_t unlock_caller3;
	uint32_t getlock_caller1;
	uint32_t getlock_caller2;
	uint32_t getlock_caller3;
	uint32_t gen_count;
	uint32_t lock_gen_count;
	uint32_t unlock_gen_count;
	uint32_t getlock_gen_count;

	uint32_t i_am_here_file;
	uint32_t i_am_here_line;
#endif
};

struct sctp_tcb {
	struct socket *sctp_socket;	/* back pointer to socket */
	struct sctp_inpcb *sctp_ep;	/* back pointer to ep */
	LIST_ENTRY(sctp_tcb) sctp_tcbhash;	/* next link in hash
						 * table */
        LIST_ENTRY(sctp_tcb) sctp_tcblist;	/* list of all of the
						 * TCB's */
        LIST_ENTRY(sctp_tcb) sctp_tcbrestarhash;	/* next link in restart
							 * hash table */
        LIST_ENTRY(sctp_tcb) sctp_asocs;	/* vtag hash list */
	struct sctp_block_entry *block_entry;	/* pointer locked by  socket
						 * send buffer */
	struct sctp_association asoc;
	/*
	 * freed_by_sorcv_sincelast is protected by the sockbuf_lock NOT the
	 * tcb_lock. Its special in this way to help avoid extra mutex calls
	 * in the reading of data.
	 */
	uint32_t freed_by_sorcv_sincelast;
	uint16_t rport;		/* remote port in network format */
	uint16_t resv;
#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
	struct mtx tcb_mtx;
#endif
};


#if defined(__FreeBSD__) && __FreeBSD_version >= 503000

/*
 * General locking concepts: The goal of our locking is to of course provide
 * consistency and yet minimize overhead. We will attempt to use
 * non-recursive locks which are supposed to be quite inexpensive. Now in
 * order to do this the goal is that most functions are not aware of locking.
 * Once we have a TCB we lock it and unlock when we are through. This means
 * that the TCB lock is kind-of a "global" lock when working on an
 * association. Caution must be used when asserting a TCB_LOCK since if we
 * recurse we deadlock.
 *
 * Most other locks (INP and INFO) attempt to localize the locking i.e. we try
 * to contain the lock and unlock within the function that needs to lock it.
 * This sometimes mean we do extra locks and unlocks and lose a bit of
 * efficency, but if the performance statements about non-recursive locks are
 * true this should not be a problem.  One issue that arises with this only
 * lock when needed is that if an implicit association setup is done we have
 * a problem. If at the time I lookup an association I have NULL in the tcb
 * return, by the time I call to create the association some other processor
 * could have created it. This is what the CREATE lock on the endpoint.
 * Places where we will be implicitly creating the association OR just
 * creating an association (the connect call) will assert the CREATE_INP
 * lock. This will assure us that during all the lookup of INP and INFO if
 * another creator is also locking/looking up we can gate the two to
 * synchronize. So the CREATE_INP lock is also another one we must use
 * extreme caution in locking to make sure we don't hit a re-entrancy issue.
 *
 * For non FreeBSD 5.x we provide a bunch of EMPTY lock macros so we can
 * blatantly put locks everywhere and they reduce to nothing on
 * NetBSD/OpenBSD and FreeBSD 4.x
 *
 */
#ifdef __APPLE__
/*
 * Appropriate macros are also provided for Apple Mac OS 10.4.x systems.
 * 10.3.x systems (SCTP_APPLE_PANTHER defined) builds use the emtpy macros.
 */
#endif

/*
 * When working with the global SCTP lists we lock and unlock the INP_INFO
 * lock. So when we go to lookup an association we will want to do a
 * SCTP_INP_INFO_RLOCK() and then when we want to add a new association to
 * the sctppcbinfo list's we will do a SCTP_INP_INFO_WLOCK().
 */

#define SCTP_IPI_COUNT_INIT()

#define SCTP_STATLOG_INIT_LOCK()  \
        mtx_init(&sctppcbinfo.logging_mtx, "sctp-logging", "sctp-log_mtx", MTX_DEF)

#define SCTP_STATLOG_LOCK() \
	do {								\
		mtx_lock(&sctppcbinfo.logging_mtx);				\
	} while (0)

#define SCTP_STATLOG_UNLOCK()		mtx_unlock(&sctppcbinfo.logging_mtx)

#define SCTP_STATLOG_DESTROY() \
	mtx_destroy(&sctppcbinfo.loggingr_mtx)


#define SCTP_INP_INFO_LOCK_INIT() \
        mtx_init(&sctppcbinfo.ipi_ep_mtx, "sctp-info", "inp_info", MTX_DEF)


#define SCTP_INP_INFO_RLOCK()	do { 					\
             mtx_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)


#define SCTP_INP_INFO_WLOCK()	do { 					\
             mtx_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)



#define SCTP_IPI_ADDR_INIT() \
        mtx_init(&sctppcbinfo.ipi_addr_mtx, "sctp-addr-wq", "sctp_addr_wq", MTX_DEF)

#define SCTP_IPI_ADDR_DESTROY(_inp) \
	mtx_destroy(&sctppcbinfo.ipi_addr_mtx)

#define SCTP_IPI_ADDR_LOCK()	do { 					\
             mtx_lock(&sctppcbinfo.ipi_addr_mtx);                         \
} while (0)

#define SCTP_IPI_ADDR_UNLOCK()		mtx_unlock(&sctppcbinfo.ipi_addr_mtx)

#define SCTP_INP_INFO_RUNLOCK()		mtx_unlock(&sctppcbinfo.ipi_ep_mtx)
#define SCTP_INP_INFO_WUNLOCK()		mtx_unlock(&sctppcbinfo.ipi_ep_mtx)

/*
 * The INP locks we will use for locking an SCTP endpoint, so for example if
 * we want to change something at the endpoint level for example random_store
 * or cookie secrets we lock the INP level.
 */
#define SCTP_INP_LOCK_INIT(_inp) \
	mtx_init(&(_inp)->inp_mtx, "sctp-inp", "inp", MTX_DEF | MTX_DUPOK)
#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	mtx_init(&(_inp)->inp_create_mtx, "sctp-create", "inp_create", \
		 MTX_DEF | MTX_DUPOK)

#define SCTP_INP_LOCK_DESTROY(_inp) \
	mtx_destroy(&(_inp)->inp_mtx)

#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp) \
	mtx_destroy(&(_inp)->inp_create_mtx)


#ifdef SCTP_LOCK_LOGGING
#define SCTP_INP_RLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#else

#define SCTP_INP_RLOCK(_inp)	do { 					\
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
        mtx_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#endif

#define SCTP_INP_INCR_REF(_inp) atomic_add_int(&((_inp)->refcount), 1)
#define SCTP_INP_DECR_REF(_inp) atomic_add_int(&((_inp)->refcount), -1)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_ASOC_CREATE_LOCK(_inp) \
	do {								\
                sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_CREATE); \
		mtx_lock(&(_inp)->inp_create_mtx);			\
	} while (0)
#else

#define SCTP_ASOC_CREATE_LOCK(_inp) \
	do {								\
		mtx_lock(&(_inp)->inp_create_mtx);			\
	} while (0)
#endif

#define SCTP_INP_RUNLOCK(_inp)		mtx_unlock(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp)		mtx_unlock(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)	mtx_unlock(&(_inp)->inp_create_mtx)

/*
 * For the majority of things (once we have found the association) we will
 * lock the actual association mutex. This will protect all the assoiciation
 * level queues and streams and such. We will need to lock the socket layer
 * when we stuff data up into the receiving sb_mb. I.e. we will need to do an
 * extra SOCKBUF_LOCK(&so->so_rcv) even though the association is locked.
 */

#define SCTP_TCB_LOCK_INIT(_tcb) \
	mtx_init(&(_tcb)->tcb_mtx, "sctp-tcb", "tcb", MTX_DEF | MTX_DUPOK)

#define SCTP_TCB_LOCK_DESTROY(_tcb)	mtx_destroy(&(_tcb)->tcb_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_TCB_LOCK(_tcb)  do {					\
        sctp_log_lock(_tcb->sctp_ep, _tcb, SCTP_LOG_LOCK_TCB);          \
	mtx_lock(&(_tcb)->tcb_mtx);                                     \
} while (0)

#else
#define SCTP_TCB_LOCK(_tcb)  do {					\
	mtx_lock(&(_tcb)->tcb_mtx);                                     \
} while (0)

#endif


#define SCTP_TCB_TRYLOCK(_tcb) 	mtx_trylock(&(_tcb)->tcb_mtx)

#define SCTP_TCB_UNLOCK(_tcb)		mtx_unlock(&(_tcb)->tcb_mtx)

#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)	      do { \
                                                if (mtx_owned(&(_tcb)->tcb_mtx)) \
                                                     mtx_unlock(&(_tcb)->tcb_mtx); \
                                              } while (0)



#ifdef INVARIANTS
#define SCTP_TCB_LOCK_ASSERT(_tcb) do { \
                            if (mtx_owned(&(_tcb)->tcb_mtx) == 0) \
                                panic("Don't own TCB lock"); \
                            } while (0)
#else
#define SCTP_TCB_LOCK_ASSERT(_tcb)
#endif

#define SCTP_ITERATOR_LOCK_INIT() \
        mtx_init(&sctppcbinfo.it_mtx, "sctp-it", "iterator", MTX_DEF)

#ifdef INVARIANTS
#define SCTP_ITERATOR_LOCK() \
	do {								\
		if (mtx_owned(&sctppcbinfo.it_mtx))			\
			panic("Iterator Lock");				\
		mtx_lock(&sctppcbinfo.it_mtx);				\
	} while (0)
#else
#define SCTP_ITERATOR_LOCK() \
	do {								\
		mtx_lock(&sctppcbinfo.it_mtx);				\
	} while (0)

#endif

#define SCTP_ITERATOR_UNLOCK()	        mtx_unlock(&sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY()	mtx_destroy(&sctppcbinfo.it_mtx)


#elif defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
/*
 * Apple MacOS X 10.4 "Tiger"
 */

/* for now, all locks use this group and attributes */
#define SCTP_MTX_GRP sctppcbinfo.mtx_grp
#define SCTP_MTX_ATTR sctppcbinfo.mtx_attr

/* Lock for INFO stuff */
#define SCTP_INP_INFO_LOCK_INIT() \
	sctppcbinfo.ipi_ep_mtx = lck_rw_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_RUNLOCK() 
#define SCTP_INP_INFO_WLOCK() 
#define SCTP_INP_INFO_WUNLOCK()
#define SCTP_INP_INFO_LOCK_DESTROY() \
        lck_rw_free(sctppcbinfo.ipi_ep_mtx, SCTP_MTX_GRP)
#define SCTP_IPI_COUNT_INIT() \
	sctppcbinfo.ipi_count_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_IPI_COUNT_DESTROY() \
        lck_mtx_free(sctppcbinfo.ipi_count_mtx, SCTP_MTX_GRP)

#define SCTP_IPI_ADDR_INIT()
#define SCTP_IPI_ADDR_DESTROY(_inp)
#define SCTP_IPI_ADDR_LOCK()
#define SCTP_IPI_ADDR_UNLOCK()


#define SCTP_STATLOG_INIT_LOCK() \
	sctppcbinfo.logging_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_STATLOG_LOCK() \
	lck_mtx_lock(sctppcbinfo.logging_mtx)
#define SCTP_STATLOG_UNLOCK() \
	lck_mtx_unlock(sctppcbinfo.logging_mtx)
#define SCTP_STATLOG_DESTROY() \
	lck_mtx_free(sctppcbinfo.logging_mtx, SCTP_MTX_GRP)


/* Lock for INP */
#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)

#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)
#define SCTP_INP_WUNLOCK(_inp)
#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)

/* Lock for TCB */
#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_TRYLOCK(_tcb)
#define SCTP_TCB_UNLOCK(_tcb)
#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)
#define SCTP_TCB_LOCK_ASSERT(_tcb)

/* iterator locks */
#define SCTP_ITERATOR_LOCK_INIT() \
	sctppcbinfo.it_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_ITERATOR_LOCK() \
	lck_mtx_lock(sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_UNLOCK() \
	lck_mtx_unlock(sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY() \
	lck_mtx_free(sctppcbinfo.it_mtx, SCTP_MTX_GRP)

/* socket locks */
#define SOCK_LOCK(_so)
#define SOCK_UNLOCK(_so)
#define SOCKBUF_LOCK(_so_buf)
#define SOCKBUF_UNLOCK(_so_buf)
#define SOCKBUF_LOCK_ASSERT(_so_buf)

#else

/*
 * Empty Lock declarations for all other platforms. Pre-process away to
 * nothing.
 */

/* Lock for INFO stuff */
#define SCTP_INP_INFO_LOCK_INIT()
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_WLOCK()
#define SCTP_IPI_COUNT_INIT()

#define SCTP_IPI_ADDR_INIT()
#define SCTP_IPI_ADDR_DESTROY(_inp)
#define SCTP_IPI_ADDR_LOCK()
#define SCTP_IPI_ADDR_UNLOCK()

#define SCTP_INP_INFO_RUNLOCK()
#define SCTP_INP_INFO_WUNLOCK()
/* Lock for INP */
#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)
#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)
#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)
#define SCTP_INP_WUNLOCK(_inp)
#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)
/* Lock for TCB */

#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_TRYLOCK(_tcb)
#define SCTP_TCB_UNLOCK(_tcb)
#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)
#define SCTP_TCB_LOCK_ASSERT(_tcb)


/* socket locks that are not here in other than 5.3 > FreeBSD */
#define SOCK_LOCK(_so)
#define SOCK_UNLOCK(_so)
#define SOCKBUF_LOCK(_so_buf)
#define SOCKBUF_UNLOCK(_so_buf)
#define SOCKBUF_LOCK_ASSERT(_so_buf)
/* iterator locks */
#define SCTP_ITERATOR_LOCK_INIT()
#define SCTP_ITERATOR_LOCK()
#define SCTP_ITERATOR_UNLOCK()
#define SCTP_ITERATOR_LOCK_DESTROY()
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 503000
#define SCTP_INCRS_DEFINED 1

#define SCTP_INCR_EP_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_ep, 1); \
	        } while (0)

#define SCTP_DECR_EP_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_ep,-1); \
	        } while (0)

#define SCTP_INCR_ASOC_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_asoc, 1); \
	        } while (0)

#define SCTP_DECR_ASOC_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_asoc, -1); \
	        } while (0)

#define SCTP_INCR_LADDR_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_laddr, 1); \
	        } while (0)

#define SCTP_DECR_LADDR_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_laddr, -1); \
	        } while (0)

#define SCTP_INCR_RADDR_COUNT() \
                do { \
 	               atomic_add_int(&sctppcbinfo.ipi_count_raddr,1); \
	        } while (0)

#define SCTP_DECR_RADDR_COUNT() \
                do { \
 	               atomic_add_int(&sctppcbinfo.ipi_count_raddr,-1); \
	        } while (0)

#define SCTP_INCR_CHK_COUNT() \
                do { \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk, 1); \
	        } while (0)

#define SCTP_DECR_CHK_COUNT() \
                do { \
                       if(sctppcbinfo.ipi_count_chunk == 0) \
                             panic("chunk count to 0?"); \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk,-1); \
	        } while (0)

#define SCTP_INCR_READQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_readq,1); \
	        } while (0)

#define SCTP_DECR_READQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_readq, -1); \
	        } while (0)
#endif

#define SCTP_INCR_STRMOQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_strmoq, 1); \
	        } while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_strmoq,-1); \
	        } while (0)



#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
/***************BEGIN APPLE Tiger count stuff**********************/
#define I_AM_HERE \
                do { \
			printf("%s:%d at %s\n", __FILE__, __LINE__ , __FUNCTION__); \
		} while (0)

#define SAVE_I_AM_HERE(_inp) \
                do { \
			(_inp)->i_am_here_file = APPLE_FILE_NO; \
			(_inp)->i_am_here_line = __LINE__; \
		} while (0)

/* save caller pc and caller's caller pc */
#if defined (__i386__)
#define SAVE_CALLERS(a, b, c) { \
        unsigned int ebp = 0; \
        unsigned int prev_ebp = 0; \
        asm("movl %%ebp, %0;" : "=r"(ebp)); \
        a = *(unsigned int *)(*(unsigned int *)ebp + 4) - 4; \
        prev_ebp = *(unsigned int *)(*(unsigned int *)ebp); \
        b = *(unsigned int *)((char *)prev_ebp + 4) - 4; \
        prev_ebp = *(unsigned int *)prev_ebp; \
        c = *(unsigned int *)((char *)prev_ebp + 4) - 4; \
}
#else
#define SAVE_CALLERS(caller1, caller2, caller3)
#endif

#define SCTP_INCRS_DEFINED 1
#define SCTP_INCR_EP_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_ep++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_EP_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_ep--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_ASOC_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
	               sctppcbinfo.ipi_count_asoc++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_ASOC_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
	               sctppcbinfo.ipi_count_asoc--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_LADDR_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
	               sctppcbinfo.ipi_count_laddr++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_LADDR_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
	               sctppcbinfo.ipi_count_laddr--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_RADDR_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
 	               sctppcbinfo.ipi_count_raddr++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_RADDR_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
 	               sctppcbinfo.ipi_count_raddr--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_CHK_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
  	               sctppcbinfo.ipi_count_chunk++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_CHK_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
  	               sctppcbinfo.ipi_count_chunk--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_READQ_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_readq++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_READQ_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_readq--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_INCR_STRMOQ_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_strmoq++; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
                do { \
                       lck_mtx_lock(sctppcbinfo.ipi_count_mtx); \
		       sctppcbinfo.ipi_count_strmoq--; \
                       lck_mtx_unlock(sctppcbinfo.ipi_count_mtx); \
	        } while (0)


/***************BEGIN APPLE PANTHER count stuff**********************/
#endif



#ifndef SCTP_INCRS_DEFINED
/***************BEGIN all othher count stuff**********************/
#define SCTP_INCR_EP_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_ep++; \
	        } while (0)

#define SCTP_DECR_EP_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_ep--; \
	        } while (0)

#define SCTP_INCR_ASOC_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_asoc++; \
	        } while (0)

#define SCTP_DECR_ASOC_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_asoc--; \
	        } while (0)

#define SCTP_INCR_LADDR_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_laddr++; \
	        } while (0)

#define SCTP_DECR_LADDR_COUNT() \
                do { \
	               sctppcbinfo.ipi_count_laddr--; \
	        } while (0)

#define SCTP_INCR_RADDR_COUNT() \
                do { \
 	               sctppcbinfo.ipi_count_raddr++; \
	        } while (0)

#define SCTP_DECR_RADDR_COUNT() \
                do { \
 	               sctppcbinfo.ipi_count_raddr--; \
	        } while (0)

#define SCTP_INCR_CHK_COUNT() \
                do { \
  	               sctppcbinfo.ipi_count_chunk++; \
	        } while (0)

#define SCTP_DECR_CHK_COUNT() \
                do { \
  	               sctppcbinfo.ipi_count_chunk--; \
	        } while (0)

#define SCTP_INCR_READQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_readq++; \
	        } while (0)

#define SCTP_DECR_READQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_readq--; \
	        } while (0)

#define SCTP_INCR_STRMOQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_strmoq++; \
	        } while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
                do { \
		       sctppcbinfo.ipi_count_strmoq--; \
	        } while (0)


#endif

#if (defined(__APPLE__) && defined(KERNEL))
#ifndef _KERNEL
#define _KERNEL
#endif
#endif

#if defined(_KERNEL)

extern struct sctp_epinfo sctppcbinfo;
extern int sctp_auto_asconf;

int SCTP6_ARE_ADDR_EQUAL(struct in6_addr *a, struct in6_addr *b);

void sctp_fill_pcbinfo(struct sctp_pcbinfo *);

struct sctp_nets *sctp_findnet(struct sctp_tcb *, struct sockaddr *);

struct sctp_inpcb *sctp_pcb_findep(struct sockaddr *, int, int);

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
int sctp_inpcb_bind(struct socket *, struct sockaddr *, struct thread *);

#else
int sctp_inpcb_bind(struct socket *, struct sockaddr *, struct proc *);

#endif

struct sctp_tcb *
sctp_findassociation_addr(struct mbuf *, int, int,
    struct sctphdr *, struct sctp_chunkhdr *, struct sctp_inpcb **,
    struct sctp_nets **);

struct sctp_tcb *
sctp_findassociation_addr_sa(struct sockaddr *,
    struct sockaddr *, struct sctp_inpcb **, struct sctp_nets **, int);

void
sctp_move_pcb_and_assoc(struct sctp_inpcb *, struct sctp_inpcb *,
    struct sctp_tcb *);

/*
 * For this call ep_addr, the to is the destination endpoint address of the
 * peer (relative to outbound). The from field is only used if the TCP model
 * is enabled and helps distingush amongst the subset bound (non-boundall).
 * The TCP model MAY change the actual ep field, this is why it is passed.
 */
struct sctp_tcb *
sctp_findassociation_ep_addr(struct sctp_inpcb **,
    struct sockaddr *, struct sctp_nets **, struct sockaddr *,
    struct sctp_tcb *);

struct sctp_tcb *
sctp_findassociation_ep_asocid(struct sctp_inpcb *,
    sctp_assoc_t);

struct sctp_tcb *
sctp_findassociation_ep_asconf(struct mbuf *, int, int,
    struct sctphdr *, struct sctp_inpcb **, struct sctp_nets **);

int sctp_inpcb_alloc(struct socket *);

int sctp_is_address_on_local_host(struct sockaddr *addr);

void sctp_inpcb_free(struct sctp_inpcb *, int);

struct sctp_tcb *
sctp_aloc_assoc(struct sctp_inpcb *, struct sockaddr *,
    int, int *, uint32_t);

int sctp_free_assoc(struct sctp_inpcb *, struct sctp_tcb *, int);

int sctp_add_local_addr_ep(struct sctp_inpcb *, struct ifaddr *);

int sctp_insert_laddr(struct sctpladdr *, struct ifaddr *);

void sctp_remove_laddr(struct sctp_laddr *);

int sctp_del_local_addr_ep(struct sctp_inpcb *, struct ifaddr *);

int sctp_del_local_addr_ep_sa(struct sctp_inpcb *, struct sockaddr *);

int sctp_add_remote_addr(struct sctp_tcb *, struct sockaddr *, int, int);

void sctp_remove_net(struct sctp_tcb *, struct sctp_nets *);

int sctp_del_remote_addr(struct sctp_tcb *, struct sockaddr *);

void sctp_pcb_init(void);

void sctp_free_remote_addr(struct sctp_nets *);

int sctp_add_local_addr_assoc(struct sctp_tcb *, struct ifaddr *);

int sctp_del_local_addr_assoc(struct sctp_tcb *, struct ifaddr *);

int sctp_del_local_addr_assoc_sa(struct sctp_tcb *, struct sockaddr *);

int
sctp_load_addresses_from_init(struct sctp_tcb *, struct mbuf *, int, int,
    int, struct sctphdr *, struct sockaddr *);

int
sctp_set_primary_addr(struct sctp_tcb *, struct sockaddr *,
    struct sctp_nets *);

int sctp_is_vtag_good(struct sctp_inpcb *, uint32_t, struct timeval *);

/* void sctp_drain(void); */

int sctp_destination_is_reachable(struct sctp_tcb *, struct sockaddr *);

/*
 * Null in last arg inpcb indicate run on ALL ep's. Specific inp in last arg
 * indicates run on ONLY assoc's of the specified endpoint.
 */
int
sctp_initiate_iterator(inp_func inpf, asoc_func af, uint32_t, uint32_t,
    uint32_t, void *, uint32_t, end_func ef, struct sctp_inpcb *, uint8_t co_off);

#if defined(__APPLE__)
void sctp_callout_alloc(struct sctp_timer *);
void sctp_callout_free(struct callout *);

void sctp_start_main_timer(void);
void sctp_stop_main_timer(void);
#endif

#ifdef __NetBSD__
extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);

#endif

#endif				/* _KERNEL */
#endif				/* !__sctp_pcb_h__ */
