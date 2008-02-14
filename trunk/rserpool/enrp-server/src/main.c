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
 * $Id: main.c,v 1.6 2008-02-14 15:14:18 volkmer Exp $
 *
 **/
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "main.h"
#include "debug.h"
#include "backend.h"
#include "cblib.h"
#include "enrp.h"
#include "asap.h"
#include "callbacks.h"
#include "registrarserver.h"

int
checkForIpv6() {
    int sd;

	useIpv6 = 0;
    sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (sd != -1) {
        useIpv6 = 1;
        close(sd);
        logDebug("ipv6 available");
        return useIpv6;
    }

    logDebug("ipv6 not available");
    return useIpv6;
}

int
parseArgs(int argc, char **argv) {
    int i;
    int printUsage = 0;
    char *enrpAnnounceAddr = NULL;
    char *enrpLocalAddr = NULL;
    char *asapAnnounceAddr = NULL;
    char *asapLocalAddr = NULL;
    uint16 enrpAnnouncePort = 0;
    uint16 enrpLocalPort = 0;
    uint16 asapAnnouncePort = 0;
    uint16 asapLocalPort = 0;

    for (i = 1; i < argc; i++) {
        if ((strncmp(argv[i], "--help", 6)) == 0) {
            printUsage = 1;
            break;
        }

        else if ((strncmp(argv[i], "--enrpRegistrarId=", 18)) == 0)
            ownId = atoi(&argv[i][18]);
		
        else if ((strncmp(argv[i], "--enrpAnnounceAddr=", 19)) == 0)
            enrpAnnounceAddr = &argv[i][19];
		
        else if ((strncmp(argv[i], "--enrpAnnouncePort=", 19)) == 0)
            enrpAnnouncePort = atoi(&argv[i][19]);

        else if ((strncmp(argv[i], "--enrpLocalAddr=", 16)) == 0)
            enrpLocalAddr = &argv[i][16];

        else if ((strncmp(argv[i], "--enrpLocalPort=", 16)) == 0)
            enrpLocalPort = atoi(&argv[i][16]);

        else if ((strncmp(argv[i], "--asapAnnounceAddr=", 19)) == 0)
            asapAnnounceAddr = &argv[i][19];

        else if ((strncmp(argv[i], "--asapAnnouncePort=", 19)) == 0)
            asapAnnouncePort = atoi(&argv[i][19]);

        else if ((strncmp(argv[i], "--asapLocalAddr=", 16)) == 0)
            asapLocalAddr = &argv[i][16];

        else if ((strncmp(argv[i], "--asapLocalPort=", 16)) == 0)
            asapLocalPort = atoi(&argv[i][16]);

        else if ((strncmp(argv[i], "--useEnrpMulticast=off", 22)) == 0)
            useEnrpAnnouncements = 0;

        else if ((strncmp(argv[i], "--useAsapMulticast=off", 22)) == 0)
            useAsapAnnouncements = 0;

        else if ((strncmp(argv[i], "--useIpv6=off", 13)) == 0)
            useIpv6 = 0;

        else if ((strncmp(argv[i], "--useLoopback=off", 17)) == 0)
            useLoopback = 0;

        else {
            logDebug("Bad argument: '%s'\n", argv[i]);
            printUsage = 1;
            break;
        }
    }

    if (printUsage) {
        printHelp(argv[0]);
        exit(-1);
    }

    logDebug("enrpRegistrarId:      0x%08x", ownId);
    logDebug("enrpLocalAddr:        %s", enrpLocalAddr);
    logDebug("enrpLocalPort:        %d", enrpLocalPort);
    logDebug("enrpAnnounceAddr:     %s", enrpAnnounceAddr);
    logDebug("enrpAnnouncePort:     %d", enrpAnnouncePort);
    logDebug("asapLocalAddr:        %s", asapLocalAddr);
    logDebug("asapLocalPort:        %d", asapLocalPort);
    logDebug("asapAnnounceAddr:     %s", asapAnnounceAddr);
    logDebug("asapAnnouncePort:     %d", asapAnnouncePort);
    logDebug("useEnrpAnnouncements: %d", useEnrpAnnouncements);
    logDebug("useAsapAnnouncements: %d", useAsapAnnouncements);
    logDebug("useIpv6:              %d", useIpv6);
    logDebug("useLoopback:          %d", useLoopback);

    /* init sockets with given information */
    initEnrpSctpSocket(enrpLocalAddr, enrpLocalPort);

    initAsapSctpSocket(asapLocalAddr, asapLocalPort);

    if (useEnrpAnnouncements)
        initEnrpUdpSocket(enrpAnnounceAddr, enrpLocalPort);

    if (useAsapAnnouncements)
        initAsapUdpSocket(asapAnnounceAddr, asapLocalPort);

    return 1;
}

int
printHelp(char *binName) {
    printf("Usage: %s\n\n", binName);
    printf("--enrpRegistrarId=<id>                 the enrp registrar id\n");
    printf("--enrpAnnounceAddr=<ipv4 or ipv6 addr> the enrp multicast group address\n");
    printf("--enrpAnnouncePort=<port>              the enrp multicast input port\n");
    printf("--enrpLocalAddr=<Addr>                 local address of enrp endpoint\n");
    printf("--enrpLocalPort=<port>                 local port of enrp endpoint\n");
    printf("--asapAnnounceAddr=<ipv4 or ipv6 addr> the asap multicast group address\n");
    printf("--asapAnnouncePort=<port>              the asap multicast input port\n");
    printf("--asapLocalAddr=<Addr>                 local address of asap endpoint\n");
    printf("--asapLocalPort=<port>                 local port of asap endpoint\n");
    printf("--useEnrpMulticast=off                 turn of the use of enrp multicast\n");
    printf("--useAsapMulticast=off                 turn of the use of asap multicast\n");
    printf("--useIpv6=off                          turn off the use of ipv6 generally\n");
    printf("--useLoopback=off                      do NOT bind to loopback addresses\n");
    printf("--help                                 print this help\n");
    printf("\n");

    return 1;
}

int
convertStringToAddr(char *str, Address *addr) {
    struct in_addr in4;
    struct in6_addr in6;

    memset(&in4, 0, sizeof(in4));
    memset(&in6, 0, sizeof(in6));
    memset(addr, 0, sizeof(Address));
	
	logDebug("trying to convert '%s'", str);

    if (str == NULL)
        return -1;

    if (inet_pton(AF_INET6, str, (void *) &in6) > 0) {
        addr->type = AF_INET6;
        addr->addr.in6 = in6;
		logDebug("converted to ipv6");

        return 1;
    } else if (inet_pton(AF_INET, str, (void *) &in4) > 0) {
        if (useIpv6) {
            ((uint32 *) &in6)[2] = htonl(0xffff);
            ((uint32 *) &in6)[3] = in4.s_addr;
            addr->type = AF_INET6;
            addr->addr.in6 = in6;

			logDebug("converted to ipv6");
            return 1;
        }

        addr->type = AF_INET;
        addr->addr.in4 = in4;

		logDebug("converted to ipv4");
        return 1;
    }

	logDebug("conversion failed");
    return -1;
}

int
initEnrpSctpSocket(char *enrpLocalAddr, uint16 port) {
    struct sockaddr_in6 in6;
    struct sockaddr_in  in4;
    Address addr;
    struct sctp_event_subscribe event;
    struct sctp_initmsg init;
    char addrBuf[INET6_ADDRSTRLEN];

    memset(addrBuf, 0, sizeof(addrBuf));

    if (convertStringToAddr(enrpLocalAddr, &addr) < 0) {
        if (useIpv6) {
            logDebug("setting enrpLocalAddr to in6addr_any for AF_INET6")
            addr.type = AF_INET6;
            addr.addr.in6 = in6addr_any;
        } else {
            logDebug("setting enrpLocalAddr to INADDR_ANY for AF_INET");
            addr.type = AF_INET;
            addr.addr.in4.s_addr = INADDR_ANY;
        }
    }

    if (port == 0)
        port = ENRP_SCTP_PORT;

    if (addr.type == AF_INET6) {
        memset(&in6, 0, sizeof(in6));
        in6.sin6_family = AF_INET6;
        in6.sin6_port = htons(port);
        in6.sin6_addr = addr.addr.in6;
#ifdef HAVE_SIN6_LEN
        in6.sin6_len = sizeof(in6);
#endif

        if ((enrpSctpFd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
            perror("ipv6 enrp sctp socket failed");
            exit(-1);
        }

        if (bind(enrpSctpFd, (struct sockaddr *) &in6, sizeof(in6)) < 0) {
            perror("ipv6 enrp sctp bind failed");
            exit(-1);
        }
        logDebug("enrp sctp socket %d is bound to %s:%d", enrpSctpFd, inet_ntop(AF_INET6, (void *) &in6.sin6_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else if (addr.type == AF_INET) {
        memset(&in4, 0, sizeof(in4));
        in4.sin_family = AF_INET;
        in4.sin_port = htons(port);
        in4.sin_addr = addr.addr.in4;
        #ifdef HAVE_SIN_LEN
            in4.sin_len = sizeof(in4);
        #endif

        if ((enrpSctpFd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
            perror("ipv4 enrp sctp socket failed"),
            exit(-1);
        }

        if (bind(enrpSctpFd, (struct sockaddr *) &in4, sizeof(in4)) < 0) {
            perror("ipv4 enrp sctp bind failed");
            exit(-1);
        }
        logDebug("enrp sctp socket %d is bound to %s:%d", enrpSctpFd, inet_ntop(AF_INET, (void *) &in4.sin_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else {
        logDebug("very wrong address type: %d", addr.type);
        exit(-1);
    }

    /* enable all events */
    memset(&event, 0, sizeof(event));
    event.sctp_association_event      = 1;
    event.sctp_shutdown_event         = 1;
    event.sctp_data_io_event          = 1;
    event.sctp_address_event          = 1;
    event.sctp_send_failure_event     = 1;
    event.sctp_peer_error_event       = 1;
    event.sctp_partial_delivery_event = 0;
   /*
    event.sctp_adaptation_layer_event = 0;
    event.sctp_authentication_event   = 0;
    event.sctp_stream_reset_events    = 0;
   */
    if (setsockopt(enrpSctpFd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
        perror("setsockopt sctp_event on enrp socket failed");
        exit(-1);
    }

    /* limit assocs to */
    init.sinit_num_ostreams   = 1;
    init.sinit_max_instreams  = 1;
    init.sinit_max_attempts   = 0;
    init.sinit_max_init_timeo = 0;
    if (setsockopt(enrpSctpFd, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init)) != 0) {
        perror("setsockopt sctp_init on enrp socket failed");
        exit(-1);
    }

    /* start listening */
    if (listen(enrpSctpFd, 10) < 0) {
        perror("listen on enrp port failed");
        exit(-1);
    }

    timerRegisterFdCallback(enrpSctpFd, enrpSctpCallback, NULL);
    enrpLocalPort = port;
    return 1;
}

int
initEnrpUdpSocket(char *enrpAnnounceAddrStr, uint16 port) {
    Address announceAddr;
    struct ipv6_mreq mreq6;
    struct ip_mreq   mreq;
    struct sockaddr_in6 in6;
    struct sockaddr_in  in4;
    char addrBuf[INET6_ADDRSTRLEN];
    const int on = 1;

    memset(addrBuf, 0, sizeof(addrBuf));

    if (convertStringToAddr(enrpAnnounceAddrStr, &announceAddr) < 0)
        announceAddr = enrpAnnounceAddr;

    if (port == 0)
        port = ENRP_UDP_PORT;

    if (announceAddr.type == AF_INET6) {
        if ((enrpUdpFd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("ipv6 enrp udp socket failed");
            return -1;
        }

#if defined(SO_REUSEPORT)
        if (setsockopt(enrpUdpFd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt reuseport");
#else
        if (setsockopt(enrpUdpFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt reuseaddr");
#endif

        if (setsockopt(enrpUdpFd, IPPROTO_IPV6, IP_MULTICAST_LOOP, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt multicast loop");

        memset(&in6, 0, sizeof(in6));
        in6.sin6_family = AF_INET6;
        in6.sin6_addr = in6addr_any;
        in6.sin6_port = htons(port);
#ifdef HAVE_SIN6_LEN
        in6.sin6_len = sizeof(struct sockaddr_in6);
#endif

        if (bind(enrpUdpFd, (struct sockaddr *) &in6, sizeof(struct sockaddr_in6)) < 0) {
            perror("ipv6 enrp udp bind failed");
            return -1;
        }
		
       mreq6.ipv6mr_multiaddr = announceAddr.addr.in6;
        mreq6.ipv6mr_interface = 0;

        if (setsockopt(enrpUdpFd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) < 0) {
            perror("setsockopt add membership on enrp udp port failed");
            return -1;
        }

        logDebug("enrpUdpFd is %d", enrpUdpFd);
        logDebug("enrpUdpFd is locally bound to %s:%d", inet_ntop(AF_INET6, (void *) &in6.sin6_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else if (announceAddr.type == AF_INET) {
        if ((enrpUdpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("ipv4 enrp udp socket failed");
            return -1;
        }

#if defined(SO_REUSEPORT)
        if (setsockopt(enrpUdpFd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, (size_t) sizeof(int)) != 0)
            perror("setsockopt reuseport");
#else
        if (setsockopt(enrpUdpFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (size_t) sizeof(int)) != 0)
            perror("setsockopt reuseaddr");
#endif
        if (setsockopt(enrpUdpFd, IPPROTO_IP, IP_MULTICAST_LOOP, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt multicast loop");

        memset(&in4, 0, sizeof(in4));
        in4.sin_family = AF_INET;
        in4.sin_addr.s_addr = htonl(INADDR_ANY);
        in4.sin_port = htons(port);
#ifdef HAVE_SIN_LEN
        in4.sin_len = sizeof(struct sockaddr_in);
#endif

        if (bind(enrpUdpFd, (struct sockaddr *) &in4, sizeof(struct sockaddr_in)) < 0) {
            perror("ipv4 enrp udp bind failed");
            return -1;
        }

        mreq.imr_multiaddr = announceAddr.addr.in4;
        mreq.imr_interface.s_addr = INADDR_ANY;

        if (setsockopt(enrpUdpFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("setsockopt add membership on enrp udp port failed");
            return -1;
        }

        logDebug("enrpUdpFd is %d", enrpUdpFd);
        logDebug("enrpUdpFd is locally bound to %s:%d", inet_ntop(AF_INET, (void *) &in4.sin_addr, addrBuf, INET_ADDRSTRLEN), port);
    }

    enrpAnnounceAddr = announceAddr;
    useEnrpAnnouncements = 1;

    timerRegisterFdCallback(enrpUdpFd, enrpUdpCallback, NULL);
    return 1;
}

int
initAsapSctpSocket(char *asapLocalAddr, uint16 port) {
    struct sockaddr_in6 in6;
    struct sockaddr_in  in4;
    Address addr;
    struct sctp_event_subscribe event;
    struct sctp_initmsg init;
    char addrBuf[INET6_ADDRSTRLEN];

    memset(addrBuf, 0, sizeof(addrBuf));

    if (convertStringToAddr(asapLocalAddr, &addr) < 0) {
        if (useIpv6) {
            logDebug("setting asapLocalAddr to in6addr_any for AF_INET6")
            addr.type = AF_INET6;
            addr.addr.in6 = in6addr_any;
        } else {
            logDebug("setting asapLocalAddr to INADDR_ANY for AF_INET");
            addr.type = AF_INET;
            addr.addr.in4.s_addr = INADDR_ANY;
        }
    }

    if (port == 0)
        port = ASAP_SCTP_PORT;

    if (addr.type == AF_INET6) {
        memset(&in6, 0, sizeof(in6));
        in6.sin6_family = AF_INET6;
        in6.sin6_port = htons(port);
        in6.sin6_addr = addr.addr.in6;
#ifdef HAVE_SIN6_LEN
        in6.sin6_len = sizeof(in6);
#endif

        if ((asapSctpFd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
            perror("ipv6 asap sctp socket failed");
            exit(-1);
        }

        if (bind(asapSctpFd, (struct sockaddr *) &in6, sizeof(in6)) < 0) {
            perror("ipv6 asap sctp bind failed");
            exit(-1);
        }
        logDebug("asap sctp socket %d is bound to %s:%d", asapSctpFd, inet_ntop(AF_INET6, (void *) &in6.sin6_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else if (addr.type == AF_INET) {
        memset(&in4, 0, sizeof(in4));
        in4.sin_family = AF_INET;
        in4.sin_port = htons(port);
        in4.sin_addr = addr.addr.in4;
        #ifdef HAVE_SIN_LEN
            in4.sin_len = sizeof(in4);
        #endif

        if ((asapSctpFd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
            perror("ipv4 asap sctp socket failed"),
            exit(-1);
        }

        if (bind(asapSctpFd, (struct sockaddr *) &in4, sizeof(in4)) < 0) {
            perror("ipv4 asap sctp bind failed");
            exit(-1);
        }
        logDebug("asap sctp socket %d is bound to %s:%d", asapSctpFd, inet_ntop(AF_INET, (void *) &in4.sin_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else {
        logDebug("very wrong address type: %d", addr.type);
        exit(-1);
    }

    /* enable all events */
    memset(&event, 0, sizeof(event));
    event.sctp_association_event      = 1;
    event.sctp_shutdown_event         = 1;
    event.sctp_data_io_event          = 1;
    event.sctp_address_event          = 1;
    event.sctp_send_failure_event     = 1;
    event.sctp_peer_error_event       = 1;
    event.sctp_partial_delivery_event = 0;
   /*
    event.sctp_adaptation_layer_event = 0;
    event.sctp_authentication_event   = 0;
    event.sctp_stream_reset_events    = 0;
   */
    if (setsockopt(asapSctpFd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
        perror("setsockopt sctp_event on asap socket failed");
        exit(-1);
    }

    /* limit assocs to */
    init.sinit_num_ostreams   = 1;
    init.sinit_max_instreams  = 1;
    init.sinit_max_attempts   = 0;
    init.sinit_max_init_timeo = 0;
    if (setsockopt(asapSctpFd, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init)) != 0) {
        perror("setsockopt sctp_init on asap socket failed");
        exit(-1);
    }

    /* start listening */
    if (listen(asapSctpFd, 10) < 0) {
        perror("listen on asap port failed");
        exit(-1);
    }

    timerRegisterFdCallback(asapSctpFd, asapSctpCallback, NULL);
    asapLocalPort = port;
    return 1;
}

int
initAsapUdpSocket(char *asapAnnounceAddrStr, uint16 port) {
    Address announceAddr;
    struct ipv6_mreq mreq6;
    struct ip_mreq   mreq;
    struct sockaddr_in6 in6;
    struct sockaddr_in  in4;
    char addrBuf[INET6_ADDRSTRLEN];
    const int on = 1;

    memset(addrBuf, 0, sizeof(addrBuf));

    if (convertStringToAddr(asapAnnounceAddrStr, &announceAddr) < 0)
        announceAddr = asapAnnounceAddr;

    if (port == 0)
        port = ASAP_UDP_PORT;

    if (announceAddr.type == AF_INET6) {
        if ((asapUdpFd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("ipv6 asap udp socket failed");
            return -1;
        }

#if defined(SO_REUSEPORT)
        if (setsockopt(asapUdpFd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt reuseport");
#else
        if (setsockopt(asapUdpFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt reuseaddr");
#endif

        if (setsockopt(asapUdpFd, IPPROTO_IPV6, IP_MULTICAST_LOOP, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt multicast loop");

        memset(&in6, 0, sizeof(in6));
        in6.sin6_family = AF_INET6;
        in6.sin6_addr = in6addr_any;
        in6.sin6_port = htons(port);
#ifdef HAVE_SIN6_LEN
        in6.sin6_len = sizeof(struct sockaddr_in6);
#endif

        if (bind(asapUdpFd, (struct sockaddr *) &in6, sizeof(struct sockaddr_in6)) < 0) {
            perror("ipv6 asap udp bind failed");
            return -1;
        }

        mreq6.ipv6mr_multiaddr = announceAddr.addr.in6;
        mreq6.ipv6mr_interface = 0;

        if (setsockopt(asapUdpFd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) < 0) {
            perror("setsockopt add membership on asap udp port failed");
            return -1;
        }

        logDebug("asapUdpFd is %d", asapUdpFd);
        logDebug("asapUdpFd is locally bound to %s:%d", inet_ntop(AF_INET6, (void *) &in6.sin6_addr, addrBuf, INET6_ADDRSTRLEN), port);
    } else if (announceAddr.type == AF_INET) {
        if ((asapUdpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            perror("ipv4 asap udp socket failed");
            return -1;
        }

#if defined(SO_REUSEPORT)
        if (setsockopt(asapUdpFd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, (size_t) sizeof(int)) != 0)
            perror("setsockopt reuseport");
#else
        if (setsockopt(asapUdpFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (size_t) sizeof(int)) != 0)
            perror("setsockopt reuseaddr");
#endif

        if (setsockopt(asapUdpFd, IPPROTO_IP, IP_MULTICAST_LOOP, (const void *) &on, (size_t)sizeof(int)) != 0)
            perror("setsockopt multicast loop");

        memset(&in4, 0, sizeof(in4));
        in4.sin_family = AF_INET;
        in4.sin_addr.s_addr = htonl(INADDR_ANY);
        in4.sin_port = htons(port);
#ifdef HAVE_SIN_LEN
        in4.sin_len = sizeof(struct sockaddr_in);
#endif

        if (bind(asapUdpFd, (struct sockaddr *) &in4, sizeof(struct sockaddr_in)) < 0) {
            perror("ipv4 asap udp bind failed");
            return -1;
        }

        mreq.imr_multiaddr = announceAddr.addr.in4;
        mreq.imr_interface.s_addr = INADDR_ANY;

        if (setsockopt(asapUdpFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("setsockopt add membership on asap udp port failed");
            return -1;
        }

        logDebug("asapUdpFd is %d", asapUdpFd);
        logDebug("asapUdpFd is locally bound to %s:%d", inet_ntop(AF_INET, (void *) &in4.sin_addr, addrBuf, INET_ADDRSTRLEN), port);
    }

    asapAnnounceAddr = announceAddr;
    useAsapAnnouncements = 1;

    return 1;
}

int
initRegistrar() {
    struct sockaddr *addrs;
    struct sockaddr *addr;
    struct sockaddr_in6 *pin6;
    struct sockaddr_in *pin4;
    char *offset;
    char addrBuf[INET6_ADDRSTRLEN + 1];
    struct timeval now;
    unsigned int seed;
    int addrCnt = 0;
    int i;

    logDebug("initializing own registrar");

    /* init own id */
	if (ownId == 0) {
		getTime(&now);
		seed = now.tv_sec | now.tv_usec;
		srand(seed);
		ownId = (uint32) rand();
		logDebug("server id is: 0x%08x", ownId);
	}

    this = registrarServerNew(ownId);
    registrarServerListAddServer(this);
    this->rsEnrpPort = enrpLocalPort;
    this->rsAsapPort = asapLocalPort;

    /* don't need a timeout timer for us */
    timerStop(this->rsLastHeardTimeout);
    timerDelete(this->rsLastHeardTimeout);
    this->rsLastHeardTimeout = NULL;
    this->rsLastHeardTimeoutCnt = 0;

    /* parse enrp sctp addrs */
    if ((addrCnt = sctp_getladdrs(enrpSctpFd, 0, &addrs)) > 0) {
        offset = (char *) addrs;
        logDebug("enrp sctp: we are locally bound to %d adresses", addrCnt);
        for (i = 0; i < addrCnt && i < MAX_ADDR_CNT; i++) {
            addr = (struct sockaddr *) offset;
            pin4 = (struct sockaddr_in  *) offset;
            pin6 = (struct sockaddr_in6 *) offset;

            if (addr->sa_family == AF_INET) {
                if (!useLoopback && pin4->sin_addr.s_addr == INADDR_LOOPBACK)
                    continue;

                this->rsEnrpAddr[i].type = AF_INET;
                memcpy(&this->rsEnrpAddr[i].addr.in4, &pin4->sin_addr, sizeof(struct in_addr));
                logDebug("addrs[%d]: AF_INET  %s", i, inet_ntop(AF_INET, (void *) &pin4->sin_addr, addrBuf, INET6_ADDRSTRLEN));
                offset += sizeof(struct sockaddr_in);
            } else if (addr->sa_family == AF_INET6) {
                if (!useLoopback && memcmp((const void *) &pin6->sin6_addr, (const void *) &in6addr_loopback, sizeof(struct in6_addr)) == 0)
                    continue;

                this->rsEnrpAddr[i].type = AF_INET6;
                memcpy(&this->rsEnrpAddr[i].addr.in6, &pin6->sin6_addr, sizeof(struct in6_addr));
                logDebug("addrs[%d]: AF_INET6 %s", i, inet_ntop(AF_INET6, (void *) &pin6->sin6_addr, addrBuf, INET6_ADDRSTRLEN));
                offset += sizeof(struct sockaddr_in6);
            }
        }

        sctp_freeladdrs(addrs);
    }

    /* parse asap sctp addrs */
    if ((addrCnt = sctp_getladdrs(asapSctpFd, 0, &addrs)) > 0) {
        offset = (char *) addrs;
        logDebug("asap sctp: we are locally bound to %d adresses", addrCnt);
        for (i = 0; i < addrCnt && i < MAX_ADDR_CNT; i++) {
            addr = (struct sockaddr *) offset;
            pin4 = (struct sockaddr_in  *) offset;
            pin6 = (struct sockaddr_in6 *) offset;

            if (addr->sa_family == AF_INET) {
                if (!useLoopback && pin4->sin_addr.s_addr == INADDR_LOOPBACK)
                    continue;

                this->rsAsapAddr[i].type = AF_INET;
                memcpy(&this->rsAsapAddr[i].addr.in4, &pin4->sin_addr, sizeof(struct in_addr));
                logDebug("addrs[%d]: AF_INET  %s", i, inet_ntop(AF_INET, (void *) &pin4->sin_addr, addrBuf, INET6_ADDRSTRLEN));
                offset += sizeof(struct sockaddr_in);
            } else if (addr->sa_family == AF_INET6) {
                if (!useLoopback && memcmp((const void *) &pin6->sin6_addr, (const void *) &in6addr_loopback, sizeof(struct in6_addr)) == 0)
                    continue;

                this->rsAsapAddr[i].type = AF_INET6;
                memcpy(&this->rsAsapAddr[i].addr.in6, &pin6->sin6_addr, sizeof(struct in6_addr));
                logDebug("addrs[%d]: AF_INET6 %s", i, inet_ntop(AF_INET6, (void *) &pin6->sin6_addr, addrBuf, INET6_ADDRSTRLEN));
                offset += sizeof(struct sockaddr_in6);
            }
        }

        sctp_freeladdrs(addrs);
    }

    return 1;
}

int
startRegistrarServerHunt() {
    serverHuntCnt = 0;

    enrpAnnounceTimer = timerCreate(enrpAnnounceTimeout, NULL, "enrp announce cycle timer");

    if (useEnrpAnnouncements) {
        logDebug("using announcements to find mentor peer");
        sendEnrpPresence(0, 1, 0);
        serverHuntTimer = timerCreate(serverHuntTimeout, NULL, "server hunt timer");
        timerStart(serverHuntTimer, TIMEOUT_SERVER_HUNT);

        return 1;
    }

    logDebug("skipped server hunt, no multicast available");
    registrarState = SERVICING;
    timerStart(enrpAnnounceTimer, PEER_HEARTBEAT_CYCLE);

    return 1;
}

int
main(int argc, char **argv) {
#ifdef DEBUG
    /* flush stderr */
    setvbuf(stdout, (char *) NULL, _IONBF, 0);
#endif /* DEBUG */

    /* init the callback library */
    initCblib();

    /* init global variables */
    initGlobals();

    /* check for ipv6 avilibility */
    checkForIpv6();

    /* parse arguments and set up sockets */
    parseArgs(argc, argv);

    /* init and add own registrar */
    initRegistrar();

    /* find other registrars */
    startRegistrarServerHunt();

    /* start actions by starting main event handle loop */
    handleEvents();

    /* practically unreachable code */
    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2008/02/14 10:21:39  volkmer
 * removed enrp presence reply required bit
 *
 * Revision 1.4  2008/02/13 17:02:48  volkmer
 * enhanced ipv6 checking
 *
 * Revision 1.3  2008/01/20 13:06:34  tuexen
 * Initialize multicast socket appropriately.
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
 * Revision 1.17  2007/12/06 01:52:53  volkmer
 * some pretty printing
 *
 * Revision 1.16  2007/12/05 23:06:42  volkmer
 * removed unneeded file descriptors
 *
 * Revision 1.15  2007/11/19 22:45:11  volkmer
 * added intialization of asap sockets, stored asap sctp information in own registrar struct
 *
 * Revision 1.14  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.13  2007/10/27 13:18:08  volkmer
 * removed debug macro
 *
 **/
