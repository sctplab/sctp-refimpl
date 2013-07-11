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
#include "sctp_csum.h"

#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#ifdef __NetBSD__
#include <sys/inttypes.h>
#endif



static unsigned char buffer1[4100];
static unsigned char buffer2[4100];
static unsigned char buffer3[4100];
static unsigned char buffer4[4100];
static struct sockaddr_in bindto,got;
static socklen_t len;

int
check_buffers()
{
        /* Prepare buffers */
	u_int32_t temp,*csum;
	int ret=0;
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer1,(sizeof(buffer1)-4));
	temp = sctp_csum_finalize(temp);
	csum = (u_int32_t *)(buffer1+(sizeof(buffer1)-4));
	if(*csum != temp){
		printf("Buffer1: Found csum:%x calculated:%x\n",
		       *csum,temp);
		ret++;
	}
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer2,(sizeof(buffer2)-4));
	temp = sctp_csum_finalize(temp);
	csum = (u_int32_t *)(buffer2+(sizeof(buffer2)-4));
	if(*csum != temp){
		printf("Buffer2: Found csum:%x calculated:%x\n",
		       *csum,temp);
		ret++;
	}

	temp = 0xffffffff;
	temp = update_crc32(temp,buffer3,(sizeof(buffer3)-4));
	temp = sctp_csum_finalize(temp);
	csum = (u_int32_t *)(buffer3+(sizeof(buffer3)-4));
	if(*csum != temp){
		printf("Buffer3: Found csum:%x calculated:%x\n",
		       *csum,temp);
		ret++;
	}

	temp = 0xffffffff;
	temp = update_crc32(temp,buffer4,(sizeof(buffer4)-4));
	temp = sctp_csum_finalize(temp);
	csum = (u_int32_t *)(buffer4+(sizeof(buffer4)-4));
	if(*csum != temp){
		printf("Buffer4: Found csum:%x calculated:%x\n",
		       *csum,temp);
		ret++;
	}
	return(ret);
}

static int
my_handle_notification(int fd,char *notify_buf) {
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
	int asocUp;
	char *str;
	char buf[256];
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	snp = (union sctp_notification *)notify_buf;
	asocUp = 0;
	switch(snp->sn_header.sn_type) {
	case SCTP_ASSOC_CHANGE:
		sac = &snp->sn_assoc_change;
		switch(sac->sac_state) {

		case SCTP_COMM_UP:
			str = "COMMUNICATION UP";
			asocUp++;
			break;
		case SCTP_COMM_LOST:
			str = "COMMUNICATION LOST";
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			asocUp++;
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			break;
		case SCTP_CANT_STR_ASSOC:
			str = "CANT START ASSOC";
			printf("EXIT:SCTP_ASSOC_CHANGE: %s, assoc=%xh\n", str,
			       (u_int32_t)sac->sac_assoc_id);
			exit(0);
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
		case SCTP_ADDR_CONFIRMED:
			str = "ADDRESS CONFIRMED";
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
	    struct sctp_adaption_event *ae;
	    ae = &snp->sn_adaption_event;
	    printf("\nSCTP_adaption_indication bits:0x%x\n",
		   (u_int)ae->sai_adaption_ind);
	  }
	  break;
	case SCTP_PARTIAL_DELIVERY_EVENT:
	  {
	    struct sctp_pdapi_event *pdapi;
	    pdapi = &snp->sn_pdapi_event;
	    printf("SCTP_PD-API event:%u\n",
		   pdapi->pdapi_indication);
	    if(pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED){
		    printf("PDI- Aborted\n");
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
	} /* end switch(snp->sn_type) */
	return(asocUp);
}


static char readBuffer[65535];	
static int sz=0;
static char controlVector[65535];
static struct msghdr msg;
int
my_sctpReadInput(int fd,int maxread)
{
	/* receive some number of datagrams and
	 * act on them.
	 */
	struct sctp_sndrcvinfo *s_info;
	int i,disped;

	struct iovec iov[2];
	unsigned char from[200];
	disped = i = 0;

	memset(&msg,0,sizeof(msg));
	memset(controlVector,0,sizeof(controlVector));
	memset(readBuffer,0,sizeof(readBuffer));
	s_info = NULL;
	iov[0].iov_base = readBuffer;
	iov[0].iov_len = maxread;
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msg.msg_name = (caddr_t)from;
	msg.msg_namelen = sizeof(from);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlVector;
	msg.msg_controllen = sizeof(controlVector);
	errno = 0;
	sz = recvmsg(fd,&msg,0);
	printf("Read fd:%d returns %d errno:%d control len is %d msgflg:%x\n",
	       fd,
	       sz,errno,
	       msg.msg_controllen,
	       msg.msg_flags);

	if (msg.msg_flags & MSG_NOTIFICATION) {
		printf("Got a notification\n");
		return(my_handle_notification(fd,readBuffer));
	}else{
		printf("Got data\n");
		return(-1);
	}
}


int
poll_fd(int fd)
{
	int cameup;
	int max,notdone;
	fd_set readfds,writefds,exceptfds;
	struct timeval tv;
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	cameup = 0;
	max = fd + 1;
	notdone = 1;
	printf("poll_fd\n");
	FD_SET(fd,&readfds);
	select(max,&readfds,&writefds,&exceptfds,NULL);
	notdone = 0;
	if(FD_ISSET(fd,&readfds)){
		printf("Read please\n");
		cameup += my_sctpReadInput(fd,4100);
	}
	return(cameup);
}

static sctp_assoc_t
dig_out_asocid()
{
	struct sctp_sndrcvinfo *s_info;
	struct cmsghdr *cmsg;
	s_info = NULL;
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
	}else{
		printf("No CMSG?\n");
		exit(0);
	}
	if(s_info == NULL){
		printf("No sinfo?\n");
		exit(0);
	}
	return(s_info->sinfo_assoc_id);
}


void
process(int fd,int magic)
{
	int fd1,num_asoc,ret,i;
	sctp_assoc_t asoc;
	struct timespec ts;
	num_asoc = 0;
	i = 1;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000;

	while(i < 4099){
		printf("pass %d\n",i);
		while(num_asoc < 2){
			printf("Have %d asoc's waiting for 2\n", num_asoc);
			ret = poll_fd(fd);
			if(ret >0 ){
				num_asoc += ret;
			}else if(ret == 0){
				sleep(1);
			}else if(ret < 0){
				printf("Got data? %d\n",sz);
				sleep(1);
			}
			printf("asoc count is %d\n",num_asoc);
		}
	again:
		printf("Reading for %d bytes from fd:%d\n",
		       i,fd);
		my_sctpReadInput(fd,i);
		if(sz == i){
			memcpy(buffer1,readBuffer,i);
		}else{
			printf("Huh I am messed up read %d wanted %d\n",
			       sz,i);
			goto again;
		}
		if(msg.msg_flags & MSG_EOR){
			printf("Huh got EOR on paritial read?\n");
			exit(0);
		}
		asoc = dig_out_asocid();
		nanosleep(&ts,NULL);

		printf("Got to peeloff %x\n",(u_int)asoc);
		fd1 = sctp_peeloff(fd,asoc);
		if(fd1 == -1){
			printf("peeloff failed %d/err:%d\n",
			       fd1,errno);
			exit(0);
		}
		my_sctpReadInput(fd1,(4100-i));
		if(sz > 0){
			memcpy(&buffer1[i],readBuffer,sz);
			printf("Copied %d bytes\n",sz);
		}else{
			printf("Huh only read %d\n",sz);
		}
		if(magic >= i){
			my_sctpReadInput(fd,i);
		}else{
			my_sctpReadInput(fd,4100);
		}
		if(sz > 0){
			memcpy(buffer2,readBuffer,sz);
			printf("copied %d bytes\n",sz);
		}else{
			printf("Huh only read %d\n",sz);
		}
		my_sctpReadInput(fd1,4100);
		if(sz > 0){
			memcpy(buffer3,readBuffer,sz);
			printf("copied %d bytes\n",sz);
		}else{
			printf("Huh only read %d\n",sz);
		}
		my_sctpReadInput(fd,4100);
		if(sz > 0){
			memcpy(buffer4,readBuffer,sz);
			printf("copied %d bytes\n",sz);
		}else{
			printf("Huh only read %d\n",sz);
		}
		printf("Check buffers\n");
		if(check_buffers()){
			exit(0);
		}
		close(fd1);
		num_asoc = 0;
		return;
	}
}

int
main(int argc, char **argv)
{
	int i,fd;
	u_int16_t myport=0;
	int magic=0;
	struct sctp_event_subscribe event;
	while((i= getopt(argc,argv,"m:M:")) != EOF){
		switch(i){
		case 'm':
			myport = (u_int16_t)strtol(optarg,NULL,0);
			break;
		case 'M':
			magic = strtol(optarg,NULL,0);
			break;
		};
	}
	/**********************socket 1 *******************/
      	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_SCTP);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	len = bindto.sin_len = sizeof(bindto);
	bindto.sin_family = AF_INET;
	printf("bind port %d\n",myport);
	bindto.sin_port = htons(myport);
	if(bind(fd,(struct sockaddr *)&bindto, len) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
		return(-1);
	}
	if(getsockname(fd,(struct sockaddr *)&got,&len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		return(-1);
	}	
	printf("fd uses port %d\n",ntohs(got.sin_port));
	listen(fd,100);
	/* enable all event notifications */
	event.sctp_data_io_event = 1;
	event.sctp_association_event = 1;
	event.sctp_address_event = 0;
	event.sctp_send_failure_event = 1;
	event.sctp_peer_error_event = 1;
	event.sctp_shutdown_event = 1;
	event.sctp_partial_delivery_event = 1;
	event.sctp_adaptation_layer_event = 1;
	if (setsockopt(fd, IPPROTO_SCTP, 
		       SCTP_EVENTS, &event, 
		       sizeof(event)) != 0) {
		printf("Gak, can't set events errno:%d\n",errno);
		exit(0);
	}
	printf("to process\n");
	process(fd,magic);
	return(0);
}

