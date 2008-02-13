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
 * $Id: registrarserver.h,v 1.3 2008-02-13 17:04:26 volkmer Exp $
 *
 **/
#ifndef _REGISTRARSERVER_H
#define _REGISTRARSERVER_H 1

#include "backend.h"
#include "cblib.h"
#include "poolelement.h"

typedef struct registrar_server_struct *RegistrarServer;
struct registrar_server_struct {
    RegistrarServer        next;
    RegistrarServer        prev;

    uint32                 rsIdentifier;
    uint32                 rsChecksum;
    Timer                  rsLastHeardTimeout;
    int                    rsLastHeardTimeoutCnt;
    int                    rsActive;

    uint16                 rsEnrpPort;
    Address                rsEnrpAddr[MAX_ADDR_CNT];
    sctp_assoc_t           rsEnrpAssocId;
    int                    rsUsesMultiCast;

    PoolElement            rsPeList;
    int                    rsIsSynchronizing;

    uint16                 rsAsapPort;
    Address                rsAsapAddr[MAX_ADDR_CNT];
    sctp_assoc_t           rsAsapAssocId;
};

extern RegistrarServer registrarServerList;
extern RegistrarServer this;

RegistrarServer
registrarServerNew(uint32 serverId);

void
registrarServerPrint(RegistrarServer server);

int
registrarServerAddPoolElement(RegistrarServer server, PoolElement newPoolElement);


int
registrarServerRemovePoolElement(RegistrarServer server, PoolElement newPoolElement);

int
registrarServerResetTimer(RegistrarServer server);

uint16
registrarServerGetChecksum(RegistrarServer server);

PoolElement
registrarServerGetPoolElementById(RegistrarServer server, uint32 pelementId);

int
registrarServerDelete(RegistrarServer server);

int
registrarServerListAddServer(RegistrarServer newServer);

int
registrarServerListRemoveServer(uint32 serverId);

void
registrarServerListPrint();

RegistrarServer
registrarServerListGetServerByServerId(uint32 serverId);

RegistrarServer
registrarServerListGetServerByAssocId(sctp_assoc_t assocId);

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
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:44:27  volkmer
 * removed debug macro
 *
 **/
