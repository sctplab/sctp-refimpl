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
 * $Author: tuexen $
 * $Id: poolelement.h,v 1.3 2008-03-06 17:56:50 tuexen Exp $
 *
 **/
#ifndef _POOLELEMENT_H
#define _POOLELEMENT_H 1

#include <netinet/in.h>
#include <netinet/sctp.h>

#include "backend.h"
#include "checksum.h"
#include "cblib.h"

struct pool_element_struct;

typedef struct pool_element_struct *PoolElement;
struct pool_element_struct {
    PoolElement     poolNext;
    PoolElement     poolPrev;

    PoolElement     serverNext;
    PoolElement     serverPrev;

    void*           peServerPool;
    void*           peRegistrarServer;

    uint32          peIdentifier;
    uint32          peHomeRegistrarServer;
    int32           peRegistrationLife;
    checksumType    peChecksum;
    int             peIsDirty;

    Timer           peLastHeardTimeout;
    int             peLastHeardTimeoutCnt;
    Timer           peLifetimeExpiryTimer;

    uint16          peAsapTransportPort;
    Address         peAsapTransportAddr[MAX_ADDR_CNT];
    sctp_assoc_t    peAsapAssocId;

    uint8           peUserTransportProtocol;
    uint16          peUserTransportPort;
    Address         peUserTransportAddr[MAX_ADDR_CNT];
    uint16          peUserTransportUse;

    uint32          pePolicyID;
    uint32          pePolicyWeightLoad;
    uint32          pePolicyLoadDegradation;
};

PoolElement
poolElementNew(uint32 pelementId);

int
poolElementPrint(PoolElement pe);

int
poolElementDelete(PoolElement pe);

#endif

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2008/01/11 00:59:08  volkmer
 * added handle space synchronizing flags
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.15  2007/11/19 22:37:37  volkmer
 * removed comment
 *
 * Revision 1.14  2007/11/07 12:55:14  volkmer
 * added asap assocId to pe struct
 *
 * Revision 1.13  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.12  2007/10/27 12:46:52  volkmer
 * removed debug macro
 *
 **/
