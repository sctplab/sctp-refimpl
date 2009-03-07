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
#include <sys/types.h>
#include <sys/stat.h>
#include <printMessageHandler.h>

FILE *print_output = NULL;

static void
timevalfix(struct timeval *t1)
{
	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

void
timevaladd(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

void
timevalsub(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}


void
printAnAddress(struct sockaddr *a)
{
	char stringToPrint[256];
	u_short prt;
	char *srcaddr, *txt;

	if (print_output == NULL) {
		print_output = stdout;
	}
	if (a == NULL) {
		fprintf(print_output, "NULL\n");
		return;
	}
	if (a->sa_family == AF_INET) {
		srcaddr = (char *)&((struct sockaddr_in *)a)->sin_addr;
		txt = "IPv4 Address: ";
		prt = ntohs(((struct sockaddr_in *)a)->sin_port);
	} else if (a->sa_family == AF_INET6) {
		srcaddr = (char *)&((struct sockaddr_in6 *)a)->sin6_addr;
		prt = ntohs(((struct sockaddr_in6 *)a)->sin6_port);
		txt = "IPv6 Address: ";
	} else {
		return;
	}
	if (inet_ntop(a->sa_family, srcaddr, stringToPrint, sizeof(stringToPrint))) {
		if (a->sa_family == AF_INET6) {
			fprintf(print_output, "%s%s:%d scope:%d\n",
			    txt, stringToPrint, prt,
			    ((struct sockaddr_in6 *)a)->sin6_scope_id);
		} else {
			fprintf(print_output, "%s%s:%d\n", txt, stringToPrint, prt);
		}

	} else {
		fprintf(print_output, "%s unprintable?\n", txt);
	}
}


void
printArry(uint8_t * data, int sz)
{
	/* if debug is on hex dump a array */
	int i, j, linesOut;
	char buff1[64];
	char buff2[64];
	char *ptr1, *ptr2, *dptrlast, *dptr;
	char *hexes = "0123456789ABCDEF";

	if (print_output == NULL) {
		print_output = stdout;
	}
	ptr1 = buff1;
	ptr2 = buff2;
	dptrlast = dptr = (char *)data;
	for (i = 0, linesOut = 0; i < sz; i++) {
		*ptr1++ = hexes[0x0f & ((*dptr) >> 4)];
		*ptr1++ = hexes[0x0f & (*dptr)];
		*ptr1++ = ' ';
		if ((*dptr >= 040) && (*dptr <= 0176))
			*ptr2++ = *dptr;
		else
			*ptr2++ = '.';
		dptr++;
		if (((i + 1) % 16) == 0) {
			*ptr1 = 0;
			*ptr2 = 0;
			fprintf(print_output, "%s %s\n", buff1, buff2);
			linesOut++;
			ptr1 = buff1;
			ptr2 = buff2;
			dptrlast = dptr;
		}
	}
	if ((linesOut * 16) < sz) {
		char spaces[64];
		int dist, sp;

		j = (linesOut * 16);
		dist = ((16 - (i - j)) * 3);
		*ptr1 = 0;
		*ptr2 = 0;
		for (sp = 0; sp < dist; sp++) {
			spaces[sp] = ' ';
		}
		spaces[sp] = 0;
		fprintf(print_output, "%s %s%s\n", buff1, spaces, buff2);
	}
	fflush(print_output);
}

static void
messageInput(void *arg, messageEnvolope * msg)
{
	/* receive some number of datagrams and act on them. */
	int disped = 0;
	distributor *o;
	struct print_msg_info *pri;

	pri = (struct print_msg_info *)arg;
	char *poi;

	o = pri->d;
	if (print_output == NULL) {
		print_output = stdout;
	}
	/* display a text message */
	if (isascii(((char *)msg->data)[0])) {
		if (!pri->quiet) {
			fprintf(print_output, "From:");
			printAnAddress(msg->from);
			if (msg->to) {
				fprintf(print_output, "To:");
				printAnAddress(msg->to);
			}
		}
		poi = (char *)msg->data;
		poi[msg->siz] = 0;
		if (msg->siz) {
			if ((poi[msg->siz - 1] == '\n') || (poi[msg->siz - 1] == '\r')) {
				((char *)msg->data)[msg->siz - 1] = 0;
			}
		}
		if (msg->siz > 1) {
			if ((poi[msg->siz - 2] == '\n') || (poi[msg->siz - 2] == '\r')) {
				((char *)msg->data)[msg->siz - 1] = 0;
			}
		}
		if (!pri->quiet) {
			fprintf(print_output, "PPID:%d strm:%d seq:%d %d:'%s'\n",
			    msg->protocolId,
			    msg->streamNo,
			    msg->streamSeq,
			    msg->siz,
			    (char *)msg->data);
		} else {
			fprintf(print_output, "%s\n", (char *)msg->data);
		}
		disped = 1;
	} else {
		if (!pri->quiet) {
			fprintf(print_output, "From:");
			printAnAddress(msg->from);
			if (msg->to) {
				fprintf(print_output, "To:");
				printAnAddress(msg->to);
			}
		}
		fprintf(print_output, "PPID:%d strm:%d seq:%d %d:\n",
		    msg->protocolId,
		    msg->streamNo,
		    msg->streamSeq,
		    msg->siz);
		printArry((uint8_t *) msg->data, (msg->siz > 260) ? 260 : msg->siz);
		disped = 1;
	}
	if ((disped) && (!pri->quiet)) {
		fprintf(print_output, "\n>");
		fflush(print_output);
	}
}

struct print_msg_info *
create_printMessageHandler(distributor * o, int quiet)
{
	struct print_msg_info *inf;

	if (print_output == NULL) {
		print_output = stdout;
	}
	inf = MALLOC(sizeof(struct print_msg_info));
	if (inf == NULL) {
		fprintf(print_output, "No memory\n");
		return (NULL);
	}
	memset(inf, 0, sizeof(struct print_msg_info));
	inf->d = o;
	inf->quiet = quiet;
	if (dist_msgSubscribe(o, messageInput,
	    DIST_SCTP_PROTO_ID_DEFAULT,
	    DIST_STREAM_DEFAULT,
	    0x7fffffff, (void *)inf) != LIB_STATUS_GOOD) {
		FREE(inf);
		inf = NULL;
		return (inf);
	}
	return (inf);
}


void
destroy_printMessageHandler(struct print_msg_info *i)
{
	dist_msgUnsubscribe(i->d,
	    messageInput,
	    DIST_SCTP_PROTO_ID_DEFAULT,
	    DIST_STREAM_DEFAULT,
	    (void *)i);
	FREE(i);
}

static char fullpath_to_output[512];
void
printSetOutput(char *fullpath)
{
	FILE *io;

	if (print_output == NULL) {
		print_output = stdout;
	}
	strcpy(fullpath_to_output, fullpath);
	io = fopen(fullpath, "a+");
	if (io != NULL) {
		fprintf(print_output, "Log File set to new file %s\n", fullpath);
		if (print_output != stdout) {
			fclose(print_output);
		}
		print_output = io;
	} else {
		fprintf(print_output, "Can't set log file output errno:%d on file:%s\n",
		    errno,
		    fullpath);
	}
}

void
printFlushOutput(void)
{
  if (print_output)
    fflush(print_output);
  else
	fflush(stdout);
}

static void
printRollLogFile(int maxfiles)
{
	char newfile[512];
	struct stat sbuf;
	int i, fnd = 0;
	FILE *newout;

	for (i = 0; i < maxfiles; i++) {
		sprintf(newfile, "%s.%d",
		    fullpath_to_output, i);
		if (stat(newfile, &sbuf) == -1) {
			fnd = 1;
			break;
		}
	}
	if (!fnd) {
		fprintf(print_output, "****Warning: max log files reached purging %s****\n",
		    newfile);
		unlink(newfile);
	}
	if (link(fullpath_to_output, newfile) == -1) {
		fprintf(print_output, "Can't unlink to move file:%d\n", errno);
		return;
	}
	if (unlink(fullpath_to_output) == -1) {
		fprintf(print_output, "Can't unlink old file err:%d\n", errno);
	}
	newout = fopen(fullpath_to_output, "w+");
	if (newout == NULL) {
		fprintf(print_output, "Can't open new file?\n");
		return;
	}
	fclose(print_output);
	print_output = newout;
	fprintf(print_output, "****Log file rolls over****\n");
}

void
coreFileMoveCheck(char *procname, int maxfiles)
{
  char corefile[512];
  char newfile[512];
  struct stat sbuf;
  int fnd=0;
  int i;
  if (procname == NULL)
	return;
  sprintf(corefile, "%s.core", procname);
  if (stat(corefile, &sbuf) == -1) {
	/* no core */
	return;
  }
  /* need to move */
  for (i = 0; i < maxfiles; i++) {
	sprintf(newfile, "%s.%d",
		    corefile, i);
	if (stat(newfile, &sbuf) == -1) {
	  fnd = 1;
	  break;
	}
  }
  if (!fnd) {
	/* nothing free, just unlink the last one */
	fprintf(print_output, "coreMove:Unlinking largest file to move core to\n");
	unlink(newfile);
  }
  fprintf(print_output, "coreMove: moves core %s to %s\n", corefile, newfile);
  if (link(corefile, newfile) == -1) {
	fprintf(print_output, "coreMove:Can't unlink to move file:%d\n", errno);
	return;
  }
  if (unlink(corefile) == -1) {
	fprintf(print_output, "coreMove:Can't unlink old file err:%d\n", errno);
  }
}

void
printCheckLogFile(int maxsize, int maxfiles)
{
	struct stat sbuf;

	if (print_output == stdout) {
		/* Nothing to do */
		return;
	}
	if (fstat(fileno(print_output), &sbuf) == -1) {
		/* can't stat it */
		return;
	}
	if ((sbuf.st_mode & S_IFMT) != S_IFREG) {
		/* Not a regular file */
		return;
	}
	if (sbuf.st_size > maxsize) {
		/* Need to roll over the file */
		printRollLogFile(maxfiles);
	}
}
