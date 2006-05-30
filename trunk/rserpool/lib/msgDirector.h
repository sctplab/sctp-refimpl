/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/msgDirector.h,v 1.1 2006-05-30 18:02:02 randall Exp $


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



