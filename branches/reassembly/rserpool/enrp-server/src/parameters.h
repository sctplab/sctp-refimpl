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
 * $Author: volkmer $
 * $Id: parameters.h,v 1.2 2008-01-06 20:47:43 volkmer Exp $
 *
 **/
#ifndef PARAMETERS_H
#define PARAMETERS_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "backend.h"
#include "checksum.h"

/* to pack types in structs properly */
#pragma pack(1)


/* enrp and asap common parameters */
#define PARAM_IPV4_ADDR                     0x0001
#define PARAM_IPV6_ADDR                     0x0002
#define PARAM_DCCP_TRANSPORT                0x0003
#define PARAM_SCTP_TRANSPORT                0x0004
#define PARAM_TCP_TRANSPORT                 0x0005
#define PARAM_UDP_TRANSPORT                 0x0006
#define PARAM_UDP_LITE_TRANSPORT            0x0007
#define PARAM_POOL_MEMBER_SELECTION_POLICY  0x0008
#define PARAM_POOL_HANDLE                   0x0009
#define PARAM_POOL_ELEMENT                  0x000a
#define PARAM_SERVER_INFORMATION            0x000b
#define PARAM_OPERATION_ERROR               0x000c
#define PARAM_COOKIE                        0x000d
#define PARAM_PE_IDENTIFIER                 0x000e
#define PARAM_PE_CHECKSUM                   0x000f

#define TRANSPORT_USE_DATA_ONLY             0x0000
#define TRANSPORT_USE_DATA_PLUS_CONTROL     0x0001

#define POLICY_ROUND_ROBIN                  0x00000001
#define POLICY_WEIGHTED_ROUND_ROBIN         0x00000002
#define POLICY_RANDOM                       0x00000003
#define POLICY_WEIGHTED_RANDOM              0x00000004
#define POLICY_LEAST_USED                   0x40000001
#define POLICY_LEAST_USED_W_DEGRADATION     0x40000002
#define POLICY_PRIORITY_LEAST_USED          0x40000003
#define POLICY_RANDOMIZED_LEAST_USED        0x40000004

#define ERROR_UNSPECIFIED_ERROR                 0x0
#define ERROR_UNRECOGNIZED_PARAMETER            0x1
#define ERROR_UNRECOGNIZED_MESSAGE              0x2
#define ERROR_INVALID_VALUES                    0x3
#define ERROR_NONUINIQUE_PE_IDENTIFIER          0x4
#define ERROR_INCONSISTENT_POOLING_POLICY       0x5
#define ERROR_LACK_OF_RESOURCES                 0x6
#define ERROR_INCONSISTENT_TRANSPORT_TYPE       0x7
#define ERROR_INCONSISTENT_DATA_CONTROL_CONFIG  0x8
#define ERROR_UNKNOWN_POOLHANDLE                0x9
#define ERROR_SECURITY_CONSIDERATIONS           0xa

struct paramHeader {
    uint16 type;
    uint16 length;
};

struct paramIpv4Addr {
    struct paramHeader hdr;
    struct in_addr     addr;
};

struct paramIpv6Addr {
    struct paramHeader hdr;
    struct in6_addr    addr;
};

struct paramDccpTransport {
    struct paramHeader hdr;
    uint16 dccpPort;
    uint16 reserved;
    uint32 dccpServiceCode;
};

struct paramSctpTransport {
    struct paramHeader hdr;
    uint16 sctpPort;
    uint16 transportUse;
};

struct paramTcpTransport {
    struct paramHeader hdr;
    uint16 tcpPort;
    uint16 reserved;
};

struct paramUdpTransport {
    struct paramHeader hdr;
    uint16 udpPort;
    uint16 reserved;
};

struct paramUdpLiteTransport {
    struct paramHeader hdr;
    uint16 udpLitePort;
    uint16 reserved;
};

struct paramPoolMemberSelectionPolicy {
    struct paramHeader hdr;
    uint32             policyType;
    uint32             reserved[4];
};

struct paramPoolHandle {
    struct paramHeader hdr;
};

struct paramPoolElement {
    struct paramHeader hdr;
    uint32 peIdentifier;
    uint32 homeEnrpServerIdentifier;
    int32  registrationLife;
};

struct paramServerInformation {
    struct paramHeader hdr;
    uint32             serverId;
};

struct paramOperationError {
    struct paramHeader hdr;
};

struct paramCookie {
    struct paramHeader hdr;
};

struct paramPeIdentifier {
    struct paramHeader hdr;
    uint32             peIdentifier;
};

struct paramPeChecksum {
    struct paramHeader hdr;
    uint16             checksum;
    uint16             padding;
};

struct errorCause {
    uint16 causeCode;
    uint16 causeLength;
};


int
initParamIpv4Addr(struct paramIpv4Addr *param, struct in_addr addr);

int
initParamIpv6Addr(struct paramIpv6Addr *param, struct in6_addr addr);

int
initParamDccpTransport(struct paramDccpTransport *param, uint16 port, uint32 serviceCode);

int
initParamSctpTransport(struct paramSctpTransport *param, uint16 port, uint16 transportUse);

int
initParamTcpTransport(struct paramTcpTransport *param, uint16 port);

int
initParamUdpTransport(struct paramUdpTransport *param, uint16 port);

int
initParamUdpLiteTransport(struct paramUdpLiteTransport *param, uint16 port);

int
initParamPoolMemberSelectionPolicy(struct paramPoolMemberSelectionPolicy *param, uint32 policyType, uint32 loadWeight, uint32 loadDeg);

int
initParamPoolHandle(struct paramPoolHandle *param, char *poolHandle);

int
initParamPoolElement(struct paramPoolElement *param, uint32 peIdentifier, uint32 homeEnrpServerIdentifier, int32 registrationLife);

int
initParamServerInformation(struct paramServerInformation *param, uint32 serverId);

int
initParamOperationError(struct paramOperationError *param);

int
initParamCookie(struct paramCookie *param);

int
initParamPeIdentifier(struct paramPeIdentifier *param, uint32 peIdentifier);

int
initParamPeChecksum(struct paramPeChecksum *param, checksumTypeFolded checksum);

int
initErrorCause(struct errorCause *cause, uint16 causeCode);

#endif /* PARAMETERS_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.10  2007/12/02 22:07:56  volkmer
 * fixed dccp service code missing
 *
 * Revision 1.9  2007/11/19 22:36:52  volkmer
 * adapted parameters to new version of param draft
 *
 * Revision 1.8  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.7  2007/10/27 12:47:18  volkmer
 * removed debug macro
 *
 **/
