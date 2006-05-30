/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/HashedTbl.h,v 1.1 2006-05-30 18:02:01 randall Exp $


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
#ifndef __HashedTbl_h__
#define __HashedTbl_h__

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>

#ifdef  _SYSTYPE_SVR4
double sqrt(double);
#endif

#ifdef SUN
#include <osfcn.h>
#include <libc.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include "return_status.h"

#define MAXPRIME 1565807

typedef struct HashTblEnt{
  int key;
  void* keyP;
  int keySize;
  void* data;
  int postion;
  struct HashTblEnt *next;
} HashTblEnt;

void HashTblEnt_Init(HashTblEnt *e);


typedef struct HashTbl{
	HashTblEnt* collection;
}HashTbl;

typedef struct HashTableHandle{
  u_long bucket;
  u_long whichOne;
  int capacityAtTime;
} HashTableHandle;

void HashTableHandle_Init(HashTableHandle *t);



/* A HashedTbl is a general purpose hashing table that can be
 * used to keep track of data elements.
 *  
 * This set of C libraries creates and manages a "Hash Object"
 * you must initialize a object and then always supply that
 * object to the routines. It is quite dynamic and allows
 * one to add/pull from/find/search/ return to postions in
 * the table.
 * 
 * GENERAL USE:
 * 
 * First you must allocate a table:
 *
 * HashedTbl *mytbl
 * mytbl = HashedTbl_create("my name",25);
 * 
 * "my name" is a internal name so that if you ever have to go
 * in the debugger a string name is recorded with the table.
 * some printf()'ing functions present this name with there output.
 *
 * The size (25) is a estimate of how big, or how many items I will
 * want in the table. The table will be sized to 2 * estimate and then
 * rounded up to the next prime number. If the table grows beyond 
 * 1/2 used, the table size will be doubled and rehashed. This does
 * require some expense when it happens but its not as bad as it sounds
 * since the table maintains internal hashed int key's that just need
 * to be re-mod'ed over the new prime number key to find the new
 * bucket.
 *
 * To use this library generally you allocate a chunk of data you want hashed,
 * this chunk may also have the key in it (you are responsible for allocating
 * both but the table can keep them has seperate things if you wish, I 
 * generally have one structure with key and data in it I allocate).
 * Once you have your "foo" allocated and filled in you use the member
 * 
 * struct mystruct{
 *  char data1[20];
 *  char key[10];
 *  char data2[30];
 * }*foo;
 * 
 * foo = calloc(1,sizeof(struct mystruct));
 * memcpy(foo->data2,stuff,sizeof(stuff));
 * memcpy(foo->data1,otherstuff,sizeof(otherstuff));
 * memcpy(foo->key,akey,sizeof(akey));
 * 
 * HashedTbl_enter(mytbl,(void *)foo->key,(void *)foo,sizeof(foo->key));
 * 
 * to put the foo in the hash table. The return value should be LIB_STATUS_OK to say
 * it was entered. If you get a LIB_STATUS_BAD1 returned, either no memory was available
 * or possbily (unlikely in the above form) the key calculated to be
 * a negative number. If a LIB_STATUS_BAD is returned that key is already in the
 * table or the mytbl variable you supplied points to NULL.
 * 
 * Now a little digression, 
 * 
 * There are usually 3 sets of functions for every action, Plain as
 * represented above "HashedTbl_enter()" or Keyed as in "HashedTbl_enterKeyed()"
 * and Bucket as in "HashedTbl_enterBucket()". These have the following
 * differences:
 * Plain - this type takes your key masses it with an internal algorithm
 *         which produces a postive integer, and then modulo's this
 *         to the table size to select the bucket. It allocates a Bucket
 *         structure, places it all in it and drops it into the bucket
 *         (which is a linked list of items).
 *
 * Keyed - this is similar to the above, except the internal algorithm is
 *         not run. Instead you provide your own translated integer
 *         key along with all the other arguments. This is for those that
 *         have a better hashing mechanism to reduce there key. You still
 *         need to supply the pointer to the key and the size of the key so
 *         that on lookup, multiple keys with the same hash (collisions) can
 *         be sorted through. If you use a Keyed type of entry you can't use
 *         a plain type of entry on it afterwards.. since the Plain algorithm
 *         would most likely NOT hash to the key you made up :)
 *
 * Bucket- this type of method is similar to the above except you are providing
 *         the pre-populated bucket. The idea here is if you have two tables and
 *         you want to move an item from one table to another, you can remove a
 *         bucket (plain or keyed too) from one table and then enter it to another
 *         table. You only get one method on the enterBucket since you have the
 *         key already put into the table (i.e. keyed or plain) inside the
 *         structure.
 * 
 * So continuing our example without superior knowledge of a better key we would,
 * to find an entry in the table do the following:
 *
 * void *v;
 * 
 * v = HashedTbl_lookup(mytbl,(const void *)keyIwant sizeof(keyIwant),&keypp);
 * 
 * on return if v is NULL it is not in the table. The keypp is a pointer to the
 * key itself. This can be useful if you store the key and the entry inside
 * seperate data elements. You can provide a NULL pointer in place of 
 * keypp i.e. 
 *
 * v = HashedTbl_lookup(mytbl,(const void *)keyIwant sizeof(keyIwant),NULL);
 * 
 * And this will work too, just not filling in the keypp. When you
 * store the data as we did with the structure you would use this
 * second signature.
 *
 * Removing things from the table can be done as follows:
 * 
 * v =  HashedTbl_remove(mytbl,(const void *)keyIwant,sizeof(keyIwant),&keypp);
 *
 * Again if keypp is not NULL it is filled with the pointer to the key. This
 * removes it from the table. The memory is stuff you allocated i.e. in
 * v and possibly keypp so if you really want to destroy them you would need
 * to do the appropraite free(). 
 *
 * Of course you should check "v" for null .. i.e. if it is not in the
 * table v will return a NULL. 
 * 
 * Another thing you may wish to do is SEARCH/REMOVE ALL THINGS:
 * 
 * To do these sorts of things there are three members for you. rewind,
 * HashTbl_searchNext(), HashTbl_getNext(), HashTbl_rewind(), 
 * HashedTbl_searchPrev(), HashedTbl_getNextBucket(),
 * HashedTbl_savePostion(), HashedTbl_returnToPostion()
 * 
 * the functions generally start wherever from where you last made a query
 * So if we wanted to look at everything in the table we would:
 * void *v,*key;
 * int keylimit;
 * char keybuf[100];
 * key = (void *)keybuf;
 * keylimit = sizeof(keybuf);
 * HashTbl_rewind(mytbl);
 * while((v = HashTbl_searchNext(mytbl,&key,&keylimit)) != NULL){
 *  do something with key the size of the key is in keylimit
 *  keylimit = sizeof(keybuf);
 * }
 * 
 * This will visit everything in the table (as long as you DO NOT INSERT while
 * looking at each element). The reason key is given seperate (you can let
 * it default to NULL) is if you keep the data and the key seperately you may
 * want to look at the key for the data, I always (as I said above) tend to
 * like to put the key and data in the same structure .. so I don't to often
 * need to give a & of a *key  to the searchNext().
 * 
 * getNext works the same way with one exception. When getNext returns the
 * entry is removed from the table. getNext is commonly used when you want
 * to pull all things out of the table, cast them to what they really are and then
 * free them, remember you must allocate and manage the pointers that are
 * put into the table. If for some reason you don't pull everything out of
 * the table, it will be pulled out when you do the HashedTbl_destroy() but
 * of course it won't be freed (the data in the table). This would
 * result in a yucky memory leak :<
 * 
 * There is also a way to save a postion and return to it within
 * a hash table. This can be useful for long audit routines. What
 * happens is you allocated a HashTableHandle. Provide it to the
 * HashedTbl_savePostion() function. Then when you want to return
 * to that postion you do a HashedTbl_returnToPostion() with the
 * marker. This will return you so the next getNext()/searchNext() will
 * continue you where you left off (getting the next item). You can have
 * multiple markers on the table. This is good even if the item where
 * the marker is has been removed between calls.. the marker will be
 * adjusted up automatically .. kinda neat.
 * 
 * OBSCURE THINGS:
 * 
 * There are a few other incidental members getSize/getCapacity size is what is
 * in it capacity is how much it can hold (remember this is dynamic and will
 * change if the hash table grows to much).
 * 
 * getName() will copy out the string you created the table with the int 
 * limits the size that is copied into the char *.
 * 
 * getResizeCount() will tell you how many times this table has had
 * to grow, since this is kind of expensive if this is to high you should
 * think about increasing the original size of the table.
 * 
 * static functions:
 * isPrime will tell if a number is prime, this uses a  grungy brute force
 * method to find if something is prime. This function is not used by
 * the table anymore, since it uses a fixed static array of primes
 * instead of doing the brute force, to find the next prime number. You
 * may want to use it ... I believe the little routine I made to generate
 * the static structure does.
 * 
 * translateKey(const char*,int n) - will run the internal hashing algorithm on
 * a block pointed to by the char* for n size.
 * 
 * spitOutCollisionCount() will dump to the stdout a analysis of how
 * many entries are all by them self in a bucket, share a bucket with two
 * entries, share a bucket with three entries or have more than 4 entries
 * in a bucket. This can be used to help figure out if you need to use
 * your own key or how good your own key is :)
 * 
 */
#define RESIZE_THRESHOLD 2		 /** bit shift 2 to get % of resize 25% */
#define GROWTH_AMOUNT 1			 /** How much to grow GROWTH_AMOUNT != RESIZE_THRESHOLD.*/

typedef struct HashedTbl{
  char myName[64];
  int currentSize;
  int currentUsed;
  int circularAt;
  int resizeCount;
  int lastIndexUsed;
  u_long lastHandle;
  int strayCount;
  HashTblEnt *strays;
  HashTblEnt *resetNeeded;
  HashTbl* hTbl;
}HashedTbl;

/* Functions that are for the tables internal
 * use and not public consumption.
 */
HashTblEnt * __Hashed_lookup(HashedTbl *,void *,int);
HashTblEnt * __Hashed_lookupKeyed(HashedTbl *,int,void *,int);
void __HashedTbl_reSize(HashedTbl *);
int __HashedTbl_getOptSize(int,int *);
int ___getHandle(HashedTbl *);

HashedTbl *
HashedTbl_create(char*name,int sizeEstimate);

void
HashedTbl_destroy(HashedTbl *tbl);

/*
 * An enter routine with the Bucket (moving from one table to another).
 */
int HashedTbl_enterBucket(HashedTbl *tbl,HashTblEnt *bucket);


/*
 * A enter routine that is generic and runs the internal hashing 
 * function.
 *
 * key - is a pointer to the key data which will be kept by the table
 * for reference to when comparing.
 * datum - The data pointer to be kept.
 * sizeKey - The size of the key dat.
 */
int HashedTbl_enter(HashedTbl *tbl,void* key,void* datum ,int sizekey);

/*
 * The keyed version of the enter routine. Here you are supplying
 * the already pre-hashed integer (with your hashing routine) and
 * of course a pointer to the key data and all the rest of the stuff.
 *
 * key - A translated key.
 * datum - The data pointer to be kept.
 * keyp - is a pointer to the key data which will be kept by the table
 * for reference to when comparing.
 * sizeKey - The size of the key data.
 */
int
HashedTbl_enterKeyed(HashedTbl *tbl,int keyint,void* datum,void* key,int sizeKey);



/*
 * The most common lookup function. It is used when you are allowing the
 * table to use its internal hashing function.
 *
 * keyP - Pointer to key data.
 * sizeKey - lenght of the key.
 */
void* HashedTbl_lookup(HashedTbl * tbl,const void *key ,int keysize,void **keypp);


/*
 * Lookup function to find a entry in the table when you are supplying
 * the hashing algorithm.
 *
 * key - Translated key value.
 * keyP - Pointer to key data.
 * sizeKey - lenght of the key.
 * keyfill, pointer to void pointer to get a return key pointer
 */
void* HashedTbl_lookupKeyed(HashedTbl * tbl,int keyint ,const void *key,int sizekey,void **keypp);

/*
 * The removal function when you want to remove a entry and it
 * was entered using the internal tables keying mechanism.
 *
 * keyP - Pointer to key data.
 * sizeKey - lenght of the key.
 * kold - Is a pointer to a void * that will be filled in with
 * the key pointer.
 */
void* HashedTbl_remove(HashedTbl *tbl,void* key,int sizekey,void** keypp);


/*
 * The removal function that is used when you are doing the key translation. 
 *
 * key - Integer translated key.
 * keyP - Pointer to key data.
 * sizeKey - length of the key.
 * kold - Is a pointer to a void * that will be filled in with
 * the key pointer.
 */
void* HashedTbl_removeKeyed(HashedTbl *,int,void*,int,void**);

/*
 * This function is the one used when you wish to look up
 * a key in the table and move it to another table.  It
 * returns the internal structure used to maintain the
 * table. The data is yours to free if you do not
 * put it into another hash table.
 */
HashTblEnt *HashedTbl_removeBucket(HashedTbl *tbl,void *key,int sizekey);

/*
 * This is the version of removeBucket where you are supplying the
 * key instead of using the tables internal hashing mechanism.
 *
 */
HashTblEnt *HashTbl_removeBucketKeyed(HashedTbl *tbl,int keyint,void* key,int sizekey);

/*
 * Destructivly remove the next entry from the hash table keyP is
 * filled with the void * key pointer and the returned value is the
 * data placed in the hash table.
 *
 * keyP - key pointer to be filled.
 */
void* HashedTbl_getNext(HashedTbl *tbl,void **keypp);

/*
 * NON- Destructivly examine the next entry from the hash table keyP is
 * filled with the void * key pointer and the returned value is the
 * data placed in the hash table.
 *
 * keyP - key pointer to a pointer to be filled in.
 * keyS - size of the key in keyP, the int size is assigned in here.
 */
void* HashedTbl_searchNext(HashedTbl *tbl,void **keypp,int *keyS);

/*
 * NON- Destructivly examine the previous entry in the hash table keyP is
 * filled with the void * key pointer and the returned value is the
 * data placed in the hash table. After this operation a searchNext
 * will return the same data element again.
 *
 * keyP - key pointer to be filled.
 */
void* HashedTbl_searchPrev(HashedTbl *tbl,void **keypp);

/*
 * Reset the hashtable to start at the beginning.
 */
void HashedTbl_rewind(HashedTbl *tbl); 


/*
 * Returns how many entries are IN the table.
 */
int HashedTbl_getSize(HashedTbl *tbl);

/*
 * Returns how many entries can fit in
 * the current table, i.e. what is the size of the allocated
 * array. Only 1/2 of what is returned will fit before
 * the table is expanded.
 */
int HashedTbl_getCapacity(HashedTbl *tbl);

/*
 * Returns the string based name that was given to the table.
 *
 * fillname - string to fillup.
 * fillsize - maxium size that fillname will hold.
 */
int HashedTbl_getName(HashedTbl *tbl,char *stringtofill,int maxsizeofstring);

/*
 * Return the number of times the hash table has been
 * grown by a resize. This is a expensive operation (resizing) and
 * should be minimized in a production system so you may want to
 * use this to audit (occasionally) to see if your intial estimate
 * was off to badly.
 */
int HashedTbl_getResizeCount(HashedTbl *tbl);

/*
 * This function will send to the standard output a breakdown
 * of how many entries are in each bucket. It tells you
 * how many singleton, double, triple and over triple
 * entries are in the hash table. It can be useful if
 * you wish to develop your own keying mechanism.
 */
int HashedTbl_spitOutCollisionCount(HashedTbl *tbl);

/*
 * This function will translate a array of characters
 * of a given size into a interger key. This is the 
 * default key generation code used if a alternate integer
 * key is not provided. 
 *
 * buf - The data buffer containing the key.
 * sz - The size in octects of the key in the
 * data buffer.
 */
int HashedTbl_translateKey(const char *buf,int sz);

/*
 * Destructivly remove the next entry from the hash table.
 * The data returned is a pointer to a allocated hash bucket
 * structure. The caller is responsible for deleting the bucket
 * after they are finished. This is most commonly used to
 * move things from one table to another without memory
 * allocation or copying (as long as the new table does
 * not resize :/).
 *
 * unused - A unused parameter to deliniate this member from
 * the other getNext().
 */
HashTblEnt *HashedTbl_getNextBucket(HashedTbl *tbl);

/*
 * This function will takes the input hash table handle and
 * marks the current postion (during a search). 
 * The postion can then be used with a returnToPostion
 * to reset the hash table to that marker. The return value 
 * LIB_STATUS_GOOD indicates a marker was generated; LIB_STATUS_BAD
 * tells you that you are at the end of the hash table i.e. no more left
 * to mark upon.
 */
int HashedTbl_savePostion(HashedTbl *tbl,HashTableHandle *marker);

/*
 * This function will return you to the postion in the
 * table that the marker indicates you left off at.
 * If the hash table has been re-hashed the marker will be
 * invalid and a LIB_STATUS_BAD will be returned. Otherwise you
 * will be repostioned to the previous location. Please
 * note that if an add of a entry has transpired
 * you may not see the added entry in your search through
 * the hash table. Deleting an entry will not cause a problem
 * but adding will :/
 */
int HashedTbl_returnToPostion(HashedTbl *tbl,HashTableHandle *marker);

/*
 * This function will return a boolean 
 * value 0 for false and 1 for true. If the
 * input number n is prime it will return true.
 * If it is not prime it will return false. It is
 * grungy and just does a simple calculation to 
 * see if the number is prime.
 *
 * n - The number being tested to determine if it
 * is a prime number
 */
int
HashedTbl_isPrime(int n);


/*
 * This function will audit the internal table mechanism
 * to see if the hash table code itself has a fault. It 
 * was used by me during some early debugging about
 * 8 - 10 years ago.. yick's. What it does is run
 * through the table to count to see if the size the
 * table thinks it is matches the actual number of elements
 * in it. It should ALWAYS return LIB_STATUS_GOOD. If not it
 * is either a memory corruption OR a bug in the table.. yeah
 * we probably still have some... but you never know :)
 */
int HashedTbl_auditTable(HashedTbl *);

#endif

