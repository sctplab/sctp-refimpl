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
#ifndef __sctp_os_netbsd_h__
#define __sctp_os_netbsd_h__

/*
 * includes
 */
#include "rnd.h"
#include <sys/rnd.h>

/*
 * general memory allocation
 */
#define SCTP_MALLOC(var, type, size, name) \
    do { \
	MALLOC(var, type, size, M_PCB, M_NOWAIT); \
    } while (0)

#define SCTP_FREE(var)	FREE(var, M_PCB)


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

/*
 * timers
 */
#include <sys/callout.h>
typedef struct callout sctp_os_timer_t;

#define SCTP_OS_TIMER_INIT	callout_init
#define SCTP_OS_TIMER_START	callout_reset
#define SCTP_OS_TIMER_STOP	callout_stop
#define SCTP_OS_TIMER_PENDING	callout_pending
#define SCTP_OS_TIMER_ACTIVE	callout_active
#define SCTP_OS_TIMER_DEACTIVATE callout_deactivate


/*
 * Functions
 */
#if NRND > 0
#define SCP_READ_RANDOM(buf, len)	rnd_extract_data(buf, len, RND_EXTRACT_ANY);
#else
extern void SCTP_READ_RANDOM(void *buf, uint32_t len);
#endif

/*
 * Other NetBSD Specific
 */
/* emulate the atomic_xxx() functions... */
#define atomic_add_int(addr, val)	(*(addr) += val)
#define atomic_subtract_int(addr, val)	(*(addr) -= val)

#endif
