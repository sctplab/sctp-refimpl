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
 * $Id: takeover.h,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/

#ifndef _TAKEOVER_H_
#define _TAKEOVER_H 1

#include "cblib.h"
#include "backend.h"

#define TAKEOVER_TIMEOUT 60000

typedef struct takeover_struct *Takeover;
struct takeover_struct {
    /* next in linked list */
    Takeover      next;
    Takeover      prev;

    /* server to be taken over */
    uint32        targetServerId;

    /* list of serverIds which need to ack the process */
    int           listSize;
    uint32       *ackedServers;
    int           ackedServerCount;

    /* in this time all other servers need to ack the process */
    Timer         expireTimer;
};

extern Takeover takeoverList;

Takeover
takeoverNew(uint32 targetServerId);

Takeover
takeoverLookup(uint32 targetServerId);

int
takeoverAckServer(Takeover takeover, uint32 serverId);

int
takeoverDoTakeover(Takeover takeover);

void
takeoverCallback(void *arg);

void
takeoverListPrint();

int
takeoverPrint(Takeover takeover);

int
takeoverDelete(Takeover takeover);

#endif /* _TAKEOVER_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2007/11/06 08:21:30  volkmer
 * reformated the copyright statement
 *
 * Revision 1.9  2007/10/27 12:45:26  volkmer
 * fixed 2 minor bugs and removed debug macro
 *
 **/
