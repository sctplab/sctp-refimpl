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


#ifndef __hlist_h__
#define __hlist_h__
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memcheck.h>
#include <return_status.h>
#include <HashedTbl.h>

struct hlist_hlink{
  struct hlist_hlink *prev;
  struct hlist_hlink *next;
  void* ent;
};

typedef struct hlist_hlink hlist_hlink;

typedef struct {
  hlist_hlink *head;
  hlist_hlink *tail;
  hlist_hlink *curr;
  HashedTbl   *entries;
  int wrapFlag;
  int cntIn;
}hlist_t;

/* default size of the hash table */
#define HLIST_DEFAULT_SIZE 256


/* This file is MUCH like the dlist object, with
 * one exception. It also maintains a hashed table
 * in parrallel with the list. This way you can
 * pull an entry directly without hunting through
 * the list. I.e. if you know element E is in the
 * list, you can simply do a hlist_pullThis(hlist, E);
 * and it will return E to you if sucessful. This
 * is useful for where you want to maintain order in
 * a list, but you also at times need to pull it from
 * the list. An example of this is where one would be
 * maintaining a list of say timers.
 */


/* create a hlist object to use */
hlist_t *
hlist_create(char *name, int sizeing);

/* This is called to destroy the linked list 
 * created by the hlist_create() function.
 * When you call this if the list is not
 * empty the hlist_clear() that is called will
 * wipe out all references in the list.
 */
void hlist_destroy(hlist_t *);

/* Clear all entries from the linked list.
 * This quite dangerous since it does not free
 * anything held in it, only the resources used by
 * it.
 */
void hlist_clear(hlist_t *);

/* 
 * This will pull out of the list the next entry in it.
 * The data returned is a pointer the void * saved in the list.
 */
void* hlist_getNext(hlist_t *);

/* 
 * Pull the next entry from the list returning the llist_slink
 * structure holding it. This turns over the allocated data structure
 * llist_slink to the caller. If you use this you are responsible
 * for freeing the llist_slink returned to you.
 */
hlist_hlink* hlist_getNextHlist(hlist_t *);

/* 
 * Look at the next item in the list. This leaves it on
 * the list. The appropriate index is moved so that
 * the next call after this one will pick up the
 * next item. This allows you to "walk" the list.
 */
void* hlist_get(hlist_t *);

/*
 * Look at the previous entry on the list. This does NOT
 * change the current pointer or advance the list. It is
 * also non-destructive. It does not pull previous from
 * the list.
 */
void* hlist_getPrev(hlist_t *);

/* 
 * Has one "walks" the list, you may determine you 
 * wish to pull out what you are looking at. This can
 * be accomplished by calling this function.
 */
void* hlist_getThis(hlist_t *);


/*
 * Pull the entry (item) that
 * was put into the list earlier.
 */
void* hlist_pullThis(hlist_t *, void *item);

/*
 * Postion the list so that item (which is on
 * the list) will be the next one retrieved. If
 * item is not on the list ! LIB_STATUS_OK is returned.
 * Note that if you do an insert, it will insert
 * to the previous entry since you have NOT yet
 * looked that the entry you moved to.
 */
int hlist_moveToThis(hlist_t *o, void *item);

/*
 * Return a boolean, true if item is in the
 * list, false if it is not.
 */
int hlist_isItInTheList(hlist_t *o, void *item);
  
/*
 * Pull the item (entry)'s hlink structure
 * from the list. You own the hlist_hlink
 * memory unless you stick it in another list.
 */
hlist_hlink* hlist_pullThis_hlink(hlist_t *, void *item);

/*
 * Has one "walks" the list, you may determine you
 * would like to "swap" out a entry in the list. This
 * can be done by calling this function. It will take
 * the void * you are passing and replace it, giving
 * you back the void * that was in the list.
 */
void* hlist_replaceThis(hlist_t *,void * entry);

/* 
 * Has you "walk" the list, this function will
 * let you pull the current item from the list
 * including the llist_slink structure. You are
 * responsible afterwards for freeing the memory
 * associated with the llist_slink structure.
 */
hlist_hlink* hlist_getThisHlist(hlist_t *);

/* Print to stdout the number of entries in the list.*/
void hlist_printCnt(hlist_t *);

/* 
 * rewind or reset the list to the top so that
 * when you walk it you start at the beginning.
 */
void hlist_reset(hlist_t *);

/* 
 * Insert this item into the list at the
 * very front of the list.
 */
int hlist_insert(hlist_t *,void* entry);

/*
 * Append this item to the very back of the list.
 */
int hlist_append(hlist_t *,void* entry);

/*
 * Append this hlink structure to the list, useful when
 * moving one element from one list to another. The llist_hlink
 * structure after the operation is given over to the list
 * and will be free()'d by the list.
 */
int hlist_appendhlink(hlist_t *,hlist_hlink* entry);

/* 
 * Place the entry into the list at the current
 * point. If the use does a hlist_reset() followed
 * by this call it is the same as if the user
 * did a hlist_insert(). This operation will
 * place the entry before the last one I looked
 * at.
 */
int hlist_insertHere(hlist_t *,void* entry);

/* 
 * Place the entry into the list at the
 * current point only append it there. So it
 * places it after the last one I looked at.
 */
int hlist_appendHere(hlist_t *,void* entry);

/* 
 * Debugging routine that will print
 * the list.
 */
void hlist_printList(hlist_t *);

/*
 * Debugging, audit the list and whine
 * about anything incorrect.
 */
void hlist_audit(hlist_t *o);

/* 
 * return the number of entries in
 * the list.
 */
int hlist_getCnt(hlist_t *);

/* Move the list to the end. This is
 * only safe if you call get() right after it.
 */
int hlist_getToTheEnd(hlist_t *);

/*
 * Move the current pointer back
 * one. 
 */
int hlist_backUpOne(hlist_t *);

/* 
 * Look at the last element on the list.
 */
void *hlist_lookLastOne(hlist_t *);

/* 
 * Look at the next to last element on the list.
 */
void *hlist_lookN2LastOne(hlist_t *);

#endif
