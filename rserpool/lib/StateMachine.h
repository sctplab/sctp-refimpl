/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/StateMachine.h,v 1.1 2006-05-30 18:02:01 randall Exp $


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

#ifndef __StateMachine_h__
#define __StateMachine_h__
#include <sys/types.h>
#include <stdio.h>
#include <return_status.h>
#include <distributor.h>

#define STATE_MACH_ENTER	0x0001
#define STATE_MACH_EXIT		0x0002
#define STATE_MACH_MESSAGE	0x0003
#define STATE_MACH_TIMER	0x0004
#define STATE_MACH_CALLBACK	0x0005
#define STATE_MACH_AUDITTICK	0x0006
#define STATE_MACH_FD		0x0007
#define STATE_MACH_START	0x0008
#define STATE_MACH_STOP		0x0009

typedef void(*stateFunction)(void *objarg,int event,int arg1,void *arg2);

typedef struct stateMachineS{
  distributor *obj;
  stateFunction currentState;
  void *objarg;
  int timerCnt;
  int lastFree;
  int *timerArray;
  int noStart;
}stateMachineS;

/* creation/destruction function */
stateMachineS *
createStateMachine(distributor *o,void *objarg,int numTimers,
		   stateFunction firstState, int noStartup);

void
destroyStateMachine(stateMachineS *obj);

/* Accesor to change state, could be used by the outside as well */
int
changeState(stateMachineS *obj,stateFunction newstate,int arg1,void *arg2);

/* Subscription things that the initial state should setup */

int
stateMachineAddFd(stateMachineS *obj,int fd, int events);

int
stateMachineDelFd(stateMachineS *obj,int fd);

int
stateMachineWantsMsg(stateMachineS *obj,int sctp_proto, int stream_no,int priority);

int
stateMachineNoMsg(stateMachineS *obj,int sctp_proto, int stream_no,int priority);

int
stateMachineWantsAudit(stateMachineS *obj);

int
stateMachineDisableAudit(stateMachineS *obj);


/* Timer control for state machine */
int *
startStateTimer(stateMachineS *obj,int sec, int usec);

int
stopStateTimer(stateMachineS *obj,int *timerNo);

/******************************************/
/* Accesor function for outside to do a
 * call back
 */
/******************************************/

void
stateCallBack(stateMachineS *obj,int arg1,void *arg2);

/******************************************/
/* distributor access functions of the SM */
/******************************************/
void
stateTimer(void *sm, void *timerNo);

void
stateMessage(void *sm, messageEnvolope *msg);

void
stateAudit(void *sm);

int
stateFd(void *sm,int fd, int event);

void
stateStartStopFun(void *sm,int eventType);


#endif
