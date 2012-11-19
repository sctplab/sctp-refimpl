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
 * $Id: util.c,v 1.3 2008-02-13 17:06:21 volkmer Exp $
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include "util.h"
#include "debug.h"
#include "enrp.h"
#include "asap.h"
#include "backend.h"
#include "callbacks.h"
#include "poolelement.h"
#include "registrarserver.h"

uint8
checkAndIdentifyMsg(char *msg, size_t len, sctp_assoc_t assocId) {
    struct msgHeader *hdr;
    uint8 type = 0;
    uint16 length;
    char *offset;
    uint16 paramType;
    size_t bufLen;
    int addrCnt;
    RegistrarServer server;
    uint32 senderServerId = 0;

    if (len < 12 || msg == NULL) {
        logDebug("msg is empty ot too small");
        return 0;
    }

    hdr = (struct msgHeader *) msg;
    type = hdr->type;
    length = ntohs(hdr->length);

    if (length != len) {
        logDebug("buffer length does not match paket length");
        /* TODO:
         * send error
         */
        return 0;
    }

    senderServerId = *((int *) msg + 1);
    server = registrarServerListGetServerByServerId(senderServerId);
    logDebug("msg of type %d and length %d received from 0x%08x", type, length, senderServerId);

    if (server != NULL) {
        if (server->rsEnrpAssocId != assocId) {
            server->rsEnrpAssocId = assocId;
            logDebug("added assocId %d to peer 0x%08x", assocId, senderServerId);
        }
    }

    if (msg)
        return type;

    /* TODO:
     * implement real checking!!
     */

    bufLen = len;
    offset = msg;

    switch(type) {
    case ENRP_PRESENCE:
        if (bufLen > sizeof(struct enrpPresence)) {
            offset += sizeof(struct enrpPresence);
            bufLen -= sizeof(struct enrpPresence);

            if (checkTLV(offset, bufLen) == PARAM_SERVER_INFORMATION) {
                offset += sizeof(struct paramServerInformation);
                bufLen -= sizeof(struct paramServerInformation);

                if (checkTLV(offset, bufLen) == PARAM_SCTP_TRANSPORT) {
                    offset += sizeof(struct paramSctpTransport);
                    bufLen -= sizeof(struct paramSctpTransport);

                    while ((paramType = checkTLV(offset, bufLen)) != 0) {
                        if (addrCnt == MAX_ADDR_CNT) {
                            logDebug("to much adresses in sctp transport param");
                            return 0;
                        }

                        if (paramType == PARAM_IPV4_ADDR) {
                            offset += sizeof(struct paramIpv4Addr);
                            bufLen -= sizeof(struct paramIpv4Addr);
                            addrCnt++;
                        } else if (paramType == PARAM_IPV6_ADDR) {
                            offset += sizeof(struct paramIpv6Addr);
                            bufLen -= sizeof(struct paramIpv6Addr);
                            addrCnt++;
                        } else {
                            return 0;
                            /* TODO:
                             * send param error
                             */
                        }
                    }
                } else {
                    return 0;
                    /* TODO:
                     * send param error
                     */
                }
            } else {
                return 0;
                /* TODO:
                 * send param error
                 */
            }
        } else {
            return 0;
            /* TODO:
             * send param error
             */
        }
        break;

    case ENRP_HANDLE_TABLE_REQUEST:
        if (bufLen != sizeof(struct enrpHandleTableRequest))
            return 0;
        break;

    case ENRP_HANDLE_TABLE_RESPONSE:
        if (bufLen > sizeof(struct enrpHandleTableResponse)) {
            offset += sizeof(struct enrpHandleTableResponse);
            bufLen -= sizeof(struct enrpHandleTableResponse);

            while ((paramType = checkTLV(offset, bufLen)) != PARAM_POOL_HANDLE) {
                offset += (size_t) ((struct paramHeader *) offset)->length;
                offset += 4 - (((struct paramHeader *) offset)->length) % 4;
                bufLen -= ((struct paramHeader *) offset)->length;
                bufLen -= 4 - (((struct paramHeader *) offset)->length) % 4;

                while ((paramType = checkTLV(offset, bufLen)) != PARAM_POOL_ELEMENT) {
                    offset += sizeof(struct paramPoolElement);
                    bufLen -= sizeof(struct paramPoolElement);

                    if ((paramType = checkTLV(offset, bufLen)) == PARAM_IPV6_ADDR) {
                        offset += sizeof(struct paramIpv6Addr);
                        bufLen -= sizeof(struct paramIpv6Addr);
                    }
                }
            }
        }
        break;

    case ENRP_HANDLE_UPDATE:
        break;

    case ENRP_LIST_REQUEST:
        break;

    case ENRP_LIST_RESPONSE:
        break;

    case ENRP_INIT_TAKEOVER:
        break;

    case ENRP_INIT_TAKEOVER_ACK:
        break;

    case ENRP_TAKEOVER_SERVER:
        break;

    case ENRP_ERROR:
        break;

    default:
        return 0;
    }

    return type;
}

int
sendBufferToPoolElement(char *offset, size_t bufSize, PoolElement *pe) {/* , int asapTransportExpected) {*/
    struct paramPoolElement *pelement;
    struct paramPoolMemberSelectionPolicy *policy;
    int length;
    Address userAddr[MAX_ADDR_CNT];
    int userAddrCnt;
    uint16 userPort;
    uint16 userTransportUse;
    uint8 userProtocol;
    int i;
    char *originalOffset = offset;

    Address asapAddr[MAX_ADDR_CNT];
    int asapAddrCnt;
    uint16 asapPort;
    uint16 asapTransportUse;
    uint8 asapProtocol;
    
    *pe = NULL;

    if (checkTLV(offset, bufSize) != PARAM_POOL_ELEMENT) {
        logDebug("buffer is not a pool element");
        return -1;
    }

    pelement = (struct paramPoolElement *) offset;
    offset += sizeof(struct paramPoolElement);
    bufSize -= sizeof(struct paramPoolElement);

    logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
    length = sendBufferToTransportAddresses(offset, bufSize, userAddr, &userAddrCnt, &userPort, &userTransportUse, &userProtocol);
    offset += length;
    bufSize -= length;
    logDebug("offset: %p, bufSize %d", offset, (int) bufSize);

    if (length < 0) {
        logDebug("user address parsing failed");
        return -1;
    }

    printBuf(offset, bufSize, "rest of buffer");

    if (checkTLV(offset, bufSize) == PARAM_POOL_MEMBER_SELECTION_POLICY) {
        policy = (struct paramPoolMemberSelectionPolicy *) offset;
        offset += ntohs(policy->hdr.length);
        bufSize -= ntohs(policy->hdr.length);
        logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
    } else {
        logDebug("no policy given:, %d", checkTLV(offset, bufSize));
        return -1;
    }
    
    if (bufSize > 0) {

        length = sendBufferToTransportAddresses(offset, bufSize,
                                                asapAddr, &asapAddrCnt,
                                                &asapPort, &asapTransportUse,
                                                &asapProtocol);

		offset += length;
		bufSize -= length;

        logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
        if (length < 0) {
            logDebug("asap address parsing failed");
            return -1;
        }
    }

    /*
     * init pe
     */
    *pe = poolElementNew(ntohl(pelement->peIdentifier));
    if (*pe) {
        (*pe)->peRegistrationLife      = ntohl(pelement->registrationLife);
        (*pe)->peHomeRegistrarServer   = ntohl(pelement->homeEnrpServerIdentifier);
        (*pe)->peUserTransportUse      = ntohs(userTransportUse);
        (*pe)->peUserTransportProtocol = userProtocol;
        (*pe)->peUserTransportPort     = userPort;
        for (i = 0; i < MAX_ADDR_CNT; i++) {
           (*pe)->peUserTransportAddr[i] = userAddr[i];
        }
        (*pe)->peAsapTransportPort     = userPort;
        for (i = 0; i < MAX_ADDR_CNT; i++) {
           (*pe)->peAsapTransportAddr[i] = userAddr[i];
        }
        (*pe)->pePolicyID              = ntohl(policy->policyType);
        (*pe)->pePolicyWeightLoad      = ntohs(policy->hdr.length) >= 12 ? ntohl(policy->reserved[0]) : 0;
        (*pe)->pePolicyLoadDegradation = ntohs(policy->hdr.length) >= 16 ? ntohl(policy->reserved[1]) : 0;

        poolElementPrint(*pe);
    }

    logDebug("offset: %p, bufSize %d", offset, (int) bufSize);
    return (int) (offset - originalOffset);
}

int
poolElementToSendBuffer(PoolElement pe, char *offset) {
    struct paramPoolElement *pelement;
    struct paramPoolMemberSelectionPolicy *policy;
    int length;
    char* originalOffset = offset;

    if (!pe) {
        logDebug("pe is NULL");
        return -1;
    }

    if (!offset) {
        logDebug("offset is NULL");
        return -1;
    }

    pelement = (struct paramPoolElement *) offset;
    initParamPoolElement(pelement,
                         pe->peIdentifier,
                         pe->peHomeRegistrarServer,
                         pe->peRegistrationLife);

    offset += sizeof(struct paramPoolElement);


    length = transportAddressesToSendBuffer(offset,
                                            pe->peUserTransportAddr,
                                            pe->peUserTransportPort,
                                            pe->peUserTransportUse,
                                            pe->peUserTransportProtocol);

    if (length == -1) {
        logDebug("user address to buffer conversion failed");
        return -1;
    } else {
        offset += length;
        pelement->hdr.length = htons(length + ntohs(pelement->hdr.length));
    }


    policy = (struct paramPoolMemberSelectionPolicy *) offset;
    length = initParamPoolMemberSelectionPolicy(policy, pe->pePolicyID, pe->pePolicyWeightLoad, pe->pePolicyLoadDegradation);
    if (length == -1) {
        logDebug("policy to buffer conversion failed");
        return -1;
    } else {
        offset += length;
        pelement->hdr.length = htons(length + ntohs(pelement->hdr.length));
    }


    length = transportAddressesToSendBuffer(offset,
                                            pe->peAsapTransportAddr,
                                            pe->peAsapTransportPort,
                                            TRANSPORT_USE_DATA_ONLY,
                                            IPPROTO_SCTP);

    if (length == -1) {
        logDebug("asap address to buffer conversion failed");
        return -1;
    } else {
        offset += length;
        pelement->hdr.length = htons(length + ntohs(pelement->hdr.length));
    }


    return (int)((long)offset - (long)originalOffset);;
}

int
transportAddressesToSendBuffer(char *offset, Address *addrs,
                               uint16 port, uint16 transportUse,
                               uint8 protocol) {
    struct paramSctpTransport *sctpTrans;
    struct paramUdpTransport *udpTrans;
    struct paramTcpTransport *tcpTrans;
    struct paramIpv4Addr *ipv4Addr;
    struct paramIpv6Addr *ipv6Addr;
    
    int ipv4AddrCnt = 0;
    int ipv6AddrCnt = 0;
    int length = 0;
    int i;
    char *storeOffset = offset;

    if (offset == NULL) {
        logDebug("offset is NULL");
        return -1;
    }

    switch ((int)protocol) {
    case IPPROTO_SCTP:
        sctpTrans = (struct paramSctpTransport *) offset;
        offset += sizeof(struct paramSctpTransport);
        initParamSctpTransport(sctpTrans, port, transportUse);

        for (i = 0; i < MAX_ADDR_CNT; i++) {
            if (addrs[i].type == AF_INET) {
                ipv4Addr = (struct paramIpv4Addr *) offset;
                offset += sizeof(struct paramIpv4Addr);
                length = initParamIpv4Addr(ipv4Addr, addrs[i].addr.in4);
                ipv4AddrCnt++;
            } else if(addrs[i].type == AF_INET6) {
                ipv6Addr = (struct paramIpv6Addr *) offset;
                offset += sizeof(struct paramIpv6Addr);
                length = initParamIpv6Addr(ipv6Addr, addrs[i].addr.in6);
                ipv6AddrCnt++;
            } else
                continue;

            sctpTrans->hdr.length = htons(length + ntohs(sctpTrans->hdr.length));
        }
        
        if ((ipv4AddrCnt == 0) && (ipv6AddrCnt == 0)) {
            logDebug("no adress was added to the sctp transport param");
            offset = storeOffset;
            return -1;
        }

        logDebug("added %d ipv4 addrs and %d ipv6 addrs to sctp transport parameter", ipv4AddrCnt, ipv6AddrCnt);
        return ntohs(sctpTrans->hdr.length);

    case IPPROTO_TCP:
        tcpTrans = (struct paramTcpTransport *) offset;
        offset += sizeof(struct paramTcpTransport);
        initParamTcpTransport(tcpTrans, port);

        if (addrs[0].type == AF_INET) {
            ipv4Addr = (struct paramIpv4Addr *) offset;
            offset += sizeof(struct paramIpv4Addr);
            length = initParamIpv4Addr(ipv4Addr, addrs[0].addr.in4);
            logDebug("added ipv4 addr to tcp transport parameter");
        } else if(addrs[0].type == AF_INET6) {
            ipv6Addr = (struct paramIpv6Addr *) offset;
            offset += sizeof(struct paramIpv6Addr);
            length = initParamIpv6Addr(ipv6Addr, addrs[0].addr.in6);
            logDebug("added ipv6 addr to tcp transport parameter");
        } else {
            logDebug("invalid address in addrs");
            return -1;
        }

        tcpTrans->hdr.length = htons(length + ntohs(tcpTrans->hdr.length));

        return ntohs(tcpTrans->hdr.length);

    case IPPROTO_UDP:
        udpTrans = (struct paramUdpTransport *) offset;
        offset += sizeof(struct paramUdpTransport);
        initParamUdpTransport(udpTrans, port);

        if (addrs[0].type == AF_INET) {
            ipv4Addr = (struct paramIpv4Addr *) offset;
            offset += sizeof(struct paramIpv4Addr);
            length = initParamIpv4Addr(ipv4Addr, addrs[0].addr.in4);
            logDebug("added ipv4 addr to udp transport parameter");
        } else if(addrs[0].type == AF_INET6) {
            ipv6Addr = (struct paramIpv6Addr *) offset;
            offset += sizeof(struct paramIpv6Addr);
            length = initParamIpv6Addr(ipv6Addr, addrs[0].addr.in6);
            logDebug("added ipv6 addr to tcp transport parameter");
        } else {
            logDebug("invalid address in addrs");
            return -1;
        }

        udpTrans->hdr.length = htons(length + ntohs(udpTrans->hdr.length));

        return ntohs(udpTrans->hdr.length);

    default:
        logDebug("unknown protocol");
        return -1;
    }

    return -1;
}

int
sendBufferToTransportAddresses(char *offset, size_t bufSize,
                               Address *addrs, int *addrCnt,
                               uint16 *port, uint16 *transportUse,
                               uint8 *protocol) {
    struct paramHeader *hdr;
    struct paramSctpTransport *sctpTrans;
    struct paramUdpTransport *udpTrans;
    struct paramTcpTransport *tcpTrans;
    struct paramIpv4Addr *ipv4Addr;
    struct paramIpv6Addr *ipv6Addr;
    Address addr[MAX_ADDR_CNT];
    uint16 paramType;
    int addressCnt = 0;
    int success;
    char* originalOffset = offset;
    int i;

    for (i = 0; i < MAX_ADDR_CNT; i++) {
       addrs[i].type = 0;
    }

    if (offset == NULL)
        return -1;

    if (bufSize < sizeof(struct paramHeader))
        return -1;

    hdr = (struct paramHeader *) offset;
    *addrCnt = 0;

    logDebug("offset: %p, bufSize: %d", offset, (int) bufSize);
    printBuf(offset, bufSize, "buffer");

    switch (ntohs(hdr->type)) {
    case PARAM_SCTP_TRANSPORT:
        logDebug("parsing sctp transport parameter");
        success = 0;
        if (checkTLV(offset, bufSize) == PARAM_SCTP_TRANSPORT) {
            size_t pos = sizeof(struct paramSctpTransport);
            size_t len = ntohs(hdr->length);

            sctpTrans = (struct paramSctpTransport *) offset;
            offset += sizeof(struct paramSctpTransport);
            bufSize -= sizeof(struct paramSctpTransport);
            
            logDebug("offset: %p, bufSize: %d", offset, (int) bufSize);

            success = 1;

            while (pos < len) {
                paramType = checkTLV(offset, bufSize);

                if (*addrCnt > MAX_ADDR_CNT) {
                    success = 1;
                    break;
                }

                if (paramType == PARAM_IPV4_ADDR) {
                    logDebug("parsing ipv4 parameter");

                    ipv4Addr = (struct paramIpv4Addr *) offset;
                    offset += sizeof(struct paramIpv4Addr);
                    bufSize -= sizeof(struct paramIpv4Addr);
                    pos += sizeof(struct paramIpv4Addr);
                    logDebug("offset: %p, bufSize: %d", offset, (int) bufSize);

					if (!useLoopback && ipv4Addr->addr.s_addr == INADDR_LOOPBACK) {
						logDebug("skipping ipv4 loopback address");
					} else {
						addr[addressCnt].type = AF_INET;
						addr[addressCnt].addr.in4 = ipv4Addr->addr;
						addressCnt++;
					}
                } else if (paramType == PARAM_IPV6_ADDR) {
                    logDebug("parsing ipv6 parameter");

                    ipv6Addr = (struct paramIpv6Addr *) offset;
                    bufSize -= sizeof(struct paramIpv6Addr);
                    offset += sizeof(struct paramIpv6Addr);
                    pos += sizeof(struct paramIpv6Addr);
                    logDebug("offset: %p, bufSize: %d", offset, (int) bufSize);

					if (!useLoopback && memcmp((const void *) &ipv6Addr->addr, (const void *) &in6addr_loopback, sizeof(struct in6_addr)) == 0) {
						logDebug("skipping ipv6 loopback address");
					} else {
						addr[addressCnt].type = AF_INET6;
						addr[addressCnt].addr.in6 = ipv6Addr->addr;
						addressCnt++;
					}
                } else {
                    logDebug("could not find any address in here");
                    success = 0;
                    break;
                    /* TODO:
                     * send param error
                     */
                }
            }
        } else {
            logDebug("no proper sctp transport param found");
            return -1;
        }

        if (success == 0) {
            logDebug("parsing failed, bufSize is %lu ", (unsigned long) bufSize);
            return -1;
        }

        *port = ntohs(sctpTrans->sctpPort);
        *transportUse = sctpTrans->transportUse;
        *protocol = IPPROTO_SCTP;
        *addrCnt = addressCnt;
        memcpy(addrs, &addr, sizeof(Address) * addressCnt);

        {
            int j;
            char addrBuf[INET6_ADDRSTRLEN];
            logDebug("parsed port: %d, parsed adresses:", *port);
            for (j = 0; j < *addrCnt; j++) {
                if (addrs[j].type == AF_INET6) {
                    inet_ntop(AF_INET6, (void *) &addrs[j].addr.in6, addrBuf, sizeof(addrBuf));
                    logDebug("%s", addrBuf);
                } else if (addrs[j].type == AF_INET) {
                    inet_ntop(AF_INET, (void *) &addrs[j].addr.in4, addrBuf, sizeof(addrBuf));
                    logDebug("%s", addrBuf);
                }
            }
        }

        logDebug("found %d addresses in sctp transport parameter ", *addrCnt);
        logDebug("offset: %p, offset: %d", offset, (int) bufSize);
        break;

    case PARAM_TCP_TRANSPORT:
        logDebug("parsing tcp transport parameter");
        if (checkTLV(offset, bufSize) == PARAM_TCP_TRANSPORT) {
            tcpTrans = (struct paramTcpTransport *) offset;
            offset += sizeof(struct paramTcpTransport);
            bufSize -= sizeof(struct paramTcpTransport);

            if ((paramType = checkTLV(offset, bufSize)) != 0) {
                if (paramType == PARAM_IPV4_ADDR) {
                    ipv4Addr = (struct paramIpv4Addr *) offset;
                    offset += sizeof(struct paramIpv4Addr);
                    bufSize -= sizeof(struct paramIpv4Addr);
                    addr[0].type = AF_INET;
                    addr[0].addr.in4 = ipv4Addr->addr;
                } else if (paramType == PARAM_IPV6_ADDR) {
                    ipv6Addr = (struct paramIpv6Addr *) offset;
                    offset += sizeof(struct paramIpv6Addr);
                    bufSize -= sizeof(struct paramIpv6Addr);
                    addr[0].type = AF_INET6;
                    addr[0].addr.in6 = ipv6Addr->addr;
                }
            }
        } else {
            logDebug("no proper tcp transport param found");
            return -1;
        }

        if (bufSize != 0) {
            logDebug("parsing failed, bufSize is %lu ", (unsigned long)bufSize);
        }

        *port = ntohs(tcpTrans->tcpPort);
        *transportUse = TRANSPORT_USE_DATA_ONLY;
        *protocol = IPPROTO_TCP;
        *addrCnt = 1;

        memcpy(addrs, &addr, sizeof(Address));
        break;

    case PARAM_UDP_TRANSPORT:
        logDebug("parsing udp transport parameter");
        if (checkTLV(offset, bufSize) == PARAM_UDP_TRANSPORT) {
            udpTrans = (struct paramUdpTransport *) offset;
            offset += sizeof(struct paramUdpTransport);
            bufSize -= sizeof(struct paramUdpTransport);

            if ((paramType = checkTLV(offset, bufSize)) != 0) {
                if (paramType == PARAM_IPV4_ADDR) {
                    ipv4Addr = (struct paramIpv4Addr *) offset;
                    offset += sizeof(struct paramIpv4Addr);
                    bufSize -= sizeof(struct paramIpv4Addr);
                    addr[0].type = AF_INET;
                    addr[0].addr.in4 = ipv4Addr->addr;
                } else if (paramType == PARAM_IPV6_ADDR) {
                    ipv6Addr = (struct paramIpv6Addr *) offset;
                    offset += sizeof(struct paramIpv6Addr);
                    bufSize -= sizeof(struct paramIpv6Addr);
                    addr[0].type = AF_INET6;
                    addr[0].addr.in6 = ipv6Addr->addr;
                }
            }
        } else {
            logDebug("no proper udp transport param found");
            return -1;
        }

        if (bufSize != 0) {
            logDebug("parsing failed, bufSize is %lu ", (unsigned long)bufSize);
            return -1;
        }

        *addrCnt = 1;
        *port = ntohs(udpTrans->udpPort);
        *transportUse = TRANSPORT_USE_DATA_ONLY;
        *protocol = IPPROTO_UDP;

        memcpy(addrs, &addr, sizeof(Address));
        break;

    default:
        logDebug("unknown transport parameter: %d ", ntohs(hdr->type));
        return -1;
    }

    logDebug("offset: %p, original: %p", offset, originalOffset);

    return (int)(offset - originalOffset);
}

uint16
checkTLV(char *pos, ssize_t bufSize) {
    struct paramHeader *hdr;

    if (pos == NULL)
        return 0;

    if (bufSize < (ssize_t)sizeof(struct paramHeader))
        return 0;

    hdr = (struct paramHeader *) pos;

    if (ntohs(hdr->length) > bufSize)
        return 0;

    return ntohs(hdr->type);
}


/*
 * assume, that buf is allocated big enough
 */
int
addressesToSysCallBuffer(Address *addrs, uint16 port, int addrCnt, char *buf) {
    int count = 0;
    int size = 0;
    int i;
    char *offset;
    struct sockaddr_in  *in4;
    struct sockaddr_in6 *in6;

    if (addrs == NULL) {
        logDebug("addrs is NULL");
        return -1;
    }

    if (addrCnt < 1) {
        logDebug("addrCnt is less than 1");
        return -1;
    }

    if (buf == NULL) {
        logDebug("buf is NULL");
        return -1;
    }

    memset(buf, 0, addrCnt * sizeof(struct sockaddr_in6));

    offset = buf;
    for (i = 0; i < addrCnt && i < MAX_ADDR_CNT; i++) {
        if (addrs[i].type == AF_INET) {
            in4 = (struct sockaddr_in *) offset;
            in4->sin_family = AF_INET;
            in4->sin_port = htons(port);
#ifdef HAVE_SIN_LEN
            in4->sin_len = sizeof(struct sockaddr_in);
#endif
            memcpy(&in4->sin_addr, &addrs[i].addr.in4, sizeof(struct in_addr));
            printBuf(offset, 16, "ipv4");
            offset += sizeof(struct sockaddr_in);
            size += sizeof(struct sockaddr_in);
            count++;
            continue;
        } else if (addrs[i].type == AF_INET6) {
            in6 = (struct sockaddr_in6 *) offset;
            in6->sin6_family = AF_INET6;
            in6->sin6_port = htons(port);
#ifdef HAVE_SIN6_LEN
            in6->sin6_len = sizeof(struct sockaddr_in6);
#endif
            memcpy(&in6->sin6_addr, &addrs[i].addr.in6, sizeof(struct in6_addr));
            printBuf(offset, 28, "ipv6");
            offset += sizeof(struct sockaddr_in6);
            size += sizeof(struct sockaddr_in6);
            count++;
            continue;
        } else {
            logDebug("unsupported address family: %d", addrs[i].type);
            return -1;
        }
    }

    printBuf(buf, size, "address buffer");

    return count;
}

int
sysCallBufferToAddresses(char *buf, size_t bufLen, Address *addrs) {
    int addrCnt;
    int i;
    size_t size;
    char *offset;
    struct sockaddr     *in;
    struct sockaddr_in  *in4;
    struct sockaddr_in6 *in6;

    if (buf == NULL) {
        logDebug("buf is NULL");
        return -1;
    }

    if (addrs == NULL) {
        logDebug("addrs is NULL");
        return -1;
    }

    if (bufLen < sizeof(struct sockaddr_in)) {
        logDebug("buf is too small");
        return -1;
    }

    addrCnt = 0;
    size = 0;
    offset = buf;
    for (i = 0; i < MAX_ADDR_CNT; i++) {
        if (size + sizeof(struct sockaddr) > bufLen)
            break;
        in = (struct sockaddr *) offset;
        switch (in->sa_family) {
        case AF_INET:
            if (size + sizeof(struct sockaddr_in) > bufLen)
                return addrCnt;
            in4 = (struct sockaddr_in *) offset;
            addrs[i].type = AF_INET;
            memcpy(&addrs[i].addr.in4, &in4->sin_addr, sizeof(struct in_addr));
            break;
        case AF_INET6:
            if (size + sizeof(struct sockaddr_in6) > bufLen)
                return addrCnt;
            in6 = (struct sockaddr_in6 *) offset;
            addrs[i].type = AF_INET6;
            memcpy(&addrs[i].addr.in6, &in6->sin6_addr, sizeof(struct in6_addr));
            break;
        default:
            logDebug("unknown type: %d", in->sa_family);
            return addrCnt;
        }
    }

    return addrCnt;
}

int
addressListPrint(Address *addrs) {
    char addrBuf[INET6_ADDRSTRLEN];
    int i;

    if (addrs == NULL) {
        logDebug("addrs is NULL");
        return -1;
    }

    for (i = 0; i < MAX_ADDR_CNT; i++) {
        if (addrs[i].type == AF_INET)
            logDebug("addrs[%d]: AF_INET  %s", i, inet_ntop(AF_INET, (void *) &addrs[i].addr.in4, addrBuf, INET6_ADDRSTRLEN));
        if (addrs[i].type == AF_INET6)
            logDebug("addrs[%d]: AF_INET6 %s", i, inet_ntop(AF_INET6, (void *) &addrs[i].addr.in6, addrBuf, INET6_ADDRSTRLEN));
    }

    return 1;
}

void
enterServicingMode() {
    logDebug("server hunt finished: entering normal servicing mode");
    timerDelete(serverHuntTimer);
    registrarState = SERVICING;
    serverHuntTimer = NULL;
    timerStart(enrpAnnounceTimer, PEER_HEARTBEAT_CYCLE);
    
    logDebug("starting asap server announcements with cycletime %dms", ASAP_SERVER_ANNOUNCE_TIMEOUT);
    asapAnnounceTimer = timerCreate(asapAnnounceTimeout, NULL, "asap announce cycle timer");
    timerStart(asapAnnounceTimer, ASAP_SERVER_ANNOUNCE_TIMEOUT);
}

#ifndef HAVE_SCTP_SENDX

/* ###### Get socklen for given address ################################## */
static size_t getSocklen(const struct sockaddr* address)
{
   switch(address->sa_family) {
      case AF_INET:
         return(sizeof(struct sockaddr_in));
       break;
      case AF_INET6:
         return(sizeof(struct sockaddr_in6));
       break;
      default:
         abort();
       break;
   }
}

#define AS_UNSPECIFIED          0
#define AS_MULTICAST_NODELOCAL  1
#define AS_LOOPBACK             2
#define AS_MULTICAST_LINKLOCAL  3
#define AS_UNICAST_LINKLOCAL    4
#define AS_MULTICAST_SITELOCAL  5
#define AS_UNICAST_SITELOCAL    6
#define AS_MULTICAST_ORGLOCAL   7
#define AS_MULTICAST_GLOBAL     8
#define AS_UNICAST_GLOBAL      10

/* ###### Get IPv4 address scope ######################################### */
static unsigned int getScopeIPv4(const uint32_t* address)
{
    uint32_t a;
    uint8_t  b1;
    uint8_t  b2;

    /* Unspecified */
    if(*address == 0x00000000) {
       return(0);
    }

    /* Loopback */
    a = ntohl(*address);
    if((a & 0x7f000000) == 0x7f000000) {
       return(2);
    }

    /* Class A private */
    b1 = (uint8_t)(a >> 24);
    if(b1 == 10) {
       return(AS_UNICAST_SITELOCAL);
    }

    /* Class B private */
    b2 = (uint8_t)((a >> 16) & 0x00ff);
    if((b1 == 172) && (b2 >= 16) && (b2 <= 31)) {
       return(AS_UNICAST_SITELOCAL);
    }

    /* Class C private */
    if((b1 == 192) && (b2 == 168)) {
       return(AS_UNICAST_SITELOCAL);
    }

    if(IN_MULTICAST(ntohl(*address))) {
       return(AS_MULTICAST_GLOBAL);
    }
    return(AS_UNICAST_GLOBAL);
}


/* ###### Get IPv6 address scope ######################################### */
static unsigned int getScopeIPv6(const struct in6_addr* address)
{
   if(IN6_IS_ADDR_V4MAPPED(address)) {
#if defined __linux__
      return(getScopeIPv4(&address->s6_addr32[3]));
#else
      return(getScopeIPv4(&address->__u6_addr.__u6_addr32[3]));
#endif
   }
   if(IN6_IS_ADDR_UNSPECIFIED(address)) {
      return(AS_UNSPECIFIED);
   }
   else if(IN6_IS_ADDR_MC_NODELOCAL(address)) {
      return(AS_MULTICAST_NODELOCAL);
   }
   else if(IN6_IS_ADDR_LOOPBACK(address)) {
      return(AS_LOOPBACK);
   }
   else if(IN6_IS_ADDR_MC_LINKLOCAL(address)) {
      return(AS_MULTICAST_LINKLOCAL);
   }
   else if(IN6_IS_ADDR_LINKLOCAL(address)) {
      return(AS_UNICAST_LINKLOCAL);
   }
   else if(IN6_IS_ADDR_MC_SITELOCAL(address)) {
      return(AS_MULTICAST_SITELOCAL);
   }
   else if(IN6_IS_ADDR_SITELOCAL(address)) {
      return(AS_UNICAST_SITELOCAL);
   }
   else if(IN6_IS_ADDR_MC_ORGLOCAL(address)) {
      return(AS_MULTICAST_ORGLOCAL);
   }
   else if(IN6_IS_ADDR_MC_GLOBAL(address)) {
      return(AS_MULTICAST_GLOBAL);
   }
   return(AS_UNICAST_GLOBAL);
}


/* ###### Get address scope ############################################## */
static unsigned int getScope(struct sockaddr* address)
{
   if(address->sa_family == AF_INET) {
      return(getScopeIPv4((uint32_t*)&((struct sockaddr_in*)address)->sin_addr));
   }
   else if(address->sa_family == AF_INET6) {
      return(getScopeIPv6(&((struct sockaddr_in6*)address)->sin6_addr));
   }
   else {
      abort();
   }
   return(0);
}

/* ###### Get one address of highest scope from address array ############ */
static struct sockaddr* getBestScopedAddress(struct sockaddr* addrs,
                                             int                    addrcnt)
{
    struct sockaddr* bestScopedAddress = addrs;
    unsigned int           bestScope         = getScope(addrs);
    unsigned int           newScope;
    struct sockaddr* a;
    int                    i;

    a = addrs;
    for (i = 1; i < addrcnt; i++) {
       a = (struct sockaddr*)((long)a + (long)getSocklen(a));
       newScope = getScope(a);
       if (newScope > bestScope) {
           bestScopedAddress = a;
           bestScope   = newScope;
       }
    }

   return bestScopedAddress;
}

ssize_t sctp_sendx(int                           sd,
                   const void*                   data,
                   size_t                        len,
                   struct sockaddr*              addrs,
                   int                           addrcnt,
                   struct sctp_sndrcvinfo*       sinfo,
                   int                           flags)
{
   struct sctp_sndrcvinfo* sri;
   struct iovec            iov[1];
   struct cmsghdr*         cmsg;
   char                    cbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
   struct msghdr           msg; 
   struct sockaddr* bestScopedAddress;
 
   iov[0].iov_base = (char *)data;
   iov[0].iov_len = len;

   bestScopedAddress = getBestScopedAddress(addrs, addrcnt);
   msg.msg_name = (struct sockaddr*)bestScopedAddress;
   msg.msg_namelen = getSocklen(bestScopedAddress);
   msg.msg_iov = iov;
   msg.msg_iovlen = 1;
   msg.msg_control = (void *)cbuf;

   cmsg = (struct cmsghdr*)cbuf;
   cmsg->cmsg_len   = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
   cmsg->cmsg_level = IPPROTO_SCTP;
   cmsg->cmsg_type  = SCTP_SNDRCV;

   sri = (struct sctp_sndrcvinfo*)CMSG_DATA(cmsg);
   memcpy(sri, sinfo, sizeof(struct sctp_sndrcvinfo));

   return(sendmsg(sd, &msg, flags));
}

#endif

/*
 * $Log: not supported by cvs2svn $
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
 * Revision 1.27  2007/12/05 23:13:06  volkmer
 * refactored enterservicingmode to util.c
 *
 * Revision 1.26  2007/12/02 22:16:14  volkmer
 * fixed and reworked sendBufferToPoolElement and sendBufferToTransportAddresses
 *
 * Revision 1.25  2007/11/19 22:34:52  volkmer
 * adapted transport to sendbuffer conversion and vice versa to new transport parameters
 *
 * Revision 1.24  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.23  2007/10/27 13:30:49  volkmer
 * removed debug macro
 *
 **/
