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
 * $Id: asap.h,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/
#ifndef _ASAP_H
#define _ASAP_H 1

#include "parameters.h"
#include "serverpool.h"
#include "poolelement.h"


/* to pack types in structs properly */
#pragma pack(1)

/* asap message types */

#define ASAP_REGISTRATION                0x01
#define ASAP_DEREGISTRATION              0x02
#define ASAP_REGISTRATION_RESPONSE       0x03
#define ASAP_DEREGISTRATION_RESPONSE     0x04
#define ASAP_HANDLE_RESOLUTION           0x05
#define ASAP_HANDLE_RESOLUTION_RESPONSE  0x06
#define ASAP_ENDPOINT_KEEP_ALIVE         0x07
#define ASAP_ENDPOINT_KEEP_ALIVE_ACK     0x08
#define ASAP_ENDPOINT_UNREACHABLE        0x09
#define ASAP_SERVER_ANNOUNCE             0x0a
#define ASAP_COOKIE                      0x0b
#define ASAP_COOKIE_ECHO                 0x0c
#define ASAP_BUSINESS_CARD               0x0d
#define ASAP_ERROR                       0x0e


#define ASAP_SERVER_ANNOUNCE_TIMEOUT    2000 /* should be 1000 */

#define BUFFER_SIZE (1<<16)

int
sendAsapAnnounceMsg(char *buf, size_t len);

int
sendAsapMsg(char *buf, size_t len, sctp_assoc_t assocId, int announcement);

/*
 * 2.2.1 ASAP_REGISTRATION
 */

struct asapRegistration {
    uint8 type;
    uint8 flags;
    uint16 length;
};

int
handleAsapRegistration(void *buf, ssize_t len, sctp_assoc_t assocId);

/*
 * 2.2.2 ASAP_DEREGISTRATION
 */

struct asapDeregistration {
    uint8 type;
    uint8 flags;
    uint16 length;
};

int
handleAsapDeregistration(void *buf, ssize_t len, sctp_assoc_t assocId);

/*
 * 2.2.3 ASAP_REGISTRATION_RESPONSE
 */

struct asapRegistrationResponse {
    uint8 type;
    uint8 flags;
    uint16 length;
};

size_t
initAsapRegistrationResponse(struct asapRegistrationResponse *msg, int rejectFlag);

int
sendAsapRegistrationResponse(sctp_assoc_t assocId, int rejectFlag, ServerPool pool, uint32 peIdentifier);

/*
 * 2.2.4 ASAP_DEREGISTRATION_RESPONSE
 */

struct asapDeregistrationResponse {
    uint8 type;
    uint8 flags;
    uint16 length;
};

size_t
initAsapDeregistrationResponse(struct asapDeregistrationResponse *msg);

int
sendAsapDeregistrationResponse(sctp_assoc_t assocId, ServerPool pool, uint32 peIdentifier);

/*
 * 2.2.5 ASAP_HANDLE_RESOLUTION
 * TODO: tcp? udp?
 */

struct asapHandleResolution {
    uint8 type;
    uint8 flags;
    uint16 length;
};

int
handleAsapHandleResolution(void *buf, ssize_t len, sctp_assoc_t assocId);

/*
 * 2.2.6 ASAP_HANDLE_RESOLUTION_RESPONSE
 */

struct asapHandleResolutionResponse {
    uint8 type;
    uint8 flags;
    uint16 length;
};

size_t
initAsapHandleResolutionResponse(struct asapHandleResolutionResponse *msg, int acceptUpdatesFlag);

int
sendAsapHandleResolutionResponse(int acceptUpdatesFlag, sctp_assoc_t assocId, ServerPool pool);

/*
 * 2.2.7 ASAP_ENDPOINT_KEEP_ALIVE
 */

struct asapEndpointKeepAlive {
    uint8 type;
    uint8 flags;
    uint16 length;
    uint32 serverId;
};

size_t
initAsapEndpointKeepAlive(struct asapEndpointKeepAlive *msg, uint32 serverId, int homeFlag);

int
sendAsapEndpointKeepAlive(PoolElement element, int homeFlag);

/*
 * 2.2.8 ASAP_ENDPOINT_KEEP_ALIVE_ACK
 */

struct asapEndpointKeepAliveAck {
    uint8 type;
    uint8 flags;
    uint16 length;
};

int
handleAsapEndpointKeepAliveAck(void *buf, ssize_t len, sctp_assoc_t assocId);

/*
 * 2.2.9 ASAP_ENDPOINT_UNREACHABLE
 */

struct asapEndpointUnreachable {
    uint8 type;
    uint8 flags;
    uint16 length;
};

int
handleAsapEndpointUnreachable(void *buf, ssize_t len, sctp_assoc_t assocId);

/*
 * 2.2.10 ASAP_SERVER_ANNOUNCE
 */

struct asapServerAnnounce {
    uint8 type;
    uint8 flags;
    uint16 length;
    uint32 serverId;
};

size_t
initAsapServerAnnounce(struct asapServerAnnounce *msg, uint32 serverId);

int
sendAsapServerAnnounce();

/*
 * 2.2.14 ASAP_ERROR
 */

struct asapError {
    uint8 type;
    uint8 flags;
    uint16 length;
};

size_t
initAsapError(struct asapError *msg);

int
sendAsapError();

int
handleAsapError(void *buf, ssize_t len, sctp_assoc_t assocId);

#endif /* _ASAP_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2007/12/02 22:13:05  volkmer
 * do not use asapUdpSendFdfixed handle registrationimplemented handle resolution and sendHandleResolutuin
 *
 * Revision 1.9  2007/11/07 12:57:07  volkmer
 * added missing prototypes
 *
 * Revision 1.8  2007/11/06 08:21:48  volkmer
 * changed signature of sendAsapEndpointKeepAlive
 *
 * Revision 1.7  2007/11/05 00:05:47  volkmer
 * reformated the copyright statementstarted implementing asapre- /deregistartion needs to be tested
 *
 **/
