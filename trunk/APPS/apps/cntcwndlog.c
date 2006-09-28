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
	int i;
	FILE *out, *io=NULL;
	char *logfile=NULL;
	int seen_time=0;
	uint32_t first_timeevent_sec;
	uint32_t first_timeevent_usec;
	int interval = 1000;
	int time_relative = 0;
	int add_to_relative = 0;
	int fixed_constant = 1;
	int32_t cur_sec, cur_usec, cur_cnt=0;
	int at;
	struct sctp_cwnd_log log;
	while((i= getopt(argc,argv,"l:i:c:ra:")) != EOF)
	{
		switch(i) {
		case 'a':
			add_to_relative = strtol(optarg, NULL, 0);
			break;
		case 'r':
			time_relative = 1;
			break;
		case 'c':
			fixed_constant = strtol(optarg, NULL, 0);
			if(fixed_constant == 0) {
				printf("fixed multiplier cannot be 0\n");
				fixed_constant = 1;
			}
			break;
		case 'i':
			interval = strtol(optarg, NULL, 0);
			break;
		case 'l':
			logfile = optarg;
			break;
		default:
			break;
		};
	}
	if (logfile == NULL) {
		printf("Use %s -l binary_logfile\n", argv[0]);
		return(1);
	}
	out = fopen(logfile,"r");
	if(out == NULL) {
		printf("Can't open file %d\n",errno);	
		return(3);
	}
	at = 0;
	while(fread((void *)&log, sizeof(log), 1,out) > 0) {
		uint32_t sec, usec;
		if (seen_time == 0) {
			cur_sec = (log.time_event >> 20) & 0x0fff;
			cur_usec = (log.time_event & 0x000fffff);
			first_timeevent_sec = (log.time_event >> 20) & 0x0fff;
			first_timeevent_usec = (log.time_event & 0x000fffff);
			first_timeevent_usec += add_to_relative;
			seen_time = 1;
		}
		sec = (log.time_event >> 20) & 0x0fff;
		usec = (log.time_event & 0x000fffff);
		/* skip any time event before the sync point */
		if(time_relative) {
			if(sec < first_timeevent_sec) {
				continue;
			}
			if ((sec == first_timeevent_sec) && 
			    (usec < first_timeevent_usec)) {
				continue;
			}
		}
		/* skip any time event before the sync point */
		if(sec == cur_sec) {
		compare_usec:
			if(usec < (cur_usec + interval)) {
				cur_cnt++;
			} else {
				if(time_relative) {
					if(cur_usec >= first_timeevent_usec) {
						printf("%d.%d  %d\n", 
						       (cur_sec-first_timeevent_sec), 
						       (cur_usec-first_timeevent_usec), 
						       (cur_cnt*fixed_constant));
					} else {
						/* borrow case */
						cur_sec--;
						cur_usec += 1000000;
						printf("%d.%d  %d\n", 
						       (cur_sec-first_timeevent_sec), 
						       (cur_usec-first_timeevent_usec), 
						       (cur_cnt*fixed_constant));
					}
				} else {
					printf("%d.%d  %d\n", 
					       cur_sec,
					       cur_usec,
					       (cur_cnt*fixed_constant));
				}
				cur_sec = (log.time_event >> 20) & 0x0fff;
				cur_usec = (log.time_event & 0x000fffff);
				cur_cnt = 0;
			}
		} else if (sec > cur_sec) {
			/* borrow from sec */
			usec += 1000000;
			goto compare_usec;
		}
		at++;
	}
	if(time_relative) {
		if(cur_usec >= first_timeevent_usec) {
			printf("%d.%d  %d\n", 
			       (cur_sec-first_timeevent_sec), 
			       (cur_usec-first_timeevent_usec), 
			       (cur_cnt*fixed_constant));
		} else {
			/* borrow case */
			cur_sec--;
			cur_usec += 1000000;
			printf("%d.%d  %d\n", 
			       (cur_sec-first_timeevent_sec), 
			       (cur_usec-first_timeevent_usec), 
			       (cur_cnt*fixed_constant));
		}
	} else {
		printf("%d.%d  %d\n", 
		       cur_sec,
		       cur_usec,
		       (cur_cnt*fixed_constant));
	}
						

	fclose(out);
	if(io) {
		fclose(io);
		io = NULL;
	}
	return(0);
}
