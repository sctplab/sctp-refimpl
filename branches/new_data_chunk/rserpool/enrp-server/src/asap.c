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
 * $Id: asap.c,v 1.4 2008-02-14 10:19:03 volkmer Exp $
 *
 **/
 
#include <string.h>
#include <stdlib.h>

#include "asap.h"
#include "parameters.h"
#include "serverpool.h"
#include "registrarserver.h"
#include "util.h"
#include "enrp.h"
#include "debug.h"

int
sendAsapAnnounceMsg(char *buf, size_t msgLen) {
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    ssize_t sentLen = -1;

    switch (asapAnnounceAddr.type) {
    case AF_INET:
        memset(&in4, 0, sizeof(in4));
        in4.sin_family = AF_INET;
        in4.sin_addr = asapAnnounceAddr.addr.in4;
        in4.sin_port = htons(asapAnnouncePort);
#ifdef HAVE_SIN_LEN
        in4.sin_len = sizeof(struct sockaddr_in);
#endif

        if ((sentLen = sendto(asapUdpFd, (const void *)buf, msgLen, 0, (const struct sockaddr *) &in4, sizeof(struct sockaddr_in))) < 0)
            perror("sendto: ipv4 asap announce sending failed");
        break;

    case AF_INET6:
        memset(&in6, 0, sizeof(in6));
        in6.sin6_family = AF_INET6;
        in6.sin6_addr = asapAnnounceAddr.addr.in6;
        in6.sin6_port = htons(asapAnnouncePort);
#ifdef HAVE_SIN6_LEN
        in6.sin6_len = sizeof(struct sockaddr_in6);
#endif

        if ((sentLen = sendto(asapUdpFd, (const void *)buf, msgLen, 0, (const struct sockaddr *) &in6, sizeof(struct sockaddr_in6))) < 0)
            perror("sendto: ipv6 asap announce sending failed");
        break;

    default:
        logDebug("asap announce addr is uninitialized! EXIT!!!");
        exit(-1);
    }

    return sentLen;
}

int
sendAsapMsg(char *buf, size_t len, sctp_assoc_t assocId, int announcement) {
    struct sctp_sndrcvinfo sinfo;
    int sentLen = 0;

    if (!buf) {
        logDebug("buf is NULL");
        return -1;
    }

    if (len < 8) {
        logDebug("aborting, len is too small: %d", (int) len);
        return -1;
    }

    if (announcement) {
        logDebug("sending as announcement");
        return sendAsapAnnounceMsg(buf, len);
    }

    /* TODO:
     * verify associd
     * against what?
     * assume its valid
     */

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.sinfo_assoc_id = assocId;
    sinfo.sinfo_ppid = htonl(ASAP_SCTP_PPID);

    if ((sentLen = sctp_send(asapSctpFd, (void *)buf, len, &sinfo, 0)) < 0) {
        logDebug("sctp_send to %d failed", assocId);
        perror("sctp_send");
        return -1;
    }

    logDebug("sent msg via associd %d. sctp_send returned: %d", assocId, sentLen);
    return 1;
}

/*
 * 2.2.1 ASAP_REGISTRATION
 */

int
handleAsapRegistration(void *buf, ssize_t len, sctp_assoc_t assocId) {
	struct asapRegistration *msg;
	struct paramPoolHandle *phandle;
	struct paramPoolElement *pElem;
	PoolElement element;
	ServerPool pool;
	char *offset;
	size_t bufSize;
    char poolHandle[POOLHANDLE_SIZE + 1];
	uint16 length;
    int poolCreated = 0;

    printBuf((char *)buf, len, "asap registration receive buffer");

	element = NULL;
	memset(poolHandle, 0, sizeof(poolHandle));
	msg = (struct asapRegistration *) buf;
	length = ntohs(msg->length);
	bufSize = length - sizeof(struct asapRegistration);
	offset = (char *) msg + sizeof(struct asapRegistration);
	
	if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
		phandle = (struct paramPoolHandle *) offset;
		length = ntohs(phandle->hdr.length);
		strncpy(poolHandle, ((char *) phandle + 4), length - 4);
		if (length % 4 != 0)
			length += 4 - (length % 4);
		
        offset += length;
		bufSize -= length;
		logDebug("poolHandle: '%s'", poolHandle);
	} else {
		logDebug("malformed packet, poolhandle parameter");
		return -1;
	}

	if (checkTLV(offset, bufSize) == PARAM_POOL_ELEMENT) {
		pElem = (struct paramPoolElement *) offset;
		length = ntohs(pElem->hdr.length);
        logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
		length = sendBufferToPoolElement(offset, bufSize, &element);
        offset += length;
        bufSize -= length;
        logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
        logDebug("element: %p", (void *) element);

		if (bufSize > 0) {
			logDebug("malformed packet, pool element parameter");
			return -1;
		}
        
        if (element == NULL) {
            logDebug("malformed packet, unparseable pool element parameter");
            return -1;
        }

        element->peAsapAssocId = assocId;
		element->peHomeRegistrarServer = ownId;
	} else {
		logDebug("malformed packet, pool element parameter");
		return -1;
	}

	if ((pool = serverPoolListGetPool(poolHandle)) == NULL) {
		pool = serverPoolNew(poolHandle);
		poolCreated = 1;
        serverPoolListAddPool(pool);
    }
    
    if (!policyCheck(pool->spPolicy, element)) {
        logDebug("pool policy does not match element policy");

        if (poolCreated) {
            serverPoolListRemovePool(pool->spHandle);
            serverPoolDelete(pool);
        }

        return -1;
    }
    
    /* everything ok */         
    serverPoolAddPoolElement(pool, element);
    registrarServerAddPoolElement(this, element);
         
    sendEnrpHandleUpdate(pool, element, ENRP_UPDATE_ACTION_ADD_PE);
         
    sendAsapRegistrationResponse(assocId, 0, pool, element->peIdentifier);
    
    /* TODO:
     * verify return codes
     */

	return 1;
}

/*
 * 2.2.2 ASAP_DEREGISTRATION
 */

int
handleAsapDeregistration(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct asapDeregistration *msg;
    struct paramPoolHandle *phandle;
    struct paramPeIdentifier *peIdent;
    PoolElement element = NULL;
    uint32 peId = 0;
    ServerPool pool = NULL;
    RegistrarServer registrar = NULL;
    char *offset;
    size_t bufSize;
    char poolHandle[POOLHANDLE_SIZE + 1];
    int length = 0;

    msg = (struct asapDeregistration *) buf;
    memset(poolHandle, 0, sizeof(poolHandle));    
    length = ntohs(msg->length);

    bufSize = length - sizeof(struct asapRegistration);
    offset = (char *) msg + sizeof(struct asapRegistration);

    if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolHandle, ((char *) phandle + 4), length - 4);
        if (length % 4 != 0)
            length += 4 - (length % 4);
        offset += length;
        bufSize -= length;
        logDebug("poolHandle: '%s'", poolHandle);
        pool = serverPoolListGetPool(poolHandle);

        if (pool == NULL) {
            logDebug("unknown poolHandle, aborting deregistration");
            return -1;
        }
    }    

    if (checkTLV(offset, bufSize) == PARAM_PE_IDENTIFIER) {
        peIdent = (struct paramPeIdentifier *) offset;
        peId = ntohl(peIdent->peIdentifier);
        element = serverPoolGetPoolElementById(pool, peId);

        if (element == NULL) {
            logDebug("unknown pool element 0x%08x, aborting deregistration", peId);
            return -1;
        }
    }

    /* verify ownership of pe */
    registrar = (RegistrarServer) element->peRegistrarServer;
    
    if (registrar != this) {
        logDebug("pool element is owned by 0x%08x, aborting deregistration atempt", registrar->rsIdentifier);
        return -1;
    }

    sendEnrpHandleUpdate(pool, element, ENRP_UPDATE_ACTION_DEL_PE);
    sendAsapDeregistrationResponse(assocId, pool, peId);

    registrarServerRemovePoolElement(registrar, element);
    serverPoolRemovePoolElement(pool, peId);

    /* TODO:
     * verify message is from pe which claims to be deregistrating
     * where and when will the element be deleted?
     * 
     * */

	return 1;
}

/*
 * 2.2.3 ASAP_REGISTRATION_RESPONSE
 */

size_t
initAsapRegistrationResponse(struct asapRegistrationResponse *msg, int rejectFlag) {
	msg->type = ASAP_REGISTRATION_RESPONSE;
	msg->flags = (rejectFlag == 1) ? 0x1 : 0x0;
	msg->length = htons(4);
	
	return 4;
}

int
sendAsapRegistrationResponse(sctp_assoc_t assocId, int rejectFlag, ServerPool pool, uint32 peIdentifier) {
	struct asapRegistrationResponse *msg;
    struct paramPoolHandle *pHandle;
    struct paramPeIdentifier *peIdent;
	char buf[BUFFER_SIZE];
    char *offset;
    int length = 0;
    int msgLen = 0;
    int ret = 0;

    if (pool == NULL) {
        logDebug("pool is NULL");
        return -1;
    }

	memset(buf, 0, sizeof(buf));
    offset = buf;

	msg = (struct asapRegistrationResponse *) buf;
	length = initAsapRegistrationResponse(msg, rejectFlag);
    offset += length;
    
    pHandle = (struct paramPoolHandle *) offset;
    length = initParamPoolHandle(pHandle, pool->spHandle);
    offset += length;

    peIdent = (struct paramPeIdentifier *) offset;
    length = initParamPeIdentifier(peIdent, peIdentifier);
	offset += length;
    
    msgLen = offset - buf;
    msg->length = htons(msgLen);

    printBuf(buf, msgLen, "asap registration response send buffer");

    ret = sendAsapMsg(buf, msgLen, assocId, 0);
    logDebug("sent asap registration response");

    return (ret == (int)msgLen) ? 1 : -1;
}

/*
 * 2.2.4 ASAP_DEREGISTRATION_RESPONSE
 */

size_t
initAsapDeregistrationResponse(struct asapDeregistrationResponse *msg) {
	msg->type = ASAP_DEREGISTRATION_RESPONSE;
	msg->flags = 0;
	msg->length = htons(4);
	
	return 4;
}

int
sendAsapDeregistrationResponse(sctp_assoc_t assocId, ServerPool pool, uint32 peIdentifier) {
    struct asapDeregistrationResponse *msg; 
    struct paramPoolHandle *pHandle;
    struct paramPeIdentifier *peIdent;
    char buf[BUFFER_SIZE];
    char *offset;
    int msgLen = 0;
    int ret = 0;
        
    memset(buf, 0, sizeof(buf));
    offset = buf;

    msg = (struct asapDeregistrationResponse *) buf;
    offset += initAsapDeregistrationResponse(msg);
    
    pHandle = (struct paramPoolHandle *) offset;
    offset += initParamPoolHandle(pHandle, pool->spHandle);
    
    peIdent = (struct paramPeIdentifier *) offset;
    offset += initParamPeIdentifier(peIdent, peIdentifier);
    
    msgLen = offset - buf;
    msg->length = htons(msgLen);

    printBuf(buf, msgLen, "asap registration response send buffer");

    ret = sendAsapMsg(buf, msgLen, assocId, 0);
    logDebug("sent asap registration response");

    return (ret == (int) msgLen) ? 1 : -1;
}

/*
 * 2.2.5 ASAP_HANDLE_RESOLUTION
 */

int
handleAsapHandleResolution(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct asapHandleResolution *msg;
    struct paramPoolHandle *phandle;
    ServerPool pool;
    char poolHandle[POOLHANDLE_SIZE + 1];
    uint16 length;
    char *offset;
    size_t bufSize;
    int sBit;

    printBuf((char *)buf, len, "asap registration receive buffer");

    memset(poolHandle, 0, sizeof(poolHandle));
    msg = (struct asapHandleResolution *) buf;
    length = ntohs(msg->length);
    bufSize = length - sizeof(struct asapHandleResolution);
    offset = (char *) msg + sizeof(struct asapHandleResolution);
    sBit = (msg->flags & 0x01) ? 1 : 0;

    if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolHandle, ((char *) phandle + 4), length - 4);
        if (length % 4 != 0)
            length += 4 - (length % 4);
        
        offset += length;
        bufSize -= length;
        logDebug("poolHandle: '%s'", poolHandle);
    } else {
        logDebug("malformed packet, poolhandle parameter");
        return -1;
    }
    
    if (bufSize != 0) {
        logDebug("malformed packet!");
        return -1;
    }

    /* TODO:
     * manag sbit state.
     * store that per associd.
     * need associd db anyway
     * 
     * */

    pool = NULL;
    pool = serverPoolListGetPool(poolHandle);

    if (pool == NULL) {
        logDebug("unknown pool: %s, aborting", poolHandle);
        
        return -1;
    }

    sendAsapHandleResolutionResponse(0, assocId, pool);

    return 1;
}

/*
 * 2.2.6 ASAP_HANDLE_RESOLUTION_RESPONSE
 */

size_t
initAsapHandleResolutionResponse(struct asapHandleResolutionResponse *msg, int acceptUpdatesFlag) {
	msg->type = ASAP_HANDLE_RESOLUTION_RESPONSE;
	msg->flags = (acceptUpdatesFlag == 1) ? 0x1 : 0x0;
	msg->length = htons(4);
	
	return 4;
}

int
sendAsapHandleResolutionResponse(int acceptUpdatesFlag, sctp_assoc_t assocId, ServerPool pool) {
	struct asapHandleResolutionResponse *msg;
	struct paramPoolHandle *pHandle;
	char buf[BUFFER_SIZE];
	int length = 0;
    int i = 0;
    int ret = 0;
    int msgLen = 0;
    char *offset;
    int count;
    PoolElement pe;
    uint32 peIds[5];
	
	memset(buf, 0, sizeof(buf));
	msg = (struct asapHandleResolutionResponse *) buf;
	length = initAsapHandleResolutionResponse(msg, acceptUpdatesFlag);
	offset = (char *) buf;
    offset += length;
    msgLen = length;
	
    ret = serverPoolGetHandleResponse(pool, &count, peIds);
    
    if (ret < 0) {
        logDebug("sths wrong, aborting");
        
        return -1;
    }

    pHandle = (struct paramPoolHandle *) offset;
    length = initParamPoolHandle(pHandle, pool->spHandle);
    offset += length;
    
    for (i = 0; i < count; i++) {
        pe = serverPoolGetPoolElementById(pool, peIds[i]);
        
        if (pe == NULL) {
            logDebug("unknown pe: 0x%08x, aborting", peIds[i]);
            
            return -1;
        }
        
        length = poolElementToSendBuffer(pe, offset);
        if (length <= 0) {
            logDebug("length is less or equal to 0, aborting");
            
            return -1;
        }

        offset += length;
    }

    
    msgLen = offset - buf;
    msg->length = htons(msgLen);

    ret = sendAsapMsg(buf, msgLen, assocId, 0);
    logDebug("sent asap handle resolution response");

    return (ret == (int)msgLen) ? 1 : -1;
}

/*
 * 2.2.7 ASAP_ENDPOINT_KEEP_ALIVE
 */

size_t
initAsapEndpointKeepAlive(struct asapEndpointKeepAlive *msg, uint32 serverId, int homeFlag) {
	msg->type = ASAP_ENDPOINT_KEEP_ALIVE;
	msg->flags = (homeFlag == 1) ? 0x1 : 0x0;
	msg->length = htons(8);
	msg->serverId = htonl(serverId);
	
	return 8;
}

int
sendAsapEndpointKeepAlive(PoolElement element, int homeFlag) {
	struct asapEndpointKeepAlive *msg;
    struct paramPoolHandle *pHandle;
    ServerPool pool;
    char *offset;
    char buf[BUFFER_SIZE];
	int length = 0;
    int ret = 0;
    int msgLen = 0;
    int sucess;
    
    if (element == NULL) {
        logDebug("element is NULL");
        return -1;
    }
    
    pool = (ServerPool) element->peServerPool;
    
    if (pool == NULL) {
        logDebug("pool is NULL");
        return -1;
    }

	memset(buf, 0, sizeof(buf));
    offset = buf;
	msg = (struct asapEndpointKeepAlive *) buf;
	length = initAsapEndpointKeepAlive(msg, ownId, homeFlag);
    offset += length;
    
    pHandle = (struct paramPoolHandle *) buf;
    length = initParamPoolHandle(pHandle, pool->spHandle);
    offset += length;

    msgLen = offset - buf;
    msg->length = htons(msgLen);

    ret = sendAsapMsg(buf, msgLen, element->peAsapAssocId, 0);
    logDebug("sent asap endpoint keep alive");

    /* TODO:
     * 
     * start a timer?
     * 
     * */

    sucess = (ret == (int) msgLen) ? 1 : -1;
    if (sucess == 1) {
        return 1;
    }
    
    /* send failed, delete pe */
    logDebug("removing pe 0x%08x due to sctp send failure", element->peIdentifier);
    sendEnrpHandleUpdate(pool, element, ENRP_UPDATE_ACTION_DEL_PE);

    registrarServerRemovePoolElement(this, element);
    serverPoolRemovePoolElement(pool, element->peIdentifier);
    
    return 1;
}

/*
 * 2.2.8 ASAP_ENDPOINT_KEEP_ALIVE_ACK
 */
int
handleAsapEndpointKeepAliveAck(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct asapEndpointKeepAliveAck *msg;
    struct paramPeIdentifier *peIdent;
    struct paramPoolHandle *phandle;
    ServerPool pool;
    char poolHandle[POOLHANDLE_SIZE + 1];
    uint32 peIdentifier;
    uint16 length;
    char *offset;
    size_t bufSize;

    memset(poolHandle, 0, sizeof(poolHandle));
    msg = (struct asapEndpointKeepAliveAck *) buf;
    length = ntohs(msg->length);
    bufSize = length - sizeof(struct asapEndpointKeepAliveAck);
    offset = (char *) msg + sizeof(struct asapEndpointKeepAliveAck);

    if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolHandle, ((char *) phandle + 4), length - 4);
        if (length % 4 != 0)
            length += 4 - (length % 4);
        
        offset += length;
        bufSize -= length;
        logDebug("poolHandle: '%s'", poolHandle);
    } else {
        logDebug("malformed packet, poolhandle parameter");
        return -1;
    }
    
    if (checkTLV(offset, bufSize) == PARAM_PE_IDENTIFIER) {
        peIdent = (struct paramPeIdentifier *) offset;
        peIdentifier = ntohl(peIdent->peIdentifier);
        offset += length;
        bufSize -= length;
    } else {
        logDebug("malformed packet, poolhandle parameter");
        return -1;
    }

    if (bufSize != 0) {
        logDebug("malformed packet!");
        return -1;
    }

    pool = NULL;
    pool = serverPoolListGetPool(poolHandle);

    if (pool == NULL) {
        logDebug("unknown pool: %s, aborting", poolHandle);
        
        return -1;
    }
    
    /* TODO:
     * 
     * what exactly needs to be done here?
     * 
     * */

    return 1;
}

/*
 * 2.2.9 ASAP_ENDPOINT_UNREACHABLE
 */

int
handleAsapEndpointUnreachable(void *buf, ssize_t len, sctp_assoc_t assocId) {
    struct asapEndpointUnreachable *msg;
    struct paramPeIdentifier *peIdent;
    struct paramPoolHandle *phandle;
    ServerPool pool;
    PoolElement element;
    char poolHandle[POOLHANDLE_SIZE + 1];
    uint32 peIdentifier;
    uint16 length;
    char *offset;
    size_t bufSize;

    memset(poolHandle, 0, sizeof(poolHandle));
    msg = (struct asapEndpointUnreachable *) buf;
    length = ntohs(msg->length);
    bufSize = length - sizeof(struct asapEndpointUnreachable);
    offset = (char *) msg + sizeof(struct asapEndpointUnreachable);

    if (checkTLV(offset, bufSize) == PARAM_POOL_HANDLE) {
        phandle = (struct paramPoolHandle *) offset;
        length = ntohs(phandle->hdr.length);
        strncpy(poolHandle, ((char *) phandle + 4), length - 4);
        if (length % 4 != 0)
            length += 4 - (length % 4);
        
        offset += length;
        bufSize -= length;
        logDebug("poolHandle: '%s'", poolHandle);
    } else {
        logDebug("malformed packet, poolhandle parameter");
        return -1;
    }
    
    if (checkTLV(offset, bufSize) == PARAM_PE_IDENTIFIER) {
        peIdent = (struct paramPeIdentifier *) offset;
        peIdentifier = ntohl(peIdent->peIdentifier);
        offset += length;
        bufSize -= length;
    } else {
        logDebug("malformed packet, poolhandle parameter");
        return -1;
    }

    if (bufSize != 0) {
        logDebug("malformed packet!");
        return -1;
    }

    pool = NULL;
    pool = serverPoolListGetPool(poolHandle);

    if (pool == NULL) {
        logDebug("unknown pool: %s, aborting", poolHandle);
        
        return -1;
    }

    element = serverPoolGetPoolElementById(pool, peIdentifier);

    if (element == NULL) {
        logDebug("pe 0x%08x is not in pool %s, aborting", peIdentifier, poolHandle);
        
        return -1;
    }

    sendAsapEndpointKeepAlive(element, 0);


    return 1;
}

/*
 * 2.2.10 ASAP_SERVER_ANNOUNCE
 */

size_t
initAsapServerAnnounce(struct asapServerAnnounce *msg, uint32 serverId) {
	msg->type = ASAP_SERVER_ANNOUNCE;
	msg->flags = 0;
	msg->length = htons(8);
	msg->serverId = htonl(serverId);
	
	return 8;
}

int
sendAsapServerAnnounce() {
	struct asapServerAnnounce *msg;
	char buf[BUFFER_SIZE];
    char *offset;
	int length = 0;
    size_t msgLen, sentLen;

	memset(buf, 0, sizeof(buf));
    offset = (char *) &buf;
	msg = (struct asapServerAnnounce *) buf;
	
	initAsapServerAnnounce(msg, ownId);
    offset += sizeof(*msg);
	
	length = transportAddressesToSendBuffer(offset, this->rsAsapAddr, this->rsAsapPort, TRANSPORT_USE_DATA_ONLY, IPPROTO_SCTP);
	logDebug("length of transport address parameter is %d", length);
	
	if (length == -1) {
		logDebug("address to buffer conversion failed, any transport information");
        return -1;
	} else {
		offset += length;
        msgLen = offset - buf;
		msg->length = htons(msgLen);
	}

	printBuf(buf, msgLen, "asap server announce send buffer");

	sentLen = sendAsapMsg(buf, msgLen, 0, 1);
    logDebug("sent asap server announce");

	return (sentLen == msgLen) ? 1 : 0;
}

/*
 * 2.2.14 ASAP_ERROR
 */

size_t
initAsapError(struct asapError *msg) {
	msg->type = ASAP_ERROR;
	msg->flags = 0;
	msg->length = htons(4);
	
	return 4;
}

int
sendAsapError() {
    logDebug("not implemented yet");

    return -1;
}

int
handleAsapError(void *buf, ssize_t len, sctp_assoc_t assocId) {
    logDebug("not implemented yet");
    
    return -1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2008/02/13 16:58:30  volkmer
 * initialize peHomeregistrar correctly
 *
 * Revision 1.2  2007/12/27 01:06:27  skaliann
 * rserpool
 * - Fixed compilation errors and warnings
 * - modified the parameter type values to the latest draft
 * - Handle the case when the overall selection policy parameter is not sent
 *
 * enrp-server
 * - added poolHandle parameter to the HANDLE_RESOLUTION_RESPONSE
 * - Fixed a crash in policy selection code
 * - Fixed pelement->peIdentifier ntohl
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.15  2007/12/03 23:44:26  volkmer
 * fixed bug in handleasapderegestration
added pe removing stuff to sendasapendpointkeepalive
added handleasapendpointkeepaliveack, unsure what exactly needs to be done there
added handleasapendpoint unreachable
 *
 * Revision 1.14  2007/12/02 22:46:59  volkmer
 * use correct fd ... fixed sendAsapRegistrationResponse
 *
 * Revision 1.13  2007/12/02 22:13:05  volkmer
 * do not use asapUdpSendFd
fixed handle registration
implemented handle resolution and sendHandleResolutuin
 *
 * Revision 1.12  2007/11/19 22:43:48  volkmer
 * added asap send announce, changed send server announce to work with rsplib, changed registration response
 *
 * Revision 1.11  2007/11/07 13:02:02  volkmer
 * added more comments, fixed sendAsapServerAnnounce, implemented sendAsapEndpointkeepAlive
 *
 * Revision 1.10  2007/11/06 08:21:48  volkmer
 * changed signature of sendAsapEndpointKeepAlive
 *
 * Revision 1.9  2007/11/05 00:05:47  volkmer
 * reformated the copyright statement

started implementing asap
re- /deregistartion needs to be tested
 *
 **/
