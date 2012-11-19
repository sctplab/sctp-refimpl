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
 * $Id: registrarserver.c,v 1.2 2008-02-13 17:04:26 volkmer Exp $
 *
 **/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "registrarserver.h"
#include "backend.h"
#include "cblib.h"
#include "callbacks.h"
#include "enrp.h"
#include "poolelement.h"
#include "checksum.h"
#include "util.h"
#include "debug.h"


RegistrarServer registrarServerList;
RegistrarServer this;


RegistrarServer
registrarServerNew(uint32 serverId) {
    RegistrarServer server = NULL;
    char timerName[TIMER_DESCRIPTION_LENGTH + 1];

    server = (RegistrarServer) malloc(sizeof(*server));
    if (server == NULL)
        return NULL;

    memset(server, 0, sizeof(*server));
    memset(timerName, 0, sizeof(timerName));

    server->rsIdentifier = serverId;
    server->rsActive = 1;
    server->rsChecksum = 0;

    snprintf(timerName, TIMER_DESCRIPTION_LENGTH, "peer last heard timeout for registrar 0x%08x", serverId);
    server->rsLastHeardTimeout = timerCreate(peerLastHeardTimeout, &server->rsIdentifier, timerName);
    timerStart(server->rsLastHeardTimeout, MAX_TIME_LAST_HEARD);

    logDebug("created new registrar server with id: 0x%08x", serverId);
    return server;
}

int
registrarServerAddPoolElement(RegistrarServer server, PoolElement newPoolElement) {
    if (newPoolElement == NULL) {
        logDebug("newPoolElement is NULL");
        return -1;
    }

    if (server == NULL) {
        logDebug("server is NULL");
        return -1;
    }

    /* check if already in peList */
    if (registrarServerGetPoolElementById(server, newPoolElement->peIdentifier) != NULL) {
        logDebug("pelement with id 0x%08x is already in list, can not add it", newPoolElement->peIdentifier);
        return -1;
    }

    server->rsChecksum = checksumAdd(server->rsChecksum, newPoolElement->peChecksum);

    newPoolElement->serverPrev = NULL;
    newPoolElement->serverNext = server->rsPeList;
    if (server->rsPeList) {
        server->rsPeList->serverPrev = newPoolElement;
    }
    server->rsPeList = newPoolElement;

    newPoolElement->peRegistrarServer = server;

    timerStart(newPoolElement->peLifetimeExpiryTimer, newPoolElement->peRegistrationLife);

    logDebug("added 0x%08x to server", newPoolElement->peIdentifier);
    return 1;
}

int
registrarServerRemovePoolElement(RegistrarServer server, PoolElement pelement) {
    if (server == NULL) {
        logDebug("server is NULL");
        return -1;
    }

    if (pelement == NULL) {
        logDebug("pelement is NULL");
        return -1;
    }

    timerStop(pelement->peLifetimeExpiryTimer);
    server->rsChecksum = checksumSub(server->rsChecksum, pelement->peChecksum);

    pelement->peRegistrarServer = NULL;

    if (pelement->serverPrev)
        pelement->serverPrev->serverNext = pelement->serverNext;
    if (pelement->serverNext)
        pelement->serverNext->serverPrev = pelement->serverPrev;
    if (server->rsPeList == pelement) {
        server->rsPeList = pelement->serverNext;
    }
    pelement->serverPrev = NULL;
    pelement->serverNext = NULL;

    logDebug("removed 0x%08x from list", pelement->peIdentifier);
    return 1;
}

int
registrarServerResetTimer(RegistrarServer server) {
    if (server == NULL) {
        logDebug("server is NULL");
        return -1;
    }

    if (server->rsLastHeardTimeout == NULL) {
        logDebug("server->rsLastHeardTimeout is NULL");
        return -1;
    }

    logDebug("resetting last heard timer for 0x%08x", server->rsIdentifier);
    timerStart(server->rsLastHeardTimeout, MAX_TIME_LAST_HEARD);
    server->rsLastHeardTimeoutCnt = 0;

    return 1;
}

uint16
registrarServerGetChecksum(RegistrarServer server) {
    if (server == NULL) {
        logDebug("server is NULL, returned checksum is not valid");
        return (uint16) -1;
    }

    return checksumFold(server->rsChecksum);
}

PoolElement
registrarServerGetPoolElementById(RegistrarServer server, uint32 pelementId) {
    PoolElement pelement = NULL;
    PoolElement peList = NULL;

    if (server == NULL) {
        logDebug("server is NULL");
        return NULL;
    }

    peList = server->rsPeList;

    for (pelement = peList; pelement; pelement = pelement->serverNext) {
        if (pelement->peIdentifier == pelementId) {
            return pelement;
        }
    }

    return NULL;
}

void
registrarServerPrint(RegistrarServer server) {
    PoolElement pelement;

    logDebug("--- Server 0x%08x ---", server->rsIdentifier);
    logDebug("rsAsapPort:    %d", server->rsAsapPort);
    logDebug("rsAsapAddr:");
    addressListPrint(server->rsAsapAddr);
    logDebug("rsEnrpPort:    %d", server->rsEnrpPort);
    logDebug("rsEnrpAddr:");
    addressListPrint(server->rsEnrpAddr);

    for (pelement = server->rsPeList; pelement; pelement = pelement->serverNext) {
       poolElementPrint(pelement);
    }
}

int
registrarServerDelete(RegistrarServer server) {
    logDebug("deleting registrar server 0x%08x", server->rsIdentifier);

    if (server == NULL) {
        logDebug("server is NULL");
        return -1;
    }

    if (server->rsPeList != NULL) {
        logDebug("rsPeList is not NULL");
        return -1;
    }

    if (server->rsLastHeardTimeout != NULL) {
        logDebug("deleting timer");
        timerStop(server->rsLastHeardTimeout);
        timerDelete(server->rsLastHeardTimeout);
    }

    memset(server, 0, sizeof(*server));
    free(server);

    return 1;
}

int
registrarServerListAddServer(RegistrarServer newRegistrarServer) {
    if (!newRegistrarServer) {
        logDebug("newRegistrarServer is NULL\n");
        return -1;
    }

    /* check if already in registrarServerList */
    if (registrarServerListGetServerByServerId(newRegistrarServer->rsIdentifier) != NULL) {
        logDebug("server with id 0x%08x is already in list, can not add it\n", newRegistrarServer->rsIdentifier);
        return -1;
    }

    newRegistrarServer->prev = NULL;
    newRegistrarServer->next = registrarServerList;
    if (registrarServerList) {
        registrarServerList->prev = newRegistrarServer;
    }
    registrarServerList = newRegistrarServer;

    logDebug("added 0x%08x to list", newRegistrarServer->rsIdentifier);
    return 1;
}

int
registrarServerListRemoveServer(uint32 serverId) {
    RegistrarServer server;

    /* check if not in registrarServerList */
    if ((server = registrarServerListGetServerByServerId(serverId)) == NULL) {
        logDebug("0x%08x is not in list, can not delete it", serverId);
        return -1;
    }

    if (server->prev) {
        server->prev->next = server->next;
    }
    if (server->next) {
        server->next->prev = server->prev;
    }
    if (registrarServerList == server) {
        registrarServerList = server->next;
    }

    registrarServerDelete(server);

    logDebug("removed 0x%08x from list", serverId);
    return 1;
}

void
registrarServerListPrint() {
    RegistrarServer server = NULL;

    logDebug("###### Registrar Server List ######");
    for (server = registrarServerList; server; server = server->next) {
        registrarServerPrint(server);
    }
    logDebug("###################################");
}

RegistrarServer
registrarServerListGetServerByServerId(uint32 serverId) {
    RegistrarServer server = NULL;

    for (server = registrarServerList; server; server = server->next) {
        if (server->rsIdentifier == serverId) {
            return server;
        }
    }

    return NULL;
}

RegistrarServer
registrarServerListGetServerByAssocId(sctp_assoc_t assocId) {
    RegistrarServer server = NULL;

    for (server = registrarServerList; server; server = server->next) {
        if (server->rsEnrpAssocId == assocId) {
            return server;
        }
    }

    return NULL;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.19  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.18  2007/10/27 12:44:27  volkmer
 * removed debug macro
 *
 **/
