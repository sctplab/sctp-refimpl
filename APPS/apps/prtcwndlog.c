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

int
main(int argc, char **argv)
{
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
		/* 41 */ "Unknown"
	};
#define FROM_STRING_MAX 41
	FILE *out;
	int at;
	struct sctp_cwnd_log log;
	if(argc < 2){
		printf("use %s log-file\n",argv[0]);
		return (1);
	}
	out = fopen(argv[1],"r");
	if(out == NULL) {
		printf("Can't open file %d\n",errno);	
		return(3);
	}
	at = 0;
	if(log.from >=  FROM_STRING_MAX)
		log.from = FROM_STRING_MAX;

	while(fread((void *)&log, sizeof(log), 1,out) > 0) {
		if(log.event_type == SCTP_LOG_EVENT_CWND) {
			if((log.from == SCTP_CWND_LOG_NOADV_CA) ||
			   (log.from == SCTP_CWND_LOG_NOADV_SS)) {
				printf("%d: Network:%x Cwnd:%d flight:%d flight+acked:%d %s\n",
				       at,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from]);

			}else {
				printf("%d: Network:%x New_cwnd:%d flight:%d cwnd_augment:%d adjusted from:%s\n",
				       at,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from]);
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
				printf("%d: Network:%x Flight:%d burst cnt:%d - send error:%d %s\n",
				       at,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       (int)log.x.cwnd.cwnd_new_value,
				       from_str[log.from]);
			} else if (log.from == SCTP_MAX_BURST_APPLIED) {
				printf("%d: Network:%x Flight:%d burst cnt:%d %s\n",
				       at,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from]);
			} else if (log.from == SCTP_MAX_IFP_APPLIED) {
				printf("%d: Network:%x Flight:%d ifp_send_qsize: %d ifp_send_qlimit:%d %s\n",
				       at,
				       (u_int)log.x.cwnd.net,
				       (int)log.x.cwnd.inflight,
				       (int)log.x.cwnd.cwnd_new_value,
				       (int)log.x.cwnd.cwnd_augment,
				       from_str[log.from]);
			}

		}else if(log.event_type == SCTP_LOG_EVENT_BLOCK) {
			printf("%d:mb-max:%d mb-used:%d sb-max:%d sb-used:%d send/sent cnt:%d strq_cnt:%d from %s\n",
			       at,
			       (log.x.blk.maxmb*1024),
			       (int)log.x.blk.onmb,
			       (log.x.blk.maxsb*1024),
			       (int)log.x.blk.onsb,
			       log.x.blk.send_sent_qcnt,
			       log.x.blk.stream_qcnt,
			       from_str[log.from]);
		}else if(log.event_type == SCTP_LOG_EVENT_STRM) {
			   if((log.from == SCTP_STR_LOG_FROM_INSERT_MD) ||
			      (log.from == SCTP_STR_LOG_FROM_INSERT_TL)) {
				   /* have both the new entry and 
				    * the previous entry to print.
				    */
				   printf("%s: tsn=%u sseq=%u %s tsn=%u sseq=%u\n",
					  from_str[log.from],
					  log.x.strlog.n_tsn,
					  log.x.strlog.n_sseq,
					  ((log.from == SCTP_STR_LOG_FROM_INSERT_MD) ? "Before" : "After"),
					  log.x.strlog.e_tsn,
					  log.x.strlog.e_sseq
					   );
			   }else {
				   printf("%s: tsn=%u sseq=%u\n",
					  from_str[log.from],
					  log.x.strlog.n_tsn,
					  log.x.strlog.n_sseq);
			   }

		} else if(log.event_type == SCTP_LOG_EVENT_FR) {
			if(log.from == SCTP_FR_LOG_BIGGEST_TSNS) {
				printf("%s: biggest_tsn_in_sack=%u biggest_new_tsn:%u cumack:%u\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       log.x.fr.largest_new_tsn,
				       log.x.fr.tsn);
			} else if (log.from == SCTP_FR_T3_TIMEOUT) {
				printf("%s: first:%u last:%u cnt:%d\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       log.x.fr.largest_new_tsn,
				       (int)log.x.fr.tsn);
			} else if ((log.from == SCTP_FR_T3_MARKED) ||
				   (log.from == SCTP_FR_MARKED)) {
				printf("%s: TSN:%u send-count:%d\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       (int)log.x.fr.largest_new_tsn);
			} else if ((log.from == SCTP_FR_T3_STOPPED)  ||
				   (log.from == SCTP_FR_T3_MARK_TIME)) {
				printf("%s: TSN:%u Seconds:%u Usecond:%d\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       log.x.fr.largest_new_tsn,
				       (int)log.x.fr.tsn);
			} else {
				printf("%s: biggest_new_tsn:%u tsn_to_fr:%u fr_tsn_thresh:%u\n",
				       from_str[log.from],
				       log.x.fr.largest_tsn,
				       log.x.fr.largest_new_tsn,
				       log.x.fr.tsn);
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
	return(0);
}
