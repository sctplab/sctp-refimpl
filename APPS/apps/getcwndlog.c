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
	FILE *out;
	char buf[8000];
	int num,tot, numat;
	struct sctp_cwnd_log_req *req;
	struct sctp_cwnd_log *logp;
	int sd,siz,at,ret;
	int clear=0;
	int xxx;

	if(argc < 2){
		printf("use %s log-file [clear]\n",argv[0]);
		return (1);
	}
	if(argc > 2) {
		if (strncmp("clear", argv[2], 5) == 0){
			printf("I will clear log file after retreiveal\n");
			clear = 1;
		}
	}
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
	if(sd == -1){
		printf("no socket %d\n",errno);
		return(2);
	}
	req = (struct sctp_cwnd_log_req *)buf;
	num = 2000/sizeof(struct sctp_cwnd_log);
	out = fopen(argv[1],"w+");
	if(out == NULL) {
		printf("Can't open file %d\n",errno);	
		return(3);
	}
	at = tot = 0;
	logp = req->log;
	numat = SCTP_STAT_LOG_SIZE;
	while(tot < numat ) {
		memset(buf,0,sizeof(buf));
		req->num_ret = num;
		req->start_at = at;
		req->end_at = (at + (num -1));

		siz = sizeof(struct sctp_cwnd_log_req) + (num * sizeof(struct sctp_cwnd_log));
		if((xxx = getsockopt(sd,IPPROTO_SCTP,
			      SCTP_GET_STAT_LOG, req, &siz)) != 0) {
			printf("error %d can't get\n",errno);
			break;
		}
		if(req->num_in_log < numat) {
			numat = req->num_in_log;
		}
		at += req->num_ret;
		tot += req->num_ret;
		ret = fwrite((void *)logp, sizeof(struct sctp_cwnd_log),
		       req->num_ret,out);
		printf("Got start:%d end:%d num:%d wrote out ret:%d (tot in %d)\n",
		       req->start_at, req->end_at, req->num_ret, ret, req->num_in_log);
	}
	if(clear) {
		if(setsockopt(sd,IPPROTO_SCTP,
			      SCTP_CLR_STAT_LOG, &clear, sizeof(clear)) != 0) {
			printf("error %d can't clear log file\n",errno);
		} else {
			printf ("Log filed cleared\n");
		}
	}
	close(sd);
	fclose(out);
	return(0);
}
