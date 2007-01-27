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
#ifndef __sctp_os_openbsd_h__
#define __sctp_os_openbsd_h__

/*
 * includes
 */
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

#include <machine/limits.h>
#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

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
#include <netinet/in_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#endif				/* INET6 */

#include <netinet/in_pcb.h>
#include <dev/rndvar.h>

#include <machine/stdarg.h>

#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
s
#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>
#endif

#if defined(HAVE_NRL_INPCB)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#endif

#undef IPSEC

/* port ranges */
extern int ipport_firstauto;
extern int ipport_lastauto;
extern int ipport_hifirstauto;
extern int ipport_hilastauto;

#define SCTP_LIST_EMPTY(list)	LIST_EMPTY(list)

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

#define SCTP_HASH_INIT(size, hashmark) hashinit(size, M_PCB, M_WAITOK, hashmark)
#define SCTP_HASH_FREE(table, hashmark) hashdestroy(table, M_PCB, hashmark)

/*
 * timers
 */
#include <sys/timeout.h>
typedef struct timeout sctp_os_timer_t;

/* map sctp os timer into timeout for OpenBSD */
#define SCTP_OS_TIMER_INIT(args)	/* TBD */
#define SCTP_OS_TIMER_START(c, ticks, func, arg) \
    do { \
	timeout_set((c), (func), (arg)); \
	timeout_add((c), (ticks)); \
    } while (0)
#define SCTP_OS_TIMER_STOP	timeout_del
#define SCTP_OS_TIMER_STOP_DRAIN timeout_del
#define SCTP_OS_TIMER_PENDING	timeout_pending
#define SCTP_OS_TIMER_ACTIVE	timeout_initialized
#define SCTP_OS_TIMER_DEACTIVATE(args)	/* TBD */

/* is the endpoint v6only?  Always dual bind. */
#define SCTP_IPV6_V6ONLY(inp)	(0)

/*
 * Functions
 */
#define sctp_read_random(buf, len)	get_random_bytes(buf, len)

/*
 * Other OpenBSD Specific
 */

#endif
