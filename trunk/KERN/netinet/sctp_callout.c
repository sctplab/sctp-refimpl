/*-
 * Copyright (c) 2001-2006, Cisco Systems, Inc. All rights reserved.
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

#include <netinet/sctp_os.h>
#include <netinet/sctp_callout.h>
#include <netinet/sctp_pcb.h>

/*
 * Callout/Timer routines for OS that doesn't have them
 */
#ifdef __APPLE__
int ticks = 0;
#else
extern int ticks;
#endif

void
sctp_os_timer_init(sctp_os_timer_t *c)
{
	bzero(c, sizeof(*c));
}

void
sctp_os_timer_start(sctp_os_timer_t *c, int to_ticks, void (*ftn) (void *),
	      void *arg) {
	int s;

	s = splhigh();
	if (c->c_flags & SCTP_CALLOUT_PENDING)
		sctp_os_timer_stop(c);

	/*
	 * We could spl down here and back up at the TAILQ_INSERT_TAIL, but
	 * there's no point since doing this setup doesn't take much time.
	 */
	if (to_ticks <= 0)
		to_ticks = 1;

	c->c_arg = arg;
	c->c_flags |= (SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING);
	c->c_func = ftn;
	c->c_time = ticks + to_ticks;
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_lock_exclusive(sctp_calloutq_mtx);
#endif
	TAILQ_INSERT_TAIL(&sctppcbinfo.callqueue, c, tqe);
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_unlock_exclusive(sctp_calloutq_mtx);
#endif
	splx(s);
}

int
sctp_os_timer_stop(sctp_os_timer_t *c)
{
	int s;

	s = splhigh();
	/*
	 * Don't attempt to delete a callout that's not on the queue.
	 */
	if (!(c->c_flags & SCTP_CALLOUT_PENDING)) {
		c->c_flags &= ~SCTP_CALLOUT_ACTIVE;
		splx(s);
		return (0);
	}
	c->c_flags &= ~(SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING |
			SCTP_CALLOUT_FIRED);
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_lock_exclusive(sctp_calloutq_mtx);
#endif
	TAILQ_REMOVE(&sctppcbinfo.callqueue, c, tqe);
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_unlock_exclusive(sctp_calloutq_mtx);
#endif
	c->c_func = NULL;
	splx(s);
	return (1);
}

#if defined(__APPLE__)
/*
 * For __APPLE__, use a single main timer at a faster resolution than
 * fastim.  The timer just calls this existing callout infrastructure.
 */
#endif
void
sctp_fasttim(void)
{
	sctp_os_timer_t *c, *n;
	struct calloutlist locallist;
	int inited = 0;
	int s;

	s = splhigh();
#if defined(__APPLE__)
	/* update our tick count */
	ticks += sctp_main_timer_ticks;
/*
printf("sctp_fasttim: ticks = %u (added %u)\n", (uint32_t)ticks,
       (uint32_t)sctp_main_timer_ticks);
*/
#endif

#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_lock_exclusive(sctp_calloutq_mtx);
#endif
	/* run through and subtract and mark all callouts */
	c = TAILQ_FIRST(&sctppcbinfo.callqueue);
	while (c) {
		n = TAILQ_NEXT(c, tqe);
		if (c->c_time <= ticks) {
			c->c_flags |= SCTP_CALLOUT_FIRED;
			c->c_time = 0;
			TAILQ_REMOVE(&sctppcbinfo.callqueue, c, tqe);
			if (inited == 0) {
				TAILQ_INIT(&locallist);
				inited = 1;
			}
			/* move off of main list */
			TAILQ_INSERT_TAIL(&locallist, c, tqe);
		}
		c = n;
	}
	/* Now all the ones on the locallist must be called */
	if (inited) {
		c = TAILQ_FIRST(&locallist);
		while (c) {
			/* remove it */
			TAILQ_REMOVE(&locallist, c, tqe);
			/* now validate that it did not get canceled */
			if (c->c_flags & SCTP_CALLOUT_FIRED) {
				c->c_flags &= ~SCTP_CALLOUT_PENDING;
				splx(s);
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
				lck_rw_unlock_exclusive(sctp_calloutq_mtx);
#endif
				(*c->c_func) (c->c_arg);
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
				lck_rw_lock_exclusive(sctp_calloutq_mtx);
#endif
				s = splhigh();
			}
			c = TAILQ_FIRST(&locallist);
		}
	}
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
	lck_rw_unlock_exclusive(sctp_calloutq_mtx);
#endif
#if defined(__APPLE__)
	/* restart the main timer */
	sctp_start_main_timer();
#endif
	splx(s);
}
