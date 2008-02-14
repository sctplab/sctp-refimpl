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
 * $Id: backend.c,v 1.2 2008-02-14 15:05:00 volkmer Exp $
 *
 **/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cblib.h"
#include "debug.h"
#include "enrp.h"
#include "backend.h"
#include "callbacks.h"
#include "registrarserver.h"
#include "serverpool.h"
#include "takeover.h"


int             registrarState;
uint32          mentorServerId;
uint32          ownId;

int             useIpv6;
int             useLoopback;

int             enrpSctpFd;
int             enrpUdpFd;
int             useEnrpAnnouncements;

int             asapSctpFd;
int             asapUdpFd;
int             useAsapAnnouncements;

uint16          enrpLocalPort;
Address         enrpAnnounceAddr;
uint16          enrpAnnouncePort;
Timer           enrpAnnounceTimer;

uint16          asapLocalPort;
Address         asapAnnounceAddr;
uint16          asapAnnouncePort;
Timer           asapAnnounceTimer;

int             serverHuntCnt;
Timer           serverHuntTimer;

#ifdef DEBUG
Timer           endTimer;
#endif /* DEBUG */


int
initGlobals() {
    enrpSctpFd                       = -1;
    enrpLocalPort                    = ENRP_SCTP_PORT;

    useEnrpAnnouncements             = 1;
    enrpUdpFd                        = -1;
    enrpAnnounceAddr.type            = AF_INET;
    enrpAnnounceAddr.addr.in4.s_addr = inet_addr(ENRP_ANNOUNCE_ADDR);
    enrpAnnouncePort                 = ENRP_UDP_PORT;
    
    asapSctpFd                       = -1;
    asapLocalPort                    = ASAP_SCTP_PORT;

    useAsapAnnouncements             = 1;
    asapUdpFd                        = -1;
    asapAnnounceAddr.type            = AF_INET;
    asapAnnounceAddr.addr.in4.s_addr = inet_addr(ASAP_ANNOUNCE_ADDR);
    asapAnnouncePort                 = ASAP_UDP_PORT;

	ownId                            = 0;
    useIpv6                          = 0;
    useLoopback                      = 1;
    registrarState                   = INITIALIZING;
    mentorServerId                   = 0;
    registrarServerList              = NULL;
    serverPoolList                   = NULL;
    takeoverList                     = NULL;

#ifdef DEBUG
    endTimer                         = NULL;
#endif /* DEBUG */

    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.13  2007/12/05 23:06:42  volkmer
 * removed unneeded file descriptors
 *
 * Revision 1.12  2007/11/19 22:41:14  volkmer
 * some sorting for eye candy
 *
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:21:54  volkmer
 * added some asap adresses
 *
 **/
