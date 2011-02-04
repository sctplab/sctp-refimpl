/*-
 * Copyright (c) 2007, by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2008-2011, by Randall Stewart. All rights reserved.
 * Copyright (c) 2008-2011, by Michael Tuexen. All rights reserved.
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
#ifndef __sctp_os_netbsd_h__
#define __sctp_os_netbsd_h__

/*
 * includes
 */
#include "opt_ipsec.h"
#include "opt_inet.h"
#include "opt_sctp.h"

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
#include <sys/queue.h>

#include <machine/limits.h>
#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/net_osdep.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifdef INET6
#include <sys/domain.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#endif				/* INET6 */

#include <netinet/in_pcb.h>

#include "rnd.h"
#include <sys/rnd.h>

#include <machine/stdarg.h>

#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif				/* IPSEC */

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>
#endif


#if defined(HAVE_NRL_INPCB)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#endif

#define SCTP_BASE_INFO(type) system_base_info.sctppcbinfo.type
#define SCTP_BASE_STATS system_base_info.sctpstat
#define SCTP_BASE_STAT(elem)     system_base_info.sctpstat.elm

#define SCTP_MAX_VRF_ID 0
#define SCTP_SIZE_OF_VRF_HASH 1

#define SCTP_DEFAULT_VRFID 0
/*
 * Access to IFN's to help with src-addr-selection
 */

/* This could return VOID if the index works 
 * but for BSD we provide both.
 */
#define SCTP_GET_IFN_VOID_FROM_ROUTE(ro) (void *)ro->ro_rt->rt_ifp
#define SCTP_GET_IF_INDEX_FROM_ROUTE(ro) ro->ro_rt->rt_ifp->if_index



/*
 * general memory allocation
 */
#define SCTP_MALLOC(var, type, size, name) \
    do { \
	MALLOC(var, type, size, M_PCB, M_NOWAIT); \
    } while (0)

#define SCTP_FREE(var)	FREE(var, M_PCB)

#define SCTP_M_COPYM	m_copym

/*
 * zone allocation functions
 */
#include <sys/pool.h>
typedef struct pool sctp_zone_t;
void *sctp_pool_get(struct pool *, int);
void sctp_pool_put(struct pool *, void *);

/* SCTP_ZONE_INIT: initialize the zone */
#define SCTP_ZONE_INIT(zone, name, size, number) \
	pool_init(&(zone), size, 0, 0, 0, name, NULL);

/* SCTP_ZONE_GET: allocate element from the zone */
#define SCTP_ZONE_GET(zone) \
	sctp_pool_get(&zone, PR_NOWAIT);

/* SCTP_ZONE_FREE: free element from the zone */
#define SCTP_ZONE_FREE(zone, element) \
	sctp_pool_put(&zone, element);

#define SCTP_HASH_INIT(size, hashmark) hashinit(size, HASH_LIST, M_PCB, M_WAITOK, hashmark)
#define SCTP_HASH_FREE(table, hashmark) hash_destroy(table, M_PCB, hashmark)

/*
 * timers
 */
#include <sys/callout.h>
typedef struct callout sctp_os_timer_t;

#define SCTP_OS_TIMER_INIT	callout_init
#define SCTP_OS_TIMER_START	callout_reset
#define SCTP_OS_TIMER_STOP_DRAIN callout_stop
#define SCTP_OS_TIMER_STOP	callout_stop
#define SCTP_OS_TIMER_PENDING	callout_pending
#define SCTP_OS_TIMER_ACTIVE	callout_active
#define SCTP_OS_TIMER_DEACTIVATE callout_deactivate


/* is the endpoint v6only? */
#define SCTP_IPV6_V6ONLY(inp) (((struct in6pcb *)inp)->in6p_flags & IN6P_IPV6_V6ONLY)

/*
 * routes, output, etc.
 */
typedef struct route	sctp_route_t;
typedef struct rtentry	sctp_rtentry_t;
#define SCTP_RTALLOC(ro)	rtalloc((struct route *)ro)

/*
 * SCTP AUTH
 */
#define HAVE_SHA2

#if NRND > 0
#define SCP_READ_RANDOM(buf, len)	rnd_extract_data(buf, len, RND_EXTRACT_ANY);
#else
extern void SCTP_READ_RANDOM(void *buf, uint32_t len);
#endif

struct mbuf *
sctp_get_mbuf_for_msg(unsigned int space_needed, 
		      int want_header, int how, int allonebuf, int type);


#ifdef USE_SCTP_SHA1
#include <netinet/sctp_sha1.h>
#else
#include <sys/sha1.h>
/* map standard crypto API names */
#define SHA1_Init	SHA1Init
#define SHA1_Update	SHA1Update
#define SHA1_Final	SHA1Final
#endif

#if defined(HAVE_SHA2)
#include <sys/sha2.h>
#endif

#include <sys/md5.h>
/* map standard crypto API names */
#define MD5_Init	MD5Init
#define MD5_Update	MD5Update
#define MD5_Final	MD5Final


/*
 * Other NetBSD Specific
 */
/* emulate the atomic_xxx() functions... */
#define atomic_add_int(addr, val)	(*(addr) += val)
#define atomic_subtract_int(addr, val)	(*(addr) -= val)

#endif
