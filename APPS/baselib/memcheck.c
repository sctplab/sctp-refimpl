#include <memcheck.h>
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

#define MEMORY_PADDING 96
#define PADDING_HALF 48

#define MARKA 0xf1018810
#define MARKB 0xf1014410

struct memLinks {
	uint32_t markA;
	uint32_t markB;
	uint32_t csum;
	int usedFree;
	size_t size;
	size_t req;
	struct memLinks *next;
	struct memLinks *nextFree;
	char *powner;
	char *owner;
	int bpowner;
	int bowner;
	u_char data[8];
};

static int countOf32 = 0;
static struct memLinks *free32 = 0;
static struct memLinks *head32 = 0;
static int countOf64 = 0;
static struct memLinks *free64 = 0;
static struct memLinks *head64 = 0;
static int countOf128 = 0;
static struct memLinks *free128 = 0;
static struct memLinks *head128 = 0;
static int countOf512 = 0;
static struct memLinks *free512 = 0;
static struct memLinks *head512 = 0;
static int countOf1024 = 0;
static struct memLinks *free1024 = 0;
static struct memLinks *head1024 = 0;

static u_long highest = 0;
static int memory_inited = 0;
static u_long lowest = 0xfffffff;

/*static pthread_mutex_t mu;*/

void
initMemoryLock()
{
	/* pthread_mutex_init(&mu,NULL); */
	memory_inited = 1;
}

void
verifyMemoryOf(struct memLinks *at)
{
	unsigned int i, s;

	s = 0;
	for (i = 0; i < (at->size - sizeof(struct memLinks)); i++) {
		s += (uint8_t)at->data[i];
	}
	if (s != at->csum) {
		DEBUG_PRINTF("memcheck:usedFree:%d,size:%d req:%d powner:%s:%d owner:%s:%d sum:%x c:%x\n",
		    at->usedFree,
		    (int)at->size,
		    (int)at->req,
		    at->powner,
		    (int)at->bpowner,
		    at->owner,
		    (int)at->bowner,
		    (u_int)at->csum,
		    s);
		DEBUG_PRINTF("memcheck:memory has been changed while free - here it is:\n");
		for (i = 0; (u_int)i < (at->size - sizeof(struct memLinks)); i++) {
		  DEBUG_PRINTF("%2.2x ", at->data[i]);
		  if (((i + 1) % 16) == 0) {
			DEBUG_PRINTF("\n");
		  }
		}
		if (i && ((i % 16) != 0)) {
		  DEBUG_PRINTF("\n");
		}
		DEBUG_PRINTF("memcheck:continuing but there is a problem!\n");
	}
}

void
setMemoryOfFreed(struct memLinks *at)
{
    int i;
	unsigned int s;

	s = 0;
	for (i = 0; i < (at->size - sizeof(struct memLinks)); i++) {
		s += (uint8_t)at->data[i];
	}
	at->csum = s;
}

void *
memcheck_malloc(size_t size, char *x, int y)
{
	char *newd;
	u_char *endPat;
	struct memLinks **from, **addTo, *at, *prev;
	int *cntUp;
	void *retVal;
	int i, j, ii, jj;
	size_t memSize;

	if (memory_inited == 0) {
		initMemoryLock();
	}
	if (size == 8) {
		ii = 0;
		jj = i;
	}
	/* pick a queue. */
	if (size <= 32) {
		from = &free32;
		addTo = &head32;
		cntUp = &countOf32;
		memSize = 32 + MEMORY_PADDING + sizeof(struct memLinks);
	} else if (size <= 64) {
		cntUp = &countOf64;
		addTo = &head64;
		from = &free64;
		memSize = 64 + MEMORY_PADDING + sizeof(struct memLinks);
	} else if (size <= 128) {
		cntUp = &countOf128;
		addTo = &head128;
		from = &free128;
		memSize = 128 + MEMORY_PADDING + sizeof(struct memLinks);
	} else if (size <= 512) {
		cntUp = &countOf512;
		addTo = &head512;
		from = &free512;
		memSize = 512 + MEMORY_PADDING + sizeof(struct memLinks);
	} else {
		cntUp = &countOf1024;
		from = &free1024;
		addTo = &head1024;
		if (size <= 1024) {
			memSize = 1024 + MEMORY_PADDING + sizeof(struct memLinks);
		} else {
			memSize = size + MEMORY_PADDING + sizeof(struct memLinks);
		}
	}
	retVal = NULL;
	at = NULL;
	if (from != NULL) {
		/* See if one exists. */
		prev = NULL;
		for (at = *from; at != NULL; at = at->nextFree) {
			if ((at->usedFree == 0) &&
			    (at->size - (sizeof(struct memLinks) + MEMORY_PADDING)) >= size) {
				retVal = (void *)&at->data[PADDING_HALF];
				if (prev == NULL) {
					*from = at->nextFree;
				} else {
					prev->nextFree = at->nextFree;
				}
				break;
			} else {
				prev = at;
			}
		}
	}
	if (retVal == NULL) {
		/* Must grow. */
		int xx;

		xx = *cntUp;
		xx++;
		*cntUp = xx;
		newd = (char *)malloc(memSize + 16);
		if (newd == NULL) {
			return ((void *)NULL);
		}
		memset(newd, 0, (memSize+16));
		if (((unsigned long)newd + memSize + 16) > highest) {
			highest = (unsigned long)newd + memSize + 16;
		}
		if ((unsigned long)newd < lowest) {
			lowest = (unsigned long)newd;
		}
		at = (struct memLinks *)((unsigned long)newd + 8);
		at->markA = MARKA;
		at->markB = MARKB;
		at->size = memSize;
		at->req = size;
		at->next = *addTo;
		at->nextFree = NULL;
		at->powner = 0;
		at->bpowner = 0;
		at->owner = x;
		at->bowner = y;
		*addTo = at;
		at->usedFree = 1;
		retVal = (void *)&at->data[PADDING_HALF];
	} else {
		verifyMemoryOf(at);
		/*
		 * Mark the reused here, that way the previous previous will
		 * point two levels if verifyMemoryOf() fails.
		 */
		at->req = size;
		at->usedFree = 1;
		at->powner = at->owner;
		at->bpowner = at->bowner;
		at->owner = x;
		at->bowner = y;
	}
	/* Now prep the data portion. */
	endPat = at->data;
	if (((unsigned long)endPat < lowest) || ((unsigned long)endPat > highest)) {
		DEBUG_PRINTF("memcheck:New error -- Internal error\n");
		abort();
	}
	j = 250;
	for (i = 0; i < PADDING_HALF; i++) {
		endPat[i] = (u_char)j;
		j--;
	}

	if (((unsigned long)endPat < lowest) || ((unsigned long)endPat > highest)) {
		DEBUG_PRINTF("memcheck:New error -- Internal error2\n");
		abort();
	}
	endPat = &at->data[(PADDING_HALF + size)];
	for (i = 0; i < PADDING_HALF; i++) {
		/* 1 - 20 in last 20 bytes. */
		endPat[i] = (u_char)(j);
		j--;
	}
	if (size == 8) {
		ii = 0;
		jj = ii;
	}
	/* pthread_mutex_unlock(&mu); */
	return (retVal);
}


void *
memcheck_calloc(size_t num, size_t size, char *x, int y)
{
	size_t z;
	void *v;

	z = num * size;
	v = memcheck_malloc(z, x, y);
	if (v) {
		memset(v, 0, z);
	}
	return (v);
}

struct memLinks *
validMemory(struct memLinks *at)
{
	struct memLinks *tt, *newat;

	if ((at->markA == MARKA) &&
	    (at->markB == MARKB)) {
		if (at->next) {
			if (((u_long)at->next < lowest) ||
			    ((u_long)at->next > highest)) {
				tt = NULL;
			} else {
				tt = at;
			}
		} else {
			tt = at;
		}
	} else {
		tt = NULL;
	}
	if (tt == NULL) {
		/* attempt to see if it has a 8 byte pad i.e. a array */
		newat = (struct memLinks *)((u_long)(at) - 8);
		if ((newat->markA == MARKA) &&
		    (newat->markB == MARKB)) {
			if (newat->next) {
				if (((u_long)newat->next < lowest) ||
				    ((u_long)newat->next > highest)) {
					tt = NULL;
				} else {
					tt = newat;
				}
			} else {
				tt = newat;
			}
		} else {
			tt = NULL;
		}
	}
	if (tt == NULL) {
		/* attempt to see if it has a 4 byte pad (for lynxOS) */
		newat = (struct memLinks *)((u_long)(at) - 4);
		if ((newat->markA == MARKA) &&
		    (newat->markB == MARKB)) {
			if (newat->next) {
				if (((u_long)newat->next < lowest) ||
				    ((u_long)newat->next > highest)) {
					tt = NULL;
				} else {
					tt = newat;
				}
			} else {
				tt = newat;
			}
		} else {
			tt = NULL;
		}
	}
	return (tt);
}


void
memcheck_free(void *delData, char *file, int line)
{
	struct memLinks *at, *tt;
	u_long calc;
	u_char *endPat;
	size_t size;
	int i, j;

	if (memory_inited == 0) {
		initMemoryLock();
	}
	if (delData == NULL) {
		DEBUG_PRINTF("memcheck:Attempt to free NULL pointer %s:%d\n",
		    file, line
		    );
		abort();
	}
	if (((unsigned long)delData < lowest) || ((unsigned long)delData > highest)) {
		DEBUG_PRINTF("memcheck:Bounds error - trying to free memory outside valid bounds %s:%d\n", file, line);
		abort();
	}
	calc = (unsigned long)delData;
	calc -= PADDING_HALF;
	calc -= (sizeof(struct memLinks) - sizeof(at->data));
	at = (struct memLinks *)calc;
	tt = validMemory(at);
	if (tt == NULL) {
		DEBUG_PRINTF("memcheck:Attempt to free memory not alocated from my queue %s:%d\n", file, line);
		abort();
	}
	at = tt;
	size = at->req;
	if (at->usedFree == 1) {
		at->usedFree = 0;
	} else {
		DEBUG_PRINTF("memcheck:Attempt to free memory already free %s:%d\n", file, line);
		abort();
	}
	endPat = at->data;
	j = 250;
	for (i = 0; i < PADDING_HALF; i++) {
		if (endPat[i] != (u_char)j) {
			DEBUG_PRINTF("memcheck:Overwrote memory at beg of block at %d 0x%x vs 0x%x %s:%d\n",
			    i, (u_char)endPat[i], (u_char)j, file, line);
			DEBUG_PRINTF("memcheck:Owner:%s:%d Previous Owner %s:%d size:%d req:%d\n",
			    at->owner, (int)at->bowner,
			    at->powner, (int)at->bpowner,
			    (int)at->size, (int)at->req);
			abort();
		}
		j--;
	}

	endPat = &at->data[(PADDING_HALF + size)];
	for (i = 0; i < PADDING_HALF; i++) {
		if (endPat[i] != (u_char)(j)) {
			DEBUG_PRINTF("memchek:Overwrote memory at end of allocated block at %d 0x%x vs 0x%x %s:%d\n",
			    i, (u_char)endPat[i], (u_char)j, file, line);
			DEBUG_PRINTF("memcheck:Owner:%s:%d Previous Owner %s:%d size:%d req:%d\n",
			    at->owner, (int)at->bowner,
			    at->powner, (int)at->bpowner,
			    (int)at->size, (int)at->req);
			abort();
		}
		j--;
	}
	setMemoryOfFreed(at);
	if (size <= 32) {
		tt->nextFree = free32;
		free32 = tt;
	} else if (size <= 64) {
		tt->nextFree = free64;
		free64 = tt;
	} else if (size <= 128) {
		tt->nextFree = free128;
		free128 = tt;
	} else if (size <= 512) {
		tt->nextFree = free512;
		free512 = tt;
	} else {
		tt->nextFree = free1024;
		free1024 = tt;
	}
}

void
printStats(struct memLinks *who, int printAll, char *queueName)
{
	int cnt, bcnt;
	struct memLinks *at;
	int i, j;
	u_char *endPat;

	at = who;
	bcnt = cnt = 0;
	while (at != NULL) {
		if (at->usedFree != 0) {
			bcnt++;
		}
		cnt++;
		at = at->next;
	}
	if ((printAll) && bcnt) {
		at = who;
		DEBUG_PRINTF("memcheck:%s\n", queueName);
		while (at != NULL) {
			if ((at->usedFree != 0) && (at->owner != NULL)) {
				DEBUG_PRINTF("memcheck:Piece 0x%p size:%d used:%d left un-freed owner:%s:%d\n",
				    &at->data[PADDING_HALF],
				    (int)at->size,
				    (int)at->req,
				    at->owner,
				    (int)at->bowner);
				if (printAll > 1) {
					for (i = 0; (u_int)i < at->req; i++) {
						DEBUG_PRINTF("%2.2x ", at->data[(PADDING_HALF + i)]);
						if (((i + 1) % 16) == 0) {
							DEBUG_PRINTF("\n");
						}
					}
					if (i && ((i % 16) != 0)) {
						DEBUG_PRINTF("\n");
					}
				}
				endPat = at->data;
				j = 250;
				for (i = 0; i < PADDING_HALF; i++) {
					if (endPat[i] != (u_char)j) {
						DEBUG_PRINTF("memcheck:Overwritten memory at beg of allocated block at %p\n",
						    at);
					}
					j--;
				}
				endPat = &at->data[(PADDING_HALF + at->req)];
				for (i = 0; i < PADDING_HALF; i++) {
					if (endPat[i] != (u_char)(j)) {
						DEBUG_PRINTF("memcheck:Overwrtiten memory at end of allocated block at %p\n",
						    at);
					}
					j--;
				}
			}
			at = at->next;
		}
	} else if (printAll == 0) {
		DEBUG_PRINTF("memcheck:%s\n", queueName);
		DEBUG_PRINTF("memcheck:Totals:%d on Queue %d Aloc'd\n", cnt, bcnt);
	}
}

void
printMemStatus(int printAll)
{
	printStats(head32, printAll, "32 Byte Queue");
	printStats(head64, printAll, "64 Byte Queue");
	printStats(head128, printAll, "128 Byte Queue");
	printStats(head512, printAll, "512 Byte Queue");
	printStats(head1024, printAll, "1024 Byte Queue");
}

void
memInfo(int queue, int num)
{
	struct memLinks *theOne, *at;
	int i;

	if (queue == 32) {
		theOne = head32;
	} else if (queue == 64) {
		theOne = head64;
	} else if (queue == 128) {
		theOne = head128;
	} else if (queue == 512) {
		theOne = head512;
	} else if (queue == 1024) {
		theOne = head1024;
	} else {
		DEBUG_PRINTF("memcheck:Unknown queue %d 32/64/128/512/1024\n", queue);
		return;
	}
	for (i = 0, at = theOne; at != NULL; at = at->next, i++) {
		if (num == i) {
			DEBUG_PRINTF("memcheck:Found the %dth entry:\n", num);
			DEBUG_PRINTF("memcheck:0x%p size:%d used:%d left un-freed owner:%s:%d (%s:%d)\n",
			    &at->data[PADDING_HALF],
			    (int)at->size,
			    (int)at->req,
			    at->owner, at->bowner,
			    at->powner, at->bpowner);

			for (i = 0; (u_int)i < at->req; i++) {
				DEBUG_PRINTF("%2.2x ", at->data[(PADDING_HALF + i)]);
				if (((i + 1) % 16) == 0) {
					DEBUG_PRINTF("\n");
				}
			}
			if (i && ((i % 16) != 0)) {
				DEBUG_PRINTF("\n");
			}
			break;
		}
	}
}

void
markMemory(void *mem, char *mark, int line)
{
	struct memLinks *at, *tt;
	u_long calc;

	if (mem == NULL)
		return;
	if (((unsigned long)mem < lowest) || ((unsigned long)mem > highest)) {
		DEBUG_PRINTF("Bounds error - trying to mark memory outside valid bounds\n");
		abort();
	}
	calc = (unsigned long)mem;
	calc -= PADDING_HALF;
	calc -= (sizeof(struct memLinks) - sizeof(at->data));
	at = (struct memLinks *)calc;
	tt = validMemory(at);
	if (tt == NULL) {
		DEBUG_PRINTF("Attempt to free memory not alocated from my queue \n");
		abort();
	}
	at = tt;
	if (tt == NULL) {
		DEBUG_PRINTF("Attempt to mark memory not alocated from my queue \n");
		abort();
	}
	at->owner = mark;
	at->bowner = line;
}
