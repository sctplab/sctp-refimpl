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
 * $Id: poolelement.c,v 1.2 2008-02-13 17:03:41 volkmer Exp $
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "poolelement.h"
#include "registrarserver.h"
#include "debug.h"
#include "util.h"
#include "callbacks.h"

PoolElement
poolElementNew(uint32 pelementId) {
    PoolElement pe;
    char timerName[TIMER_DESCRIPTION_LENGTH + 1];

    pe = (PoolElement) malloc(sizeof(*pe));
    if (pe == NULL) {
        logDebug("pe is NULL: out of memory");

        return NULL;
    }

    memset(pe, 0, sizeof(*pe));
    memset(timerName, 0, sizeof(timerName));

    snprintf(timerName, TIMER_DESCRIPTION_LENGTH, "registration timeout for 0x%08x", pelementId);

    pe->peLifetimeExpiryTimer = timerCreate(handlePoolElementLifetimeExpiry, pe, timerName);
    if (pe->peLifetimeExpiryTimer == NULL) {
        free(pe);
        return NULL;
    }

    pe->peIdentifier = pelementId;
    logDebug("created new pool element with id 0x%08x", pelementId);
    
    return pe;
}

int
poolElementPrint(PoolElement pe) {
    if (pe == NULL) {
        logDebug("pe is NULL");
        return -1;
    }

    logDebug("--- Pool Element 0x%08x ---",     pe->peIdentifier);
    logDebug("peHomeRegistrarServer:   0x%08x", pe->peHomeRegistrarServer);
    logDebug("peRegistrationLife:      %d",     pe->peRegistrationLife);
    logDebug("peChecksum:              0x%08x", pe->peChecksum);
    logDebug("peAsapTransportPort:     %d"  ,   pe->peAsapTransportPort);
    logDebug("peASAPTransportAddr");
    addressListPrint(pe->peAsapTransportAddr);
    logDebug("peUserTransportProtocol: %d",     pe->peUserTransportProtocol);
    logDebug("peUserTransportAddr");
    addressListPrint(pe->peUserTransportAddr);
    logDebug("peUserTransportUse:      %d",     pe->peUserTransportUse);
    logDebug("pePolicyID:              %d",     pe->pePolicyID);
    logDebug("pePolicyWeightLoad:      %d",     pe->pePolicyWeightLoad);
    logDebug("pePolicyLoadDegradation: %d",     pe->pePolicyLoadDegradation);

    return 1;
}

int
poolElementDelete(PoolElement pe) {
    if (pe == NULL) {
        logDebug("pe is NULL");
        return -1;
    }

    if (pe->peLastHeardTimeout != NULL) {
        logDebug("deleting LastHeardTimeout timer from poolElement");
        timerDelete(pe->peLastHeardTimeout);
    } else 
        logDebug("peLastHeardTimeout is NULL !!!");
        
    if (pe->peLifetimeExpiryTimer != NULL) {
        logDebug("deleting LifetimeExpiryTimer timer from poolElement");
        timerDelete(pe->peLifetimeExpiryTimer);
    } else
        logDebug("peLifetimeExpiryTimer is NULL !!!");


    logDebug("deleting pool element 0x%08x", pe->peIdentifier);

    memset(pe, 0, sizeof(*pe));
    free(pe);

    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.14  2007/12/06 01:52:53  volkmer
 * some pretty printing
 *
 * Revision 1.13  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.12  2007/10/27 12:46:52  volkmer
 * removed debug macro
 *
 **/
