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
 * $Id: enrp.c,v 1.8 2008-02-14 15:10:16 volkmer Exp $
 *
 **/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include "debug.h"
#include "enrp.h"
#include "backend.h"
#include "parameters.h"
#include "checksum.h"
#include "registrarserver.h"
#include "poolelement.h"
#include "serverpool.h"
#include "takeover.h"
#include "util.h"

size_t
initEnrpPresence(struct enrpPresence *msg, uint32 receiverId) {
    msg->hdr.type = ENRP_PRESENCE;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

size_t
initEnrpListRequest(struct enrpListRequest *msg, uint32 receiverId) {
    msg->hdr.type = ENRP_LIST_REQUEST;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

size_t
initEnrpListResponse(struct enrpListResponse *msg, uint32 receiverId, int rejectBit) {
    msg->hdr.type = ENRP_LIST_RESPONSE;
    msg->hdr.flags = (rejectBit > 0) ? ENRP_LIST_RESPONSE_REJECT_BIT : 0x00;
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

size_t
initEnrpHandleTableRequest(struct enrpHandleTableRequest *msg, uint32 receiverId, int ownBit) {
    msg->hdr.type = ENRP_HANDLE_TABLE_REQUEST;
    msg->hdr.flags = (ownBit > 0) ? ENRP_HANDLE_TABLE_REQUEST_OWN_BIT : 0x00;
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

size_t
initEnrpHandleTableResponse(struct enrpHandleTableResponse *msg, uint32 receiverId, int moreBit, int rejectBit) {
    msg->hdr.type = ENRP_HANDLE_TABLE_RESPONSE;
    msg->hdr.flags = 0x00 | ((moreBit > 0) ? ENRP_HANDLE_TABLE_RESPONSE_MORE_BIT : 0x00) | ((rejectBit > 0) ? ENRP_HANDLE_TABLE_RESPONSE_REJECT_BIT : 0x00);
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

size_t
initEnrpHandleUpdate(struct enrpHandleUpdate *msg, uint32 receiverId, uint16 updateAction) {
    msg->hdr.type = ENRP_HANDLE_UPDATE;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(16);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    msg->updateAction = htons(updateAction);
    msg->reserved = 0;
    return 16;
}

size_t
initEnrpInitTakeover(struct enrpInitTakeover *msg, uint32 receiverId, uint32 targetId) {
    msg->hdr.type = ENRP_INIT_TAKEOVER;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(16);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    msg->targetServerId = htonl(targetId);
    return 16;
}

size_t
initEnrpInitTakeoverAck(struct enrpInitTakeoverAck *msg, uint32 receiverId, uint32 targetId) {
    msg->hdr.type = ENRP_INIT_TAKEOVER_ACK;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(16);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    msg->targetServerId = htonl(targetId);
    return 16;
}

size_t
initEnrpTakeoverServer(struct enrpTakeoverServer *msg, uint32 receiverId, uint32 targetId) {
    msg->hdr.type = ENRP_TAKEOVER_SERVER;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(16);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    msg->targetServerId = htonl(targetId);
    return 16;
}

size_t
initEnrpError(struct enrpError *msg, uint32 receiverId) {
    msg->hdr.type = ENRP_ERROR;
    msg->hdr.flags = 0;
    msg->hdr.length = htons(12);
    msg->senderServerId = htonl(ownId);
    msg->receiverServerId = htonl(receiverId);
    return 12;
}

int
handleEnrpMsg(void *buf, ssize_t len, sctp_assoc_t assocId) {
    logDebug("not implemented yet");

    return 1;
}

int
handleEnrpPresence(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpPresence *pres;
    struct paramPeChecksum *chkSum;
    struct paramServerInformation *srvInfo;
    RegistrarServer server = NULL;
    PoolElement pe;
    Takeover takeover = NULL;
    Address addrs[MAX_ADDR_CNT];
    char *offset;
    size_t bufLen;
    sctp_assoc_t newAssocId;
    checksumTypeFolded checksum;
    uint32 senderServerId;
    uint16 length;
    uint16 port;
    uint16 transportUse;
    uint8 protocol;
    int addrCnt = 0;
    int isNew = 0;
    int ret;
    int i;

    bufLen = len;
    offset = (char *) buf;

    pres = (struct enrpPresence *) offset;
    senderServerId = ntohl(pres->senderServerId);
    length = ntohs(pres->hdr.length);
    bufLen -= sizeof(struct enrpPresence);
    offset += sizeof(struct enrpPresence);

    chkSum = (struct paramPeChecksum *) offset;
    checksum = ntohs(chkSum->checksum);
    bufLen -= sizeof(struct paramPeChecksum);
    offset += sizeof(struct paramPeChecksum);

	if (senderServerId == ownId) {
        logDebug("discarding own presence message");
        return -1;
    }

    if (length != len) {
        logDebug("buffer lentgh does not match packet length");
        return -1;
    }

    if ((len - bufLen) > 0) {
        if (checkTLV(offset, bufLen) == PARAM_SERVER_INFORMATION) {
            srvInfo = (struct paramServerInformation *) offset;
            offset += sizeof(struct paramServerInformation);
            bufLen -= sizeof(struct paramServerInformation);

            memset(addrs, 0, sizeof(addrs));
            ret = sendBufferToTransportAddresses(offset, bufLen, addrs, &addrCnt, &port, &transportUse, &protocol);
            if (ret == -1 || protocol != IPPROTO_SCTP) {
                logDebug("msg does not contain a proper sctp transport parameter");
                return -1;
            }
            
            offset += ret;
            bufLen -= ret;
            
            if (bufLen != 0) {
                logDebug("buffer should be empty by now");
                exit(-1);
            }
        } else {
            logDebug("there was a parameter error: type = %d", checkTLV(offset, bufLen));
            /* TODO:
             * send param error
             */
        }
    } else {
        logDebug("there was a message error");
        /* TODO:
         * send msg error
         */
    }

    server = registrarServerListGetServerByServerId(senderServerId);
    if (server != NULL) {
        logDebug("found 0x%08x in peer list", senderServerId);
        registrarServerResetTimer(server);
        if (server->rsActive == 0) {
            logDebug("server is active again");
            server->rsActive = 1;
            takeover = takeoverLookup(server->rsIdentifier);
            if (takeover != NULL) {
                logDebug("deleting takeover");
                takeoverDelete(takeover);
            }
        }
        
        
        if (registrarServerGetChecksum(server) != checksum) {
        	logDebug("checksum mismatch, resyncing: server checksum: 0x%x stored checksum: 0x%x", registrarServerGetChecksum(server), checksum);
        	server->rsIsSynchronizing = 1;
        	
        	for (pe = server->rsPeList; pe; pe = pe->serverNext)
        		pe->peIsDirty = 1;
        	
        	sendEnrpHandleTableRequest(senderServerId, 1, assocId);
        }
    } else {
        logDebug("0x%08x is not in peer list", senderServerId);
        server = registrarServerNew(senderServerId);
        if (server == NULL) {
            logDebug("registrar server allocation failed");
            return -1;
        }
        registrarServerListAddServer(server);
        server->rsChecksum = 0;
        server->rsEnrpPort = port;
        for (i = 0; i < MAX_ADDR_CNT; i++) {
           server->rsEnrpAddr[i] = addrs[i];
        }
        logDebug("added 0x%08x to peer list", senderServerId);
        isNew = 1;
    }

    if (assocId == 0 && (isNew == 1 && addrCnt > 0)) {
        ret = createAssocToPeer(addrs, addrCnt, port, senderServerId, &newAssocId);
        if (ret == -1) {
            logDebug("couldn't establish an sctp assoc to 0x%08x", senderServerId);
            return -1;
        }

        server->rsEnrpAssocId = newAssocId;
    } else
        newAssocId = assocId;

    /* server hunt mode */
    if (registrarState == INITIALIZING) {
        logDebug("using  0x%08x as mentor peer", senderServerId);
        mentorServerId = senderServerId;

        enterServicingMode();

        sendEnrpPresence(mentorServerId, 1, newAssocId);
        sendEnrpListRequest(mentorServerId, newAssocId);
        sendEnrpHandleTableRequest(mentorServerId, 0, newAssocId);

        return 1;
    }

    return 1;
}

int
handleEnrpListRequest(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpListRequest *list;
    RegistrarServer server;
    uint32 senderServerId;
    int rejectBit = 0;

    logDebug("got enrp list request msg");

    list = (struct enrpListRequest *) buf;
    senderServerId = ntohl(list->senderServerId);
    if (senderServerId == ownId) {
        logDebug("discarding own list request message");
        return -1;
    }
    
    if (registrarState == INITIALIZING)
        rejectBit = 1;

    if ((server = registrarServerListGetServerByServerId(senderServerId)) != NULL)
        registrarServerResetTimer(server);
    else {
        logDebug("couldn't find server in peer list: 0x%08x", senderServerId);
        return -1;
    }

    sendEnrpListResponse(senderServerId, rejectBit, assocId);

    return 1;
}

int
handleEnrpListResponse(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpListResponse *list;
    RegistrarServer server;
    struct paramServerInformation *srvInfo;
    char *offset;
    size_t bufSize;
    Address addrs[MAX_ADDR_CNT];
    uint32 senderServerId;
    uint32 srvInfoServerId;
    uint16 port;
    uint16 transportUse;
    uint8 protocol;
    int ret;
    int addrCnt;
    int peerCnt = 0;
    int rejectBit = 0;

    logDebug("got enrp list response msg");

    list = (struct enrpListResponse *) buf;
    senderServerId = ntohl(list->senderServerId);
    if (senderServerId == ownId) {
        logDebug("discarding own list request message");
        return -1;
    }
    
    rejectBit = list->hdr.flags & ENRP_LIST_RESPONSE_REJECT_BIT;
    
    if ((server = registrarServerListGetServerByServerId(senderServerId)) != NULL)
        registrarServerResetTimer(server);
    else {
        logDebug("couldn't find server in peer list: 0x%08x", senderServerId);
        return -1;
    }

    if (rejectBit) {
        logDebug("list request from 0x%08x was rejected", senderServerId);
        
        /*
         * TODO: request from somewhere else
         */
        
        return 1;
    }

    offset = (char *) buf + sizeof(struct enrpListResponse);
    bufSize = ntohs(list->hdr.length) - sizeof(struct enrpListResponse);

    while (checkTLV(offset, bufSize) == PARAM_SERVER_INFORMATION) {
        srvInfo = (struct paramServerInformation *) offset;
        offset += sizeof(struct paramServerInformation);
        bufSize += sizeof(struct paramServerInformation);
        srvInfoServerId = ntohl(srvInfo->serverId);

        if ((server = registrarServerListGetServerByServerId(srvInfoServerId)) == NULL) {
            if (srvInfoServerId != ownId) {
                server = registrarServerNew(srvInfoServerId);
                registrarServerListAddServer(server);
            } else
                logDebug("do not add own serverid");
        }

        memset(&addrs, 0, sizeof(addrs));
        addrCnt = 0;
        port = 0;
        transportUse = 0;
        protocol = 0;

        ret = sendBufferToTransportAddresses(offset, bufSize, addrs, &addrCnt, &port, &transportUse, &protocol);
        if (ret < 0) {
            logDebug("sendbuffer to pe conversion failed, aborting");
            return -1;
        }

        if (protocol == IPPROTO_SCTP && addrCnt > 0) {
            memcpy(&server->rsEnrpAddr, addrs, sizeof(Address) * addrCnt);
        }

        peerCnt++;
    }

    return 1;
}

int
handleEnrpHandleTableRequest(void *buf, ssize_t len, sctp_assoc_t assocId) {
    int ownBit = 0;
    int rejectBit = 0;
    struct enrpHandleTableRequest *handle;
    RegistrarServer server;
    uint32 senderServerId;
    logDebug("got enrp handle table request msg");

    handle = (struct enrpHandleTableRequest *) buf;
    senderServerId = ntohl(handle->senderServerId);
    if (senderServerId == ownId) {
        logDebug("discarding own handle tabele request message");
        return -1;
    }

    if (registrarState == INITIALIZING)
        rejectBit = 1;

    if (handle->hdr.flags & ENRP_HANDLE_TABLE_REQUEST_OWN_BIT)
        ownBit = 1;

    if ((server = registrarServerListGetServerByServerId(senderServerId)) != NULL)
        registrarServerResetTimer(server);
    else {
        logDebug("couldn't find server in peer list: 0x%08x\n", senderServerId);
        return -1;
    }

    sendEnrpHandleTableResponse(senderServerId, ownBit, rejectBit, assocId);
    return 1;
}

int
handleEnrpHandleTableResponse(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpHandleTableResponse *handle;
    RegistrarServer server;
    PoolElement pelement;
    struct paramPoolHandle *phandle;
    ServerPool pool;
    char *offset;
    size_t bufSize;
    char poolhandle[POOLHANDLE_SIZE + 1];
    int length;
    int moreBit;
    int rejectBit;
    uint32 senderServerId;

    logDebug("got enrp handle table response msg");

    handle = (struct enrpHandleTableResponse *) buf;
    senderServerId = ntohl(handle->senderServerId);
    if (senderServerId == ownId) {
        logDebug("discarding own handle tabele request message");
        return -1;
    }

    rejectBit = handle->hdr.flags & ENRP_HANDLE_TABLE_RESPONSE_REJECT_BIT;
    moreBit = handle->hdr.flags & ENRP_HANDLE_TABLE_RESPONSE_MORE_BIT;

    if (rejectBit)  {
        logDebug("handle table request was rejected");
        /* TODO
         * request from somewhere else
         */
        
        return -1;
    }

    if ((server = registrarServerListGetServerByServerId(senderServerId)) != NULL)
        registrarServerResetTimer(server);
    else {
        logDebug("couldn't find server in peer list: 0x%08x", senderServerId);
        return -1;
    }

    offset = (char *) buf + sizeof(struct enrpHandleTableResponse);
    bufSize = len;

    memset(poolhandle, 0, sizeof(poolhandle));

    while (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolhandle, ((char *) phandle + 4), length - 4);
        if (length % 4 != 0)
            length += 4 - (length % 4);
        offset += length;
        bufSize -= length;

        if ((pool = serverPoolListGetPool(poolhandle)) == NULL) {
            pool = serverPoolNew(poolhandle);
            serverPoolListAddPool(pool);
        }

        while (checkTLV(offset, bufSize) == PARAM_POOL_ELEMENT) {
            pelement = NULL;
            length = sendBufferToPoolElement(offset, bufSize, &pelement);

            if (length < 0) {
                logDebug("pool element conversion failed, aborting");
                return -1;
            }
            
            offset += length;
            bufSize -= length;

            if (pelement != NULL) {
                if (serverPoolGetPoolElementById(pool, pelement->peIdentifier) == NULL) {
                    serverPoolAddPoolElement(pool, pelement);
                    if ((server = registrarServerListGetServerByServerId(senderServerId)) != NULL) {
                    	/*
                    	 * TODO:
                    	 * if server has pe
                    	 * if server is synchronizing
                    	 * 
                    	 * 
                    	 * move information into pe in db and remove isdirty flag
                    	 * mergePe function?   can not overwrite because of assocId and 
                    	 * list management information
                    	 * 
                    	 * at end of function: if not
                    	 * 
                    	 * global list of syncs?
                    	 */
                    
                        registrarServerAddPoolElement(server, pelement);
                        logDebug("added pool element 0x%08x to server 0x%08x", pelement->peIdentifier, server->rsIdentifier);
                    }
                    else {
                        logDebug("registrar is unknown?!");
                    }
                }
                else {
                    poolElementDelete(pelement);
                }
            }
            else {
                return 0;
            }
        }
    }
    serverPoolListPrint();
    
/* TODO: handle space sync
 * 
 * 
 *     if (moreBit) {
    	logDebug("requesting more handle table informations");
    	sendEnrpHandleTableRequest(senderServerId, 0, assocId);
    } else {
    	what??
    }

 */   

    return 1;
}

int
handleEnrpHandleUpdate(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpHandleUpdate *hupdate;
    struct paramPoolHandle *phandle;
    RegistrarServer senderServer;
    RegistrarServer newPoolElementServer;
    RegistrarServer existingPoolElementServer;
    PoolElement newPoolElement = NULL;
    PoolElement existingPoolElement;
    ServerPool pool;
    uint16 updateAction;
    char *offset;
    size_t bufSize;
    char poolhandle[POOLHANDLE_SIZE + 1];
    uint16 length;
    uint32 senderServerId;

    logDebug("got enrp handle update msg");

    hupdate = (struct enrpHandleUpdate *) buf;
    senderServerId = ntohl(hupdate->senderServerId);
    updateAction   = ntohs(hupdate->updateAction);
    if (senderServerId == ownId) {
        logDebug("discarding own handle update message");
        return -1;
    }

    if ((senderServer = registrarServerListGetServerByServerId(senderServerId)) != NULL)
        registrarServerResetTimer(senderServer);
    else {
        logDebug("couldn't find server in peer list: 0x%08x", senderServerId);
        return -1;
    }

    offset = (char *) buf + sizeof (*hupdate);
    bufSize = len - sizeof (*hupdate);

    memset(poolhandle, 0, sizeof(poolhandle));

    if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolhandle, ((char *) phandle + 4), length - 4);
        if ((length % 4) != 0)
            length += 4 - (length % 4);
        offset += length;
        bufSize -= length;
        pool = serverPoolListGetPool(poolhandle);
        if (pool == NULL) {
            pool = serverPoolNew(poolhandle);
            serverPoolListAddPool(pool);
        }

        if (checkTLV(offset, bufSize) == PARAM_POOL_ELEMENT) {
            length = sendBufferToPoolElement(offset, bufSize, &newPoolElement);
            
            if (length <= 0) {
                logDebug("pool element converison failed, aborting");
                return -1;
            }
        }
    } else {
        logDebug("pool handle parameter not found, aborting");
        
        return -1;
    }

    if (newPoolElement == NULL) {
        logDebug("bad handle update message");
        return -1;
    }


    serverPoolListPrint();
    registrarServerListPrint();


    newPoolElementServer = registrarServerListGetServerByServerId(newPoolElement->peHomeRegistrarServer);
    existingPoolElement = serverPoolGetPoolElementById(pool, newPoolElement->peIdentifier);
    if (existingPoolElement) {
        existingPoolElementServer = registrarServerListGetServerByServerId(existingPoolElement->peHomeRegistrarServer);
    }
    else {
        existingPoolElementServer = NULL;
    }

    if ((existingPoolElement) &&
        ((updateAction == ENRP_UPDATE_ACTION_DEL_PE) ||
         (updateAction == ENRP_UPDATE_ACTION_ADD_PE))) {
        if (existingPoolElementServer) {
            registrarServerRemovePoolElement(existingPoolElementServer, existingPoolElement);
        }
        serverPoolRemovePoolElement(pool, existingPoolElement->peIdentifier);
        existingPoolElement = NULL;
    }
    if (updateAction == ENRP_UPDATE_ACTION_ADD_PE) {
        serverPoolAddPoolElement(pool, newPoolElement);
        if (newPoolElementServer) {
            registrarServerAddPoolElement(newPoolElementServer, newPoolElement);
        }
        newPoolElement = NULL;
    }
    else if (updateAction != ENRP_UPDATE_ACTION_DEL_PE) {
        logDebug("bad update action");
    }

    if (newPoolElement) {
        poolElementDelete(newPoolElement);
    }

    serverPoolListPrint();
    return 0;
}

int
handleEnrpInitTakeover(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpInitTakeover *msg;
    RegistrarServer targetServer = NULL;
    Takeover takeover;
    int sendAck = 0;
    uint32 senderServerId;
    uint32 targetServerId;
    uint32 receiverServerId;

    logDebug("got enrp init takeover msg");

    msg = (struct enrpInitTakeover *) buf;
    senderServerId = ntohl(msg->senderServerId);
    targetServerId = ntohl(msg->targetServerId);
    receiverServerId = ntohl(msg->receiverServerId);

    logDebug("senderServerId: 0x%08x", senderServerId);
    logDebug("targetServerId: 0x%08x", targetServerId);

    if (senderServerId == ownId) {
        logDebug("discarding own init takeover message");
        return -1;
    }

    if (targetServerId == ownId) {
        logDebug("somebody is trying to overtake us");
        sendEnrpPresence(0, 0, 0);
        return -1;
    }

    takeover = takeoverLookup(targetServerId);

    if (takeover == NULL)
        sendAck = 1;
    else {
        if (senderServerId < ownId)
            sendAck = 1;
        else
            sendAck = 0;
    }

    if (sendAck) {
        /* mark server inactive */
        targetServer = registrarServerListGetServerByServerId(targetServerId);
        if (targetServer != NULL) {
            logDebug("marking 0x%08x as not active", targetServer->rsIdentifier);
            targetServer->rsActive = 0;

            /* send takeover init ack */
            sendEnrpInitTakeoverAck(receiverServerId, targetServerId, assocId);
        } else
            logDebug("targetserver 0x%08x is not in peer list", targetServerId);
    } else
        logDebug("ignoring init takeover message");

    return 1;
}

int
handleEnrpInitTakeoverAck(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpInitTakeoverAck *msg;
    Takeover takeover;
    uint32 senderServerId;
    uint32 targetServerId;

    logDebug("got enrp init takeover ack msg");

    msg = (struct enrpInitTakeoverAck *) buf;
    senderServerId = ntohl(msg->senderServerId);
    targetServerId = ntohl(msg->targetServerId);
    logDebug("senderServerId: 0x%08x", senderServerId);
    logDebug("targetServerId: 0x%08x", targetServerId);

    if (senderServerId == ownId) {
        logDebug("discarding own init takeover ack message!");
        return -1;
    }

    if (targetServerId == ownId) {
        logDebug("abort takeover by sending presence");
        sendEnrpPresence(0, 1, 0);
        return 1;
    }

    takeover = takeoverLookup(targetServerId);
    if (takeover == NULL) {
        logDebug("unknown takeover target");
        return -1;
    }

    takeoverAckServer(takeover, senderServerId);

    return 1;
}

int
handleEnrpTakeoverServer(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct enrpTakeoverServer *msg;
    RegistrarServer targetServer = NULL;
    RegistrarServer takeoverServer = NULL;
    PoolElement element;
    uint32 senderServerId;
    uint32 targetServerId;

    logDebug("got enrp takeover server msg");

    msg = (struct enrpTakeoverServer *) buf;
    senderServerId = ntohl(msg->senderServerId);
    targetServerId = ntohl(msg->targetServerId);
    logDebug("senderServerId: 0x%08x", senderServerId);
    logDebug("targetServerId: 0x%08x", targetServerId);

    if (senderServerId == ownId) {
        logDebug("discarding own takeover server message!");
        return -1;
    }

    if (targetServerId == ownId) {
        logDebug("abort takeover by sending presence");
        sendEnrpPresence(0, 1, 0);
        return 1;
    }

    targetServer = registrarServerListGetServerByServerId(targetServerId);
    if (targetServer == NULL) {
        logDebug("target server 0x%08x is not in list", targetServerId);
        return -1;
    }

    takeoverServer = registrarServerListGetServerByServerId(senderServerId);
    if (takeoverServer == NULL) {
        logDebug("takeover server 0x%08x is not in list", senderServerId);
        return -1;
    }

    element = targetServer->rsPeList;
    logDebug("moving all pool elements from 0x%08x to 0x%08x", targetServer->rsIdentifier, takeoverServer->rsIdentifier);
    for (; element != NULL; element = element->serverNext) {
        registrarServerRemovePoolElement(targetServer, element);
        registrarServerAddPoolElement(takeoverServer, element);
    }

    registrarServerListRemoveServer(targetServer->rsIdentifier);
    registrarServerDelete(targetServer);

    return 1;
}

int
sendEnrpMsg(char *buf, size_t len, sctp_assoc_t assocId, int udpAnnounc) {
    int sentLen = 0;
    struct sctp_sndrcvinfo sinfo;

    if (!buf) {
        logDebug("buf is NULL");
        return -1;
    }

    if (len < 12) {
        logDebug("len is too small %d, aborting", (int) len);
        return -1;
    }

    if (udpAnnounc || (assocId == 0)) {
        logDebug("sending as announcement");
        return sendEnrpAnnounceMsg(buf, len, udpAnnounc);
    }

    /* TODO:
     * verify associd
     * against what?
     * assume its valid
     */

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.sinfo_assoc_id = assocId;
    sinfo.sinfo_ppid = htonl(ENRP_SCTP_PPID);

    if ((sentLen = sctp_send(enrpSctpFd, (void *)buf, len, &sinfo,
#ifdef MSG_NOSIGNAL
       MSG_NOSIGNAL
#else
       0
#endif
    )) < 0) {
        logDebug("sctp_send to %d failed", assocId);
        perror("sctp_send");
        return -1;
    }

    logDebug("sent msg via associd %d. sctp_send returned: %d", assocId, sentLen);
    return sentLen;
}

ssize_t
sendEnrpAnnounceMsg(char *buf, size_t msgLen, int udpAnnounce) {
    RegistrarServer server;
    struct enrpMsgHeader *hdr;
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    struct sctp_sndrcvinfo sinfo;
    ssize_t sentLen = -1;

	if (udpAnnounce) {
		logDebug("sending as udp announcement");
		switch (enrpAnnounceAddr.type) {
		case AF_INET:
			memset(&in4, 0, sizeof(in4));
			in4.sin_family = AF_INET;
			in4.sin_addr = enrpAnnounceAddr.addr.in4;
			in4.sin_port = htons(enrpAnnouncePort);
	#ifdef HAVE_SIN_LEN
			in4.sin_len = sizeof(struct sockaddr_in);
	#endif

			if ((sentLen = sendto(enrpUdpFd, (const void *)buf, msgLen, 0, (const struct sockaddr *) &in4, sizeof(struct sockaddr_in))) < 0)
				perror("sendto: ipv4 send presence announcment failed");
			break;

		case AF_INET6:
			memset(&in6, 0, sizeof(in6));
			in6.sin6_family = AF_INET6;
			in6.sin6_addr = enrpAnnounceAddr.addr.in6;
			in6.sin6_port = htons(enrpAnnouncePort);
	#ifdef HAVE_SIN6_LEN
			in6.sin6_len = sizeof(struct sockaddr_in6);
	#endif

			if ((sentLen = sendto(enrpUdpFd, (const void *)buf, msgLen, 0, (const struct sockaddr *) &in6, sizeof(struct sockaddr_in6))) < 0)
				perror("sendto: ipv6 send presence announcment failed");
			break;

		default:
			logDebug("enrp announce addr is uninitialized! EXIT!!!");
			exit(-1);
		}
	}

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.sinfo_ppid = htonl(ENRP_SCTP_PPID);

    for (server = registrarServerList; server; server = server->next) {
        if (server->rsIdentifier == ownId)
            continue;

        hdr = (struct enrpMsgHeader *) buf;
        hdr->receiverServerId = htonl(server->rsIdentifier);
        logDebug("try to send presence to 0x%08x via %d", server->rsIdentifier, server->rsEnrpAssocId );
        sinfo.sinfo_assoc_id = server->rsEnrpAssocId;

        if ((sentLen = sctp_send(enrpSctpFd, (const void *) buf, msgLen, &sinfo,
#ifdef MSG_NOSIGNAL
           MSG_NOSIGNAL
#else
           0
#endif
        )) < 0)
            perror("sctp_send: send presence announcment failed");
    }

    return sentLen;
}

ssize_t
sendEnrpMsgToAll(char *buf, size_t msgLen) {
    RegistrarServer server;
    struct enrpMsgHeader *hdr;
    struct sctp_sndrcvinfo sinfo;
    ssize_t sentLen = -1;
    int i, addrCnt;
    char addrBuf[MAX_ADDR_CNT * sizeof(struct sockaddr_in6)];

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.sinfo_ppid = htonl(ENRP_SCTP_PPID);

    for (server = registrarServerList; server; server = server->next) {
        if (server->rsIdentifier == ownId)
            continue;

        hdr = (struct enrpMsgHeader *) buf;
        hdr->receiverServerId = htonl(server->rsIdentifier);
        logDebug("try to send presence to 0x%08x", server->rsIdentifier);
        sinfo.sinfo_assoc_id = server->rsEnrpAssocId;

        addrCnt = 0;
        for (i = 0; i < MAX_ADDR_CNT; i++) {
           if ((server->rsEnrpAddr[i].type == AF_INET) ||
               (server->rsEnrpAddr[i].type == AF_INET6)) {
               addrCnt++;
           }
        }
        if (addrCnt < 1) {
           logDebug("no ENRP address?");
           continue;
        }


        if (addressesToSysCallBuffer((Address*)&server->rsEnrpAddr, server->rsEnrpPort, addrCnt, addrBuf) != addrCnt) {
           logDebug("address parsing failed, abort");
           continue;
        }

        if ((sentLen = sctp_sendx(enrpSctpFd, (void *) buf, msgLen,
                                  (struct sockaddr *)&addrBuf, addrCnt, &sinfo,
#ifdef MSG_NOSIGNAL
                                  MSG_NOSIGNAL
#else
                                  0
#endif
        )) < 0) {
            perror("sctp_sendx");
        }
    }

    return sentLen;
}

int
sendEnrpPresence(uint32 receiverId, int sendInfo, sctp_assoc_t assocId) {
    struct enrpPresence *pres;
    struct paramPeChecksum *chkSum;
    struct paramServerInformation *srvInfo;
    char buf[BUFFER_SIZE];
    char *offset;
    size_t msgLen, sentLen;
    int length = 0;
	int oldLen = 0;

    logDebug("receiverId: 0x%08x, sendInfo: %d, assocId: %d", receiverId, sendInfo, assocId);

    memset(buf, 0, BUFFER_SIZE);
    offset = buf;

    pres = (struct enrpPresence *) offset;
    initEnrpPresence(pres, receiverId);
    offset += sizeof(*pres);

    chkSum = (struct paramPeChecksum *) offset;
    length = initParamPeChecksum(chkSum, registrarServerGetChecksum(this));
    
	oldLen = htons(ntohs(pres->hdr.length) + length);
	pres->hdr.length = oldLen;
    offset += sizeof(*chkSum);

    if (sendInfo) {
        srvInfo = (struct paramServerInformation *) offset;
        initParamServerInformation(srvInfo, ownId);
        offset += sizeof(*srvInfo);

        length = transportAddressesToSendBuffer(offset, this->rsEnrpAddr, this->rsEnrpPort, TRANSPORT_USE_DATA_ONLY, IPPROTO_SCTP);

        if (length == -1) {
            logDebug("address to buffer conversion failed, omitting server information parameter");
            offset = buf + oldLen;
        } else {
            offset += length;
            srvInfo->hdr.length = htons((uint16) length + ntohs(srvInfo->hdr.length));
            pres->hdr.length = htons(ntohs(pres->hdr.length) + ntohs(srvInfo->hdr.length));
        }
    }

    msgLen = offset - buf;
    if ((msgLen - ntohs(pres->hdr.length)) != 0)
        logDebug("sth's wrong: calculated size %d, does not match buffer length %d", (int) msgLen, ntohs(pres->hdr.length));

    printBuf(buf, msgLen, "enrp presence send buffer");

    /* if receiverid is 0 it will be sent as an announcement */
    sentLen = sendEnrpMsg(buf, msgLen, assocId, receiverId == 0);
    logDebug("sent enrp presence to 0x%08x", receiverId);

    return (sentLen == msgLen) ? 1 : 0;
}

int
sendEnrpListRequest(uint32 receiverId, sctp_assoc_t assocId) {
    struct enrpListRequest *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    int ret;

    memset(buf, 0, sizeof(buf));
    msg = (struct enrpListRequest *) buf;
    msgLen = initEnrpListRequest(msg, receiverId);

    printBuf(buf, msgLen, "enrp list request send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp list request to 0x%08x", receiverId);

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpListResponse(uint32 receiverId, int rejectBit, sctp_assoc_t assocId) {
    RegistrarServer server;
    struct enrpListResponse *response;
    struct paramServerInformation *srvInfo;
    char buf[BUFFER_SIZE];
    char *offset;
    size_t msgLen;
    int ret;
    int length;

    memset(buf, 0, BUFFER_SIZE);
    offset = (char *) buf;
	server = NULL;
	
    response = (struct enrpListResponse *) offset;
    msgLen = initEnrpListResponse(response, receiverId, rejectBit);
    offset += sizeof(struct enrpListResponse);

    if (rejectBit) {
        logDebug("rejecting enrp list response requests")
        
        response->hdr.flags = 0x00 & ENRP_LIST_RESPONSE_REJECT_BIT;
    } else {
        for (server = registrarServerList; server; server = server->next) {
            if (server->rsActive == 0) {
                logDebug("skipping inactive server 0x%08x", server->rsIdentifier);
                continue;
            }
            
            logDebug("adding server information for 0x%08x ", server->rsIdentifier);
            srvInfo = (struct paramServerInformation *) offset;
            initParamServerInformation(srvInfo, server->rsIdentifier);
            offset += sizeof(*srvInfo);
    
            length = transportAddressesToSendBuffer(offset, server->rsEnrpAddr, server->rsEnrpPort, TRANSPORT_USE_DATA_ONLY, IPPROTO_SCTP);
            logDebug("length of transport address parameter is %d", length);
    
            if (length == -1) {
                logDebug("address to buffer conversion failed, omitting server information parameter");
                offset = (char *) response + sizeof(*response);
            } else {
                offset += length;
                srvInfo->hdr.length = htons((uint16) length + ntohs(srvInfo->hdr.length));
                response->hdr.length = htons(ntohs(response->hdr.length) + ntohs(srvInfo->hdr.length));
            }
        }
    }

    msgLen = offset - buf;
    response->hdr.length = htons(msgLen);

    printBuf(buf, msgLen, "enrp list response send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp list response to 0x%08x", receiverId);

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpHandleTableRequest(uint32 receiverId, int ownBit, sctp_assoc_t assocId) {
    struct enrpHandleTableRequest *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    int ret;

    msg = (struct enrpHandleTableRequest *) buf;
    msgLen = initEnrpHandleTableRequest(msg, receiverId, ownBit);

    printBuf(buf, msgLen, "enrp list request send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp handle table request to 0x%08x", receiverId);
    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpHandleTableResponse(uint32 receiverId, int moreBit, int rejectBit, sctp_assoc_t assocId) {
    struct enrpHandleTableResponse *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    ServerPool pool;
    PoolElement pelement;
    char *offset;
    struct paramPoolHandle *phandle;
    int ret;
    int length;
    int addedPoolElement;

    msg = (struct enrpHandleTableResponse *) buf;
    offset = (char *) msg;
    msgLen = initEnrpHandleTableResponse(msg, receiverId, 0, rejectBit);
    offset += sizeof(struct enrpHandleTableResponse);

    for (pool = serverPoolList; pool; pool = pool->next) {
        addedPoolElement = 0;
        phandle = (struct paramPoolHandle *) offset;
        offset += initParamPoolHandle(phandle, pool->spHandle);

        for (pelement = pool->spPeList; pelement; pelement = pelement->poolNext) {
            length = poolElementToSendBuffer(pelement, offset);
            if (length != -1) {
                addedPoolElement = 1;
                offset += length;

                logDebug("added pool element 0x%08x to send buffer", pelement->peIdentifier);
                poolElementPrint(pelement);
            }
        }
        if (addedPoolElement == 0) {
            logDebug("did not add any pool elements to send buffer");
        }
    }

    msgLen = offset - buf;
    msg->hdr.length = htons(msgLen);

    printBuf(buf, msgLen, "enrp handle table response send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp handle table response to 0x%08x", receiverId);

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpHandleUpdate(ServerPool pool, PoolElement element, uint16 handleAction) {    
    struct enrpHandleUpdate *msg;
    struct paramPoolHandle *pHandle;
    char buf[BUFFER_SIZE];
    size_t msgLen = 0;
    char *offset;
    int length;
    int ret;
    
    if (pool == NULL) {
        logDebug("pool is NULL, aborting");
        return -1;
    }
    
    if (element == NULL) {
        logDebug("element is NULL, aborting");
        return -1;
    }
    
    if (!(handleAction == ENRP_UPDATE_ACTION_ADD_PE ||
          handleAction == ENRP_UPDATE_ACTION_DEL_PE)) {
        logDebug("unknown handle update action: 0x%04x", handleAction);
        return -1;
    }
    
    memset(buf, 0, sizeof(buf));
    offset = buf;
    
    msg = (struct enrpHandleUpdate *) buf;
    initEnrpHandleUpdate(msg, 0, handleAction);
    offset += sizeof(struct enrpHandleUpdate);
    
    pHandle = (struct paramPoolHandle *) offset;
    length = initParamPoolHandle(pHandle, pool->spHandle);
    offset += length;
    
    length = poolElementToSendBuffer(element, offset);
    if (length != -1) {
        offset += length;

        logDebug("added pool element 0x%08x to send buffer", element->peIdentifier);
        poolElementPrint(element);
    }
    
    msgLen = offset - buf;
    msg->hdr.length = htons(msgLen);

    printBuf(buf, msgLen, "enrp handle update send buffer");

    ret = sendEnrpMsg(buf, msgLen, 0, 0);

    return (ret == (int) msgLen) ? 1 : -1;
}

int
sendEnrpInitTakeover(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId) {
    struct enrpInitTakeover *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    int ret;

    msg = (struct enrpInitTakeover *) buf;
    msgLen = initEnrpInitTakeover(msg, receiverId, targetId);

    printBuf(buf, msgLen, "enrp init takeover ack send buffer");

    ret = sendEnrpMsgToAll(buf, msgLen);
    logDebug("sent enrp init takeover as announcement");

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpInitTakeoverAck(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId) {
    struct enrpInitTakeoverAck *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    int ret;

    msg = (struct enrpInitTakeoverAck *) buf;
    msgLen = initEnrpInitTakeoverAck(msg, receiverId, targetId);

    printBuf(buf, msgLen, "enrp init takeover ack send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp init takeover ack to 0x%08x", receiverId);

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpTakeoverServer(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId) {
    struct enrpTakeoverServer *msg;
    char buf[BUFFER_SIZE];
    size_t msgLen;
    int ret;

    msg = (struct enrpTakeoverServer *) buf;
    msgLen = initEnrpTakeoverServer(msg, receiverId, targetId);

    printBuf(buf, msgLen, "enrp takeover server send buffer");

    ret = sendEnrpMsgToAll(buf, msgLen);
    logDebug("sent enrp takeover server as announcement");

    return (ret == (int)msgLen) ? 1 : -1;
}

int
sendEnrpError(uint32 receiverId, sctp_assoc_t assocId, int causeId, char *paramBuf, size_t bufLen) {
    struct enrpError *msg;
    struct errorCause *cause;
    char buf[BUFFER_SIZE];
    char *offset;
    size_t msgLen;
    int ret;
    int length;
    
/*    switch(causeId) {
        case ERROR_UNSPECIFIED_ERROR:
        case ERROR_UNRECOGNIZED_PARAMETER:
        case ERROR_UNRECOGNIZED_MESSAGE:
        case ERROR_INVALID_VALUES:
        case ERROR_NONUINIQUE_PE_IDENTIFIER:
        case ERROR_INCONSISTENT_POOLING_POLICY:
        case ERROR_LACK_OF_RESOURCES:
        case ERROR_INCONSISTENT_TRANSPORT_TYPE:
        case ERROR_INCONSISTENT_DATA_CONTROL_CONFIG:
        case ERROR_UNKNOWN_POOLHANDLE:
        case ERROR_SECURITY_CONSIDERATIONS:
        default:
            logDebug("unrecognized error cause id: %d, aborting", causeId);
            return -1;
    }
*/
    msg = (struct enrpError *) buf;
    msgLen = initEnrpError(msg, receiverId);
    offset = buf + msgLen;
    cause = (struct errorCause *) offset;
    
    length = initErrorCause(cause, ERROR_UNSPECIFIED_ERROR);

    if (length != -1) {
        offset += length;

        logDebug("added error cause %d to send buffer", cause->causeCode);
    }

    msgLen = offset - buf;
    msg->hdr.length = htons(msgLen);

    printBuf(buf, msgLen, "enrp error server send buffer");

    ret = sendEnrpMsg(buf, msgLen, assocId, 0);
    logDebug("sent enrp error message");

    return (ret == (int)msgLen) ? 1 : -1;
}

int
createAssocToPeer(Address *addrs, int addrCnt, uint16 port, uint32 serverId, sctp_assoc_t *assocId) {
    RegistrarServer server;
    int ret;
    int parsedCount = 0;
    sctp_assoc_t newAssocId;
    char addrBuf[MAX_ADDR_CNT * sizeof(struct sockaddr_in6)];

    *assocId = 0;

    if (addrs == NULL) {
        logDebug("addrs is NULL");
        return -1;
    }

    if (addrCnt < 1) {
        logDebug("addrCnt is less than 1");
        return -1;
    }

    if ((server = registrarServerListGetServerByServerId(serverId)) == NULL) {
        logDebug("0x%08x is not in registrar servr list", serverId);        
        return -1;
    }

    memset(addrBuf, 0, sizeof(addrBuf));
    parsedCount = addressesToSysCallBuffer(addrs, port, addrCnt, addrBuf);
    if (parsedCount != addrCnt) {
        logDebug("address parsing failed, abort");

        return -1;
    }

    if ((ret = sctp_connectx(enrpSctpFd, (struct sockaddr *) addrBuf, addrCnt, &newAssocId)) < 0) {
        logDebug("error while trying to establish assoc to 0x%08x", serverId);
        perror("sctp_connectx");
        
        return -1;
    }

    logDebug("new assocId for 0x%08x is %d", serverId, newAssocId);

    *assocId = newAssocId;
    server->rsEnrpAssocId = newAssocId;

    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.7  2008/02/14 10:21:39  volkmer
 * removed enrp presence reply required bit
 *
 * Revision 1.6  2008/02/13 17:01:22  volkmer
 * fixed enrp flags and some checksum stuff
 *
 * Revision 1.5  2008/01/12 13:16:04  tuexen
 * Get rid of warnings.
 * Frank Volkmer: Line 1151 might dereference a NULL pointer (server).
 *                Please fix this!
 *
 * Revision 1.4  2008/01/12 13:10:21  tuexen
 * Get it compiling.
 *
 * Revision 1.3  2008/01/11 00:59:51  volkmer
 * implemented ownChildsOnlBit in enrp list handling and sending
 * introduced enrp message flags bitmasks
 * started working on handle space sync
 *
 * Revision 1.2  2008/01/06 20:47:43  volkmer
 * added basic enrp error sending
 * added ownchildsonlybit to list request
 * enhanced main paramter handling
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.33  2007/12/06 01:52:15  volkmer
 * moved peliftimeexpirytimeoutcallback
 *
 * Revision 1.32  2007/12/05 23:10:27  volkmer
 * changed createassoctopeer to use sctp_connectx
 * don't use special sendFd
 * fixed a bug in handle table response
 *
 * Revision 1.31  2007/12/02 22:14:23  volkmer
 * adapted to new sendbuffertopoolelement method
 *
 * Revision 1.30  2007/11/19 22:39:20  volkmer
 * modified enrp presence handling and sending
 *
 * Revision 1.29  2007/11/05 00:04:27  volkmer
 * reformated the copyright statement
 * implemented sendenrphandleupdate
 *
 * Revision 1.28  2007/10/27 15:05:20  volkmer
 * removed debug macro
 *
 **/
