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
 * $Id: util.h,v 1.2 2008-02-13 17:06:50 volkmer Exp $
 *
 **/

#ifndef _UTIL_H
#define _UTIL_H 1

#include "backend.h"
#include "poolelement.h"

#define ADD_PADDING(x) ((((x) + 3) >> 2) << 2)

uint8
checkAndIdentifyMsg(char *msg, size_t len, sctp_assoc_t assocId);

uint16
checkTLV(char *pos, ssize_t bufLength);

int
transportAddressesToSendBuffer(char *offset, Address *addrs,
                               uint16 port, uint16 transportUse,
                               uint8 protocol);
int
sendBufferToTransportAddresses(char *offset, size_t bufSize,
                               Address *addrs, int *addrCnt,
                               uint16 *port, uint16 *transportUse,
                               uint8 *protocol);

int
poolElementToSendBuffer(PoolElement pe, char *offset);

int
sendBufferToPoolElement(char *offset, size_t bufSize, PoolElement *pe);

int
addressesToSysCallBuffer(Address *addrs, uint16 port, int addrCnt, char *buf);

int
addressListPrint(Address *addrs);

void
enterServicingMode();

#ifndef HAVE_SCTP_SENDX
ssize_t sctp_sendx(int                      sd,
                   const void*              data,
                   size_t                   len,
                   struct sockaddr*         addrs,
                   int                      addrcnt,
                   struct sctp_sndrcvinfo*  sinfo,
                   int                      flags);
#endif

#endif

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.13  2007/12/05 23:13:06  volkmer
 * refactored enterservicingmode to util.c
 *
 * Revision 1.12  2007/12/02 22:16:14  volkmer
 * fixed and reworked sendBufferToPoolElement and sendBufferToTransportAddresses
 *
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 13:30:49  volkmer
 * removed debug macro
 *
 **/
