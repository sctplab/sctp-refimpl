/*
 * Copyright (C) 2008 Randall R. Stewart
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
the SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@lakerest.net

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/
#include <processObj.h>
#include <printMessageHandler.h>

static
int 
process_fdHandler(void *v, int fd, int event)
{
	int readin = 0, consumed = 0;
	uint32_t len = 0;
	struct processDescriptor *pd;

	pd = (struct processDescriptor *)v;

	/* A bit of sanity please */
	if (fd != fileno(pd->pip)) {
		DEBUG_PRINTF("PROCOBJ:Distribution error, my fd:%d fd sent:%d - shutdown\n",
		    fd, fileno(pd->pip));
		dist_setDone(pd->d);
		/* return 0, we are done */
		return (0);
	}
	if (event & POLLIN) {
		/* A read event */
		readin = read(fileno(pd->pip), &pd->recvBuffer[pd->readIndex],
		    (PROC_BUFFER_SIZE - pd->readIndex));
		if (readin < 0) {
			/*
			 * We need proper error handling here. We need to
			 * recognize when sockets shutdown and then start
			 * the reconnect to the SatMap Engine. For now we
			 * bail.
			 */
			DEBUG_PRINTF("PROCOBJ:Got an error on reading from the FD:%d, bail\n", errno);
			dist_setDone(pd->d);
			return (0);
		}
		/* update the end and where we next read to */
		pd->bufEnd += readin;
		pd->readIndex += readin;

readmore:
		len = pd->bufEnd - pd->bufStart;
		if (len)
			consumed = (*pd->mf) ((void *)pd, &pd->recvBuffer[pd->bufStart], len, pd->auxObj);
		else
			consumed = 0;

		if (consumed > 0) {
			pd->bufStart += consumed;
		} else if (consumed < 0) {
			/* abort the buffer */
			DEBUG_PRINTF("PROCOBJ:Proc aborted a buffer start:%d stop:%d\n",
			    pd->bufStart, pd->bufEnd);
			pd->bufEnd = pd->bufStart = pd->readIndex = 0;
			goto done;
		}
		if ((((int)readin == 0) && ((int)consumed == (int)len)) ||
		    ((readin == 0) && (consumed == 0))) {
			/*
			 * 0 length read - EOF and either the consumer read
			 * the entire buffer, or the consumer is no longer
			 * eating the data. If so the connection is over.
			 */
			if (pd->mx) {
				(*pd->mx) ((void *)pd, &pd->recvBuffer[pd->bufStart], len, pd->auxObj);
			}
			dist_deleteFd(pd->d, fileno(pd->pip));
			pclose(pd->pip);
			pd->pip = NULL;
			return (0);
		} else if (consumed) {
			/*
			 * we consumed some, but there are more records to
			 * read
			 */
			goto readmore;
		}
done:
		/* data left in buffer */
		len = pd->bufEnd - pd->bufStart;
		if ((pd->bufEnd == pd->bufStart) ||
		    (pd->maxlen == 0) ||
		    ((pd->maxlen - len) <= (PROC_BUFFER_SIZE - pd->readIndex))) {
			/*
			 * Do we need to rearrange data? we do this when all
			 * the data is consumed or there is not room to put
			 * another largest size message in the buffer i.e.
			 * what we have left in the buffer taken away from
			 * our largest message size gives us how much more
			 * we need to collect, if there is not that much
			 * room (TCP_BUFFER_SIZE - index) then we must
			 * rearrange. Note if the user sends a 0 for maxlen
			 * we ALWAYS re-arrange.
			 */
			if (pd->bufStart > 0) {
				/*
				 * Relocate the buffer to the top. Note that
				 * for now we just always move the data to
				 * the top. We will need to optimize this
				 * later to either use a true circular
				 * buffer <or> only relocate when there is
				 * not room for the largest message from the
				 * bufStart -> to the end of the buffer. We
				 * probably don't need to do the circular
				 * buffer, when we move to SCTP we would
				 * never have bufStart be anything but 0.
				 * The split message is only a TCP issue.
				 */
				/* sanity */
				if (pd->bufStart > pd->bufEnd) {
					/* ahh, TSNH */
					DEBUG_PRINTF("PROCOBJ:pd->bufStart:%d > pd->bufEnd:%d - aborts the buffer",
					    pd->bufStart, pd->bufEnd);
					pd->bufStart = pd->bufEnd = pd->readIndex = 0;
				} else if (pd->bufEnd > pd->bufStart) {
					/*
					 * move up the data to front of
					 * buffer
					 */
					memcpy(pd->recvBuffer, &pd->recvBuffer[pd->bufStart], (pd->bufEnd - pd->bufStart));
					pd->bufEnd -= pd->bufStart;
					pd->readIndex -= pd->bufStart;
					pd->bufStart = 0;
				} else {
					/*
					 * nothing left in the buffer i.e.
					 * bufEnd == bufStart
					 */
					pd->bufEnd = pd->bufStart = pd->readIndex = 0;
				}
			}
		}
	}
	if (event & POLLHUP) {
		dist_deleteFd(pd->d, fileno(pd->pip));
		pclose(pd->pip);
		pd->pip = NULL;
		if (pd->mx) {
			(*pd->mx) ((void *)pd, &pd->recvBuffer[pd->bufStart], len, pd->auxObj);
		}
		return (0);
	}
	return (0);
}


struct processDescriptor *
initialize_processObj(distributor * d, processMsgFramer mf,
    processExits px,
    char *command, int maxbuflen, void *aux)
{
	struct processDescriptor *pd;

	pd = (struct processDescriptor *)MALLOC(sizeof(struct processDescriptor));
	if (pd == NULL) {
		DEBUG_PRINTF("No memory for process descriptor err:%d\n", errno);
outof:
		dist_setDone(d);
		return (pd);
	}
	memset(pd, 0, sizeof(struct processDescriptor));
	pd->commandString = (char *)MALLOC(strlen(command) + 1);
	if (pd->commandString == NULL) {
		DEBUG_PRINTF("No memory for command string '%s'\n", command);
		FREE(pd);
		pd = NULL;
		goto outof;
	}
	pd->mf = mf;
	pd->mx = px;
	strcpy(pd->commandString, command);
	pd->pip = popen(command, "r+");
	if (pd->pip == NULL) {
		DEBUG_PRINTF("Can't open command '%s' error:%d\n", command, errno);
out:
		FREE(pd->commandString);
		FREE(pd);
		pd = NULL;
		goto outof;
	}
	pd->d = d;
	pd->auxObj = aux;

	if (dist_addFd(pd->d, fileno(pd->pip),
	    process_fdHandler,
	    POLLIN,
	    (void *)pd) != LIB_STATUS_GOOD) {
		DEBUG_PRINTF("Can't setup the fd of pip on the distributor - errno:%d", errno);
		pclose(pd->pip);
		goto out;
	}
	return (pd);
}

int
sendToProcess(struct processDescriptor *pd, char *msg, int len)
{
	int ret;

	if (pd->pip == NULL) {
		DEBUG_PRINTF("No process\n");
		return (-1);
	}
	if (msg[(len - 1)] != '\n') {
		DEBUG_PRINTF("No nl at end of msg/command\n");
		return (-1);
	}
	ret = fputs(msg, pd->pip);
	fflush(pd->pip);
	return (ret);
}

void
destroy_process(struct processDescriptor *pd)
{
	if (pd == NULL) {
		return;
	}
	if (pd->pip) {
		dist_deleteFd(pd->d, fileno(pd->pip));
		pclose(pd->pip);
		pd->pip = NULL;
	}
	FREE(pd->commandString);
	FREE(pd);
}
