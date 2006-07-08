#include <userInputModule.h>

#include <distributor.h>
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
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
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>

int print_mode = 0;

static int
cmd_getpegs(int fd)
{
	u_int32_t sctp_pegs[SCTP_NUMBER_OF_PEGS];
	socklen_t siz;
	int i;
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
		"chun_drp", /* 88 */
		"nollsndq", /* 89 */
		"ll_c_err", /* 90 */
		"nollcwnd", /* 91 */
		"nollbrst", /* 92 */
		"no_ifqsp", /* 93 */
		"rcvnotcb", /* 94 */
		"rcvhadtc", /* 95 */
		"pd_InRcv", /* 96 */
		"pdhad2wa", /* 97 */
		"pdhad2rc", /* 98 */
		"pdnostok", /* 99 */
		"entersor", /* 100 */
		"rch_frmk", /* 101 */
		"fndbyaid", /* 102 */
		"bad_asid", /* 103 */
		"efr_strt", /* 104 */
		"efr_stop", /* 105 */
		"efr_expr", /* 106 */
		"efr_mark", /* 107 */
		"t3winprb", /* 108 */
		"rengalls", /* 109 */
		"irengeds", /* 110 */
		"efr_stot", /* 111 */
		"efr_sts1", /* 112 */
		"efr_sts2", /* 113 */
		"efr_sts3", /* 114 */
		"efr_sts4", /* 115 */
		"efr_stai", /* 116 */
		"efr_stao", /* 117 */
		"efr_stat", /* 118 */
		"t3_rtran", /* 119 */

	};
	printf("there are %d pegs\n",
	       SCTP_NUMBER_OF_PEGS);
	siz = sizeof(sctp_pegs);
	if(getsockopt(fd,IPPROTO_SCTP,
		      SCTP_GET_PEGS,
		      sctp_pegs,&siz) != 0){
		printf("Can't get peg counts err:%d\n",errno);
		return(0);
	}
	if(print_mode == 1) {
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
	} else if (print_mode == 0) {
		/* retransmit prints */
		printf("Total number of retransmits is %d\n", (sctp_pegs[SCTP_FAST_RETRAN] + 
							       sctp_pegs[SCTP_FR_INAWINDOW] + 
							       sctp_pegs[SCTP_T3_MARKED_TSNS]));
		printf("Total number of time-outs %d\n", sctp_pegs[SCTP_TMIT_TIMER]);
		printf("Total number of Fast Retransmits %d\n", (sctp_pegs[SCTP_FAST_RETRAN] + 
								 sctp_pegs[SCTP_FR_INAWINDOW]));
		printf("Total number of FR in the same window %d\n", sctp_pegs[SCTP_FR_INAWINDOW]);
	}
	return(0);
}

int
main(int argc, char **argv)
{

	int fd;
 	if(argc > 1) {
		/* get all pegs */
		print_mode = 1;
	}

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd == -1) {
		printf("sorry no socket\n");
		return(-1);
	}
	cmd_getpegs(fd);
	return(0);
}
