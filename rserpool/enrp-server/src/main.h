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
 * $Id: main.h,v 1.2 2008-02-14 10:21:39 volkmer Exp $
 *
 **/
#ifndef _MAIN_H
#define _MAIN_H 1

#include "backend.h"

int
initEnrpSctpSocket(char *enrpLocalAddr, uint16 port);

int
initEnrpUdpSocket(char *enrpAnnounceAddr, uint16 port);

int
initAsapSctpSocket(char *asapLocalAddr, uint16 port);

int
initAsapUdpSocket(char *asapAnnounceAddr, uint16 port);

int
initRegistrar();

int
printHelp(char *binName);

int
convertStringToAddr(char *str, Address *addr);

int
parseArgs(int argc, char **argv);

int
startRegistrarServerHunt();

int
checkForIpv6();

#endif /* _MAIN_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.9  2007/12/06 01:52:53  volkmer
 * some pretty printing
 *
 * Revision 1.8  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.7  2007/10/27 13:18:08  volkmer
 * removed debug macro
 *
 **/
