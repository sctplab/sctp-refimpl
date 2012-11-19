#ifndef __printDebug_h__
#define __printDebug_h__
/*
 * Copyright (C) 2008 Randall R. Stewart
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
the SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@lakerest.net

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/

#include <stdio.h>
/* print them all */
#define DEBUG_PRINTF_ON 1


#ifdef DEBUG_PRINTF_ON

extern FILE *print_output;

#define DEBUG_PRINTF_L1_ON 1
#define DEBUG_PRINTF_L2_ON 1
#define DEBUG_PRINTF_L3_ON 1

#define DEBUG_PRINTF(params...) do {		\
    fprintf(((print_output == NULL) ? stdout : print_output), params);	\
    fflush(((print_output == NULL) ? stdout : print_output));		\
  } while (0)

#else

#define DEBUG_PRINTF(params...)

#endif


/* level 1 and above */
#ifdef DEBUG_PRINTF_L1_ON
#define DEBUG_PRINTF_L2_ON 1
#define DEBUG_PRINTF_L3_ON 1

#define DEBUG_PRINTF_LVL1(params...) do {		\
    fprintf(((print_output == NULL) ? stdout : print_output), params);	\
    fflush(((print_output == NULL) ? stdout : print_output));		\
  } while (0)

#else

#define DEBUG_PRINTF_LVL1(params...)

#endif


/* level 2 and above */
#ifdef DEBUG_PRINTF_L2_ON
#define DEBUG_PRINTF_L3_ON 1

#define DEBUG_PRINTF_LVL2(params...) do {		\
    fprintf(((print_output == NULL) ? stdout : print_output), params);	\
    fflush(((print_output == NULL) ? stdout : print_output));		\
  } while (0)

#else

#define DEBUG_PRINTF_LVL2(params...)

#endif

/* level 3 */
#ifdef DEBUG_PRINTF_L3_ON

#define DEBUG_PRINTF_LVL3(params...) do {		\
    fprintf(((print_output == NULL) ? stdout : print_output), params);	\
    fflush(((print_output == NULL) ? stdout : print_output));		\
  } while (0)

#else

#define DEBUG_PRINTF_LVL3(params...)

#endif



#endif
