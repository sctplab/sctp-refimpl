/*	$Header: /usr/sctpCVS/rserpool/lib/HashedTbl.c,v 1.1 2006-05-30 18:02:01 randall Exp $ */

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

#include "HashedTbl.h"
#include "primeList.h"
// General purpose hashing table class

void HashTblEnt_Init(HashTblEnt *e){
  e->keyP = NULL;
  e->data = NULL;
  e->key = -1;
  e->keySize = 0;
  e->postion = 0;
  e->next = NULL;
}

void HashTableHandle_Init(HashTableHandle *t){
    t->bucket = t->whichOne = 0;
    t->capacityAtTime = 0;
}

void HashedTbl_rewind(HashedTbl *e){ 
  e->circularAt = 0; 
  e->resetNeeded = NULL; 
}

int HashedTbl_getCapacity(HashedTbl *e) { return(e->currentSize); }
int HashedTbl_getSize(HashedTbl *e) { return(e->currentUsed); }
int HashedTbl_getResizeCount(HashedTbl *e) { return e->resizeCount; }

HashedTbl *
HashedTbl_create(char* name,int suggest) 
{
  HashedTbl *e;
  e = calloc(1,sizeof(HashedTbl));
  if(e == NULL)
    return(NULL);
  if(strlen(name) > (sizeof(e->myName)-1)){
    strncpy(e->myName,name,(sizeof(e->myName)-1));
    e->myName[(sizeof(e->myName)-1)] = 0;
  }else{
    strcpy(e->myName,name);
  }
  e->strayCount = 0;
  e->strays = NULL;
  e->circularAt = 0;
  e->resetNeeded = NULL;
  e->resizeCount = 0;
  e->lastHandle = 0;
  e->currentUsed = 0;
  e->lastIndexUsed = 0;
  e->currentSize = __HashedTbl_getOptSize((suggest<<GROWTH_AMOUNT),&e->lastIndexUsed);
  e->hTbl = calloc(e->currentSize,sizeof(HashTbl));
  if(e->hTbl == NULL){
    free(e);
    return(NULL);
  }else{
    memset(e->hTbl,0,(e->currentSize*sizeof(HashTbl)));
  }
  return(e);
}

void
HashedTbl_destroy(HashedTbl *o)
{
  int i;
  HashTblEnt* hunt,*kill;
  /* get rid of all collection linked lists */
  if(o->hTbl != NULL){
    for(i=0;i<o->currentSize;i++){
      /* For every Bucket list */
      hunt = o->hTbl[i].collection;
      while(hunt != NULL){
	/* munge through the buckets killing entries */
	kill = hunt;
	hunt = hunt->next;
	kill->next = NULL;
	free(kill);
      }
    }
    /* zap the table */
    free(o->hTbl);
    o->hTbl = NULL;
    o->currentSize = 0;
    o->currentUsed = 0;
  }
  free(o);
}


int
HashedTbl_enterBucket(HashedTbl *o,HashTblEnt *pp)
{
  int where;
  /* Check to see if there is a proper table */
  if(o->hTbl == NULL)
    /* huh, TSNH :) */
    return(LIB_STATUS_BAD);

  if(pp->key < 0){
    /* can't put a -1 key in table. */
    return(LIB_STATUS_BAD1);
  }
  /* is it in the table already, if so reject it */
  if(HashedTbl_lookupKeyed(o,pp->key,pp->keyP,pp->keySize,NULL)){
    /* already in the table. */
    return(LIB_STATUS_BAD);
  }
  if((o->currentUsed+1) >= (o->currentSize/2)){
    /* grow the table when we get close to 1/2 density */
    __HashedTbl_reSize(o);
  }
  /* what bucket should we use */
  where = pp->key % o->currentSize;
  if(where < 0){
    /* Not allowed to have a negative bucket, huh? TSNH */
    return(LIB_STATUS_BAD2);
  }
  if(o->hTbl[where].collection == NULL){
    /* simple first in, drop it in the bucket */
    o->hTbl[where].collection = pp;
    pp->next = NULL;
  }else{
    /* more complex :) insert at top of chain */
    pp->next = o->hTbl[where].collection;
    o->hTbl[where].collection = pp;
  }
  o->currentUsed++;
  return(LIB_STATUS_GOOD);
}


void
__HashedTbl_reSize(HashedTbl *o)
{
  HashTbl *oldTbl;
  HashTblEnt *pp,*hunt;
  int oldSize;
  int oldUsed,i,calc,ret;

  o->resizeCount++;
  /* copy the old table to a spare area */
  oldSize = o->currentSize;
  oldUsed = o->currentUsed;
  oldTbl = o->hTbl;

  /* double table size */
  /* Figure out how much we should grow */
  calc = o->currentSize >> GROWTH_AMOUNT;
  /* grow bye 50 % if g_a = 1. */
  o->currentSize += calc;

  o->currentUsed = 0;

  /* get next best prime number for size. */
  o->currentSize = __HashedTbl_getOptSize(o->currentSize,&o->lastIndexUsed);
	

  o->hTbl = calloc(o->currentSize,sizeof(HashTbl));
  if(o->hTbl == NULL){
    /* real trouble I can't increase the table size  */
    /* restore to old size and return */
    o->currentSize = oldSize;
    o->hTbl = oldTbl;
    return;
  }
  /* move the table by redistributing the keys out */
  for(i=0;i<oldSize;i++){
    if(oldTbl[i].collection != NULL){
      hunt = oldTbl[i].collection;
      do{
	pp = hunt;
	hunt = pp->next;
	pp->next = NULL;
	pp->postion = 0;
	if((ret = HashedTbl_enterBucket(o,pp)) < 1){
	  /* Add to stray list, this really
	   * should NOT happen.
	   */
	  pp->next = o->strays;
	  o->strays = pp;
	  o->strayCount++;
	}
      }while(hunt != NULL);
    }
  }
  /* now kill out the old empty table */
  free(oldTbl);
  o->lastHandle = 0;
  return;
}

int
HashedTbl_enterKeyed(HashedTbl *o,int key,void* newt,void* keyp,int sizeKey)
{
  int where;
  HashTblEnt* pp;

  /* verify there is a table :) */
  if(o->hTbl == NULL)
    return(LIB_STATUS_BAD);

  /* key less than 0 not allowed !! */
  if(key < 0){
    /* can't put a -1 key in table. */
    return(LIB_STATUS_BAD1);
  }

  /* is it already in the table? */
  if(HashedTbl_lookupKeyed(o,key,keyp,sizeKey,NULL)){
    /* already in the table. */
    return(LIB_STATUS_BAD);
  }

  /* do we need to grow the table ? */
  if((o->currentUsed+1) >= (o->currentSize - (o->currentSize >> RESIZE_THRESHOLD))){
    /* if we reach Specifed power of 2 full */
    __HashedTbl_reSize(o);
  }

  /* calculate the bucket */
  where = key % o->currentSize;
  if(where < 0){
    /* Huh, TSNH */
    return(LIB_STATUS_BAD1);
  }
  pp = calloc(1,sizeof(HashTblEnt));
  if(pp == NULL)
    return(LIB_STATUS_BAD1);

  HashTblEnt_Init(pp);
  pp->key = key;
  pp->keySize = sizeKey;
  pp->data = newt;
  pp->keyP = keyp;
  /* set marker for rewind/seek to 0 */
  pp->postion = 0;
  if(o->hTbl[where].collection == NULL){
    /* simple first in, just plop in the list */
    o->hTbl[where].collection = pp;
  }else{
    /* Hmm insert ahead of other entries */
    pp->next = o->hTbl[where].collection;
    o->hTbl[where].collection = pp;
  }
  o->currentUsed++;
  return(LIB_STATUS_GOOD);
}


int
HashedTbl_enter(HashedTbl *o,void *key,void* newt,int sizeKey)
{
  int newKey;
  /* Do we have a real table here? */
  if(o->hTbl == NULL)
    return(LIB_STATUS_BAD);
  /* Just figure out the key with our hash algorithm */
  newKey = HashedTbl_translateKey((char *)key,sizeKey);
  /* now call the Keyed entry routine */
  return(HashedTbl_enterKeyed(o,newKey,newt,key,sizeKey));
}


void *
HashedTbl_lookupKeyed(HashedTbl *o,int key,const void *keyP,int sizeKey,void **keyFill)
{
  int where;
  /* return the void* stored in the table OR 
   * NULL if we can not find it
   */
  if(!o->currentUsed){
    /* nothing in the table */
    return(NULL);
  }

  where = key % o->currentSize;
  if(where < 0){
    /* Huh? a key that becomes less than 0 is not allowed in the
     * table.
     */
    return(NULL);
  }
  if(o->hTbl[where].collection != NULL){
    /* look for the key index and match of the memory area */
    HashTblEnt* pp;

    /* grab off the bucket array */
    pp = o->hTbl[where].collection;
    /* now search through it all */
    while(pp != NULL){
      /* if the key matches AND the keySize is the same,
       * then compare the key memory.. if they match
       * we found the entry
       */
      if((pp->key == key) && (sizeKey == pp->keySize) && 
	 (memcmp(pp->keyP,keyP,sizeKey) == 0)){
	/* found it */
	if(keyFill != NULL){
	  /* fill up the key pointer for the user */
	  *keyFill = pp->keyP;
	}
	/* return the data pointer */
	return(pp->data);
      }
      /* naw, not this one check the next */
      pp = pp->next;
    }
  }
  /* not found */
  return(NULL);
}

void *
HashedTbl_lookup(HashedTbl *o,const void *keyP,int sizeKey,void **keyFill)
{
  int newKey;
  /* First get a key from the hashing algorithm */
  newKey = HashedTbl_translateKey((char *)keyP,sizeKey);
  /* now call the normalized Keyed look up */
  return(HashedTbl_lookupKeyed(o,newKey,keyP,sizeKey,keyFill));
 
}

void*
HashedTbl_removeKeyed(HashedTbl *o,int key,void *keyP,int sizeKey,void **Okey)
{
  int where;
  void* tptr, *kptr;

  if(key < 0){
    /* huh, we can't have a key less than 0 */
    return(NULL);
  }
  where = key % o->currentSize;
  if(where < 0){
    /* huh, TSNH */
    return(NULL);
  }

  /* Lets go out to the bucket list and find it now */
  kptr = tptr = NULL;
  if(o->hTbl[where].collection != NULL){
    HashTblEnt* pp,*prev;
    /* grab the bucket */
    pp = o->hTbl[where].collection;
    prev = NULL;
    while(pp != NULL){
      /* look at each bucket */
      if((pp->key == key) && 
	 (sizeKey == pp->keySize) && 
	 (memcmp(pp->keyP,keyP,sizeKey) == 0)){
	/* if the key, and keysize are the same do the
	 * memory compare, if they match we got our
	 * criminal
	 */
	/* Pull off the relavant data */
	tptr = pp->data;
	kptr = pp->keyP;
	/* ok do we have a predecessor? */
	if(prev == NULL){
	  /* Nop this is the first in the list, copy the
	   * pointer
	   */
	  o->hTbl[where].collection = pp->next;
	}else{
	  /* yep, fix the next pointer */
	  prev->next = pp->next;
	}
	/* Ok, lets see if the position index was pointing at us */
	if((o->resetNeeded != NULL) && (o->resetNeeded == pp)){
	  /* yes, we need to move the last read pointer to the next one. */
	  if(pp->next != NULL){
	    /* copy the postion pointer if we need to */
	    pp->next->postion = pp->postion;
	  }
	  /* ok now copy the next into the resetNeeded feild */
	  o->resetNeeded = pp->next;
	}
	/* ok now we free the bucket, drop out count and break */
	free(pp);
	o->currentUsed--;
	break;
      }else{
	/* nop save the previous, and move to the next
	 * bucket entry.
	 */
	prev = pp;
	pp = pp->next;
      }
    }
  }
  /* does the user want a copy of the key pointer? */
  if(Okey != NULL)
    /* yes, copy it over for them */
    *Okey = kptr;
  /* ok return the data removed */
  return(tptr);
}

void*
HashedTbl_remove(HashedTbl *o,void *keyP,int sizeKey,void **Okey)
{
  int newKey;
  /* First translate the key into a int */
  newKey = HashedTbl_translateKey((char *)keyP,sizeKey);
  /* now call the Keyed remove routine */
  return(HashedTbl_removeKeyed(o,newKey,keyP,sizeKey,Okey));
}

int
HashedTbl_isPrime(int n)
{
  int divisor;
  int prime = 1;

  int limit;
  double input;

  input = n * 1.0;
  limit = (int)(sqrt(input));
  for(divisor = 2;divisor <= limit;divisor++){
    if((n % divisor) == 0){
      prime = 0;
      break;
    }
  }
  return(prime);
}



#ifdef HASH_OPTIMIZE_FOR_TIME
int
__HashedTbl_getOptSize(int suggest,int *lastIndex)
{
  int cur;
  for(cur=*lastIndex;cur<PRIME_NUMBER_COUNT;cur++){
    if(suggest < Hashed_primeList[cur]){
      /* found one bigger than the suggested, we will take it */
      *lastIndex = cur;
      return(Hashed_primeList[cur]);
    }
  }
  return(suggest);
}
#else

int
__HashedTbl_getOptSize(int suggest,int *lastIndex)
{
  int cur;
  /* Brute force look for a prime
   * number, but takes less space.
   */
  for(cur=suggest;cur<MAXPRIME;cur++){
    if(HashedTbl_isPrime(cur)){
      return(cur);
    }
  }
  /* can't find a prime below */
  /* maxprime. return suggestion. */
  return(suggest);
}

#endif

void *
HashedTbl_getNext(HashedTbl *o,void **keyP)
{
  int i;
  void* tptr;
  HashTblEnt* pp;

  if(o->currentUsed){
    /* look in the overall table */
    for(i=o->circularAt;i<o->currentSize;i++){
      /* Start at where we left off and go through each bucket
       * until we find one to return
       */
      if(o->hTbl[i].collection != NULL){
	/* got some in this bucket list 
	 * lets pull the first one.
	 */
	pp = o->hTbl[i].collection;
	/* reset the bucket beyond this one */
	o->hTbl[i].collection = pp->next;
	pp->next = NULL;
	tptr = pp->data;
	/* If we are done with this bucket fix
	 * the circulatAt to point to the next 
	 * bucket.
	 */
	if(o->hTbl[i].collection == NULL)
	  o->circularAt = i+1;
	else
	  /* otherwise just fix it so we do come back here,
	   * this line is really just paraniod behavior :)
	   */
	  o->circularAt = i;
	/* now do we need to copy out the key? */
	if(keyP != NULL)
	  *keyP = pp->keyP;
	/* throw away the bucket container and subtract
	 * out the count, pass back the data to the
	 * user
	 */
	free(pp);
	o->currentUsed--;
	return(tptr);
      }
    }
  }
  if(!o->currentUsed){
    /* none left in table, look to strays */
    if(o->strayCount == 0)
      return(NULL);
    else{
      /* Now return the stray. */
      pp = o->strays;
      o->strays = o->strays->next;
      /* setup the null pointer in next. */
      pp->next = NULL;
      /* decrease the stray's */
      o->strayCount--;
      /* pull of what we return */
      tptr = pp->data;
      /* copy out if the caller wants the key */
      if(keyP != NULL)
	*keyP = pp->keyP;
      /* clean up the bucket */
      free(pp);
      return(tptr);
    }
  }
  /* nothing left */
  return(NULL);
}

void *
HashedTbl_searchNext(HashedTbl *o,void **keyP,int *keyS)
{
  void* tptr;
  int i;

  if(!o->currentUsed)
    /* none in the table to look at */
    return(NULL);

  /* ok first do we have a bucket we are looking in ? */
  if(o->resetNeeded != NULL){
    /* yes, lets look here */
    tptr = o->resetNeeded->data;
    /* does the guy want the key too? */
    if(keyP != NULL)
      *keyP = o->resetNeeded->keyP;
    /* ok copy the key size out as well */
    if(keyS != NULL)
      *keyS = o->resetNeeded->keySize;
    /* now move resetNeeded to next in bucket */
    o->resetNeeded = o->resetNeeded->next;
    return(tptr);
  }
  /* if we reach here we are not in the middle of a bucket list */
  for(i=o->circularAt;i<o->currentSize;i++){
    if(o->hTbl[i].collection != NULL){
      /* here is a non-empty bucket list */
      HashTblEnt* pp;
      /* pull out the first one */
      pp = o->hTbl[i].collection;
      tptr = pp->data;
      /* does the user want the key pointer? */
      if(keyP != NULL)
	*keyP = pp->keyP;
      /* does the user want the size of the key pointer? */
      if(keyS != NULL)
	*keyS = pp->keySize;
      /* ok advance our resetNeeded pointer to pick up the
       * next bucket
       */
      o->resetNeeded = pp->next;
      /* now set to run on the next bucket list
       * after this is all empty.
       */
      o->circularAt = i + 1;
      return(tptr);
    }
  }
  /* none left to look at */
  return(NULL);
}

int
HashedTbl_getName(HashedTbl *o,char *nmBuf,int sz)
{
  /* copy out the name within memory bounds */
  if((u_int)(sz-1) < strlen(o->myName)){
    strncpy(nmBuf,o->myName,(sz-1));
    nmBuf[(sz-1)] = 0;
  }else{
    strcpy(nmBuf,o->myName);
  }
  return(LIB_STATUS_GOOD);
}


int
HashedTbl_translateKey(const char *key,int len)
{
  unsigned long calcKey;
  int multipler,newKey;
  int i,zit,runSum;

  /* a fun hashing mesh for translating a
   * arbitrary string of unsigned char's into
   * a integer. 
   */
  multipler = 1;
  zit = 0;
  calcKey = 0;
  runSum = 0;
  for(i=(len-1);i>=0;i--){
    calcKey += (key[i] * multipler);
    if(i > 0)
      runSum += (key[i] * i);
    else
      runSum += key[i];

    multipler *= 128;
    zit++;
    if(((zit % 4) == 0) || (multipler == 0))
      multipler = i + runSum;
  }
  calcKey += runSum;
  newKey = abs((int)calcKey);
  return(newKey);
}

HashTblEnt *
HashedTbl_getNextBucket(HashedTbl *o)
{
  int i;
  if(!o->currentUsed)
    /* nobody left */
    return(NULL);

  /* Start at the place we left off */
  for(i=o->circularAt;i<o->currentSize;i++){
    if(o->hTbl[i].collection != NULL){
      /* we have a candidate bucket list */
      HashTblEnt* pp;
      pp = o->hTbl[i].collection;
      o->hTbl[i].collection = pp->next;
      o->currentUsed--;
      if(o->hTbl[i].collection != NULL)
	o->circularAt = i;
      else
	o->circularAt = i + 1;
      return(pp);
    }
  }
  return(NULL);
}

HashTblEnt *
HashedTbl_removeBucketKeyed(HashedTbl *o,int key,void* keyP,int sizeKey)
{
  int where;
  where = key % o->currentSize;

  /* negative keys are not allowed */
  if(where < 0){
    return(NULL);
  }
  /* ok, lets gind one to remove */
  if(o->hTbl[where].collection != NULL){
    HashTblEnt *pp,*prev;

    /* there is one here lets see if it is our man */
    pp = o->hTbl[where].collection;
    prev = NULL;
    while(pp != NULL){
      if((pp->key == key) && (sizeKey == pp->keySize) &&
	 (memcmp(pp->keyP,keyP,sizeKey) == 0)){
	/* found the one that is wanted */
	if(prev == NULL){
	  /* ok, it was the first in the bucket list */
	  o->hTbl[where].collection = pp->next;
	}else{
	  /* fix the previous to go beyond this
	   * guy.
	   */
	  prev->next = pp->next;
	}
	/* now what about the pointers and positions and such */
	if((o->resetNeeded != NULL) && (o->resetNeeded == pp)){
	  /* We need to move the last read pointer to the next one */
	  if(pp->next != NULL){
	    /* position fix too ? */
	    pp->next->postion = pp->postion;
	  }
	  o->resetNeeded = pp->next;
	}
	o->currentUsed--;
	pp->next = NULL;
	return(pp);
      }else{
	/* not this guy move to next in bucket list */
	prev = pp;
	pp = pp->next;
      }
    }
  }
  /* not found */
  return(NULL);
}


HashTblEnt *
HashedTbl_removeBucket(HashedTbl *o,void *keyP,int sizeKey)
{
  int newKey;
  newKey= HashedTbl_translateKey((char *)keyP,sizeKey);
  return(HashedTbl_removeBucketKeyed(o,newKey,keyP,sizeKey));
}


HashTblEnt *
__HashedTbl_lookupKeyed(HashedTbl *o,int key,void *keyP,int sizeKey)
{
  int where;
  if(!o->currentUsed){
    return(0);
  }

  where = key % o->currentSize;
  if(where < 0){
    /* TSNH */
    return(NULL);
  }

  if(o->hTbl[where].collection != NULL){
    // look up for key in collection list
    HashTblEnt* pp;
    pp = o->hTbl[where].collection;
    while(pp != NULL){
      if((pp->key == key) && (sizeKey == pp->keySize) &&
	 (memcmp(pp->keyP,keyP,sizeKey) == 0)){
	/* found it */
	return(pp);
      }
      pp = pp->next;
    }
  }
  return(NULL);	
}

HashTblEnt *
__HashedTbl_lookup(HashedTbl *o,void *keyP,int sizeKey)
{
  int key = HashedTbl_translateKey((char *)keyP,sizeKey);
  return(__HashedTbl_lookupKeyed(o,key,keyP,sizeKey));
}


int
HashedTbl_spitOutCollisionCount(HashedTbl *o)
{
  /* debug routine */
  int first,second,moreThanThree,third,i;
  HashTblEnt *pp;
  int count;
  first = second = third = moreThanThree = 0;
  for(i=0;i<o->currentSize;i++){
    if(o->hTbl[i].collection != NULL){
      pp = o->hTbl[i].collection;
      count = 0;
      while(pp != NULL){
	count++;
	pp = pp->next;
      }
      switch(count){
      case 1:
	first++;
	break;
      case 2:
	second++;
	break;
      case 3:
	third++;
	break;
      default:
	moreThanThree++;
	break;
      }
    }
  }
  return(LIB_STATUS_GOOD);
}

int
HashedTbl_getHandle(HashedTbl *o)
{
  o->lastHandle++;
  if((o->lastHandle > 0x0000ffff) ||
     (o->lastHandle == 0)){
    o->lastHandle = 1;
  }
  return(o->lastHandle);
}

int
HashedTbl_savePostion(HashedTbl *o,HashTableHandle *marker)
{
  int retVal;
  marker->bucket = o->circularAt-1;
  marker->capacityAtTime = o->currentSize;
  if(o->resetNeeded != NULL){
    retVal = 0;
    if(o->resetNeeded->postion == 0){
      o->resetNeeded->postion = (int)(0x00010000 | HashedTbl_getHandle(o));
      marker->whichOne = (u_long)o->resetNeeded->postion & 0x0000ffff;
    }else{
      u_short x;
      marker->whichOne = (u_long)o->resetNeeded->postion & 0x0000ffff;
      x = (u_short)((o->resetNeeded->postion >> 16) & 0x0000ffff);
      x++;
      o->resetNeeded->postion = (int)((x << 16) | (marker->whichOne));
    }
  }else{
    int i;
    retVal = 1;
    for(i=marker->bucket+1;i<o->currentSize;i++){
      if(o->hTbl[i].collection != NULL){
	retVal = 0;
	marker->bucket = i;
	if(o->hTbl[i].collection->postion == 0){
	  o->hTbl[i].collection->postion = (int)(0x00010000 | HashedTbl_getHandle(o));
	  marker->whichOne = (u_long)o->hTbl[i].collection->postion & 0x0000ffff;
	}else{
	  u_short x;
	  marker->whichOne = (u_long)o->hTbl[i].collection->postion & 0x0000ffff;
	  x = (u_short)((o->hTbl[i].collection->postion >> 16) & 0x0000ffff);
	  x++;
	  o->hTbl[i].collection->postion = (int)((x << 16) | (marker->whichOne));
	}
	break;
      }
    }
  }
  if(retVal){
    marker->bucket = o->currentSize-1;
    return( LIB_STATUS_BAD);
  }
  return(LIB_STATUS_GOOD);
}

int
HashedTbl_returnToPostion(HashedTbl *o,HashTableHandle *marker)
{
  HashTblEnt *pp,*qq;

  if(marker->capacityAtTime != o->currentSize){
    /* it has grown or a invalid  handle is given. */
    return(LIB_STATUS_BAD);
  }
  if(marker->whichOne == 0){
    /* we have returned to the end of the table */
    /* good, but the next get will return NULL */
    o->circularAt = o->currentSize;
    return(LIB_STATUS_GOOD);
  }
  pp = o->hTbl[marker->bucket].collection;
  if(pp != NULL){
    while(pp != NULL){
      if((u_long)(pp->postion&0x0000ffff) == marker->whichOne){
	/* Found the guy. Clear the marker. */
	u_short x,y;
	x = ((pp->postion >> 16) & 0x0000ffff);
	x--;
	if(x == 0){
	  pp->postion = 0;
	}else{
	  y = (u_short)(pp->postion & 0x0000ffff);
	  pp->postion = (int)((x << 16) | y);
	}
	break;
      }else{
	qq = pp->next;
	pp = qq;
      }
    }
  }
  /* Move to this index + 1 since that is  where circularAt was at. */
  o->circularAt = marker->bucket + 1;
  if(pp == NULL){
    /* Not found so we must be at the next bucket. 
     * It was probably removed.
     */
    o->resetNeeded = NULL;
    return(LIB_STATUS_GOOD);
  }else{
    /* ok, we need to setup to possibly continue in
     * this bucket list.
     */
    o->resetNeeded = pp;
  }
  return(LIB_STATUS_GOOD);
}



void *
HashedTbl_searchPrev(HashedTbl *o,void **keyP)
{
  int i;
  HashTblEnt *pp,*qq;

  if(!o->currentUsed)
    /* none in table. */
    return(NULL);

  if(o->resetNeeded != NULL){
    /* May be in this bucket. */
    if(o->circularAt > 0){
      pp = o->hTbl[(o->circularAt-1)].collection;
    }else{
      /* Huh something's not right */
      pp = o->hTbl[0].collection;
    }
    if(pp == o->resetNeeded){
      /* We are at the top of the bucket so don't look we must back up. */
      o->resetNeeded = NULL;
      o->circularAt--;
      if(o->circularAt < 0){
	o->circularAt = 0;
      }
    }else{
      while(pp != NULL){
	if(pp == o->resetNeeded){
	  /* we hit here if resetNeeded is first entry in
	   * bucket.. which it should not be.
	   */
	  break;
	}
	if(pp->next == o->resetNeeded){
	  /* Found the guy */
	  o->resetNeeded = pp;
	  if(keyP != NULL)
	    *keyP = o->resetNeeded->keyP;
	  return(o->resetNeeded->data);
	}else{
	  qq = pp->next;
	  pp = qq;
	}
      }
    }
  }
  for(i=o->circularAt-1;i>=0;i--){
    if(o->hTbl[i].collection != NULL){
      /* Somethings in the bucket,  get the last one. */
      o->resetNeeded = o->hTbl[i].collection;
      while(o->resetNeeded->next != NULL){
	/* Move forward to last one. */
	pp = o->resetNeeded->next;
	o->resetNeeded = pp;
      }
      if(keyP != NULL)
	*keyP = o->resetNeeded->keyP;
      o->circularAt = i+1;
      return(o->resetNeeded->data);
    }
  }
  return(NULL);
}

int
HashedTbl_auditTable(HashedTbl *o)
{
  HashTblEnt *pp,*qq;
  int totalCnt,i;
  totalCnt = 0;
  /* verify the sum of data in the hashtable 
   * versus what is really in it
   */
  for(i=0;i<o->currentSize;i++){
    if(o->hTbl[i].collection == NULL)
      continue;
    pp = o->hTbl[i].collection;
    while(pp != NULL){
      totalCnt++;
      qq = pp->next;
      pp = qq;
    }
  }
  if(totalCnt != o->currentUsed){
    return(LIB_STATUS_BAD);
  }
  return(LIB_STATUS_GOOD);
}


