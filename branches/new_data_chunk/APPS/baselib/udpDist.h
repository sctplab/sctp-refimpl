/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

$Header: /usr/sctpCVS/APPS/baselib/udpDist.h,v 1.3 2008-12-26 14:45:12 randall Exp $

*/

/*
 * Copyright (C) 2002 Cisco Systems Inc,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@cisco.com

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/

#ifndef __udpDist_h__
#define __udpDist_h__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/param.h>
#include <netdb.h>

#include <netinet/ip6.h>
#include <netinet/icmp6.h>


#ifdef __Lynx__
#include <socket.h>
#include <sockio.h>
#else
#include <sys/socket.h>

#if ! ( defined(LINUX) || defined(TRU64) || defined(AIX))
#include <sys/sockio.h>
#endif

#endif
#include <time.h>
#include <string.h>

#if (defined (LINUX) || defined(TRU64))
#include <stropts.h>
#endif

#include <net/if.h>

#if ! (defined (LINUX))
#include <net/if_dl.h>
#endif

#include <errno.h>
#include <distributor.h>


/* 
 * This defines a udp hookup to the
 * base distributor. You can use the 
 * udp fd directly with sendto etc.
 * This will take messages, reformat them
 * and dump them up the distributor stack.
 *
 */

struct udpDist{
  distributor *dist;
  int udpfd;
};

/* The bigger this is the more stack you
 * use on the input side. 64k is the 
 * max you should make it though.
 */
#define UDPDIST_MAXMESSAGE 16384

#ifdef	__cplusplus
extern "C" {
#endif

struct udpDist *
createudpDist(distributor *dist,u_short port,u_short family);

void
destroyudpDist(struct udpDist *);

#ifdef	__cplusplus
}
#endif


#endif
