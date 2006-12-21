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
#include <dev/rndvar.h>

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
#include <sys/timeout.h>
typedef struct timeout sctp_os_timer_t;

/* map sctp os timer into timeout for OpenBSD */
#define sctp_os_timer_init(args)
#define sctp_os_timer_start(c, ticks, func, arg) \
    do { \
	timeout_set((c), (func), (arg)); \
	timeout_add((c), (ticks)); \
    } while (0)
#define sctp_os_timer_stop	timeout_del
#define sctp_os_timer_pending	timeout_pending
#define sctp_os_timer_active	timeout_initialized


/*
 * Functions
 */
#define sctp_read_random(buf, len)	get_random_bytes(buf, len)

/*
 * Other OpenBSD Specific
 */

#endif
