/*	$Header: /usr/sctpCVS/APPS/baselib/msgDirector.c,v 1.2 2008-12-26 14:45:12 randall Exp $ */

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

#include <msgDirector.h>
/*
 * You provide a function that looks as follows.
 * Your function will be called with the specific
 * object looked up by the query function. The
 * query function has access to the hash table stored
 * with this object.
 */
void
distribMessage(void *obj, messageEnvolope * msg)
{
	void *passObj;
	messageDir *dir;

	dir = (messageDir *) obj;
	passObj = (*dir->query) (dir->objinfo, msg, dir->hash);
	if (passObj == NULL)
		/* no one to handle the message */
		return;
	/* we have a live one :), pass it off */
	(*dir->mf) (passObj, msg);
}

/*
 * Creation and registration.
 */

messageDir *
messageDir_create(distributor * o,
    void *objinfo,
    messageFunc mf,
    messageQuery qry,
    int priority)
{
	int x;
	messageDir *ret;

	ret = (messageDir *) CALLOC(1, sizeof(messageDir));
	if (ret == NULL)
		return (ret);
	ret->hash = HashedTbl_create("msgD tb", 26);
	if (ret->hash == NULL) {
		FREE(ret);
		return (NULL);
	}
	ret->obj = o;
	ret->objinfo = objinfo;
	ret->mf = mf;
	ret->query = qry;
	/* now register for messages */
	x = dist_msgSubscribe(o, distribMessage,
	    DIST_SCTP_PROTO_ID_DEFAULT,
	    DIST_STREAM_DEFAULT, priority,
	    (void *)ret);
	if (x < LIB_STATUS_GOOD) {
		/* error, we can't register? */
		HashedTbl_destroy(ret->hash);
		FREE(ret);
		return (NULL);
	}
	return (ret);
}

/*
 * Destructor function.
 */
int
messageDir_destroy(messageDir * dir)
{
	dist_msgUnsubscribe(dir->obj,
	    distribMessage,
	    DIST_SCTP_PROTO_ID_DEFAULT,
	    DIST_STREAM_DEFAULT,
	    (void *)dir);
	HashedTbl_destroy(dir->hash);
	FREE(dir);
	return (LIB_STATUS_GOOD);
}
