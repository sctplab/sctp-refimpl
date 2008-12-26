/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

$Header: /usr/sctpCVS/APPS/baselib/msgDirector.h,v 1.3 2008-12-26 14:45:12 randall Exp $
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
kmorneau@cisco.com

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/

#ifndef __msgDirector_h__
#define __msgDirector_h__

#include <distributor.h>
#include <HashedTbl.h>

typedef void *(*messageQuery)(void *,messageEnvolope *,HashedTbl *hash);

typedef struct messageDir{
  distributor *obj;
  HashedTbl *hash;
  void *objinfo;
  messageFunc mf;
  messageQuery query;
}messageDir;


/*
 * You must provide two functions to use this little object.
 * You first have a function that when given a hash table and a
 * messageEnvolope, it will look into the envolope at the message
 * and see if it has stored anything in the table for this
 * message. In other words if the message contained something
 * like a call reference number, this function KNOWS where that
 * call reference number is within the message. It finds it
 * And then hash's it against the table. If it has something
 * (say a statemachine object or some such) it returns the
 * pointer to that object. This function must match the
 * messageQuery signature above. 
 * 
 * The second function you provide is a standard messageFunc. This
 * could be like the main entry to the state machine or something
 * along those lines. This function would then get called as
 * if the distributor had called your function with the void * and
 * the message. 
 *
 * The concept here is to segregate the knowledge of message contents
 * into a non-library function that can do a lookup for you and
 * then distribute the call to the particulary object. It is a
 * kind of a second level nesting to keep the distributor from
 * having to call 1,000's of time your function with a message.
 * In other words, say I created 1,000 call object StateMachines
 * each state machine might want to subscribe to messages. This
 * means that each SM would get a call for a inbound message, say
 * call reference 5. Now we end up calling possibly 999 times 
 * all of the state functions looking for the one to consume the
 * message (by NULLing the data pointer). But with this little
 * redirection, instead we call one guy (the msgDirector) and it
 * then calls a user provided function that finds the object
 * we need. Returns this to the director. Then the director
 * calls just call reference 5's StateMachine. A nice level
 * of indirection for a very low cost... and we still keep
 * knowledge of what a message is OUT of the library.
 */
void
distribMessage(void *obj, messageEnvolope *msg);

/* You create one of these to register for messages
 * and then query your messageQuery function to find
 * the object associated with this message. It then
 * will call a message function that will then
 * pass the object in the void
 */
messageDir *
messageDir_create(distributor *o,
 		  void *objinfo, 
	 	  messageFunc mf, 
		  messageQuery qry,
		  int priority);

int
messageDir_destroy(messageDir *dir);

#endif



