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

#include <byte_work.h>

void
byte_place_short_in_msg(uint8_t *msg_p, unsigned short val)
{
  /* This places shorts in network byte order in
   * a packed array someplace in the msg.
   */
  msg_p[0] = (uint8_t)((0xff00 & val) >> 16);
  msg_p[1] = (uint8_t)(0x00ff & val);
}


void
byte_place_int_in_msg(uint8_t *msg_p, unsigned int val)
{
  /* This places an int in network byte order in
   * a packed array someplace in the msg.
   */
  msg_p[0] = (uint8_t)((0xff000000 & val) >> 24);
  msg_p[1] = (uint8_t)((0x00ff0000 & val) >> 16);
  msg_p[2] = (uint8_t)((0x0000ff00 & val) >> 8);
  msg_p[3] = (uint8_t)(0x000000ff & val);
}



uint32_t
byte_extract_int_from_msg(uint8_t *msg_p)
{
  /* This places an int in network byte order in
   * a packed array someplace in the msg.
   */
  uint32_t v;
  v =  (msg_p[0] << 24);
  v |= (msg_p[1] << 16);
  v |= (msg_p[2] << 8);
  v |= (msg_p[3]);
  return (v);
}


uint16_t
byte_extract_short_from_msg(uint8_t *msg_p)
{
  /* This places shorts in network byte order in
   * a packed array someplace in the msg.
   */
  uint16_t v;
  v = (msg_p[0] << 8);
  v |= msg_p[1];
  return (v);
}
