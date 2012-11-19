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
 * $Id: takeover.c,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/
#include <stdlib.h>
#include <string.h>

#include "registrarserver.h"
#include "takeover.h"
#include "cblib.h"
#include "backend.h"
#include "debug.h"
#include "asap.h"
#include "enrp.h"


Takeover takeoverList;


Takeover
takeoverNew(uint32 targetServerId) {
    Takeover newTakeover = NULL;
    int registrarCount = 0;
    RegistrarServer server = NULL;
    char timerName[TIMER_DESCRIPTION_LENGTH];

    if (targetServerId == ownId) {
        logDebug("can not takeover myself, ABORT");
        return NULL;
    }

    server = registrarServerListGetServerByServerId(targetServerId);

    if (server == NULL) {
        logDebug("server is not in list");
        return NULL;
    }

    server->rsActive = 0;

    newTakeover = (Takeover) malloc(sizeof(*newTakeover));
    memset(newTakeover, 0, sizeof(*newTakeover));

    for (server = registrarServerList; server->next; server = server->next) {
        if (server->rsIdentifier == ownId || server->rsIdentifier == targetServerId)
            continue;
        registrarCount++;
    }

    memset(timerName, 0, sizeof(timerName));
    snprintf(timerName, TIMER_DESCRIPTION_LENGTH, "takeoverTimer for targetServer: 0x%08x", targetServerId);

    newTakeover->targetServerId = targetServerId;
    newTakeover->expireTimer = timerCreate(takeoverCallback, newTakeover, timerName);
    newTakeover->next = NULL;
    newTakeover->prev = NULL;
    newTakeover->listSize = registrarCount;
    newTakeover->ackedServers = (uint32 *) malloc(registrarCount * sizeof(uint32));

    memset(newTakeover->ackedServers, 0, registrarCount * sizeof(uint32));

    newTakeover->prev = NULL;
    newTakeover->next = takeoverList;
    if (takeoverList) {
       takeoverList->prev = newTakeover;
    }
    takeoverList = newTakeover;

    timerStart(newTakeover->expireTimer, TAKEOVER_TIMEOUT);
    return newTakeover;
}

Takeover
takeoverLookup(uint32 targetServerId) {
    Takeover tmp = NULL;

    logDebug("looking up takeoverprocess 0x%08x", targetServerId)

    for (tmp = takeoverList; tmp != NULL; tmp = tmp->next)
        if (tmp->targetServerId == targetServerId)
            return tmp;

    return NULL;
}

int
takeoverAckServer(Takeover takeover, uint32 serverId) {
    RegistrarServer server = NULL;
    int i;

    if (takeover == NULL) {
        logDebug("takeover is NULL");
        return -1;
    }

    server = registrarServerListGetServerByServerId(serverId);
    if (server == NULL) {
        logDebug("server is not in list");
        return -1;
    }

    for (i = 0; i < takeover->listSize; i++) {
        if (takeover->ackedServers[i] == serverId) {
            logDebug("already acked serverId of 0x%08x", serverId);
            return -1;
        }
        if (takeover->ackedServers[i] == 0) {
            takeover->ackedServers[i] = serverId;
            takeover->ackedServerCount++;
        }
    }

    if (takeover->listSize == takeover->ackedServerCount) {
        logDebug("all servers have acked the init, doing the takeover")
        takeoverDoTakeover(takeover);
    }

    return 1;
}

int
takeoverDoTakeover(Takeover takeover) {
    RegistrarServer targetServer;
    PoolElement     element;
    uint32          serverId;

    if (takeover == NULL) {
        logDebug("takeover is NULL");
        return -1;
    }

    targetServer = registrarServerListGetServerByServerId(takeover->targetServerId);
    if (targetServer == NULL) {
        logDebug("targetServer to be taken over is NULL, abort\n\n");
        return -1;
    }

    serverId = targetServer->rsIdentifier;

    sendEnrpTakeoverServer(0, takeover->targetServerId, 0);

    if (targetServer->rsPeList == NULL) {
        logDebug("target server has no elements to move");
    } else {
        logDebug("moving all pool elements from 0x%08x to 0x%08x", targetServer->rsIdentifier, ownId);
        element = targetServer->rsPeList;
        while (element) {
            PoolElement nextElement = element->serverNext;
            registrarServerRemovePoolElement(targetServer, element);
            registrarServerAddPoolElement(this, element);
            element->peHomeRegistrarServer = ownId;
            
            sendAsapEndpointKeepAlive(element, 1);
            element = nextElement;
        }
    }

    takeoverDelete(takeover);

    registrarServerListRemoveServer(serverId);
    logDebug("finished takeover of 0x%08x", serverId);

    return 1;
}

void
takeoverCallback(void *arg) {
    Takeover takeover = (Takeover) arg;

    logDebug("takeoverCallback called");

    if (arg == NULL) {
        logDebug("arg is NULL");
        return;
    }

    logDebug("the takeover for 0x%08x timed out, doing the takeover", takeover->targetServerId);

    takeoverDoTakeover(takeover);

    return;
}

void
takeoverListPrint() {
    Takeover tmp;

    if (takeoverList == NULL) {
        logDebug("takeoverList is empty");
        return;
    }

    logDebug("printing list of takeover processes");
    for (tmp = takeoverList; tmp != NULL; tmp = tmp->next)
        takeoverPrint(tmp);

    return;
}

int
takeoverPrint(Takeover takeover) {
    int i;

    if (!takeover) {
        logDebug("takeover is NULL");
        return -1;
    }

    logDebug("address: %p - next: %p", (void *)takeover, (void *)takeover->next);
    logDebug("targetServerId: 0x%08x", takeover->targetServerId);

    for (i = 0; i < takeover->listSize; i++)
        logDebug("ackedServers[%d]: 0x%08x", i, takeover->ackedServers[i]);
    timerPrint(takeover->expireTimer);

    return 1;
}

int
takeoverDelete(Takeover takeover) {
    if (!takeover) {
        logDebug("takeover is NULL");
        return -1;
    }

    if (!takeover->expireTimer) {
        logDebug("expireTimer in takeover is NULL");
        return -1;
    } else {
        timerStop(takeover->expireTimer);
        timerDelete(takeover->expireTimer);
    }

    if (takeover->ackedServers == NULL ) {
        logDebug("ackedServers is NULL");
    } else {
        memset(takeover->ackedServers, 0, takeover->listSize * sizeof(uint32));
        free(takeover->ackedServers);
    }

    logDebug("deleted takeover process data for targetServerId: 0x%08x", takeover->targetServerId);

    if (takeover->prev) {
        takeover->prev->next = takeover->next;
    }
    if (takeover->next) {
        takeover->next->prev = takeover->prev;
    }
    if (takeoverList == takeover) {
        takeoverList = takeover->next;
    }

    memset(takeover, 0, sizeof(*takeover));
    free(takeover);
    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.17  2007/11/06 08:21:30  volkmer
 * reformated the copyright statement
 *
 * Revision 1.16  2007/10/27 12:45:26  volkmer
 * fixed 2 minor bugs and removed debug macro
 *
 **/
