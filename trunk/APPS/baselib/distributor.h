/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

$Header: /usr/sctpCVS/APPS/baselib/distributor.h,v 1.6 2009-01-15 14:35:15 randall Exp $
*/
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
/*
The SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@cisco.com
kmorneau@cisco.com

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"

*/

#ifndef __distributor_h__
#define __distributor_h__
#include <sys/types.h>
#include <stdio.h>
#include <poll.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memcheck.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <return_status.h>
#include <dlist.h>
#include <hlist.h>
#include <HashedTbl.h>
#include <sys/queue.h>
/*
 * This is the main distributor class. You ususally
 * build one of these in your main, build other
 * objects/registration calls and then call the 
 * process() member which runs a poll loop.
 * Events call you, these events can be:
 *
 * - Message, a message sent in to the stack by
 *            some other function (a lot of times
 *            a fd event for example).
 *
 * - File descriptor (fd) - Usually a service
 *            utility is registered against all
 *            open fd's and then the function turns
 *            the poll wakeups into messages sent
 *            to the distributor.
 *
 * - Alarm - the distributor has a built in
 *           timer mechanism. This mechanism
 *           will allow you to start/stop timers
 *           it does not use signals at all.
 *
 * - audit tick - A lazy audit tick is provided
 *           that runs when you are idle.
 *
 * - start  - the actual start of the event loop.
 *
 * - stop   - the actual stopping of the event loop.
 *
 * This file contains numerous defintions that 
 * allow you to register and change these things
 * as well as some basic def's (like the process() fnc).
 *
 */

/* The message envolope structure is what is
 * sent around with every message.
 */


typedef struct messageEnvolope{
  /* the Following are SCTP specific
   * entries. You don't get them for
   * any other of the protocols, unless
   * per-chance the TCP side might use the
   * multipe streams to identify multiple tcp
   * connections.
   */
  /* Question: Do we need an association id besides
   * the from address?
   */
  uint32_t TSN;
  uint32_t protocolId;
  sctp_assoc_t assoc_id;
  uint32_t streamNo;
  uint32_t streamSeq;
  /* The total fields are used to carry
   * the base pointer and size for
   * the free() operation, if you snarf
   * this data.
   */
  int totSize;
  void *totData;
  /* Here is the actual data that
   * you look at, it may or maynot be
   * set to the same as totData and totSize.
   */
  int siz;
  void *data;
  /* This is who sent you the message, it
   * can be changed and if this happens in
   * your stack the origFrom will be set
   * to the actual transport that brought
   * the message in.
   */
  void *from;
  uint32_t from_len;
  void *to;
  uint32_t to_len;
  int type;
  /* this is the base transport that 
   * brought the PDU in. In the
   * beginning both from and origFrom
   * are the same. Later the 'from' may
   * change as different functions
   * process the inbound PDU. I.e asap.
   */
  void *origFrom;
  int origType;
  int takeOk;	/* if true, the message (totData) can be stolen i.e it's
		 * alloc'd with a calloc/malloc, and if no one takes it
		 * it will be free()'d at the end.
		 */
  void *distrib;	/* distributor sending msg */
  void *sender;		/* object sending msg i.e. SCTP etc */
}messageEnvolope;

/* Used to map an SCTP event to a distributor message. See SCTPnotify().
 */
#define SCTP_MAX_NOTIFY_SIZE 1024
typedef struct eventNotify{
  int eventType; /* association event, one of SCTP_NOTIFY_* */
  char data[SCTP_MAX_NOTIFY_SIZE];
}eventNotify;


/* When your function call gets called with a message
 * you can do three things:
 * a) ignore it, but just returning
 * b) process it and stop others from seeing it by setting  data = NULL.
 * c) process it and let others see it by NOT setting data = NULL
 *
 * if you do (c) you can actually build a protocol stack by
 * having the incoming data be changed and manipulated, and
 * possibly sending something back out of your function call.
 *
 * When you do process a message you may have the option
 * to "steal" the messsage. You check the takeOk flag, if this
 * is true the sender malloc'd the message and you can
 * take over control of the data. This makes it so you
 * don't have to copy the data to your own buffer. The drawback
 * to this is that the data pointer may NOT be the same thing
 * as where the message was allocated... other functions may
 * have changed the message using (c) above. So you must always
 * only use/change and work with the data pointer. If you take
 * the data you keep a copy of the totData pointer and when
 * you are ready to free the memory it is this pointer you
 * pass to the free call. Doing otherwise will cause a memory
 * leak. To "steal" the data, you set the totData to NULL, after
 * copying to your own private are of course. This tells the
 * distributor that the message has been consumed, just like
 * it would if you set data = NULL yet it also tells the
 * distributor NOT to free the data. The normal behavior
 * of the stack is to free(totData) after all interested
 * parties have looked at it.
 *
 * Now you subscribe to a mention (with members below) and are
 * allowed to specify SCTP stream and SCTP protocol ID. There
 * is also a "generic" bucket (DIST_SCTP_PROTO_ID_DEFAULT and
 * DIST_STEAM_DEFAULT) that allows you to get the un subscribed
 * for things. The basic algorithm goes something like:
 *
 * Look up the protocol and stream id is there a list
 * of fcn's subscribed for this. If so pass this through
 * the linked list of subscribers until someone indicates that the
 * data is consumed (or steals it). After which terminate processing.
 * However, if no one takes it, go get the default stream and
 * protocol and let that list look at it.
 *
 * As mentioned there is a linked list of subscribers. When you
 * register you specify an int priority. If every one uses the same
 * priority (say 10) then the first one registered gets the
 * message first, second one second and so on. However if the
 * value of say the last one registering is 9, it would be
 * inserted ahead of all of the other registered entries of
 * 10 and above. This priority field allows you to control
 * the order of registration a bit better.
 *
 * Well thats enough about messages, I will discuss more
 * later below in the actual functions.
 */
#define DIST_STREAM_DEFAULT        0x10000


/* These are the signatures of the functions that
 * you must provide. Each type has a purpose
 *
 *
 * This first one is for the lazy audit. You, in
 * subscribing pass a void (the object) and 
 * thus get called with this value.
 */
typedef void(*auditFunc)(void *);

/* This is the start/stop function sig, the void
 * is again your registered arg, the int is the
 * flag telling you if it is a start or a stop
 */
typedef void(*startStopFunc)(void *,int);
/*
 * This is your timer signature, it passes you
 * two voids if the alarm goes off.
 */
typedef void(*timerFunc)(void *,void *, int);
/*
 * Here is our message passing friend. You see
 * the message envolope and a void * arg which
 * can be though of as the object.
 *
 */
typedef void(*messageFunc)(void *,messageEnvolope *);
/*
 * The service function is used by file descriptor
 * service functions. The void * is the argument
 * the second argument is the file descriptor that
 * woke up and the third is the actual POLL flags
 * returned on the revents filed.
 */
typedef int(*serviceFunc)(void *,int,int);


/* Her are flags passed to the start/stop
 * registration routine. This is also passed
 * to you to tell you if you got a start or
 * a stop. In calling your function you will
 * only get either DIST_CALL_ME_ON_START or
 * DIST_CALL_ME_ON_STOP.
 */
#define DIST_DONT_CALL_ME		0x0000
#define DIST_CALL_ME_ON_START		0x0001
#define DIST_CALL_ME_ON_STOP		0x0002
#define DIST_CALL_ME_ON_STARTSTOP	0x0003

/* the following are the guts of
 * stuff that the distributor is
 * keeping
 */

typedef struct auditTick{
  void *arg;
  auditFunc tick;
}auditTick;

typedef struct StartStop{
  int flags;
  void *arg;
  startStopFunc activate;
}StartStop;

typedef struct hashableTimeEnt {
  void *arg1;
  void *arg2;
  timerFunc action;
  int timer_issued_cnt;
}hashableTimeEnt;

typedef struct timerEntry{
  LIST_ENTRY(timerEntry) next;
  struct timeval started;
  struct timeval expireTime;
  hashableTimeEnt ent;
  u_long tv_sec;
  u_long tv_usec;
}timerEntry;

#define TIMERS_NUMBER_OF_ENT 3600 /* 1 for every second in an hour */
LIST_HEAD(timerentries, timerEntry);

typedef struct fdEntry{
  int onTheList;
  int fdItServices;
  void *arg;
  serviceFunc fdNotify;
}fdEntry;

typedef struct hashableDlist{
  uint32_t streamNo;
  dlist_t *list;
}hashableDlist;

typedef struct hashableHash{
  int protocolId;
  HashedTbl *hash;
}hashableHash;


typedef struct distributor{
  /* The global list of subscribers for fd's  being tracked */
  dlist_t *fdSubscribersList;
  /* The Hash Table of fd-> subscriber */
  HashedTbl *fdSubscribers;
  /* The global list of subscribers for msg's */
  dlist_t *msgSubscriberList;
  /* The Global table of SCTP-PID -> Stream HashTbl */
  HashedTbl *msgSubscribers;
  /* The Global table of dlist_t for each stream subscribed
   * for with NO SCTP-PID specified
   */
  hashableHash *msgSubZero;
  /* Here is where the subscriber would fall if
   * it did not care about either stream or SCTP
   * protocol id. In a TCP communication, all of
   * them would end up here.
   */
  hashableDlist *msgSubZeroNoStrm;

  /* list of entries that want a audit tick */
  dlist_t *lazyClockTickList;
  dlist_t *startStopList;
  hlist_t *tmp_timer_list;
  
  int numfds;
  int numused;
  int timer_issued_cnt;
  struct pollfd *fdlist;
  struct timerentries  timers[TIMERS_NUMBER_OF_ENT];
  timerEntry *soonest_timer;
  HashedTbl *timerhash;
  int lasttimeoutchk;
  int timer_cnt;
  int notdone;
  struct timeval lastKnownTime;
  struct timeval idleClockTick;
  uint8_t msgs_round_robin;
  uint8_t no_dup_timer;
  uint8_t resv[2];
}distributor;

/*************************/
/* protocolID protocol's */
/*************************/
/* DIST_SCTP_PROTO_ID_DEFAULT is used to
 * specify the "unspecified" protocol. This will
 * vector to its own hash table of streams. If
 * a PID is not found or 0 it uses this table.
 */
#define DIST_SCTP_PROTO_ID_DEFAULT  0

/* Some registration protocols we will use
 * to segregate messages in the sinfo_ppid field.
 */
#define CONNECT_EVENT_PROTOCOL  0xf0000001
#define DISTRIBUTE_TCP_PROTOCOL 0xf0000002
#define DISTRIBUTE_SCTP_NOTIFY  0xf0000003

/*******************************************/
/* Special registration stream for default */
/*******************************************/
/* Use DIST_STREAM_DEFAULT if you wish to get
 * any stream after it has gone through the
 * lookup of the individual stream and passing
 * through that message list (if any).
 */
#define DIST_STREAM_DEFAULT        0x10000

/***********************/
/* origType protocol's */
/***********************/
/* protocol types found in type and origType 
 * in the message envolope, more will be added
 * as thing progress (possibly).
 */
#define PROTOCOL_Unknown     0x0
#define PROTOCOL_Sctp        0x1
#define PROTOCOL_Tcp         0x2
#define PROTOCOL_Udp         0x3
#define PROTOCOL_Asap        0x4
#define PROTOCOL_Enrp        0x5
#define PROTOCOL_MultiCast   0x6
#define PROTOCOL_Sctp_Notify 0x7




/* According to http://www.isi.edu/in-notes/iana/assignments/protocol-numbers
 * 61 is "any host internal protocol" so we should never see an IP packet with
 * this protocol ID.
 */
#define PROTOCOL_SCTP_EVENT 61

typedef struct msgConsumer{
  /* 0= don't care, 1= I want it first, 2= I want it 
   * second... etc. if tied first one wins.
   */
  int precedence;
  /* if 0, no registration */
  u_int SCTPprotocolId;
  /* if negative, no registration */
  int SCTPstreamId;
  /*  void (*messageFunc)(void *,messageEnvolope *);*/
  messageFunc msgNotify;
  /* argument for passing */
  void *arg;
}msgConsumer;

/* this is the default size of the poll list
 * if you are planning more than this in the
 * fd side of things you may want to grow
 * this. The list will grow by itself, but
 * if you pre-know you need more space you can save the memory
 * copies by raising this value.
 */
#define DEFAULT_POLL_LIST_SZ 10

/* Here is the default idle period */
#define DIST_IDLE_CLOCK_SEC  60
#define DIST_IDLE_CLOCK_USEC 0


#define DIST_TIMER_START_CNT 101
/* 
 * This is the creation function. You normally
 * call this in main and get a pointer back as
 * one of the first things you do.
 */
distributor *
createDistributor();

/* 
 * Here we have how you destroy the thing you
 * created. All sorts of stuff is done down
 * in here to clean up all the structures
 * so it is a good idea to call this
 * at the end of your process when main
 * is about to end.
 */
void
destroyDistributor(distributor *o);

/* 
 * If you want to start a timer here is the function for
 * you 'f' points to your function to be called if
 * the timer expires. You always (on all of these) include
 * a pointer to the distributor itself (the object :>). The
 * t_sec and t_usec are the seconds and microseconds to
 * expire the timer in. And the arg1 and arg2 are the
 * two arguments to pass to the function 'f'
 */
int
dist_TimerStart(distributor *o,timerFunc f, u_long t_sec,
		u_long t_usec, void *arg1, void *arg2);

/* 
 * This function will STOP a timer you started with
 * dist_TimerStart(). The way a timer is identified to
 * stop is by function 'f' and the two args that were
 * passed arg1 and arg2. If you start multiple timers
 * (with different times) and the same 'f' and arg1 and arg2
 * a call to this will stop one of them, not all.
 */
int
dist_TimerStop(distributor *o,timerFunc f, void *arg1, void *arg2, int ent);

/*
 * This is how you get a file descriptor added to the
 * poll loop. You tell the fd itself, and the service function.
 * mask is the poll() flags that you want POLLIN etc. arg is
 * the argument that will be passed to you fdNotify function.
 */

int
dist_addFd(distributor *o,int fd,serviceFunc fdNotify,int mask,void *arg);

/* 
 * This is how you STOP watching a file descriptor. Note that only
 * the fd in needed to pull something from the list.
 */
int
dist_deleteFd(distributor *o,int fd);

/* This is how you dynamically change the
 * poll mask... say add POLLOUT etc.
 */
int
dist_changeFd(distributor *o,int fd,int newmask);

/*
 * Normal behavior is that messages always
 * start distribution at the head of the list.
 * if you set round robin, then the list
 * of message consumers continues where it
 * left off and wraps. This breaks all the
 * priority stuff and makes all consumers
 * round robin within the two sets of
 * consumers note that there are two sets
 * one for the stream/protocol and then
 * the generic. Call the set to turn
 * on the behavior, call the clr to turn
 * it off.
 */
#define dist_setRoundRobinMsgs(o) do {	\
  o->msgs_round_robin = 1;			\
}while(0)
  
#define dist_clrRoundRobinMsgs(o) do {	\
  o->msgs_round_robin = 0;			\
}while(0)

/* Normal behavior of the distributor
 * is to allow NO duplicate timers to be
 * entered in the list.  If you want to
 * have duplicates then you must record
 * the postive return value from the timer
 * start (1-7fffffff) and use that with the
 * stop. If you do the default, no duplicates,
 * the id will not be used for stopping the
 * timer.
 */
int dist_set_No_Dup_timer(distributor *o);
int dist_clr_No_Dup_timer(distributor *o);


/* Oh, heres some older comment sof mine. I will leave
 * them in as well even if they may be
 * duplicated :) It has a bit more detail on
 * all the stuff underneath ...
 */

/*
 * The Message subscription is based on a two level
 * hash table. First we hash the inbound SCTP protocol
 * identification. This yeilds a subsequent Hash table.
 * This Hash table is hashed on the stream number. This
 * will boil down to a list of subscribers (dlist_t). Now
 * If there is no PID specified or not available (such as
 * a TCP connection) then the "Zero" table is used. If 
 * there is no stream (as in TCP) or the lookup of the
 * stream fails, we use the msgSubZeroNoStrm list. 
 * Also if a PID list is found and a individual stream
 * list, after processing, if no data takers are in the
 * dlist_t, then we drop back to the PID's default - no
 * stream dlist_t followed by the, no PID stream list, followed
 * by the msgSubZeroNoStrm list (assuming no one collects
 * the message). Note a function may register itself for
 * more than one type of filtering i.e. stream  1 and stream 2 of
 * PID 5 AND the generic ones as well. It must be prepared to see
 * the same data twice if it registers in a generic bin. Note
 * that at any time we will only capture 2 lists to look in, so the
 * most any func will be called is twice (even if on the 4 possible
 * lists).
 */


/* Here is how you subscribe to get the 
 * lazy audit tick. The function 'af' will
 * be called with the arg any time the
 * poll() falls out of the loop without a
 * timeout happening... i.e. no events went on
 * at all for a specified period.
 */
int
dist_wants_audit_tick(distributor *o,auditFunc af,void *arg);

/* Here is how you un-subscribe to that same
 * audit tick.
 */
int
dist_no_audit_tick(distributor *o,auditFunc af,void *arg);

/* Here is how you register for messages, as described in
 * the beginning, the protocol and stream no help dictate
 * which things you want. The priority will help guide
 * where in the linked list you land, lower numbers should
 * be the objects more likely to get messages. The arg
 * is passed with the message to the 'mf' function.
 */

int
dist_msgSubscribe(distributor *o,messageFunc mf,int sctp_proto, int stream_no,int priority,void *arg);

/* Here is how to stop getting messages. Remember it is
 * possible to register more than once and then see
 * a message more than once...
 */
int
dist_msgUnsubscribe(distributor *o,messageFunc mf,int sctp_proto, int stream_no,void *arg);

/* This function will start the gracefull termination of
 * the event loop. If your program is ready to terminate
 * this should be called. It will help things wrap up
 * gracefully.
 */
int
dist_setDone(distributor *o);

/* Here is the function you call to start the event loop 
 * it will not return until some function does a dist_setDone on
 * this object. Remember there can be more than one
 * distributor in multiple threads.
 */
int
dist_process(distributor *o);

/* Here is how someone sends a message to the stack.
 * this is usually done by a fd service routine but
 * it could be done by an alarm or lazy clock tick
 * too (beware if you are multi-threaded though, it
 * is not safe for any but the thread running the
 * distributor to call).
 */
void
dist_sendmessage(distributor *o,messageEnvolope *);

/* This can adjust the value of the poll() timeout. By
 * so doing you influence how long you have to be
 * idle to get a audit tick.
 */
void
dist_changelazyclock(distributor *o,struct timeval *tm);

/* this member is the universal startup/shutdown registration
 * function. You provide your function and an argument (arg). The
 * how mask gets filled with what action you want to have
 * registered (or not registered) for. 'sf' only will be
 * called right before poll()ing begins AND right before
 * the return of dist_process() after dist_setDone has been
 * called.
 */
int
dist_startupChange(distributor *o,startStopFunc sf, void *arg, int howmask);

#endif














