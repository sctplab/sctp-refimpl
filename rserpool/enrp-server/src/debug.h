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
 * $Id: debug.h,v 1.3 2008-02-14 10:22:44 volkmer Exp $
 *
 **/
#ifndef _DEBUG_H
#define _DEBUG_H 1

#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#define DEBUG           1
#define DEBUG_PRINT_BUF 0

#if DEBUG == 1
#define logDebug(fmt...)\
    {\
        struct timeval now;\
        gettimeofday(&now, NULL);\
        fprintf(stdout, "%ld:%06d ", now.tv_sec, (int) now.tv_usec);\
        fprintf(stdout, "%s:%d:%s - ", __FILE__, __LINE__, __FUNCTION__);\
        fprintf(stdout, fmt);\
        fprintf(stdout, "\n");\
    }

#if DEBUG_PRINT_BUF == 1
#define printBuf(buf, size, bufferName)\
    {\
        int i = 0;\
        logDebug("len: %d - %s", (int) size, bufferName);\
        fprintf(stdout, "0000 ");\
        do {\
                if (i % 8 == 0 && i != 0)\
                        fprintf(stdout, "\n%.4hx ", i);\
                else if (i % 4 == 0 && i != 0)\
                        fprintf(stdout, " ");\
                fprintf(stdout, "%x", (((buf)[i] >> 4) & 0x0f));\
                fprintf(stdout, "%x", ((buf)[i] & 0x0f));\
                fprintf(stdout, " ");\
        } while (++i < (int) size);\
        fprintf(stdout, "\n");\
    }

#else

#define printBuf(buf, size, bufferName) {}
#endif /* DEBUG_PRINT_BUF */

#else

#define logDebug(fmt...) {}
#define printBuf(buf, size, bufferName) {}

#endif /* DEBUG */

#endif /* _DEBUG_H */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2008/02/13 16:59:17  volkmer
 * disabled printBuf debugging
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.12  2007/12/02 22:08:18  volkmer
 * added length information to printBuf
 *
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:26:04  volkmer
 * removed debug macro for function name
 *
 **/
