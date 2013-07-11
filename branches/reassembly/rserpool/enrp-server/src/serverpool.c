/*
 * Copyright (c) 2006-2008, Michael Tuexen, Frank Volkmer. All rights reserved.
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
 * $Author: volkmer $
 * $Id: serverpool.c,v 1.3 2008-02-13 17:04:55 volkmer Exp $
 *
 **/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serverpool.h"
#include "poolelement.h"
#include "debug.h"
#include "checksum.h"
#include "util.h"
#include "parameters.h"

ServerPool serverPoolList;


ServerPool
serverPoolNew(char *poolHandle) {
    ServerPool pool = NULL;

    if (!poolHandle) {
        logDebug("given poolHandle is NULL");
        return NULL;
    }

    if (strlen(poolHandle) == 0) {
        logDebug("poolHandle length is 0");
        return NULL;
    }

    pool = (ServerPool) malloc(sizeof(*pool));
    memset(pool, 0, sizeof(*pool));

    pool->next     = NULL;
    pool->prev     = NULL;
    pool->spPeList = NULL;
    strncpy(pool->spHandle, poolHandle, POOLHANDLE_SIZE);

    logDebug("successfully created pool: %s", poolHandle);
    return pool;
}

void
serverPoolPrint(ServerPool pool) {
    PoolElement pelement = NULL;

    logDebug("Pool \"%s\":", pool->spHandle);

    for (pelement = pool->spPeList; pelement; pelement = pelement->poolNext) {
        poolElementPrint(pelement);
    }
}

PoolElement
serverPoolGetPoolElementById(ServerPool pool, uint32 pelementId) {
    PoolElement pelement;

    if (!pool)
        return NULL;

    for (pelement = pool->spPeList; pelement; pelement = pelement->poolNext)
        if (pelement->peIdentifier == pelementId)
            return pelement;

    return NULL;
}

int
serverPoolAddPoolElement(ServerPool pool, PoolElement newPoolElement) {
    char buf[POOLHANDLE_SIZE + 5];
    size_t poolHandleLen;
	uint32 peId;

    if (!newPoolElement) {
        logDebug("newPoolElement is NULL\n");
        return -1;
    }

    if (!pool) {
        logDebug("pool is NULL\n");
        return -1;
    }

    /* calculate checksum for pool element */
    memset(buf, 0, sizeof(buf));
    poolHandleLen = strlen(pool->spHandle);
    if (poolHandleLen < POOLHANDLE_SIZE) {
        memcpy(buf, pool->spHandle, poolHandleLen);
		peId = htonl(newPoolElement->peIdentifier);
		memcpy(buf + ADD_PADDING(poolHandleLen), &peId, 4);
        newPoolElement->peChecksum = checksumCompute(0, buf, ADD_PADDING(poolHandleLen) + 4);
    } else {
        logDebug("pool handle size is too big, can not calculate checksum");
        return -1;
    }

    /* check if already in peList */
    if (serverPoolGetPoolElementById(pool, newPoolElement->peIdentifier) != NULL) {
        logDebug("pelement with id 0x%08x is already in list, can not add it\n", newPoolElement->peIdentifier);
        return -1;
    }

    newPoolElement->poolPrev = NULL;
    newPoolElement->poolNext = pool->spPeList;
    if (pool->spPeList) {
        pool->spPeList->poolPrev = newPoolElement;
    }
    pool->spPeList = newPoolElement;

    newPoolElement->peServerPool = pool;

    logDebug("added 0x%08x to pool %s", newPoolElement->peIdentifier, pool->spHandle);
    return 1;
}

int
serverPoolRemovePoolElement(ServerPool pool, uint32 pelementId) {
    PoolElement pelement;

    /* check if not in peList */
    if ((pelement = serverPoolGetPoolElementById(pool, pelementId)) == NULL) {
        logDebug("0x%08x is not in list, can not delete it", pelementId);
        return -1;
    }

    if (pelement->poolPrev) {
        pelement->poolPrev->poolNext = pelement->poolNext;
    }
    if (pelement->poolNext) {
        pelement->poolNext->poolPrev = pelement->poolPrev;
    }
    if (pool->spPeList == pelement) {
        pool->spPeList = pelement->poolNext;
    }

    pelement->peServerPool = NULL;

    poolElementDelete(pelement);

    logDebug("removed 0x%08x from list", pelementId);
    return 1;
}

int
serverPoolDelete(ServerPool pool) {
    if (pool == NULL) {
        logDebug("pool is NULL");
        return -1;
    }

    if (pool->spPeList != NULL) {
        logDebug("spPeList is not NULL");
        return -1;
    }

    logDebug("pool %s will be deleted", pool->spHandle);
    memset(pool, 0, sizeof(*pool));
    free(pool);

    return 1;
}

int
serverPoolGetHandleResponse(ServerPool pool, int *count, uint32 *peIds) {
    PoolElement pe;
    Policy policy;

    if (pool == NULL) {
        logDebug("pool is NULL");
        
        return -1;
    }

    policy = pool->spPolicy;
    if (policy && policy->pId != POLICY_ROUND_ROBIN) {
        logDebug("unsupported policy");
        
        return -1;
    }
    
    /* most simple policy: return first 5 elements */
    *count = 0;
    memset(peIds, 0, sizeof(uint32) * 5);
    pe = pool->spPeList;
    poolElementPrint(pe);
    
    for (; *count < 5; (*count)++) {
        peIds[*count] = pe->peIdentifier;
        
        if (pe->serverNext != NULL) {
            pe = pe->serverNext;
        } else {
        	++(*count);
            break;
        }
    }
    
    /* TODO:
     * 
     * use policy struct to store handle resolution data
     * 
     * put methods to into policy.c/.h
     * 
     * */
    
    
    
    return 1;
}

int
serverPoolListAddPool(ServerPool newPoolElement) {
    if (!newPoolElement) {
        logDebug("newPoolElement is NULL");
        return -1;
    }

    /* check if pool already exists */
    if (serverPoolListGetPool(newPoolElement->spHandle) != NULL) {
        logDebug("pool %s already exists", newPoolElement->spHandle);
        return -1;
    }

    if (serverPoolList) {
        serverPoolList->prev = newPoolElement;
        newPoolElement->prev = NULL;
        newPoolElement->next = serverPoolList;
        serverPoolList       = newPoolElement;
    } else {
        serverPoolList       = newPoolElement;
        newPoolElement->next = NULL;
        newPoolElement->prev = NULL;
    }

    logDebug("successfully added %s to serverpool list", newPoolElement->spHandle);
    return 1;
}

void
serverPoolListPrint() {
    ServerPool pool;

    logDebug("###### Server Pool List ######");
    for (pool = serverPoolList; pool; pool = pool->next) {
        serverPoolPrint(pool);
    }
    logDebug("##############################");
}

ServerPool
serverPoolListGetPool(char *poolHandle) {
    ServerPool pool = NULL;

    if (!poolHandle) {
        logDebug("poolHandle is NULL");
        return pool;
    }

    for (pool = serverPoolList; pool; pool = pool->next)
        if (strncmp(pool->spHandle, poolHandle, POOLHANDLE_SIZE) == 0)
            return pool;

    return NULL;
}

int
serverPoolListRemovePool(char *poolHandle) {
    ServerPool pool;

    if (!poolHandle) {
        logDebug("poolHandle is NULL");
        return -1;
    }

    /* check if not in serverPoolList */
    if ((pool = serverPoolListGetPool(poolHandle)) == NULL) {
        logDebug("%s is not in list, can not delete it", poolHandle);
        return -1;
    }

    if (serverPoolList == pool) {
        serverPoolList = NULL;
    } else {
        pool->next->prev = pool->prev;
    }

    serverPoolDelete(pool);

    logDebug("removed %s from server pool list\n", poolHandle);
    return 1;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2007/12/27 01:06:27  skaliann
 * rserpool
 * - Fixed compilation errors and warnings
 * - modified the parameter type values to the latest draft
 * - Handle the case when the overall selection policy parameter is not sent
 *
 * enrp-server
 * - added poolHandle parameter to the HANDLE_RESOLUTION_RESPONSE
 * - Fixed a crash in policy selection code
 * - Fixed pelement->peIdentifier ntohl
 *
 * Revision 1.1  2007/12/06 18:30:27  randall
 * cloned all code over from M Tuexen's repository. May yet need
 * some updates.
 *
 * Revision 1.15  2007/12/02 22:07:18  volkmer
 * added method to get handle response information
 *
 * Revision 1.14  2007/11/06 08:18:43  volkmer
 * reformated the copyright statement
 *
 * Revision 1.13  2007/10/27 12:42:00  volkmer
 * removed debug macros
added policys to the pool struct
 *
 **/
