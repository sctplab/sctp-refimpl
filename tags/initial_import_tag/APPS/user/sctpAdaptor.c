/*	$Header: /usr/sctpCVS/APPS/user/sctpAdaptor.c,v 1.40 2011-05-09 09:42:07 randall Exp $ */

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
#define __BSD_SCTP_STACK__
#ifndef HAVE_SA_LEN
#define HAVE_SA_LEN
#endif
#endif
#include <netinet/in.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#ifndef linux 
#include <net/if_types.h>
#include <net/if_dl.h>
#endif
#include <netinet/in.h>

#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/uio.h>
#include <netdb.h>

#include <sctpAdaptor.h>
#include <netinet/sctp.h>

static int continualINIT=0;
static unsigned char to_ip[256];
static socklen_t to_ip_len;
static struct sockaddr_storage bind_ss[100];

sctpAdaptorMod *object_in = NULL;
static int lastStream=0;
static int lastStreamSeq=0;
static char *dataout=NULL;
static int dtsize=0;
static distributor *big_o=NULL;
static int bindSpecific = 0;
static int v6only = 0;
static int v4only = 0;
static int mainFd;

int scope_id = 0;	/* scope_id for destination and local binds */

static struct sctp_sndrcvinfo o_info;

static void
handle_notification(int fd,char *notify_buf) 
{
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
#if defined(__BSD_SCTP_STACK__)
	struct sctp_authkey_event *auth;
	struct sctp_stream_reset_event *strrst;
#endif
	int asocDown;
	char *str;
	char buf[256];
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	asocDown = 0;
	snp = (union sctp_notification *)notify_buf;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {

		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			asocDown = 1;
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			asocDown = 1;
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CANT START ASSOC";
			asocDown = 1;
			break;
		default:
			str = "UNKNOWN";
		} /* end switch(sac->sac_state) */
		printf("SCTP_ASSOC_CHANGE: %s, sac_error=0x%x assoc=0x%x\n",
		       str,
		       (uint32_t)sac->sac_error,
		       (uint32_t)sac->sac_assoc_id);
		break;
	case SCTP_PEER_ADDR_CHANGE:
		spc = &snp->sn_paddr_change;
		switch(spc->spc_state) {
		case SCTP_ADDR_AVAILABLE:
			str = "ADDRESS AVAILABLE";
			break;
		case SCTP_ADDR_UNREACHABLE:
			str = "ADDRESS UNAVAILABLE";
			break;
		case SCTP_ADDR_REMOVED:
			str = "ADDRESS REMOVED";
			break;
		case SCTP_ADDR_ADDED:
			str = "ADDRESS ADDED";
			break;
		case SCTP_ADDR_MADE_PRIM:
			str = "ADDRESS MADE PRIMARY";
			break;
#if defined(__BSD_SCTP_STACK__)
		case SCTP_ADDR_CONFIRMED:
			str = "ADDRESS CONFIRMED";
			break;
#endif
		default:
			str = "UNKNOWN";
		} /* end switch */
		sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
		if (sin6->sin6_family == AF_INET6) {
			char scope_str[16];
			snprintf(scope_str, sizeof(scope_str)-1, " scope %u",
				 sin6->sin6_scope_id);
			inet_ntop(AF_INET6, (char*)&sin6->sin6_addr, buf, sizeof(buf));
			strcat(buf, scope_str);
		} else {
			sin = (struct sockaddr_in *)&spc->spc_aaddr;
			inet_ntop(AF_INET, (char*)&sin->sin_addr, buf, sizeof(buf));
		}
		printf("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=0x%x\n",
		       str, buf, (uint32_t)spc->spc_assoc_id);
		break;
	case SCTP_REMOTE_ERROR:
		sre = &snp->sn_remote_error;
		printf("SCTP_REMOTE_ERROR: assoc=0x%x\n",
		       (uint32_t)sre->sre_assoc_id);
		break;

#if defined(__BSD_SCTP_STACK__)
	case SCTP_AUTHENTICATION_EVENT:
	{
		auth = (struct sctp_authkey_event *)&snp->sn_auth_event;
		printf("SCTP_AUTHKEY_EVENT: assoc=0x%x - ",
		       (uint32_t)auth->auth_assoc_id);
		switch(auth->auth_indication) {
		case SCTP_AUTH_NEWKEY:
			printf("AUTH_NEWKEY");
			break;
		case SCTP_AUTH_NO_AUTH:
			printf("AUTH_NO_AUTH");
			break;
		case SCTP_AUTH_FREE_KEY:
			printf("AUTH_FREE_KEY");
			break;
		default:
			printf("Indication 0x%x", auth->auth_indication);
		}
		printf(" key %u, alt_key %u\n", auth->auth_keynumber,
		       auth->auth_altkeynumber);
		break;
	}
	case SCTP_SENDER_DRY_EVENT:
	  break;
	case SCTP_STREAM_RESET_EVENT:
	{
		int len;
		char *strscope="unknown";
		strrst = (struct sctp_stream_reset_event *)&snp->sn_strreset_event;
		printf("SCTP_STREAM_RESET_EVENT: assoc=0x%x\n",
		       (uint32_t)strrst->strreset_assoc_id);
		if(strrst->strreset_flags & SCTP_STRRESET_FAILED) {
			printf("Failed\n");
			break;
		}
		if (strrst->strreset_flags & SCTP_STRRESET_INBOUND_STR) {
			strscope = "inbound";
		} else if (strrst->strreset_flags & SCTP_STRRESET_OUTBOUND_STR) {
			strscope = "outbound";
		}
		if (strrst->strreset_flags & SCTP_STRRESET_ADD_STREAM) {
		  
		  printf("Added streams %s new stream total is:%d\n",
				 strscope,
				 strrst->strreset_list[0]
				 );
		  break;
		}
		if(strrst->strreset_flags & SCTP_STRRESET_ALL_STREAMS) {
			printf("All %s streams have been reset\n",
			       strscope);
		} else {
			int i,cnt=0;
			len = ((strrst->strreset_length - sizeof(struct sctp_stream_reset_event))/sizeof(uint16_t));
			printf("Streams ");
			for ( i=0; i<len; i++){
				cnt++;
				printf("%d",strrst->strreset_list[i]);
				if ((cnt % 16) == 0) {
					printf("\n");
				} else {
					printf(",");
				}
			}
			if((cnt % 16) == 0) {
				/* just put out a cr */
				printf("Have been reset %s\n",strscope);
			} else {
				printf(" have been reset %s\n",strscope);
			}
		}
	}
	break;
#endif
	case SCTP_SEND_FAILED:
	{
		char *msg;
		static char msgbuf[200];
		ssf = &snp->sn_send_failed;
		if(ssf->ssf_flags == SCTP_DATA_UNSENT)
			msg = "data unsent";
		else if(ssf->ssf_flags == SCTP_DATA_SENT)
			msg = "data sent";
		else{
			sprintf(msgbuf,"unknown flags:%d",ssf->ssf_flags);
			msg = msgbuf;
		}
		printf("SCTP_SEND_FAILED: assoc=0x%x flag indicate:%s\n",
		       (uint32_t)ssf->ssf_assoc_id,msg);

	}

		break;
	case SCTP_ADAPTION_INDICATION:
	  {
	    struct sctp_adaption_event *ae;
	    ae = &snp->sn_adaption_event;
	    printf("SCTP_ADAPTION_INDICATION: assoc=0x%x - indication:0x%x\n",
		   (uint32_t)ae->sai_assoc_id, (uint32_t)ae->sai_adaption_ind);
	  }
	  break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	  {
	    struct sctp_pdapi_event *pdapi;
	    messageEnvolope msgout;	    

	    pdapi = &snp->sn_pdapi_event;
	    printf("SCTP_PD-API event:%u\n",
		   pdapi->pdapi_indication);
	    if(pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED){
	      printf("Sending up a truncated message\n");
		/* Ok we can deliver this guy now */
		if(o_info.sinfo_assoc_id){
		  msgout.protocolId = o_info.sinfo_ppid;
		  lastStream = msgout.streamNo = o_info.sinfo_stream;
		  lastStreamSeq = msgout.streamSeq = o_info.sinfo_ssn;
		}else{
		  printf("Gak! no info set (in pdapi)?\n");
		}
	      msgout.takeOk = 1;
	      msgout.totSize = dtsize;
	      msgout.totData = dataout;
	      msgout.siz = dtsize;
	      msgout.data = dataout;
	      /* We lie here */
	      msgout.from = (void *)to_ip;
	      msgout.type = PROTOCOL_Sctp;
	      msgout.to = NULL;
	      msgout.origFrom = NULL;
	      msgout.origType =  PROTOCOL_Unknown;
	      msgout.sender = (void *)&mainFd;
	      if(big_o)
		dist_sendmessage(big_o,&msgout);
	      else{
		free(dataout);
		printf("big_o not set?\n");
	      }
	      dataout = NULL;
	      dtsize = 0;
	    }
	  }
	  break;

	case SCTP_SHUTDOWN_EVENT:
                sse = &snp->sn_shutdown_event;
		printf("SCTP_SHUTDOWN_EVENT: assoc=0x%x\n",
		       (uint32_t)sse->sse_assoc_id);
		break;
	default:
		printf("Unknown notification event type=0x%x\n", 
		       snp->sn_header.sn_type);
	} /* end switch(snp->sn_header.sn_type) */
	if(continualINIT && asocDown){
	  int ret;
	  struct sockaddr *sa;

	  continualINIT--;
	  if(continualINIT < 0)
	    continualINIT = 0;
	  sa = (struct sockaddr *)to_ip;
	  ret = connect(fd, sa, to_ip_len);	  
	  printf("Continual INIT spawns a connect %d left ret:%d err:%d\n",
		 continualINIT,ret,errno);
	}
}

extern void SCTPPrintAnAddress(struct sockaddr *a);
int sctp_verbose = 0;

int
sctpReadInput(int fd, distributor *o,sctpAdaptorMod *r)
{
  /* receive some number of datagrams and act on them. */
  struct sctp_sndrcvinfo *s_info;
  socklen_t from_len;
  int sz,i,disped,ll;
  messageEnvolope msgout;
  struct msghdr msg;
  struct iovec iov[2];
  unsigned char from[200];
  char readBuffer[65535];
  char controlVector[65535];

  disped = i = 0;

  from_len = sizeof(from);
  if((fd == mainFd) && 
     (r->model & SCTP_TCP_TYPE) &&
     (r->model & SCTP_TCP_IS_LISTENING)){
    /* are we using TCP model here, maybe */
    int newfd;
    newfd = accept(fd, (struct sockaddr *)from, &from_len);
    if(newfd != -1){
      printf("New fd accepted fd=%d from:",newfd);
      SCTPPrintAnAddress((struct sockaddr *)from);
      dist_addFd(o,newfd,sctpFdInput,POLLIN,(void *)r);
      printf("Added new fd, now returning\n");
      return(0);
    }else{
      printf("Accept failed err:%d\n",errno);
      printf("Fall through and let read fail (hopefully :>)\n");
    }
  }
  s_info = NULL;
  iov[0].iov_base = readBuffer;
  ll = iov[0].iov_len = sizeof(readBuffer);
  iov[1].iov_base = NULL;
  iov[1].iov_len = 0;
  msg.msg_name = (caddr_t)from;
  msg.msg_namelen = from_len;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = (caddr_t)controlVector;
  msg.msg_flags = 0;
  msg.msg_controllen = sizeof(controlVector);
  errno = 0;
  sz = sctp_recvmsg (fd, 
		     readBuffer, 
		     (size_t)ll,
		     (struct sockaddr *)&from,
		     &from_len,
		     &o_info,
		     &msg.msg_flags);
  s_info = &o_info;

  if(sz <= 0){
    if (sz == 0) {
      if (fd != mainFd) {
        printf("Connected fd:%d  gets 0/0 remove from dist\n",fd);
        dist_deleteFd(o,fd);
        return(0);
      } else {
	/* our mainFd went EOF on us... probably a 1-to-1 socket */
        printf("Main fd:%d got EOF... please quit and restart now...\n",fd);
	close(fd);
        dist_deleteFd(o,fd);
	return(0);
      }
    }
    if((errno != ENOBUFS) && (errno != EAGAIN))
      printf("Read returns %d errno:%d control len is %d msgflg:0x%x\n",
	     sz,errno,
	     msg.msg_controllen,msg.msg_flags);
    if(errno == EBADF){
      /* FD went bad */
      if(fd != mainFd){
	close(fd);
	printf("FD:%d now closes\n",fd);
      }
      printf("Bad FD removed\n");
      dist_deleteFd(o,fd);
    }
    if(msg.msg_flags & MSG_EOR){
      if(dataout != NULL)
	goto deliverIt;
    }
    return(0);
  }
  big_o = o;
  if (msg.msg_flags & MSG_NOTIFICATION) {
    handle_notification(fd,readBuffer);
    return(0);
  }

  msgout.takeOk = 0;
  msgout.protocolId = 0;
  lastStream = msgout.streamNo = 0;
  lastStreamSeq = msgout.streamSeq = 0;

  if((dataout == NULL) && (msg.msg_flags & MSG_EOR)){
    if(o_info.sinfo_assoc_id){
      msgout.protocolId = o_info.sinfo_ppid;
      lastStream = msgout.streamNo = o_info.sinfo_stream;
      lastStreamSeq = msgout.streamSeq = o_info.sinfo_ssn;
    }else{
      printf("Gak! no info set with full msg\n");
    }
    msgout.takeOk = 0;
    msgout.totSize = sz;
    msgout.totData = readBuffer;
    msgout.siz = sz;
    msgout.data = readBuffer;
    msgout.from = &from;
    msgout.type = PROTOCOL_Sctp;
    msgout.to = NULL;
    msgout.origFrom = NULL;
    msgout.origType =  PROTOCOL_Unknown;
    msgout.sender = (void *)&mainFd;
    dist_sendmessage(o,&msgout);
  }else{
    char *newout;
    if(sctp_verbose) {
	    printf("PARTIAL DELIVERY OF %d bytes (dtsize:%d)\n",
		   sz,dtsize);
    }
    newout = calloc(1,(sz + dtsize));
    if(newout == NULL){
      printf("Gak! I am out of memory? .. drop a message\n");
      if(dataout)
	free(dataout);
      dataout = NULL;
      dtsize = 0;
      return(0);
    }
    if(dataout){
      memcpy(newout,dataout,dtsize);
    }
    memcpy(&newout[dtsize],readBuffer,sz);
    dtsize += sz;
    if(dataout){
      free(dataout);
    }
    dataout = newout;
  deliverIt:
    if(msg.msg_flags & MSG_EOR){
      /* Ok we can deliver this guy now */
      if(o_info.sinfo_assoc_id){
	msgout.protocolId = o_info.sinfo_ppid;
	lastStream = msgout.streamNo = o_info.sinfo_stream;
	lastStreamSeq = msgout.streamSeq = o_info.sinfo_ssn;
      }else{
	printf("Gak! no info set with msg delivery?\n");
      }
      msgout.takeOk = 1;
      msgout.totSize = dtsize;
      msgout.totData = dataout;
      msgout.siz = dtsize;
      msgout.data = dataout;
      msgout.from = &from;
      msgout.type = PROTOCOL_Sctp;
      msgout.to = NULL;
      msgout.origFrom = NULL;
      msgout.origType =  PROTOCOL_Unknown;
      msgout.sender = (void *)&mainFd;
      dist_sendmessage(o,&msgout);
      dataout = NULL;
      dtsize = 0;
    }
  }
  return(0);
}

int
sctpFdInput(void *arg,int fd ,int event)
{
  int ret;
  sctpAdaptorMod *m;
  m = (sctpAdaptorMod *)arg;
  /* setin the object in session */
  object_in = m;
  ret = sctpReadInput(fd, m->o, m);
  return(ret);
}


int
SCTP_setContinualInit(int value)
{
  continualINIT = value;
  return(value);
}

int
SCTP_getContinualInit(void)
{
  return(continualINIT);
}

/* A protocol-independent substitute to SCTP_setIPaddr(), SCTP_setIPaddr6(),
 * SCTP_setport().
 */
int
SCTP_setIPaddrinfo(struct addrinfo *res)
{
    if (res->ai_family != AF_INET && res->ai_family != AF_INET6) {
        printf("protocol family unsupported\n");
        return 1;
    }
    memcpy(to_ip, res->ai_addr, res->ai_addrlen);
    to_ip_len = res->ai_addrlen;
    return 0;
}

void
SCTP_setIPaddr(uint32_t addr)
{
  struct sockaddr_in *to;
  to = (struct sockaddr_in *)to_ip;
  to->sin_family = AF_INET;
#ifdef HAVE_SA_LEN
  to->sin_len = sizeof(struct sockaddr_in);
#endif
  to_ip_len = sizeof(struct sockaddr_in);
  to->sin_addr.s_addr = addr;
}

void
SCTP_setIPaddr6(uint8_t *addr)
{
  struct sockaddr_in6 *to;
  to = (struct sockaddr_in6 *)to_ip;
  to->sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
  to->sin6_len = sizeof(struct sockaddr_in6);
#endif
  to_ip_len = sizeof(struct sockaddr_in6);
  memcpy((char *)&to->sin6_addr,addr,sizeof(struct in6_addr));
}

void
SCTP_setIPv6scope(uint32_t scope)
{
  struct sockaddr_in6 *to;
  to = (struct sockaddr_in6 *)to_ip;
  to->sin6_scope_id = scope;
  scope_id = scope;
}

void
SCTP_setBindAddr(uint32_t addr)
{
  struct sockaddr_in *sin;
  sin = (struct sockaddr_in *)&bind_ss[bindSpecific];
  sin->sin_family = AF_INET;
#ifdef HAVE_SA_LEN
  sin->sin_len = sizeof(struct sockaddr_in);
#endif
  sin->sin_addr.s_addr = addr;
  bindSpecific++;
}

void
SCTP_setBind6Addr(uint8_t *addr)
{
  struct sockaddr_in6 *sin6;
  sin6 = (struct sockaddr_in6 *)&bind_ss[bindSpecific];
  sin6->sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
  sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
  memcpy((char *)&sin6->sin6_addr, addr, sizeof(struct in6_addr));
  bindSpecific++;
}

/* XXX works only for IPv4
 * prt must be in network byte order
 */
void
SCTP_setport(uint16_t prt)
{
  struct sockaddr_in *to;
  to = (struct sockaddr_in *)to_ip;
  to->sin_port = prt;
}

void
SCTP_setV4only(void) {
	v4only = 1;
}

void
SCTP_setV6only(void) {
	v6only = 1;
}

struct sockaddr *
SCTP_getAddr(socklen_t *len)
{
    if (len != NULL)
	*len = to_ip_len;
    return ((struct sockaddr *)to_ip);
}

uint16_t SCTP_getport()
{
  return(((struct sockaddr_in *)to_ip)->sin_port);
}

int
SCTP_getBind(void)
{
  return (bindSpecific);
}

void
SCTPnotify(int event, char *data, int size)
{
}


sctpAdaptorMod *
create_SCTP_adaptor(distributor *o,uint16_t port, int model, int rwnd , int swnd )
{
	sctpAdaptorMod *r;
	socklen_t length;
	struct sockaddr_in6 inAddr6,myAddr6;
	struct sctp_event_subscribe event;
	int optval;
	socklen_t optlen;
	int bindsa_len;

	memset(&inAddr6,0,sizeof(inAddr6));
	memset(&myAddr6,0,sizeof(myAddr6));
	inAddr6.sin6_port = htons(port);
	myAddr6.sin6_port = 0;
	inAddr6.sin6_family = AF_INET6;
	myAddr6.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
	inAddr6.sin6_len = sizeof(struct sockaddr_in6);
	myAddr6.sin6_len = sizeof(struct sockaddr_in6);
#endif

	memset(to_ip,0,sizeof(to_ip));
	r = calloc(1,sizeof(sctpAdaptorMod));
	if(r == NULL)
		return(r);
	r->o = o;
	if(model & SCTP_UDP_TYPE){
		if (v4only) {
			mainFd = r->fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
		} else {
			mainFd = r->fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
		}
	}else{
		if (v4only) {
			mainFd = r->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
		} else {
			mainFd = r->fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
		}
	}
	if(r->fd < 0){
		printf("errno:%d\n", errno);
		free(r);
		return(NULL);
	}
	optval = 1;
	errno = 0;
	/*  ret = ioctl(r->fd,FIONBIO,&optval);
	    printf("ret from FIONBIO ioctl is %d err:%d\n",
	    ret,errno);
	*/
	r->model = model & (SCTP_UDP_TYPE|SCTP_TCP_TYPE);
	if (v6only) {
		/* set this to v6 only... */
		optval = 1;
		if (setsockopt(r->fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optval,
			       sizeof(optval)) == -1) {
			close(r->fd);
			free(r);
			return(NULL);
		}
	}
	/* fill in specific local server address if doing bind specific */
	if (bindSpecific) {
		int i, ret;
		for (i=0; i<bindSpecific; i++) {
			struct sockaddr *sa = (struct sockaddr *)&bind_ss[i];
			/* copy bind address in */
			memcpy(&inAddr6, &bind_ss[i], sizeof(struct sockaddr_in6));
			/* set desired port */
			if (sa->sa_family == AF_INET) {
				struct sockaddr_in *sin = (struct sockaddr_in *)&inAddr6;
				sin->sin_port = htons(port); 
				bindsa_len = sizeof(struct sockaddr_in);
			} else {
				inAddr6.sin6_port = htons(port);
				bindsa_len = sizeof(struct sockaddr_in6);
				inAddr6.sin6_scope_id = scope_id;
			}
			errno = 0;
			ret = sctp_bindx(r->fd, 
							 (struct sockaddr *)&inAddr6, 1, 
							 SCTP_BINDX_ADD_ADDR);
		}
	} else {
		if (v4only) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&inAddr6;
			memset(sin, 0, sizeof(*sin));
			sin->sin_family = AF_INET;
#ifdef HAVE_SA_LEN
			sin->sin_len = sizeof(struct sockaddr_in);
#endif
			sin->sin_port = htons(port);
			bindsa_len = sizeof(struct sockaddr_in);
		} else {
			inAddr6.sin6_port = htons(port);
			bindsa_len = sizeof(struct sockaddr_in6);
			inAddr6.sin6_scope_id = scope_id;
		}
	}
	/* enable all event notifications */
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_address_event = 1;
	event.sctp_send_failure_event = 1;
	event.sctp_peer_error_event = 1;
	event.sctp_shutdown_event = 1;
	event.sctp_partial_delivery_event = 1;
#if defined(__BSD_SCTP_STACK__)
	event.sctp_adaptation_layer_event = 1;
#else
	event.sctp_adaption_layer_event = 1;
#endif
#if defined(__BSD_SCTP_STACK__)
	event.sctp_authentication_event = 1;
	event.sctp_sender_dry_event = 1;
	event.sctp_stream_reset_event = 1;
#endif

	if (setsockopt(r->fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
	}
	optlen = 4;
	if(swnd) {
		optval = swnd;
		if(setsockopt(r->fd, SOL_SOCKET, SO_SNDBUF, &optval, optlen) != 0){
			printf("err:%d could not set sndbuf\n",errno);
		}
	}
	if (rwnd){
		optval = rwnd;
		if(setsockopt(r->fd, SOL_SOCKET, SO_RCVBUF, &optval, optlen) != 0){
			printf("err:%d could not set rcvbuf\n",errno);
		}
	}
	optval = 0;
	if(getsockopt(r->fd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen) != 0)
		printf("err:%d could not read sndbuf\n",errno);
	else
		printf("snd buffer is %d\n",optval);

	optval = 0;
	if(getsockopt(r->fd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen) != 0)
		printf("err:%d could not read rcvbuf\n",errno);
	else
		printf("rcv buffer is %d\n",optval);

	if ((port) && (bindSpecific == 0)) {
		if(bind(r->fd,(struct sockaddr *)&inAddr6, bindsa_len) < 0){
			printf("bind failed err:%d\n",errno);
			close(r->fd);
			free(r);
			return(NULL);
		}
	}

	if(model & SCTP_UDP_TYPE){
		printf("Calling listen for one-to-many model\n");
		listen(r->fd,1);
#ifdef __APPLE__
		{
			int opt = 0;
			/* fix Apple listen() issue */
			setsockopt(r->fd, IPPROTO_SCTP, SCTP_LISTEN_FIX, &opt, sizeof(opt));
		}
#endif
	}
	if(port){
		length = sizeof(myAddr6);
		if(getsockname(r->fd, (struct sockaddr *)&myAddr6, &length) < 0){
			printf("get sockname failed err:%d\n",errno);
			close(r->fd);
			free(r);
			return(NULL);
		}	

		if(port != ntohs(myAddr6.sin6_port)){
			printf("Can't get my port:%d got %d\n",port,ntohs(myAddr6.sin6_port));
			close(r->fd);
			free(r);
			return(NULL);
		}
	}
	dist_addFd(o,r->fd,sctpFdInput,POLLIN,(void *)r);
	object_in = r;
	return(r);
}

void
destroy_SCTP_adaptor(sctpAdaptorMod *r)
{
  dist_deleteFd(r->o,r->fd);
  close(r->fd);
  free(r);
  object_in = NULL;
  r = NULL;
}

void
restore_SCTP_fd(sctpAdaptorMod *r)
{
  if(r->fd != mainFd){
    printf("Restoring fd from %d to main\n",r->fd);
    r->fd = mainFd;
  }
}

void
SCTP_setcurrent(sctpAdaptorMod *r)
{
  object_in = r;
}
