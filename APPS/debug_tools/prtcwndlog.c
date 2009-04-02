#include <stdio.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
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

#include "../../KERN/netinet/sctp_constants.h"
#include <netinet/sctp_uio.h>
#include <netinet/sctp.h>

static char *event_names[] = {
	"SCTP_LOG_EVENT_UNKNOWN", 	/* 00 */
	"SCTP_LOG_EVENT_CWND",    	/* 01 */
	"SCTP_LOG_EVENT_BLOCK",   	/* 02 */
	"SCTP_LOG_EVENT_STRM",	 	/* 03 */
	"SCTP_LOG_EVENT_FR ",	 	/* 04 */
	"SCTP_LOG_EVENT_MAP",	 	/* 05 */
	"SCTP_LOG_EVENT_MAXBURST ",	/* 06 */
	"SCTP_LOG_EVENT_RWND",	  	/* 07 */
	"SCTP_LOG_EVENT_MBCNT",	  	/* 08 */
	"SCTP_LOG_EVENT_SACK",	  	/* 09 */
	"SCTP_LOG_LOCK_EVENT",	  	/* 10 */
	"SCTP_LOG_EVENT_RTT",	  	/* 11 */
	"SCTP_LOG_EVENT_SB",	  	/* 12 */
	"SCTP_LOG_EVENT_NAGLE",	  	/* 13 */
	"SCTP_LOG_EVENT_WAKE",	  	/* 14 */
	"SCTP_LOG_MISC_EVENTS",	  	/* 15 */
	"SCTP_LOG_EVENT_CLOSE",	  	/* 16 */
	"SCTP_LOG_EVENT_MBUF",	  	/* 17 */
	"SCTP_LOG_CHUNK_PROC", 		/* 18 */
	"SCTP_LOG_ERROR_RET", 		/* 19 */
	"SCTP_LOG_MAX_EVENT",	  	/* 20 */
};

static char *from_str[]= {
	/* 0  */ "Unknown",
	/* 1  */ "FastRetransmit",
	/* 2  */ "T3-Timer",
	/* 3  */ "Max_burst",
	/* 4  */ "Slow-Start",
	/* 5  */ "Congestion-Avoidance",
	/* 6  */ "Satellite",
	/* 7  */ "Blocking",
	/* 8  */ "Wakeup",
	/* 9  */ "Check",
	/* 10 */ "Del-2Strm",
	/* 11 */ "Del-Immed",
	/* 12 */ "Insert-hd",
	/* 13 */ "Insert-md",
	/* 14 */ "Insert-tl",
	/* 15 */ "Mark_tsn ",
	/* 16 */ "Express-D",
	/* 17 */ "Set_biggest",
	/* 18 */ "Test_strike",
	/* 19 */ "Strike_second",
	/* 20 */ "T3-Timeout",
	/* 21 */ "slide_map",
	/* 22 */ "slide_range",
	/* 23 */ "slide_result",
	/* 24 */ "slide_clear_arry",
	/* 25 */ "slide_NONE",
	/* 26 */ "T3-Mark time",
	/* 27 */ "T3-Marked",
	/* 28 */ "T3-Stopped Marking",
	/* 29 */ "NOT-USED",
	/* 30 */ "FR-Marked",
	/* 31 */ "No Cwnd advance from SS",
	/* 32 */ "No Cwnd advance from CA",
	/* 33 */ "Max Burst Applied",
	/* 34 */ "Max IFP Queue Stops Output",
	/* 35 */ "Max Burst stops at Error",
	/* 36 */ "SACK increases rwnd",     
	/* 37 */ "SEND decreases rwnd",
	/* 38 */ "SACK sets rwnd",
	/* 39 */ "MBCNT Increase",
	/* 40 */ "MBCNT Decrease",
	/* 41 */ "MBCNT chkset",
	/* 42 */ "New SACK",
	/* 43 */ "Tsn Acked",
	/* 44 */ "TSN Revoked",
	/* 45 */ "Lock TCB",
	/* 46 */ "Lock INP",
	/* 47 */ "Lock Socket",
	/* 48 */ "Lock Socket-rcvbuf",
	/* 49 */ "Lock Socket-sndbuf",
	/* 50 */ "Lock Create", 
	/* 51 */ "Initial RTT",
	/* 52 */ "RTT Variance",
	/* 53 */ "SB Alloc",
	/* 54 */ "SB Free",
	/* 55 */ "SB result", 
	/* 56 */ "Duplicate TSN sent",
	/* 57 */ "Early FR",
	/* 58 */ "Early FR cwnd report",
	/* 59 */ "Early FR cwnd Start",
	/* 60 */ "Early FR cwnd Stop",
	/* 61 */ "Log from a Send",
	/* 62 */ "Log at Initialization",
	/* 63 */ "Log from T3 Timeout",
	/* 64 */ "Log from SACK",
	/* 65 */ "No new cumack",
	/* 66 */ "Log from a re-transmission",
	/* 67 */ "Check TSN to strike",
	/* 68 */ "chunk output completes",
	/* 69 */ "fill_out_queue_called",
	/* 70 */ "fill_out_queue_fills",
	/* 71 */ "log_free_sent",
	/* 72 */ "Nagle stops  transmit",
	/* 73 */ "Nagle allows transmit",
	/* 74 */ "Awake SND In SACK Processing",
	/* 75 */ "Awake SND In FWD_TSN Processing",
	/* 76 */ "No wake from sack",
	/* 77 */ "Pre-Send",
	/* 78 */ "End-of-Send",
	/* 79 */ "Sack proc done",
	/* 80 */ "Reason CO Completes",
	/* 81 */ "Blocking-I",
	/* 82 */ "Enter user rcvd",
	/* 83 */ "User rcv does sack",
	/* 84 */ "sorecv blocks on entry",
	/* 85 */ "sorecv blocks for more",
	/* 86 */ "sorecv done",
	/* 87 */ "sack-rwnd update",
	/* 88 */ "enter sorecv",
	/* 89 */ "enter sorecv pl",
	/* 90 */ "++via ip-input",
	/* 91 */ "++via my allocation",
	/* 92 */ "--I am freeing",
	/* 93 */ "++via my copy of mbufs",
	/* 94 */ "sorecv free ctrl",
	/* 95 */ "sorecv does copy",
	/* 96 */ "sorecv does slock",
	/* 97 */ "sorecv does adj",
	/* 98 */ "sorecv bottom while",
	/* 99 */ "sorecv pass bf",
	/* 100 */ "sorecv adj d",
	/* 101 */ "unknown max",
	/* 102 */ "randy stuff 1",
	/* 103 */ "randy stuff 2",
	/* 104 */ "strmout log assign",
	/* 105 */ "strmout log send",
	/* 106 */ "flight log down-ca",
	/* 107 */ "flight log up",
	/* 108 */ "flight log down-gap",
        /* 109 */ "flight log donw-rsnd",
	/* 110 */ "flight log up-rsnd",
	/* 111 */ "flight log down t3",
	/* 112 */ "flight log down wp",
	/* 113 */ "flight up revoke",
	/* 114 */ "flight down pdrp",
	/* 115 */ "flight down pmtu",
	/* 116 */ "log sack normal",
	/* 117 */ "log sack express",
	/* 118 */ "tsn enters",
	/* 119 */ "Tsn Enters in_data",
	/* 120 */ "clear threshold",
	/* 121 */ "increment threshold",
	/* 122 */ "flight log down wp",
	/* 123 */ "FWD TSN check",
	/* 124 */ "Fwd TSN reasm check",
	/* 125 */ "max"

};


#define FROM_STRING_MAX 126

int graph_mode = 0;
int comma_sep = 0;
int first_time = 1;

int stat_only = 0;
uint64_t first_timeevent;
int seen_time=0;
int time_relative = 0;
int relative_to_each = 0;
int add_to_relative=0;
int64_t prev_time = 0;
int at;
FILE *io=NULL;
static uint32_t cnt_event[SCTP_LOG_MAX_EVENT];
static uint32_t cnt_type[SCTP_LOG_MAX_TYPES];
char ts[100];

static void
print_event_cwnd(struct sctp_cwnd_log *log) 
{
	if((log->from == SCTP_CWND_LOG_NOADV_CA) ||
	   (log->from == SCTP_CWND_LOG_NOADV_SS)) {
		printf("%s: Net:%p Cwnd:%d flt:%d flt+acked:%d (atpc:%d npc:%d) %s (pc=%x, sendcnt:%d,strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       (int)log->x.cwnd.cwnd_new_value,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cwnd_augment,
		       log->x.cwnd.meets_pseudo_cumack,
		       log->x.cwnd.need_new_pseudo_cumack,
		       from_str[log->from],
		       (u_int)log->x.cwnd.pseudo_cumack,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str
			);

	} else if (log->from == SCTP_CWND_LOG_FROM_T3){
		printf("%s Network:%p at T3 cwnd:%d flight:%d pc:%x (net==lnet ?:%d,sendcnt:%d,strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (int)log->x.cwnd.pseudo_cumack,
		       (int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);

	} else if (log->from == SCTP_CWND_LOG_FILL_OUTQ_FILLS) {
		printf("%s fill_outqueue adds %d bytes onto net:%p cwnd:%d flight:%d (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       (int)log->x.cwnd.cwnd_augment,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);


	} else if (log->from == SCTP_CWND_LOG_FILL_OUTQ_CALLED) {
		printf("%s fill_outqueue called on net:%p cwnd:%d flight:%d (sendcnt:%d,strcnt:%d) mark:%d\n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str,
		       (int)log->x.cwnd.cwnd_augment
			);

	} else if (log->from == SCTP_SEND_NOW_COMPLETES) {
		printf("%s Send completes sending %d (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       (int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);
	} else if (log->from == SCTP_CWND_LOG_FROM_BRST){
		printf("%s Network:%p at cwnd_event (BURST) cwnd:%d flight:%d decrease by %d (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (u_int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);

	} else if (log->from == SCTP_CWND_LOG_FROM_SACK){
		printf("%s Net:%p at cwnd_event (SACK) cwnd:%d flight:%d pq:%x atpc:%d needpc:%d (tsn:%x,sendcnt:%d,strcnt:%d) \n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (u_int)log->x.cwnd.pseudo_cumack,
		       log->x.cwnd.meets_pseudo_cumack,
		       log->x.cwnd.need_new_pseudo_cumack,
		       (u_int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);

	} else if (log->from == SCTP_CWND_INITIALIZATION){
		printf("%s Network:%p initializes cwnd:%d flight:%d pq:%x (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (u_int)log->x.cwnd.pseudo_cumack,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);
	} else if ((log->from == SCTP_CWND_LOG_FROM_SEND) ||
		   (log->from == SCTP_CWND_LOG_FROM_RESEND)){
		printf("%s Network:%p cwnd:%d flight:%d pq:%x %s tsn:%x (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (u_int)log->x.cwnd.pseudo_cumack,
		       from_str[log->from],
		       (u_int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);
	}else {
		printf("%s:CWND %s net:%p cwnd:%d flight:%d aug:%d (sendcnt:%d,strcnt:%d)\n",
		       ts,
		       from_str[log->from],
		       log->x.cwnd.net,
		       log->x.cwnd.cwnd_new_value,
		       log->x.cwnd.inflight,
		       (u_int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str);
	}
}

static void
print_event_nagle(struct sctp_cwnd_log *log) 
{
	int un_sent;
	un_sent = log->x.nagle.total_in_queue - log->x.nagle.total_flight;
	un_sent += ((log->x.nagle.count_in_queue - log->x.nagle.count_in_flight) * 16);

	printf("%d: stcb:%p total_flight:%u total_in_queue:%u c_in_que:%d c_in_flt:%d  un_sent:%d %s\n",
	       at,
	       log->x.nagle.stcb,
	       log->x.nagle.total_flight,
	       log->x.nagle.total_in_queue,
	       log->x.nagle.count_in_queue,
	       log->x.nagle.count_in_flight,
	       (un_sent),
	       from_str[log->from]);
}

static void
print_event_sb(struct sctp_cwnd_log *log) 
{
	printf("%d: %s stcb:%p sb_cc:%x stcb_sbcc:%x %s:%d\n",
	       at,
	       from_str[log->from],
	       log->x.sb.stcb,
	       log->x.sb.so_sbcc,
	       log->x.sb.stcb_sbcc,
	       ((log->from == SCTP_LOG_SBALLOC) ? "increment" : "decrement"),
	       log->x.sb.incr);
}


static void
print_event_mbuf(struct sctp_cwnd_log *log) 
{
	printf("%s mbuf:%p data:%p len:%d flags:%x refcnt:%d extbuf:%p %s\n",
	       ts,
	       log->x.mb.mp,
	       log->x.mb.data,
	       (int)log->x.mb.size,
	       (uint32_t)log->x.mb.mbuf_flags,
	       (int)log->x.mb.refcnt,
	       log->x.mb.ext,
	       from_str[log->from]);
}

static void
print_event_close(struct sctp_cwnd_log *log) 
{
	char *close_events[] = {
		"Top of sctp_inpcb_free",
		"Stop its allgone",
		"imm=0, some still closing",
		"Still closing stage 3",
		"Delay start inp-kill timer, ref",
		"Clear to purge INP",
		"Top of free_asoc",
		"Its gone",
		"Delay and start timer, its freeing",
		"Set ATBF, start timer, wait",
		"Clear to purge asoc",
		"Association now purged",
		"INPCB allocate",
		"sctp_close() must abort",
		"sctp_close() can shutdown",
		"close from attach",
		"close from abort",
		"sctp_close() called",
		"Unknown"
	};
	if(log->x.close.loc > 18) {
		log->x.close.loc = 18;
	}
	printf("%s: inp:%p sctp_flags:%x stcb:%p asoc state:%x e:%d-%s\n",
	       ts,
	       log->x.close.inp,
	       (u_int)log->x.close.sctp_flags,
	       log->x.close.stcb,
	       (u_int)log->x.close.state,
	       log->x.close.loc,
	       close_events[log->x.close.loc]);
}

static void
print_event_lock(struct sctp_cwnd_log *log) 
{
	printf("%s sock:%p inp:%p inp:%d tcb:%d info:%d sock:%d sockrb:%d socksb:%d cre:%d\n",
	       from_str[log->from],
	       log->x.lock.sock,
	       log->x.lock.inp,
	       log->x.lock.inp_lock,
	       log->x.lock.tcb_lock,
	       log->x.lock.info_lock,
	       log->x.lock.sock_lock,
	       log->x.lock.sockrcvbuf_lock,
	       log->x.lock.socksndbuf_lock,
	       log->x.lock.create_lock
		);
}

static void
print_event_sack(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_LOG_NEW_SACK) {
		printf("%s - cum-ack:%x old-cumack:%x dups:%d gaps:%d\n", 
		       from_str[log->from],
		       log->x.sack.cumack,
		       log->x.sack.oldcumack,
		       (int)log->x.sack.numDups,
		       (int)log->x.sack.numGaps
			);
	} else if(log->from == SCTP_LOG_FREE_SENT) {
		printf("%s - cum-ack:%x freeing TSN:%x\n",
		       from_str[log->from],
		       log->x.sack.cumack,
		       log->x.sack.tsn
			);
	} else {
		printf("%s - cum-ack:%x biggestTSNAcked:%x TSN:%x dups:%d gaps:%d\n", 
		       from_str[log->from],
		       log->x.sack.cumack,
		       log->x.sack.oldcumack,
		       log->x.sack.tsn,
		       (int)log->x.sack.numDups,
		       (int)log->x.sack.numGaps
			);

	}
}

static void
print_event_mbcnt(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_LOG_MBCNT_INCREASE) {
		printf("%s - tqs(%d + %d) = %d mb_tqs(%d + %d) = %d\n",
		       from_str[log->from],
		       log->x.mbcnt.total_queue_size,
		       log->x.mbcnt.size_change,
		       (log->x.mbcnt.total_queue_size + log->x.mbcnt.size_change),
		       log->x.mbcnt.total_queue_mb_size,
		       log->x.mbcnt.mbcnt_change,
		       (log->x.mbcnt.total_queue_mb_size + log->x.mbcnt.mbcnt_change));
	} else if (log->from == SCTP_LOG_MBCNT_DECREASE) {
		printf("%s - tqs(%d - %d) = %d mb_tqs(%d - %d) = %d\n",
		       from_str[log->from],
		       log->x.mbcnt.total_queue_size,
		       log->x.mbcnt.size_change,
		       (log->x.mbcnt.total_queue_size - log->x.mbcnt.size_change),
		       log->x.mbcnt.total_queue_mb_size,
		       log->x.mbcnt.mbcnt_change,
		       (log->x.mbcnt.total_queue_mb_size - log->x.mbcnt.mbcnt_change));

	} else {
		printf("Unkowns MBCNT event %d\n", log->from);
	}
}

static void
print_event_rwnd(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_INCREASE_PEER_RWND) {
		printf("%s - rwnd:%d increase:(%d + overhead:%d) -> %d\n",
		       from_str[log->from],
		       log->x.rwnd.rwnd,
		       log->x.rwnd.send_size,
		       log->x.rwnd.overhead,
		       (log->x.rwnd.rwnd + log->x.rwnd.send_size + log->x.rwnd.overhead));
	} else if (log->from == SCTP_DECREASE_PEER_RWND) {
		printf("%s - rwnd:%d decrease:(%d + overhead:%d) -> %d\n",
		       from_str[log->from],
		       log->x.rwnd.rwnd,
		       log->x.rwnd.send_size,
		       log->x.rwnd.overhead,
		       (log->x.rwnd.rwnd - (log->x.rwnd.send_size + log->x.rwnd.overhead)));

	} else if (log->from == SCTP_SET_PEER_RWND_VIA_SACK) {
		printf("%s - rwnd:%d decrease:(%d + overhead:%d) arwnd:%d ->%d\n",
		       from_str[log->from],
		       log->x.rwnd.rwnd,
		       log->x.rwnd.send_size,
		       log->x.rwnd.overhead,
		       log->x.rwnd.new_rwnd,
		       (log->x.rwnd.new_rwnd - (log->x.rwnd.send_size + log->x.rwnd.overhead))
			);
	} else {
		printf("Unknown RWND event\n");
	}
}

static void
print_event_maxburst(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_MAX_BURST_ERROR_STOP) {
		printf("%s: Network:%p Flight:%d burst cnt:%d - send error:%d %s (sendcnt:%d strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cwnd_augment,
		       (int)log->x.cwnd.cwnd_new_value,
		       from_str[log->from],
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str
			);
	} else if (log->from == SCTP_MAX_BURST_APPLIED) {
		printf("%s: Network:%p Flight:%d burst cnt:%d %s (sendcnt:%d strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cwnd_augment,
		       from_str[log->from],
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str
			);
	} else if (log->from == SCTP_MAX_IFP_APPLIED) {
		printf("%s: Network:%p Flight:%d ifp_send_qsize: %d ifp_send_qlimit:%d %s (sendcnt:%d strcnt:%d)\n",
		       ts,
		       log->x.cwnd.net,
		       (int)log->x.cwnd.inflight,
		       (int)log->x.cwnd.cwnd_new_value,
		       (int)log->x.cwnd.cwnd_augment,
		       from_str[log->from],
		       (int)log->x.cwnd.cnt_in_send,
		       (int)log->x.cwnd.cnt_in_str
			);
	}
}

static void
print_event_misc(struct sctp_cwnd_log *log) 
{
#ifdef __APPLE_OLD__
	/* APPLE lock logging */
	char *filename;
	char *operation;
	/*
	 * log1 = file number
	 * log2 = line number
	 * log3 = lock/mutex addr,
	 * log4 = lock/mutex operation
	 */
#define BEFORE_LOCK_SHARED        0x01
#define BEFORE_LOCK_EXCLUSIVE     0x02
#define BEFORE_TRY_LOCK_EXCLUSIVE 0x05
#define BEFORE_LOCK_SOCKET        0x06

#define AFTER_LOCK_SHARED         0x11
#define AFTER_LOCK_EXCLUSIVE      0x12
#define AFTER_TRY_LOCK_EXCLUSIVE  0x13
#define AFTER_LOCK_SOCKET         0x14

#define UNLOCK_SHARED             0x21
#define UNLOCK_EXCLUSIVE          0x22
#define UNLOCK_SOCKET             0x23

	switch (log->x.misc.log1) {
	case 1:
		filename = "sctp_asconf.c";
		break;
	case 2:
		filename = "sctp_input.c";
		break;
	case 3:
		filename = "sctp_output.c";
		break;
	case 4:
		filename = "sctp_pcb.c";
		break;
	case 5:
		filename = "sctp_peeloff.c";
		break;
	case 6:
		filename = "sctp_timer.c";
		break;
	case 7:
		filename = "sctp_usrreq.c";
		break;
	case 8:
		filename = "sctp_util.c";
		break;
	case 9:
		filename = "sctp_usrreq6.c";
		break;
	default:
		filename = "unknown file";
		break;
	}

	switch (log->x.misc.log4) {
	case BEFORE_LOCK_SHARED:
		operation = "before lock shared";
		break;
	case BEFORE_LOCK_EXCLUSIVE:
		operation = "before lock exclusive";
		break;
	case BEFORE_TRY_LOCK_EXCLUSIVE:
		operation = "before try lock exclusive";
		break;
	case BEFORE_LOCK_SOCKET:
		operation = "before lock socket";
		break;

	case AFTER_LOCK_SHARED:
		operation = "after lock shared";
		break;
	case AFTER_LOCK_EXCLUSIVE:
		operation = "after lock exclusive";
		break;
	case AFTER_TRY_LOCK_EXCLUSIVE:
		operation = "after try lock exclusive";
		break;
	case AFTER_LOCK_SOCKET:
		operation = "after lock socket";
		break;

	case UNLOCK_SHARED:
		operation = "unlock shared";
		break;
	case UNLOCK_EXCLUSIVE:
		operation = "unlock exclusive";
		break;
	case UNLOCK_SOCKET:
		operation = "unlock socket";
		break;

	default:
		operation = "unknown operation";
		break;
	}
	printf("%s Line %u: %s, addr 0x%x\n",
	       filename,
	       log->x.misc.log2,
	       operation,
	       log->x.misc.log3);
#elif defined(__APPLE__)
	printf("%s: Type %3u, Line %4u %u %u\n",
	       ts,
	       log->x.misc.log1,
	       log->x.misc.log2,
	       log->x.misc.log3,
	       log->x.misc.log4);
#else
	if(log->from == SCTP_REASON_FOR_SC) {
		printf("%s:%s num_out:%u reason_code:%u cwnd_full:%u sendonly1:%u\n",
		       ts,
		       from_str[log->from],
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log3,
		       log->x.misc.log4);
				       
	} else if (log->from == SCTP_FWD_TSN_CHECK) {
		if (log->x.misc.log1 == 0xee) {
			printf("%s:%s contemplate cum:%u apa:%u old-apa:%u\n", 
			       ts,
			       from_str[log->from],
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);

		} else if (log->x.misc.log1 == 0xff) {
			printf("%s:%s output %u %u %u\n", 
			       ts,
			       from_str[log->from],
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		} else {
			printf("%s:%s advpeerack:%u tp1tsn:%u %u %u\n", 
			       ts,
			       from_str[log->from],
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_THRESHOLD_CLEAR) {
		printf("%s:Clear asoc threshold old val:%d new val:%d FILE:%x LINE:%d\n",
		       ts,
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log3,
		       log->x.misc.log4);
	} else if (log->from == SCTP_THRESHOLD_INCR) {
		printf("%s:Increment asoc threshold old val:%d new val:%d FILE:%x LINE:%d\n",
		       ts,
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log3,
		       log->x.misc.log4);
	} else if (log->from == SCTP_ENTER_USER_RECV) {
		if(!graph_mode) {
			printf("%s user_rcv: dif:%d freed:%d sincelast:%d rwnd_req:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_USER_RECV_SACKS) {
		if(!graph_mode) {
			if(log->x.misc.log4 == 0) {
				printf("%s no sack rwnd:%d reported:%d sincelast:%d\n",
				       ts,
				       log->x.misc.log1,
				       log->x.misc.log2,
				       log->x.misc.log3);
			} else {
				printf("%s send sack rwnd:%d reported:%d sincelast:%d dif:%d\n",
				       ts,
				       log->x.misc.log1,
				       log->x.misc.log2,
				       log->x.misc.log3,
				       log->x.misc.log4);
			}
		}
	} else if (log->from == SCTP_SORECV_BLOCKSA) {
		if(!graph_mode) {
			printf("%s Enter and block sb_cc:%u reading:%d\n",
			       ts,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_STRMOUT_LOG_ASSIGN) {
	  printf("%s Stream seq Assigned stcb:%x len:%x strm:%d seq:%d\n",
			   ts, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       (log->x.misc.log3 >> 16),
		       (log->x.misc.log3 & 0x0000ffff)
			);
	} else if (log->from == SCTP_STRMOUT_LOG_SEND) {
	  printf("%s Stream seq Send stcb:%x len:%x strm:%d seq:%d tsn:%x\n",
			   ts, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       (log->x.misc.log3 >> 16),
		       (log->x.misc.log3 & 0x0000ffff),
		       log->x.misc.log4);

	} else if (log->from == SCTP_FLIGHT_LOG_UP_REVOKE) {
		printf("%s Flight Up revoked (net:%x) net-flight:%d incr:%d TSN:%x newflt:%d\n", 
		       ts, log->x.misc.log3, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log4,
		       (log->x.misc.log1 + log->x.misc.log2)
			);
	} else if (log->from == SCTP_FLIGHT_LOG_UP) {
		printf("%s Flight Up (net:%x) net-flight:%d incr:%d TSN:%x newflt:%d\n", 
		       ts, log->x.misc.log3, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log4,
		       (log->x.misc.log1 + log->x.misc.log2)
			);
	} else if (log->from == SCTP_FLIGHT_LOG_UP_RSND) {
		printf("%s Flight Up-rsnd (net:%x) net-flight:%d incr:%d TSN:%x newflt:%d\n", 
		       ts, log->x.misc.log3, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log4,
		       (log->x.misc.log1 + log->x.misc.log2)
			);
	} else if ((log->from == SCTP_SACK_LOG_NORMAL) ||
		   (log->from == SCTP_SACK_LOG_EXPRESS)) {
		char c;
		if (log->from == SCTP_SACK_LOG_EXPRESS)
			c = 'E';
		else
			c = 'N';
		printf("%s new-cum:%u new-rwnd:%d last-cum:%u last-arwnd:%d(%c)\n",
		       ts,
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log3,
		       log->x.misc.log4,
		       c
			);
	} else if ((log->from == SCTP_FLIGHT_LOG_DOWN_CA) ||
		   (log->from == SCTP_FLIGHT_LOG_DOWN_GAP) ||
		   (log->from == SCTP_FLIGHT_LOG_DOWN_RSND_TO) ||
		   (log->from == SCTP_FLIGHT_LOG_DOWN_WP) ||
		   (log->from == SCTP_FLIGHT_LOG_DOWN_PDRP) ||
		   (log->from == SCTP_FLIGHT_LOG_DOWN_RSND)) {
		char *xx;
		if (log->from == SCTP_FLIGHT_LOG_DOWN_CA) {
			xx = "decrease-CUMACK";
		} else if (log->from == SCTP_FLIGHT_LOG_DOWN_GAP) {
			xx = "decrease-GAPACK";
		} else if (log->from == SCTP_FLIGHT_LOG_DOWN_RSND_TO) {
			xx = "decrease-TO";
		} else if (log->from == SCTP_FLIGHT_LOG_DOWN_RSND) {
			xx = "decrease-FR";
		} else if (log->from == SCTP_FLIGHT_LOG_DOWN_WP) {
			xx = "decrease-PDRP";
		} else if (log->from == SCTP_FLIGHT_LOG_DOWN_WP) {
			xx = "WP-recovery";
		} else {
			xx = "huh";
		}

		printf("%s %s (net:%x) net-flight:%d decr:%d TSN:%x newflt:%d\n", 
		       ts,
		       xx,
		       log->x.misc.log3, 
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log4,
		       (log->x.misc.log1 - log->x.misc.log2)
			);

	} else if (log->from == SCTP_SORECV_BLOCKSB) {
		if(!graph_mode) {
			printf("%s Blocking freed:%d rwnd:%d sb_cc:%u reading:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_SORECV_DONE) {
		if(graph_mode) {
			printf("%s %d:EXIT\n", ts, log->x.misc.log4);
		} else {
			printf("%s freed_so_far_at_exit:%d last_rep rwnd::%d rwnd:%d sb_cc:%u\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_SORECV_ENTER) {
		if(graph_mode) {
			printf("%s %d:ENTER\n", ts, log->x.misc.log3);
		} else {
			printf("%s enter srcv rwndreq:%d ieeor:%d sb_cc:%u inp:%x \n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_SORECV_ENTERPL) {
		if(graph_mode) {
			printf("%s %d:READ\n", ts, log->x.misc.log3);
		} else {
			printf("%s pass_lock rwndreq:%d canblk:%d sb_cc:%u uioreq:%d \n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else if (log->from == SCTP_SORCV_FREECTL) {
		if(!graph_mode) {
			printf("%s free control sb_cc:%u\n",
			       ts,
			       log->x.misc.log1);
		}
	} else if (log->from == SCTP_SORCV_DOESCPY) {				
		if(!graph_mode) {
			printf("%s copied data of %d  sb_cc:%u\n",
			       ts,
			       log->x.misc.log2,
			       log->x.misc.log1);
		}
	} else if (log->from == SCTP_SORCV_DOESLCK) {
		if(!graph_mode) {
			printf("%s Does the lock cp:%d mlen:%d  sb_cc:%u\n",
			       ts,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log1);
		}
	} else if (log->from == SCTP_SORCV_DOESADJ) {
		if(!graph_mode) {
			printf("%s Does adjust sb_cc:%u ctl->len:%d cp_len:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3
				);
		}
	} else if (log->from == SCTP_SORCV_PASSBF) {
		if(!graph_mode) {
			printf("%s Past sb_subtract  sb_cc:%u ctl->len:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2);
		} else {
			printf("%s %d READ\n",
			       ts,
			       log->x.misc.log1);
		}

	} else if (log->from == SCTP_SORCV_ADJD) {
		if(!graph_mode) {
			printf("%s Adjusts done  sb_cc:%u ctl->len:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2
				);
		}

	} else if (log->from == SCTP_SORCV_BOTWHILE) {
		if(!graph_mode) {
			printf("%s Bottom while sb_cc:%u ctl->len:%d\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2);
		}
	} else if (log->from == SCTP_SACK_RWND_UPDATE) {
		if(comma_sep) {
			if(first_time) {
				fprintf(io, "TS,RWND,A-RWND,FLIGHT\n");
				first_time = 0;
			}
			fprintf(io,"%s,%d,%d,%d\n",
				ts, 
				log->x.misc.log2, 
				log->x.misc.log1,
				log->x.misc.log3);
		} else if(graph_mode) {
			printf("%s %d:RWND\n", ts, log->x.misc.log2);
			printf("%s %d:ARWND\n",ts, log->x.misc.log1);
			printf("%s %d:FLIGHT\n", ts, log->x.misc.log3);
			printf("%s %d:OUT_QUEUE\n", ts, log->x.misc.log4);
		} else {
			printf("%s s-rwnd:%d calc_rwnd:%d  flight:%d total_oqs:%x\n",
			       ts,
			       log->x.misc.log1,
			       log->x.misc.log2,
			       log->x.misc.log3,
			       log->x.misc.log4);
		}
	} else {
		printf("%s:%s log1:%u log2:%u log3:%u log4:%u\n",
		       ts,
		       from_str[log->from],
		       log->x.misc.log1,
		       log->x.misc.log2,
		       log->x.misc.log3,
		       log->x.misc.log4);
	}
#endif
}

static void
print_event_wakeup(struct sctp_cwnd_log *log) 
{

	char *str;
	switch(log->x.wake.sctpflags) {
	case 0:
		str = "Will send wakeup";
		break;
	case 1:
		str = "Defer wakeup";
		break;
	case 2:
		str = "Needs output wakeup";
		break;
	case 3:
		str = "Deferred and needs output wakeup";
		break;
	case 4:
		str = "Needs input wakeup";
		break;
	case 5:
		str = "Deferred and needs input wakeup";
		break;
	case 6:
		str = "Needs input/output wakeup";
		break;
	case 7:
		str = "Deferred and needs input/output wakeup";
		break;
	default:
		str = "Unknown";
	}
	printf("%s WUP:%s inp:%p cnt:%d fs:%d sd:%d st:%d str:%d co:%d) s:%s sb:%x\n",
	       ts,
	       from_str[log->from],
	       log->x.wake.stcb,
	       log->x.wake.wake_cnt,
	       log->x.wake.flight,
	       log->x.wake.send_q,
	       log->x.wake.sent_q,
	       log->x.wake.stream_qcnt,
	       log->x.wake.chunks_on_oque,
	       str,
	       (u_int)log->x.wake.sbflags
		);
}

static void
print_event_block(struct sctp_cwnd_log *log) 
{
	printf("%s:BLK: onq:%d send:%d flight:%d chk:%d sendcnt:%d strmcnt:%d rwnd:%d %s\n",
	       ts,
	       (int)log->x.blk.onsb,
	       (int)log->x.blk.sndlen,
	       (int)(log->x.blk.flight_size * 1024),
	       log->x.blk.chunks_on_oque,
	       log->x.blk.send_sent_qcnt,
	       log->x.blk.stream_qcnt,
	       log->x.blk.peer_rwnd,
	       from_str[log->from]
		);
}


static void
print_event_strm(struct sctp_cwnd_log *log) 
{
	if((log->from == SCTP_STR_LOG_FROM_INSERT_MD) ||
	   (log->from == SCTP_STR_LOG_FROM_INSERT_TL)) {
		/* have both the new entry and 
		 * the previous entry to print.
		 */
		printf("%s: stcb:%p tsn=%x strm:%u sseq=%u %s tsn=%x sseq=%u\n",
		       from_str[log->from],
		       log->x.strlog.stcb,
		       (u_int)log->x.strlog.n_tsn,
		       log->x.strlog.strm,
		       log->x.strlog.n_sseq,
		       ((log->from == SCTP_STR_LOG_FROM_INSERT_MD) ? "Before" : "After"),
		       (u_int)log->x.strlog.e_tsn,
		       log->x.strlog.e_sseq
			);
	}else {
		printf("%s:stcb:%p  tsn=%x strm %u sseq=%u\n",
		       from_str[log->from],
		       log->x.strlog.stcb,
		       (u_int)log->x.strlog.n_tsn,
		       log->x.strlog.strm,
		       (u_int)log->x.strlog.n_sseq);
	}


}


static void
print_event_fr(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_FR_LOG_BIGGEST_TSNS) {
		printf("%s: biggest_tsn_in_sack=%x biggest_new_tsn:%x cumack:%x\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       (u_int)log->x.fr.largest_new_tsn,
		       (u_int)log->x.fr.tsn);
	} else if (log->from == SCTP_FR_T3_TIMEOUT) {
		printf("%s: first:%x last:%x cnt:%d\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       (u_int)log->x.fr.largest_new_tsn,
		       (int)log->x.fr.tsn);

	} else if (log->from == SCTP_FR_DUPED) {
		printf("%s: TSN:%x \n",
		       from_str[log->from],
		       (uint)ntohl(log->x.fr.largest_tsn));
	} else if ((log->from == SCTP_FR_CWND_REPORT) ||
		   (log->from == SCTP_FR_CWND_REPORT_START) ||
		   (log->from == SCTP_FR_CWND_REPORT_STOP)) {
		printf("%s: net flight:%d net cwnd:%d tot flight:%d\n",
		       from_str[log->from],
		       log->x.fr.largest_tsn,
		       (int)log->x.fr.largest_new_tsn,
		       (int)log->x.fr.tsn);
	} else if ((log->from == SCTP_FR_T3_MARKED) ||
		   (log->from == SCTP_FR_MARKED_EARLY) ||
		   (log->from == SCTP_FR_MARKED)) {
		printf("%s: TSN:%x send-count:%d\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       (int)log->x.fr.largest_new_tsn);
	} else if ((log->from == SCTP_FR_T3_STOPPED)  ||
		   (log->from == SCTP_FR_T3_MARK_TIME)) {
		printf("%s: TSN:%x Seconds:%u Usecond:%d\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       log->x.fr.largest_new_tsn,
		       (int)log->x.fr.tsn);
	} else if ((log->from == SCTP_FR_LOG_STRIKE_CHUNK) ||
		   (log->from == SCTP_FR_LOG_CHECK_STRIKE)) {
		printf("%s: biggest_newly_acked:%x TSN:%x sent_flag:%x\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       (u_int)log->x.fr.largest_new_tsn,
		       (u_int)log->x.fr.tsn);
	} else {
		printf("%s: biggest_new_tsn:%x tsn_to_fr:%x fr_tsn_thresh:%x\n",
		       from_str[log->from],
		       (u_int)log->x.fr.largest_tsn,
		       (u_int)log->x.fr.largest_new_tsn,
		       (u_int)log->x.fr.tsn);
	}
}

static void
print_event_map(struct sctp_cwnd_log *log) 
{
	if(log->from == SCTP_MAP_SLIDE_FROM) {
		printf("%s: Slide From:%u Slide to:%u lgap:%u - SLIDE FROM\n",
		       from_str[log->from],
		       (int)log->x.map.base,
		       (int)log->x.map.cum,
		       (int)log->x.map.high);
	} else if(log->from == SCTP_MAP_SLIDE_NONE) {
		printf("%s: (distance:%u + slide_from:%u) > array_size:%u (TSNH :-0) - NONE\n",
		       from_str[log->from],
		       (int)log->x.map.base,
		       (int)log->x.map.cum,
		       (int)log->x.map.high);
	} else if(log->from == SCTP_MAP_TSN_ENTERS) {
		printf("%s: tsn:%u  cumack:%u highest:%u - TSN_IN\n",
		       from_str[log->from],
		       (int)log->x.map.base,
		       (int)log->x.map.cum,
		       (int)log->x.map.high);

	} else {
		printf("%s: Map Base:%u Cum Ack:%u Highest TSN:%u\n",
		       from_str[log->from],
		       log->x.map.base,
		       log->x.map.cum,
		       log->x.map.high);

	}
}

static void
print_event_chunk(struct sctp_cwnd_log *log) 
{
	printf("%s Chunk processed-inp:%x stcb:%x chunktype:%d length:%d\n",
	       ts,
	       log->x.misc.log1,
	       log->x.misc.log2,
	       (int)log->x.misc.log3,
	       (int)log->x.misc.log4);
}

static void
print_event_error_out(struct sctp_cwnd_log *log) 
{
	printf("%s Error return-inp:%x stcb:%x error:%d\n",
	       ts,
	       log->x.misc.log1,
	       log->x.misc.log2,
	       (int)log->x.misc.log3);
}

static void
print_event_rtt(struct sctp_cwnd_log *log) 
{
	printf("%s rtt:%d\n",
	       ts,
	       log->x.misc.log2);
}

void
do_print_of_log(struct sctp_cwnd_log *log) 
{
	if(stat_only) {
		cnt_type[log->from]++;
		if(log->event_type < SCTP_LOG_MAX_EVENT)
			cnt_event[log->event_type]++;
		else
			printf("Event type %d greater than max\n", log->event_type);
		return;
	}
	if(time_relative == 0){ 
		sprintf(ts, "%llu", (unsigned long long)log->time_event);
	} else {
		if (seen_time == 0) {
			first_timeevent = log->time_event;
			seen_time = 1;
		} else if (relative_to_each){
			first_timeevent -= prev_time;
		}
		/* Now print and continue. */
		sprintf(ts, "%llu", (unsigned long long)log->time_event);
		prev_time = log->time_event;
	}
	switch( log->event_type) {
	case SCTP_LOG_EVENT_CWND:
		print_event_cwnd(log);
		break;
	case SCTP_LOG_EVENT_NAGLE:
		print_event_nagle(log);
		break;
	case SCTP_LOG_EVENT_SB:
		print_event_sb(log);
		break;
	case SCTP_LOG_EVENT_MBUF:
		print_event_mbuf(log);
		break;
	case SCTP_LOG_EVENT_CLOSE:
		print_event_close(log);
		break;
	case SCTP_LOG_LOCK_EVENT:
		print_event_lock(log);
		break;
	case SCTP_LOG_EVENT_SACK:
		print_event_sack(log);
		break;
	case SCTP_LOG_EVENT_MBCNT:
		print_event_mbcnt(log);
		break;
	case SCTP_LOG_EVENT_RWND:
		print_event_rwnd(log);
		break;
	case SCTP_LOG_EVENT_MAXBURST:
		print_event_maxburst(log);
		break;
	case SCTP_LOG_MISC_EVENT:
		print_event_misc(log);
		break;
	case SCTP_LOG_EVENT_WAKE:
		print_event_wakeup(log);
		break;
	case SCTP_LOG_EVENT_BLOCK:
		print_event_block(log);
		break;
	case SCTP_LOG_EVENT_STRM:
		print_event_strm(log);
		break;
	case SCTP_LOG_EVENT_FR:
		print_event_fr(log);
		break;
	case SCTP_LOG_EVENT_MAP:
		print_event_map(log);
		break;
	case SCTP_LOG_CHUNK_PROC:
		print_event_chunk(log);
		break;
	case SCTP_LOG_ERROR_RET:
		print_event_error_out(log);
		break;
	case SCTP_LOG_EVENT_RTT:
		print_event_rtt(log);
		break;
	default:
		break;
	};
	at++;
}

int
main(int argc, char **argv)
{
	int i;
	char buffer[512];
	FILE *infile;
	char *logfile=NULL, *ep=NULL, *nep=NULL;
	char *logtype, *from, *log1, *log2, *log3, *log4, *sctp;
	int quiet=0;
	int seq, cnt=0;
	struct sctp_cwnd_log log;
	while((i= getopt(argc,argv,"l:sgc:rRa:q")) != EOF)
	{
		switch(i) {
		case 'q':
			quiet = 1;
			break;
		case 'r':
			time_relative = 1;
			break;
		case 'R':
			time_relative = 1;
			relative_to_each = 1;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 's':
			stat_only = 1;
			break;
		case 'a':
			add_to_relative = strtol(optarg, NULL, 0);
			break;
		case 'g':
			graph_mode = 1;
			break;
		case 'c': 
		{
			char fname[100];
			sprintf(fname, "%s.csv", optarg);
			io = fopen(fname, "w+");
			if(io == NULL) {
				fprintf(stderr,"Can't open %s for writing, sorry\n",
					fname);
				exit(-1);
			}
			comma_sep = 1;
		}
		break;
		default:
			break;
		};
	}
	if (logfile == NULL) {
		printf("Use %s -l ktrdump [-s] or -l - (for stdin)\n", 
		       argv[0]);
		printf("Hint use ktrdump -q -t -o file\n");
		return(1);
	} else if (strcmp(logfile, "-") == 0) {
		infile = stdin;
	} else {
		infile = fopen(logfile,"r");
	}
	if(infile == NULL) {
		printf("Can't open file %d\n",errno);	
		return(3);
	}
	at = 0;
	if(stat_only) {
		memset(cnt_type,0, sizeof(cnt_type));
		memset(cnt_event,0, sizeof(cnt_type));
	}
	while (fgets(buffer, sizeof(buffer), infile) != NULL) {
		cnt++;
		/* parse out the log file */
                /*      0       3359070622 15[117]:1fead531-38d7f-1fead530-38d24 */
		seq = strtol(buffer, &ep, 0);
		if(ep == NULL) {
			/* ignore line */
			if(quiet == 0) printf("Skipping line %d no seq number\n", cnt);
			continue;
		}
		log.time_event =  strtoull(ep, &nep, 0);
		if (nep == NULL) {
			/* No 64 bit value there */
			if(quiet == 0) printf("Skipping line %d no 64 bit ts\n", cnt);
			continue;
		}
		sctp = strtok(nep, "SCTP:");
		if(sctp == NULL) {
			if(quiet == 0) printf("Skipping line %d no SCTP:\n", cnt);
			continue;
		}
		sctp = strtok(NULL, ":");
		if(sctp == NULL) {
			if(quiet == 0) printf("Skipping line %d no SCTP:\n", cnt);
			continue;
		}
	
		logtype = strtok(NULL, "[");
		if(logtype == NULL) {
			if(quiet == 0) printf("Skipping line %d no logtype\n", cnt);
			continue;
		}

		from = strtok(NULL, "]");
		if(from == NULL) {
			if(quiet == 0) printf("Skipping line %d no from\n", cnt);
			continue;
		}
		log1 = strtok(NULL, "-");
		if(log1 == NULL) {
		out:
			if(quiet == 0) printf("Skipping line %d no log1 skip\n", cnt);
			continue;
		}
		if(log1[0] != ':') {
			goto out;
		}
		log1++;
		log2 = strtok(NULL, "-");
		if(log2 == NULL) {
			if(quiet == 0) printf("Skipping line %d no log2\n", cnt);
			continue;
		}
		log3 = strtok(NULL, "-");
		if(log3 == NULL) {
			if(quiet == 0) printf("Skipping line %d no log3\n", cnt);
			continue;
		}
		log4 = strtok(NULL, "\n");
		if(log4 == NULL) {
			if(quiet == 0) printf("Skipping line %d no log4\n", cnt);
			continue;
		}
		log.from = (u_int8_t)strtoul(from, NULL, 0);
		log.event_type = (u_int8_t)strtoul(logtype, NULL, 0);
		log.x.misc.log1 = strtoul(log1, NULL, 16);
		log.x.misc.log2 = strtoul(log2, NULL, 16);
		log.x.misc.log3 = strtoul(log3, NULL, 16);
		log.x.misc.log4 = strtoul(log4, NULL, 16);
		do_print_of_log(&log);
	}
	if(stat_only) {
		for(i=0;i< SCTP_LOG_MAX_EVENT;i++) {
			printf("%s = %u\n", event_names[i], cnt_event[i]);
		}
		for(i=0;i<SCTP_LOG_MAX_TYPES;i++) {
			printf("%s = %u\n", from_str[i], cnt_type[i]);
		}
	}
	if(io) {
		fclose(io);
		io = NULL;
	}
	return(0);
}
