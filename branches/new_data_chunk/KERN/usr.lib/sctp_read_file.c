/*	$KAME: sctp_sys_calls.c,v 1.9 2004/08/17 06:08:53 itojun Exp $ */

/*
 * Copyright (C) 2002, 2003, 2004 Cisco Systems Inc,
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include <net/if_dl.h>

static void
hexDump(void *vdb, int sz)
{
  int i,j, prn;
  unsigned char str[20];
  char *db;
  db = (unsigned char *)vdb;

  for(i=0, j=0; i<sz; i++) {
    prn = (db[i] & 0x000000ff);
    printf("%2.2x ", prn);
    if((db[i] >= '!') &&
       (db[i] <= 'z')) {
      str[j++] = db[i];
    }else {
      str[j++] = '.';
    }
    if(((i+1) % 16) == 0) {
      str[16] = 0;
      printf(" %s\n", str);
      j = 0;
    }
  }
  if(sz % 16) {
    str[j] = 0;
    printf(" %s\n", str);
    j = 0;
  }
}


int
main (int argc, char **argv)
{
  FILE *io;
  struct sctp_sndrcvinfo *s_info;
  struct msghdr msg;
  struct iovec iov[2];
  char controlVector[200000];
  char dbuf[200000];
  struct stat sb;
  ssize_t sz;

  if(argc < 2) {
    printf("Need a file name\n");
    printf("use %s filename\n", argv[0]);
    return (0);
  }

  if (stat(argv[1], &sb) == -1) {
    printf("Error %d can't stat file\n", errno);
    return (-1);
  }
  io = fopen(argv[1], "r");
  if(io == NULL) {
    printf("Gak, can't write control file failed to open errno:%d\n", errno);
    goto out_of_here;
  }
  if(fread((void *)&msg, sizeof(msg), 1, io) != 1) {
    printf("Error can't read msg err:%d\n",errno);
    goto out_of_here_close;
  }
  printf("Read the msg vector\n");
  printf("msg_name %x len:%d flags:%x\n", 
	 (u_int)msg.msg_name, msg.msg_namelen,
	 msg.msg_flags);
  printf("msg_iovlen:%d\n", msg.msg_iovlen);
  printf("msg_controllen:%d\n", msg.msg_controllen);
  printf("msg_flags:%x\n", msg.msg_flags);
  sz = sb.st_size - (sizeof(msg) + msg.msg_controllen);
  printf("Data left over is %d\n", sz);
  if(fread(controlVector, msg.msg_controllen, 1, io) != 1) {
    printf("Error can't read control data err:%d\n",errno);
    goto out_of_here_close;
  }
  printf("---------Control-----------------\n");
  hexDump(controlVector, msg.msg_controllen);
  if(fread(dbuf, sz, 1, io) != 1) {
    printf("Error can't read data err:%d\n",errno);
  }
  printf("---------Data-----------------\n");
  hexDump(dbuf, sz);
  out_of_here_close:
    fclose(io);
  out_of_here:
    return(0);
}
