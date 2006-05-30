/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/llist.h,v 1.1 2006-05-30 18:02:02 randall Exp $


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

#ifndef __llist_h__
#define __llist_h__
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "return_status.h"

struct llist_slink{
  struct llist_slink *next;
  void* ent;
};

typedef struct llist_slink llist_slink;

/* This is a general purpose linked list. 
 * This is a basic a set of singlely linked list maniplulation tools. It handles
 * all manipulation of the linked list llist_t.
 */ 

typedef struct{
  llist_slink *last;
  llist_slink *n2last;
  llist_slink *curr;
  llist_slink *currm1;
  int wrapFlag;
  int cntIn;
}llist_t;

/* create a llist object to use */

llist_t *llist_create();

/* This is called to destroy the linked list 
 * created by the llist_create() function.
 * When you call this if the list is not
 * empty the llist_clear() that is called will
 * wipe out all references in the list.
 */
void llist_destroy(llist_t *);

/* Clear all entries from the linked list.
 * This quite dangerous since it does not free
 * anything held in it, only the resources used by
 * it.
 */
void llist_clear(llist_t *);

/* 
 * This will pull out of the list the next entry in it.
 * The data returned is a pointer the void * saved in the list.
 */
void* llist_getNext(llist_t *);

/* 
 * Pull the next entry from the list returning the llist_slink
 * structure holding it. This turns over the allocated data structure
 * llist_slink to the caller. If you use this you are responsible
 * for freeing the llist_slink returned to you.
 */
llist_slink* llist_getNextSlist(llist_t *);

/* 
 * Look at the next item in the list. This leaves it on
 * the list. The appropriate index is moved so that
 * the next call after this one will pick up the
 * next item. This allows you to "walk" the list.
 */
void* llist_get(llist_t *);


/* 
 * Has one "walks" the list, you may determine you 
 * wish to pull out what you are looking at. This can
 * be accomplished by calling this function.
 */
void* llist_getThis(llist_t *);

/*
 * Has one "walks" the list, you may determine you
 * would like to "swap" out a entry in the list. This
 * can be done by calling this function. It will take
 * the void * you are passing and replace it, giving
 * you back the void * that was in the list.
 */
void* llist_replaceThis(llist_t *,void *);

/* 
 * Has you "walk" the list, this function will
 * let you pull the current item from the list
 * including the llist_slink structure. You ware
 * responsible afterwards for freeing the memory
 * associated with the llist_slink structure.
 */
llist_slink* llist_getThisSlist(llist_t *);

/* Print to stdout the number of entries in the list.*/
void llist_printCnt(llist_t *);

/* 
 * rewind or reset the list to the top so that
 * when you walk it you start at the beginning.
 */
void llist_reset(llist_t *);

/* 
 * Insert this item into the list at the
 * very front of the list.
 */
int llist_insert(llist_t *,void*);

/*
 * Append this item to the very back of the list.
 */
int llist_append(llist_t *,void*);

/*
 * Append this slink structure to the list, useful when
 * moving one element from one list to another. The llist_slink
 * structure after the operation is given over to the list
 * and will be free()'d by the list.
 */
int llist_appendslink(llist_t *,llist_slink*);


/* 
 * Place the entry into the list at the current
 * point. If the use does a llist_reset() followed
 * by this call it is the same as if the user
 * did a llist_insert(). This operation will
 * place the entry before the last one I looked
 * at.
 */
int llist_insertHere(llist_t *,void*);


/* 
 * Place the entry into the list at the
 * current point only append it there. So it
 * places it after the last one I looked at.
 */
int llist_appendHere(llist_t *,void*);


/* 
 * Debugging routine that will print
 * the list.
 */
void llist_printList(llist_t *);

/* 
 * return the number of entries in
 * the list.
 */
int llist_getCnt(llist_t *);

/* Move the list to the end. This is
 * only safe if you call get() right after it.
 */
int llist_getToTheEnd();

/*
 * Move the current pointer back
 * one. This can be expensive, you
 * should use the dlist if you need to
 * do a lot of these :)
 */
int llist_backUpOne(llist_t *);

/* 
 * Look at the last element on the list.
 */
void *llist_lookLastOne(llist_t *);

/* 
 * Look at the next to last element on the list (expensive), use
 * a dlist if you need this one a lot.
 */
void *llist_lookN2LastOne(llist_t *);


#endif
