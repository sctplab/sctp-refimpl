/*	$Header: /usr/sctpCVS/APPS/baselib/StateMachine.c,v 1.2 2008-12-26 14:45:12 randall Exp $ */

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
#include <StateMachine.h>


/******************************************/
/* distributor access functions of the SM */
/******************************************/
void
stateTimer(void *sm, void *timerNo, int timno)
{
	stateMachineS *obj;
	int *val;

	obj = (stateMachineS *) sm;
	val = (int *)timerNo;
	*val = 0;
	(*obj->currentState) (obj->objarg, STATE_MACH_TIMER, 0, timerNo);
}

void
stateMessage(void *sm, messageEnvolope * msg)
{
	stateMachineS *obj;

	obj = (stateMachineS *) sm;
	(*obj->currentState) (obj->objarg, STATE_MACH_MESSAGE, 0, (void *)msg);
}

void
stateStartStopFun(void *sm, int eventType)
{
	stateMachineS *obj;

	obj = (stateMachineS *) sm;
	if (eventType & DIST_CALL_ME_ON_START)
		(*obj->currentState) (obj->objarg, STATE_MACH_START, 0, 0);
	else if (eventType & DIST_CALL_ME_ON_STOP)
		(*obj->currentState) (obj->objarg, STATE_MACH_STOP, 0, 0);
}

void
stateAudit(void *sm)
{
	stateMachineS *obj;

	obj = (stateMachineS *) sm;
	(*obj->currentState) (obj->objarg, STATE_MACH_AUDITTICK, 0, 0);
}

int
stateFd(void *sm, int fd, int event)
{
	stateMachineS *obj;

	obj = (stateMachineS *) sm;
	(*obj->currentState) (obj->objarg, STATE_MACH_FD, fd, (void *)&event);
	return (0);
}

/* creation/destruction function */
stateMachineS *
createStateMachine(distributor * o, void *objarg, int numTimers,
    stateFunction firstState, int noStartup)
{
	stateMachineS *ret;

	ret = (stateMachineS *) CALLOC(1, sizeof(stateMachineS));
	if (ret == NULL) {
		return (NULL);
	}
	ret->obj = o;
	ret->currentState = firstState;
	ret->lastFree = 0;
	ret->objarg = objarg;
	ret->noStart = noStartup;
	if (numTimers == 0) {
		numTimers++;
	}
	ret->timerArray = (int *)CALLOC(numTimers, sizeof(int));
	if (ret->timerArray == NULL) {
		FREE(ret);
		return (NULL);
	}
	ret->timerCnt = numTimers;
	memset(ret->timerArray, 0, (sizeof(int) * numTimers));
	if (noStartup) {
		dist_startupChange(ret->obj, stateStartStopFun,
		    (void *)ret, DIST_CALL_ME_ON_STARTSTOP);
	} else {
		/*
		 * if the no startup option is set, then call the Start
		 * state right away.
		 */
		(*ret->currentState) (ret->objarg, STATE_MACH_START, 0, 0);
	}
	return (ret);
}

void
destroyStateMachine(stateMachineS * obj)
{
	int i;

	if (obj->noStart == 0) {
		dist_startupChange(obj->obj, stateStartStopFun,
		    (void *)obj, DIST_DONT_CALL_ME);
	} else {
		(*obj->currentState) (obj->objarg, STATE_MACH_STOP, 0, 0);
	}
	for (i = 0; i < obj->timerCnt; i++) {
		if (obj->timerArray[i] != 0) {
			/* timer is running, stop it */
			dist_TimerStop(obj->obj, stateTimer, (void *)obj, (void *)&obj->timerArray[i], 0);
			obj->timerArray[i] = 0;
		}
	}
	FREE(obj->timerArray);
	memset(obj, 0, sizeof(stateMachineS));
	FREE(obj);
}

/* Accesor to change state, could be used by the outside as well */
int
changeState(stateMachineS * obj, stateFunction newstate, int arg1, void *arg2)
{
	/* Exit state call may be removed .. not sure if we need this */
	(*obj->currentState) (obj->objarg, STATE_MACH_EXIT, arg1, arg2);
	obj->currentState = newstate;
	/* now call the new state and tell them that we have just entered */
	(*obj->currentState) (obj->objarg, STATE_MACH_ENTER, arg1, arg2);
	return (LIB_STATUS_GOOD);
}


/* Subscription things that the initial state should setup */


int
stateMachineAddFd(stateMachineS * obj, int fd, int events)
{
	return (dist_addFd(obj->obj, fd, stateFd, events, (void *)obj));
}

int
stateMachineDelFd(stateMachineS * obj, int fd)
{
	return (dist_deleteFd(obj->obj, fd));
}

int
stateMachineWantsMsg(stateMachineS * obj, int sctp_proto, int stream_no, int priority)
{
	int ret;

	ret = dist_msgSubscribe(obj->obj, stateMessage, sctp_proto, stream_no, priority, (void *)obj);
	return (ret);
}


int
stateMachineNoMsg(stateMachineS * obj, int sctp_proto, int stream_no, int priority)
{
	return (dist_msgUnsubscribe(obj->obj, stateMessage, sctp_proto, stream_no, (void *)obj));
}

int
stateMachineWantsAudit(stateMachineS * obj)
{
	return (dist_wants_audit_tick(obj->obj, stateAudit, (void *)obj));
}

int
stateMachineDisableAudit(stateMachineS * obj)
{
	return (dist_no_audit_tick(obj->obj, stateAudit, (void *)obj));
}


/* Timer control for state machine */
int *
startStateTimer(stateMachineS * obj, int sec, int usec)
{
	int i, fnd, ret;

	fnd = 0;

	for (i = obj->lastFree; i < obj->timerCnt; i++) {
		if (obj->timerArray[i] == 0) {
			fnd = 1;
			break;
		}
	}
	if (!fnd) {
		/* no timers left */
		for (i = 0; i < obj->lastFree; i++) {
			if (obj->timerArray[i] == 0) {
				fnd = 1;
				break;
			}
		}
	}
	if (!fnd) {
		return (NULL);
	}
	obj->lastFree = i + 1;
	ret = dist_TimerStart(obj->obj, stateTimer, sec, usec, (void *)obj,
	    (void *)&obj->timerArray[i]);
	if (ret >= LIB_STATUS_GOOD) {
		obj->timerArray[i] = 1;
		return (&obj->timerArray[i]);
	}
	return (NULL);
}

int
stopStateTimer(stateMachineS * obj, int *timerNo)
{
	int ret;

	ret = dist_TimerStop(obj->obj, stateTimer, (void *)obj, (void *)timerNo, 0);
	if (ret >= LIB_STATUS_GOOD)
		*timerNo = 0;
	return (ret);
}

/******************************************/
/* Accesor function for outside to do a
 * call back
 */
/******************************************/

void
stateCallBack(stateMachineS * obj, int arg1, void *arg2)
{
	(*obj->currentState) (obj->objarg, STATE_MACH_CALLBACK, arg1, arg2);
}
