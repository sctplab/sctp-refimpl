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
 * $Id: backend.h,v 1.2 2008-02-14 15:05:00 volkmer Exp $
 *
 **/
#ifndef _BACKEND_H_
#define _BACKEND_H_ 1

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

#include "cblib.h"

#pragma pack(1)

#define OPERATION_SCOPE_SIZE          32
#define SERVER_DESCRIPTION_SIZE       32
#define MAX_ADDR_CNT                  32
#define POLICY_DESCRIPTION_SIZE      255

#define ENRP_ANNOUNCE_ADDR_V4 "239.0.0.51"
#define ASAP_ANNOUNCE_ADDR_V4 "239.0.0.50"

#define ENRP_ANNOUNCE_ADDR_V6 "ff08::51"
#define ASAP_ANNOUNCE_ADDR_V6 "ff08::50"

#define ENRP_ANNOUNCE_ADDR ENRP_ANNOUNCE_ADDR_V4
#define ASAP_ANNOUNCE_ADDR ASAP_ANNOUNCE_ADDR_V4

/* as defined in http://www.iana.org/assignments/port-numbers */
#define ENRP_SCTP_PPID        12
#define ENRP_UDP_PORT       9901
#define ENRP_SCTP_PORT      9901
#define ENRP_SCTP_TLS_PORT  9902

#define ASAP_SCTP_PPID        11
#define ASAP_TCP_PORT       3863
#define ASAP_UDP_PORT       3863
#define ASAP_SCTP_PORT      3863
#define ASAP_TCP_TLS_PORT   3864
#define ASAP_SCTP_TLS_PORT  3864

typedef uint8_t       uint8;
typedef uint16_t      uint16;
typedef uint32_t      uint32;

typedef int8_t        int8;
typedef int16_t       int16;
typedef int32_t       int32;

enum rsState {
    INITIALIZING = 0,
    SERVICING,
    SYNCHRONIZING
};

typedef struct address_struct Address;
struct address_struct  {
    int type;
    union {
        struct in_addr  in4;
        struct in6_addr in6;
    } addr;
};

struct msgHeader {
    uint8 type;
    uint8 flags;
    uint16 length;
};


extern int             registrarState;
extern uint32          mentorServerId;
extern uint32          ownId;

extern int             useIpv6;
extern int             useLoopback;

extern int             enrpSctpFd;
extern int             enrpUdpFd;
extern int             useEnrpAnnouncements;

extern int             asapSctpFd;
extern int             asapUdpFd;
extern int             useAsapAnnouncements;

extern uint16          enrpLocalPort;
extern Address         enrpAnnounceAddr;
extern uint16          enrpAnnouncePort;
extern Timer           enrpAnnounceTimer;

extern uint16          asapLocalPort;
extern Address         asapAnnounceAddr;
extern uint16          asapAnnouncePort;
extern Timer           asapAnnounceTimer;

extern int             serverHuntCnt;
extern Timer           serverHuntTimer;

#ifdef DEBUG
extern Timer           endTimer;
#endif /* DEBUG */

int
initGlobals();

#endif /* _BACKEND_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.15  2007/12/05 23:06:42  volkmer
 * removed unneeded file descriptors
 *
 * Revision 1.14  2007/11/19 22:42:18  volkmer
 * changed default asap multicast addr
 *
 * Revision 1.13  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.12  2007/10/27 12:21:54  volkmer
 * added some asap adresses
 *
 **/
