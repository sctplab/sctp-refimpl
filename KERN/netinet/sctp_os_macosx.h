/*-
 * Copyright (c) 2006, Cisco Systems, Inc. All rights reserved.
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/random.h>

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
#define SCTP_ZONE_GET(zone) \
	zalloc(zone);

/* SCTP_ZONE_FREE: free element from the zone */
#define SCTP_ZONE_FREE(zone, element) \
	zfree(zone, element);

/*
 * timers
 */
#include <netinet/sctp_callout.h>
typedef struct callout sctp_os_timer_t;

/*
 * Functions
 */
#define SCTP_READ_RANDOM(buf, len)	read_random(buf, len)

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
#define SCTP_BUF_GET_FLAGS(m) (m->m_flags);

/*
 * For APPLE this just accesses the M_PKTHDR length so it operates on an
 * mbuf with hdr flag. Other O/S's may have seperate packet header and mbuf
 * chain pointers.. thus the macro.
 */
#define SCTP_HEADER_TO_CHAIN(m) (m)
#define SCTP_HEADER_LEN(m) (m->m_pkthdr.len)
#define SCTP_GET_HEADER_FOR_OUTPUT(len) sctp_get_mbuf_for_msg(len, 1, M_DONTWAIT, 1, MT_DATA)

/* Attach the chain of data into the sendable packet. */
#define SCTP_ATTACH_CHAIN(pak, m, packet_length) do { \
	pak->m_next = m; \
	pak->m_pkthdr.len = packet_length; \
} while(0)

/* Other m_pkthdr type things */
#define SCTP_IS_IT_BROADCAST(dst, m) in_broadcast(dst, m->m_pkthdr.rcvif)
#define SCTP_IS_IT_LOOPBACK(m) ((m->m_pkthdr.rcvif == NULL) || (m->m_pkthdr.rcvif->if_type != IFT_LOOP))


/*
 * This converts any input packet header into the chain of data holders,
 * for APPLE its a NOP.
 */
#define SCTP_PAK_TO_BUF(i_pak) (i_pak)

/* Macro's for getting length from V6/V4 header */
#define SCTP_GET_IPV4_LENGTH(iph) (iph->ip_len)
#define SCTP_GET_IPV6_LENGTH(ip6) (ntohs(ip6->ip6_plen))


/*
 * Other MacOS specific
 */

/* Apple KPI defines for atomic operations */
#include <libkern/OSAtomic.h>
#define atomic_add_int(addr, val)	OSAddAtomic(val, (SInt32 *)addr)
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

/* locks */
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
#ifdef _KERN_LOCKS_H_
extern lck_rw_t *sctp_calloutq_mtx;
#else
extern void *sctp_calloutq_mtx;
#endif
#endif

#endif
