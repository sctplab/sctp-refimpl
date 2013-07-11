/*	$Header: /usr/sctpCVS/rserpool/lib/distributor.c,v 1.1 2006-05-30 18:02:01 randall Exp $ */

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

#include <distributor.h>

hashableHash *
create_hashableHash(int pid,char *string,int sz)
{
  hashableHash *o;
  o = calloc(1,sizeof(hashableHash));
  if(o != NULL){
    o->hash = HashedTbl_create(string,sz);
    if(o->hash != NULL){
      o->protocolId = pid;
    }else{
      free(o);
      o = NULL;
    }
  }
  return(o);
}

void
destroy_hashableHash(hashableHash *o)
{
  HashedTbl_destroy(o->hash);
  free(o);
}



hashableDlist *
create_hashdlist(int strm)
{
  hashableDlist *o;
  o = calloc(1,sizeof(hashableDlist));
  if(o != NULL){
    o->list = dlist_create();
    if(o->list != NULL){
      o->streamNo = strm;
    }else{
      free(o);
      o = NULL;
    }
  }
  return(o);
}

void
destroy_hashDlist(hashableDlist *o)
{
  dlist_destroy(o->list);
  free(o);
}


distributor *
createDistributor()
{
  int ret,i;
  distributor *o;
  o = (distributor *)calloc(1,sizeof(distributor));
  if(o == NULL)
    return(NULL);
  o->fdSubscribersList = dlist_create();
  if(o->fdSubscribersList == NULL){
    /* failed to create */
    free(o);	
    return(NULL);
  }
  o->lazyClockTickList = dlist_create();
  if(o->lazyClockTickList == NULL){
    dlist_destroy(o->fdSubscribersList);
    free(o);	
    return(NULL);
  }
  o->startStopList = dlist_create();
  if(o->startStopList == NULL){
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    free(o);	
    return(NULL);
  }


  /* we specify 11 subscribers */
  o->fdSubscribers = HashedTbl_create("File Descriptor Subscribers", 11);
  if(o->fdSubscribers == NULL){
    /* failed to create */
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);	
    return(NULL);
  }
  o->msgSubscriberList = dlist_create();
  if(o->msgSubscriberList == NULL){
    /* failed to create */
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);	
    return(NULL);
  }
  o->msgSubscribers = HashedTbl_create("Master Hash of Hashes", 11);
  if(o->msgSubscribers == NULL){
    /* failed to create */
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);	
    return(NULL);
  }
  o->msgSubZero = create_hashableHash(DIST_SCTP_PROTO_ID_DEFAULT,
				      "Master Hash of Zero PID",
				      11);
  if(o->msgSubZero == NULL){
    /* failed to create */
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);	
    return(NULL);
  }
  o->msgSubZeroNoStrm = create_hashdlist(DIST_STREAM_DEFAULT);
  if(o->msgSubZeroNoStrm == NULL){
    /* failed to create */
    destroy_hashableHash(o->msgSubZero);
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);	
    return(NULL);
  }
  o->numfds = DEFAULT_POLL_LIST_SZ;
  o->numused = 0;
  o->fdlist = (struct pollfd *)calloc(DEFAULT_POLL_LIST_SZ,sizeof(struct pollfd));
  if(o->fdlist == NULL){
    /* failed to create */
    destroy_hashDlist(o->msgSubZeroNoStrm);
    destroy_hashableHash(o->msgSubZero);
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);
    return(NULL);
  }
  /* init fd structs */
  for(i=0;i<o->numfds;i++){
    o->fdlist[i].fd = -1;
    o->fdlist[i].events = 0;
    o->fdlist[i].revents = 0;
  }
  o->timerlist = dlist_create();
  if(o->timerlist == NULL){
    /* failed to create */
    free(o->fdlist);
    destroy_hashDlist(o->msgSubZeroNoStrm);
    destroy_hashableHash(o->msgSubZero);
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);
    return(NULL);
  }
  /* now that everything is created lets set some
   * of the relationships up.
   */

  /* we first insert into the msgSubZero Hash table the
   * default stream list.
   */
  ret = HashedTbl_enterKeyed(o->msgSubZero->hash,
		       o->msgSubZeroNoStrm->streamNo,
		       (void*)o->msgSubZeroNoStrm,
		       (void*)&o->msgSubZeroNoStrm->streamNo,
		       sizeof(int));
  if(ret != LIB_STATUS_GOOD){
    /* We have a problem. bail. */
    dlist_destroy(o->timerlist);
    free(o->fdlist);
    destroy_hashDlist(o->msgSubZeroNoStrm);
    destroy_hashableHash(o->msgSubZero);
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);
    return(NULL);
  }

  /* Now we must put the default PID hash into
   * the master hashtable.
   */
  ret = HashedTbl_enterKeyed(o->msgSubscribers,
			     o->msgSubZero->protocolId,
		       (void*)o->msgSubZero,
		       (void*)&o->msgSubZero->protocolId,
		       sizeof(int));
  if(ret != LIB_STATUS_GOOD){
    /* We have a problem. bail. */
    dlist_destroy(o->timerlist);
    free(o->fdlist);
    destroy_hashDlist(o->msgSubZeroNoStrm);
    destroy_hashableHash(o->msgSubZero);
    HashedTbl_destroy(o->msgSubscribers);
    dlist_destroy(o->msgSubscriberList);
    HashedTbl_destroy(o->fdSubscribers);
    dlist_destroy(o->fdSubscribersList);
    dlist_destroy(o->lazyClockTickList);
    dlist_destroy(o->startStopList);
    free(o);
    return(NULL);
  }
  /* set the I am not done flag */
  o->notdone = 1;

  /* setup clock tick */
  o->idleClockTick.tv_sec = DIST_IDLE_CLOCK_SEC;
  o->idleClockTick.tv_usec = DIST_IDLE_CLOCK_USEC;
  /* get the last known time setup */
  gettimeofday(&o->lastKnownTime,(struct timezone *)NULL);

  return(o);
}

void
destroyDistributor(distributor *o)
{
  timerEntry *te;
  fdEntry *fe;
  hashableHash *hh;
  hashableDlist *he;
  msgConsumer *mc;
  auditTick *at;
  StartStop *sat;
  void *keyP;
  /* we must get each element out of the
   * multi-keyed tables before destroying.
   * since both hashable lists that we
   * keep a pointer to for shortcuts in
   * our main structure are in the individual lists
   * and will be found that way there is no need
   * to do a :
   * destroy_hashDlist(o->msgSubZeroNoStrm);
   * <or>
   * destroy_hashableHash(o->msgSubZero);
   */

  /* the list of timers first. */
  dlist_reset(o->timerlist);
  while((te = (timerEntry *)dlist_getNext(o->timerlist)) != NULL){
    free(te);
  }
  dlist_destroy(o->timerlist);

  /* now free audit tick list */
  dlist_reset(o->lazyClockTickList);
  while((at = (auditTick *)dlist_getNext(o->lazyClockTickList)) != NULL){
    free(at);
  }
  dlist_destroy(o->lazyClockTickList);

  /* now free the start/stop list */
  dlist_reset(o->startStopList);
  while((sat = (StartStop *)dlist_getNext(o->startStopList)) != NULL){
    free(sat);
  }
  dlist_destroy(o->startStopList);

  /* get rid of all the file descriptors */
  free(o->fdlist);
  /* here we dump the hash table since all entries
   * are also maintained on the dlist too
   */
  HashedTbl_destroy(o->fdSubscribers);

  /* Now go through and free up all on our list */
  dlist_reset(o->fdSubscribersList);
  while((fe = (fdEntry *)dlist_getNext(o->fdSubscribersList)) != NULL){
    free(fe);
  }
  dlist_destroy(o->fdSubscribersList);

  /* Now we must go through the hash table
   * of hashableHashes. And for each hashableHash
   * pull all the hashableLists out. Each list is
   * just destroyed since we will cleanup the underlying
   * msgConsumers when we destroy the global msgSubscriberList.
   *
   */
  HashedTbl_rewind(o->msgSubscribers);
  while((hh = (hashableHash *)HashedTbl_getNext(o->msgSubscribers,&keyP)) != NULL){
    /* we have a hashable hash, clean it up */
    HashedTbl_rewind(hh->hash);
    while((he = (hashableDlist *)HashedTbl_getNext(hh->hash,&keyP)) != NULL){
      /* we purge since list entries are all in msgSubscribers */
      destroy_hashDlist(he);
    }
    destroy_hashableHash(hh);
  }
  HashedTbl_destroy(o->msgSubscribers);

  /* Now lets go out and get the msg subscriber entries too. */
  dlist_reset(o->msgSubscriberList);
  while((mc = (msgConsumer *)dlist_getNext(o->msgSubscriberList)) != NULL){
    free(mc);
  }
  dlist_destroy(o->msgSubscriberList);
  /* ok free the base and we are clean */
  free(o);
}

int
dist_TimerStart(distributor *o,timerFunc f, u_long a, u_long b, void *c, void *d)
{
  int ret;
  timerEntry *te,*pe;
  /* allocate a new timer element */
  te = calloc(1,sizeof(timerEntry));
  if(te == NULL){
    return(LIB_STATUS_BAD);
  }
  te->arg1 = c;
  te->arg2 = d;
  te->action = f;
  te->tv_sec = a;
  te->tv_usec = b;
#ifdef MINIMIZE_TIME_CALLS
  te->started.tv_sec = o->lastKnownTime.tv_sec;
  te->started.tv_usec = o->lastKnownTime.tv_usec;
#else
  gettimeofday(&te->started,(struct timezone *)NULL);
  /* update the time, which is used before we go
   * back into the poll()/select().
   */
  o->lastKnownTime.tv_sec = te->started.tv_sec;
  o->lastKnownTime.tv_usec = te->started.tv_usec;
#endif
  /* now setup the expire time */
  te->expireTime.tv_sec = te->started.tv_sec + a;
  te->expireTime.tv_usec = te->started.tv_usec + b;

  while(te->expireTime.tv_usec > 1000000){
    te->expireTime.tv_sec++;
    te->expireTime.tv_usec -= 1000000;
  }
  /* put in a special precaution because I am
   * paranoid. Set bad status in case my logic
   * is wrong :/
   */
  ret = LIB_STATUS_BAD;
  /* carefully place it on the list */
  dlist_reset(o->timerlist);
  while((pe = (timerEntry *)dlist_get(o->timerlist)) != NULL){
    if(pe->expireTime.tv_sec > te->expireTime.tv_sec){
      /* this one expires after the new one */
      ret = dlist_insertHere(o->timerlist,(void *)te);
      break;
    }else if(pe->expireTime.tv_sec == te->expireTime.tv_sec){
      /* got to check usec's since equal seconds of expiration */
      if(pe->expireTime.tv_usec > te->expireTime.tv_usec){
	/* ok we are expiring some time sooner than this
	 * dude
	 */
	ret = dlist_insertHere(o->timerlist,(void *)te);
	break;
      }
    }
    /* otherwise we expire after this one and fall out 
     * hitting the next get.
     */
  }
  if(pe == NULL){
    /* at the end of the list, and never got in append it. */
    ret = dlist_append(o->timerlist,(void *)te);
  }
  dlist_reset(o->timerlist);
  if(ret != LIB_STATUS_GOOD){
    /* we could not insert it into the list */
    /* clean up and fail */
    free(te);
    return(LIB_STATUS_BAD);
  }
  /* and now return */
  return(LIB_STATUS_GOOD);
}

int
dist_TimerStop(distributor *o,timerFunc f, void *a, void *b)
{
  /* find the first timer with matching function and args */
  /* and remove it from the list */
  timerEntry *pe,*pf;  
  dlist_reset(o->timerlist);
  while((pe = (timerEntry *)dlist_get(o->timerlist)) != NULL){
    if((pe->action == f) &&
       (pe->arg1 == a) &&
       (pe->arg2 == b)){
      pf = dlist_getThis(o->timerlist);
      if(pf != pe){
	/* tragic list failure, here
	 * during debug only.
	 */
	abort();
	return(LIB_STATUS_BAD1);
      }else{
	free(pe);
	break;
      }
    }
  }
  if(pe == NULL){
    /* never found it on the list */
    return(LIB_STATUS_BAD);
  }
  return(LIB_STATUS_GOOD);
}


int
dist_addFd(distributor *o,int fd,serviceFunc fdNotify, int mask,void *arg)
{
  fdEntry *fe;
  int i,fnd,ret;
  /* first allocate a fd service structure and init it */
  fe = calloc(1,sizeof(fdEntry));
  if(fe == NULL){
    return(LIB_STATUS_BAD);
  }
  /* init it */
  fe->onTheList = 0;
  fe->fdItServices = fd;
  fe->fdNotify = fdNotify;
  fe->arg = arg;

  /* now place it in the hash table and verify its unique */
  ret = HashedTbl_enterKeyed(o->fdSubscribers,
			     fe->fdItServices,
			     (void*)fe,
			     (void*)&fe->fdItServices,
			     sizeof(int));  

  if(ret != LIB_STATUS_GOOD){
    /* no good, probably already in */
    free(fe);
    return(ret);
  }
  /* now add it to the linked list */
  ret = dlist_append(o->fdSubscribersList,(void *)fe);
  if(ret != LIB_STATUS_GOOD){
    /* now we must back it out of the hash table */
    HashedTbl_removeKeyed(o->fdSubscribers,
			  fe->fdItServices,
			  (void*)&fe->fdItServices,
			  sizeof(int),
			  (void **)NULL);
    free(fe);
    return(ret);
  }
  /* now plop in a place in the fd select array */
  /* first do we need to grow array? */
  if((o->numused+1) >= o->numfds){
    /* yes, this will push us to have none spare */
    struct pollfd *tp;
    tp = (struct pollfd *)calloc((o->numfds*2),sizeof(struct pollfd));
    if(tp == NULL){
      void *v;
      /* can't get bigger we are out of here */
      HashedTbl_removeKeyed(o->fdSubscribers,
			    fe->fdItServices,
			    (void*)&fe->fdItServices,
			    sizeof(int),
			    (void **)NULL);
      /* push pointer so next get will get the last one */
      dlist_getToTheEnd(o->fdSubscribersList);
      /* do that so we can do a getThis() */
      v = dlist_get(o->fdSubscribersList);
      /* now pull it */
      v = dlist_getThis(o->fdSubscribersList);
      free(fe);
      return(LIB_STATUS_BAD1);
    }
    memcpy(tp,o->fdlist,(sizeof(struct pollfd) * o->numfds));
    /* init the rest of the struct */
    for(i=o->numfds;i<(o->numfds*2);i++){
      tp[i].fd = -1;
      tp[i].events = 0;
      tp[i].revents = 0;
    }
    /* adjust the size */
    o->numfds *= 2;
    /* free old list */
    free(o->fdlist);
    /* place in our new one */
    o->fdlist = tp;
  }
  fnd = 0;
  /* now lets find a entry and use it */
  for(i=o->numused;i<o->numfds;i++){
    if(o->fdlist[i].fd == -1){
      /* found a empty spot */
      o->fdlist[i].fd = fd;
      o->fdlist[i].events = mask;
      o->numused++;
      fnd = 1;
      break;
    }
  }
  if(!fnd){
    for(i=0;i<o->numused;i++){
      if(o->fdlist[i].fd == -1){
	/* found a empty spot */
	o->fdlist[i].fd = fd;
	o->fdlist[i].events = mask;
	o->numused++;
	fnd = 1;
	break;
      }
    }
  }
  if(!fnd){
    /* TSNH */
    void *v;
    HashedTbl_removeKeyed(o->fdSubscribers,
			  fe->fdItServices,
			  (void*)&fe->fdItServices,
			  sizeof(int),
			  (void **)NULL);
    /* push pointer so next get will get the last one */
    dlist_getToTheEnd(o->fdSubscribersList);
    /* do that so we can do a getThis() */
    v = dlist_get(o->fdSubscribersList);
    /* now pull it */
    v = dlist_getThis(o->fdSubscribersList);
    free(fe);
    return(LIB_STATUS_BAD);
  }
  /* now add it to the list of service functions */
  return(LIB_STATUS_GOOD);
}


int
dist_deleteFd(distributor *o,int fd)
{
  fdEntry *fe;
  int i,removed;
  /* Look up the fd, and remove */
  removed = -1;
  fe = HashedTbl_removeKeyed(o->fdSubscribers,
			     fd,
			     (void*)&fd,
			     sizeof(int),
			     (void **)NULL);
  /* find it in the fdSubscribersList and remove */
  dlist_reset(o->fdSubscribersList);
  while((fe = (fdEntry *)dlist_get(o->fdSubscribersList)) != NULL){
    if(fe->fdItServices == fd){
      fe = dlist_getThis(o->fdSubscribersList);
    }
  }
  /* remove it from the fdlist and subtract current used */
  for(i=0;i<o->numfds;i++){
    if(o->fdlist[i].fd == fd){
      o->fdlist[i].fd = -1;
      o->fdlist[i].events = 0;
      o->fdlist[i].revents = 0;
      removed = i;
      o->numused--;
      break;
    }
  }
  /* now free the memory */
  if(fe != NULL){
    free(fe);
  }
  /* For netbsd we need to collapse the list */
  if(removed != -1){
    if(removed != o->numused){
      /* if removed == o->numused 
       * we removed the last one, no
       * action needed.
       */
      o->fdlist[removed].fd = o->fdlist[o->numused].fd;
      o->fdlist[removed].events  = o->fdlist[o->numused].events;
      o->fdlist[removed].revents  = o->fdlist[o->numused].revents;
      o->fdlist[o->numused].fd = -1;
      o->fdlist[o->numused].events = 0;
      o->fdlist[o->numused].revents = 0;
    }
  }
  return(LIB_STATUS_GOOD);
}

int
dist_changeFd(distributor *o,int fd,int newmask)
{
  int i;

  for(i=0;i<o->numfds;i++){
    if(o->fdlist[i].fd == fd){
      o->fdlist[i].events = newmask;
      break;
    }
  }
  return(LIB_STATUS_GOOD);
}

int
__place_mc_in_dlist(dlist_t *dl,msgConsumer *mc)
{
  msgConsumer *at;
  dlist_reset(dl);
  while((at = (msgConsumer *)dlist_get(dl)) != NULL){
    /* if this one has a greater precedence our
     * new one slips in ahead of the one we are
     * looking at on the list 
     */
    if(at->precedence > mc->precedence){
      return(dlist_insertHere(dl,(void *)mc));
    }
  }
  /* place it at the end of the list */
  return(dlist_append(dl,(void *)mc));
}

void
__pull_from_dlist(dlist_t *dl,msgConsumer *mc)
{
  msgConsumer *at;
  dlist_reset(dl);
  while((at = (msgConsumer *)dlist_get(dl)) != NULL){
    if(at == mc){
      at = (msgConsumer *)dlist_getThis(dl);
      break;
    }
  }
}

msgConsumer *
__pull_from_dlist2(dlist_t *dl,messageFunc mf,int sctp_proto, int stream_no,void *arg)
{
  msgConsumer *at,*nat;
  nat = NULL;
  dlist_reset(dl);
  while((at = (msgConsumer *)dlist_get(dl)) != NULL){
    if((at->SCTPprotocolId == sctp_proto) &&
       (at->SCTPstreamId == stream_no) &&
       (at->msgNotify == mf) &&
       (at->arg == arg)){
      nat = (msgConsumer *)dlist_getThis(dl);
      if(nat != at){
	/* help */
	abort();
      }
      break;
    }
  }
  return(nat);
}


int
dist_msgSubscribe(distributor *o,messageFunc mf,int sctp_proto, int stream_no,int priority, void *arg)
{
  int ret;
  msgConsumer *mc;
  hashableHash *hh;
  HashedTbl *hofl;
  hashableDlist *hdl;
  /* Ok, first we look up the hashTable with the sctp_proto number 
   * If we don't find it we add a new hash table for this to
   * the master hash table. Once we have the master
   * hash table and our sub-hash table, we then lookup
   * the stream_no to find the hashDlist structure.
   * With that we add this guy to the list (after allocating a 
   * entry to pop on and fill it in). As a short cut we look to
   * see if the two are the default values, if so dump it right
   * in to our pre-stored list.
   */
  
  /* first get a msgConsumer */
  mc = (msgConsumer *)calloc(1,sizeof(msgConsumer));
  if(mc == NULL){
    /* no memory, out of here */
    return(LIB_STATUS_BAD);
  }
  mc->precedence = priority;
  mc->SCTPprotocolId = sctp_proto;
  mc->SCTPstreamId = stream_no;
  mc->msgNotify = mf;
  mc->arg = arg;

  /* now that we have our structure see if we take the
   * short cut.
   */
  if((sctp_proto == DIST_SCTP_PROTO_ID_DEFAULT) &&
     (stream_no == DIST_STREAM_DEFAULT)){
    ret = __place_mc_in_dlist(o->msgSubZeroNoStrm->list,mc);
    if(ret != LIB_STATUS_GOOD){
      free(mc);
    }
    return(ret);
  }
  /* ok if we reach here, we must put it on some
   * other list in all of our hash tables. So much
   * for the short cut :)
   */
  /* first find the right hashableHash.
   */
  hh = HashedTbl_lookupKeyed(o->msgSubscribers,
			     sctp_proto,
			     &sctp_proto,
			     sizeof(int),
			     (void **)NULL);
  if(hh == NULL){
    /* no hashable hash table exists for the protocol type */
    /* we must create it */
    char string[32];
    sprintf(string,"protocolId:0x%x",sctp_proto);
    hh = create_hashableHash(sctp_proto,string,11);
    if(hh == NULL){
      /* failed to alloc-create */
      free(mc);
      return(LIB_STATUS_BAD);
    }
    /* now we must enter it in the master table */
    ret = HashedTbl_enterKeyed(o->msgSubscribers,hh->protocolId,(void *)hh,&hh->protocolId,sizeof(int));
    if(ret != LIB_STATUS_GOOD){
      /* something bad is happening */
      free(mc);
      destroy_hashableHash(hh);
      return(LIB_STATUS_BAD);
    }
    /* ok, we now have the hasable hash in the table and hofl setup right */
    hofl = hh->hash;
  }else{
    /* we found one */
    hofl = hh->hash;
  }
  /* now lets find the low level hashDlist */
  hdl = (hashableDlist *)HashedTbl_lookupKeyed(hofl,
					       stream_no,
					       &stream_no,
					       sizeof(int),
					       (void **)NULL);
  if(hdl == NULL){
    /* it does not exist so we must
     * build one and dump it in the hofl table.
     */
    hdl = create_hashdlist(stream_no);
    if(hdl == NULL){
      /* if we are in trouble, we don't kill the
       * previous hash table we built since
       * it may be used again when the memory problem
       * is cleared.. of course we may not even
       * have created it too.
       */
      free(mc);
      return(LIB_STATUS_BAD);
    }
    /* now add it to our hash table */
    ret = HashedTbl_enterKeyed(hofl,hdl->streamNo,(void *)hdl,&hdl->streamNo,sizeof(int));
    if(ret != LIB_STATUS_GOOD){
      /* no way to add the dlist to the table.. we are out of here. */
      free(mc);
      destroy_hashDlist(hdl);
      return(LIB_STATUS_BAD);
    }
    /* ok its in the table and now hdl points to the
     * guy we need to add to.
     */
  }
  /* now lets place it on the global list */
  ret = __place_mc_in_dlist(o->msgSubscriberList,mc);
  if(ret != LIB_STATUS_GOOD){
    free(mc);
    return(ret);
  }
  /* now on the individual list */
  ret = __place_mc_in_dlist(hdl->list,mc);
  if(ret != LIB_STATUS_GOOD){
    __pull_from_dlist(o->msgSubscriberList,mc);
    free(mc);
  }
  return(ret);
}

int
dist_msgUnsubscribe(distributor *o,messageFunc mf,int sctp_proto, int stream_no,void *arg)
{
  msgConsumer *mc,*mc2;
  hashableHash *hh;
  HashedTbl *hofl;
  hashableDlist *hdl;

  /* first lets pull it from the master table of whats in the dlist*/
  mc2 = mc = __pull_from_dlist2(o->msgSubscriberList,mf,sctp_proto,stream_no,arg);
  hh = HashedTbl_lookupKeyed(o->msgSubscribers,
			     sctp_proto,
			     &sctp_proto,
			     sizeof(int),
			     (void **)NULL);
  if(hh != NULL){
    hofl = hh->hash;
    hdl = (hashableDlist *)HashedTbl_lookupKeyed(hofl,
						 stream_no,
						 &stream_no,
						 sizeof(int),
						 (void **)NULL);
    if(hdl != NULL){
      /* ok lets find the entry here */
      mc2 = __pull_from_dlist2(hdl->list,mf,sctp_proto,stream_no,arg);
    }
  }
  if(mc && mc2){
    if(mc != mc2){
      /* help! */
      abort();
    }
    /* normal case */
    free(mc);
  }else if(mc){
    free(mc);
  }else if(mc2){
    /* strange should have been in other list */
    free(mc2);
  }else{
    return(LIB_STATUS_BAD);
  }
  return(LIB_STATUS_GOOD);
}

int
dist_setDone(distributor *o)
{
  o->notdone = 0;
  return(LIB_STATUS_GOOD);
}

void
__check_time_out_list(distributor *o)
{
  /* some timeout has occured ? */
  timerEntry *te;
  dlist_dlink *dl,*tmpl;
  tmpl = NULL;
  /* first get the time */
  gettimeofday(&o->lastKnownTime,(struct timezone *)NULL);

  /* get each one and have a look */
  dlist_reset(o->timerlist);
  while((te = (timerEntry *)dlist_get(o->timerlist)) != NULL){
    if((te->expireTime.tv_sec < o->lastKnownTime.tv_sec) ||
       ((te->expireTime.tv_sec == o->lastKnownTime.tv_sec) &&
	(te->expireTime.tv_usec < o->lastKnownTime.tv_usec))
       ){
      /* ok this one has expired */
      dl = dlist_getThisSlist(o->timerlist);
      if(dl != NULL){
	dl->next = tmpl;
	tmpl = dl;
      }
    }else{
      /* ok the time is beyond this one I am looking at */
      /* we should have no more after this */
      break;
    }
  }
  dl = tmpl;
  /* call every timer that is up on our temp list */
  while(dl != NULL){
    tmpl = dl->next;
    te = (timerEntry *)dl->ent;
    free(dl);	
    (*te->action)(te->arg1,te->arg2);
    free(te);
    dl = tmpl;
  }
}

void
__sendClockTick(distributor *o)
{
  /* distribute a clock tick to all subscribers for ticks */
  auditTick *at;
  dlist_reset(o->lazyClockTickList);
  while((at = (auditTick *)dlist_get(o->lazyClockTickList)) != NULL){
    (*at->tick)(at->arg);
  }
}

void
__sendStartStopEvent(distributor *o,int event)
{
  /* distribute a clock tick to all subscribers for ticks */
  StartStop *at;
  dlist_reset(o->startStopList);
  while((at = (StartStop *)dlist_get(o->startStopList)) != NULL){
    if(at->flags & event)
      (*at->activate)(at->arg,event);
  }
}


int
dist_startupChange(distributor *o,startStopFunc sf, void *arg, int howmask)
{
  int chnged;
  StartStop *at;
  chnged = 0;
  dlist_reset(o->startStopList);
  while((at = (StartStop *)dlist_get(o->startStopList)) != NULL){
    if((sf == at->activate) && (arg == at->arg)){
      chnged = 1;
      at->flags = howmask;
      if(howmask == DIST_DONT_CALL_ME){
	/* pull it from the list too */
	void *v;
	v = dlist_getThis(o->startStopList);
	if(v != at){
	  printf("Serious error, TSNH!\n");
	  return(LIB_STATUS_BAD3); 
	}else{
	  free(at);
	}
      }
    }
  }
  if(!chnged){
    /* new entry */
    int ret;
    at = calloc(1,sizeof(StartStop));
    if(at == NULL){
      /* no memory */
      return(LIB_STATUS_BAD); 
    }
    at->activate = sf;
    at->flags = howmask;
    at->arg = arg;
    ret = dlist_append(o->startStopList,(void *)at);
    if(ret != LIB_STATUS_GOOD){
      free(at);
      return(LIB_STATUS_BAD);
    }
  }
  return(LIB_STATUS_GOOD);
}


int
dist_process(distributor *o)
{
  int i,ret,errOut;
  int clockTickFlag;
  int doingMultipleReads;
  timerEntry *te;
  fdEntry *fe;
  dlist_t *list1;

  int timeout;
#ifdef USE_SELECT
  struct timeval timeoutt;
  int cnt;
  fd_set readfds,writefds,exceptfds;
#endif

  list1 = dlist_create();
  if(list1 == NULL){
    printf("Error no memory\n");
    return(LIB_STATUS_BAD);
  }
  i = 0;
  doingMultipleReads = 0;
  __sendStartStopEvent(o,DIST_CALL_ME_ON_START);
  while(o->notdone){
    dlist_reset(o->timerlist);
    te = dlist_get(o->timerlist);
#ifdef USE_SELECT
    /* spin through and setup things.*/
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    cnt = 0;
    for(i=0;i<o->numfds;i++){
      if(o->fdlist[i].events & POLLIN){
	if(o->fdlist[i].fd > cnt){
	  cnt = o->fdlist[i].fd;
	}
	FD_SET(o->fdlist[i].fd,&readfds);
      }
      if(o->fdlist[i].events & POLLOUT){
	if(o->fdlist[i].fd > cnt){
	  cnt = o->fdlist[i].fd;
	}
	FD_SET(o->fdlist[i].fd,&writefds);	
      }
      cnt++;
    }
#endif
    /* prepare timeout */
#ifndef MINIMIZE_TIME_CALLS
    gettimeofday(&o->lastKnownTime,(struct timezone *)NULL);
#endif
    if(te != NULL){
      /* yep, a timer is running use the delta */
      clockTickFlag = 0;
      if(te->expireTime.tv_sec >= o->lastKnownTime.tv_sec){
	timeout = ((te->expireTime.tv_sec - o->lastKnownTime.tv_sec) * 1000);
	if(te->expireTime.tv_usec > o->lastKnownTime.tv_usec){
	  timeout += ((te->expireTime.tv_usec - o->lastKnownTime.tv_usec)/1000);
	}else if(te->expireTime.tv_usec < o->lastKnownTime.tv_usec){
	  /* borrow 1 second from the upper seconds 
	   */
	  timeout += (((1000000 - o->lastKnownTime.tv_usec) + te->expireTime.tv_usec)/1000);
	  /* subtract out the borrowed time above */
	  timeout -= 1000;
	}
	/* subtract off 10ms this is due to
	 * the clock synchronization issues. What
	 * happens is the O/S will round up leaving us a
	 * drift upwards. This compensates for that drift,
	 * we only do this if we have more than 11ms to wait.
	 */
	if(timeout > 11){
	  timeout -= 10;
	}
      }else{
	/* expire time is past? */
	timeout = 0;
      }
      if(timeout <= 0){
	/* problem, time out has occured */
	__check_time_out_list(o);
	continue;
      }
    }else{
      /* no timeout setup default */
      clockTickFlag = 1;
      timeout = (o->idleClockTick.tv_sec * 1000) + o->idleClockTick.tv_usec/1000;
    }
#ifdef USE_SELECT
    if(doingMultipleReads){
      /* poll for fd only */
      timeoutt.tv_sec = 0;
      timeoutt.tv_usec = 0;
    }else{
      timeoutt.tv_sec = (timeout/1000);
      timeoutt.tv_usec = (timeout%1000) * 1000;
    }
    ret = select(cnt,&readfds,&writefds,&exceptfds,&timeoutt);
#else
    if(doingMultipleReads){
      /* non-blocking poll */
      ret = poll(o->fdlist,o->numused,0);
    }else{
      ret = poll(o->fdlist,o->numused,timeout);
    }
#endif
    /* save off the error from poll/select */
    errOut = errno;
    /* we always check the timeout updating the time in the process */
    __check_time_out_list(o);
    if(ret > 0){
      /* ok we have something to do */
#ifdef USE_SELECT
      /* translate the select terminology into the poll fundamentals */
      for(i=0;i<o->numfds;i++){
	if(o->fdlist[i].fd == -1)
	  continue;
	o->fdlist[i].revents = 0;
	if(FD_ISSET(o->fdlist[i].fd,&readfds)){
	  o->fdlist[i].revents |= POLLIN;
	}
	if(FD_ISSET(i,&writefds)){
	  o->fdlist[i].revents |= POLLOUT;
	}
      }
#endif
      /* now rip through and call all the appropriate
       * functions
       */
      for(i=0;i<o->numfds;i++){
	if(o->fdlist[i].fd == -1){
	  continue;
	}
	if(o->fdlist[i].revents == 0){
	  continue;
	}
	/* got a live one */
	fe = (fdEntry *)HashedTbl_lookupKeyed(o->fdSubscribers,
					      o->fdlist[i].fd,
					      &o->fdlist[i].fd,
					      sizeof(int),
					      (void **)NULL);
	if(fe == NULL){
	  /* huh? TSNH */
	  o->fdlist[i].fd = -1;
	  o->numused--;
	  if(i != o->numused){
	    o->fdlist[i].fd = o->fdlist[o->numused].fd;
	    o->fdlist[i].events = o->fdlist[o->numused].events;
	    o->fdlist[i].revents = o->fdlist[o->numused].revents;
	    o->fdlist[o->numused].fd = -1;
	    o->fdlist[o->numused].events = 0;
	    o->fdlist[o->numused].revents = 0;
	  }
	  continue;
	}
	if((*fe->fdNotify)(fe->arg,o->fdlist[i].fd,o->fdlist[i].revents) > 0){
	  if(fe->onTheList == 0){
	    if(dlist_append(list1,(void *)fe) < LIB_STATUS_GOOD){
	      printf("Can't append??\n");
	    }else{
	      fe->onTheList = 1;
	    }
	  }
	}
	o->fdlist[i].revents = 0;
      }
    }else if((ret == 0) && (clockTickFlag) && (!doingMultipleReads)){
      __sendClockTick(o);
    }else if((ret == 0) && (doingMultipleReads)){
      dlist_reset(list1);
      fe = (fdEntry *)dlist_get(list1);
      if((*fe->fdNotify)(fe->arg,o->fdlist[i].fd,0) <= 0){
	void *v;
	fe->onTheList = 0;
	v = dlist_getThis(list1);
	if(v != (void *)fe){
	  printf("Help TSNH!\n");
	}
      }
    }
    if(dlist_getCnt(list1)){
      /* if we have some on the list then we need
       * to arrange to just poll non-blocking
       */
      doingMultipleReads = 1;
    }else{
      doingMultipleReads = 0;
    }
  }
  __sendStartStopEvent(o,DIST_CALL_ME_ON_STOP);
  dlist_destroy(list1);
  return(LIB_STATUS_GOOD);
}

void
__distribute_msg_o_list(dlist_t *dl,messageEnvolope *msg)
{
  /* distribute over a list the particular message *msg */
  msgConsumer *at;
  dlist_reset(dl);
  while((at = (msgConsumer *)dlist_get(dl)) != NULL){
    (*at->msgNotify)(at->arg,msg);
    if((msg->data == NULL) || (msg->totData == NULL)){
      break;
    }
  }
}


void
dist_sendmessage(distributor *o,messageEnvolope *msg)
{
  hashableHash *hh;
  HashedTbl *hofl;
  hashableDlist *hdl;

  /* first lets lookup in the master table for the protocol */
  msg->distrib = (void *)o;
  hdl = NULL;
  hh = HashedTbl_lookupKeyed(o->msgSubscribers,
			     msg->protocolId,
			     &msg->protocolId,
			     sizeof(int),
			     (void **)NULL);
  if(hh != NULL){
    /* ok do we have a protocolID specific entry? - yes */
    hofl = hh->hash;
    /* is the stream there? */
    hdl = (hashableDlist *)HashedTbl_lookupKeyed(hofl,
						 msg->streamNo,
						 &msg->streamNo,
						 sizeof(int),
						 (void **)NULL);
    if(hdl == NULL){
      /* no, see if the default stream is there */
      int x;
      x = DIST_STREAM_DEFAULT;
      hdl = (hashableDlist *)HashedTbl_lookupKeyed(hofl,
						   x,
						   &x,
						   sizeof(int),
						   (void **)NULL);
    }
  }
  if(hdl != NULL){
    /* ok lets send it over this list */
    __distribute_msg_o_list(hdl->list,msg);
  }
  /* if we did not consume and we were on some other list, pass it over the 
   * default list.
   */
  if((msg->data != NULL) && (msg->totData != NULL) && (hdl != o->msgSubZeroNoStrm)){
    __distribute_msg_o_list(o->msgSubZeroNoStrm->list,msg);
  }
  if((msg->totData != NULL) && (msg->data != NULL) && (msg->takeOk)){
    free(msg->totData);
  }
}



int
dist_wants_audit_tick(distributor *o,auditFunc af,void *arg)
{
  auditTick *at;
  at = calloc(1,sizeof(auditTick));
  if(at == NULL){
    return(LIB_STATUS_BAD);
  }
  at->arg = arg;
  at->tick = af;
  return(dlist_append(o->lazyClockTickList,(void *)at));
}

int
dist_no_audit_tick(distributor *o,auditFunc af,void *arg)
{
  /* find and remove this guy */
  auditTick *at;
  dlist_reset(o->lazyClockTickList);
  while((at = (auditTick *)dlist_get(o->lazyClockTickList)) != NULL){
    if((at->arg == arg) && (at->tick == af)){
      /* got it */
      dlist_getThis(o->lazyClockTickList);
      free(at);
      return(LIB_STATUS_GOOD);
    }
  }
  return(LIB_STATUS_BAD);
}
void
dist_changelazyclock(distributor *o,struct timeval *tm)
{
  o->idleClockTick.tv_sec = tm->tv_sec;
  o->idleClockTick.tv_usec = tm->tv_usec;
}
