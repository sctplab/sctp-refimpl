/*-
 * Copyright (c) 2006-2007, Cisco Systems, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its 
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
#ifndef __sctp_os_macosx_h__
#define __sctp_os_macosx_h__

/*
 * includes
 */
#include <sctp.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h> 
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/resourcevar.h>
#include <sys/uio.h>
#if defined(__APPLE__) && !defined(SCTP_APPLE_PANTHER)
#include <sys/proc_internal.h>
#include <sys/uio_internal.h>
#endif
#include <sys/random.h>
/*#include <sys/queue.h>*/
#include <sys/appleapiopts.h>

#include <machine/limits.h>

#include <IOKit/IOLib.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>

#ifdef INET6
#include <sys/domain.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#endif /* INET6 */

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif /* IPSEC */

#include <stdarg.h>

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>
extern struct fileops socketops;
#endif /* HAVE_SCTP_PEELOFF_SOCKOPT */


#if defined(HAVE_NRL_INPCB)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#endif

#if defined(KERNEL) && !defined(_KERNEL)
#define _KERNEL
#endif

#define SCTP_LIST_EMPTY(list)	LIST_EMPTY(list)

/*
 * debug macro
 */
#if defined(SCTP_DEBUG)
#define SCTPDBG(level, params...)					\
{									\
    do {								\
	if (sctp_debug_on & level ) {					\
	    printf(params);						\
	}								\
    } while (0);							\
}
#define SCTPDBG_ADDR(level, addr)					\
{									\
    do {								\
	if (sctp_debug_on & level ) {					\
	    sctp_print_address(addr);					\
	}								\
    } while (0);							\
}
#define SCTP_PRINTF(params...)	printf(params)
#else
#define SCTPDBG(level, params...)
#define SCTPDBG_ADDR(level, addr)
#define SCTP_PRINTF(params...)
#endif

/*
 * Local address and interface list handling
 */
#define SCTP_MAX_VRF_ID		0
#define SCTP_SIZE_OF_VRF_HASH	1
#define SCTP_IFNAMSIZ		IFNAMSIZ
#define SCTP_DEFAULT_VRFID	0
#define SCTP_DEFAULT_TABLEID	0
#define SCTP_VRF_ADDR_HASH_SIZE	16
#define SCTP_VRF_IFN_HASH_SIZE	3
#define SCTP_VRF_DEFAULT_TABLEID(vrf_id)	0

#define SCTP_IFN_IS_IFT_LOOP(ifn) ((ifn)->ifn_type == IFT_LOOP)

/*
 * Access to IFN's to help with src-addr-selection
 */
/* This could return VOID if the index works but for BSD we provide both. */
#define SCTP_GET_IFN_VOID_FROM_ROUTE(ro) (void *)ro->ro_rt->rt_ifp
#define SCTP_GET_IF_INDEX_FROM_ROUTE(ro) (ro)->ro_rt->rt_ifp->if_index
#define SCTP_ROUTE_HAS_VALID_IFN(ro) ((ro)->ro_rt && (ro)->ro_rt->rt_ifp)

/* 
 * for per socket level locking strategy:
 * SCTP_INP_SO(sctpinp): returns socket on base inp structure from sctp_inpcb
 * SCTP_SOCKET_LOCK(so, refcnt): locks socket so with refcnt
 * SCTP_SOCKET_UNLOCK(so, refcnt): unlocks socket so with refcnt
 * SCTP_MTX_LOCK(lck): lock mutex
 * SCTP_MTX_UNLOCK(lck): unlock mutex
 * SCTP_MTX_TRYLOCK(lck): try lock mutex
 * SCTP_LOCK_EX(lck): lock exclusive
 * SCTP_UNLOCK_EX(lck): unlock exclusive
 * SCTP_TRYLOCK_EX(lck): trylock exclusive
 * SCTP_LOCK_SHARED(lck): lock shared
 * SCTP_UNLOCK_SHARED(lck): unlock shared
 * SCTP_TRYLOCK_SHARED(lck): trylock shared
 */
#define SCTP_PER_SOCKET_LOCKING
#define SCTP_INP_SO(sctpinp)	(sctpinp)->ip_inp.inp.inp_socket
#define SCTP_SOCKET_LOCK(so, refcnt)	socket_lock(so, refcnt)
#define SCTP_SOCKET_UNLOCK(so, refcnt)	socket_unlock(so, refcnt)
#define SCTP_MTX_LOCK(mtx)	lck_mtx_lock(mtx)
#define SCTP_MTX_UNLOCK(mtx)	lck_mtx_unlock(mtx)
#define SCTP_MTX_TRYLOCK(mtx)	lck_mtx_try_lock(mtx)
#define SCTP_LOCK_EXC(lck)	lck_rw_lock_exclusive(lck)
#define SCTP_UNLOCK_EXC(lck)	lck_rw_unlock_exclusive(lck)
#define SCTP_TRYLOCK_EXC(lck)	lck_rw_try_lock_exclusive(lck)
#define SCTP_LOCK_SHARED(lck)	lck_rw_lock_shared(lck)
#define SCTP_UNLOCK_SHARED(lck)	lck_rw_unlock_shared(lck)
#define SCTP_TRYLOCK_SHARED(lck) lck_rw_try_lock_shared(lck)

/*
 * general memory allocation
 */
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
#define SCTP_MALLOC(var, type, size, name) \
    do { \
	MALLOC(var, type, size, M_PCB, M_WAITOK); \
    } while (0)
#else
#define SCTP_MALLOC(var, type, size, name) \
    do { \
	MALLOC(var, type, size, M_PCB, M_NOWAIT); \
    } while (0)
#endif

#define SCTP_FREE(var)	FREE(var, M_PCB)

#define SCTP_MALLOC_SONAME(var, type, size) \
    do { \
	MALLOC(var, type, size, M_SONAME, M_WAITOK | M_ZERO); \
    } while (0)
#define SCTP_FREE_SONAME(var)	FREE(var, M_SONAME)

/*
 * zone allocation functions
 */
typedef struct vm_zone *sctp_zone_t;
extern zone_t kalloc_zone(vm_size_t);	/* XXX */

/* SCTP_ZONE_INIT: initialize the zone */
#define SCTP_ZONE_INIT(zone, name, size, number) \
	zone = (void *)kalloc_zone(size);

/* SCTP_ZONE_GET: allocate element from the zone */
#define SCTP_ZONE_GET(zone, type) \
	(type *)zalloc(zone);

/* SCTP_ZONE_FREE: free element from the zone */
#define SCTP_ZONE_FREE(zone, element) \
	zfree(zone, element);

#define SCTP_HASH_INIT(size, hashmark) hashinit(size, M_PCB, hashmark)
#define SCTP_HASH_FREE(table, hashmark) SCTP_FREE(table)

struct mbuf *sctp_m_copym(struct mbuf *m, int off, int len, int wait);

#define SCTP_M_COPYM sctp_m_copym

/*
 * timers
 */
#include <netinet/sctp_callout.h>
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
#ifdef _KERN_LOCKS_H_
extern lck_rw_t *sctp_calloutq_mtx;
#else
extern void *sctp_calloutq_mtx;
#endif
#define SCTP_TIMERQ_LOCK()	lck_rw_lock_exclusive(sctp_calloutq_mtx)
#define SCTP_TIMERQ_UNLOCK()	lck_rw_unlock_exclusive(sctp_calloutq_mtx)
#define SCTP_TIMERQ_LOCK_INIT()	sctp_calloutq_mtx = lck_rw_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_TIMERQ_LOCK_DESTROY() lck_rw_free(sctp_calloutq_mtx, SCTP_MTX_GRP)
#endif

/* Mbuf manipulation and access macros  */
#define SCTP_BUF_LEN(m) (m->m_len)
#define SCTP_BUF_NEXT(m) (m->m_next)
#define SCTP_BUF_NEXT_PKT(m) (m->m_nextpkt)
#define SCTP_BUF_RESV_UF(m, size) m->m_data += size
#define SCTP_BUF_AT(m, size) m->m_data + size
#define SCTP_BUF_IS_EXTENDED(m) (m->m_flags & M_EXT)
#define SCTP_BUF_EXTEND_SIZE(m) (m->m_ext.ext_size)
#define SCTP_BUF_TYPE(m) (m->m_type)
#define SCTP_BUF_RECVIF(m) (m->m_pkthdr.rcvif)
#define SCTP_BUF_PREPEND(m, plen, how) ((m) = sctp_m_prepend_2((m), (plen), (how)))
struct mbuf *sctp_m_prepend_2(struct mbuf *m, int len, int how);


/*************************/
/*      MTU              */
/*************************/
#define SCTP_GATHER_MTU_FROM_IFN_INFO(ifn, ifn_index) ((struct ifnet *)ifn)->if_mtu
#define SCTP_GATHER_MTU_FROM_ROUTE(sctp_ifa, sa, rt) ((rt != NULL) ? rt->rt_rmx.rmx_mtu : 0)
#define SCTP_GATHER_MTU_FROM_INTFC(sctp_ifn) ((sctp_ifn->ifn_p != NULL) ? ((struct ifnet *)(sctp_ifn->ifn_p))->if_mtu : 0)
#define SCTP_SET_MTU_OF_ROUTE(sa, rt, mtu) \
	do { \
		if (rt != NULL) \
			rt->rt_rmx.rmx_mtu = mtu; \
	} while (0) 
/* (de-)register interface event notifications */
#define SCTP_REGISTER_INTERFACE(ifhandle, ifname)
#define SCTP_DEREGISTER_INTERFACE(ifhandle, ifname)

/*************************/
/* These are for logging */
/*************************/
/* return the base ext data pointer */
#define SCTP_BUF_EXTEND_BASE(m) (m->m_ext.ext_buf)
 /* return the refcnt of the data pointer */
#define SCTP_BUF_EXTEND_REFCNT(m) (*m->m_ext.ref_cnt)
/* return any buffer related flags, this is
 * used beyond logging for apple only.
 */
#define SCTP_BUF_GET_FLAGS(m) (m->m_flags)

/*
 * For APPLE this just accesses the M_PKTHDR length so it operates on an
 * mbuf with hdr flag. Other O/S's may have seperate packet header and mbuf
 * chain pointers.. thus the macro.
 */
#define SCTP_HEADER_TO_CHAIN(m) (m)
#define SCTP_DETACH_HEADER_FROM_CHAIN(m)
#define SCTP_HEADER_LEN(m) (m->m_pkthdr.len)
#define SCTP_GET_HEADER_FOR_OUTPUT(o_pak) 0
#define SCTP_RELEASE_HEADER(m)
#define SCTP_RELEASE_PKT(m)	sctp_m_freem(m)

static inline int SCTP_GET_PKT_VRFID(void *m, uint32_t vrf_id) {
	vrf_id = SCTP_DEFAULT_VRFID;
	return (0);
}
static inline int SCTP_GET_PKT_TABLEID(void *m, uint32_t table_id) {
	table_id = SCTP_DEFAULT_TABLEID;
	return (0);
}


/* Attach the chain of data into the sendable packet. */
#define SCTP_ATTACH_CHAIN(pak, m, packet_length) do { \
                                                 pak = m; \
                                                 pak->m_pkthdr.len = packet_length; \
                         } while(0)

/* Other m_pkthdr type things */
#define SCTP_IS_IT_BROADCAST(dst, m) in_broadcast(dst, m->m_pkthdr.rcvif)
#define SCTP_IS_IT_LOOPBACK(m) ((m->m_pkthdr.rcvif == NULL) || (m->m_pkthdr.rcvif->if_type == IFT_LOOP))

#define SCTP_ALIGN_TO_END(m, len) if(m->m_flags & M_PKTHDR) { \
                                     MH_ALIGN(m, len); \
                                  } else if ((m->m_flags & M_EXT) == 0) { \
                                     M_ALIGN(m, len); \
                                  }


/*
 * This converts any input packet header into the chain of data holders,
 * for APPLE its a NOP.
 */

/* Macro's for getting length from V6/V4 header */
#define SCTP_GET_IPV4_LENGTH(iph) (iph->ip_len)
#define SCTP_GET_IPV6_LENGTH(ip6) (ip6->ip6_plen)

/* get the v6 hop limit */
#define SCTP_GET_HLIM(inp, ro)	in6_selecthlim((struct in6pcb *)&inp->ip_inp.inp, (ro ? (ro->ro_rt ? (ro->ro_rt->rt_ifp) : (NULL)) : (NULL)));

/* is the endpoint v6only? */
#define SCTP_IPV6_V6ONLY(inp)	(((struct inpcb *)inp)->inp_flags & IN6P_IPV6_V6ONLY)
/* is the socket non-blocking? */
#define SCTP_SO_IS_NBIO(so)	((so)->so_state & SS_NBIO)
#define SCTP_SET_SO_NBIO(so)	((so)->so_state |= SS_NBIO)
#define SCTP_CLEAR_SO_NBIO(so)	((so)->so_state &= ~SS_NBIO)
/* get the socket type */
#define SCTP_SO_TYPE(so)	((so)->so_type)
/* reserve sb space for a socket */
#define SCTP_SORESERVE(so, send, recv)	soreserve(so, send, recv)
/* wakeup a socket */
#define SCTP_SOWAKEUP(so)	wakeup(&(so)->so_timeo)
/* clear the socket buffer state */
#define SCTP_SB_CLEAR(sb)	\
	(sb).sb_cc = 0;		\
	(sb).sb_mb = NULL;	\
	(sb).sb_mbcnt = 0;

#define SCTP_SB_LIMIT_RCV(so) so->so_rcv.sb_hiwat
#define SCTP_SB_LIMIT_SND(so) so->so_snd.sb_hiwat

/*
 * routes, output, etc.
 */
typedef struct route	sctp_route_t;
typedef struct rtentry	sctp_rtentry_t;
#define SCTP_RTALLOC(ro, vrf_id, table_id) rtalloc_ign((struct route *)ro, 0UL)

/* Future zero copy wakeup/send  function */
#define SCTP_ZERO_COPY_EVENT(inp, so)

/*
 * IP output routines
 */
#define SCTP_IP_ID(inp) (ip_id)

#define SCTP_IP_OUTPUT(result, o_pak, ro, stcb, vrf_id, table_id) \
{ \
	int o_flgs = 0; \
	if (stcb && stcb->sctp_ep && stcb->sctp_ep->sctp_socket) { \
		o_flgs = IP_RAWOUTPUT | (stcb->sctp_ep->sctp_socket->so_options & SO_DONTROUTE); \
	} else { \
		o_flgs = IP_RAWOUTPUT; \
	} \
	result = ip_output(o_pak, NULL, ro, o_flgs, NULL); \
}

#define SCTP_IP6_OUTPUT(result, o_pak, ro, ifp, stcb, vrf_id, table_id) \
{ \
 	if (stcb && stcb->sctp_ep) \
		result = ip6_output(o_pak, \
				    ((struct in6pcb *)(stcb->sctp_ep))->in6p_outputopts, \
				    (ro), 0, 0, ifp, NULL); \
	else \
		result = ip6_output(o_pak, NULL, (ro), 0, 0, ifp, NULL); \
}

struct mbuf *
sctp_get_mbuf_for_msg(unsigned int space_needed, 
		      int want_header, int how, int allonebuf, int type);


/*
 * SCTP AUTH
 */
#define SCTP_READ_RANDOM(buf, len)	read_random(buf, len)

#ifdef USE_SCTP_SHA1
#include <netinet/sctp_sha1.h>
#else
#include <crypto/sha1.h>
/* map standard crypto API names */
#define SHA1_Init	SHA1Init
#define SHA1_Update	SHA1Update
#define SHA1_Final(x,y)	SHA1Final((caddr_t)x, y)
#endif

#if defined(HAVE_SHA2)
#include <crypto/sha2/sha2.h>
#endif

#include <sys/md5.h>
/* map standard crypto API names */
#define MD5_Init	MD5Init
#define MD5_Update	MD5Update
#define MD5_Final	MD5Final


/*
 * Other MacOS specific
 */

/* Apple KPI defines for atomic operations */
#include <libkern/OSAtomic.h>
#define atomic_add_int(addr, val)	OSAddAtomic(val, (SInt32 *)addr)
#define atomic_fetchadd_int(addr, val)	OSAddAtomic(val, (SInt32 *)addr)
#define atomic_subtract_int(addr, val)	OSAddAtomic((-val), (SInt32 *)addr)
#define atomic_add_16(addr, val)	OSAddAtomic16(val, (SInt16 *)addr)
#define atomic_cmpset_int(dst, exp, src) OSCompareAndSwap(exp, src, (UInt32 *)dst)

/* additional protosw entries for Mac OS X 10.4 */
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
int sctp_lock(struct socket *so, int refcount, int lr);
int sctp_unlock(struct socket *so, int refcount, int lr);

#ifdef _KERN_LOCKS_H_
lck_mtx_t *sctp_getlock(struct socket *so, int locktype);
#else
void *sctp_getlock(struct socket *so, int locktype);
#endif /* _KERN_LOCKS_H_ */
void sctp_lock_assert(struct socket *so);
void sctp_unlock_assert(struct socket *so);
#endif /* SCTP_APPLE_FINE_GRAINED_LOCKING */

/* emulate the BSD 'ticks' clock */
extern int ticks;

/* XXX: Hopefully temporary until APPLE changes to newer defs like other BSDs */
#define if_addrlist	if_addrhead
#define if_list		if_link
#define ifa_list	ifa_link

/* MacOS specific timer functions */
extern unsigned int sctp_main_timer;
extern int sctp_main_timer_ticks;

void sctp_start_main_timer(void);
void sctp_stop_main_timer(void);

/* address monitor thread */
void sctp_address_monitor_start(void);
void sctp_address_monitor_destroy(void);
#define SCTP_PROCESS_STRUCT thread_t

#endif
