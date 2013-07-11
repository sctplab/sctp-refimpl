/*	$Header: /usr/sctpCVS/APPS/user/userInputModule.h,v 1.3 2009-01-11 13:23:46 randall Exp $ */

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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
#include <poll.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <stddef.h>
#include <net/if.h>
#ifndef __userInputModule_h__
#define __userInputModule_h__

#include "sctpAdaptor.h"
#include <distributor.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
#include <netinet/sctp_constants.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
  int    stream;
  int    nbRequests;
  int    counter;
  int    started;
  int    blockAfter;
  int    blockDuring;
  time_t blockTime;
} SPingPongContext;

#define MAX_PING_PONG_CONTEXTS 10
  void SCTPPrintAnAddress(struct sockaddr *a);
  void checkBulkTranfer(void *v,void *xx, int tmrno);	/* timer function */
  void sctpInput(void *m,messageEnvolope *msg);
  void handleStdin(sctpAdaptorMod *mod);
  u_long translateIPAddress(char *);
  void resetPingPongTable(void);
  int stdinInFdHandler(void *arg,int fd, int event);
  void initUserInterface(distributor *o,
			 sctpAdaptorMod *mod
			 );
  void destroyUserInterface(void);
#ifdef	__cplusplus
}
#endif


#endif




