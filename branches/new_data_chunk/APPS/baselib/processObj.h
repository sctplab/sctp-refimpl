#ifndef __processObj_h__
#define __processObj_h__
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
#include <sys/types.h>
#include <distributor.h>
#include <stdlib.h>
#include <unistd.h>
#include <memcheck.h>


#define PROC_BUFFER_SIZE 65536

typedef int (*processMsgFramer)(void *obj, uint8_t *buf, uint32_t length, void *auxObj);
typedef void (*processExits)(void *obj, uint8_t *buf, uint32_t length, void *auxObj);


struct processDescriptor {
  distributor *d;
  FILE *pip;
  char *commandString;
  void *auxObj;
  processMsgFramer mf;
  processExits mx;
  uint8_t recvBuffer[PROC_BUFFER_SIZE];
  uint32_t readIndex;
  uint32_t bufStart;
  uint32_t bufEnd;
  uint32_t maxlen;
};

struct processDescriptor *
initialize_processObj(distributor *d, processMsgFramer, processExits, char *command, int maxbuflen, void *auxObj);

int
sendToProcess(struct processDescriptor *pd, char *msg, int len);

void
destroy_process(struct processDescriptor *pd);


#endif
