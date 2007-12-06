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
 * $Id: checksum.c,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/
#include "checksum.h"
#include "parameters.h"

checksumType
checksumCompute(checksumType sum, const char* buf, size_t size) {
    uint16 *ptr = (uint16 *) buf;

    while (size >= sizeof(*ptr)) {
        sum += *ptr++;
        size -= sizeof(*ptr);
    }

    /* add left-over byte, if any */
    if (size > 0)
        sum += * (uint8 *) ptr;

    return(sum);
}

checksumType
checksumAdd(const checksumType a, const checksumType b) {
    return a + b;
}

checksumType
checksumSub(const checksumType a, const checksumType b) {
    return a - b;
}

checksumTypeFolded
checksumFold(checksumType sum) {
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return (uint16) ~sum;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.7  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.6  2007/10/27 12:25:08  volkmer
 * added checksumtypes
 *
 **/
