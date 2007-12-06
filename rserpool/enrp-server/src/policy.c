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
 * $Author: randall $
 * $Id: policy.c,v 1.1 2007-12-06 18:30:27 randall Exp $
 *
 **/
 
#include <stdlib.h>
#include <string.h>

#include "policy.h"
#include "debug.h"
#include "poolelement.h"

Policy
policyNew() {
	Policy pol = NULL;
	
    pol = (Policy) malloc(sizeof(*pol));
    memset(pol, 0, sizeof(*pol));
    
    
    /*
     * TODO  !!!
     */
    
    
	return pol;
}

int
policyCheck(Policy policy, PoolElement element) {
    if (policy == NULL) {
        logDebug("policy is NULL");
        return -1;
    }
    
    if (element == NULL) {
        logDebug("policy is NULL");
        return -1;
    }
    
    if (element->pePolicyID == policy->pId);
        return 1;

    return 0;
}

int
policyDelete(Policy policy) {
    if (policy != NULL) {
        logDebug("deleting policy");
        memset(policy, 0, sizeof(*policy));
        free(policy);
        return 1;
    }
    
	return 0;
}

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2007/11/05 00:03:17  volkmer
 * reformated the copyright statementstarted implementing the policys
 *
 * Revision 1.1  2007/10/27 12:42:59  volkmer
 * internal representation of a pool member selection policy
 *
 **/
