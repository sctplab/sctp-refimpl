/*
 * Copyright (c) 2006-2008, Michael Tuexen, Frank Volkmer. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Michael Tuexen nor Frank Volkmer nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * $Author: volkmer $
 * $Id: cblib.c,v 1.3 2008-02-14 15:05:42 volkmer Exp $
 *
 **/

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "debug.h"
#include "cblib.h"

static struct cbarg {
    int fd;
    void *arg;
    void (*callback) (void *);
} cb[FD_SETSIZE];

static Timer timerList = NULL;
static fd_set read_fds;

static void *
dummy (void *arg) {
    int fd;

    fd = *(int *)arg;

    logDebug("this function should not have been called. clearing fd %u", fd);
    FD_CLR(fd, &read_fds);

    return arg;
}

void
initCblib() {
    int fd;

    FD_ZERO(&read_fds);
    for (fd = 0; fd < FD_SETSIZE; fd++) {
        cb[fd].fd       = fd;
        cb[fd].callback = (void (*)(void *)) dummy;
        cb[fd].arg      = (void *) (&cb[fd].fd);
    }

    return;
}

void
timerRegisterFdCallback(int fd, void (*callback)(void *), void *arg) {
    if ((fd <= 0) || (fd >= FD_SETSIZE) || (callback == NULL)) {
        logDebug("invalid arguments");
        return;
    }

    cb[fd].arg = arg;
    cb[fd].callback = callback;
    FD_SET(fd, &read_fds);
}

void
timerRegisterStdinCallback(void (*callback)(void *), void *arg) {
    if (callback == NULL) {
        logDebug("invalid argument, callback is NULL");
        return;
    }

    cb[0].callback = callback;
    cb[0].arg      = arg;
    FD_SET(0, &read_fds);
}

Timer
timerCreate(void (*callback)(void *), void *arg, char *description) {
    Timer timer;

    if (callback == NULL) {
        logDebug("callback is NULL");
        return NULL;
    }

    if (description == NULL) {
        logDebug("description is NULL");
        return NULL;
    }

    timer = (Timer) malloc(sizeof(*timer));

    if (timer) {
        memset((void *)timer, 0, sizeof(*timer));
        strncpy(timer->description, description, TIMER_DESCRIPTION_LENGTH);
        timer->callback  = callback;
        timer->arg       = arg;
    } else
        logDebug("could not allocate memory");

    logDebug("created timer: %.*s", TIMER_DESCRIPTION_LENGTH, timer->description);

    return timer;
}

void
timerDelete(Timer timer) {
    if (timer == NULL) {
        logDebug("timer is NULL");
        return;
    }

    if ((timer->next != NULL) || (timer->prev != NULL) ||
        (timer->start_time.tv_sec != 0) || (timer->start_time.tv_usec != 0) ||
        (timer->stop_time.tv_sec  != 0) || (timer->stop_time.tv_usec  != 0)) {
        logDebug("timer %.*s not stopped", TIMER_DESCRIPTION_LENGTH, timer->description);
        timerStop(timer);
    }

    assert(timer->next == NULL);
    assert(timer->prev == NULL);
    assert(timerList != timer);

    memset(timer, 0, sizeof(*timer));
    free((void *) timer);
}

int
timevalBefore(struct timeval *tv1, struct timeval *tv2) {
    if (tv1 == NULL) {
        logDebug("inalid argument: tv1 is NULL");
        return 1;
    }

    if (tv2 == NULL) {
        logDebug("inalid argument: tv2 is NULL");
        return 1;
    }

    if ((tv1->tv_sec < tv2->tv_sec) || ((tv1->tv_sec == tv2->tv_sec) && (tv1->tv_usec < tv2->tv_usec)))
        return 1;

    return 0;
}

void
getTime(struct timeval *tv) {
    memset((void *)tv, 0, sizeof(struct timeval));

    if (gettimeofday(tv, NULL) < 0)
        logDebug("current time unknown");
}

void
timerStart(Timer timerToStart, unsigned int timeout) {
    struct timeval now;
    Timer timer;

    logDebug("timer name: %s", timerToStart->description);

    if (timerToStart == NULL) {
        logDebug("invalid argument: timerToStart is NULL");
        return;
    }

    /* check if timerToStart is not aready started yet. */
    getTime(&now);
    if ((timerToStart->next != NULL) || (timerToStart->prev != NULL) ||
        (timerToStart->start_time.tv_sec != 0) || (timerToStart->start_time.tv_usec != 0) ||
        (timerToStart->stop_time.tv_sec  != 0) || (timerToStart->stop_time.tv_usec  != 0)) {
        logDebug("timer is already started: %.*s", TIMER_DESCRIPTION_LENGTH, timerToStart->description);
        return;
    }

    /* set start_time and stop_time */
    timerToStart->start_time.tv_sec  = now.tv_sec;
    timerToStart->start_time.tv_usec = now.tv_usec;
    timerToStart->stop_time.tv_sec   = now.tv_sec  + (timeout / 1000);
    timerToStart->stop_time.tv_usec  = now.tv_usec + 1000 * (timeout % 1000);
    if (timerToStart->stop_time.tv_usec >= 1000000) {
        timerToStart->stop_time.tv_sec++;
        timerToStart->stop_time.tv_usec -= 1000000;
    }

    /* check for timerList */
    if (timerList == NULL) {
        timerList = timerToStart;
        return;
    }

    /* put into top of timerList */
    timer = timerList;
    if (timevalBefore(&timerToStart->stop_time, &timer->stop_time)) {
        timerToStart->next = timer;
        timerToStart->prev = NULL;
        timer->prev        = timerToStart;
        timerList          = timerToStart;
        return;
    }

    /* put into timerList */
    for (timer = timerList; timer->next; timer = timer->next)
        if (timevalBefore(&timerToStart->stop_time, &timer->next->stop_time))
            break;

    if (timer->next) {
        timer->next->prev = timerToStart;
    }
    timerToStart->next = timer->next;
    timer->next = timerToStart;
    timerToStart->prev = timer;

}

void
timerRestart(Timer timer, unsigned int timeout) {
    if (timer == NULL) {
        logDebug("timer is NULL");
        return;
    }

    timerStop(timer);
    timerStart(timer, timeout);

    return;
}

void
timerStop(Timer timerToStop) {
    Timer timer = NULL;

    if (timerToStop == NULL) {
        logDebug("timerToStop is NULL");
        return;
    }

    logDebug("timer name: %s", timerToStop->description);
    /* timerListPrint(); */

    /* find timer in timerList */
    for (timer = timerList; timer; timer = timer->next)
        if (timer == timerToStop)
            break;

    if (timer == NULL) {
        logDebug("timerToStop %p is not in timerList", (void *)timerToStop);
        return;
    }

    assert(timer == timerToStop);
    if (timer->prev) {
       timer->prev->next = timer->next;
    }
    if (timer->next) {
       timer->next->prev = timer->prev;
    }
    if (timerList == timer) {
       timerList = timer->next;
       assert((timerList == NULL) || (timerList->prev == NULL));
    }

    timerToStop->prev               = NULL;
    timerToStop->next               = NULL;
    timerToStop->start_time.tv_sec  = 0;
    timerToStop->start_time.tv_usec = 0;
    timerToStop->stop_time.tv_sec   = 0;
    timerToStop->stop_time.tv_usec  = 0;

    return;
}

void
timerPrint(Timer timer) {
    if (timer == NULL) {
        logDebug("timer is NULL");
        return;
    }

    logDebug("s: %ld:%06d cb: (%9p) p: %8p n: %8p - %.*s", timer->stop_time.tv_sec, (int) timer->stop_time.tv_usec, timer->arg, (void *)timer->prev, (void *)timer->next, TIMER_DESCRIPTION_LENGTH, timer->description);
}

void
timerListPrint(void) {
    #ifdef DEBUG
    Timer timer;

    for (timer = timerList; timer; timer = timer->next)
        timerPrint(timer);

    #endif
}

static void
handleEvent(void) {
    int fd, n;
    Timer timer;
    struct timeval now;
    struct timeval timeout;
    struct timeval *timeout_ptr;
    fd_set rset;

    logDebug("being called, timer list");
    timerListPrint();

    getTime(&now);

    timer = timerList;
    if ((timerList) && (timevalBefore(&timer->stop_time, &now))) {
        logDebug("handling timer event (%.*s) which should have already been handled", TIMER_DESCRIPTION_LENGTH, timer->description);
        timerStop(timer);
        (*timer->callback)(timer->arg);
        return;
    }

    if (timerList) {
        timer = timerList;
        timeout.tv_sec  = timer->stop_time.tv_sec  - now.tv_sec;
        timeout.tv_usec = timer->stop_time.tv_usec - now.tv_usec;
        if (timeout.tv_usec < 0) {
            timeout.tv_sec--;
            timeout.tv_usec += 1000000;
        }
        timeout_ptr = &timeout;
    } else
        timeout_ptr = NULL;

    rset = read_fds;
    n = select(FD_SETSIZE, &rset, NULL, NULL, timeout_ptr);

    if (n == 0) {
        timer = timerList;
        timerStop(timer);
        logDebug("handling timer event: %.*s", TIMER_DESCRIPTION_LENGTH, timer->description);
        (*timer->callback)(timer->arg);
    } else {
        for (fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &rset)) {
                logDebug("handling fd event for fd: %d", fd);
                (*(cb[fd].callback))(cb[fd].arg);
            }
        }
    }

    return;
}

void
handleEvents(void) {
    while (1)
        handleEvent();
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2008/01/08 20:31:40  tuexen
 * Fix a bug in start_timer().
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.20  2007/12/02 22:08:42  volkmer
 * some pretty printing
 *
 * Revision 1.19  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.18  2007/10/27 12:23:12  volkmer
 * removed debug macro
 *
 **/
