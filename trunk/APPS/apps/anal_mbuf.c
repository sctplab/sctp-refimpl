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

struct track_mbufs {
	uint32_t ext;
	int refcnt;
	int linealoc;
	int lastfreed;
	int in_use;
	int from;
	int size;
};

struct track_mbufs list[10000];

int last_free=0;
int highest_added=0;

int
is_ext_in_list(uint32_t ext, int *at)
{
	int listat;
	for(listat = 0; listat <=highest_added; listat++) {
		if(list[listat].in_use == 0)
			continue;
		if(list[listat].ext == ext) {
			*at = listat;
			return(1);
		}
	}
	return(0);
}

int
add_to_list()
{
	int at;

	for(at = last_free; at < 10000; at++) {
		if(list[at].in_use == 0) {
			list[at].in_use = 1;
			if(at > highest_added)
				highest_added = at;
			return(at);
		}
	}
	printf("Help out of space?\n");
	exit(-1);
}

int
del_from_list(int at)
{
	list[at].in_use = 0;
	if(at < last_free)
		last_free = at;

}

static int missed_frees=0;
static int not_founds=0;

void
log_mbuf_input(struct sctp_mbuf_log *mb, int line, int at)
{
	int put_at;
	if(mb->ext == 0)
		return;
	if(is_ext_in_list((uint32_t)mb->ext, &put_at)) {
		/* missed the free */
		missed_frees=0;
	} else {
		put_at =  add_to_list();
	}
	list[put_at].ext = (uint32_t)mb->ext;
	list[put_at].refcnt = (int)mb->refcnt;
	list[put_at].linealoc = line;
	list[put_at].from = at;
	list[put_at].size = (int)mb->size;

}

void
log_mbuf_free(struct sctp_mbuf_log *mb, int line)
{
	int at;
	if(mb->ext == 0)
		return;

	if(is_ext_in_list((uint32_t)mb->ext, &at)) {
		/* found it */
		list[at].lastfreed = line;
		if(mb->refcnt == 1) {
			del_from_list(at);
		} else {
			list[at].refcnt = (int)mb->refcnt;
		}
	} else {
		not_founds++;
	}
}

int
main(int argc, char **argv)
{
	int i, cnt;
	int care_about_local_allocs = 0;
	FILE *out;
	char *logfile=NULL;
	int at;
	struct sctp_cwnd_log log;
	cnt = 0;
	while((i= getopt(argc,argv,"l:c")) != EOF)
	{
		switch(i) {
		case 'c':
			care_about_local_allocs = 1;
			break;
		case 'l':
			logfile = optarg;
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
	for(at=0;at < 10000; at++) {
		list[i].in_use = 0;
	}
	at = 0;



	while(fread((void *)&log, sizeof(log), 1,out) > 0) {
		cnt++;
		if(log.event_type == SCTP_LOG_EVENT_MBUF) {
			if(log.from == SCTP_MBUF_INPUT) { 
				log_mbuf_input(&log.x.mb, cnt, 1);
			} else if (log.from == SCTP_MBUF_IALLOC) {
				if(care_about_local_allocs) {
					log_mbuf_input(&log.x.mb, cnt, 2);
				}
			} else if (log.from == SCTP_MBUF_IFREE) {
				log_mbuf_free(&log.x.mb, cnt);
			} else if (log.from == SCTP_MBUF_ICOPY) {
				log_mbuf_input(&log.x.mb, cnt, 3);
			}else {
				printf("Unknown mbuf event %d\n", log.from);
			}
		} 
		at++;
	}
	fclose(out);
	printf("I had %d not_founds on free and %d missed_frees\n",
	       not_founds, missed_frees);
	for(at=0;at <=highest_added; at++) {
		if(list[at].in_use) {
			printf("Entry %d allocated at line:%d last-freed line:%d ext:%x refcnt:%d size:%d (from:%d)\n",
			       at, 
			       list[at].linealoc,
			       list[at].lastfreed,
			       list[at].ext,
			       list[at].refcnt,
			       list[at].size,
			       list[at].from);
		}
	}
	return(0);
}
