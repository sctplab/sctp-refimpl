#include <stdio.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
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
	"SCTP_LOG_EVENT_UNKNOWN",
	"SCTP_LOG_EVENT_CWND",
	"SCTP_LOG_EVENT_BLOCK",
	"SCTP_LOG_EVENT_STRM",
	"SCTP_LOG_EVENT_FR ",
	"SCTP_LOG_EVENT_MAP",
	"SCTP_LOG_EVENT_MAXBURST ",
	"SCTP_LOG_EVENT_RWND",
	"SCTP_LOG_EVENT_MBCNT",
	"SCTP_LOG_EVENT_SACK",
	"SCTP_LOG_LOCK_EVENT",
	"SCTP_LOG_EVENT_RTT",
	"SCTP_LOG_EVENT_SB",
	"SCTP_LOG_EVENT_NAGLE",
	"SCTP_LOG_EVENT_WAKE",
	"SCTP_LOG_MAX_EVENT",
	"SCTP_LOG_EVENT_CLOSE"
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
	/* 94 */ "unknown"
};

#define FROM_STRING_MAX 94

int graph_mode = 0;
int comma_sep = 0;
int first_time = 1;

static uint32_t cnt_event[SCTP_LOG_MAX_EVENT];
static uint32_t cnt_type[SCTP_LOG_MAX_TYPES];
int
main(int argc, char **argv)
{
	int stat_only=0,i;
	char ts[100];
	FILE *out, *io=NULL;
	char *logfile=NULL;
	uint32_t first_timeevent_sec;
	uint32_t first_timeevent_usec;
	int seen_time=0;
	int time_relative = 0;
	int relative_to_each = 0;
	int add_to_relative=0;
	int32_t prev_time = 0;
	int at;
	struct sctp_cwnd_log log;
	while((i= getopt(argc,argv,"l:sgc:rRa:")) != EOF)
	{
		switch(i) {
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
		printf("Use %s -l binary_logfile [-s]\n", argv[0]);
		return(1);
	}
	out = fopen(logfile,"r");
	if(out == NULL) {
		printf("Can't open file %d\n",errno);	
		return(3);
	}
	at = 0;
	if(log.from >=  FROM_STRING_MAX)
		log.from = FROM_STRING_MAX;

	if(stat_only) {
		memset(cnt_type,0, sizeof(cnt_type));
		memset(cnt_event,0, sizeof(cnt_type));
	}

	while(fread((void *)&log, sizeof(log), 1,out) > 0) {
		if(stat_only) {
			cnt_type[log.from]++;
			if(log.event_type < SCTP_LOG_MAX_EVENT)
				cnt_event[log.event_type]++;
			else
				printf("Event type %d greater than max\n", log.event_type);
			continue;
		}
		if(time_relative == 0){ 
			sprintf(ts, "%d.%6.6d",
				((log.time_event >> 20) & 0x0fff),
				((log.time_event & 0x000fffff)));
		} else {
			uint32_t sec, usec;
			if (seen_time == 0) {
				first_timeevent_sec = (log.time_event >> 20) & 0x0fff;
				first_timeevent_usec = (log.time_event & 0x000fffff);
				first_timeevent_usec += add_to_relative;
				if(first_timeevent_usec > 1000000) {
					first_timeevent_sec++;
					first_timeevent_usec -= 1000000;
				}
				seen_time = 1;
			} else if (relative_to_each){
				first_timeevent_sec = (prev_time >> 20) & 0x0fff;
				first_timeevent_usec = (prev_time & 0x000fffff);
			}
			sec = (log.time_event >> 20) & 0x0fff;
			usec = (log.time_event & 0x000fffff);
			/* skip any time event before the sync point */
			if(sec < first_timeevent_sec) {
				continue;
			}
			if ((sec == first_timeevent_sec) && 
			    (usec < first_timeevent_usec)) {
				continue;
			}
			/* Now print and continue. */
			if(sec > first_timeevent_sec) {
				/* do we need to borrow? */
				if(first_timeevent_usec > usec) {
					/* a borrow is in order */
					sec--;
					usec += 1000000;
				}
				sec -= first_timeevent_sec;
				usec -= first_timeevent_usec;
			} else {
				sec -= first_timeevent_sec;
				usec -= first_timeevent_usec;
			}
			prev_time = log.time_event;
			sprintf(ts, "%d.%6.6d", sec, usec);
		}
		if(log.event_type == SCTP_LOG_EVENT_CWND) {
			if((log.from == SCTP_CWND_LOG_NOADV_CA) ||
			   (log.from == SCTP_CWND_LOG_NOADV_SS)) {
				printf("%s: Net:%x Cwnd:%d flt:%d flt+acked:%d (atpc:%d npc:%d) %s (pc=%x, sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       log.x.cwnd.meets_pseudo_cumack,
				       log.x.cwnd.need_new_pseudo_cumack,
				       from_str[log.from],
				       (u_int)log.x.cwnd.pseudo_cumack,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str
					);

			} else if (log.from == SCTP_CWND_LOG_FROM_T3){
				printf("%s Network:%x at T3 cwnd:%d flight:%d pc:%x (net==lnet ?:%d,sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (int)log.x.cwnd.pseudo_cumack,
				       (int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);

			} else if (log.from == SCTP_CWND_LOG_FILL_OUTQ_FILLS) {
				printf("%s fill_outqueue adds %d bytes onto net:%x cwnd:%d flight:%d (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (int)log.x.cwnd.cwnd_augment,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);


			} else if (log.from == SCTP_CWND_LOG_FILL_OUTQ_CALLED) {
				printf("%s fill_outqueue called on net:%x cwnd:%d flight:%d (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);

			} else if (log.from == SCTP_SEND_NOW_COMPLETES) {
				printf("%s Send completes sending %d (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);
			} else if (log.from == SCTP_CWND_LOG_FROM_BRST){
				printf("%s Network:%x at cwnd_event (BURST) cwnd:%d flight:%d decrease by %d (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (u_int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);

			} else if (log.from == SCTP_CWND_LOG_FROM_SACK){
				printf("%s Net:%x at cwnd_event (SACK) cwnd:%d flight:%d pq:%x atpc:%d needpc:%d (tsn:%x,sendcnt:%d,strcnt:%d) \n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (u_int)log.x.cwnd.pseudo_cumack,
				       log.x.cwnd.meets_pseudo_cumack,
				       log.x.cwnd.need_new_pseudo_cumack,
				       (u_int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);

			} else if (log.from == SCTP_CWND_INITIALIZATION){
				printf("%s Network:%x initializes cwnd:%d flight:%d pq:%x (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (u_int)log.x.cwnd.pseudo_cumack,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);
			} else if ((log.from == SCTP_CWND_LOG_FROM_SEND) ||
				   (log.from == SCTP_CWND_LOG_FROM_RESEND)){
				printf("%s Network:%x cwnd:%d flight:%d pq:%x %s tsn:%x (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (u_int)log.x.cwnd.pseudo_cumack,
				       from_str[log.from],
				       (u_int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);
			}else {
				printf("%s:CWND %s net:%x cwnd:%d flight:%d aug:%d (sendcnt:%d,strcnt:%d)\n",
				       ts,
				       from_str[log.from],
				       (u_int)log.x.cwnd.net,
				       log.x.cwnd.cwnd_new_value,
				       log.x.cwnd.inflight,
				       (u_int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str);
			}
		}else if(log.event_type == SCTP_LOG_EVENT_NAGLE) {
			int un_sent;
			un_sent = log.x.nagle.total_in_queue - log.x.nagle.total_flight;
			un_sent += ((log.x.nagle.count_in_queue - log.x.nagle.count_in_flight) * 16);

			printf("%d: stcb:%x total_flight:%u total_in_queue:%u c_in_que:%d c_in_flt:%d  un_sent:%d %s\n",
			       at,
			       log.x.nagle.stcb,
			       log.x.nagle.total_flight,
			       log.x.nagle.total_in_queue,
			       log.x.nagle.count_in_queue,
			       log.x.nagle.count_in_flight,
			       (un_sent),
			       from_str[log.from]);
			
		}else if(log.event_type == SCTP_LOG_EVENT_SB) {
			printf("%d: %s stcb:%x sb_cc:%x stcb_sbcc:%x %s:%d\n",
			       at,
			       from_str[log.from],
			       log.x.sb.stcb,
			       log.x.sb.so_sbcc,
			       log.x.sb.stcb_sbcc,
			       ((log.from == SCTP_LOG_SBALLOC) ? "increment" : "decrement"),
			       log.x.sb.incr);
		}else if(log.event_type == SCTP_LOG_EVENT_MBUF) {
			printf("%s mbuf:%x data:%x len:%d flags:%x refcnt:%d extbuf:%x %s\n",
			       ts,
			       (uint32_t)log.x.mb.mp,
			       (uint32_t)log.x.mb.data,
			       (int)log.x.mb.size,
			       (uint32_t)log.x.mb.mbuf_flags,
			       (int)log.x.mb.refcnt,
			       (uint32_t)log.x.mb.ext,
			       from_str[log.from]);
			       
		}else if(log.event_type == SCTP_LOG_EVENT_RTT) {
			if(log.x.rto.rttvar) {
				printf("%d: Net:%x  Old-Rtt:%d Change:%d Direction=%s from:%s\n",
				       at, 
				       log.x.rto.net,
				       log.x.rto.rtt,
				       log.x.rto.rttvar,
				       ((log.x.rto.direction == 0) ? "-" : "+"),
				       from_str[log.from]
					);
			}
		}else if(log.event_type == SCTP_LOG_EVENT_CLOSE) {
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
				"sctp_close() can shutdown",
				"sctp_close() must abort",
				"close from attach",
				"close from abort",
				"sctp_close() called",
				"Unknown"
			};
			if(log.x.close.loc > 18) {
				log.x.close.loc = 18;
			}
			printf("%s: inp:%x sctp_flags:%x stcb:%x asoc state:%x e:%d-%s\n",
			       ts,
			       (u_int)log.x.close.inp,
			       (u_int)log.x.close.sctp_flags,
			       (u_int)log.x.close.stcb,
			       (u_int)log.x.close.state,
			       log.x.close.loc,
			       close_events[log.x.close.loc]);
 		}else if(log.event_type == SCTP_LOG_LOCK_EVENT) {
			printf("%s sock:%x inp:%x inp:%d tcb:%d info:%d sock:%d sockrb:%d socksb:%d cre:%d\n",
			       from_str[log.from],
			       log.x.lock.sock,
			       log.x.lock.inp,
			       log.x.lock.inp_lock,
			       log.x.lock.tcb_lock,
			       log.x.lock.info_lock,
			       log.x.lock.sock_lock,
			       log.x.lock.sockrcvbuf_lock,
			       log.x.lock.socksndbuf_lock,
			       log.x.lock.create_lock
				);
		}else if(log.event_type == SCTP_LOG_EVENT_SACK) {
			if(log.from == SCTP_LOG_NEW_SACK) {
				printf("%s - cum-ack:%x old-cumack:%x dups:%d gaps:%d\n", 
				       from_str[log.from],
				       log.x.sack.cumack,
				       log.x.sack.oldcumack,
				       (int)log.x.sack.numDups,
				       (int)log.x.sack.numGaps
					);
			} else if(log.from == SCTP_LOG_FREE_SENT) {
				printf("%s - cum-ack:%x freeing TSN:%x\n",
				       from_str[log.from],
				       log.x.sack.cumack,
				       log.x.sack.tsn
					);
			} else {
				printf("%s - cum-ack:%x biggestTSNAcked:%x TSN:%x dups:%d gaps:%d\n", 
				       from_str[log.from],
				       log.x.sack.cumack,
				       log.x.sack.oldcumack,
				       log.x.sack.tsn,
				       (int)log.x.sack.numDups,
				       (int)log.x.sack.numGaps
					);

			}
		}else if(log.event_type == SCTP_LOG_EVENT_MBCNT) {
			if(log.from == SCTP_LOG_MBCNT_INCREASE) {
				printf("%s - tqs(%d + %d) = %d mb_tqs(%d + %d) = %d\n",
				       from_str[log.from],
				       log.x.mbcnt.total_queue_size,
				       log.x.mbcnt.size_change,
				       (log.x.mbcnt.total_queue_size + log.x.mbcnt.size_change),
				       log.x.mbcnt.total_queue_mb_size,
				       log.x.mbcnt.mbcnt_change,
				       (log.x.mbcnt.total_queue_mb_size + log.x.mbcnt.mbcnt_change));
			} else if (log.from == SCTP_LOG_MBCNT_DECREASE) {
				printf("%s - tqs(%d - %d) = %d mb_tqs(%d - %d) = %d\n",
				       from_str[log.from],
				       log.x.mbcnt.total_queue_size,
				       log.x.mbcnt.size_change,
				       (log.x.mbcnt.total_queue_size - log.x.mbcnt.size_change),
				       log.x.mbcnt.total_queue_mb_size,
				       log.x.mbcnt.mbcnt_change,
				       (log.x.mbcnt.total_queue_mb_size - log.x.mbcnt.mbcnt_change));

			} else {
				printf("Unkowns MBCNT event %d\n", log.from);
			}
		}else if(log.event_type == SCTP_LOG_EVENT_RWND) {
			if(log.from == SCTP_INCREASE_PEER_RWND) {
				printf("%s - rwnd:%d increase:(%d + overhead:%d) -> %d\n",
				       from_str[log.from],
				       log.x.rwnd.rwnd,
				       log.x.rwnd.send_size,
				       log.x.rwnd.overhead,
				       (log.x.rwnd.rwnd + log.x.rwnd.send_size + log.x.rwnd.overhead));
			} else if (log.from == SCTP_DECREASE_PEER_RWND) {
				printf("%s - rwnd:%d decrease:(%d + overhead:%d) -> %d\n",
 				       from_str[log.from],
				       log.x.rwnd.rwnd,
				       log.x.rwnd.send_size,
				       log.x.rwnd.overhead,
				       (log.x.rwnd.rwnd - (log.x.rwnd.send_size + log.x.rwnd.overhead)));

			} else if (log.from == SCTP_SET_PEER_RWND_VIA_SACK) {
				printf("%s - rwnd:%d decrease:(%d + overhead:%d) arwnd:%d ->%d\n",
 				       from_str[log.from],
				       log.x.rwnd.rwnd,
				       log.x.rwnd.send_size,
				       log.x.rwnd.overhead,
				       log.x.rwnd.new_rwnd,
				       (log.x.rwnd.new_rwnd - (log.x.rwnd.send_size + log.x.rwnd.overhead))
					);
			} else {
				printf("Unknown RWND event\n");
			}
		}else if(log.event_type == SCTP_LOG_EVENT_MAXBURST) {
			if(log.from == SCTP_MAX_BURST_ERROR_STOP) {
				printf("%s: Network:%x Flight:%d burst cnt:%d - send error:%d %s (sendcnt:%d strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cwnd_new_value,
				       from_str[log.from],
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str
					);
			} else if (log.from == SCTP_MAX_BURST_APPLIED) {
				printf("%s: Network:%x Flight:%d burst cnt:%d %s (sendcnt:%d strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from],
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str
					);
			} else if (log.from == SCTP_MAX_IFP_APPLIED) {
				printf("%s: Network:%x Flight:%d ifp_send_qsize: %d ifp_send_qlimit:%d %s (sendcnt:%d strcnt:%d)\n",
				       ts,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from],
				       (int)log.x.cwnd.cnt_in_send,
				       (int)log.x.cwnd.cnt_in_str
					);
			}

		} else if (log.event_type == SCTP_LOG_MISC_EVENT) {
#ifdef __APPLE__
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

			switch (log.x.misc.log1) {
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

			switch (log.x.misc.log4) {
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
			       log.x.misc.log2,
			       operation,
			       log.x.misc.log3);
#else
			if(log.from == SCTP_REASON_FOR_SC) {
				printf("%s:%s num_out:%u reason_code:%u cwnd_full:%u sendonly1:%u\n",
				       ts,
				       from_str[log.from],
				       log.x.misc.log1,
				       log.x.misc.log2,
				       log.x.misc.log3,
				       log.x.misc.log4);
				       
			} else if (log.from == SCTP_ENTER_USER_RECV) {
				if(!graph_mode) {
					printf("%s user_rcv: dif:%d freed:%d sincelast:%d rwnd_req:%d\n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_USER_RECV_SACKS) {
				if(!graph_mode) {
					if(log.x.misc.log4 == 0) {
						printf("%s no sack rwnd:%d reported:%d sincelast:%d\n",
						       ts,
						       log.x.misc.log1,
						       log.x.misc.log2,
						       log.x.misc.log3);
					} else {
						printf("%s send sack rwnd:%d reported:%d sincelast:%d dif:%d\n",
						       ts,
						       log.x.misc.log1,
						       log.x.misc.log2,
						       log.x.misc.log3,
						       log.x.misc.log4);
					}
				}
			} else if (log.from == SCTP_SORECV_BLOCKSA) {
				if(!graph_mode) {
					printf("%s Enter and block sb_cc:%d reading:%d\n",
					       ts,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_SORECV_BLOCKSB) {
				if(!graph_mode) {
					printf("%s Blocking freed:%d rwnd:%d sb_cc:%d reading:%d\n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_SORECV_DONE) {
				if(graph_mode) {
					printf("%s %d:EXIT\n", ts, log.x.misc.log4);
				} else {
					printf("%s freed_so_far_at_exit:%d last_rep rwnd::%d rwnd:%d sb_cc:%d\n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_SORECV_ENTER) {
				if(graph_mode) {
					printf("%s %d:ENTER\n", ts, log.x.misc.log3);
				} else {
					printf("%s enter srcv rwndreq:%d ieeor:%d sb_cc:%d uioreq:%d \n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_SORECV_ENTERPL) {
				if(graph_mode) {
					printf("%s %d:READ\n", ts, log.x.misc.log3);
				} else {
					printf("%s pass_lock rwndreq:%d canblk:%d sb_cc:%d uioreq:%d \n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else if (log.from == SCTP_SORCV_FREECTL) {
				if(!graph_mode) {
					printf("%s free control sb_cc:%d\n",
					       ts,
					       log.x.misc.log1);
				}
			} else if (log.from == SCTP_SORCV_DOESCPY) {				
				if(!graph_mode) {
					printf("%s copied data of %d  sb_cc:%d\n",
					       ts,
					       log.x.misc.log2,
					       log.x.misc.log1);
				}
			} else if (log.from == SCTP_SORCV_DOESLCK) {
				if(!graph_mode) {
					printf("%s Does the lock cp:%d mlen:%d  sb_cc:%d\n",
					       ts,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log1);
				}
			} else if (log.from == SCTP_SORCV_DOESADJ) {
				if(!graph_mode) {
					printf("%s Does adjust sb_cc:%d\n",
					       ts,
					       log.x.misc.log1);
				}
			} else if (log.from == SCTP_SORCV_PASSBF) {
				if(!graph_mode) {
					printf("%s Past sb_subtract  sb_cc:%d\n",
					       ts,
					       log.x.misc.log1);
				}

			} else if (log.from == SCTP_SORCV_ADJD) {
				if(!graph_mode) {
					printf("%s Adjusts done  sb_cc:%d\n",
					       ts,
					       log.x.misc.log1);
				}

			} else if (log.from == SCTP_SORCV_BOTWHILE) {
				if(!graph_mode) {
					printf("%s Bottom while sb_cc:%d\n",
					       ts,
					       log.x.misc.log1);
				}
			} else if (log.from == SCTP_SACK_RWND_UPDATE) {
				if(comma_sep) {
					if(first_time) {
						fprintf(io, "TS,RWND,A-RWND,FLIGHT\n");
						first_time = 0;
					}
					fprintf(io,"%s,%d,%d,%d\n",
						ts, 
						log.x.misc.log2, 
						log.x.misc.log1,
						log.x.misc.log3);
				} else if(graph_mode) {
					printf("%s %d:RWND\n", ts, log.x.misc.log2);
					printf("%s %d:ARWND\n",ts, log.x.misc.log1);
					printf("%s %d:FLIGHT\n", ts, log.x.misc.log3);
				} else {
					printf("%s s-rwnd:%d calc_rwnd:%d  flight:%d cumacktsn:%x\n",
					       ts,
					       log.x.misc.log1,
					       log.x.misc.log2,
					       log.x.misc.log3,
					       log.x.misc.log4);
				}
			} else {
				printf("%s:%s log1:%u log2:%u log3:%u log4:%u\n",
				       ts,
				       from_str[log.from],
				       log.x.misc.log1,
				       log.x.misc.log2,
				       log.x.misc.log3,
				       log.x.misc.log4);
			}
#endif
		} else if (log.event_type == SCTP_LOG_EVENT_WAKE) {
			char *str;
			switch(log.x.wake.sctpflags) {
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
			printf("%s WUP:%s tcb:%x cnt:%d fs:%d sd:%d st:%d str:%d co:%d) s:%s sb:%x\n",
			       ts,
			       from_str[log.from],
			       log.x.wake.stcb,
			       log.x.wake.wake_cnt,
			       log.x.wake.flight,
			       log.x.wake.send_q,
			       log.x.wake.sent_q,
			       log.x.wake.stream_qcnt,
			       log.x.wake.chunks_on_oque,
			       str,
			       (u_int)log.x.wake.sbflags
				);
		}else if(log.event_type == SCTP_LOG_EVENT_BLOCK) {
			printf("%s:BLK: onq:%d send:%d flight:%d chk:%d sendcnt:%d strmcnt:%d rwnd:%d %s\n",
			       ts,
			       (int)log.x.blk.onsb,
			       (int)log.x.blk.sndlen,
			       (int)(log.x.blk.flight_size * 1024),
			       log.x.blk.chunks_on_oque,
			       log.x.blk.send_sent_qcnt,
			       log.x.blk.stream_qcnt,
			       log.x.blk.peer_rwnd,
			       from_str[log.from]
				);
		}else if(log.event_type == SCTP_LOG_EVENT_STRM) {
			if((log.from == SCTP_STR_LOG_FROM_INSERT_MD) ||
			   (log.from == SCTP_STR_LOG_FROM_INSERT_TL)) {
				/* have both the new entry and 
				 * the previous entry to print.
				 */
				printf("%s: tsn=%x sseq=%u %s tsn=%x sseq=%u\n",
				       from_str[log.from],
				       (u_int)log.x.strlog.n_tsn,
				       log.x.strlog.n_sseq,
				       ((log.from == SCTP_STR_LOG_FROM_INSERT_MD) ? "Before" : "After"),
				       (u_int)log.x.strlog.e_tsn,
				       log.x.strlog.e_sseq
					);
			}else {
				printf("%s: tsn=%x sseq=%x\n",
				       from_str[log.from],
				       (u_int)log.x.strlog.n_tsn,
				       (u_int)log.x.strlog.n_sseq);
			}

		} else if(log.event_type == SCTP_LOG_EVENT_FR) {
			if(log.from == SCTP_FR_LOG_BIGGEST_TSNS) {
				printf("%s: biggest_tsn_in_sack=%x biggest_new_tsn:%x cumack:%x\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       (u_int)log.x.fr.largest_new_tsn,
				       (u_int)log.x.fr.tsn);
			} else if (log.from == SCTP_FR_T3_TIMEOUT) {
				printf("%s: first:%x last:%x cnt:%d\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       (u_int)log.x.fr.largest_new_tsn,
				       (int)log.x.fr.tsn);

			} else if (log.from == SCTP_FR_DUPED) {
				printf("%s: TSN:%x \n",
				       from_str[log.from],
				       (uint)ntohl(log.x.fr.largest_tsn));
			} else if ((log.from == SCTP_FR_CWND_REPORT) ||
                                   (log.from == SCTP_FR_CWND_REPORT_START) ||
                                   (log.from == SCTP_FR_CWND_REPORT_STOP)) {
				printf("%s: net flight:%d net cwnd:%d tot flight:%d\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       (int)log.x.fr.largest_new_tsn,
				       (int)log.x.fr.tsn);
			} else if ((log.from == SCTP_FR_T3_MARKED) ||
				   (log.from == SCTP_FR_MARKED_EARLY) ||
				   (log.from == SCTP_FR_MARKED)) {
				printf("%s: TSN:%x send-count:%d\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       (int)log.x.fr.largest_new_tsn);
			} else if ((log.from == SCTP_FR_T3_STOPPED)  ||
				   (log.from == SCTP_FR_T3_MARK_TIME)) {
				printf("%s: TSN:%x Seconds:%u Usecond:%d\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       log.x.fr.largest_new_tsn,
				       (int)log.x.fr.tsn);
			} else if ((log.from == SCTP_FR_LOG_STRIKE_CHUNK) ||
				   (log.from == SCTP_FR_LOG_CHECK_STRIKE)) {
				printf("%s: biggest_newly_acked:%x TSN:%x sent_flag:%x\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       (u_int)log.x.fr.largest_new_tsn,
				       (u_int)log.x.fr.tsn);
			} else {
				printf("%s: biggest_new_tsn:%x tsn_to_fr:%x fr_tsn_thresh:%x\n",
				       from_str[log.from],
				       (u_int)log.x.fr.largest_tsn,
				       (u_int)log.x.fr.largest_new_tsn,
				       (u_int)log.x.fr.tsn);
			}

#define SCTP_FR_T3_MARKED           27
#define SCTP_FR_T3_STOPPED          28
#define SCTP_FR_MARKED              30


		} else if(log.event_type == SCTP_LOG_EVENT_MAP) {
			if(log.from == SCTP_MAP_SLIDE_FROM) {
				printf("%s: Slide From:%d Slide to:%d lgap:%d\n",
				       from_str[log.from],
				       (int)log.x.map.base,
				       (int)log.x.map.cum,
				       (int)log.x.map.high);
			} else if(log.from == SCTP_MAP_SLIDE_NONE) {
				printf("%s: (distance:%d + slide_from:%d) > array_size:%d (TSNH :-0)\n",
				       from_str[log.from],
				       (int)log.x.map.base,
				       (int)log.x.map.cum,
				       (int)log.x.map.high);
			} else {

				printf("%s: Map Base:%u Cum Ack:%u Highest TSN:%u\n",
				       from_str[log.from],
				       log.x.map.base,
				       log.x.map.cum,
				       log.x.map.high);

			}
		} 
		at++;
	}
	fclose(out);
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
