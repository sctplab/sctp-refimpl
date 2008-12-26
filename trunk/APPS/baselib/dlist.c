/*	$Header: /usr/sctpCVS/APPS/baselib/dlist.c,v 1.3 2008-12-26 14:45:12 randall Exp $ */

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

#include <dlist.h>

dlist_t *
dlist_create()
{
	dlist_t *t;

	t = (dlist_t *) CALLOC(1, sizeof(dlist_t));
	if (t == NULL)
		return (NULL);
	t->wrapFlag = 0;
	t->head = t->tail = t->curr = NULL;
	t->cntIn = 0;
	return (t);
}

void
dlist_destroy(dlist_t * o)
{
	if (o == NULL)
		return;
	dlist_clear(o);
	FREE(o);
}

void
dlist_clear(dlist_t * o)
{
	dlist_dlink *tmp;

	/* none in list */
	if ((o == NULL) || (o->head == NULL))
		return;

	/* go through and FREE every one */
	while ((tmp = o->head) != NULL) {
		o->head = tmp->next;
		FREE(tmp);
	}
	/* clean up time */
	o->wrapFlag = 0;
	o->cntIn = 0;
	o->head = o->tail = NULL;
	o->curr = NULL;
}

void *
dlist_getNext(dlist_t * o)
{
	void *e;
	dlist_dlink *tmp;

	/* empty list */
	if (o == NULL)
		return (NULL);

	if (o->head == NULL) {
		o->wrapFlag = 0;
		return (NULL);
	}
	/* start at the head */
	tmp = o->head;
	/* set head to advance */
	o->head = tmp->next;
	if (o->head) {
		/*
		 * its not null so null out prev.
		 */
		o->head->prev = NULL;
		/* Fix the curr pointer if needed */
		if (o->curr == tmp)
			o->curr = o->head;
	} else {
		/* none left */
		o->tail = NULL;
		o->curr = NULL;
	}
	/* lower count */
	o->cntIn--;

	/*
	 * do sanity check, are we not empty and have a cnt that is
	 * mis-matching?
	 */
	if ((o->cntIn <= 0) && (o->head != NULL)) {
		printf("Bad o->head pointer\n");
		abort();
	}
	/*
	 * are we set to one and the tail not equal to the head?
	 */
	if ((o->cntIn == 1) && (o->head != o->tail)) {
		printf("Only 1 and o->head not o->tail!\n");
		abort();
	}
	/* ok we are set */
	e = tmp->ent;
	FREE(tmp);
	return (e);
}

dlist_dlink *
dlist_getNextSlist(dlist_t * o)
{
	dlist_dlink *tmp;

	if (o == NULL) {
		return (NULL);
	}
	/* are we already empty ? */
	if (o->head == NULL) {
		o->wrapFlag = 0;
		return (NULL);
	}
	/* ok setup to start */
	tmp = o->head;
	o->head = tmp->next;
	if (o->head) {
		/* some left, null out prev */
		o->head->prev = NULL;
		/* do we need to fix curr pointer? */
		if (o->curr == tmp)
			o->curr = o->head;
	} else {
		/* none left */
		o->tail = NULL;
		o->curr = NULL;
		o->wrapFlag = 0;
	}
	o->cntIn--;

	/* sanity check time */
	if ((o->cntIn <= 0) && (o->head != NULL)) {
		printf("Bad o->head pointer\n");
		abort();
	}
	if ((o->cntIn == 1) && (o->head != o->tail)) {
		printf("Only 1 and o->head not o->tail!\n");
		abort();
	}
	/* ok null its pointers */
	tmp->prev = NULL;
	tmp->next = NULL;
	return (tmp);
}

void *
dlist_get(dlist_t * o)
{
	void *ent;

	/* a sanity with a turse note */
	if (o == NULL)
		return NULL;

	if ((o->cntIn == 0) && (o->head != NULL)) {
		return (NULL);
	}
	/* none left */
	if (o->head == NULL)
		return (NULL);

	/* ok we have wrapped to the end */
	if ((o->curr == NULL) && o->wrapFlag)
		return (NULL);

	/* set to begin now */
	if (o->curr == NULL)
		o->curr = o->head;

	/* pull the entry */
	ent = o->curr->ent;
	/* advance the pointer */
	o->curr = o->curr->next;
	if (o->curr == NULL)
		/* we have wrapped now */
		o->wrapFlag = 1;
	/* ok back goes the entry */
	return (ent);
}

void *
dlist_getThis(dlist_t * o)
{
	void *ent;
	dlist_dlink *getOut;

	if (o == NULL)
		return NULL;

	/* none in the list ? */
	if (o->head == NULL) {
		return (NULL);
	}
	if ((o->curr == NULL) && (o->wrapFlag)) {
		/* at the end get the tail */
		getOut = o->tail;
	} else if ((o->curr == NULL) && (!o->wrapFlag)) {
		/* trivial case, no hunt in list done */
		return (dlist_getNext(o));
	} else if ((o->curr == NULL) && (o->wrapFlag)) {
		/* this should not happen */
		return (NULL);
	} else {
		/* ok, we are somewhere pointing in the list */
		if (o->curr->prev == o->head) {
			/*
			 * we just looked at the beginning so same as
			 * initial get
			 */
			return (dlist_getNext(o));
		} else if (o->curr->prev != NULL) {
			/*
			 * Ok prev is our man
			 */
			getOut = o->curr->prev;
		} else {
			/* prev is null at head of list */
			return (dlist_getNext(o));
		}
	}
	/* save off entry of victim */
	ent = getOut->ent;

	/* are we removign last one, if so reset tail */
	if (getOut == o->tail) {
		o->tail = getOut->prev;
	}
	/* is there a prev to this, if so correct and fix */
	if (getOut->prev) {
		getOut->prev->next = getOut->next;
	}
	/* is there a next to be fixed, if so fix it */
	if (getOut->next) {
		getOut->next->prev = getOut->prev;
	}
	/* is this the head we are adjusting? if so reset the head */
	if (getOut == o->head) {
		o->head = getOut->next;
	}
	/* lower the count */
	o->cntIn--;
	/* sanity check time */
	if ((o->cntIn <= 0) && (o->head != NULL)) {
		printf("Bad o->head pointer\n");
		abort();
	}
	if ((o->cntIn == 1) && (o->head != o->tail)) {
		printf("Only 1 and o->head not o->tail!\n");
		abort();
	}
	getOut->next = NULL;
	getOut->prev = NULL;
	FREE(getOut);
	return (ent);
}

void *
dlist_replaceThis(dlist_t * o, void *entry)
{
	void *en;

	if (o == NULL)
		return NULL;

	/* none to replace */
	if (o->head == NULL)
		return (NULL);

	/* I wrapped? */
	if (o->wrapFlag) {
		/* yep so replace one at the tail */
		en = o->tail->ent;
		o->tail->ent = entry;
	} else if (o->curr == NULL) {
		/* never searched, so replace one at head */
		en = o->head->ent;
		o->head->ent = entry;
	} else {
		/*
		 * ok, we must replace relative to the current
		 */
		if (o->curr->prev == NULL) {
			/* if prev is null we are at the beginning */
			en = o->head->ent;
			o->head->ent = entry;
		} else {
			/* ok replace the one I last gave out in a get */
			en = o->curr->prev->ent;
			o->curr->prev->ent = entry;
		}
	}
	/* return the entry being replaced */
	return (en);
}


dlist_dlink *
dlist_getThisSlist(dlist_t * o)
{
	dlist_dlink *getOut;

	if (o == NULL)
		return NULL;

	/* none to give out */
	if (o->head == NULL) {
		return (NULL);
	}
	/* are we at the beginning */
	if ((o->curr == NULL) && (o->wrapFlag == 0)) {
		/* yep at the beginning */
		return (dlist_getNextSlist(o));
	} else if (o->curr == NULL) {
		/* no, maybe the end */
		getOut = o->tail;
	} else {
		/* we are in the list */
		if (o->curr->prev == o->head) {
			/*
			 * other case where we just gave out the beginning
			 * of the list
			 */
			return (dlist_getNextSlist(o));
		} else if (o->curr->prev != NULL) {
			/* fix to give out the previous one */
			getOut = o->curr->prev;
		} else {
			/* we must have just given out the head */
			return (dlist_getNextSlist(o));
		}
	}
	/* ok is it the tail, if so fix the tail pointer */
	if (getOut == o->tail) {
		o->tail = getOut->prev;
	}
	/* if it has a prev, fix it */
	if (getOut->prev) {
		getOut->prev->next = getOut->next;
	}
	/* if it has a next, fix it */
	if (getOut->next) {
		getOut->next->prev = getOut->prev;
	}
	/* if it is the head, fix up a new head pointer */
	if (getOut == o->head) {
		o->head = getOut->next;
	}
	getOut->prev = NULL;
	getOut->next = NULL;
	o->cntIn--;
	/* sanity time */
	if ((o->cntIn <= 0) && (o->head != NULL)) {
		printf("Bad o->head pointer\n");
		abort();
	}
	if ((o->cntIn == 1) && (o->head != o->tail)) {
		printf("Only 1 and o->head not o->tail!\n");
		abort();
	}
	/* you get the link back */
	return (getOut);
}

void
dlist_reset(dlist_t * o)
{
	if (o == NULL)
		return;
	o->curr = o->head;
	o->wrapFlag = 0;
}

int
dlist_insert(dlist_t * o, void *entry)
{
	dlist_dlink *temp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	/* roll a new one */
	temp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
	if (temp == NULL) {
		/* we are hosed */
		return (LIB_STATUS_BAD);
	}
	/* do we have any in the list? */
	if (o->head != NULL) {
		/* roll a new one into the head */
		temp->ent = entry;
		temp->prev = o->head->prev;
		temp->next = o->head;
		o->head->prev = temp;
		o->head = temp;
	} else {
		/* only one fix head and tail */
		temp->ent = entry;
		temp->prev = NULL;
		temp->next = NULL;
		o->tail = o->head = temp;
	}
	o->cntIn++;
	return (LIB_STATUS_GOOD);
}

int
dlist_append(dlist_t * o, void *entry)
{
	dlist_dlink *temp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	temp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
	if (temp == NULL) {
		/* we are hosed */
		return (LIB_STATUS_BAD);
	}
	if (o->tail != NULL) {
		temp->ent = entry;
		temp->prev = o->tail;
		temp->next = o->tail->next;
		o->tail->next = temp;
		o->tail = temp;
		o->cntIn++;
		return (LIB_STATUS_GOOD);
	}
	if (o->head != NULL) {
		/*
		 * this really should not happen, it means our tail was
		 * messed up somewhere.
		 */
		o->tail = o->head;
		temp->ent = entry;
		temp->prev = o->tail;
		temp->next = o->tail->next;
		o->tail->next = temp;
		o->tail = temp;
		o->cntIn++;
		return (LIB_STATUS_GOOD);
	}
	/* first one in the list */
	temp->ent = entry;
	temp->prev = NULL;
	temp->next = NULL;
	o->tail = o->head = temp;
	o->cntIn++;
	return (LIB_STATUS_GOOD);
}

int
dlist_appenddlink(dlist_t * o, dlist_dlink * entry)
{
	if (o == NULL)
		return (LIB_STATUS_BAD);

	/* make sure my front and back pointers are NULL */
	entry->next = NULL;
	entry->prev = NULL;

	/* now where do we place it */
	if (o->tail == NULL) {
		/* no tail */
		if (o->head == NULL) {
			/* no head either so it becomes our list */
			o->head = o->tail = entry;
		} else {
			/* hmm, this means are tail was corrupt? TSNH */

			/* walk to end of tail to fix */
			for (o->tail = o->head; o->tail->next != NULL; o->tail = o->tail->next);
			/* now use tail has previous */
			entry->prev = o->tail;
			/* if we have a prev, set it's next to new guy */
			if (o->tail->prev)
				o->tail->next = entry;
			else
				/* otherwise the tail is the head, put it in */
				o->head->next = entry;
			/* now set the tail to this new guy */
			o->tail = entry;
		}
	} else {
		/* ok, we have a head and tail set to something */
		entry->prev = o->tail;
		o->tail->next = entry;
		o->tail = entry;
	}
	o->cntIn++;
	return (LIB_STATUS_GOOD);
}

int
dlist_insertHere(dlist_t * o, void *entry)
{
	dlist_dlink *tmp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	/* any in list? */
	if (o->head == NULL) {
		/* no its empty */
		tmp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
		if (tmp == NULL) {
			/* we are hosed */
			return (LIB_STATUS_BAD);
		}
		o->head = o->tail = tmp;
		tmp->next = tmp->prev = NULL;
		tmp->ent = entry;
		o->cntIn++;
		return (LIB_STATUS_GOOD);
	}
	/* ok, here we must add, verify that the head/tail are right */
	if ((o->tail == NULL) && (o->head != NULL)) {
		/* TSNH, corrupt tail?  recover it ... */
		for (o->tail = o->head; o->tail->next != NULL; o->tail = o->tail->next);
	}
	if (o->wrapFlag == 1) {
		/*
		 * we had given out the last one when the guy asked us to
		 * enter it
		 */
		if (o->tail->prev) {
			tmp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
			if (tmp == NULL) {
				/* we are hosed */
				return (LIB_STATUS_BAD);
			}
			tmp->ent = entry;
			tmp->prev = o->tail->prev;
			tmp->next = o->tail;
			o->tail->prev->next = tmp;
			o->tail->prev = tmp;
			o->cntIn++;
			return (LIB_STATUS_GOOD);
		} else {
			/*
			 * Hmm wrapflag is set and no tail? treat has
			 * insert.
			 */
			return (dlist_insert(o, entry));
		}
	} else if (o->curr == NULL) {
		/* ok we have not walked anything yet */
		return (dlist_insert(o, entry));
	} else {
		/* we are in the middle somewhere */
		if (o->curr == o->head) {
			/* at the head */
			return (dlist_insert(o, entry));
		}
		if (o->curr->prev == o->head) {
			/* just given out the head */
			return (dlist_insert(o, entry));
		}
		if (o->curr->prev) {
			tmp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
			if (tmp == NULL) {
				/* we are hosed */
				return (LIB_STATUS_BAD);
			}
			tmp->ent = entry;
			tmp->prev = o->curr->prev->prev;
			tmp->next = o->curr->prev;
			if (tmp->prev) {
				tmp->prev->next = tmp;
			}
			if (tmp->next) {
				tmp->next->prev = tmp;
			}
			o->cntIn++;
		} else {
			return (dlist_insert(o, entry));
		}
	}
	return (LIB_STATUS_GOOD);
}

int
dlist_appendHere(dlist_t * o, void *entry)
{
	dlist_dlink *tmp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (o->head == NULL) {
		/* nothing in yet */
		return (dlist_append(o, entry));
	}
	if (o->wrapFlag) {
		/* we wrapped */
		return (dlist_append(o, entry));
	}
	if (o->curr == NULL) {
		/* we append to head */
		tmp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
		if (tmp == NULL) {
			/* we are hosed */
			return (LIB_STATUS_BAD);
		}
		tmp->ent = entry;
		tmp->prev = o->head;
		tmp->next = o->head->next;
		o->cntIn++;
		o->head->next = tmp;
		if (tmp->next) {
			tmp->next->prev = tmp;
		} else {
			o->tail = tmp;
		}
		return (LIB_STATUS_GOOD);
	}
	/* ok we are in the list somewhere */
	tmp = (dlist_dlink *) CALLOC(1, sizeof(dlist_dlink));
	if (tmp == NULL) {
		/* we are hosed */
		return (LIB_STATUS_BAD);
	}
	tmp->ent = entry;
	tmp->prev = o->curr->prev;
	tmp->next = o->curr;
	o->cntIn++;
	if (tmp->prev) {
		tmp->prev->next = tmp;
	}
	if (tmp->next) {
		tmp->next->prev = tmp;
	} else {
		o->tail = tmp;
	}
	return (LIB_STATUS_GOOD);
}

int
dlist_getToTheEnd(dlist_t * o)
{
	if (o == NULL)
		return (LIB_STATUS_BAD);
	o->wrapFlag = 0;
	o->curr = o->tail;
	return (LIB_STATUS_GOOD);
}

int
dlist_backUpOne(dlist_t * o)
{
	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (o->head == NULL)
		return (LIB_STATUS_BAD);

	if (o->curr != NULL) {
		o->curr = o->curr->prev;
	}
	if (o->curr == NULL) {
		o->curr = o->head;
	}
	o->wrapFlag = 0;
	return (LIB_STATUS_GOOD);
}

void *
dlist_lookLastOne(dlist_t * o)
{
	if (o == NULL)
		return (NULL);

	if (o->tail)
		return (o->tail->ent);
	else
		return (NULL);
}

void *
dlist_lookN2LastOne(dlist_t * o)
{
	if (o == NULL)
		return (NULL);

	if (o->tail) {
		if (o->tail->prev)
			return (o->tail->prev->ent);
		else
			return (NULL);
	} else {
		return (NULL);
	}
}

void
dlist_printCnt(dlist_t * o)
{
	if (o == NULL)
		return;

	printf("On queue %d\n", o->cntIn);
}

void
dlist_printList(dlist_t * o)
{
	dlist_dlink *t;
	int cnt;

	if (o == NULL)
		return;

	printf("O->Head:%p o->tail:%p o->curr:%p cnt:%d\n",
	    o->head, o->tail, o->curr, o->cntIn);
	cnt = 0;
	for (t = o->head; t != NULL; t = t->next) {
		printf("entry[%d]:%p prev:%p next:%p\n",
		    cnt, t, t->prev, t->next);
		cnt++;
	}
}

int 
dlist_getCnt(dlist_t * o)
{
	if (o == NULL)
		return -1;

	return o->cntIn;
}
