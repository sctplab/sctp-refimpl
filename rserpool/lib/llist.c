/*	$Header: /usr/sctpCVS/rserpool/lib/llist.c,v 1.1 2006-05-30 18:02:02 randall Exp $ */

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

#include "llist.h"

llist_t *
llist_create()
{
  llist_t *o;

  o = calloc(1,sizeof(llist_t));
  if(o == NULL)
    return(NULL);
  o->curr = o->currm1 = NULL;
  o->cntIn = 0;
  o->n2last = o->last = NULL;
  o->wrapFlag = 0;
  return(o);
}

void
llist_destroy(llist_t *o)
{
  llist_clear(o);
  free(o);
}

int
llist_append(llist_t *o,void* a)
{
  llist_slink *tmp;
  if(o->last){
    /* entries in the list */
    o->n2last = o->last;
    tmp = calloc(1,sizeof(llist_slink));
    if(tmp == NULL){
      return(LIB_STATUS_BAD);
    }
    tmp->ent = a;
    tmp->next = o->last->next;
    o->last->next = tmp;
    o->last = tmp;
  }else{
    /* no entires in the list */
    o->n2last = o->last = calloc(1,sizeof(llist_slink));
    if(o->last == NULL){
      return(LIB_STATUS_BAD);
    }
    o->last->ent = a;
    o->last->next = NULL;
    o->last->next = o->last;
  }
  o->cntIn++;
  return(LIB_STATUS_GOOD);
}

int
llist_appendslink(llist_t *o,llist_slink* a)
{

  if(a == NULL)
    /* We don't allow a NULL llist_slink struct to be entered in
     * the list
     */
    return(LIB_STATUS_BAD);

  if(o->last){
    /* one or more entries in the list */
    o->n2last = o->last;
    a->next = o->last->next;
    o->last->next = a;
    o->last = a;
  }else{
    /* no entries in list. */
    o->n2last = o->last = a;
    o->last->next = o->last;
  }
  o->cntIn++;
  return(LIB_STATUS_GOOD);
}

int
llist_insert(llist_t *o,void* a)
{
  llist_slink *tmp;
  if(o->last){
    tmp = calloc(1,sizeof(llist_slink));
    if(tmp == NULL){
      return(LIB_STATUS_BAD);
    }
    tmp->ent = a;
    tmp->next = o->last->next;
    o->last->next = tmp;
  }else{
    o->n2last = o->last = calloc(1,sizeof(llist_slink));
    if(o->last == NULL){
      return(LIB_STATUS_BAD);
    }
    o->last->ent = a;
    o->last->next = NULL;
    o->last->next = o->last;
  }
  o->cntIn++;
  return(LIB_STATUS_GOOD);
}

void
llist_clear(llist_t *o)
{
  void *pp;

  while((pp = llist_getNext(o)) != NULL){
    pp = NULL;
  }
  o->n2last = o->last = NULL;
  o->curr = o->currm1 = NULL;
}

void*
llist_getNext(llist_t *o)
{
  llist_slink *f;
  void *r;
  /* check if we have any in the list */
  if(o->last == NULL)
    return(NULL);

  /* ok save off some pointers */
  f = o->last->next;
  r = f->ent;
  f->ent = NULL;

  /* now is this the last one? */
  if(f == o->last){
    /* yep, none left after this */
    o->n2last = o->last = NULL;
    o->curr = o->currm1 = NULL;
  }else{
    if(o->n2last == o->last->next){
      /* Yes, Removing next to last one. We
       * cheat here and just move n2last to
       * the last one.
       */
      o->n2last = o->last;
    }
    o->last->next = f->next;
    f->next = NULL;
  }
  /* free up the memory */
  free(f);
  f = NULL;
  /* reduce number in */
  o->cntIn--;
  /* and out goes the stored entry */
  return(r);
}

llist_slink*
llist_getNextSlist(llist_t *o)
{
  llist_slink* f;
  /* is there one to pull? */
  if(o->last == NULL)
    return(NULL);

  f = o->last->next;
  if(f == o->last){
    /* last one turn off the lights */
    o->n2last = o->curr = o->currm1 = o->last = NULL;
  }else{
    if(o->last->next == o->n2last){
      /* cheat again */
      o->n2last = o->last;
    }
    o->last->next = f->next;
  }
  f->next = NULL;
  o->cntIn--;
  return(f);
}


void*
llist_get(llist_t *o)
{
  /* any in the list? */
  if(o->last == NULL)
    /* no */
    return(NULL);

  if(o->wrapFlag){
    /* we have wrapped.  we only return NULL once  at wrap. */
    o->wrapFlag = 0;
    /* setup so we will rewind to start */
    o->curr = o->currm1 = NULL;
    return(NULL);
  }

  if((o->curr == NULL) || (o->currm1 == NULL)){
    /* rewind to start */
    o->curr = o->last->next;
    o->currm1 = o->last;
    if(o->curr == o->last){
      /* only one in list so set the wrapFlag */
      o->wrapFlag = 1;
    }
    return(o->curr->ent);
  }
  /* ok setup to return the next value */
  o->currm1 = o->curr;
  o->curr = o->curr->next;
  if(o->curr == NULL){
    return(NULL);
  }
  if(o->curr == o->last){
    /* gotten all the way around so set the flag */
    o->wrapFlag = 1;
  }
  return(o->curr->ent);
}

void*
llist_getThis(llist_t *o)
{
  void* r;
  /* some in list? */
  if(o->last == NULL)
    /* no */
    return(NULL);

  if((o->curr == NULL) || (o->currm1 == NULL)){
    /* start at the beginning */
    o->curr = o->last->next;
    o->currm1 = o->last;
  }
  /* get off the entry to return */
  r = o->curr->ent;
  /* null it */
  o->curr->ent = NULL;

  if(o->currm1 == o->curr){
    /* only one item in list */
    free(o->last);
    o->n2last = o->curr = o->currm1 = o->last = NULL;
  }else{
    if(o->last == o->curr){
      /* reset to next to last one. */
      o->last = o->currm1;
      if((o->cntIn-1) > 1){
	llist_slink *temp;
	/* Special case I must fix n2last */
	for(temp = o->last->next;temp != o->last;temp = temp->next){
	  /* Hunt until we get temp->next is last
	   * then we are at last -1.
	   */
	  if(temp->next == o->last){
	    o->n2last = temp;
	    break;
	  }
	}
      }
    }
    o->currm1->next = o->curr->next;
    o->curr->next = NULL;
    free(o->curr);
    o->curr = o->currm1->next;
  }
  o->cntIn--;
  return(r);
}

int
llist_backUpOne(llist_t *o)
{
  llist_slink *temp;
  if(o->cntIn < 2){
    /* can't back up if there are not at least 2. */
    return(LIB_STATUS_BAD);
  }
  for(temp = o->curr;temp->next!=o->curr;temp=temp->next){
    /* move through list. */
    if(temp->next == o->currm1){
      o->curr = o->currm1;
      o->currm1 = temp;
      return(LIB_STATUS_GOOD);
    }
  }
  return(LIB_STATUS_BAD);
}

void*
llist_replaceThis(llist_t *o,void *e)
{
  void *r;
  /* Any in the list? */
  if(o->last == NULL)
    /* no */
    return (NULL);

  if((o->curr == NULL) || (o->currm1 == NULL)){
    o->curr = o->last->next;
    o->currm1 = o->last;
  }
  r = o->curr->ent;
  o->curr->ent = e;
  return(r);
}

llist_slink*
llist_getThisSlist(llist_t *o)
{
  llist_slink *r;

  /* any in list? */
  if(o->last == NULL)
    return(NULL);

  /* setup the pointers as necessary */
  if((o->curr == NULL) || (o->currm1 == NULL)){
    o->curr = o->last->next;
    o->currm1 = o->last;
  }

  r = o->curr;
  if(o->currm1 == o->curr){
    /* only one item in list */
    o->curr = o->currm1 = o->last = NULL;
  }else{
    if(o->last == o->curr){
      /* reset to next to last one. */
      o->last = o->currm1;
      if((o->cntIn-1) > 1){
	llist_slink *temp;
	/* Special case I must fix n2last */
	for(temp = o->last->next;temp != o->last;temp = temp->next){
	  /* Hunt until we get temp->next is last
	   * then we are at last -1.
	   */
	  if(temp->next == o->last){
	    o->n2last = temp;
	    break;
	  }
	}
      }
    }
    o->currm1->next = o->curr->next;
    o->curr->next = NULL;
    o->curr = o->currm1->next;
  }
  o->cntIn--;
  return(r);
}

void
llist_reset(llist_t *o)
{ 
  o->curr = o->currm1 = NULL;
  o->wrapFlag=0;
  return;
}

int
llist_insertHere(llist_t *o, void* a)
{
  llist_slink *tmp;
  if(o->currm1){
    tmp = calloc(1,sizeof(llist_slink));
    if(tmp == NULL){
      return(LIB_STATUS_BAD);
    }
    tmp->ent = a;
    tmp->next = o->currm1->next;
    if(o->currm1 == o->n2last){
      /* Put in right before last. */
      o->n2last = tmp;
    }
    o->currm1->next = tmp;
  }else{
    /* List is reset so we drop to
     * a typical insert.
     */
    return(llist_insert(o,a));
  }
  o->cntIn++;
  return(LIB_STATUS_GOOD);
}

int
llist_appendHere(llist_t *o,void* a)
{
  llist_slink *tmp;
  if(o->curr){
    if(o->curr == o->currm1){
      /* special case for only one item on list 
       * just do a append.
       */
      return(llist_append(o,a));
    }else if(o->curr == o->last){
      /* special case where the pointers are set
       * to the last one so it degenerates to a
       * simple append 
       */
      return(llist_append(o,a));
    }else{
      /* ok the curr is out in the list
       * somewhere. Append at where we
       * are.
       */
      tmp = calloc(1,sizeof(struct llist_slink));
      if(tmp == NULL){
	return(LIB_STATUS_BAD);
      }
      tmp->ent = a;
      tmp->next = o->curr->next;
      if(o->curr == o->n2last)
	// putting it after n2last.
	o->n2last = tmp;
      o->curr->next = tmp;
    }
  }else{
    /* degenerate to an append if list as not
     * been touched
     */
    return(llist_append(o,a));

  }
  o->cntIn++;
  return(LIB_STATUS_GOOD);
}

void
llist_printCnt(llist_t *o)
{
  printf("On queue %d\n",o->cntIn);
}

void
llist_printList(llist_t *o)
{
  llist_slink *ll,*pp;
  int i;
  if(o->last == NULL){
    printf("List is empty\n");
    return;
  }else{
    printf("There are %d items on the list\n",o->cntIn);
  }
  ll = o->last->next;
  i = 0;
  do{
    printf("%d:Me 0x%x Entry 0x%x Next->0x%x\n",
	   i,(unsigned int)ll,(unsigned int)ll->ent,(unsigned int)ll->next);
    pp = ll->next;
    ll = pp;
    i++;
  }while(ll != o->last);
  printf("%d:Me 0x%x Entry 0x%x Next->0x%x\n",
	 i,(unsigned int)o->last,(unsigned int)o->last->ent,(unsigned int)o->last->next);
  printf("List ends\n");
}

int
llist_getToTheEnd(llist_t *o)
{
  if(o->last == NULL)
    return(0);
  o->wrapFlag = 0;
  o->curr = o->last;
  if((o->n2last == o->curr) && (o->cntIn > 1)){
    llist_slink *temp;
    /* Special case I must fix n2last */
    for(temp = o->last->next;temp != o->last;temp = temp->next){
      /* Hunt until we get temp->next is last
       * then we are at last -1.
       */
      if(temp->next == o->last){
	o->n2last = temp;
	break;
      }
    }
  }
  o->currm1 = o->n2last;
  return(1);
}

void *
llist_lookLastOne(llist_t *o)
{
  if(o->last == NULL)
    return(NULL);
  return(o->last->ent);
}

void *
llist_lookN2LastOne(llist_t *o)
{
  if(o->n2last == NULL)
    return(NULL);
  return(o->n2last->ent);
}

int llist_getCnt(llist_t *o){
    return o->cntIn;
}
