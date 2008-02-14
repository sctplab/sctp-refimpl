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
 * $Id: callbacks.h,v 1.2 2008-02-14 10:21:39 volkmer Exp $
 *
 **/
#ifndef _CALLBACKS_H
#define _CALLBACKS_H 1

void
handleSctpEvent(void *buf, size_t len);

void
enrpSctpCallback(void *arg);

void
enrpUdpCallback(void *arg);

void
serverHuntTimeout(void *arg);

void
enrpAnnounceTimeout(void *arg);

void
peerLastHeardTimeout(void *arg);

void
asapUdpCallback(void *arg);

void
asapSctpCallback(void *arg);

void
asapAnnounceTimeout(void *arg);

void
handlePoolElementLifetimeExpiry(void *arg);

#ifdef DEBUG
void
endTimeout(void *arg);
#endif /* DEBUG */

#endif /* _CALLBACKS_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.12  2007/12/06 01:52:15  volkmer
 * moved peliftimeexpirytimeoutcallback
 *
 * Revision 1.11  2007/11/05 00:06:05  volkmer
 * reformated the copyright statementstarted working on the asapsctpcallback
 *
 * Revision 1.10  2007/10/27 13:25:58  volkmer
 * added asap callbacksremoved debug macro
 *
 **/
