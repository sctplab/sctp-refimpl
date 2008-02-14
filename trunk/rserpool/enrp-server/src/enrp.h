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
 * $Id: enrp.h,v 1.7 2008-02-14 15:10:16 volkmer Exp $
 *
 **/
#ifndef _ENRP_H
#define _ENRP_H 1

#include <sys/types.h>

#include "parameters.h"
#include "poolelement.h"
#include "serverpool.h"

/* to pack types in structs properly */
#pragma pack(1)

/* enrp message types */
#define ENRP_PRESENCE                0x01
#define ENRP_HANDLE_TABLE_REQUEST    0x02
#define ENRP_HANDLE_TABLE_RESPONSE   0x03
#define ENRP_HANDLE_UPDATE           0x04
#define ENRP_LIST_REQUEST            0x05
#define ENRP_LIST_RESPONSE           0x06
#define ENRP_INIT_TAKEOVER           0x07
#define ENRP_INIT_TAKEOVER_ACK       0x08
#define ENRP_TAKEOVER_SERVER         0x09
#define ENRP_ERROR                   0x0a

#define BUFFER_SIZE                  (1<<16)

#define ENRP_UPDATE_ACTION_ADD_PE    0x0000
#define ENRP_UPDATE_ACTION_DEL_PE    0x0001

/* default thresholds */
#define MAX_NUMBER_SERVER_HUNT      2 /* should be 3 */
#define TIMEOUT_SERVER_HUNT      1000 /* should be 2000 */
#define PEER_HEARTBEAT_CYCLE     2500 /* should be 30000 */
#define SERVER_ANNOUNCE_CYCLE    2500 /* should be 5000 */
#define MAX_TIME_LAST_HEARD      5000 /* should be 61000 */
#define MAX_TIME_NO_RESPONSE        3
#define MAX_BAD_PE_REPORT           3

#define ENRP_HANDLE_TABLE_REQUEST_OWN_BIT      0x01
#define ENRP_HANDLE_TABLE_RESPONSE_REJECT_BIT  0x01
#define ENRP_HANDLE_TABLE_RESPONSE_MORE_BIT    0x02
#define ENRP_LIST_RESPONSE_REJECT_BIT          0x01

/*
 *  Message structures
 */

struct enrpMsgHeader {
    uint8 type;
    uint8 flags;
    uint16 length;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpPresence {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpHandleTableRequest {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpHandleTableResponse {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpHandleUpdate {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
    uint16 updateAction;
    uint16 reserved;
};

struct enrpListRequest {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpListResponse {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

struct enrpInitTakeover {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
    uint32 targetServerId;
};

struct enrpInitTakeoverAck {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
    uint32 targetServerId;
};

struct enrpTakeoverServer {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
    uint32 targetServerId;
};

struct enrpError {
    struct msgHeader hdr;
    uint32 senderServerId;
    uint32 receiverServerId;
};

/*
 *  Message initialization function prototypes
 */

size_t
initEnrpPresence(struct enrpPresence *msg, uint32 receiverId);

size_t
initEnrpHandleTableRequest(struct enrpHandleTableRequest *msg, uint32 receiverId, int ownBit);

size_t
initEnrpHandleTableResponse(struct enrpHandleTableResponse *msg, uint32 receiverId, int moreBit, int rejectBit);

size_t
initEnrpHandleUpdate(struct enrpHandleUpdate *msg, uint32 receiverId, uint16 updateAction);

size_t
initEnrpListRequest(struct enrpListRequest *msg, uint32 receiverId);

size_t
initEnrpListResponse(struct enrpListResponse *msg, uint32 receiverId,int rejectBit);

size_t
initEnrpInitTakeover(struct enrpInitTakeover *msg, uint32 receiverId, uint32 targetId);

size_t
initEnrpInitTakeoverAck(struct enrpInitTakeoverAck *msg, uint32 receiverId, uint32 targetId);

size_t
initEnrpTakeoverServer(struct enrpTakeoverServer *msg, uint32 receiverId, uint32 targetId);

size_t
initEnrpError(struct enrpError *msg, uint32 receiverId);

/*
 *  Message sending function prototypes
 */

int
sendEnrpMsg(char *buf, size_t len, sctp_assoc_t assocId, int udpAnnounce);

ssize_t
sendEnrpAnnounceMsg(char *buf, size_t msgLen, int udpAnnounce);

int
sendEnrpPresence(uint32 receiverId, int sendInfo, sctp_assoc_t assocId);

int
sendEnrpHandleTableRequest(uint32 receiverId, int ownBit, sctp_assoc_t assocId);

int
sendEnrpHandleTableResponse(uint32 receiverId, int ownBit, int rejectBit, sctp_assoc_t assocId);

int
sendEnrpHandleUpdate(ServerPool pool, PoolElement element, uint16 handleAction);

int
sendEnrpListRequest(uint32 receiverId, sctp_assoc_t assocId);

int
sendEnrpListResponse(uint32 receiverId, int rejectBit, sctp_assoc_t assocId);

int
sendEnrpInitTakeover(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId);

int
sendEnrpInitTakeoverAck(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId);

int
sendEnrpTakeoverServer(uint32 receiverId, uint32 targetId, sctp_assoc_t assocId);

int
sendEnrpError(uint32 receiverId, sctp_assoc_t assocId, int causeId, char *paramBuf, size_t bufLen);

/*
 *  Message handling function prototypes
 */

int
handleEnrpPresence(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpHandleTableRequest(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpHandleTableResponse(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpHandleUpdate(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpListRequest(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpListResponse(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpInitTakeover(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpInitTakeoverAck(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpTakeoverServer(void *buf, ssize_t len, sctp_assoc_t assocId);

int
handleEnrpError(void *buf, ssize_t len, sctp_assoc_t assocId);

int
createAssocToPeer(Address *addrs, int addrCnt, uint16 port, uint32 serverId, sctp_assoc_t *assocId);

#endif

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2008/02/14 10:21:39  volkmer
 * removed enrp presence reply required bit
 *
 * Revision 1.5  2008/02/13 17:02:15  volkmer
 * fixed enrp flags stuff
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
 * Revision 1.15  2007/12/06 01:52:15  volkmer
 * moved peliftimeexpirytimeoutcallback
 *
 * Revision 1.14  2007/12/05 23:10:27  volkmer
 * changed createassoctopeer to use sctp_connectx
 * don't use special sendFd
 * fixed a bug in handle table response
 *
 * Revision 1.13  2007/11/19 22:38:08  volkmer
 * removed checksum param from enrp presence message
 *
 * Revision 1.12  2007/11/07 12:57:07  volkmer
 * added missing prototypes
 *
 * Revision 1.11  2007/11/05 00:04:27  volkmer
 * reformated the copyright statement
 * implemented sendenrphandleupdate
 *
 * Revision 1.10  2007/10/27 15:05:20  volkmer
 * removed debug macro
 *
 **/
