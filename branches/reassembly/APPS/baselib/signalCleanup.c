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
#include <signalCleanup.h>

static distributor *dis = NULL;

static void
sigHandler(int sig)
{
	if (dis) {
		dist_setDone(dis);
	}
}

void
initialize_sigcleanup(distributor * o, int *sigs, int len)
{
	int i;

	if ((sigs == NULL) || (o == NULL)) {
		printf("Can't init signal handlers - EINVAL\n");
	}
	dis = o;
	for (i = 0; i < len; i++) {
		if ((sigs[i] == -1) ||
		    (sigs[i] == 0) ||
		    (sigs[i] > SIGUSR2)
		    ) {
			break;
		}
		signal(sigs[i], sigHandler);
	}
}

void
sigcleanup_ignore(distributor * o, int *sigs, int len)
{
	int i;

	if ((sigs == NULL) || (o == NULL)) {
		printf("Can't init signal handlers - EINVAL\n");
	}
	dis = o;
	for (i = 0; i < len; i++) {
		if ((sigs[i] == -1) ||
		    (sigs[i] == 0) ||
		    (sigs[i] > SIGUSR2)
		    ) {
			break;
		}
		signal(sigs[i], SIG_IGN);
	}
}
