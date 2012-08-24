#ifndef __sctpAdaptor_h__
#define __sctpAdaptor_h__
/*	$Header: /usr/sctpCVS/APPS/user/sctpAdaptor.h,v 1.2 2006-01-25 18:46:40 lei Exp $ */

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

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <netinet/sctp_uio.h>
#endif
#include <distributor.h>

/* This constant (SCTP_MAX_READBUFFER) define
 * how big the read/write buffer is
 * when we enter the fd event notification
 * the buffer is put on the stack, so the bigger
 * it is the more stack you chew up, however it
 * has got to be big enough to handle the bigest
 * message this O/S will send you. In solaris
 * with sockets (not TLI) we end up at a value
 * of 64k. In TLI we could do partial reads to
 * get it all in with less hassel.. but we
 * write to sockets for generality.
 */
#define SCTP_MAX_READBUFFER	400000


#define SCTP_UDP_TYPE         0x00001
#define SCTP_TCP_TYPE         0x00002
#define SCTP_TCP_IS_LISTENING 0x00004

typedef struct sctpAdaptorMod{
  distributor *o;
  int fd;
  int model;
}sctpAdaptorMod;

sctpAdaptorMod *
create_SCTP_adaptor(distributor *o, u_short port,int model,int rwnd, int swnd);

void
destroy_SCTP_adaptor(sctpAdaptorMod *);

int
sctpReadInput(int fd, distributor *o,sctpAdaptorMod *);

int
sctpFdInput(void *arg,int fd ,int event);

int
SCTP_setContinualInit(int value);
int
SCTP_getContinualInit(void);

int
SCTP_setIPaddrinfo(struct addrinfo *res);

void SCTP_setIPaddr(u_int addr);

void SCTP_setport(u_short prt);

void SCTP_setBindAddr(u_int addr);
void SCTP_setBind6Addr(u_char *addr);
void SCTP_setV4only(void);
void SCTP_setV6only(void);

void restore_SCTP_fd(sctpAdaptorMod *r);

void
SCTP_setIPaddr6(u_char *addr);

void
SCTP_setIPv6scope(u_int scope);

u_short SCTP_getport();

void SCTP_setcurrent(sctpAdaptorMod *r);

struct sockaddr *SCTP_getAddr(socklen_t *len);

int SCTP_getBind(void);

#endif

