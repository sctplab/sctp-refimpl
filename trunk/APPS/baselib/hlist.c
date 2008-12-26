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

#include <hlist.h>
#include <printMessageHandler.h>

hlist_t *
hlist_create(char *name, int siz)
{
	hlist_t *t;

	t = (hlist_t *) CALLOC(1, sizeof(hlist_t));
	if (t == NULL)
		return (NULL);
	t->wrapFlag = 0;
	t->head = t->tail = t->curr = NULL;
	t->cntIn = 0;
	if (siz)
		t->entries = HashedTbl_create(name, siz);
	else
		t->entries = HashedTbl_create(name, HLIST_DEFAULT_SIZE);
	if (t->entries == NULL) {
		/* no memory */
		FREE(t);
		return (NULL);
	}
	return (t);
}

void
hlist_destroy(hlist_t * o)
{
	void *v;

	if (o == NULL)
		return;

	if (o->cntIn) {
		HashedTbl_rewind(o->entries);

		while ((v = HashedTbl_getNext(o->entries, NULL)) != NULL) {
			 /* Clear all entries */ ;
		}
	}
	HashedTbl_destroy(o->entries);
	o->entries = NULL;
	hlist_clear(o);
	FREE(o);
}

void
hlist_clear(hlist_t * o)
{
	hlist_hlink *tmp;

	if (o == NULL)
		return;

	/* none in list */
	if (o->head == NULL)
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
hlist_getNext(hlist_t * o)
{
	void *e;
	hlist_hlink *tmp;

	if (o == NULL)
		return (NULL);

	/* empty list */
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

	/* remove it from the hash */
	HashedTbl_remove(o->entries, &e, sizeof(void *), NULL);

	FREE(tmp);
	return (e);
}

hlist_hlink *
hlist_getNextHlist(hlist_t * o)
{
	hlist_hlink *tmp;

	if (o == NULL)
		return (NULL);

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
	/* remove it from the hash */
	HashedTbl_remove(o->entries, &tmp->ent, sizeof(void *), NULL);

	/* ok null its pointers */
	tmp->prev = NULL;
	tmp->next = NULL;
	return (tmp);
}

void *
hlist_get(hlist_t * o)
{
	void *ent;

	/* a sanity with a turse note */
	if (o == NULL)
		return (NULL);

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
hlist_getPrev(hlist_t * o)
{
	void *ent;

	if (o == NULL)
		return (NULL);
	/* a sanity with a turse note */
	if ((o->cntIn == 0) && (o->head != NULL)) {
		return (NULL);
	}
	/* none left */
	if (o->head == NULL)
		return (NULL);

	/* ok we have wrapped to the end */
	if ((o->curr == NULL) && o->wrapFlag) {
		if (o->tail) {
			return (o->tail->ent);
		} else {
			return (NULL);
		}
	}
	/* set to begin now */
	if (o->curr == NULL) {
		o->curr = o->head;
		return (NULL);
	}
	/* copy out the prev entry */
	if (o->curr->prev) {
		ent = o->curr->prev->ent;
	} else {
		ent = NULL;
	}
	return (ent);
}


void *
hlist_getThis(hlist_t * o)
{
	void *ent;
	hlist_hlink *getOut;

	if (o == NULL)
		return (NULL);

	/* none in the list ? */
	if (o->head == NULL) {
		return (NULL);
	}
	if ((o->curr == NULL) && (o->wrapFlag)) {
		/* at the end get the tail */
		getOut = o->tail;
	} else if ((o->curr == NULL) && (!o->wrapFlag)) {
		/* trivial case, no hunt in list done */
		return (hlist_getNext(o));
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
			return (hlist_getNext(o));
		} else if (o->curr->prev != NULL) {
			/*
			 * Ok prev is our man
			 */
			getOut = o->curr->prev;
		} else {
			/* prev is null at head of list */
			return (hlist_getNext(o));
		}
	}
	/* save off entry of victim */
	ent = getOut->ent;

	/* remove it from the hash */
	HashedTbl_remove(o->entries, &ent, sizeof(void *), NULL);

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
hlist_pullThis(hlist_t * o, void *item)
{
	hlist_hlink *getOut;
	void *ent;

	if (o == NULL)
		return (NULL);

	getOut = (hlist_hlink *) HashedTbl_lookup(o->entries, &item, (sizeof(void *)), NULL);
	if (getOut == NULL) {
		/* Not found? */
		return (NULL);
	}
	/* Fix the curr pointer if needed */
	if (o->curr == getOut) {
		o->curr = getOut->next;
		if (o->curr == NULL)
			o->wrapFlag = 1;
	}
	/* Fix the head if needed */
	if (o->head == getOut) {
		o->head = getOut->next;
	}
	/* Fix the tail if needed */
	if (o->tail == getOut) {
		o->tail = getOut->prev;
	}
	/* Now pull the item from the list */
	if (getOut->prev) {
		getOut->prev->next = getOut->next;
	}
	if (getOut->next) {
		getOut->next->prev = getOut->prev;
	}
	HashedTbl_remove(o->entries, &item, sizeof(void *), NULL);
	o->cntIn--;
	ent = getOut->ent;
	getOut->prev = NULL;
	getOut->next = NULL;
	FREE(getOut);
	return (ent);
}

int 
hlist_isItInTheList(hlist_t * o, void *item)
{
	hlist_hlink *it;

	if (o == NULL)
		return (0);

	it = (hlist_hlink *) HashedTbl_lookup(o->entries, &item, (sizeof(void *)), NULL);
	if (it == NULL) {
		/* Nope */
		return (0);
	}
	return (1);
}

int 
hlist_moveToThis(hlist_t * o, void *item)
{
	hlist_hlink *it;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	it = (hlist_hlink *) HashedTbl_lookup(o->entries, &item, (sizeof(void *)), NULL);
	if (it == NULL) {
		/* Not found? */
		return (LIB_STATUS_BAD);
	}
	o->curr = it;
	o->wrapFlag = 0;
	return (LIB_STATUS_GOOD);
}

hlist_hlink *
hlist_pullThis_hlink(hlist_t * o, void *item)
{
	hlist_hlink *getOut;

	if (o == NULL)
		return (NULL);

	getOut = (hlist_hlink *) HashedTbl_lookup(o->entries, &item, (sizeof(void *)), NULL);
	if (getOut == NULL) {
		/* Not found? */
		return (NULL);
	}
	/* Fix the curr pointer if needed */
	if (o->curr == getOut) {
		o->curr = getOut->next;
		if (o->curr == NULL)
			o->wrapFlag = 1;
	}
	/* Fix the head if needed */
	if (o->head == getOut) {
		o->head = getOut->next;
	}
	/* Fix the tail if needed */
	if (o->tail == getOut) {
		o->tail = getOut->prev;
	}
	/* Now pull the item from the list */
	if (getOut->prev) {
		getOut->prev->next = getOut->next;
	}
	if (getOut->next) {
		getOut->next->prev = getOut->prev;
	}
	HashedTbl_remove(o->entries, &item, sizeof(void *), NULL);
	o->cntIn--;
	getOut->prev = NULL;
	getOut->next = NULL;
	/* It's yours */
	return (getOut);
}

void *
hlist_replaceThis(hlist_t * o, void *entry)
{
	void *en;

	if (o == NULL)
		return (NULL);

	/* none to replace */
	if (o->head == NULL)
		return (NULL);

	/* I wrapped? */
	if (o->wrapFlag) {
		/* yep so replace one at the tail */
		en = o->tail->ent;
		/* remove it from the hash */
		HashedTbl_remove(o->entries, &o->tail->ent, sizeof(void *), NULL);
		o->tail->ent = entry;
		HashedTbl_enter(o->entries, &o->tail->ent, o->tail, (sizeof(void *)));
	} else if (o->curr == NULL) {
		/* never searched, so replace one at head */
		en = o->head->ent;
		HashedTbl_remove(o->entries, &o->head->ent, sizeof(void *), NULL);
		o->head->ent = entry;
		HashedTbl_enter(o->entries, &o->head->ent, o->head, (sizeof(void *)));
	} else {
		/*
		 * ok, we must replace relative to the current
		 */
		if (o->curr->prev == NULL) {
			/* if prev is null we are at the beginning */
			en = o->head->ent;
			HashedTbl_remove(o->entries, &o->head->ent, sizeof(void *), NULL);
			o->head->ent = entry;
			HashedTbl_enter(o->entries, &o->head->ent, o->head, (sizeof(void *)));
		} else {
			/* ok replace the one I last gave out in a get */
			en = o->curr->prev->ent;
			HashedTbl_remove(o->entries, &en, sizeof(void *), NULL);
			o->curr->prev->ent = entry;
			HashedTbl_enter(o->entries, &o->curr->prev->ent, o->curr->prev, (sizeof(void *)));
		}
	}
	/* return the entry being replaced */
	return (en);
}


hlist_hlink *
hlist_getThisHlist(hlist_t * o)
{
	hlist_hlink *getOut;

	if (o == NULL)
		return (NULL);

	/* none to give out */
	if (o->head == NULL) {
		return (NULL);
	}
	/* are we at the beginning */
	if ((o->curr == NULL) && (o->wrapFlag == 0)) {
		/* yep at the beginning */
		return (hlist_getNextHlist(o));
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
			return (hlist_getNextHlist(o));
		} else if (o->curr->prev != NULL) {
			/* fix to give out the previous one */
			getOut = o->curr->prev;
		} else {
			/* we must have just given out the head */
			return (hlist_getNextHlist(o));
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
	/* Remove it from the hash */
	HashedTbl_remove(o->entries, &getOut->ent, sizeof(void *), NULL);

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
hlist_reset(hlist_t * o)
{
	if (o == NULL)
		return;
	o->curr = o->head;
	o->wrapFlag = 0;
}

int
hlist_insert(hlist_t * o, void *entry)
{
	hlist_hlink *temp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	/* roll a new one */
	temp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
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
	/* add it to the hash */
	HashedTbl_enter(o->entries, &temp->ent, temp, (sizeof(void *)));
	return (LIB_STATUS_GOOD);
}

int
hlist_append(hlist_t * o, void *entry)
{
	hlist_hlink *temp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (hlist_isItInTheList(o, entry)) {
	  /* already in the list silly */
	  return (LIB_STATUS_BAD);
	}
	temp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
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
		goto out;
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
		goto out;
	}
	/* first one in the list */
	temp->ent = entry;
	temp->prev = NULL;
	temp->next = NULL;
	o->tail = o->head = temp;
	o->cntIn++;
out:
	/* add it to the hash */
	HashedTbl_enter(o->entries, &temp->ent, temp, (sizeof(void *)));
	return (LIB_STATUS_GOOD);
}

int
hlist_appendhlink(hlist_t * o, hlist_hlink * entry)
{
	/* make sure my front and back pointers are NULL */
	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (hlist_isItInTheList(o, entry->ent)) {
	  /* already in the list silly */
	  return (LIB_STATUS_BAD);
	}

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
	/* Add it to the hash list */
	HashedTbl_enter(o->entries, &entry->ent, entry, (sizeof(void *)));
	o->cntIn++;
	return (LIB_STATUS_GOOD);
}

int
hlist_insertHere(hlist_t * o, void *entry)
{
	hlist_hlink *tmp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (hlist_isItInTheList(o, entry)) {
	  /* already in the list silly */
	  return (LIB_STATUS_BAD);
	}
	/* any in list? */
	if (o->head == NULL) {
		/* no its empty */
		tmp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
		if (tmp == NULL) {
			/* we are hosed */
			return (LIB_STATUS_BAD);
		}
		o->head = o->tail = tmp;
		tmp->next = tmp->prev = NULL;
		tmp->ent = entry;
		HashedTbl_enter(o->entries, &tmp->ent, tmp, (sizeof(void *)));
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
			tmp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
			if (tmp == NULL) {
				/* we are hosed */
				return (LIB_STATUS_BAD);
			}
			tmp->ent = entry;
			tmp->prev = o->tail->prev;
			tmp->next = o->tail;
			o->tail->prev->next = tmp;
			o->tail->prev = tmp;
			HashedTbl_enter(o->entries, &tmp->ent, tmp, (sizeof(void *)));
			o->cntIn++;
			return (LIB_STATUS_GOOD);
		} else {
			/*
			 * Hmm wrapflag is set and no tail? treat has
			 * insert.
			 */
			return (hlist_insert(o, entry));
		}
	} else if (o->curr == NULL) {
		/* ok we have not walked anything yet */
		return (hlist_insert(o, entry));
	} else {
		/* we are in the middle somewhere */
		if (o->curr == o->head) {
			/* at the head */
			return (hlist_insert(o, entry));
		}
		if (o->curr->prev == o->head) {
			/* just given out the head */
			return (hlist_insert(o, entry));
		}
		if (o->curr->prev) {
			tmp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
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
			HashedTbl_enter(o->entries, &tmp->ent, tmp, (sizeof(void *)));
			o->cntIn++;
		} else {
			return (hlist_insert(o, entry));
		}
	}
	return (LIB_STATUS_GOOD);
}

int
hlist_appendHere(hlist_t * o, void *entry)
{
	hlist_hlink *tmp;

	if (o == NULL)
		return (LIB_STATUS_BAD);

	if (hlist_isItInTheList(o, entry)) {
	  /* already in the list silly */
	  return (LIB_STATUS_BAD);
	}

	if (o->head == NULL) {
		/* nothing in yet */
		return (hlist_append(o, entry));
	}
	if (o->wrapFlag) {
		/* we wrapped */
		return (hlist_append(o, entry));
	}
	if (o->curr == NULL) {
		/* we append to head */
		tmp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
		if (tmp == NULL) {
			/* we are hosed */
			return (LIB_STATUS_BAD);
		}
		tmp->ent = entry;
		tmp->prev = o->head;
		tmp->next = o->head->next;
		HashedTbl_enter(o->entries, &tmp->ent, tmp, (sizeof(void *)));
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
	tmp = (hlist_hlink *) CALLOC(1, sizeof(hlist_hlink));
	if (tmp == NULL) {
		/* we are hosed */
		return (LIB_STATUS_BAD);
	}
	tmp->ent = entry;
	tmp->prev = o->curr->prev;
	tmp->next = o->curr;
	HashedTbl_enter(o->entries, &tmp->ent, tmp, (sizeof(void *)));
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
hlist_getToTheEnd(hlist_t * o)
{
	if (o == NULL)
		return (LIB_STATUS_BAD);

	o->wrapFlag = 0;
	o->curr = o->tail;
	return (LIB_STATUS_GOOD);
}

int
hlist_backUpOne(hlist_t * o)
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
hlist_lookLastOne(hlist_t * o)
{
	if (o->tail)
		return (o->tail->ent);
	else
		return (NULL);
}

void *
hlist_lookN2LastOne(hlist_t * o)
{
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
hlist_printCnt(hlist_t * o)
{
	if (o == NULL)
		return;

	printf("On queue %d\n", o->cntIn);
}

void
hlist_printList(hlist_t * o)
{
	hlist_hlink *t;
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
	printf("Found %d entries, list has:%d hash has:%d\n",
	    cnt, o->cntIn, HashedTbl_getSize(o->entries));
}

void
hlist_audit(hlist_t * o)
{
	int fnd = 0, cnt;
	hlist_hlink *t, *ii;

	if (o == NULL)
		return;

	if (o->cntIn != HashedTbl_getSize(o->entries)) {
		printf("List size %d does not match Hash Size %d\n",
		    o->cntIn, HashedTbl_getSize(o->entries));
		return;
	}
	for (t = o->head, cnt = 0; t != NULL; t = t->next) {
		cnt++;
	}
	if (cnt != o->cntIn) {
		printf("I counted %d and I have %d?\n",
		    o->cntIn, cnt);
		return;
	}
	/* validate the hash has everything we do */
	for (t = o->head; t != NULL; t = t->next) {
		if (!hlist_isItInTheList(o, t->ent)) {
			printf("Warning item:%p is in the list but not the hash\n", t->ent);
		}
	}
	HashedTbl_rewind(o->entries);
	while ((ii = (hlist_hlink *) HashedTbl_searchNext(o->entries, NULL, NULL)) != NULL) {
		fnd = 0;
		for (t = o->head; t != NULL; t = t->next) {
			if (t->ent == ii->ent) {
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
			printf("Item %p was in the hash table but NOT the list\n", ii->ent);
		}
	}
}

int 
hlist_getCnt(hlist_t * o)
{
	if (o == NULL)
		return (-1);

	return o->cntIn;
}
