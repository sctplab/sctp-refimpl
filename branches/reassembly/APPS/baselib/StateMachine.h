/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

$Header: /usr/sctpCVS/APPS/baselib/StateMachine.h,v 1.3 2008-12-26 14:45:12 randall Exp $
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
stateTimer(void *sm, void *timerNo, int timno);

void
stateMessage(void *sm, messageEnvolope *msg);

void
stateAudit(void *sm);

int
stateFd(void *sm,int fd, int event);

void
stateStartStopFun(void *sm,int eventType);


#endif
