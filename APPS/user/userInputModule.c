/*	$Header: /usr/sctpCVS/APPS/user/userInputModule.c,v 1.1.1.1 2004-06-23 13:07:29 randall Exp $ */

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
/* Parser for user commands.
 *
 * Support for command completion and history added by
 * Marco Molteni <mmolteni@cisco.com>.
 */

#include <userInputModule.h>

#include <distributor.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <poll.h>
#include <sys/uio.h>


/* These should not be included except for our test
 * module to generate TLV's to dump in the REL-REQ
 */
/* Line editing. */
#include <readline/readline.h>
#include <readline/history.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
static int execute_line(const char *line);
static char *command_generator(char *text, int state);

int loop_sleep = 0;

/* Line editing.
 * Each command must have its own function, with the prefix "cmd_".
 */
static int cmd_setloopsleep(char *argv[], int argc);
static int cmd_getloopsleep(char *argv[], int argc);
static int cmd_abort(char *argv[], int argc);
static int cmd_addip(char *argv[], int argc);
static int cmd_assoc(char *argv[], int argc);
static int cmd_bindx(char *argv[], int argc);
static int cmd_bulk(char *argv[], int argc);
static int cmd_bulkstat(char *argv[], int argc);
static int cmd_chgcookielife(char *argv[], int argc);
static int cmd_xconnect(char *argv[], int argc);
static int cmd_continual(char *argv[], int argc);
static int cmd_cwndlog(char *argv[], int argc);
static int cmd_clrcwndlog(char *argv[], int argc);
static int cmd_defretryi(char *argv[], int argc);
static int cmd_defretrys(char *argv[], int argc);
static int cmd_defrwnd(char *argv[], int argc);
static int cmd_delip(char *argv[], int argc);
static int cmd_doheartbeat(char *argv[], int argc);
static int cmd_getcurcookielife(char *argv[], int argc);
static int cmd_getcwndpost(char *argv[], int argc);
static int cmd_getdefcookielife(char *argv[], int argc);
static int cmd_gethbdelay(char *argv[], int argc);
static int cmd_getpegs(char *argv[], int argc);
static int cmd_clrpegs(char *argv[], int argc);

static int cmd_getprimary(char *argv[], int argc);
static int cmd_getrtt(char *argv[], int argc);
static int cmd_getsnd(char *argv[], int argc);
static int cmd_heart(char *argv[], int argc);
static int cmd_heartdelay(char *argv[], int argc);
static int cmd_help(char *argv[], int argc);
static int cmd_hulkstart(char *argv[], int argc);
static int cmd_hulkstop(char *argv[], int argc);
static int cmd_bulkstop(char *argv[], int argc);
static int cmd_initmultping(char *argv[], int argc);
static int cmd_inqueue(char *argv[], int argc);
static int cmd_multping(char *argv[], int argc);
static int cmd_netstats(char *argv[], int argc);
static int cmd_ping(char *argv[], int argc);
static int cmd_quit(char *argv[], int argc);
static int cmd_rftp(char *argv[], int argc);
static int cmd_printrftpstat(char *argv[], int argc);
static int cmd_rwnd(char *argv[], int argc);
static int cmd_send(char *argv[], int argc);
static int cmd_getmaxburst(char *argv[], int argc);
static int cmd_setmaxburst(char *argv[], int argc);
static int cmd_sendloop(char *argv[], int argc);
static int cmd_sendloopend(char *argv[], int argc);
static int cmd_sendreltlv(char *argv[], int argc);
static int cmd_setbulkmode(char *argv[], int argc);
static int cmd_setdefcookielife(char *argv[], int argc);
static int cmd_setdefstream(char *argv[], int argc);
static int cmd_getfragmentation(char *argv[], int argc);
static int cmd_setfragmentation(char *argv[], int argc);
static int cmd_getautoclose(char *argv[], int argc);
static int cmd_setautoclose(char *argv[], int argc);
static int cmd_seterr(char *argv[], int argc);
static int cmd_sethost(char *argv[], int argc);
static int cmd_setinittsn(char *argv[], int argc);
static int cmd_getinittsn(char *argv[], int argc);
static int cmd_sethost6(char *argv[], int argc);
static int cmd_setneterr(char *argv[], int argc);
static int cmd_setopts(char *argv[], int argc);
static int cmd_setpay(char *argv[], int argc);
static int cmd_setport(char *argv[], int argc);
static int cmd_setprimary(char *argv[], int argc);
static int cmd_setautoasconf(char *argv[], int argc);
static int cmd_getautoasconf(char *argv[], int argc);
static int cmd_setremprimary(char *argv[], int argc);
static int cmd_setscope(char *argv[], int argc);
static int cmd_setstreams(char *argv[], int argc);
static int cmd_startmultping(char *argv[], int argc);
static int cmd_setadaption(char *argv[], int argc);
static int cmd_setblocking(char *argv[], int argc);
static int cmd_getadaption(char *argv[], int argc);
static int cmd_tella(char *argv[], int argc);
static int cmd_term(char *argv[], int argc);
static int cmd_whereto(char *argv[], int argc);
static int cmd_getpcbinfo(char *argv[], int argc);
static int cmd_getrtomin(char *argv[], int argc);
static int cmd_setrtomin(char *argv[], int argc);
static int cmd_getinitparam(char *argv[], int argc);
static int cmd_setinitparam(char *argv[], int argc);
static int cmd_gettimetolive(char *argv[], int argc);
static int cmd_settimetolive(char *argv[], int argc);
static int cmd_getlocaladdrs(char *argv[], int argc);
static int cmd_restorefd(char *argv[], int argc);
static int cmd_listen(char *argv[], int argc);
static int cmd_setfd(char *argv[], int argc);
static int cmd_getevents(char *argv[], int argc);
static int cmd_getstatus(char *argv[], int argc);
static int cmd_closefd(char *argv[], int argc);
static int cmd_silent(char *argv[], int argc);
static int cmd_silentcnt(char *argv[], int argc);
static int cmd_streamreset(char *argv[], int argc);
static int cmd_deliverydump(char *argv[], int argc);
static int cmd_sendsent(char *argv[], int argc);
static int cmd_getroute(char *argv[], int argc);

static int cmd_getmaxseg(char *argv[], int argc);
static int cmd_setmaxseg(char *argv[], int argc);

static int cmd_getnodelay(char *argv[], int argc);
static int cmd_setnodelay(char *argv[], int argc);

static int cmd_getmsdelay(char *argv[], int argc);
static int cmd_setmsdelay(char *argv[], int argc);

static int cmd_setdebug(char *argv[], int argc);
#ifndef SCTP_NO_TCP_MODEL
static int cmd_peeloff(char *argv[], int argc);
#endif /* !SCTP_NO_TCP_MODEL */

/* Line editing. */
typedef int f2(char **, int);

struct command {
    char *co_name;      /* Command name. */
    char *co_desc;      /* Command description. */
    f2 *co_func;        /* Function to call to execute the command. */
};
static int msdelaymode = 0;

static int silent_mode = 0;
static unsigned int silent_count = 0;
static time_t time_started;
/* Line editing.
 * Store the commands and the help descriptions.
 * Please keep this list sorted in alphabetical order.
 */
static struct command commands[] = {
    {"abort", "abort - abort the existing association",
     cmd_abort},
    {"addip", "addip address how - add ip address where how is the mask/action to pass\n"
     "                    SCTP_ACTION_UPDATE_ALL_ASSOC=0x1\n"
     "                    SCTP_ACTION_UPDATE_ENDPOINT=0x02\n"
     "                    SCTP_ACTION_UPDATE_ONLY_ASSOC=0x04\n"
     "                    SCTP_ACTION_QUEUE_REQUEST_ONLY=0x08\n"
     "                    (which can all be or'd together if you want)",
     cmd_addip},
    {"assoc", "assoc - associate with the set destination",
     cmd_assoc},
    {"bindx", "bindx type address - add/delete address to the endpoint\n"
     "               type: 0=add, 1=delete\n",
     cmd_bindx},
    {"bulk", "bulk size stream count - send a bulk of messages",
     cmd_bulk},
    {"bulkstat", "bulkstat - display count of bulk packets and sent (seen resets)",
     cmd_bulkstat},
    {"bulkstop", "bulkstop - stop the bulk transfer",
     cmd_bulkstop},

    {"chgcookielife", "chgcookielife val - change the current assoc cookieLife",
     cmd_chgcookielife},
    {"closefd", "closefd fd# - close the tcp connected fd",
     cmd_closefd},
    {"connectx", "connectx addr [addr's] - associate to the list of addresses\n",
     cmd_xconnect},

    {"continual", "continual num - set continuous init to num times", 
     cmd_continual},
    {"cwndlog", "cwndlog - get cwnd log", 
     cmd_cwndlog},
    {"clrcwndlog", "clrcwndlog - clear cwnd log", 
     cmd_clrcwndlog},
    {"defretryi", "defretryi num - set a new association failure threshold for initing", 
     cmd_defretryi},
    {"defretrys", "defretrys num - set a new association failure threshold for sending", 
     cmd_defretrys},
    {"defrwnd", "defrwnd num - set the default rwnd of the SCTP", 
     cmd_defrwnd},
    {"delip", "delip address [how] - delete ip address where how is the mask/action to pass", 
     cmd_delip},
    {"doheartbeat", "doheartbeat - preform a on demand HB", 
     cmd_doheartbeat},
    {"getautoasconf", "getautoasconf current auto-asconf setting\n",
     cmd_getautoasconf},
    {"getautoclose", "getautoclose - get autoclose value",
     cmd_getautoclose},
    {"getcurcookielife", "getcurcookielife - display current assoc cookieLife",
     cmd_getcurcookielife},
    {"getcwndpost", "getcwndpost - display cwnd post information collected",
     cmd_getcwndpost},
    {"getdefcookielife", "getdefcookielife - display default cookie life",
     cmd_getdefcookielife},
    {"getevents", "getevents - display the event registration status",
     cmd_getevents},
    {"getstatus", "getstatus - display the associaiton status",
     cmd_getstatus},
    {"heartctl", "heartctl on/off/allon/alloff - Turn HB on or off to the destination or all dests",
     cmd_heart},
    {"gethbdelay", "gethbdelay [ep] - get the hb delay",
     cmd_gethbdelay},
    {"getinitparam", "getinitparam - get the default INIT parameters",
     cmd_getinitparam},
    {"getinittsn", "getinittsn - get the association initial TSN",
    cmd_getinittsn},
    {"getlocaladdrs", "getlocaladdrs - get the local addresses for this assoc/ep",
       cmd_getlocaladdrs},
    {"getloopsleep", "getloopsleep - Displays the sleep time between each loopreq before the send of loop-resp",
       cmd_getloopsleep},

    {"getmaxburst", "getmaxburst - retrieve the maxburst setting",
     cmd_getmaxburst},
    {"getmaxseg", "getmaxseg - retrieve the maxseg size",
     cmd_getmaxseg},
    {"getmsdelay", "getmsdelay - get the delayed send mode",
     cmd_getmsdelay},
    {"getnodelay", "getnodelay - get the setting of the nagle algorithm off or on",
     cmd_getnodelay},

    {"getpegs", "getpegs - retrieve the peg counts",
     cmd_getpegs},
    {"clrpegs", "clrpegs - clear the peg counts",
     cmd_clrpegs},
    {"getpcbinfo", "getpcbinfo- retrieve the pcb counts",
     cmd_getpcbinfo},
    {"getprimary", "getprimary - tells which net number is primary",
     cmd_getprimary},
    {"getroute", "getroute [4|6|N] address - getaroute",
     cmd_getroute},
    {"getrtt", "getrtt - Return the RTO of the current default address of the assoc",
     cmd_getrtt},
    {"getrtomin", "getrtomin - Return the RTO MIN/MAX for the endpoint",
     cmd_getrtomin},

    {"getsnd", "getsnd <asocid> - get the socketbuffer info for a asoc",
     cmd_getsnd},

    {"gettimetolive", "gettimetolive - get the number of microseconds messages live",
     cmd_gettimetolive},

    {"help", "help [cmd] - display help for cmd, or for all the commands if no cmd specified",
     cmd_help},
    {"hulkstart", "hulkstart filename - start the hulk hogan process",
     cmd_hulkstart},
    {"hulkstop", "hulkstop - stop the hulk hogan process",
     cmd_hulkstop},
    {"initmultping", "initmultping - init contexts for a fast test",
     cmd_initmultping},
    {"inqueue", "inqueue - report inqueue counts",
     cmd_inqueue},
    {"listen", "listen backlog - executes a listen",
     cmd_listen},
    {"multping", "multping size stream count blockafter sec - add a ping pong context and block\n"
     "after <blockafter> instances for <sec> seconds",
     cmd_multping},
    {"netstats", "netstats - return all network stats",
     cmd_netstats},
#ifndef SCTP_NO_TCP_MODEL
    {"peeloff", "peeloff - peel off the current to assoc",
     cmd_peeloff},
#endif /* !SCTP_NO_TCP_MODEL */
    {"ping", "ping size stream count - play ping pong",
     cmd_ping},
    {"quit", "quit - quit the program",
     cmd_quit},
    {"rftp", "rftp filename strm1 strm2 blocksz [n] - round-trip ftp",
     cmd_rftp},

    {"rftp_stat", "rftp_stat - get stream status on read side of rftp and clear",
     cmd_printrftpstat},
    {"rwnd", "rwnd - get rwnds",
     cmd_rwnd},
    {"restorefd", "restorefd - restores the fd to the base for the TCP model",
     cmd_restorefd},
    {"send", "send string [n] - send string to a peer if a peer is set [and retrans n times]",
     cmd_send},
    {"sendloop", "sendloop [num] - send test script loopback request of num size",
     cmd_sendloop},
    {"sendloopend", "sendloopend [num] - send test script loopback request of num size and terminate",
     cmd_sendloopend},
    {"sendreltlv", "sendreltlv num - send a rel-tlv of num bytes of data",
     cmd_sendreltlv},
    {"setadaption", "setadaption bits - include a adaption tlv sending these bits",
     cmd_setadaption},
    {"setblocking", "setblocking on/off - turn on/off FIONBIO",
     cmd_setblocking},


    {"getadaption", "getadaption - get the adaption bits sent",
     cmd_getadaption},
    {"setfragmentation", "setfragmentation on/off - turn on/off fragmentation",
     cmd_setfragmentation},
    {"getfragmentation", "getfragmentation - get fragmentation state",
     cmd_getfragmentation},
    {"setautoclose", "setautoclose value - turn on/off autoclose time",
     cmd_setautoclose},
    {"setbulkmode", "setbulkmode ascii/binary - set bulk transfer mode",
     cmd_setbulkmode},
    {"setdefcookielife", "setdefcookielife num - set default cookie life",
     cmd_setdefcookielife},
    {"setdefstream", "setdefstream num - set the default stream to",
     cmd_setdefstream},
    {"seterr", "seterr num - set the association send error thresh",
     cmd_seterr},
    {"setfd", "setfd num - set the fd to num (tcp model only)",
     cmd_setfd},
    {"sethost", "sethost host|X.Y.Z.A - set the destination host IP address",
     cmd_sethost},
    {"sethost6", "sethost6 host|xx:xx:xx...:xx - set the destination host IPv6 address",
     cmd_sethost6},
    {"setinitparam", "setinitparam OS MIS maxinit initrtomax - set the association initial parameters",
    cmd_setinitparam},

    {"setinittsn", "setinittsn TSN - set the association initial TSN for debugging",
    cmd_setinittsn},

    {"setloopsleep", "setloopsleep val - Sets the sleep time between each loopreq before the send of loop-resp",
       cmd_setloopsleep},


    {"setmaxburst", "setmaxburst val - set the maxburst setting to val",
     cmd_setmaxburst},

    {"setmaxseg", "setmaxseg val - set the maxseg size",
     cmd_setmaxseg},
    
    {"setneterr", "setneterr net val - set the association network error thresh",
     cmd_setneterr},

    {"setmsdelay", "setmsdelay 0/1 - set the delayed send mode 0=off/1=on",
     cmd_setmsdelay},


    {"setnodelay", "setnodelay 0/1 - set no delay 1=on 0=off (nagle on = 0 nagle off = 1)",
     cmd_setnodelay},


    {"silentcnt", "silentcnt - get current rcv count in silent mode",
     cmd_silentcnt},

    {"silent", "silent - toggle silent running mode - toggle print on/off",
     cmd_silent},

    {"setautoasconf", "setautoasconf 0/1 - turn on/off auto asconf",
     cmd_setautoasconf},

     {"setdebug", "setdebug val - set sctp kernel debug level [off/all.. etc]",
     cmd_setdebug},

    {"setheartdelay", "setheartdelay time [ep]- Add number of seconds + RTO to hb interval",
     cmd_heartdelay},

    {"setopts", "setopts val - set options send options to value",
     cmd_setopts},
    {"setpay", "setpay payloadt - set the payload type",
     cmd_setpay},
    {"setport", "setport port - set the destination SCTP port number",
     cmd_setport},
    {"setprimary", "setprimary - set current ip address to the primary address",
     cmd_setprimary},
    {"setremprimary", "setremprimary address - set remote's primary address",
     cmd_setremprimary},
    {"setrtomin", "setrtomin rtoinitial rtomin rtomax - Sets rto initial/min/max for the endpoint",
     cmd_setrtomin},
    {"setscope", "setscope num - Set the IPv6 scope id",
     cmd_setscope},
    {"setstreams", "setstreams numpreopenstrms - set the number of streams I request",
     cmd_setstreams},
    {"settimetolive", "settimetolive mstolive - set the number of microseconds messages live",
     cmd_settimetolive},

    {"startmultping", "startmultping - start the defined ping pong contexts ",
     cmd_startmultping},

    {"streamreset", "streamreset [send|recv|both] [all || num num num] - reset streams ",
     cmd_streamreset},
    
    {"send_qdump", "send_qdump - Dump all send/t Q's ",
     cmd_sendsent },
    {"deliverydump", "deliverydump - Dump all delivery Q stats ",
     cmd_deliverydump},

    {"tella", "tella host - confess what translateIPAddress returns",
     cmd_tella},
    {"term", "term - terminate the set destination association (graceful shutdown)",
     cmd_term},
    {"whereto", "whereto - tell where the default sends",
     cmd_whereto},
    {NULL, NULL, NULL}
};
static sctpAdaptorMod *adap;

int payload=0;
int pingPongCount=0;
char pingBuffer[SCTP_MAX_READBUFFER];
int pingBufSize=0;
int pingStream=0;
int defStream = 0;

distributor *dist;

/* SCTP_PB */
static SPingPongContext pingPongTable[MAX_PING_PONG_CONTEXTS];
int pingPongsDefined=0;

int bulkCount=0;
int bulkBufSize=0;
int bulkStream=0;
int bulkSeen=0;
int time_to_live=0;

struct sctpdataToProduce{
  char string[8];
  struct timeval sent;
  struct timeval recvd;
  struct timeval stored;
  u_int seq;
};

static u_int curSeq=0;
static int period = 20000;
FILE *hulkfile=NULL;

FILE *rftp_in1=NULL, *rftp_in2=NULL;
FILE *rftp_out1=NULL, *rftp_out2=NULL;
int rftp_strm1, rftp_strm2, rftp_bsz, rftp_rt;
int rftp_ending1, rftp_ending2;

/* how many message to read out at one time, max */
#define LIMIT_READ_AT_ONCE 200

extern int mainnotDone;
extern int destinationSet;
extern int portSet;
extern int sendOptions;
extern int bulkInProgress;
extern int bulkPingMode;
extern int bindSpecific;
extern int scope_id;
  
/* wire test structures */
typedef struct {
    u_char  type;
    u_char  padding;
    u_short dgramLen;
    u_long  dgramID;
}testDgram_t;

#define SCTP_TEST_LOOPREQ   1
#define SCTP_TEST_LOOPRESP  2
#define SCTP_TEST_SIMPLE    3
#define SCTP_TEST_RFTP	    4
#define SCTP_TEST_RFTPRESP  5


char *namelist[]={
  "SCTP_PEG_SACKS_SEEN",
  "SCTP_PEG_SACKS_SENT",
  "SCTP_PEG_TSNS_SENT",
  "SCTP_PEG_TSNS_RCVD",
  "SCTP_DATAGRAMS_SENT",
  "SCTP_DATAGRAMS_RCVD",
  "SCTP_RETRANTSN_SENT",
  "SCTP_DUPTSN_RECVD",
  "SCTP_HBR_RECV",
  "SCTP_HBA_RECV",
  "SCTP_HB_SENT",
  "SCTP_DATA_DG_SENT",
  "SCTP_DATA_DG_RECV",
  "SCTP_TMIT_TIMER",
  "SCTP_RECV_TIMER",
  "SCTP_HB_TIMER",  
  "SCTP_FAST_RETRAN",
  "SCTP_TSNS_READ",
  "SCTP_NONE_LEFT_TOSEND",
  "SCTP_NONE_LEFT_RWND_GATE",
  "SCTP_NONE_LEFT_CWND_GATE",
  "SCTP_SEND_STREAM_0",
  "SCTP_SEND_STREAM_1",
  "SCTP_SEND_STREAM_2",
  "SCTP_SEND_STREAM_3",
  "SCTP_SEND_STREAM_OTHER",
  "SCTP_RECV_STREAM_0",
  "SCTP_RECV_STREAM_1",
  "SCTP_RECV_STREAM_2",
  "SCTP_RECV_STREAM_3",
  "SCTP_RECV_STREAM_OTHER",
  "UNKNOWN"
};



static sctp_assoc_t
get_assoc_id()
{
  struct sctp_paddrparams sp;
  int siz;
  struct sockaddr *sa;

  /* First get the assoc id */
  siz = sizeof(struct sctp_paddrparams);
  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
  errno = 0;
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
    printf("Failed to get assoc id with HB info err:%d\n",errno);
    return(0);
  }
  /* We depend on the fact that 0 can never be returned */
  return(sp.spp_assoc_id);
}



void
SCTPPrintAnAddress(struct sockaddr *a)
{
  char stringToPrint[256];
  u_short prt;
  char *srcaddr,*txt;
  if (a == NULL) {
	  printf("NULL\n");
	  return;
  }
  if(a->sa_family == AF_INET){
    srcaddr = (char *)&((struct sockaddr_in *)a)->sin_addr;
    txt = "IPv4 Address: ";
    prt = ntohs(((struct sockaddr_in *)a)->sin_port);
  }else if(a->sa_family == AF_INET6){
    srcaddr = (char *)&((struct sockaddr_in6 *)a)->sin6_addr;
    prt = ntohs(((struct sockaddr_in6 *)a)->sin6_port);
    txt = "IPv6 Address: ";
  }else if(a->sa_family == AF_LINK){
    int i;
    char tbuf[200];
    u_char adbuf[200];
    struct sockaddr_dl *dl;

    dl = (struct sockaddr_dl *)a;
    strncpy(tbuf,dl->sdl_data,dl->sdl_nlen);
    tbuf[dl->sdl_nlen] = 0;
    printf("Intf:%s (len:%d)Interface index:%d type:%x(%d) ll-len:%d ",
	   tbuf,
	   dl->sdl_nlen,
	   dl->sdl_index,
	   dl->sdl_type,
	   dl->sdl_type,
	   dl->sdl_alen
	   );
    memcpy(adbuf,LLADDR(dl),dl->sdl_alen);
    for(i=0;i<dl->sdl_alen;i++){
      printf("%2.2x",adbuf[i]);
      if(i<(dl->sdl_alen-1))
	printf(":");
    }
    printf("\n");
    /*	u_short	sdl_route[16];*/	/* source routing information */
    return;
  }else{
    return;
  }
  if(inet_ntop(a->sa_family,srcaddr,stringToPrint,sizeof(stringToPrint))){
	  if(a->sa_family == AF_INET6){
		  printf("%s%s:%d scope:%d\n",
		      txt,stringToPrint,prt,
		      ((struct sockaddr_in6 *)a)->sin6_scope_id);
	  }else{
		  printf("%s%s:%d\n",txt,stringToPrint,prt);
	  }

  }else{
    printf("%s unprintable?\n",txt);
  }
}



int
sctpSEND(int fd,int defStream,char *s_buff,int sndsz,
	 struct sockaddr *to,int options, int payload,int not_used)
{
  int sz;
  struct msghdr msg;
  struct iovec iov[2];
  char controlVector[256];
  struct sctp_sndrcvinfo *s_info;
  struct cmsghdr *cmsg;

  iov[0].iov_base = s_buff;
  iov[0].iov_len = sndsz;
  iov[1].iov_base = NULL;
  iov[1].iov_len = 0;
  msg.msg_name = (caddr_t)to;
  msg.msg_namelen = to->sa_len;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = (caddr_t)controlVector;
  
  cmsg = (struct cmsghdr *)controlVector;

  cmsg->cmsg_level = IPPROTO_SCTP;
  cmsg->cmsg_type = SCTP_SNDRCV;
#if defined(__FreeBSD__) 
  cmsg->cmsg_len = _ALIGN(sizeof(struct cmsghdr)) + sizeof(struct sctp_sndrcvinfo);
#elif defined(__APPLE__)
  cmsg->cmsg_len = ALIGN(sizeof(struct cmsghdr)) + sizeof(struct sctp_sndrcvinfo);
#else
  cmsg->cmsg_len = __CMSG_ALIGN(sizeof(struct cmsghdr)) + sizeof(struct sctp_sndrcvinfo);
#endif
  s_info = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);

  s_info->sinfo_stream = defStream;
  s_info->sinfo_ssn = 0;
  s_info->sinfo_flags = options;
  s_info->sinfo_ppid = payload;
  s_info->sinfo_context = 0;
  s_info->sinfo_assoc_id = 0;
  s_info->sinfo_timetolive = time_to_live;
  errno = 0;
  msg.msg_controllen = cmsg->cmsg_len;
  sz = sendmsg(fd,&msg,0);
  if((sz <=0) && (errno != ENOBUFS)){
    printf("Return from sendmsg is %d errno %d\n",
	   sz,errno);
  }
  return(sz);
}


int
sctpTERMINATE(int fd,struct sockaddr *to)
{
  char buf[16];
  return(sctpSEND(fd,0,buf,0,to,MSG_EOF,0,0));
}


/*--------------------------------------------------*/

void resetPingPongTable() 
{
  int i;
  for(i=0;i<MAX_PING_PONG_CONTEXTS;i++){
    pingPongTable[i].stream = -1;
    pingPongTable[i].nbRequests = 0;
    pingPongTable[i].counter = 0;
    pingPongTable[i].started = 0;
    pingPongTable[i].blockAfter = 0;
    pingPongTable[i].blockDuring = 0;
    pingPongTable[i].blockTime = 0;
  }
  pingPongsDefined = 0;
  return;
}

/*--------------------------------------------------*/

void initPingPongTable(int max)
{
  int i;
  
  for(i=0;i<max;i++){
    pingPongTable[i].stream = i;
    pingPongTable[i].nbRequests = 2000;
    pingPongTable[i].counter = 0;
    pingPongTable[i].started = 0;
    if(i == 2){
      pingPongTable[i].blockAfter = 100;
      pingPongTable[i].blockDuring = 3;
    }else{
      pingPongTable[i].blockAfter = 0;
      pingPongTable[i].blockDuring = 0;
    }
    pingPongsDefined++;
  } 
  return;
}


u_long translateIPAddress(char *host)
{

  struct sockaddr_in sa;
  struct hostent *hp;
  int len,cnt,i;

  sa.sin_addr.s_addr = htonl(inet_network(host));
  len = strlen(host);
  cnt = 0;
  for(i=0;i<len;i++){
    if(host[i] == '.')
      cnt++;
    else if(host[i] == ':')
      cnt++;
  }
  if(cnt < 3){
    /* make it fail since it can't be a . or : based address */
    sa.sin_addr.s_addr = 0xffffffff;
  }
  if(sa.sin_addr.s_addr == 0xffffffff){
    hp = gethostbyname(host);
    if(hp == NULL){
      return(htonl(strtoul(host,NULL,0)));
    }
    memcpy(&sa.sin_addr,hp->h_addr_list[0],sizeof(sa.sin_addr));
  }
  return(sa.sin_addr.s_addr);
}

void
fillPingBuffer(int fd,int len)
{
  u_int *pbp;
  int i,rlen;
  rlen = (len/4) + 1;
  pbp = (u_int *)&pingBuffer[4];
  for(i=0;i<rlen;i++){
    *pbp = (u_int)mrand48();
    pbp++;
  }
}

int
sendBulkTransfer(int fd,int sz)
{
  char buffer[10000];
  testDgram_t *tt;
  static unsigned int dgramCount=0;
  int sndsz,ret;

  if((sz+sizeof(testDgram_t)) > (sizeof(buffer))){
    sndsz = sizeof(buffer) -  sizeof(testDgram_t);
  }else{
    sndsz = sz +  sizeof(sizeof(testDgram_t));
  }
  memset(buffer,0,sndsz);
  tt = (testDgram_t *)buffer;
  tt->type = SCTP_TEST_SIMPLE;
  tt->dgramLen = sndsz - sizeof(testDgram_t);
  tt->dgramID = dgramCount++;
  
  ret = sctpSEND(fd,defStream,buffer,sndsz,SCTP_getAddr(),sendOptions,payload,0);
  return(ret);
}


void
checkBulkTranfer(void *v,void *xxx)
{	
  int fd;
  int ret,cnt;
  cnt = 0;
  ret = 0;
  fd = (int)v;

  while(bulkCount > 0){
    if(bulkPingMode == 0){
      fillPingBuffer(fd,(bulkBufSize-4));
      ret = sctpSEND(fd,bulkStream,pingBuffer,bulkBufSize,SCTP_getAddr(),sendOptions,0,0);
    }else{
      ret = sendBulkTransfer(fd,bulkBufSize);
    }
    if(ret < 0){
      dist_TimerStart(dist,checkBulkTranfer,0,
		      20000,v,xxx);
      return;
    }
    cnt++;
    bulkCount--;
  }
  /* ask for the time again */
  if(bulkPingMode == 0){
    strcpy(pingBuffer,"time");
    ret = sctpSEND(fd,bulkStream,pingBuffer,5,SCTP_getAddr(),sendOptions,0,0);
    if(ret < 0){
      printf("Could not get time in, will wait ret:%d\n",ret);
      dist_TimerStart(dist,checkBulkTranfer,0,
		      20000,v,xxx);
      return;
    }
  }
  bulkInProgress = 0;
  printf("bulk message are now queued time q ret:%d\n",ret);
}


void
handleMultiplePongs(int fd, struct sockaddr *from, int theStream)
{
  int    i, blocked = 0;
  time_t now;
  
  
  for(i = 0;i < MAX_PING_PONG_CONTEXTS;i++){
    if(pingPongTable[i].stream == theStream){
      pingPongTable[i].counter++;
      if(pingPongTable[i].counter >= pingPongTable[i].nbRequests){
	/* The party is finished */
	time_t x;
	struct tm *timeptr;
	pingPongsDefined--;
	x = time(0);
	timeptr = localtime(&x);
	printf("%s",asctime(timeptr));
	printf("--> Stream %d: ping pong completed \n", theStream);
	printf(">");
	fflush(stdout);
	pingPongTable[i].stream = -1;
	pingPongTable[i].nbRequests = 0;
	pingPongTable[i].counter = 0;
	pingPongTable[i].started = 0;
	pingPongTable[i].blockAfter = 0;
	pingPongTable[i].blockDuring = 0;
	pingPongTable[i].blockTime = 0;
      }else{
	if((pingPongTable[i].counter % 10) == 0){
	  printf("--> Stream %d: sending ping %d \n",theStream,pingPongTable[i].counter);
	  printf(">");
	  fflush(stdout);
	}
	if(pingPongTable[i].blockAfter == pingPongTable[i].counter){
	  printf("--> Stream %d: blocked at ping pong %d for %d seconds \n", 
		 theStream, pingPongTable[i].counter, pingPongTable[i].blockDuring);
	  printf(">");
	  pingPongTable[i].blockTime = time(0);
	  fflush(stdout);
	}else{
	  sctpSEND(fd,theStream,pingBuffer,pingBufSize,from,sendOptions,0,0);  
	}
      }
    } 
  }
  /* Restart all candidate blocked streams */
  for(i=0;i<MAX_PING_PONG_CONTEXTS;i++){
    if(pingPongTable[i].blockTime != 0){
      blocked++;
    }
  }
  for(i=0;i<MAX_PING_PONG_CONTEXTS;i++){
    if(pingPongTable[i].blockTime != 0){
      /* blockTime set means that the context is blocked */
      now = time(0);
      if(now >= (pingPongTable[i].blockTime +  pingPongTable[i].blockDuring)){
	pingPongTable[i].counter++;
	pingPongTable[i].blockTime = 0; 
	printf("--> Stream %d: unblocked after %d seconds \n", 
	       pingPongTable[i].stream, pingPongTable[i].blockDuring);
	printf(">");
	sctpSEND(fd,pingPongTable[i].stream,pingBuffer,pingBufSize,from,sendOptions,0,0);
	blocked--;
      }
      if(pingPongsDefined <= blocked){
	/* Hum: the remaining ping pongs are all blocked: we must force their unblocking, else
	   they will never wake up */
	pingPongTable[i].counter++;
	pingPongTable[i].blockTime = 0;
	printf("--> Stream %d: unblocked since the remaining %d streams are all blocked \n", 
	       pingPongTable[i].stream, blocked);
	printf(">");
	sctpSEND(fd,pingPongTable[i].stream,pingBuffer,pingBufSize,from,sendOptions,0,0);  
      }
    }
  }
  return;
}

/*--------------------------------------------------*/

void
SCTPdataTimerTicks(void *o,void *b)
{
  int ret;
  int fd;
  struct sctpdataToProduce dp;

  fd = (int)b;
  memset(&dp,0,sizeof(dp));
  strcpy(dp.string,"hulk");
  dp.sent = dist->lastKnownTime;
  dp.seq = curSeq;
  curSeq++;
  ret = sctpSEND(fd,defStream,(char *)&dp,sizeof(dp),
		 SCTP_getAddr(),sendOptions,payload,0);      
  if(ret < 0){
    printf("Send failed? errno:%d\n",errno);
    return;
  }
  /* Restart the timer */
  dist_TimerStart(dist,
		  SCTPdataTimerTicks, 
		  0,period,o,b);
}


void
printArry(unsigned char *data,int sz)
{
  /* if debug is on hex dump a array */
  int i,j,linesOut;
  char buff1[64];
  char buff2[64];
  char *ptr1,*ptr2,*dptrlast,*dptr;
  char *hexes = "0123456789ABCDEF";
  ptr1 = buff1;
  ptr2 = buff2;
  dptrlast = dptr = (char *)data;
  for(i=0,linesOut=0;i<sz;i++){
    *ptr1++ = hexes[0x0f&((*dptr)>>4)];
    *ptr1++ = hexes[0x0f&(*dptr)];
    *ptr1++ = ' ';
    if((*dptr >= 040) && (*dptr <= 0176))
      *ptr2++ = *dptr;
    else
      *ptr2++ = '.';
    dptr++;
    if(((i+1) % 16) == 0){
      *ptr1 = 0;
      *ptr2 = 0;
      printf("%s %s\n",buff1,buff2);
      linesOut++;
      ptr1 = buff1;
      ptr2 = buff2;
      dptrlast = dptr;
    }
  }
  if((linesOut*16) < sz){
    char spaces[64];
    int dist,sp;

    j=(linesOut*16);
    dist = ((16 - (i - j)) * 3) + 2;
    *ptr1 = 0;
    *ptr2 = 0;
    for(sp=0;sp<dist;sp++){
      spaces[sp] = ' ';
    }
    spaces[sp] = 0;
    printf("%s %s%s\n",buff1,spaces,buff2);
  }
  fflush(stdout);
}

static int str1Flow=0;
static int str2Flow=0;

void
sendRftpTransfer(void *v, void *xxx)
{
  char cbuf[10000];
  int bsz, readsz, ret1, ret2, sndsz;
  testDgram_t *data;
  int limit;
  int fd;

  fd = (int)v;
  limit = 0;

  if(rftp_bsz > 10000-sizeof(testDgram_t))
    bsz = 10000-sizeof(testDgram_t);
  else bsz = rftp_bsz;

  data = (testDgram_t *)cbuf;
  data->type = SCTP_TEST_RFTP;
  while(1) {
    /* do first stream */
    if(rftp_ending1 == 0) {
      readsz = fread((void *)(cbuf+sizeof(testDgram_t)), 1, bsz, rftp_in1);
      if(readsz == 0) rftp_ending1 = 1;
    } else {
      readsz = 0;
      if(++rftp_ending1 == 5) {
	printf("read done for stream %d\n",rftp_strm1);
	fclose(rftp_in1);
      }
    }
    if(rftp_ending1 <= 5){
      int ttt;
      data->dgramLen = readsz;
      sndsz = readsz + sizeof(testDgram_t);
      ttt = time_to_live;
      time_to_live = rftp_rt;
      ret1 = sctpSEND(fd,rftp_strm1,cbuf,sndsz,SCTP_getAddr(),sendOptions,payload,0);
      time_to_live = ttt;
      if(ret1 < 0) {
	str1Flow++;
	if(readsz > 0) 
	  fseek(rftp_in1, (-1 * (long)readsz), SEEK_CUR); /* rewind position */
	break;
      } else {
	limit++;
      }
    }

    /* do second stream */
    if(rftp_ending2 == 0) {
      readsz = fread((void *)(cbuf+sizeof(testDgram_t)), 1, bsz, rftp_in2);
      if(readsz == 0) rftp_ending2 = 1;
    } else {
      readsz = 0;
      if(++rftp_ending2 == 5) {
	printf("read done for stream %d\n", rftp_strm2); 
	fclose(rftp_in2);
      }
    }
    if(rftp_ending2 <= 5){
      int ttt;
      data->dgramLen = readsz;
      sndsz = readsz + sizeof(testDgram_t);
      ttt = time_to_live;
      time_to_live = rftp_rt;
      ret2 = sctpSEND(fd,rftp_strm2,cbuf,sndsz,SCTP_getAddr(),sendOptions,payload,0);
      time_to_live = ttt;
      if(ret2 < 0) {
	str2Flow++;
	if(readsz > 0)
	  fseek(rftp_in2, (-1 * (long)readsz), SEEK_CUR); /* rewind position */
	break;
      } else {
	limit++;
      }
    }
    if(rftp_ending1 >= 5 && rftp_ending2 >= 5) {
      printf("Transfer to send queues complete s1flow:%d s2flow:%d\n",
	     str1Flow,str2Flow);
      return;
    }
  }
  if(rftp_ending1 >= 5 && rftp_ending2 >= 5) {
    printf("Transfer 2 send queues complete\n");
    return;
  }
  dist_TimerStart(dist,sendRftpTransfer,0,20000,v,xxx);
}

void
handleRftpTransfer(int fd,messageEnvolope *msg)
{
  int ret;
  char *dat;
  int wrtSz;
  if((rftp_in2 == NULL) || (rftp_in1 == NULL)){
    /* not in a transfer */
    return;
  }
  dat = (char *)((u_long)msg->data + sizeof(testDgram_t));
  wrtSz = msg->siz - sizeof(testDgram_t);

  if(msg->streamNo == rftp_strm1) {
    if(wrtSz > 0) {
      ret = fwrite((void *)dat, 1, wrtSz, rftp_out1);
      if(ret != wrtSz) {
	printf("write to strm 1 file incomplete\n");
      }
    } else {
      if(rftp_out1) {
	fclose(rftp_out1);
	rftp_out1 = NULL;
	printf("done rcv strm %d\n", rftp_strm1);
      }
    }
  }else if(msg->streamNo == rftp_strm2) {
    if(wrtSz > 0) {
      ret = fwrite((void *)dat, 1, wrtSz, rftp_out2);
      if(ret != wrtSz) {
	printf("write to strm 2 file incomplete\n");
      }
    } else {
      if(rftp_out2) {
	fclose(rftp_out2);
	rftp_out2 = NULL;
	printf("done rcv strm %d\n", rftp_strm2);
      }
    }
  }else {
    printf("RFTP bad stream num %d!\n",msg->streamNo);
  }
}


int
sendLoopRequest(int fd,int sz)
{
  char buffer[15000];
  testDgram_t *tt;
  static unsigned int dgramCount=0;
  int sndsz,ret;

  if((sz+sizeof(testDgram_t)) > (sizeof(buffer))){
    sndsz = sizeof(buffer) -  sizeof(testDgram_t);
  }else{
    sndsz = sz +  sizeof(sizeof(testDgram_t));
  }
  memset(buffer,0,sizeof(buffer));
  tt = (testDgram_t *)buffer;
  tt->type = SCTP_TEST_LOOPREQ;
  tt->dgramLen = sndsz - sizeof(testDgram_t);
  tt->dgramID = dgramCount++;
  ret = sctpSEND(fd,defStream,buffer,sndsz,SCTP_getAddr(),sendOptions,payload,0);      
  return(ret);
}


void
handlePong(int fd,struct sockaddr *from,int mode)
{
  pingPongCount--;
  if(pingPongCount <= 0){
    /* done */
    time_t x;
    struct tm *timeptr;
    x = time(0);
    timeptr = localtime(&x);
    printf("%s",asctime(timeptr));
    printf("--Ping pong completes\n");
    fflush(stdout);
    return;
  }
  if(mode == 0){
    sctpSEND(fd,pingStream,pingBuffer,pingBufSize,from,sendOptions,0,0);  
  }else{
    sendLoopRequest(fd,pingBufSize);
  }
}


void
doPingPong(int fd)
{
  time_t x;
  int i;
  struct tm *timeptr;
  if(bulkPingMode == 0){
    /* use ascii bulk ping mode.*/
    strncpy(pingBuffer,"ping",4);
    for(i=4;i<pingBufSize;i++){
      pingBuffer[i] = 'A' + (i%26);
    }
    sctpSEND(fd,pingStream,pingBuffer,pingBufSize,SCTP_getAddr(),sendOptions,payload,0);      
  }else{
     /* use binary bulk ping mode */
    sendLoopRequest(fd,pingBufSize);
  }
  x = time(0);
  timeptr = localtime(&x);
  printf("%s",asctime(timeptr));
}

void
handleHulk(int fd,
	   struct sockaddr *to,
	   struct sctpdataToProduce *dp)
{
  int ret;
  strcpy(dp->string,"sulk");
  dp->recvd = dist->lastKnownTime;
  ret = sctpSEND(fd,defStream,(char *)dp,sizeof(*dp),
		 to,sendOptions,payload,0);
  if(ret < 0){
    printf("Could not reply to a hulk with a sulk! %d:%d\n",
	   ret,errno);
  }
}

static int str1read=0;
static int str2read=0;
static u_int simplecnt=0;
static int modpacket=0;
static int phase_at = 0;
static char phase_list[] = {
	'|',
	'/',
	'-',
	'\\',
	'|',
	'/',
	'-',
	'\\',
};

	
void
sctpInput(void *arg,messageEnvolope *msg)
{
  /* receive some number of datagrams and
   * act on them.
   */

  int disped,i;
  int fd;
  testDgram_t *testptr;
  disped = i = 0;


  SCTP_setcurrent((sctpAdaptorMod *)arg);
  if(msg->type == PROTOCOL_Sctp){
    fd  = (int)msg->sender;
  }else{
    /* we don't deal with non-sctp data */
    return;
  }
  testptr = (testDgram_t *)msg->data;
  if(testptr->type == SCTP_TEST_LOOPREQ){
    /* another ping/pong type */
    /*    printf("TEST_LOOPREQ send %d bytes str:%d seq:%d\n",
	  msg->siz,msg->streamNo,msg->streamSeq);*/
    testptr->type = SCTP_TEST_LOOPRESP;
    if(loop_sleep) {
	    sleep(loop_sleep);
	    printf(">>>%c\r",phase_list[phase_at]);
	    fflush(stdout);
	    phase_at++;
	    if(phase_at > 7) {
		    phase_at = 0;
	    }
    }
    if(msdelaymode) {
	    struct timespec to,top;
	    modpacket++;
	    if((modpacket % msdelaymode) == 0) {
		    /* pause 10 ms every third msdelaymode packets */
		    modpacket = 0;
		    to.tv_sec = 0;
		    to.tv_nsec = 10000000;
		    top = to;
		    nanosleep(&to,&top);
	    }
    }
    sctpSEND(fd,msg->streamNo,msg->data,msg->siz,(struct sockaddr *)msg->from,
	     sendOptions,
	     msg->protocolId,0);
    msg->data = NULL;
  }else if(testptr->type == SCTP_TEST_LOOPRESP){
    handlePong(fd,(struct sockaddr *)msg->from,1);
    msg->data = NULL;
  }else if(testptr->type == SCTP_TEST_SIMPLE){
    simplecnt++;
    if((simplecnt % 1000) == 0){
      printf("Have received %d simple dg's\n",simplecnt);
    }
    msg->data = NULL;
  }else if(testptr->type == SCTP_TEST_RFTP){
    /* send the data back */
    testptr->type = SCTP_TEST_RFTPRESP;
    if(msg->streamNo == 1){
      str1read++;
    }else{
      str2read++;
    }
    sctpSEND(fd,msg->streamNo,msg->data,msg->siz,(struct sockaddr *)msg->from,
	     sendOptions, msg->protocolId,0);
    msg->data = NULL;
  }else if(testptr->type == SCTP_TEST_RFTPRESP){
    handleRftpTransfer(fd, msg);
    msg->data = NULL;
  }else if(strncmp(msg->data,"ping",4) == 0){
    /* it is a ping-pong message, send it back after changing
     * the first 4 bytes to pong 
     */
    strncpy(msg->data,"pong",4);
    sctpSEND(fd,msg->streamNo,msg->data,msg->siz,(struct sockaddr *)msg->from,
	     sendOptions,
	     msg->protocolId,0);
    msg->data = NULL;
  }else if(strcmp(msg->data,"time") == 0){
    struct tm *timeptr;
    time_started = time(0);
    timeptr = localtime(&time_started);
    printf("%s",asctime(timeptr));
    msg->data = NULL;
    bulkSeen = 0;
  }else if(strncmp(msg->data,"pong",4) == 0){
    if(pingPongsDefined > 0){
      handleMultiplePongs(fd,(struct sockaddr *)msg->from,msg->streamNo);
    }else{
      handlePong(fd,(struct sockaddr *)msg->from,0);
    }
  }else if(strncmp(msg->data,"hulk",4) == 0){
    handleHulk(fd,(struct sockaddr *)msg->from,
	       (struct sctpdataToProduce *)msg->data);
    msg->data = NULL;
  }else if(strncmp(msg->data,"sulk",4) == 0){
    if(hulkfile != NULL){
      struct sctpdataToProduce *dp;
      dp = (struct sctpdataToProduce *)msg->data;
      dp->stored = dist->lastKnownTime;
      fwrite(msg->data,sizeof(struct sctpdataToProduce),1,hulkfile);
    }
    msg->data = NULL;
  }else if(strncmp(msg->data,"bulk",4) == 0){
    bulkSeen++;
  }else{
    if(silent_mode == 0){
      /* display a text message */
      if(isascii(((char *)msg->data)[0])){
	printf("From:");
	SCTPPrintAnAddress(msg->from);
	if(msg->to) {
		printf("To:");
		SCTPPrintAnAddress(msg->to);
	}
	((char *)msg->data)[msg->siz] = 0;
	printf("strm:%d seq:%d %d:'%s'\n",
	       msg->streamNo,
	       msg->streamSeq,
	       msg->siz,
	       (char *)msg->data);
	disped = 1;
	msg->data = NULL;
      }else{
	printf("From:");
	SCTPPrintAnAddress(msg->from);
	if(msg->to) {
		printf("To:");
		SCTPPrintAnAddress(msg->to);
	}
	printf("strm:%d seq:%d %d:\n",
	       msg->streamNo,
	       msg->streamSeq,
	       msg->siz);
	printArry((char *)msg->data,msg->siz);
	msg->data = NULL;
	disped = 1;
      }
    }else{
      silent_count++;
    }
  }
  if(disped){
    printf(">");
    fflush(stdout);
  }
}


/*
 * Called with a complete line of input by the readline library.
 */
static void
handleStdin2(char *line)
{
    if (line == NULL || *line == '\0')
        return;
    execute_line(line);
    add_history(line);
    free(line);
}



/* Called by the distributor each time stdin is ready for reading.
 */
int
stdinInFdHandler(void *arg,int fd, int event)
{

  if(fd != 0){
    printf("fd handler called with fd=%d not 0\n",fd);
    return(0);
  }

  /* XXX [MM] as a quick and dirty hack, set the adaptor to the global "adap".
   * The correct fix is modify rl_callback_read_char to accept more
   * than one parameter.
   */

  adap = (sctpAdaptorMod *) arg;


  /* Read the next character from the input. If that completes the line,
   * then call what has been setup with rl_callback_handler_install.
   */
  rl_callback_read_char();
  return(0);
}


/* Initialize user interface handling.
 */
void
initUserInterface(distributor *o,struct sctpAdaptorMod *s)
{
  /* Init the readline library, callback mode. */

  /* Override the default completion done by the library, because
   * we want to complete commands, not filenames.
   */
#if defined(__APPLE__)
  rl_completion_entry_function = (rl_compentry_func_t *) command_generator;
#else
  rl_completion_entry_function = (Function *) command_generator;
#endif
  /*  Call handleStdin2 when a complete line of input has been entered. */
  rl_callback_handler_install(">>>", handleStdin2);

  dist = o;
  dist_addFd(o,0,stdinInFdHandler,POLLIN,(void *)s);
  resetPingPongTable();
  /* now subscribe for messages */
  dist_msgSubscribe(s->o,
		    sctpInput,
		    DIST_SCTP_PROTO_ID_DEFAULT,DIST_STREAM_DEFAULT,
		    10,(void *)s);
}


/* Clean up the terminal and readline state.
 */
void
destroyUserInterface(void)
{
    /* _rl_clean_up_for_exit(); */
    rl_deprep_term_function();
}


/* Look up NAME as the name of a command, and return a pointer to that
 * command or NULL if not found.
 */
static struct command *
find_command(char *name)
{
    int i;
    if(name == 0)
      return(NULL);
    if(name[0] == 0)
      return(NULL);
    for (i = 0; commands[i].co_name != NULL; i++)
        if (strcmp(name, commands[i].co_name) == 0)
            return(&commands[i]);

    return NULL;
}


/* Execute a command line.
 *
 * The line syntax is "command arg1 arg2 ..." (whitespace is separator).
 * Put the args in an argv[] and invoke the proper function.
 *
 * Preconditions:
 *   line is not empty
 */

static int
execute_line(const char *line)
{
    struct command *command;
    char *argv[10]; /* more than enough */
    char *buf, *cmd;
    int i, ret;

    if (*line == '\0')
	return -1;
    bzero(argv, sizeof(argv));
    if ( (buf = strdup(line)) == NULL) {
	perror("strdup");
	return -1;
    }
    /* break buf into pieces and put them in argv[] */
    cmd = strtok(buf, " \t");
    for (i = 0; i < 10; i++) {
	if ( (argv[i] = strtok(NULL, " \t")) == NULL)
	    break;
    }
    /* Since the line may have been generated by command completion
     * or by the user, we cannot be sure that it is a valid command name.
     */
    if ( (command = find_command(cmd)) == NULL) {
        printf("%s: No such command.\n", cmd);
        return -1;
    }
    ret = command->co_func(argv, i);
    free(buf);
    return ret;
}


/* Generator function for command completion, called by the readline library.
 * It returns a list of commands which match "text", one entry per invocation.
 * Each entry must be freed by the function which has been setup with
 * rl_callback_handler_install. The end of the list is marked by returning
 * NULL. When called for the first time with a new "text", "state" is set to
 * zero to allow for initialization.
 */
static char *
command_generator(char *text, int state)
{
    static int index, len;
    char *name;

    /* If this is a new word to complete, initialize now. */
    if (state == 0) {
        index = 0;
        len = strlen(text);
    }
    /* Return the next name which partially matches from the command list. */
    while ( (name = commands[index].co_name) != NULL) {
        index++;
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    return NULL;    /* no names matched */
}


/* addip address [how] - add ip address where how is the mask/action to pass.
 * New version using getaddrinfo.
 */
int
cmd_gettimetolive(char *argv[], int argc)
{
  printf("Time to live of u-sctp messages is %d microseconds\n",
	 time_to_live);
  return 0;	
}

int
cmd_sendsent(char *argv[], int argc)
{
    printf("command no longer supported\n");
  return 0;
}

int
cmd_deliverydump(char *argv[], int argc)
{
    printf("command no longer supported\n");
    return 0;
}


int
cmd_setloopsleep(char *argv[], int argc)
{
	if(argc == 0){
		printf("Missing argument, can't set unknown value\n");
		return(0);
	}
	loop_sleep = strtol(argv[0],NULL,0);
	printf("loop sleep now set to %d\n",loop_sleep);
	return(0);
}
int 
cmd_getloopsleep(char *argv[], int argc)
{
	printf("Loop sleep is set to %d seconds\n",loop_sleep);
	return(0);
}


int
cmd_streamreset(char *argv[], int argc)
{
	char buffer[2048];
	struct sctp_stream_reset *strrst;
	memset(buffer,0,sizeof(buffer));
        strrst = (struct sctp_stream_reset *)buffer;
	if(argc < 2) {
	jump_out:
		printf("Missing argument(s), 'both|send|recv' 'all|num num num'\n");
		printf("------arg 1-----------\n");
		printf("both - gets both the send and recv streams reset\n");
		printf("recv - gets your receiving stream reset\n");
		printf("send - gets your sending stream reset\n");
		printf("------arg 2(-n)-----------\n");
		printf(" all - gets you all streams\n");
		printf(" num num num - gets you the listed numbered streams\n");
		return(0);
	}
	if ((strcmp(argv[0], "recv") == 0) ||
	    (strcmp(argv[0], "RECV") == 0)) {
		strrst->strrst_flags = SCTP_RESET_LOCAL_RECV;
	} else if ((strcmp(argv[0], "send") == 0) ||
		   (strcmp(argv[0], "SEND") == 0)) {
		strrst->strrst_flags = SCTP_RESET_LOCAL_SEND;
	} else if ((strcmp(argv[0], "both") == 0) ||
		   (strcmp(argv[0], "BOTH") == 0)) {
		strrst->strrst_flags = SCTP_RESET_BOTH;
	} else {
		printf("First argument invalid, must be both|recv|send\n");
		goto jump_out;
	}
	
	if((strcmp(argv[1], "all") == 0) ||
	   (strcmp(argv[1], "ALL") == 0)) {
		strrst->strrst_num_streams = 0;
	} else {
		int i;
		strrst->strrst_num_streams = argc - 1;
		for(i=1;i<argc;i++) {
			strrst->strrst_list[(i-1)] = strtol(argv[i],NULL,0); 
			printf("reseting stream %d\n",strrst->strrst_list[i]);
		}
	}
	strrst->strrst_assoc_id = get_assoc_id();
	if(setsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_RESET_STREAMS,
		      strrst, (sizeof(*strrst)+(strrst->strrst_num_streams * sizeof(uint16_t)))) != 0){
		printf("Stream Reset fails %d\n",errno);
	}else{
		printf("Stream reset succesful\n");
	}
	return(0);
}

int
cmd_setdebug(char *argv[], int argc)
{
  u_int32_t num,newlevel;
  int sz;

  sz = sizeof(u_int32_t);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_SET_DEBUG_LEVEL,
		&num, &sz) != 0){
    printf("Sorry, Kernel debug levels not supported error:%d\n",errno);
    return(0);
  }
  if(argc == 0){
    printf("Current settings are %x\n",num);
  }else{
    newlevel = strtol(argv[0],NULL,0);
    if(newlevel == 0){
      if(strcmp(argv[0],"off") == 0){
	printf("Turning all debug off\n");
	num = 0;
      }else if(strncmp(argv[0],"timer",5) == 0){
	if (argv[0][5] == '1'){
	  printf("adding timer level 1\n");
	  num |= SCTP_DEBUG_TIMER1;
	}else if (argv[0][5] == '2'){
	  printf("adding timer level 2\n");
	  num |= SCTP_DEBUG_TIMER2;
	}else if (argv[0][5] == '3'){
	  printf("adding timer level 3\n");
	  num |= SCTP_DEBUG_TIMER3;
	}else if (argv[0][5] == '4'){
	  printf("adding timer level 4\n");
	  num |= SCTP_DEBUG_TIMER4;
	}else{
	  printf("adding all timer levels\n");
	  num |= SCTP_DEBUG_TIMER1;
	  num |= SCTP_DEBUG_TIMER2;
	  num |= SCTP_DEBUG_TIMER3;
	  num |= SCTP_DEBUG_TIMER4;
	}
      }else if(strncmp(argv[0],"output",6) == 0){
	if (argv[0][6] == '1'){
	  printf("adding output level 1\n");
	  num |= SCTP_DEBUG_OUTPUT1;
	}else if (argv[0][6] == '2'){
	  printf("adding output level 2\n");
	  num |= SCTP_DEBUG_OUTPUT2;
	}else if (argv[0][6] == '3'){
	  printf("adding output level 3\n");
	  num |= SCTP_DEBUG_OUTPUT3;
	}else if (argv[0][6] == '4'){
	  printf("adding output level 4\n");
	  num |= SCTP_DEBUG_OUTPUT4;
	}else{
	  printf("adding all output levels\n");
	  num |= SCTP_DEBUG_OUTPUT1;
	  num |= SCTP_DEBUG_OUTPUT2;
	  num |= SCTP_DEBUG_OUTPUT3;
	  num |= SCTP_DEBUG_OUTPUT4;
	}
      }else if(strncmp(argv[0],"input",5) == 0){
	if (argv[0][5] == '1'){
	  printf("adding input level 1\n");
	  num |= SCTP_DEBUG_INPUT1;
	}else if (argv[0][5] == '2'){
	  printf("adding input level 2\n");
	  num |= SCTP_DEBUG_INPUT2;
	}else if (argv[0][5] == '3'){
	  printf("adding input level 3\n");
	  num |= SCTP_DEBUG_INPUT3;
	}else if (argv[0][5] == '4'){
	  printf("adding input level 4\n");
	  num |= SCTP_DEBUG_INPUT4;
	}else{
	  printf("adding all input levels\n");
	  num |= SCTP_DEBUG_INPUT1;
	  num |= SCTP_DEBUG_INPUT2;
	  num |= SCTP_DEBUG_INPUT3;
	  num |= SCTP_DEBUG_INPUT4;
	}
      }else if(strncmp(argv[0],"util",4) == 0){
	if (argv[0][4] == '1'){
	  printf("Adding util level 1\n");
	  num |= SCTP_DEBUG_UTIL1;
	}else if (argv[0][4] == '2'){
	  printf("Adding util level 2\n");
	  num |= SCTP_DEBUG_UTIL2;
	}else{
	  printf("Adding all util levels\n");
	  num |= SCTP_DEBUG_UTIL1;
	  num |= SCTP_DEBUG_UTIL2;
	}
      }else if(strncmp(argv[0],"asconf",6) == 0){
	if (argv[0][6] == '1'){
	  printf("Adding asconf level 1\n");
	  num |= SCTP_DEBUG_ASCONF1;
	}else if (argv[0][6] == '2'){
	  printf("Adding asconf level 2\n");
	  num |= SCTP_DEBUG_ASCONF2;
	}else{
	  printf("Adding all asconf levels\n");
	  num |= SCTP_DEBUG_ASCONF1;
	  num |= SCTP_DEBUG_ASCONF2;
	}
      }else if(strncmp(argv[0],"pcb",3) == 0){
	if (argv[0][3] == '1'){
	  printf("adding pcb level 1\n");
	  num |= SCTP_DEBUG_PCB1;
	}else if (argv[0][3] == '2'){
	  printf("adding pcb level 2\n");
	  num |= SCTP_DEBUG_PCB2;
	}else if (argv[0][3] == '3'){
	  printf("adding pcb level 3\n");
	  num |= SCTP_DEBUG_PCB3;
	}else if (argv[0][3] == '4'){
	  printf("adding pcb level 4\n");
	  num |= SCTP_DEBUG_PCB4;
	}else{
	  printf("adding all pcb levels\n");
	  num |= SCTP_DEBUG_PCB1;
	  num |= SCTP_DEBUG_PCB2;
	  num |= SCTP_DEBUG_PCB3;
	  num |= SCTP_DEBUG_PCB4;
	}
      }else if(strncmp(argv[0],"indata",6) == 0){
	if (argv[0][6] == '1'){
	  printf("adding indata level 1\n");
	  num |= SCTP_DEBUG_INDATA1;
	}else if (argv[0][6] == '2'){
	  printf("adding indata level 2\n");
	  num |= SCTP_DEBUG_INDATA2;
	}else if (argv[0][6] == '3'){
	  printf("adding indata level 3\n");
	  num |= SCTP_DEBUG_INDATA3;
	}else if (argv[0][6] == '4'){
	  printf("adding indata level 4\n");
	  num |= SCTP_DEBUG_INDATA4;
	}else{
	  printf("adding all indata levels\n");
	  num |= SCTP_DEBUG_INDATA1;
	  num |= SCTP_DEBUG_INDATA2;
	  num |= SCTP_DEBUG_INDATA3;
	  num |= SCTP_DEBUG_INDATA4;
	}
      }else if(strncmp(argv[0],"usrreq",6) == 0){
	if (argv[0][6] == '1'){
	  printf("Adding usrreq level 1\n");
	  num |= SCTP_DEBUG_USRREQ1;
	}else if (argv[0][6] == '2'){
	  printf("Adding usrreq level 2\n");
	  num |= SCTP_DEBUG_USRREQ2;
	}else{
	  printf("Adding all usrreq levels\n");
	  num |= SCTP_DEBUG_USRREQ1;
	  num |= SCTP_DEBUG_USRREQ2;
	}
      }else if(strncmp(argv[0],"peel",4) == 0){
	printf("Adding the sctp peeloff level\n");
	num |= SCTP_DEBUG_PEEL1;
      }else if(strncmp(argv[0],"all",3) == 0){
	printf("adding ALL debug levels\n");
	num = SCTP_DEBUG_ALL;
      }else{
	printf("Argument unrecognzied use either 0xnumber or\n");
	printf(" [timer[x]] or [util[x]] or [input[x]] or [all] or\n");
	printf(" [off] or [peel] or usrreq[x] or indata[x] or output[x] or\n");
	printf(" [asconf[x]] or pcb[x]\n");
	printf("Where the [x] is a level number, leave it off and all\n");
	printf("levels of that type are enabled\n");
	return(0);
      }
    }else{
      num = newlevel;
      printf("Setting to level 0x%x\n",num);
    }
    if(setsockopt(adap->fd,IPPROTO_SCTP,
		  SCTP_SET_DEBUG_LEVEL,
		  &num, sizeof(num)) != 0){
      printf("Set fails error %d\n",errno);
    }else{
      printf("Set of new level suceeds\n");
    }
  }
  return(0);
}

int
cmd_silentcnt(char *argv[], int argc)
{
  if(silent_mode){
    printf("Current silent count is 0x%x\n",silent_count);
  }else{
    printf("Not in silent mode, but the last silent count was 0x%x\n",
	   silent_count);
  }
  return 0;	
}

int
cmd_silent(char *argv[], int argc)
{
  if(silent_mode){
    printf("Disabling silent running mode - inbound msg will display\n");
    silent_mode = 0;
  }else{
    printf("Enabling silent running mode - inbound msg will NOT display\n");
    silent_mode = 1;
    silent_count = 0;
  }
  return 0;	
}



int
cmd_settimetolive(char *argv[], int argc)
{
  if (argc < 1) {
    printf("settimetolive: expected 1 argument(time-in-microseconds)\n");
    return -1;
  }
  time_to_live = strtoul(argv[0],NULL,0);
  return 0;
}


static int
cmd_addip(char *argv[], int argc)
{
  /*    int fd = adap->fd;*/
  /*    char *address;*/
    /*    struct addrinfo hints, *res;*/
    /*    int how, ret;*/
    /*
    if (argc < 1) {
	printf("addip: expected at least 1 argument\n");
	return -1;
    }
    */
    /*    address = argv[0];*/
    /*    how = argv[1] != NULL ? strtol(argv[1], NULL, 0) : 0;*/

    /*    bzero(&hints, sizeof(hints));*/
    /*    hints.ai_flags = AI_NUMERICHOST;*/ /* disable name resolution */
    /*    ret = getaddrinfo(address, NULL, &hints, &res);
     if (ret != 0) {
 	printf("addip: getaddrinfo: %s\n", gai_strerror(ret));
 	return -1;
    }
    */
  /*    printf("addip: adding IP address %s, action 0x%x\n", address, how);*/
    /*  ret = sctpADDIPADDRESS(fd, res->ai_addr, SCTP_getAddr(), how);*/
    /*    if (ret != 1) {
 	printf("addip: failed, return value: %d\n", ret);
     } else {
 	printf("addip: success\n");
    }
    freeaddrinfo(res);
    */
    /*   return ret != 1 ? -1 : 0;*/
    return 0;
}

int
cmd_getrtomin(char *argv[], int argc)
{
  struct sctp_rtoinfo optval;
  int siz;

  siz = sizeof(optval);
  memset(&optval,0,sizeof(optval));
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_RTOINFO, &optval, &siz) != 0) {
    printf("Can't get RTOINFO on socket err:%d!\n",errno);
  }else{
    printf("RTO initial is %d ms\n",optval.srto_initial);
    printf("RTO MAX is %d ms\n",optval.srto_max);
    printf("RTO MIN is %d ms\n",optval.srto_min);
  }
  return(0);
}
int
cmd_setrtomin(char *argv[], int argc)
{
  struct sctp_rtoinfo optval;
  if (argc < 3) {
    printf("setrtomin: expected 3 arguments (rtoinit rtomax rtomin)\n");
    return -1;
  }
  memset(&optval,0,sizeof(optval));

  optval.srto_initial = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  optval.srto_max = (argv[1] != NULL) ? strtoul(argv[1], NULL, 0) : 0;
  optval.srto_min = (argv[2] != NULL) ? strtoul(argv[2], NULL, 0) : 0;

  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_RTOINFO, &optval, sizeof(optval)) != 0) {
    printf("Can't get RTOINFO on socket err:%d!\n",errno);
  }else{
    printf("set socket option succeeds new values are:\n");
    printf("RTO initial is now %d ms\n",optval.srto_initial);
    printf("RTO MAX is now %d ms\n",optval.srto_max);
    printf("RTO MIN is now %d ms\n",optval.srto_min);

  }
  return(0);
}

int cmd_getinitparam(char *argv[], int argc)
{
  int siz;
  struct sctp_initmsg optval;

  siz = sizeof(optval);

  memset(&optval,0,sizeof(optval));
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_INITMSG, &optval, &siz) != 0) {
    printf("Can't get SCTP_INITMSG on socket err:%d!\n",errno);
  }else{
    printf("OS we request is %d\n",optval.sinit_num_ostreams);
    printf("MIS we limit is %d \n",optval.sinit_max_instreams);
    printf("Max INIT attempts is %d\n",optval.sinit_max_attempts);
    printf("Max INIT RTO is %d\n", optval.sinit_max_init_timeo);
  }
  return(0);
}

static int 
cmd_getautoclose(char *argv[], int argc)
{
  u_int32_t adaption,sz;
  sz = sizeof(adaption);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_AUTOCLOSE,
		&adaption, &sz) != 0) {
    printf("Can't get autoclose setting socket err:%d!\n",errno);
  }else{
    if(adaption){
      printf("Autoclose is on and set to %d\n",adaption);
    }else{
      printf("Autoclose is off!\n");
    }
  }
  return(0);

}

static int cmd_getmsdelay(char *argv[], int argc)
{
	if(msdelaymode)
		printf("ms delay is ON\n");
	else
		printf("ms delay is OFF\n");
	return(0);
}
static int cmd_setmsdelay(char *argv[], int argc)
{
	if(argc < 1) {
		printf("Use setmsdelay val\n");
		return -1;
	}
	msdelaymode = strtol(argv[0],NULL,0);
	if(msdelaymode)
		printf("ms delay is now ON\n");
	else
		printf("ms delay is now OFF\n");
	return(0);
}

static int cmd_getnodelay(char *argv[], int argc)
{
  u_int32_t seg,sz;
  sz = sizeof(seg);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_NODELAY,
		&seg, &sz) != 0) {
	  printf("Can't get nagle setting socket err:%d!\n",errno);
  }else{
	  if(seg)
		  printf("Nagle is OFF\n");
	  else
		  printf("Nagle is ON\n");
  }
  return(0);
}

static int cmd_setnodelay(char *argv[], int argc)
{
  u_int32_t seg,sz;

  if(argc < 1) {
	  printf("Use setnodelay val\n");
	  return -1;
  }
  seg = strtol(argv[0],NULL,0);
  sz = sizeof(seg);

  printf("Setting max seg to %s\n",((seg) ? "OFF" : "ON"));
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_NODELAY,
		&seg, sz) != 0) {
	  printf("Can't set sctp_nodelay setting socket err:%d!\n",errno);
  }else{
	  printf("nagle is now %s\n",((seg) ? "OFF" : "ON"));

  }
  return(0);

}



static int cmd_getmaxseg(char *argv[], int argc)
{
  u_int32_t seg,sz;
  sz = sizeof(seg);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_MAXSEG,
		&seg, &sz) != 0) {
	  printf("Can't get maxseg setting socket err:%d!\n",errno);
  }else{
	  printf("Maxseg is %d\n",
		 seg);
  }
  return(0);
}

static int cmd_setmaxseg(char *argv[], int argc)
{
  u_int32_t seg,sz;

  if(argc < 1) {
	  printf("Use setmaxseg val\n");
	  return -1;
  }
  seg = strtol(argv[0],NULL,0);
  sz = sizeof(seg);

  printf("Setting max seg to %d\n",seg);
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_MAXSEG,
		&seg, sz) != 0) {
	  printf("Can't set maxseg setting socket err:%d!\n",errno);
  }else{
	  printf("Maxseg is %d\n",
		 seg);
  }
  return(0);

}


static int 
cmd_getmaxburst(char *argv[], int argc)
{
  u_int32_t burst,sz;
  sz = sizeof(burst);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_MAXBURST,
		&burst, &sz) != 0) {
	  printf("Can't get maxburst setting socket err:%d!\n",errno);
  }else{
	  printf("Maxburst is %d\n",
		 burst);
  }
  return(0);
	
}

static int 
cmd_setmaxburst(char *argv[], int argc)
{
  u_int32_t burst,sz;

  if(argc < 1) {
	  printf("Use setmaxburst val\n");
	  return -1;
  }
  burst = strtol(argv[0],NULL,0);
  sz = sizeof(burst);

  printf("Setting max burst to %d\n",burst);
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_MAXBURST,
		&burst, sz) != 0) {
	  printf("Can't set maxburst setting socket err:%d!\n",errno);
  }else{
	  printf("Maxburst is %d\n",
		 burst);
  }
  return(0);
}

static int 
cmd_setautoclose(char *argv[], int argc)
{
  u_int32_t adaption;
  if (argc < 1) {
    printf("setautoclose: expects an argument (0 turns it off)\n");
    return -1;
  }
  adaption = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_AUTOCLOSE,
		&adaption, sizeof(adaption)) != 0) {
    printf("Can't set autoclose value on socket err:%d!\n",errno);
  }
  return(0);
}


static int 
cmd_getfragmentation(char *argv[], int argc)
{
  u_int32_t adaption,sz;
  sz = sizeof(adaption);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_DISABLE_FRAGMENTS,
		&adaption, &sz) != 0) {
    printf("Can't get fragmentation setting socket err:%d!\n",errno);
  }else{
    printf("Adaption bits returned are %x\n",adaption);
    if(adaption == 1){
      printf("Fragmentation is NOT being performed by SCTP\n");
    }else{
      printf("Fragmentation is being performed by SCTP\n");
    }
  }
  return(0);
}

static int 
cmd_setfragmentation(char *argv[], int argc)
{
  u_int32_t adaption;
  if (argc < 1) {
    printf("setfragment: expects an argument (0 turns it off)\n");
    return -1;
  }
  adaption = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_DISABLE_FRAGMENTS,
		&adaption, sizeof(adaption)) != 0) {
    printf("Can't set fragmentation flag  on socket err:%d!\n",errno);
  }
  return(0);
}

static int 
cmd_setblocking(char *argv[], int argc)
{
  int on_off;
  if (argc < 1) {
    printf("setblocking: expects an argument (on/off)\n");
    return -1;
  }
  if(strcmp(argv[0],"on") == 0){
    on_off = 1;
  }else if(strcmp(argv[0],"off") == 0){
    on_off = 0;
  }else{
    printf("Argument '%s' incorrect on/off please\n",argv[0]);
    return(-1);
  }
  if(ioctl(adap->fd,FIONBIO,&on_off) != 0) {
    printf("IOCTL FIONBIO fails error:%d!\n",errno);
  }
  return(0);
}



static int 
cmd_setadaption(char *argv[], int argc)
{
  u_int32_t adaption;
  if (argc < 1) {
    printf("setadaption: expects an argument (0 turns it off)\n");
    return -1;
  }
  adaption = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ADAPTION_LAYER,
		&adaption, sizeof(adaption)) != 0) {
    printf("Can't set adaption bits on socket err:%d!\n",errno);
  }
  return(0);
}

static int 
cmd_getadaption(char *argv[], int argc)
{
  u_int32_t adaption,sz;
  sz = sizeof(adaption);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ADAPTION_LAYER,
		&adaption, &sz) != 0) {
    printf("Can't get adaption bits on socket err:%d!\n",errno);
  }else{
    printf("Adaption bits to be sent are %u\n",adaption);
  }
  return(0);
}


int cmd_setinittsn(char *argv[], int argc)
{
  u_int32_t init_tsn;
  if (argc < 1) {
    printf("setinittsn: expects a TSN number as a argument (0 turns it off)\n");
    return -1;
  }
  init_tsn = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_SET_INITIAL_DBG_SEQ, &init_tsn, sizeof(init_tsn)) != 0) {
    printf("Can't set initial tsn on socket err:%d!\n",errno);
  }
  return(0);
}

int cmd_getinittsn(char *argv[], int argc)
{
  u_int32_t init_tsn;
  int sz;
  init_tsn = 0;
  sz = sizeof(init_tsn);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_SET_INITIAL_DBG_SEQ, &init_tsn,&sz) != 0) {
    printf("Can't get initial tsn on socket err:%d!\n",errno);
  }else{
    if(init_tsn){
      printf("Initial TSN set to %x\n",init_tsn);
    }else{
      printf("Initial TSN will be a random number\n");
    }
  }
  return(0);
}


int cmd_setinitparam(char *argv[], int argc)
{
  struct sctp_initmsg optval;
  if (argc < 4) {
    printf("setinitparam: expected 4 arguments (OS MIS maxinit initrtomax)\n");
    return -1;
  }
  optval.sinit_num_ostreams = (argv[0] != NULL) ? strtoul(argv[0], NULL, 0) : 0;
  optval.sinit_max_instreams = (argv[1] != NULL) ? strtoul(argv[1], NULL, 0) : 0;
  optval.sinit_max_attempts = (argv[2] != NULL) ? strtoul(argv[2], NULL, 0) : 0;
  optval.sinit_max_init_timeo = (argv[3] != NULL) ? strtoul(argv[3], NULL, 0) : 0;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_INITMSG, &optval, sizeof(optval)) != 0) {
    printf("Can't set SCTP_INITMSG on socket err:%d!\n",errno);
  }else{
    printf("OS set to %d\n",optval.sinit_num_ostreams);
    printf("MIS limited to %d \n",optval.sinit_max_instreams);
    printf("Max INIT set to %d\n",optval.sinit_max_attempts);
    printf("Max INIT RTO set to %d\n", optval.sinit_max_init_timeo);
  }
  return(0);
}

static int
cmd_abort(char *argv[], int argc)
{
  char buf[4];
  memset(buf,0,sizeof(buf));
  return(sctpSEND(adap->fd,0,buf,0,SCTP_getAddr(),MSG_ABORT,0,0));
}
/* assoc - associate with the set destination
 */
static int
cmd_assoc(char *argv[], int argc)
{
  int fd = adap->fd;
  int ret;
  struct sockaddr *sa;

  if (!(destinationSet && portSet)) {
    printf("Please set the destination/port before\n");
    return -1;
  }
  if (fd == -1) {
    printf("sorry m is NULL?\n");
    return -1;
  }
  sa = SCTP_getAddr();
  ret = connect(fd,sa,sa->sa_len);
  printf("Connect returns %d errno:%d\n",ret,errno);
  return 0;
}

int
cmd_xconnect(char *argv[], int argc)
{
	int len,i,addrcnt=0,ret,llen;
	char *ccc;
	struct sockaddr *at;
	struct sockaddr_in *servaddr;
	struct sockaddr_in6 *servaddr6;

	if (!portSet) {
		printf("Please set a port first\n");
		return -1;
	}
	if(argc == 0) {
		printf("Use connectx ip-addr [ip-addr ip-addr]\n");
		return -1;
	}
	len = (sizeof(struct sockaddr_storage) * argc);
	ccc = malloc(len);
	if(ccc == NULL) {
		printf("Sorry can't get memory %d\n",errno);
		return 0;
	}
	memset(ccc,0,len);
	at = (struct sockaddr *)ccc;
	for(i=0;i<argc;i++){
		servaddr6 = (struct sockaddr_in6 *)at;
		if(inet_pton(AF_INET6,argv[i], &servaddr6->sin6_addr)) {
			servaddr6->sin6_family = AF_INET6;
			llen = servaddr6->sin6_len = sizeof(struct sockaddr_in6);
			servaddr6->sin6_port = SCTP_getport();
		} else {
			servaddr = (struct sockaddr_in *)at;
			if (inet_pton(AF_INET, argv[i], &servaddr->sin_addr) ){
				servaddr->sin_family = AF_INET;
				llen = servaddr->sin_len = sizeof(struct sockaddr_in);
				servaddr->sin_port = SCTP_getport();
			} else {
				printf("Skipping address %s, unknown type\n",
				       argv[i]);
				continue;
			}
		}
		addrcnt++;
		at = (struct sockaddr *)((caddr_t)at + llen);
	}
	at = (struct sockaddr *)ccc;
	servaddr = (struct sockaddr_in *)at;
	if(servaddr->sin_family == AF_INET) {
		SCTP_setIPaddr(servaddr->sin_addr.s_addr);
	} else if (servaddr->sin_family == AF_INET6) {
		servaddr6 = (struct sockaddr_in6 *)at;
		SCTP_setIPaddr6((u_char *)&servaddr6->sin6_addr);
	}
	ret = sctp_connectx(adap->fd, at, addrcnt);
	if(ret)
		printf("Connectx returns %d errno:%d\n",ret,errno);
	free(ccc);
	return 0;
}

static int
cmd_bindx(char *argv[], int argc)
{
  int bindType = 0;
  int bindOption = 0;
  char bindx_ss[sizeof(struct sctp_getaddresses) + sizeof(struct sockaddr_storage)];
  struct sctp_getaddresses *optval;
  struct sockaddr_in6 *sin6;
  struct sockaddr_in *sin;
  if (argc < 2) {
    printf("bindx: invalid number of arguments\n");
    return -1;
  }
  if (bindSpecific == 0) {
    printf("Endpoint is BOUNDALL... ignoring\n");
    return -1;
  }
  /* get the bindx type */
  bindType = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  if (bindType == 0)
    bindOption = SCTP_BINDX_ADD_ADDR;
  else if (bindType == 1)
    bindOption = SCTP_BINDX_REM_ADDR;
  else {
    printf("bindx: invalid 'type'=%u\n", bindType);
     return -1;
  }
  /* get the address and figure out what kind it is */
  optval = (struct sctp_getaddresses *)bindx_ss;
  optval->sget_assoc_id = 0;	/* for the entire endpoint */
  /* try IPv6 first... */
  sin6 = (struct sockaddr_in6 *)&optval->addr;
  sin = (struct sockaddr_in *)&optval->addr;
  if (inet_pton(AF_INET6, argv[1], &sin6->sin6_addr)) {
    /* ipv6 address specified */
    sin6->sin6_family = AF_INET6;
    sin6->sin6_len = sizeof(struct sockaddr_in6);
    sin6->sin6_scope_id = scope_id;
  } else if (inet_pton(AF_INET, argv[1], &sin->sin_addr)) {
    /* ipv4 address specified */
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(struct sockaddr_in);
  } else {
    printf("bindx: invalid address\n");
    return -1;
  }
  /* do the bindx... */
  /* FIX: put in library/syscall later... */
  if (setsockopt(adap->fd, IPPROTO_SCTP, bindOption, 
		bindx_ss, sizeof(bindx_ss)) != 0) {
    printf("Failed bindx() on socket err:%u!\n", errno);
  } else {
    printf("bindx() completed\n");
  }
  return 0;
}

/* bulk size stream count - send a bulk of messages
 */
static int
cmd_bulk(char *argv[], int argc)
{
    int fd = adap->fd;
    int ret;

    if (argc < 3) {
	printf("bulk: expected 3 arguments\n");
	return -1;
    }
    bulkBufSize = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
    bulkStream  = argv[1] != NULL ? strtol(argv[1], NULL, 0) : 0;
    bulkCount   = argv[2] != NULL ? strtol(argv[2], NULL, 0) : 0;
    if(bulkInProgress){
	printf("bulk: sorry bulk already in progress\n");
	return -1;
    }
    if(bulkBufSize > SCTP_MAX_READBUFFER){
	printf("bulk: size %d is to large, overriding to largest I can"
	       "handle %d\n", bulkBufSize, SCTP_MAX_READBUFFER);
	bulkBufSize = SCTP_MAX_READBUFFER;
    }else if (bulkBufSize <= 0) {
	printf("bulk: size must be positive\n");
	return -1;
    }
    if (bulkCount <= 0) {
	printf("bulk: count must be positive\n");
	return -1;
    }
    /* prepare ping buffer */
    /* ask for the time */
    if(bulkPingMode == 0){
      strcpy(pingBuffer,"time");
      ret = sctpSEND(fd,bulkStream,pingBuffer,5,SCTP_getAddr(),sendOptions,payload,0);
      strncpy(pingBuffer,"bulk",4);
    }
    bulkInProgress = 1;
    checkBulkTranfer((void *)fd,NULL);
    if(bulkCount == 0){
	printf("bulk: bulk message are now queued\n");
    }else{
	printf("bulk: bulk transfer now in progress\n");
    }
    return 0;
}


/* bulkseen - display count of bulk packets seen (time resets)
 */
static int
cmd_bulkstat(char *argv[], int argc)
{
  int printed = 0;
  if(bulkSeen){
    struct tm *timeptr;
    timeptr = localtime(&time_started);
    printf("bulk seen is %d\n",bulkSeen);
    printf("last recorded time ");
    printf("%s",asctime(timeptr));
    printed++;
  }
  if(bulkInProgress){
    printf("bulk count left is %d\n",bulkCount);
    printed++;
  }
  if(!printed){
    printf("No activity\n");
  }
  return 0;
}


/* chgcookielife val - change the current assoc cookieLife
 */
static int
cmd_chgcookielife(char *argv[], int argc)
{
  int num;
  struct sctp_assocparams sasoc;

  if (argc < 1) {
    printf("defretrys: expected 1 or 2 argument\n");
    return -1;
  }
  num = argv[0] != NULL ? strtoul(argv[0], NULL, 0) : 0;
  if(num == 0){
    printf("Sorry argument 1 must be a postive number\n");
    return -1;
  }
  memset(&sasoc,0,sizeof(sasoc));
  sasoc.sasoc_assoc_id = get_assoc_id();
  if(sasoc.sasoc_assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }
  /* Set in the value */
  sasoc.sasoc_cookie_life = num;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, sizeof(sasoc)) != 0) {
    printf("Failed to set option %d\n",errno);
    return 0;
  }else{
    printf("Ka-pla\n");
  }
  return 0;
}


/* continual num - set continuous init to num times
 */
static int
cmd_continual(char *argv[], int argc)
{
    int num,y;

    if (argc < 1) {
	printf("continual: expected 1 argument current cont:%d\n",
	       SCTP_getContinualInit());
	return -1;
    }
    num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
    y = SCTP_setContinualInit(num);
    printf("continual INIT set to %d\n",y);
    return 0;
}

static int cmd_clrcwndlog(char *argv[], int argc)
{
	int buf,siz;
	buf = 0;
	siz = sizeof(buf);

	if(setsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_CLR_STAT_LOG, &buf, siz) != 0) {
		printf("Could not clr log error %d\n",errno);
		return 0;
	}
	printf("cwnd log cleared\n");
	return 0;
}


static int cmd_cwndlog(char *argv[], int argc)
{
	char *cwnd_names[] ={
		"Unknown-L",
		"Fast-rxt ",
		"T3-timer ",
		"Burst-lim",
		"SlowStart",
		"CongAvoid",
		"Satellite",
		"Block    ",
		"Un-Block ",
		"Chk-Block",
		"Del-2Strm",
		"Del-Immed",
		"Insert-hd",
		"Insert-md",
		"Insert-tl",
		"Mark_tsn ",
		"Express-D",
		"Unknown-H"
	};
	struct sctp_cwnd_log_req *req;
	u_int8_t buf[2048];
	int siz,i;
	int idx;
	memset(buf,0,sizeof(buf));
	req = (struct sctp_cwnd_log_req *)buf;
	siz = sizeof(buf);
	if(getsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_GET_STAT_LOG, buf, &siz) != 0) {
		printf("Could not get log error %d\n",errno);
		return 0;
	}
	printf("num_in_log:%d returned:%d start:%d end:%d\n",
	       req->num_in_log,req->num_ret,req->start_at,
	       req->end_at);
	for(i=0;i<req->num_ret;i++){
		idx = req->log[i].from;
		if(idx < SCTP_CWND_LOG_FROM_FR) {
			idx = 0;
		}
		if(idx > SCTP_STR_LOG_FROM_EXPRS_DEL) {
			idx = SCTP_STR_LOG_FROM_EXPRS_DEL+1;
		}
		if(req->log[i].event_type == SCTP_LOG_EVENT_CWND) {
			printf("%d: net:%x chg:%d cwnd:%d flight:%d  by:%s\n",
			       i,
			       (u_int)req->log[i].x.cwnd.net,
			       (req->log[i].x.cwnd.cwnd_augment*1024),
			       (req->log[i].x.cwnd.inflight*1024),
			       (int)req->log[i].x.cwnd.cwnd_new_value,
			       cwnd_names[idx]);
		} else if (SCTP_LOG_EVENT_BLOCK) {
			printf("%s: maxmb:%d onmb:%d maxsb:%d onsb:%d send_cnt:%d strcnt:%d\n",
			       cwnd_names[idx],
			       (req->log[i].x.blk.maxmb*1024),
			       (req->log[i].x.blk.onmb *1024),
			       (req->log[i].x.blk.maxsb*1024),
			       (req->log[i].x.blk.onsb*1024),
			       req->log[i].x.blk.send_sent_qcnt,
			       req->log[i].x.blk.stream_qcnt
			       );
		} else if (SCTP_LOG_EVENT_STRM) {
			   if((idx == SCTP_STR_LOG_FROM_INSERT_MD) ||
			      (idx == SCTP_STR_LOG_FROM_INSERT_TL)) {
				   /* have both the new entry and 
				    * the previous entry to print.
				    */
				   printf("%s: tsn=%u sseq=%u %s tsn=%u sseq=%u",
					  cwnd_names[idx],
					  req->log[i].x.strlog.n_tsn,
					  req->log[i].x.strlog.n_sseq,
					  ((idx == SCTP_STR_LOG_FROM_INSERT_MD) ? "Before" : "After"),
					  req->log[i].x.strlog.e_tsn,
					  req->log[i].x.strlog.e_sseq
					   );
			   }else {
				   printf("%s: tsn=%u sseq=%u",
					  cwnd_names[idx],
					  req->log[i].x.strlog.n_tsn,
					  req->log[i].x.strlog.n_sseq);
			   }
		}

	}
	return 0;
}
/* defretryi num - set a new association failure threshold for initing
 */
static int
cmd_defretryi(char *argv[], int argc)
{
  int num;
  struct sctp_initmsg sinit;

  if (argc < 1) {
    printf("defretryi: expected 1 argument\n");
    return -1;
  }
  num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  if(num){
    memset(&sinit,0,sizeof(sinit));
    sinit.sinit_max_attempts = num;
    if(setsockopt(adap->fd,IPPROTO_SCTP,
		  SCTP_INITMSG, &sinit, sizeof(sinit)) != 0) {
      printf("Failed to set option %d\n",errno);
      return 0;
    }else{
      printf("Ka-pla\n");
    }
  }else{
    printf("Sorry argument must be at least 1\n");
  }
  return 0;
}


/* defretrys num - set a new association failure threshold for sending
 */
static int
cmd_defretrys(char *argv[], int argc)
{
  int num;
  struct sctp_assocparams sasoc;

  if (argc < 1) {
    printf("defretrys: expected 1 or 2 argument\n");
    return -1;
  }
  num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  if(num == 0){
    printf("Sorry argument 1 must be a postive number\n");
    return -1;
  }
  memset(&sasoc,0,sizeof(sasoc));
  /* Set in the value */
  sasoc.sasoc_asocmaxrxt = num;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, sizeof(sasoc)) != 0) {
    printf("Failed to set option %d\n",errno);
    return 0;
  }else{
    printf("Ka-pla\n");
  }
  /* Now set in the retry's for the assoc */
  return 0;
}


/* defrwnd num - set the default rwnd of the SCTP
 */
static int
cmd_defrwnd(char *argv[], int argc)
{
  /*    int fd = adap->fd;*/
    int num,optlen;

    if (argc < 1) {
	printf("defrwnd: expected 1 argument\n");
	return -1;
    }
    num = argv[0] != NULL ? strtoul(argv[0], NULL, 0) : 0;
    optlen = sizeof(num);
    if(num > 1500){
      setsockopt(adap->fd, SOL_SOCKET, SO_RCVBUF, &num, optlen);
    }
    return 0;
}


/* delip address [how] - delete ip address where how is the mask/action to pass
 */
static int
cmd_delip(char *argv[], int argc)
{
  /*
    int fd = adap->fd;
    char *address;
    struct addrinfo hints, *res;
    int how, ret;

    if (argc < 1) {
	printf("delip: expected at least 1 argument\n");
	return -1;
    }
    address = argv[0];
    how = argv[1] != NULL ? strtol(argv[1], NULL, 0) : 0;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
  */

    /*
     ret = getaddrinfo(address, NULL, &hints, &res);
     if (ret != 0) {
 	printf("delip: getaddrinfo: %s\n", gai_strerror(ret));
 	return -1;
     }
     printf("delip: deleting IP address %s, action 0x%x\n", address, how);
     ret = sctpDELIPADDRESS(fd, res->ai_addr, SCTP_getAddr(), how);
     if (ret != 1) {
 	printf("delip: failed, return value: %d\n", ret);
     } else {
 	printf("delip: success\n");
     }
     freeaddrinfo(res);
    return ret != 1 ? -1 : 0;
    */
    return 0;
}


/* doheartbeat - preform a on demand HB
 */
static int
cmd_doheartbeat(char *argv[], int argc)
{
  struct sockaddr *sa;
  struct sctp_paddrparams sp;
  int ret;
  int siz;
  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
  sp.spp_hbinterval = 0xffffffff;
  /* Disable this so we don't change it */

  if(setsockopt(adap->fd,IPPROTO_SCTP,
		    SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
	printf("Failed to set option %d\n",errno);
	return 0;
  }
  printf("Request returns %d\n",ret);
  return 0;
}


/* getcurcookielife - display current assoc cookieLife
 */
static int
cmd_getcwndpost(char *argv[], int argc)
{
  printf("Command no longer supported\n");
  return 0;
}

static int
cmd_getcurcookielife(char *argv[], int argc)
{
  int siz;
  struct sctp_assocparams sasoc;
  siz = sizeof(sasoc);
  memset(&sasoc,0,sizeof(sasoc));
  /* Set in the value */
  sasoc.sasoc_assoc_id = get_assoc_id();
  if(sasoc.sasoc_assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }

  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, &siz) != 0) {
    printf("Failed to set option %d\n",errno);
    return -1;
  }
  printf("Association cookie life is set to %d\n",
	 sasoc.sasoc_cookie_life);
  return 0;
}


/* getdefcookielife - display default cookie life
 */
static int
cmd_getdefcookielife(char *argv[], int argc)
{
  int siz;
  struct sctp_assocparams sasoc;
  siz = sizeof(sasoc);
  memset(&sasoc,0,sizeof(sasoc));
  /* Set in the value */
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, &siz) != 0) {
    printf("Failed to set option %d\n",errno);
    return -1;
  }
  printf("Endpoint cookie life is set to %d\n",
	 sasoc.sasoc_cookie_life);
  return 0;
}


/* gethbdelay - get the hb delay
 */
static int
cmd_gethbdelay(char *argv[], int argc)
{
  struct sctp_paddrparams sp;
  struct sockaddr *sa;
  int siz;
  int useaddr;
  useaddr = 1;
  if(argc == 1){
    if(strcmp("ep",argv[0]) == 0){
      useaddr = 0;
    }
  }
  siz = sizeof(sp);
  if(useaddr)
	  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  if(useaddr)
    memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
    printf("Failed to get HB info err:%d\n",errno);
    return 0;
  }
  if(sp.spp_assoc_id) {
	  printf("Current failure threshold is %d\n",sp.spp_pathmaxrxt);
	  printf("Current HB interval is %d\n",sp.spp_hbinterval);
	  printf("Assoc id is %x\n",(u_int)sp.spp_assoc_id);
  } else {
	  printf("EP default failure threshold is %d\n",sp.spp_pathmaxrxt);
	  printf("EP default HB interval is %d\n",sp.spp_hbinterval);
  }
  return 0;
}


/* getpegs - retrieve the peg counts
 */

static int cmd_getpcbinfo(char *argv[], int argc)
{
  struct sctp_pcbinfo optval;
  int siz;
  siz = sizeof(optval);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PCB_STATUS, &optval, &siz) != 0) {
    printf("Can't get PCB status on socket err:%d!\n",errno);
  }else{
    printf("Number of SCTP endpoints is %d\n",optval.ep_count);
    printf("Number of SCTP associations  is %d\n",optval.asoc_count);
    printf("Number of SCTP laddr's in use is %d\n",optval.laddr_count);
    printf("Number of SCTP raddr's in use is %d\n",optval.raddr_count);
    printf("Number of SCTP chunks in use is %d\n",optval.chk_count);
    printf("Number of SCTP sockq in use is %d\n",optval.sockq_count);
    printf("Mbuf track:%d\n",optval.mbuf_track);
  }
  return 0;
}

static int
cmd_clrpegs(char *argv[], int argc)
{
	int sctp_pegs,siz;
	sctp_pegs = 0;
	siz = sizeof(sctp_pegs);
	if(setsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_RESET_PEGS,
		      &sctp_pegs,siz) != 0){
		printf("Error %d could not clear pegs\n",errno);
	} else {
		printf("pegs cleared\n");
	}
	return 0;
}

static int
cmd_getpegs(char *argv[], int argc)
{
    u_int32_t sctp_pegs[SCTP_NUMBER_OF_PEGS];
    int siz,i;
    static char *pegnames[SCTP_NUMBER_OF_PEGS] = {
	"sack_rcv", /* 00 */
	"sack_snt", /* 01 */
	"tsns_snt", /* 02 */
	"tsns_rcv", /* 03 */
	"pkt_sent", /* 04 */
	"pkt_rcvd", /* 05 */
	"tsns_ret", /* 06 */
	"dup_tsns", /* 07 */
	"hbs__rcv", /* 08 */
	"hbackrcv", /* 09 */
	"htb__snt", /* 10 */
	"win_prbe", /* 11 */
	"pktswdat", /* 12 */
	"t3-timeo", /* 13 */
	"dsack-to", /* 14 */
	"hb_timer", /* 15 */
	"fst_rxts", /* 16 */
	"timerexp", /* 17 */
	"fr_inwin", /* 18 */
	"blk_rwnd", /* 19 */
	"blk_cwnd", /* 20 */
	"rwnd_drp", /* 21 */
	"bad_strm", /* 22 */
	"bad_ssnw", /* 23 */
	"drpnomem", /* 24 */
	"drpfragm", /* 25 */
	"badvtags", /* 26 */
	"badcsumv", /* 27 */
	"packetin", /* 28 */
	"mcastrcv", /* 29 */
	"hdrdrops", /* 30 */
	"no_portn", /* 31 */
	"cwnd_nf ", /* 32 */
	"co_snds ", /* 33 */
	"co_nodat", /* 34 */
	"cw_nu_ss", /* 35 */
	"max_brst", /* 36 */
	"expr_del", /* 37 */
	"no_cp_in", /* 38 */
	"cach_src", /* 39 */
	"cw_nocum", /* 40 */
	"cw_incss", /* 41 */
	"cw_incca", /* 42 */
	"cw_skip ", /* 43 */
	"cw_nu_ca", /* 44 */
	"cw_maxcw", /* 45 */
	"diff_ss ", /* 46 */
	"diff_ca ", /* 47 */
	"tqs @ ss", /* 48 */
	"sqs @ ss", /* 49 */
	"tqs @ ca", /* 50 */
	"sqq @ ca", /* 51 */
	"lmtu_mov", /* 52 */
	"lcnt_mov", /* 53 */
	"sndqctss", /* 54 */
	"sndqctca", /* 55 */
	"movemax ", /* 56 */
	"move_equ", /* 57 */
	"nagle_qo", /* 58 */
	"nagle_of", /* 59 */
	"out_fr_s", /* 60 */
	"sostrnos", /* 61 */
	"nostrnos", /* 62 */
	"sosnqnos", /* 63 */
	"nosnqnos", /* 64 */
	"intoperr", /* 65 */
	"dupssnrc", /* 66 */
	"multi-fr", /* 67 */
	"vtag-exp", /* 68 */
	"vtag-bog", /* 69 */
	"t3-safeg", /* 70 */
	"pd--mbox", /* 71 */
	"pd-ehost", /* 72 */
	"pdmb_wda", /* 73 */
	"pdmb_ctl", /* 74 */
	"pdmb_bwr", /* 75 */
	"pd_corup", /* 76 */
	"pd_nedat", /* 77 */
	"pd_errpd", /* 78 */
	"fst_prep", /* 79 */
	"pd_daNFo", /* 80 */
	"pd_dIWin", /* 81 */
	"pd_dIZrw", /* 82 */
	"pd_BadDa", /* 83 */
	"pd_dMark", /* 84 */
	"ecne_rcv", /* 85 */
	"cwr_perf", /* 86 */
	"ecne_snt", /* 87 */
    };
    printf("there are %d pegs\n",
	   SCTP_NUMBER_OF_PEGS);
    siz = sizeof(sctp_pegs);
    if(getsockopt(adap->fd,IPPROTO_SCTP,
		  SCTP_GET_PEGS,
		  sctp_pegs,&siz) != 0){
	printf("Can't get peg counts err:%d\n",errno);
	return(0);
    }
    for(i=0;i<SCTP_NUMBER_OF_PEGS;i+=4){
	if((i+1 > SCTP_NUMBER_OF_PEGS) ||
	   (i+2 > SCTP_NUMBER_OF_PEGS) ||
	   (i+3 > SCTP_NUMBER_OF_PEGS)){
	    break;
	}
	  
	printf("%s:0x%8.8x %s:0x%8.8x %s:0x%8.8x %s:0x%8.8x\n",
	       pegnames[i],sctp_pegs[i],
	       pegnames[(i+1)],sctp_pegs[(i+1)],
	       pegnames[(i+2)],sctp_pegs[(i+2)],
	       pegnames[(i+3)],sctp_pegs[(i+3)]);
    }
    if((SCTP_NUMBER_OF_PEGS%4) != 0){
	i-=4;
	for(;i<SCTP_NUMBER_OF_PEGS;i++){
	    printf("%s:0x%8.8x ",
		   pegnames[i],sctp_pegs[i]);
	}
	printf("\n");
    }
    return(0);
}


/* getprimary - tells which net number is primary
 */
static int
cmd_getprimary(char *argv[], int argc)
{
  struct sockaddr *sa;
  struct sctp_setprim prim;
  int siz;
  memset(&prim,0,sizeof(prim));
  sa = SCTP_getAddr();
  siz = sizeof(prim);
  memcpy(&prim.ssp_addr,(caddr_t)sa,sa->sa_len);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PRIMARY_ADDR,&prim,&siz) != 0) {
    printf("Can't get primary address error:%d\n",errno);
    return -1;
  }
  printf("Primary address is ");
  SCTPPrintAnAddress((struct sockaddr *)&prim.ssp_addr);
  return 0;
}

static int
cmd_getroute(char *argv[], int argc)
{
	printf("code nolonger supported (for now :>)\n");
	return(0);
}
/* getrtt - Return the RTO of the current default address of the assoc
 */


static int
cmd_getrtt(char *argv[], int argc)
{
  struct sctp_paddrinfo so;
  struct sockaddr *sa;
  int siz;

  sa = SCTP_getAddr();
  memset(&so,0,sizeof(so));
  memcpy((caddr_t)&so.spinfo_address,sa,sa->sa_len);
  siz = sizeof(so);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_GET_PEER_ADDR_INFO, &so, &siz) != 0) {
    printf("Failed to get dest info err:%d\n",errno);
    return 0;
  }
  printf("Destination state is %x\n",so.spinfo_state);
  printf("Destination RTO is %d\n",so.spinfo_rto);
  printf("Destination srtt is %d\n",so.spinfo_srtt);
  printf("Destination cwnd is %d\n",so.spinfo_cwnd);
  return 0;
}

static int
cmd_getsnd(char *argv[], int argc)
{
	struct sctp_sockstat so;
	int siz;

	if(argc != 1){
		printf("Sorry - getsnd requires associd argument\n");
		return -1;
	}
	memset(&so,0,sizeof(so));
	so.ss_assoc_id = (caddr_t)strtoul(argv[0],NULL,0);
	siz = sizeof(so);
	if(getsockopt(adap->fd,IPPROTO_SCTP,SCTP_GET_SNDBUF_USE,
		      &so, &siz) != 0) {
		printf("Failed to get dest info err:%d\n",errno);
		return 0;
	}
	printf("For association %x\n",(u_int)so.ss_assoc_id);
	printf("Total sndbuf %d\n",(int)so.ss_total_sndbuf);
	printf("Total sndbuf mbuf %d\n",(int)so.ss_total_mbuf_sndbuf);
 	printf("Total in recvbuf %d\n",(int)so.ss_total_recv_buf);
	return 0;
}


/* heart on/off - Turn HB on or off to the destination
 */
static int
print_peer_addr_info(struct sctp_paddrinfo *so)
{
  struct sctp_paddrparams sp;

  struct sockaddr *sa;
  int siz;
  siz = sizeof(sp);
  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  memset(so,0,sizeof(so));
  memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
  memcpy((caddr_t)&so->spinfo_address,sa,sa->sa_len);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
    printf("Failed to get HB info err:%d\n",errno);
    return 0;
  }
  siz = sizeof(so);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_GET_PEER_ADDR_INFO, so, &siz) != 0) {
    printf("Failed to get dest info err:%d\n",errno);
    return 0;
  }
  printf("Current fail thresh is %d\n",sp.spp_pathmaxrxt);
  printf("Current HB interval is %d\n",sp.spp_hbinterval);
  printf("Destination state is %x\n",so->spinfo_state);
  printf("Destination RTO is %d\n",so->spinfo_rto);
  printf("Destination srtt is %d\n",so->spinfo_srtt);
  printf("Destination cwnd is %d\n",so->spinfo_cwnd);
  printf("Assoc id is %x\n",(u_int)sp.spp_assoc_id);
  /* Disable this so we don't change it */
  return(0);
}


static int
cmd_heart(char *argv[], int argc)
{
	struct sctp_paddrparams sp;
	struct sctp_paddrinfo so;
	struct sockaddr *sa;
	int siz;
	char *bool;

	siz = sizeof(sp);
	if (argc != 1) {
		printf("heart: expected 1 argument\n");
		return -1;
	}
	bool = argv[0];
	sa = SCTP_getAddr();
	memset(&sp,0,sizeof(sp));
	memset(&so,0,sizeof(so));

	if(strcmp(bool, "on") == 0){
		print_peer_addr_info(&so);
		if(so.spinfo_state & SCTP_ADDR_NOHB){
			siz = sizeof(sp);
			memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
			if(sp.spp_hbinterval == 0){
				/* Set to a postive value */
				sp.spp_hbinterval = 30000;
			}
			if(setsockopt(adap->fd,IPPROTO_SCTP,
				      SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
				printf("Failed to set option %d\n",errno);
				return 0;
			}
		} else {
			printf("Heartbeat is already off\n");
		}
	}else if(strcmp(bool, "off") == 0) {
		print_peer_addr_info(&so);
		if(so.spinfo_state & SCTP_ADDR_NOHB){
			printf("Destination is already NOT heartbeating\n");
		}else{
			sp.spp_hbinterval = 0;
			siz = sizeof(sp);
			if(setsockopt(adap->fd,IPPROTO_SCTP,
				      SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
				printf("Failed to set option %d\n",errno);
				return 0;
			}
		}
	}else if(strcmp(bool, "alloff") == 0) {
		memset(&sp,0,sizeof(sp));
		if(getsockopt(adap->fd,IPPROTO_SCTP,
			      SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
			printf("Failed to get HB info err:%d\n",errno);
			return 0;
		}
		if(sp.spp_hbinterval){
			memset((caddr_t)&sp,0,sizeof(sp));
			sp.spp_hbinterval = SCTP_NO_HB;
			if(setsockopt(adap->fd,IPPROTO_SCTP,
				      SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
				printf("Failed to set option %d\n",errno);
				return 0;
			}
		}else{
			printf("All heartbeats already off\n");
		}
		memset(&sp,0,sizeof(sp));
		if(getsockopt(adap->fd,IPPROTO_SCTP,
			      SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
			printf("Failed to get HB info err:%d\n",errno);
			return 0;
		}
		printf("Current fail thresh is %d\n",sp.spp_pathmaxrxt);
		printf("Current HB interval is %d\n",sp.spp_hbinterval);
		printf("Assoc id is %x\n",(u_int)sp.spp_assoc_id);
	}else if(strcmp(bool, "allon") == 0) {
		memset((caddr_t)&sp.spp_address,0,sizeof(sp.spp_address));
		if(getsockopt(adap->fd,IPPROTO_SCTP,
			      SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
			printf("Failed to get HB info err:%d\n",errno);
			return 0;
		}
		if(sp.spp_hbinterval){
			printf("Heartbeat already on and set to %d\n",sp.spp_hbinterval);
		}else{
			/* set to the default 30 seconds */
			memset((caddr_t)&sp.spp_address,0,sizeof(sp.spp_address));
			sp.spp_hbinterval = 30000;
			if(setsockopt(adap->fd,IPPROTO_SCTP,
				      SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
				printf("Failed to set option %d\n",errno);
				return 0;
			}
		}
		memset(&sp,0,sizeof(sp));
		if(getsockopt(adap->fd,IPPROTO_SCTP,
			      SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
			printf("Failed to get HB info err:%d\n",errno);
			return 0;
		}
		printf("Current fail thresh is %d\n",sp.spp_pathmaxrxt);
		printf("Current HB interval is %d\n",sp.spp_hbinterval);
		printf("Assoc id is %x\n",(u_int)sp.spp_assoc_id);
	}else{
		printf("heart: expected on or off allon or alloff\n");
		return -1;
	}
	return 0;
}


/* heartdelay time - Add number of seconds + RTO to hb interval
 */
static int
cmd_heartdelay(char *argv[], int argc)
{
  struct sctp_paddrparams sp;
  struct sockaddr *sa;
  int siz,val;
  int setaddr;

  setaddr = 1;
  siz = sizeof(sp);
  if (argc < 1) {
    printf("heartdelay: expected 1 argument\n");
    return -1;
  }
  if(argc == 2){
    if(strcmp(argv[1],"ep") == 0){
      printf("I will set it to the base ep\n");
      setaddr = 0;
    }else{
      printf("Don't understand qualifier %s\n",argv[1]);
      return -1;
    }
  }
  val = strtol(argv[0],NULL,0);
  printf("val of hb set to %d\n",val);
  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  if(setaddr)
    memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);

  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
    printf("Failed to get HB info err:%d\n",errno);
    return 0;
  }
  printf("Current HB interval is %d\n",sp.spp_hbinterval);
  /* Disable this so we don't change it */
  sp.spp_pathmaxrxt = 0;
  memset((caddr_t)&sp.spp_address,0,sizeof(sp.spp_address));
  sp.spp_hbinterval = val;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		    SCTP_PEER_ADDR_PARAMS, &sp, siz) != 0) {
	printf("Failed to set option %d\n",errno);
	return 0;
  }
  return 0;
}


/* help [cmd] - display help for cmd, or for all the commands if no cmd specified
 */
static int
cmd_help(char *argv[], int argc)
{
    int i, printed;
    char *cmdname;
    struct command *cmd;

    printed = 0;
    cmdname = argv[0];
    if (cmdname != NULL) {
	if ( (cmd = find_command(cmdname)) != NULL) {
	    printf("%s\n", cmd->co_desc);
	} else {
	    printf("help: no match for `%s'. Commands are:", cmdname);
	    for (i = 0; commands[i].co_name != NULL; i++) {
		printf("%s", printed % 3 == 0 ? "\n" : "");
		printf("%-20.20s", commands[i].co_name);
		printed++;
	    }
	    printf("\n");
	}
    } else {
	for (i = 0; commands[i].co_name != NULL; i++) {
	    printf("%s\n", commands[i].co_desc);
	}
    }
    return 0;
}

static int
cmd_bulkstop(char *argv[], int argc)
{
    bulkCount = 0;
    return 0;
}

/* hulkstart filename - start the hulk hogan process
 */
static int
cmd_hulkstart(char *argv[], int argc)
{
    int fd = adap->fd;
    char *filename;

    if (argc < 1) {
	printf("hulkstart: Gak, no file name\n");
	return -1;
    }
    filename = argv[0];
    /* XXX [MM] symlink attack */
    hulkfile = fopen(filename, "w+");
    if(hulkfile != NULL){
      printf("hulk scheduler begun\n");
      dist_TimerStart(dist,
		      SCTPdataTimerTicks, 
		      0,period,(void *)NULL,(void *)fd);
    }else{
      printf("Can't open file err:%d for file '%s'\n",
	     errno,filename);
      return -1;
    }
    return 0;
}


/* hulkstop - stop the hulk hogan process
 */
static int
cmd_hulkstop(char *argv[], int argc)
{
    int fd = adap->fd;
    curSeq = 0;
    if(hulkfile != NULL)
      fclose(hulkfile);
    hulkfile = NULL;
    dist_TimerStop(dist,
		   SCTPdataTimerTicks, 
		      (void *)NULL,(void *)fd);
    return 0;
}


/* initmultping - init contexts for a fast test
 */
static int
cmd_initmultping(char *argv[], int argc)
{
  /*    int fd = adap->fd;*/

    /*    initPingPongTable(sctpGETNUMOUTSTREAMS(fd,SCTP_getAddr()));*/
  /*    printf("%d ping pong contexts are defined \n", pingPongsDefined);*/
  /*    printf("Type startmultping to proceed\n");*/
    return 0;
}


/* inqueue - report inqueue counts
 */
static int
cmd_inqueue(char *argv[], int argc)
{
  /*    int fd = adap->fd;*/

    /*    printf("Outbound queue count to dest = %d\n",
 	   sctpHOWMANYINQUEUE(m,SCTP_getAddr()));
	   printf("Inbound queue count = %d\n",sctpHOWMANYINBOUND(m));
    */
    return 0;
}


/* multping size stream count blockafter sec
 * - add a ping pong context and block after <blockafter> instances for
 *   <sec> seconds
 */
static int
cmd_multping(char *argv[], int argc)
{
    int  i;
    int myPingCount, myPingStream, myBlockAfter, myBlockDuring;

    if (argc != 5) {
	printf("multping: expected 5 arguments\n");
	return -1;
    }
    pingBufSize   = strtol(argv[0], NULL, 0);
    myPingStream  = strtol(argv[1], NULL, 0);
    myPingCount   = strtol(argv[2], NULL, 0);
    myBlockAfter  = strtol(argv[3], NULL, 0);
    myBlockDuring = strtol(argv[4], NULL, 0);
  
    if(pingBufSize <= 0 || pingBufSize > SCTP_MAX_READBUFFER){
	printf("size %d out of range, setting to largest I can handle %d\n",
	       pingBufSize,SCTP_MAX_READBUFFER);
	pingBufSize = SCTP_MAX_READBUFFER;
    }
    if(myPingStream < 0 || myPingStream > (MAX_PING_PONG_CONTEXTS - 1)){
	printf("stream %d out of range, setting to largest I can handle %d\n",
	       myPingStream, MAX_PING_PONG_CONTEXTS - 1);
	myPingStream = MAX_PING_PONG_CONTEXTS - 1;
    }
    if(myPingCount <= 0){
	printf("Invalid ping pong count\n");
	return -1;
    }
    if(myBlockAfter <= 0){
	printf("Invalid block after\n");
	return -1;
    }
    if(myBlockDuring <= 0){
	printf("Invalid block during\n");
	return -1;

    }

    for(i=0;i<MAX_PING_PONG_CONTEXTS;i++){
	if(pingPongTable[i].stream == -1){
	    pingPongTable[i].stream = myPingStream;
	    pingPongTable[i].counter = 0;
	    pingPongTable[i].nbRequests = myPingCount;
	    pingPongTable[i].blockAfter = myBlockAfter;
	    pingPongTable[i].blockDuring = myBlockDuring;
	    pingPongTable[i].blockTime  = 0;
	    break;
	}
    }
    pingPongsDefined++;
    printf("%d ping pong contexts are defined\n", pingPongsDefined);
    printf("Type startmultping to proceed\n");

    return 0;
}

struct sctp_getaddresses *
get_assoc_addresses(sctp_assoc_t id, int *numnets)
{
  struct sctp_getaddresses *addrs;
  struct sockaddr *sa;
  sctp_assoc_t asoc;
  caddr_t lim;
  int siz,cnt;

  asoc = id;
  siz = sizeof(sctp_assoc_t);  
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		  SCTP_GET_REMOTE_ADDR_SIZE, &asoc, &siz) != 0) {
    printf("Get sizes failed :< %d\n",errno);
    return(NULL);
  }
  if(siz < 0){
    printf("Sorry size returned is %d\n",siz);
    return(NULL);
  }
  siz = (u_int32_t)asoc;
  siz += sizeof(struct sctp_getaddresses);
  printf("Sizes for addresses are %d\n",siz);
  addrs = calloc(1,siz);
  if(addrs == NULL){
    printf("Calloc failed .. gak\n");
    return(NULL);
  }
  memset(addrs,0,siz);
  addrs->sget_assoc_id = id;
  /* Now lets get the array of addresses */
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_GET_PEER_ADDRESSES, addrs, &siz) != 0) {
    printf("get address info failed, err:%d\n",errno);
    free(addrs);
    return(NULL);
  }
  cnt = 0;
  sa = (struct sockaddr *)&addrs->addr[0];
  lim = (caddr_t)addrs + siz;
  while((caddr_t)sa < lim){
    if(sa->sa_len == 0)
      break;
    cnt++;
    sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
  }
  printf("returning %d addresses addr:%x\n",cnt, (u_int)addrs);
  if(numnets)
    *numnets = cnt;
  return(addrs);
}

static char *
sctp_network_state(u_int state)
{
    static char str_buf[256];
    int len;
    str_buf[0] = 0;
    if(state & SCTP_ADDR_NOT_REACHABLE) {
        strcat(str_buf,"UN-Reachable|");
    }
    if(state & SCTP_ADDR_REACHABLE) {
        strcat(str_buf,"Reachable|");
    }
    if(state & SCTP_ADDR_NOHB) {
        strcat(str_buf,"NO-HB|");
    }
    if(state & SCTP_ADDR_BEING_DELETED) {
        strcat(str_buf,"Being-Deleted|");
    }
    if(state & SCTP_ADDR_NOT_IN_ASSOC) {
        strcat(str_buf,"Not-In-Assoc|");
    }
    if(state & SCTP_ADDR_WAS_PRIMARY) {
        strcat(str_buf,"Was-Primary|");
    }
    if(state & SCTP_ADDR_SWITCH_PRIMARY){
        strcat(str_buf,"Primary-Switch|");
    }
    if(state & SCTP_ADDR_OUT_OF_SCOPE) {
        strcat(str_buf,"Out-Of-Scope|");
    }
    if(state & SCTP_ADDR_DOUBLE_SWITCH) {
        strcat(str_buf,"Double-Switch|");
    }
    if(state & SCTP_ADDR_UNCONFIRMED) {
        strcat(str_buf,"UNCONFIRMED");
    }
    len = strlen(str_buf);
    if(str_buf[(len-1)] == '|')
        str_buf[(len-1)] = 0;
    return(str_buf);
}


/* netstats - return all network stats
 */
static int
cmd_netstats(char *argv[], int argc)
{
    int siz,numnets,i;
    sctp_assoc_t assoc_id;
    struct sctp_getaddresses *addrs;
    struct sockaddr *sa;
    assoc_id = get_assoc_id();
    if(assoc_id == 0){
        printf("Can't find association\n");
        return(-1);
    }

    printf("Setting on assoc id %x\n",(u_int)assoc_id);
    /* Now we get the association parameters so we know howm
     * many networks there are.
     */
    addrs = get_assoc_addresses(assoc_id, &numnets);
    if(addrs == NULL){	
        printf("Can't get the addresses\n");
        return -1;
    }
    printf("There are %d destinations to the peer\n",numnets);
    sa = (struct sockaddr *)&addrs->addr[0];
    for(i=0;i<numnets;i++){
        struct sctp_paddrinfo addrwewant;
        memcpy(&addrwewant.spinfo_address,
               sa,
               sa->sa_len);
        addrwewant.spinfo_assoc_id = assoc_id;
        siz = sizeof(addrwewant);
        printf("Address[%d]:\n",i);
        SCTPPrintAnAddress(sa);
        /* Now get its specific info */
        if(getsockopt(adap->fd,IPPROTO_SCTP,
                      SCTP_GET_PEER_ADDR_INFO,
                      &addrwewant,&siz) != 0){
            printf("Failed to gather information, err:%d\n",errno);
        }else{
            printf("State:%s,",sctp_network_state((u_int)addrwewant.spinfo_state));
            printf("cwnd:%d,",addrwewant.spinfo_cwnd);
            printf("srtt:%d,",addrwewant.spinfo_srtt);
            printf("rto:%d\n",addrwewant.spinfo_rto);
        }
        sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
    }
    free(addrs);
    return 0;
}


/* ping size stream count - play ping pong
 */
static int
cmd_ping(char *argv[], int argc)
{
    int fd = adap->fd;

    if(pingPongCount){
	printf("Sorry ping-pong already in progress\n");
	return -1;
    }
    if (argc != 3) {
	printf("ping: expected 3 arguments\n");
	return -1;
    }
    pingBufSize   = strtol(argv[0], NULL, 0);
    pingStream    = strtol(argv[1], NULL, 0);
    pingPongCount = strtol(argv[2], NULL, 0);
    if(pingBufSize > SCTP_MAX_READBUFFER){
	printf("%d is to large, override to largest I can handle %d\n",
	       pingBufSize,SCTP_MAX_READBUFFER);
	pingBufSize = SCTP_MAX_READBUFFER;
    }
    /* XXX no check for pingStream */
    if(pingPongCount <= 0){
	printf("count must be positive\n");
	return -1;
    }
    printf("Starting PING size:%d stream:%d\n",pingBufSize,pingStream);
    doPingPong(fd);
    return 0;
}


/* quit - quit the program
 */
static int
cmd_quit(char *argv[], int argc)
{
    dist_setDone(dist);
    return 0;
}

static int
cmd_printrftpstat(char *argv[], int argc)
{
  printf("Stream 1 read %d\n",str1read);
  printf("Stream 2 read %d\n",str2read);
  str1read = str2read = 0;
  return 0;
}

/* rftp filename strm1 strm2 blocksz [n] - round-trip ftp
 */
static int
cmd_rftp(char *argv[], int argc)
{
    int fd = adap->fd;

    char cbuf[255];
    char *file_in;

    if((pingPongCount>0) || bulkInProgress){
	printf("Sorry bulk or ping in progress, you must wait for completion"
	       "before sending\n");
	return -1;
    }
    if (argc < 4) {
	printf("rftp needs at least 4 parameters, see help\n");
	return -1;
    }
    file_in = argv[0];
    rftp_in1 = fopen(file_in, "r");
    if(rftp_in1 == NULL){
      printf("Can't open file for strm1 err:%d for file '%s'\n", errno,file_in);
      return -1;
    }

    rftp_in2 = fopen(file_in, "r");
    if(rftp_in2 == NULL){
      printf("Can't open file for strm2 err:%d for file '%s'\n", errno,file_in);
      return -1;
    }
    
    rftp_strm1 = atoi(argv[1]);
    rftp_strm2 = atoi(argv[2]);
    rftp_bsz = atoi(argv[3]);
    if(argc == 5) 
      rftp_rt = atoi(argv[4]);
    else 
      rftp_rt = 0; /* unreliable for u-streams */
    sprintf(cbuf,"%s.strm%d.out",file_in,rftp_strm1);
    rftp_out1 = fopen(cbuf, "w+");
    if(rftp_out1 == NULL){
      printf("Can't open file err:%d for file '%s'\n", errno, cbuf);
      return -1;
    }
    sprintf(cbuf,"%s.strm%d.out",file_in,rftp_strm2);
    rftp_out2 = fopen(cbuf, "w+");
    if(rftp_out2 == NULL){
      printf("Can't open file err:%d for file '%s'\n", errno, cbuf);
      return -1;
    }
    /*=====*/
    printf("rftp started over streams %d and %d....\n", rftp_strm1, rftp_strm2);
    rftp_ending1 = rftp_ending2 = 0;
    str1Flow = str2Flow=0;

    sendRftpTransfer((void *)fd, NULL);

    return 0;
}

static int
cmd_closefd(char *argv[], int argc)
{
  int fd;
  if(argc != 1){
    printf("Missing fd closefd must have a int arg\n");	
    return(-1);
  }
  fd = strtol(argv[0], NULL, 0);
  restore_SCTP_fd(adap);
  if(fd == adap->fd){
    printf("Sorry can't close the main listening fd:%d\n",fd);
  }else{
    close(fd);
  }
  return(0);
}


static int
cmd_setfd(char *argv[], int argc)
{
  int fd;
  if(argc != 1){
    printf("Missing fd setfd must have a int arg\n");	
    return(-1);
  }
  fd = strtol(argv[0], NULL, 0);
  adap->fd = fd;
  printf("Override fd to %d\n",fd);
  return(0);
}

static int
cmd_listen(char *argv[], int argc)
{
    int backlog;
    backlog = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
    if(listen(adap->fd, backlog)){
      printf("Listen with backlog=%d fail's error %d\n",
	     backlog,errno);
    }
    if(adap->model & SCTP_TCP_TYPE){
      if(backlog){
	printf("Turning on the listening flag\n");
	adap->model |= SCTP_TCP_IS_LISTENING;
      }else{
	printf("Turning off the listening flag\n");
	adap->model &= ~SCTP_TCP_IS_LISTENING;
      }
    }
    return(0);
}

static int
cmd_restorefd(char *argv[], int argc)
{
  restore_SCTP_fd(adap);
  return(0);
}

#ifndef SCTP_NO_TCP_MODEL
static int
cmd_peeloff(char *argv[], int argc)
{
  sctp_assoc_t assoc_id;
  int newfd;
  if(adap->model & SCTP_TCP_TYPE){
    printf("Peel off is not allowed on the TCP model, restart without -t\n");
    return(-1);
  }
  assoc_id = get_assoc_id();
  if(assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }
  printf("Peel off fd:%d assoc_id:%xh\n",
	 adap->fd,(u_int)assoc_id);
  /*newfd = sctp_peeloff(adap->fd,&assoc_id); */
  if(newfd < 0){
    printf("Peel-off failed errno:%d\n",errno);
  }else{
    printf("Peeled off fd is now set to %d\n",newfd);
    dist_addFd(adap->o,newfd,sctpFdInput,POLLIN,(void *)adap);
  }
  return(0);
}
#endif /* !SCTP_NO_TCP_MODEL */
static int cmd_getlocaladdrs(char *argv[], int argc)
{
  struct sockaddr *raddr;
  struct sockaddr *sa;
  int cnt,i;
  sctp_assoc_t id;
  id = get_assoc_id();
  raddr = NULL;
  printf("Got asoc id %x\n",(u_int)id);
  cnt = sctp_getladdrs(adap->fd,id,&raddr);
  printf("Cnt returned is %d\n",cnt);
  if(raddr != NULL){
    if(id)
      printf("Got %d addresses on my end of the association\n",cnt);
    else
      printf("Got %d addresses on my endpoint\n",cnt);
    sa = raddr;
    for(i=0;i<cnt;i++){
      printf("Address[%d] ",i);
      SCTPPrintAnAddress(sa);
      sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
      if(sa->sa_len == 0)
	break;
    }
    sctp_freeladdrs(raddr);
  }
  return(0);
}
static int
cmd_getstatus(char *argv[], int argc)
{
  struct sctp_status stat;
  int sz;
  memset(&stat,0,sizeof(stat));
  stat.sstat_assoc_id = get_assoc_id();
  sz = sizeof(struct sctp_status);
  if(stat.sstat_assoc_id == 0){
    printf("Can't get assoc id\n");
    return(-1);
  }
  if (getsockopt(adap->fd, IPPROTO_SCTP, SCTP_STATUS, &stat, &sz) != 0) {
    printf("Can't do GET_STATUS socket option! err:%d\n", errno);
    return(-1);
  }
  printf("Association Id:%x\n",(u_int)stat.sstat_assoc_id);
  printf("      State   :%x\n",(u_int)stat.sstat_state);
  printf("    unacked   :%x\n",(u_int)stat.sstat_unackdata);
  printf("    pending   :%x\n",(u_int)stat.sstat_penddata);
  printf("    in-strm   :%d\n",(int)stat.sstat_instrms);
  printf("   out-strm   :%d\n",(int)stat.sstat_outstrms);
  printf("   frag-point :%d\n",(int)stat.sstat_fragmentation_point);
  printf("    Primary   : ");
  SCTPPrintAnAddress((struct sockaddr *)&stat.sstat_primary.spinfo_address);
  printf("primary-state:%d \n",stat.sstat_primary.spinfo_state);
  printf("primary-cwnd :%d \n",stat.sstat_primary.spinfo_cwnd);
  printf("primary-rtt  :%d \n",stat.sstat_primary.spinfo_srtt);
  printf("primary-rto  :%d \n",stat.sstat_primary.spinfo_rto);
  printf("primary-mtu  :%d \n",stat.sstat_primary.spinfo_mtu);
  return(0);
}

static int
cmd_getevents(char *argv[], int argc)
{
  struct sctp_event_subscribe event;
  int sz;
  sz = sizeof(event);
  memset(&event,0,sizeof(event));
  if (getsockopt(adap->fd, IPPROTO_SCTP, SCTP_EVENTS, &event, &sz) != 0) {
    printf("Can't do SET_EVENTS socket option! err:%d\n", errno);
    return(-1);
  }
  printf("DATA I/O events %s be received\n",
	 (event.sctp_data_io_event ? "Will" : "Will NOT"));

  printf("Association events %s be received\n",
	 (event.sctp_association_event ? "Will" : "Will NOT"));

  printf("Address events %s be received\n",
	 (event.sctp_address_event ? "Will" : "Will NOT"));

  printf("Send failure events %s be received\n",
	 (event.sctp_send_failure_event ? "Will" : "Will NOT"));

  printf("Peer error events %s be received\n",
	 (event.sctp_peer_error_event ? "Will" : "Will NOT"));

  printf("Shutdown events %s be received\n",
	 (event.sctp_shutdown_event ? "Will" : "Will NOT"));

  printf("PD-API events %s be received\n",
	 (event.sctp_partial_delivery_event ? "Will" : "Will NOT"));

  printf("Adaption layer events %s be received\n",
	 (event.sctp_adaption_layer_event ? "Will" : "Will NOT"));
  return(0);
}

/* rwnd - get rwnds
 */
static int
cmd_rwnd(char *argv[], int argc)
{
  struct sctp_assocparams sasoc;
  sctp_assoc_t assoc_id;
  int sz;

  assoc_id = get_assoc_id();
  if(assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }

  printf("Setting on assoc id %x\n",(u_int)assoc_id);
  sasoc.sasoc_assoc_id = assoc_id;
  sz = sizeof(sasoc);  
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		  SCTP_ASSOCINFO, &sasoc, &sz) != 0) {
    printf("get associnfo failed err:%d\n",errno);
  }else{
    printf("My rwnd:%d peers rwnd:%d\n",
	   sasoc.sasoc_local_rwnd,
	   sasoc.sasoc_peer_rwnd);
  }
  return 0;
}


/* send string [n] - send string to a peer if a peer is set
 */
static int
cmd_send(char *argv[], int argc)
{
    int fd = adap->fd;
    int ret;

    if (argc < 1) {
	printf("send: expected at least 1 argument\n");
	return -1;
    }
    if (!(destinationSet && portSet)) {
	printf("sorry first set a destination before sending\n");
	return -1;
    }
    if((pingPongCount>0) || bulkInProgress){
	printf("Sorry bulk or ping in progress, you must wait for completion"
	       "before sending\n");
	return -1;
    }
    ret = sctpSEND(fd, defStream, argv[0], strlen(argv[0]), SCTP_getAddr(),
		   sendOptions, payload, 0);
    printf("Returned %d from the send\n",ret);
    return 0;
}

/* sendloop [num] - send test script loopback request of num size
 */
static int
cmd_sendloop(char *argv[], int argc)
{
    int fd = adap->fd;
    int x,ret;

    x = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
    if(x <= 0){
      printf("num was 0? defaulting to 64\n");
      x = 64;
    }
    ret = sendLoopRequest(fd,x);
    printf("Sent loop returned %d\n",ret);
    return 0;
}


/* sendloopend [num] - send test script loopback request of num size and terminate
 */
static int
cmd_sendloopend(char *argv[], int argc)
{
    int fd = adap->fd;
    int x;

    x = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
    if(x <= 0){
      printf("num was 0? defaulting to 64\n");
      x = 64;
    }
    printf("Send loop request returns:%d\n",sendLoopRequest(fd,x));
    sctpTERMINATE(fd,SCTP_getAddr());
    printf("Sent loop and terminate queued\n");
    return 0;
}


/* sendreltlv num - send a rel-tlv of num bytes of data
 */
static int
cmd_sendreltlv(char *argv[], int argc)
{
  /*
    int fd = adap->fd;
    int sz,ret;
    char *xxx;
    struct sctpParamDesc *TLVout;
    struct SCTP_association *asoc;
     struct sockaddr *add;

    if (argc != 1) {
	printf("sendreltlv: expected 1 argument\n");
	return -1;
    }
  */
    /* This is actually a violation, we are calling a module that we
     * really should not.. but this is so we can do
     * a test by dumpping inqueue anything we want for a REL-REQ.
     */
  /*
    add = SCTP_getAddr();

    asoc = SCTPfindAssociation(fd, (struct sockaddr *)add, &sz);
    if(asoc == NULL){
	printf("sorry can't find the association\n");
	return -1;
    }
  */
    /* sz = strtol(&readBuffer[11],NULL,0); */
  /*
    sz = strtol(argv[0], NULL, 0);
    if (sz <= 0) {
	printf("sendreltlv: num must be positive\n");
	return -1;
    }
    sz += sizeof(struct sctpParamDesc);
    xxx = calloc(1,sz);
    if (xxx == NULL) {
	printf("sendreltlv: sorry malloc failed.. queue fails\n");
	return -1;
    } else {
	u_short val,len,i;
	char buf[10];

	TLVout = (struct sctpParamDesc *)xxx;
	printf("TLV value:");
	fflush(stdout);
	fgets(buf,sizeof(buf),stdin);      
	val = (u_short)strtol(buf,NULL,0);
	len = sz;
	TLVout->paramType = htons(val);
	TLVout->paramLength = htons(len);
	for(i=0,len=sizeof(struct sctpParamDesc);len<sz;len++,i++){
	    printf("Value for byte %d (0-255)",i);
	    fflush(stdout);
	    fgets(buf,sizeof(buf),stdin);
	    val = strtol(buf,NULL,0);
	    if(val > 255){
		printf("Error input:%d to big, replacing with 255\n",val);
		xxx[len] = 255;
	    }else{
		xxx[len] = val;
	    }
	}
	printf("Delayed send (y, n, e, def=n):");
	fflush(stdout);
	fgets(buf,sizeof(buf),stdin);      
	if(buf[0] == 'y'){
	    ret = SCTPqueueARelReq(fd,asoc,TLVout,1);
	}else{
	    struct sctpParamDesc *TLVout2;	
	    TLVout2 = NULL;
	    if(buf[0] == 'e'){	
		xxx = calloc(1,sz);
		if(xxx != NULL){
		    memcpy(xxx,(char *)TLVout,sz);
		}
		TLVout2 = (struct sctpParamDesc *)xxx;
	    }
	    ret = SCTPqueueARelReq(fd,asoc,TLVout,0);
  */
	    /* echo out a copy again to queue */
  /*
	    if(TLVout2 != NULL){
		ret = SCTPqueueARelReq(fd,asoc,TLVout2,0);
	    }
	}
	printf("Queue of REL-REQ returns %d\n",ret);
	if(ret < 0){
	    free(xxx);
	}
    }
  */
    return 0;
}


/* setbulkmode ascii/binary - set bulk transfer mode
 */
static int
cmd_setbulkmode(char *argv[], int argc)
{
    if (argc != 1) {
	printf("setbulkmode: expected 1 argument\n");
	return -1;
    }
    switch (argv[0][0]) {
    case 'a':
	bulkPingMode = 0;
	printf("bulk/ping transfer set to ascii mode\n");
	break;
    case 'b':
	bulkPingMode = 1;
	printf("bulk/ping transfer set to binary mode\n");
	break;
    default:
	printf("setbulkmode: expected ascii/binary\n");
	return -1;
    }
    return 0;


}


/* setdefcookielife num - set default cookie life
 */
static int
cmd_setdefcookielife(char *argv[], int argc)
{
  int num;
  struct sctp_assocparams sasoc;

  if (argc < 1) {
    printf("defretrys: expected 1 or 2 argument\n");
    return -1;
  }
  num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  if(num == 0){
    printf("Sorry argument 1 must be a postive number\n");
    return -1;
  }
  memset(&sasoc,0,sizeof(sasoc));
  /* Set in the value */
  sasoc.sasoc_cookie_life = num;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, sizeof(sasoc)) != 0) {
    printf("Failed to set option %d\n",errno);
    return 0;
  }else{
    printf("Ka-pla\n");
  }
  return 0;
}


/* setdefstream num - set the default stream to
 */
static int
cmd_setdefstream(char *argv[], int argc)
{
    if (argc != 1) {
	printf("setdefstream: expected 1 argument\n");
	return -1;
    }
    /* XXX add validity check on stream number */
    defStream = strtol(argv[0], NULL, 0);
    return 0;
}


/* seterr num - set the association send error thresh
 */
static int
cmd_seterr(char *argv[], int argc)
{
  struct sctp_assocparams sasoc;
  struct sctp_paddrparams sp;
  struct sockaddr *sa;
  int siz,num;


  if (argc < 1) {
    printf("defretrys: expected 1 argument\n");
    return -1;
  }
  num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  if(num == 0){
    printf("Sorry argument 1 must be a postive number\n");
    return -1;
  }
  memset(&sasoc,0,sizeof(sasoc));
  /* Set in the value */
  sasoc.sasoc_asocmaxrxt = num;
  /* Get the association id so I can set
   * this only on the assoc.
   */

  siz = sizeof(sp);
  sa = SCTP_getAddr();
  memset(&sp,0,sizeof(sp));
  memcpy((caddr_t)&sp.spp_address,sa,sa->sa_len);
  if(getsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0) {
    printf("Failed to get assoc id with HB info err:%d\n",errno);
    return 0;
  }
  printf("Setting on assoc id %x\n",(u_int)sp.spp_assoc_id);
  sasoc.sasoc_assoc_id = sp.spp_assoc_id;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_ASSOCINFO, &sasoc, sizeof(sasoc)) != 0) {
    printf("Failed to set option %d\n",errno);
    return 0;
  }else{
    printf("Ka-pla\n");
  }
  return 0;
}


/* sethost host|X.Y.Z.A - set the destination host IP address
 */
static int
cmd_sethost(char *argv[], int argc)
{
    struct in_addr in;

    if (inet_pton(AF_INET, argv[0], &in)) {
	SCTP_setIPaddr(in.s_addr);
    }else{
	printf("can't set IPv4: incorrect address format\n");
	return -1;
    }
    destinationSet++;
    printf("To Set ");
    SCTPPrintAnAddress(SCTP_getAddr());
    return 0;
}


/* sethost6 host|xx:xx:xx...:xx - set the destination host IPv6 address
 */
static int
cmd_sethost6(char *argv[], int argc)
{
    struct in6_addr in6;

    if(inet_pton(AF_INET6, argv[0], &in6)){
	SCTP_setIPaddr6((u_char *)&in6);
    }else{
	printf("can't set IPv6: incorrect address format\n");
	return -1;
    }
    destinationSet++;
    printf("To Set ");
    SCTPPrintAnAddress(SCTP_getAddr());
    return 0;
}


/* setneterr net val - set the association network error thresh
 */
static int
cmd_setneterr(char *argv[], int argc)
{
  struct sctp_getaddresses *addrs;
  struct sctp_paddrparams paddr;
  sctp_assoc_t assoc_id;
  int numnets;
  int net,val;

  if (argc != 2) {
    printf("setneterr: expected 2 arguments\n");
    return -1;
  }
  net = strtol(argv[0], NULL, 0);
  val = strtol(argv[1], NULL, 0);
  if (val < 1) {
    printf("setneterr: val out of range\n");
    return -1;
  }
  /* Got to get the assoc id */
  assoc_id = get_assoc_id();
  if(assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }

  addrs = get_assoc_addresses(assoc_id, &numnets);
  if (net <= 0 || net >= numnets) {
    printf("setneterr: net out of range\n");
    free(addrs);
    return -1;
  }
  printf("Setting net:%d failure threshold to %d\n",
	 net,val);
  memset(&paddr,0,sizeof(paddr));
  SCTPPrintAnAddress((struct sockaddr *)&addrs->addr[net]);
  paddr.spp_assoc_id = assoc_id;
  memcpy(&paddr.spp_address,&addrs->addr[net],sizeof(paddr.spp_address));
  paddr.spp_pathmaxrxt = val;
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		    SCTP_PEER_ADDR_PARAMS, &paddr, sizeof(paddr)) != 0) {
    free(addrs);
    printf("Failed to set option %d\n",errno);
    return 0;
  }
  free(addrs);
  return 0;
}


static int
parse_send_opt(char *p)
{
  printf("Parsing option '%s'\n",p);
  if(strcmp(p,"prsctp") == 0){
    return(MSG_PR_SCTP_TTL);
  }else if(strcmp(p,"bufbnd") == 0){
    return(MSG_PR_SCTP_BUF);
  }else if(strcmp(p,"unord") == 0){
    return(MSG_UNORDERED);
  }else if(strcmp(p,"over") == 0){
    return(MSG_ADDR_OVER);
  }else if(strcmp(p,"none") == 0){
    return(0);
  }else{
    printf("Sorry option %s not known, value must be prsctp|bufbnd|unord|over|none\n",p);
  }
  return(0);
}
/* setopts val - set options to specified value
 */
static int
cmd_setopts(char *argv[], int argc)
{
  int len,i;
  char *p;
  if (argc != 1) {
    printf("setopts: expected 1 argument\n");
    return -1;
  }
  sendOptions = 0;
  len = strlen(argv[0]);
  p = argv[0];
  for(i=0;i<len;i++){
    if(argv[0][i] == '|'){
      argv[0][i] = 0;
      sendOptions |= parse_send_opt(p);
      p = &argv[0][(i+1)];
    }
  }
  sendOptions |= parse_send_opt(p);
  printf("Send options now set to %x\n",sendOptions);
  return 0;
}


/* setpay payloadt - set the payload type
 */
static int
cmd_setpay(char *argv[], int argc)
{
    if (argc != 1) {
	printf("setpay: expected 1 argument\n");
	return -1;
    }
    /* XXX add input validation */
    payload = strtol(argv[0], NULL, 0);
    printf("payloadtype set to %d\n",payload);
    return 0;
}


/* setport port - set the destination SCTP port number
 */
static int
cmd_setport(char *argv[], int argc)
{
  unsigned short pt;

  pt = (unsigned short)strtol(argv[0],NULL,0);
  SCTP_setport(htons(pt)); 
  printf("Address set to ");
  SCTPPrintAnAddress(SCTP_getAddr());
  portSet++;
  return 0;
}


static int cmd_setautoasconf(char *argv[], int argc)
{
	int num;
	if(argc < 1){
		printf("setautoasconf needs numric value 0/1\n");
		return -1;
	}
	num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;  
	if(setsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_AUTO_ASCONF,&num,sizeof(num)) != 0) {
		printf("Can't set auto-asconf flag error:%d\n",errno);
		return -1;
	}
	printf("auto asconf is now %s\n", ((num) ? "on" : "off"));
	return 0;
}

static int cmd_getautoasconf(char *argv[], int argc)
{
	int num, optlen;
	optlen = sizeof(num);
	if(getsockopt(adap->fd,IPPROTO_SCTP,
		      SCTP_AUTO_ASCONF,&num,&optlen) != 0) {
		printf("Can't get auto-asconf flag error:%d\n",errno);
		return -1;
	}
	printf("auto asconf is now %s\n", ((num) ? "on" : "off"));
	return 0;
}


/* setprimary - set current ip address to the primary address
 */
static int
cmd_setprimary(char *argv[], int argc)
{
  struct sctp_getaddresses *addrs;
  struct sockaddr *sa;
  struct sctp_setprim prim;
  int numnets,num,i;
  sctp_assoc_t assoc_id;
  int siz;

  if(argc < 1){
    printf("setprimary needs numric index of new primary\n");
    return -1;
  }
  num = argv[0] != NULL ? strtol(argv[0], NULL, 0) : 0;
  assoc_id = get_assoc_id();
  if(assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }

  addrs = get_assoc_addresses(assoc_id, &numnets);
  if((num > numnets) || (num < 0)){
    printf("Invalid address number range is 0 to %d\n",(numnets-1));
    free(addrs);
    return -1;
  }
  sa = (struct sockaddr *)&addrs->addr[0];
  for(i=0;i<numnets;i++){
	  if(i == num){
		  memcpy(&prim.ssp_addr,
			 sa,
			 sa->sa_len);
		  break;
	  }
	  sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
  }
  prim.ssp_assoc_id = assoc_id;
  free(addrs);
  siz = sizeof(prim);
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_PRIMARY_ADDR,&prim,sizeof(prim)) != 0) {
    printf("Can't set primary address error:%d\n",errno);
    return -1;
  }
  printf("Primary address is ");
  SCTPPrintAnAddress((struct sockaddr *)&prim.ssp_addr);
  return 0;
}


/* setremprimary address - set remote's primary address
 */
static int
cmd_setremprimary(char *argv[], int argc)
{
  char *address;
  struct addrinfo hints, *res;
  struct sctp_setpeerprim sspp;
  int ret;

  if (argc != 1) {
    printf("setremprimary: expected 1 argument\n");
    return -1;
  }
  address = argv[0];

  bzero(&hints, sizeof(hints));
  hints.ai_flags = AI_NUMERICHOST; /* disable name resolution */
  ret = getaddrinfo(address, NULL, &hints, &res);
  if(ret != 0){
    printf("setremprimary: getaddrinfo: %s\n", gai_strerror(ret));
    return -1;
  }
  printf("setremprimary: setting remote primary %s\n", address);
  memset(&sspp,0,sizeof(sspp));
  sspp.sspp_assoc_id = get_assoc_id();
  if(sspp.sspp_assoc_id == 0){
    printf("Can't find association\n");
    return(-1);
  }

  memcpy(&sspp.sspp_addr,res->ai_addr,res->ai_addr->sa_len);
  if(setsockopt(adap->fd,IPPROTO_SCTP,
		SCTP_SET_PEER_PRIMARY_ADDR,&sspp,sizeof(sspp)) != 0) {
    printf("Can't set peer primary address error:%d\n",errno);
    return -1;
  }else{
    printf("Requested Peer Primary address is ");
    SCTPPrintAnAddress((struct sockaddr *)&sspp.sspp_addr);
  }
  freeaddrinfo(res);
  return(0);
}


/* setscope num - Set the IPv6 scope id
 */
static int
cmd_setscope(char *argv[], int argc)
{
    u_int scop;

    if (argc != 1) {
	printf("setscope: expected 1 argument\n");
	return -1;
    }
    scop = (u_int)strtol(argv[0],NULL,0);
    /* XXX add input validation */
    SCTP_setIPv6scope(scop);
    printf("Set scope id of %d address\n",scop);
    SCTPPrintAnAddress(SCTP_getAddr());
    return 0;
}


/* setstreams numpreopenstrms - set the number of streams I request
 */
static int
cmd_setstreams(char *argv[], int argc)
{
  printf("Obsolete, use 'setinitparam'\n");
  return 0;
}


/* startmultping - start the defined ping pong contexts
 */
static int
cmd_startmultping(char *argv[], int argc)
{
    int fd = adap->fd;
    int i;
    time_t x;
    struct tm *timeptr;

    if(pingPongsDefined <= 0){
	printf("No ping pong contexts defined\n");
    }else{
	/* prepare ping buffer */
	pingBufSize = 500;
	strncpy(pingBuffer,"ping", 4);
	for(i = 4;i<pingBufSize;i++) {
	    pingBuffer[i] = 'A' + (i%26);
	}
	for(i = 0;i< MAX_PING_PONG_CONTEXTS;i++){
	    if((pingPongTable[i].stream != -1) &&
	       (pingPongTable[i].started == 0)){
		sctpSEND(fd,pingPongTable[i].stream,pingBuffer,pingBufSize,
			 SCTP_getAddr(),sendOptions,payload,0);      
		pingPongTable[i].started = 1;
		x = time(NULL);
		timeptr = localtime(&x);
		printf("Starting on stream %d at %s",pingPongTable[i].stream,
		       asctime(timeptr));
	    }
	}
    }
    return 0;
}


/* tella host - confess what translateIPAddress returns
 * XXX IPv4 only
 */
static int
cmd_tella(char *argv[], int argc)
{
    u_long x;

    if (argc != 1) {
	printf("tella: expected 1 argument\n");
	return -1;
    }
    x = ntohl(translateIPAddress(argv[0]));
    printf("Address is %d.%d.%d.%d\n",
	   (int)((x>>24) & 0x000000ff),
	   (int)((x>>16) & 0x000000ff),
	   (int)((x>>8) & 0x000000ff),
	   (int)(x & 0x000000ff)
	   );
    return 0;
}


/* term - terminate the set destination association (graceful shutdown)
 */
static int
cmd_term(char *argv[], int argc)
{
    int fd = adap->fd;

    if(adap->model & SCTP_TCP_TYPE){
      if(adap->model & SCTP_TCP_IS_LISTENING){
	restore_SCTP_fd(adap);
	if(fd == adap->fd)
	  printf("Sorry can't shutdown the listening socket\n");
	else
	  shutdown(fd,SHUT_RDWR);
      }else{
	shutdown(fd,SHUT_RDWR);
      }
    }else{
      sctpTERMINATE(fd,SCTP_getAddr());
    }
    return 0;
}

/* whereto - tell where the default sends
 */

static int
cmd_whereto(char *argv[], int argc)
{
    struct sockaddr *to;

    printf("Currently sending to:");
    to = SCTP_getAddr();
    SCTPPrintAnAddress(to);
    return 0;
}
