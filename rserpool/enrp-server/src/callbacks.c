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
 * $Id: callbacks.c,v 1.2 2008-02-14 10:21:39 volkmer Exp $
 *
 **/
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "callbacks.h"
#include "debug.h"
#include "main.h"
#include "backend.h"
#include "enrp.h"
#include "asap.h"
#include "util.h"
#include "registrarserver.h"
#include "takeover.h"

/* TODO:
 * on shutdown or abort even: delete peer
 * on comm up: look for peer in list
 * on peer addr change: edit peer addr list
 * on send failed: resend msg in buffer
 * on remote error: delete peer
 * on abort: delete peer
 */

uint32 assocs[10];
int init = 0;

void
put(uint32 id) {
    int i = 0;

    for (; i < 10; i++) {
        if (assocs[i] == 0) {
            assocs[i] = id;
            break;
        }

    }

/*
    for (i = 0; i < 10; i++)
        if (assocs[i] != 0)
            printf("%d - %d\n", i, assocs[i]);
*/
}

void
handleSctpEvent(void *buf, size_t len) {
    union sctp_notification *sn;
    struct sctp_assoc_change *sac;
    struct sctp_send_failed *ssf;
    struct sctp_paddr_change *spc;
/*    struct sctp_shutdown_event *sse; */
    struct sockaddr_in *in4;
    struct sockaddr_in6 *in6;
    char *assocStr = NULL;
    char addrBuf[INET6_ADDRSTRLEN];

    if (init == 0) {
        memset(assocs, 0, 32);
        init = 1;
    }

    sn = (union sctp_notification *)buf;
    switch (sn->sn_header.sn_type) {
    case SCTP_ASSOC_CHANGE:
        sac = &sn->sn_assoc_change;
        logDebug("assoc change");
        logDebug("assocId: %d", sac->sac_assoc_id);


        put(sac->sac_assoc_id);


        switch(sac->sac_state) {
        case SCTP_COMM_UP:
            assocStr = "SCTP_COMM_UP";
            break;
        case SCTP_COMM_LOST:
            assocStr = "SCTP_COMM_LOST";
            break;
        case SCTP_RESTART:
            assocStr = "SCTP_RESTART";
            break;
        case SCTP_SHUTDOWN_COMP:
            assocStr = "SCTP_SHUTDOWN_COMP";
            break;
        case SCTP_CANT_STR_ASSOC:
            assocStr = "SCTP_CANT_STR_ASSOC";
        }
        logDebug("state: %s", assocStr);
        logDebug("error: %hu", sac->sac_error);
        logDebug("instr: %hu", sac->sac_inbound_streams);
        logDebug("outstr: %hu",  sac->sac_outbound_streams);
        break;

    case SCTP_SEND_FAILED:
        ssf = &sn->sn_send_failed;
        logDebug("send failed");
        logDebug("length: %hu", ssf->ssf_length);
        logDebug("error: %hu", ssf->ssf_error);
        break;

    case SCTP_PEER_ADDR_CHANGE:
        spc = &sn->sn_paddr_change;
        logDebug("peer address change");
        if (spc->spc_aaddr.ss_family == AF_INET) {
            in4 = (struct sockaddr_in *) &spc->spc_aaddr;
            logDebug("%s", inet_ntop(AF_INET, &in4->sin_addr, addrBuf, INET6_ADDRSTRLEN));
        } else {
            in6 = (struct sockaddr_in6 *) &spc->spc_aaddr;
            logDebug("%s", inet_ntop(AF_INET6, &in6->sin6_addr, addrBuf, INET6_ADDRSTRLEN));
        }
        logDebug("state: %d", spc->spc_state);
        logDebug("error: %d", spc->spc_error);
        break;

    case SCTP_SHUTDOWN_EVENT:
        logDebug("shutdown event");
        break;

    default:
        logDebug("unsupported event type: %hu", sn->sn_header.sn_type);
        exit(-1);
    }

    return;
}

void
enrpSctpCallback(void *arg) {
    ssize_t len;
    char buf[BUFFER_SIZE];
    char addrBuf[INET6_ADDRSTRLEN];
    struct sctp_sndrcvinfo sinfo;
    struct sockaddr_in6 peer;
    socklen_t peerLen;
    uint8 type;
    int flags = 0;
    sctp_assoc_t assocId;

    memset(buf, 0,  sizeof(buf));
    memset(addrBuf, 0, sizeof(addrBuf));
    memset(&peer, 0, sizeof(peer));
    memset(&sinfo, 0, sizeof(sinfo));

    peerLen = sizeof(peer);

    len = sctp_recvmsg(enrpSctpFd,
                       (void *) &buf, sizeof(buf),
                       (struct sockaddr *) &peer, &peerLen,
                       (struct sctp_sndrcvinfo *) &sinfo, &flags);

    if (flags & MSG_NOTIFICATION) {
        logDebug("received a notification");
        handleSctpEvent(buf, len);
        return;
    }

    assocId = sinfo.sinfo_assoc_id;

    type = checkAndIdentifyMsg(buf, len, assocId);
    if (type == 0) {
        logDebug("received msg is not valid");
        return;
    }

    if (peer.sin6_family == AF_INET6)
        inet_ntop(AF_INET6, (void *) &peer.sin6_addr, addrBuf, sizeof(addrBuf));
    else if (peer.sin6_family == AF_INET)
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &peer)->sin_addr, addrBuf, sizeof(addrBuf));

    logDebug("received from:     %s:%d", addrBuf, ntohs(peer.sin6_port));
    logDebug("received length:   %d", (int)len);

    switch(type) {
    case ENRP_PRESENCE:
        logDebug("Got Presence Message on assoc Id: %d", assocId);
        handleEnrpPresence(buf, len, assocId);
        break;

    case ENRP_HANDLE_TABLE_REQUEST:
        logDebug("Got Handle Table Request Message on assoc Id: %d", assocId);
        handleEnrpHandleTableRequest(buf, len, assocId);
        break;

    case ENRP_HANDLE_TABLE_RESPONSE:
        logDebug("Got Handle Table Response Message on assoc Id: %d", assocId);
        handleEnrpHandleTableResponse(buf, len, assocId);
        break;

    case ENRP_HANDLE_UPDATE:
        logDebug("Got Handle Update Message on assoc Id: %d", assocId);
        handleEnrpHandleUpdate(buf, len, assocId);
        break;

    case ENRP_LIST_REQUEST:
        logDebug("Got List Request Message on assoc Id: %d", assocId);
        handleEnrpListRequest(buf, len, assocId);
        break;

    case ENRP_LIST_RESPONSE:
        logDebug("Got List Response Message on assoc Id: %d", assocId);
        handleEnrpListResponse(buf, len, assocId);
        break;

    case ENRP_INIT_TAKEOVER:
        logDebug("Got Init Takeover Message on assoc Id: %d", assocId);
        handleEnrpInitTakeover(buf, len, assocId);
        break;

    case ENRP_INIT_TAKEOVER_ACK:
        logDebug("Got Init Takeover ACK Message on assoc Id: %d", assocId);
        handleEnrpInitTakeoverAck(buf, len, assocId);
        break;

    case ENRP_TAKEOVER_SERVER:
        logDebug("Got Takeover Message on assoc Id: %d", assocId);
        handleEnrpTakeoverServer(buf, len, assocId);
        break;

    case ENRP_ERROR:
        logDebug("Got Error Message on assoc Id: %d", assocId);
        logDebug("This Message Type is not supported yet!!");
        break;

    default:
        logDebug("Got %d Message Type on assoc Id: %d", type, assocId);
        printBuf(buf, len, "unknown message buffer");
        logDebug("This Message Type is not supported!!");
    }

    return;
}

void
enrpUdpCallback(void *arg) {
    struct msgHeader *hdr;
    ssize_t len;
    uint8 type;
    char buf[BUFFER_SIZE];
    char addrBuf[INET6_ADDRSTRLEN];
    struct sockaddr_in6 peer;
    socklen_t peerLen;

    memset(buf, 0, sizeof(buf));
    peerLen = sizeof(struct sockaddr_in6);
    len = recvfrom(enrpUdpFd, (void *)buf, sizeof(buf), 0, (struct sockaddr *) &peer, &peerLen);

    if (len < 4) {
        logDebug("msg was to short");
        return;
    }

    hdr = (struct msgHeader *) buf;
    type = hdr->type;

    if (peer.sin6_family == AF_INET6)
        inet_ntop(AF_INET6, (void *) &peer.sin6_addr, addrBuf, sizeof(addrBuf));
    else if (peer.sin6_family == AF_INET)
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &peer)->sin_addr, addrBuf, sizeof(addrBuf));
    logDebug("got message from: %s:%d", addrBuf, ntohs(peer.sin6_port));

    switch(type) {
    case ENRP_PRESENCE:
        handleEnrpPresence(buf, len, 0);
        break;

    case ENRP_HANDLE_TABLE_REQUEST:
        logDebug("discarding handle table request message via multicast");
        break;

    case ENRP_HANDLE_TABLE_RESPONSE:
        logDebug("discarding handle table response message via multicast");
        break;

    case ENRP_HANDLE_UPDATE:
        logDebug("discarding handle table update message via multicast");
        break;

    case ENRP_LIST_REQUEST:
        logDebug("discarding list request message via multicast");
        break;

    case ENRP_LIST_RESPONSE:
        logDebug("discarding list response message via multicast");
        break;

    case ENRP_INIT_TAKEOVER:
        handleEnrpInitTakeover(buf, len, 0);
        break;

    case ENRP_INIT_TAKEOVER_ACK:
        handleEnrpInitTakeoverAck(buf, len, 0);
        break;

    case ENRP_TAKEOVER_SERVER:
        handleEnrpTakeoverServer(buf, len, 0);
        break;

    case ENRP_ERROR:
        logDebug("discarding error message via multicast");
        break;

    default:
        logDebug("discarding unknown message %d via multicast", type);
    }

    return;
}

void
serverHuntTimeout(void *arg) {
    if (registrarState == INITIALIZING) {
        logDebug("initializing mode");
        if (serverHuntCnt < MAX_NUMBER_SERVER_HUNT) {
            sendEnrpPresence(0, 1, 0);
            timerStart(serverHuntTimer, TIMEOUT_SERVER_HUNT);
            serverHuntCnt++;
            return;
        }

        enterServicingMode();
        return;
    }

    logDebug("this statement should not have been reached, aborting");
    exit(-1);

    return;
}

void
enrpAnnounceTimeout(void *arg) {
    /* normal peer announcment */
    logDebug("sending peer announcement");
    sendEnrpPresence(0, 1, 0);

    /* restart timer */
    logDebug("restarting announce timer");
    timerStart(enrpAnnounceTimer, PEER_HEARTBEAT_CYCLE);

    return;
}

void
peerLastHeardTimeout(void *arg) {
    uint32 *serverId = (uint32 *) arg;
    RegistrarServer server = NULL;
    Takeover takeover;

    server = registrarServerListGetServerByServerId(*serverId);
    if (server == NULL) {
        logDebug("callback without server");
        return;
    }

    logDebug("peer 0x%08x timed out, timeout counter is %d", server->rsIdentifier, server->rsLastHeardTimeoutCnt);

    server->rsLastHeardTimeoutCnt++;
    timerStart(server->rsLastHeardTimeout, MAX_TIME_LAST_HEARD);

    if (server->rsLastHeardTimeoutCnt == MAX_TIME_NO_RESPONSE) {
        if (server->rsActive == 1) {
            logDebug("peer 0x%08x timed out, initiating takeover process", server->rsIdentifier);
            takeover = takeoverNew(server->rsIdentifier);
            sendEnrpInitTakeover(0, server->rsIdentifier, 0);
        } else
            logDebug("server is already inactive");
    }

    sendEnrpPresence(server->rsIdentifier, 1, server->rsEnrpAssocId);

    return;
}

void
asapSctpCallback(void *arg) {
    ssize_t len;
    char buf[BUFFER_SIZE];
    char addrBuf[INET6_ADDRSTRLEN];
    struct sctp_sndrcvinfo sinfo;
    struct sockaddr_in6 peer;
    struct msgHeader *hdr;
    socklen_t peerLen;
    uint8 type;
    int flags = 0;
    sctp_assoc_t assocId;

    memset(buf, 0,  sizeof(buf));
    memset(addrBuf, 0, sizeof(addrBuf));
    memset(&peer, 0, sizeof(peer));
    memset(&sinfo, 0, sizeof(sinfo));

    peerLen = sizeof(peer);

    len = sctp_recvmsg(asapSctpFd,
                       (void *) &buf, sizeof(buf),
                       (struct sockaddr *) &peer, &peerLen,
                       (struct sctp_sndrcvinfo *) &sinfo, &flags);

    if (flags & MSG_NOTIFICATION) {
        logDebug("received a notification");
        handleSctpEvent(buf, len);

        return;
    }

    assocId = sinfo.sinfo_assoc_id;
    
    if (len < 4) {
        logDebug("message is too short");
        
        return;
    }
    
    hdr = (struct msgHeader *) buf;
    type = hdr->type;

    if (peer.sin6_family == AF_INET6)
        inet_ntop(AF_INET6, (void *) &peer.sin6_addr, addrBuf, sizeof(addrBuf));
    else if (peer.sin6_family == AF_INET)
        inet_ntop(AF_INET, (void *) &((struct sockaddr_in *) &peer)->sin_addr, addrBuf, sizeof(addrBuf));

    logDebug("received from:     %s:%d", addrBuf, ntohs(peer.sin6_port));
    logDebug("received length:   %d", (int) len);

    switch(type) {
    case ASAP_REGISTRATION:
        logDebug("got asap registration on assoc id: %d", assocId);
        handleAsapRegistration(buf, len, assocId);
        break;

    case ASAP_DEREGISTRATION:
        logDebug("got asap deregistration on assoc id: %d", assocId);
        handleAsapDeregistration(buf, len, assocId);
        break;

    case ASAP_ENDPOINT_KEEP_ALIVE:
        logDebug("got asap endpoint keep alive ack on assoc id: %d", assocId);
        handleAsapEndpointKeepAliveAck(buf, len, assocId);
        break;

    case ASAP_ENDPOINT_UNREACHABLE:
        logDebug("got asap endpoint unreachable on assoc id: %d", assocId);
        handleAsapEndpointUnreachable(buf, len, assocId);
        break;

    case ASAP_HANDLE_RESOLUTION:
        logDebug("got asap handle resolution on assoc id: %d", assocId);
        handleAsapHandleResolution(buf, len, assocId);
        break;

    case ASAP_ERROR:
        logDebug("got asap error on assoc id: %d", assocId);
        printBuf(buf, len, "asap error receive buffer");
        break;

    default:
        logDebug("got unknown or unsupported %d message type on assoc id: %d", type, assocId);
        printBuf(buf, len, "unknown message buffer");
        logDebug("This Message Type is not supported!!");
    }

    return;
}

void
asapAnnounceTimeout(void *arg) {
	logDebug("sending asap announce timeout");
	sendAsapServerAnnounce();
	timerStart(asapAnnounceTimer, ASAP_SERVER_ANNOUNCE_TIMEOUT);
    
    return;
}

void
handlePoolElementLifetimeExpiry(void *arg) {
    PoolElement pelement            = (PoolElement) arg;
    ServerPool pool                 = (ServerPool) pelement->peServerPool;
    RegistrarServer registrarServer = (RegistrarServer) pelement->peRegistrarServer;

    logDebug("lifetime expired for pool element 0x%08x", pelement->peIdentifier);

    registrarServerRemovePoolElement(registrarServer, pelement);
    serverPoolRemovePoolElement(pool, pelement->peIdentifier);
}

#ifdef DEBUG
void
endTimeout(void *arg) {
    logDebug("end timer timed out, ending server");
    timerListPrint();
    takeoverListPrint();
    /* TODO:
     * registrarServerListPrint();
     * serverPoolListPrint();
     */

    exit(-1);
}
#endif /* DEBUG */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.23  2007/12/06 01:52:15  volkmer
 * moved peliftimeexpirytimeoutcallback
 *
 * Revision 1.22  2007/12/05 23:07:20  volkmer
 * refactored out a method to enter servicing mode
 *
 * Revision 1.21  2007/12/02 23:00:09  volkmer
 * init flags with 0.. had a bug with false notifications
 *
 * Revision 1.20  2007/11/19 22:40:40  volkmer
 * moved asap announce starting to server hunt callback, added asap udp callback, futher implemented asap sctp callback
 *
 * Revision 1.19  2007/11/05 00:06:05  volkmer
 * reformated the copyright statement

started working on the asapsctpcallback
 *
 * Revision 1.18  2007/10/27 13:25:58  volkmer
 * added asap callbacks
removed debug macro
 *
 **/
