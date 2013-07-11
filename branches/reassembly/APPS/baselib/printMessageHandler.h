#ifndef __printMessageHandler_h__
#define __printMessageHandler_h__
/*
 * Copyright (C) 2008 Randall R. Stewart
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
the SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@lakerest.net

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/
#include <stdio.h>
#include <distributor.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <syslog.h>
#include <stdarg.h>

struct print_msg_info {
  distributor *d;
  int quiet;
};

/*
 * This message handler is intended for
 * debug purposes. You create one of these
 * and it registers itself at the back of the
 * message stack. Any message that comes by
 * is displayed and left fly by (just in
 * case you don't register this last and
 * some other object registers for messages
 * last). This can also be used in a text
 * typing type handler where you want
 * messages displayed and maybe a stdin
 * handler does something with what you type.
 */

/* This is how you control the amount of debug print out:
 * to print all out, do:
 
#define DEBUG_PRINTF_ON 1
 
 * to print LVL1 and above, do:

#define DEBUG_PRINTF_L1_ON 1

 * to print LVL2 and above, do:

#define DEBUG_PRINTF_L2_ON 1

 * to print LVL3 only, do:

#define DEBUG_PRINTF_L3_ON 1

 */
#define SEND_ALARM_CRIT(parms...) syslog((LOG_LOCAL4|LOG_CRIT), parms)
#define SEND_ALARM_MAJR(parms...) syslog((LOG_LOCAL4|LOG_ALERT), parms)
#define SEND_ALARM_MINR(parms...) syslog((LOG_LOCAL4|LOG_ERR), parms)
#define SEND_ALARM_INFO(parms...) syslog((LOG_LOCAL4|LOG_INFO), parms)

struct print_msg_info *
create_printMessageHandler(distributor *o, int quietmode);


void
destroy_printMessageHandler(struct print_msg_info *i);
/*
 * A utility for printing an address.
 */
void printAnAddress(struct sockaddr *a);

void printArry(uint8_t *data, int sz);

void printSetOutput(char *fullpath);

void printFlushOutput(void);

void printCheckLogFile(int maxsize, int maxfiles);

void coreFileMoveCheck(char *argv0, int maxfiles);

void timevaladd(struct timeval *t1, struct timeval *t2);
void timevalsub(struct timeval *t1, struct timeval *t2);

extern FILE *print_output;

#include <printDebug.h>

/****** end of defining debug_printf levels ********/

#endif

