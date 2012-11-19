/*
 * Copyright (c) 2006-2007, Michael Tuexen, Frank Volkmer. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Michael Tuexen nor Frank Volkmer nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * $Author: tuexen $
 * $Id: serverpool.h,v 1.2 2008-03-06 17:56:50 tuexen Exp $
 *
 **/
#ifndef _SERVERPOOL_H
#define _SERVERPOOL_H 1

#include "backend.h"
#include "poolelement.h"
#include "policy.h"

#define POOLHANDLE_SIZE  32

typedef struct server_pool_struct *ServerPool;
struct server_pool_struct {
    ServerPool  next;
    ServerPool  prev;
    char        spHandle[POOLHANDLE_SIZE + 1];
    PoolElement spPeList;
	Policy      spPolicy;
};

extern ServerPool serverPoolList;

ServerPool
serverPoolNew(char *poolHandle);

PoolElement
serverPoolGetPoolElementById(ServerPool pool, uint32 pelementId);

void
serverPoolPrint(ServerPool pool);

int
serverPoolAddPoolElement(ServerPool pool, PoolElement newPoolElement);

int
serverPoolRemovePoolElement(ServerPool pool, uint32 pelementId);

int
serverPoolDelete(ServerPool pool);

void
serverPoolListPrint();

int
serverPoolGetHandleResponse(ServerPool pool, int *count, uint32 *peIds);

int
serverPoolListAddPool(ServerPool newServerPool);

ServerPool
serverPoolListGetPool(char *poolHandle);

int
serverPoolListRemovePool(char *poolHandle);

#endif

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.12  2007/12/02 22:07:18  volkmer
 * added method to get handle response information
 *
 * Revision 1.11  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.10  2007/10/27 12:42:00  volkmer
 * removed debug macros
added policys to the pool struct
 *
 **/
