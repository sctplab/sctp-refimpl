#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <sys/uio.h>
#include <arpa/inet.h>

int sd;
struct sockaddr_in to;


static void
handle_notification(int fd,char *notify_buf) {
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
	char *str;
	char buf[256];
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

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
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CANT START ASSOC";
			break;
		default:
			str = "UNKNOWN";
		} /* end switch(sac->sac_state) */
		printf("SCTP_ASSOC_CHANGE: %s, assoc=%xh\n", str,
		       (u_int32_t)sac->sac_assoc_id);
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
		default:
			str = "UNKNOWN";
		} /* end switch */
		sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
		if (sin6->sin6_family == AF_INET6) {
			inet_ntop(AF_INET6, (char*)&sin6->sin6_addr, buf, sizeof(buf));
		} else {
			sin = (struct sockaddr_in *)&spc->spc_aaddr;
			inet_ntop(AF_INET, (char*)&sin->sin_addr, buf, sizeof(buf));
		}
		printf("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=%xh\n", str,
		       buf, (u_int32_t)spc->spc_assoc_id);
		break;
	case SCTP_REMOTE_ERROR:
		sre = &snp->sn_remote_error;
		printf("SCTP_REMOTE_ERROR: assoc=%xh\n",
		       (u_int32_t)sre->sre_assoc_id);
		break;
	case SCTP_SEND_FAILED:
		ssf = &snp->sn_send_failed;
		printf("SCTP_SEND_FAILED: assoc=%xh\n",
		       (u_int32_t)ssf->ssf_assoc_id);
		break;
	case SCTP_ADAPTION_INDICATION:
	  {
	    struct sctp_adaptation_event *ae;
	    ae = &snp->sn_adaptation_event;
	    printf("\nSCTP_adaption_indication bits:0x%x\n",
		   (u_int)ae->sai_adaptation_ind);
	  }
	  break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	  {
	    struct sctp_pdapi_event *pdapi;
	    pdapi = &snp->sn_pdapi_event;
	    printf("SCTP_PD-API event:%u\n",
		   pdapi->pdapi_indication);
	    if(pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED){
		    printf("Message was truncated\n");
	    }
	  }
	  break;

	case SCTP_SHUTDOWN_EVENT:
                sse = &snp->sn_shutdown_event;
		printf("SCTP_SHUTDOWN_EVENT: assoc=%xh\n",
		       (u_int32_t)sse->sse_assoc_id);
		break;
	default:
		printf("Unknown notification event type=%xh\n", 
		       snp->sn_header.sn_type);
	} /* end switch(snp->sn_header.sn_type) */
}



void
do_a_read()
{
	struct sctp_sndrcvinfo *s_info;
	int sz,i,disped;
	struct msghdr msg;
	struct iovec iov[2];
	unsigned char from[200];
	char readBuffer[65535];
	char controlVector[65535];
	struct cmsghdr *cmsg;
  

	disped = i = 0;
	s_info = NULL;
	iov[0].iov_base = readBuffer;
	iov[0].iov_len = sizeof(readBuffer);
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msg.msg_name = (caddr_t)from;
	msg.msg_namelen = sizeof(from);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlVector;
	msg.msg_controllen = sizeof(controlVector);
	errno = 0;
	sz = recvmsg(sd,&msg,0);
	if(sz <= 0){
		if((errno != ENOBUFS) && (errno != EAGAIN))
			printf("Read returns %d errno:%d control len is %d msgflg:%x\n",
			       sz,errno,
			       msg.msg_controllen,msg.msg_flags);
		return;
	}
	if (msg.msg_flags & MSG_NOTIFICATION) {
		handle_notification(sd,readBuffer);
		return;
	}
	if(msg.msg_controllen){
		/* parse through and see if we find
		 * the sctp_sndrcvinfo
		 */
		cmsg = (struct cmsghdr *)controlVector;
		while(cmsg){
			if(cmsg->cmsg_level == IPPROTO_SCTP){
				if(cmsg->cmsg_type == SCTP_SNDRCV){
					/* Got it */
					s_info = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
					break;
				}
			}
			cmsg = CMSG_NXTHDR(&msg,cmsg);
		}
	}
	printf("Read %d bytes first string is %s\n",
	       sz,readBuffer);
	if(s_info){
		printf("str:%d sseq:%d flg:%d ppid:%u tsn:%x cum:%x\n",

		       (int)s_info->sinfo_stream,
		       (int)s_info->sinfo_ssn,
		       (int)s_info->sinfo_flags,
		       (int)ntohl(s_info->sinfo_ppid),
		       (u_int)s_info->sinfo_tsn,
		       (u_int)s_info->sinfo_cumtsn);
	}
	return;
}

void
do_listener()
{
	char cmd[128];
	int not_done = 1;
	int i,len;

	while(not_done) {
		if( fgets( cmd, 128, stdin) == NULL) {
			not_done = 0;
			printf("fgets failed err:%d\n",errno);
			continue;
		}
		len = strlen(cmd);
		if(cmd[(len-1)] == '\n') {
			cmd[(len-1)] = 0;
			len--;
		}
		if((strcmp(cmd,"done") == 0) ||
		   (strcmp(cmd,"quit") == 0)){
			not_done = 0;
			printf("exiting\n");
		} else if (strncmp(cmd, "read:",5) == 0) {
			len = strtol(&cmd[5],NULL,0);
			if(len <= 0){
				printf("Sorry number must be postive (specified %d)\n",
				       len);
				continue;
			}
			for(i=0;i<len;i++){
				do_a_read();
			}
			printf("reading completes\n");
		} else {
			printf("Only commands are read:num <or> done %s not known\n",
			       cmd);
		}
	}
}

static int send_at=1;

void
do_sender()
{
	u_int32_t ppid=0;
	u_int32_t flags=0;
	u_int16_t stream=0;
	u_int32_t timetolive=0;
	u_int32_t context=0;
	u_int32_t msg_sz=0;
	char msg[2048];
	char cmd[128];
	int not_done = 1;
	int len,i,j,ret;
	while(not_done) {
		if( fgets( cmd, 128, stdin) == NULL) {
			not_done = 0;
			printf("fgets failed err:%d\n",errno);
			continue;
		}
		len = strlen(cmd);
		if(cmd[(len-1)] == '\n') {
			cmd[(len-1)] = 0;
			len--;
		}
		if((strcmp(cmd,"done") == 0) ||
		   (strcmp(cmd,"quit") == 0)){
			not_done = 0;
			printf("exiting\n");
		} else if (strncmp(cmd, "send:",5) == 0) {
			if(msg_sz == 0){
				printf("size not set\n");
				continue;
			}
			len = strtol(&cmd[5],NULL,0);
			for (i=0; i<len; i++) {
				for (j=0; j<msg_sz; j++) {
					msg[j] = '0' + i;
				}
				sprintf(msg,"%d",send_at++);
				ret = sctp_sendmsg(sd, msg, msg_sz, (struct sockaddr *)&to, 
						   sizeof(to),
						   ppid, flags, stream, timetolive, context);


				printf("return from send %d is %d err:%d\n",
				       i,ret,errno);
			}
			printf("sending is complete\n");

		} else if (strncmp(cmd,"context:",8) == 0) {
			context = strtoul(&cmd[8],NULL,0);
			printf("context set to %u\n",
			       context);
		} else if (strncmp(cmd,"ppid:",5) == 0) {
			ppid = strtoul(&cmd[5],NULL,0);
			printf("context set to %u\n",
			       ppid);
		} else if (strncmp(cmd,"flags:",6) == 0) {
			flags = strtoul(&cmd[6],NULL,0);
			printf("flags set to %x\n",
			       flags);
		} else if (strncmp(cmd,"stream:",7) == 0) {
			stream = strtoul(&cmd[7],NULL,0);
			printf("stream set to %d\n",
			       stream);

		} else if (strncmp(cmd,"ttl:",4) == 0) {
			timetolive = strtoul(&cmd[4],NULL,0);
			printf("ttl set to %d\n",
			       timetolive);
		} else if (strncmp(cmd,"sz:",3) == 0) {
			msg_sz= strtoul(&cmd[3],NULL,0);
			printf("sz set to %d\n",
			       msg_sz);
			if(msg_sz > sizeof(msg)) {
#if defined(_LP64) || defined(__LP64__) || defined(__APPLE__)
				printf("Sorry max size is %lu, override to this value\n",
#else
				printf("Sorry max size is %u, override to this value\n",
#endif
				       sizeof(msg));
				msg_sz = sizeof(msg);
			}
		} else if (strncmp(cmd,"?",1) == 0) {
			goto help;
		} else if (strncmp(cmd,"help",4) == 0) {
		help:
			printf("Commands are:\n");
			printf("   context:ctx\n");
			printf("   stream:strno\n");
			printf("   ttl:time-to-live\n");
			printf("   flags:flags-to-use-in-send(hex)\n");
			printf("   ppid:per-proto-id\n");
			printf("   sz:sz-to-send\n");
		} else {
			printf("Command not recognized '%s'\n",cmd);
		}
	}

}


int
main(int argc, char **argv)
{
	int i;
	u_int16_t his_port=0,my_port=0;
	int host_set=0;
	int send_buf = 8000;
	int rcv_buf = 8000;
	int optlen;
	memset(&to,0,sizeof(to));
	to.sin_family = AF_INET;
	to.sin_len = sizeof(to);

	while ((i= getopt(argc,argv,"p:m:r:s:h:")) != EOF) {
		switch (i) {
		case 's':
			send_buf = strtol(optarg,NULL,0);
			break;
		case 'r':
			rcv_buf = strtol(optarg,NULL,0);
			break;
		case 'p':
			his_port = strtol(optarg,NULL,0);
			break;
		case 'm':
			my_port = strtol(optarg,NULL,0);
			break;
		case 'h':
			if(inet_pton(AF_INET, optarg, &to.sin_addr)) {
				host_set = 1;
			}
			break;
		default:
			goto exit_use;
			break;
		};
	}
	if ((his_port == 0) && (my_port == 0)) {
	exit_use:
		printf("Use %s -p port -h dest-addr  OR -m my-port [-r rcvbuf -s sndbuf]\n",
		       argv[0]);
		return(0);
	}
	optlen = 4;
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
	if(sd == -1) {
		printf("Can't open a socket '%s' err:%d\n",
		       strerror(errno),
		       errno);
		return(0);
	}
	if (setsockopt(sd,SOL_SOCKET, SO_SNDBUF, &send_buf, optlen) != 0){
		printf("Could not set sndbuf '%s err:%d\n",
		       strerror(errno),
		       errno);
	}
	if (setsockopt(sd,SOL_SOCKET, SO_RCVBUF, &rcv_buf, optlen) != 0){
		printf("Could not set rcvbuf '%s err:%d\n",
		       strerror(errno),
		       errno);
	}
	{
		struct linger linger;
		socklen_t oop;
		linger.l_onoff  = 0;
		linger.l_linger = 0;
		oop = sizeof(linger);
		if (getsockopt(sd, SOL_SOCKET, SO_LINGER, (void *)&linger, &oop) < 0) {
			perror("getsockopt");
		} else {
			printf("linger.l_onoff before calls is %d\n",
			       linger.l_onoff);
			printf("linger.l_linger before calls is %d\n",
			       linger.l_linger);
		}
	}

	if(my_port){
		to.sin_port = htons(my_port);
		if(bind(sd,(struct sockaddr *)&to, to.sin_len) < 0) {
			printf("Failed to bind address '%s' err:%d\n",
			       strerror(errno),
			       errno);
			return(0);
		}
		if(listen(sd, 5) == -1) {
			printf("Failed to set listen '%s' err:%d\n",
			       strerror(errno),
			       errno);
			return(0);
		}
		do_listener();
	} else {
		to.sin_port = htons(his_port);
		do_sender();
	}
	{
		struct linger linger;
		socklen_t oop;
		linger.l_onoff  = 0;
		linger.l_linger = 0;
		oop = sizeof(linger);
		if (getsockopt(sd, SOL_SOCKET, SO_LINGER, (void *)&linger, &oop) < 0) {
			perror("getsockopt");
		} else {
			printf("linger.l_onoff after calls is %d\n",
			       linger.l_onoff);
			printf("linger.l_linger after calls is %d\n",
			       linger.l_linger);
		}
	}
	return(0);
}
