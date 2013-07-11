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
 * $Id: parameters.c,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/
#include <string.h>
#include <stdio.h>

#include "parameters.h"
#include "serverpool.h"
#include "backend.h"
#include "debug.h"
#include "util.h"

int
initParamIpv4Addr(struct paramIpv4Addr *param, struct in_addr addr) {
    param->hdr.type = htons(PARAM_IPV4_ADDR);
    param->hdr.length = htons(8);
    param->addr = addr;
    return 8;
}

int
initParamIpv6Addr(struct paramIpv6Addr *param, struct in6_addr addr) {
    param->hdr.type = htons(PARAM_IPV6_ADDR);
    param->hdr.length = htons(20);
    param->addr = addr;
    return 20;
}

int
initParamDccpTransport(struct paramDccpTransport *param, uint16 port, uint32 serviceCode) {
    param->hdr.type = htons(PARAM_DCCP_TRANSPORT);
    param->hdr.length = htons(8);
    param->dccpPort = htons(port);
    param->reserved = 0;
    param->dccpServiceCode = htonl(serviceCode);
    return 8;
}

int
initParamSctpTransport(struct paramSctpTransport *param, uint16 port, uint16 transportUse) {
    param->hdr.type = htons(PARAM_SCTP_TRANSPORT);
    param->hdr.length = htons(8);
    param->sctpPort = htons(port);
    param->transportUse = htons(transportUse);
    return 8;
}

int
initParamTcpTransport(struct paramTcpTransport *param, uint16 port) {
    param->hdr.type = htons(PARAM_TCP_TRANSPORT);
    param->hdr.length = htons(8);
    param->tcpPort = htons(port);
    param->reserved = 0;
    return 8;
}

int
initParamUdpTransport(struct paramUdpTransport *param, uint16 port) {
    param->hdr.type = htons(PARAM_UDP_TRANSPORT);
    param->hdr.length = htons(8);
    param->udpPort = htons(port);
    param->reserved = 0;
    return 8;
}

int
initParamUdpLiteTransport(struct paramUdpLiteTransport *param, uint16 port) {
    param->hdr.type = htons(PARAM_UDP_LITE_TRANSPORT);
    param->hdr.length = htons(8);
    param->udpLitePort = htons(port);
    param->reserved = 0;
    return 8;
}

int
initParamPoolMemberSelectionPolicy(struct paramPoolMemberSelectionPolicy *param, uint32 policyType, uint32 loadWeight, uint32 loadDeg) {
    uint16 length;

    param->hdr.type   = htons(PARAM_POOL_MEMBER_SELECTION_POLICY);
    param->policyType = htonl(policyType);

    switch (policyType) {
    case POLICY_ROUND_ROBIN:
    case POLICY_RANDOM:
        length = 8;
        break;
    case POLICY_WEIGHTED_ROUND_ROBIN:
    case POLICY_WEIGHTED_RANDOM:
    case POLICY_LEAST_USED:
    case POLICY_RANDOMIZED_LEAST_USED:
        length = 12;
        param->reserved[0] = htonl(loadWeight);
        break;
    case POLICY_LEAST_USED_W_DEGRADATION:
    case POLICY_PRIORITY_LEAST_USED:
        length = 12;
        param->reserved[0] = htonl(loadWeight);
        param->reserved[1] = htonl(loadDeg);
        break;
    default:
        logDebug("unknown policyType: %d", policyType);
        return -1;
    }
    param->hdr.length = htons(length);
    return length;
}

int
initParamPoolHandle(struct paramPoolHandle *param, char *ph) {
    char poolHandle[POOLHANDLE_SIZE];
    uint16 length = 0;

    memset(poolHandle, 0, POOLHANDLE_SIZE);
    strncpy(poolHandle, ph, POOLHANDLE_SIZE);
    length = strlen(poolHandle);
    length = ADD_PADDING(length);

    memcpy((char*)param + sizeof(struct paramHeader), poolHandle, length);
    param->hdr.type = htons(PARAM_POOL_HANDLE);
    param->hdr.length = htons(length + 4);

    return length + 4;
}

int
initParamPoolElement(struct paramPoolElement *param, uint32 peIdentifier, uint32 homeEnrpServerIdentifier, int32 registrationLife) {
    param->hdr.type = htons(PARAM_POOL_ELEMENT);
    param->hdr.length = htons(16);
    param->peIdentifier = htonl(peIdentifier);
    param->homeEnrpServerIdentifier = htonl(homeEnrpServerIdentifier);
    param->registrationLife = htonl(registrationLife);
    return 16;
}

int
initParamServerInformation(struct paramServerInformation *param, uint32 serverId) {
    param->hdr.type = htons(PARAM_SERVER_INFORMATION);
    param->hdr.length = htons(8);
    param->serverId = htonl(serverId);
    return 8;
}

int
initParamOperationError(struct paramOperationError *param) {
    param->hdr.type = htons(PARAM_OPERATION_ERROR);
    param->hdr.length = htons(4);
    return 4;
}

int
initParamCookie(struct paramCookie *param) {
    param->hdr.type = htons(PARAM_COOKIE);
    param->hdr.length = htons(4);
    return 4;
}

int
initParamPeIdentifier(struct paramPeIdentifier *param, uint32 peIdentifier) {
    param->hdr.type = htons(PARAM_PE_IDENTIFIER);
    param->hdr.length = htons(8);
    param->peIdentifier = htonl(peIdentifier);
    return 8;
}

int
initParamPeChecksum(struct paramPeChecksum *param, checksumTypeFolded checksum) {
    param->hdr.type = htons(PARAM_PE_CHECKSUM);
    param->hdr.length = htons(6);
    param->checksum = htons(checksum);
    param->padding = 0;
    return 8;
}

int
initErrorCause(struct errorCause *cause, uint16 causeCode) {
    cause->causeCode = htons(causeCode);
    cause->causeLength = htons(4);
    return 4;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2007/12/02 22:07:56  volkmer
 * fixed dccp service code missing
 *
 * Revision 1.12  2007/11/19 22:36:52  volkmer
 * adapted parameters to new version of param draft
 *
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:47:18  volkmer
 * removed debug macro
 *
 **/
