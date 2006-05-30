/*	$Header: /usr/sctpCVS/rserpool/lib/udpDist.c,v 1.1 2006-05-30 18:02:03 randall Exp $ */

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

#include <udpDist.h>

int
udpFDawaken(void *o,int fd,int event)
{
  int ret;
  char readbuf[UDPDIST_MAXMESSAGE],*dat;
  messageEnvolope env;
  struct sockaddr_storage s;  
  socklen_t sa_len;
  struct udpDist *obj;
  obj = (struct udpDist *)o;
  if(fd != obj->udpfd){
    printf("Huh? got a fd event %d on fd:%d my udpfd is %d .. deleting fd\n",
	   event,fd,obj->udpfd);
    dist_deleteFd(obj->dist,fd);
    return(0);
  }
  if(event == POLLIN){
    sa_len = sizeof(s);
    ret = recvfrom(obj->udpfd,readbuf,sizeof(readbuf),0,
		   (struct sockaddr *)&s,&sa_len);
    if(ret <= 0){
      return(0);
    }
    /* SCTP things we don't support */
    env.TSN = 0;
    env.protocolId = 0;
    env.streamNo = 0;
    env.streamSeq = 0;
    env.siz = env.totSize = ret;
  
    env.from = (void *)&s;
    env.from_len = (u_int)sa_len;
    env.type = PROTOCOL_Udp;
    /* No others yet, we are the originator */
    env.origFrom = NULL;
    env.origType =  PROTOCOL_Unknown;
    /* Don't have the to info */
    env.to = NULL;
    env.to_len = 0;
    env.distrib = (void *)obj->dist;
    env.sender = (void *)obj;	
    /* ok, we have a message of ret bytes, distribute it */
    dat = calloc(1,ret);
    if(dat != NULL){
      memcpy(dat,readbuf,ret);
      env.totData = env.data = dat;
      env.takeOk = 1;
    }else{
      env.totData = env.data = readbuf;
      env.takeOk = 0;
    }
    dist_sendmessage(obj->dist,&env);
  }
  return(0);
}


struct udpDist *
createudpDist(distributor *dist,u_short port,u_short family)
{
  struct udpDist *obj;
  obj = calloc(1,sizeof(struct udpDist));
  if(obj == NULL){
    return(NULL);
  }
  obj->dist = dist;
  obj->udpfd = socket(family,SOCK_DGRAM,IPPROTO_UDP);
  if(obj->udpfd == -1){
    printf("Can't open UDP fd error:%d\n",errno);
    free(obj);
    return(NULL);	
  }

  if(family == AF_INET){
    struct sockaddr_in sin;
    memset((char *)&sin,0,sizeof(sin));
    /* we take any port and address */
    sin.sin_port = port;
    sin.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
    sin.sin_len = sizeof(struct sockaddr_in);
#endif
    bind(obj->udpfd,(struct sockaddr *)&sin,sizeof(struct sockaddr_in));
  }else if(family == AF_INET6){
    struct sockaddr_in6 sin6;
    memset((char *)&sin6,0,sizeof(sin6));
    /* we take any port and address */
    sin6.sin6_port = port;
    sin6.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
    sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
    bind(obj->udpfd,(struct sockaddr *)&sin6,sizeof(struct sockaddr_in6));
  }else{
    printf("Unsupported family %d\n",family);
    close(obj->udpfd);
    free(obj);
    return(NULL);
  }
  /* Now verify binding if applicable */
  if(port){
    struct sockaddr_storage s;
    struct sockaddr_in *sin;
    socklen_t sa_len;

    sa_len = sizeof(struct sockaddr_storage);
    memset((char *)&s,0,sizeof(s));
#ifdef HAVE_SA_LEN
    s.ss_len = sa_len;
#endif
    if(getsockname(obj->udpfd,(struct sockaddr *)&s,&sa_len) < 0){
      /* Can't get name? */
      printf("Error can't get socket name after bind %d\n",errno);
      close(obj->udpfd);
      free(obj);
      return(NULL);
    }
    /* we use socaddr_in for comparison, either is
     * in the same place for the port
     */
    sin = (struct sockaddr_in *)&s;
    if(port != sin->sin_port){
      printf("Failed to get port %d got %d\n",
	     ntohs(port),ntohs(sin->sin_port));
      close(obj->udpfd);
      free(obj);
      return(NULL);
    }
  }
  if(dist_addFd(obj->dist,
		obj->udpfd,
		udpFDawaken,
		POLLIN,
		(void *)obj) != LIB_STATUS_GOOD){
    printf("Can't add read fd to distributor for %d\n",obj->udpfd);
    free(obj);
    close(obj->udpfd);
    return(NULL);
  }
  return(obj);
}

void
destroyudpDist(struct udpDist *obj)
{
  dist_deleteFd(obj->dist,obj->udpfd);
  close(obj->udpfd);
  free(obj);
}
