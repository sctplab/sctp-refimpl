/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/udpDist.h,v 1.1 2006-05-30 18:02:03 randall Exp $


The SCTP reference implementation  is free software; 
you can redistribute it and/or modify it under the terms of 
the GNU Library General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

the SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  

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
