/*
 * Copyright (c) 2006-2007, Michael Tuexen, Frank Volkmer. All rights reserved.
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
 * $Author: randall $
 * $Id: cblib.h,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/

#ifndef _CBLIB_H
#define _CBLIB_H 1

#include <sys/time.h>

#pragma pack(1)

#define TIMER_DESCRIPTION_LENGTH 100

typedef struct timer_struct *Timer;
struct timer_struct {
    Timer           prev;
    Timer           next;
    struct timeval  start_time;
    struct timeval  stop_time;
    char            description[TIMER_DESCRIPTION_LENGTH + 1];
    void            *arg;
    void            (*callback) (void *);
};

Timer
timerCreate(void (*callback)(void *), void *arg, char *description);

void
timerDelete(Timer timer);

void
timerStart(Timer timer, unsigned int timeout);

void
timerRestart(Timer timer, unsigned int timeout);

void
timerStop(Timer timer);

void
timerRegisterFdCallback(int fd, void (*callback)(void *), void *arg);

void
timerRegisterStdinCallback(void (*callback)(void *), void *arg);

void
timerPrint(Timer timer);

void
timerListPrint(void);

void
handleEvents(void);

void
initCblib(void);

int
timevalBefore(struct timeval *tv1, struct timeval *tv2);

void
getTime(struct timeval *tv);

#endif /* _CBLIB_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:23:12  volkmer
 * removed debug macro
 *
 **/
