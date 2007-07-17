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
static struct sockaddr_in bindto,got,to;
static socklen_t len;

static u_int32_t at = 468700;

void
prepare_buffers()
{
        /* Prepare buffers */
	u_int32_t temp,*p;
	int i;

	p = (u_int32_t *)buffer1;
	printf("Buffer1 data value starts at 0x%x", at);
	for(i=0;i<(sizeof(buffer1)/4);i++){
		*p = at;
		at++;
		p++;
	}
	/* backup over the last int */
	p--;
	*p = 0;
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer1,(sizeof(buffer1)-4));
	*p = sctp_csum_finalize(temp);
	printf(", csum is 0x%x\n", *p);

	p = (u_int32_t *)buffer2;
	printf("Buffer2 data value starts at 0x%x", at);
	for(i=0;i<(sizeof(buffer2)/4);i++){
		*p = at;
		at++;
		p++;
	}
	p--;
	*p = 0;
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer2,(sizeof(buffer2)-4));
	*p = sctp_csum_finalize(temp);
	printf(", csum is 0x%x\n", *p);

	p = (u_int32_t *)buffer3;
	printf("Buffer3 data value starts at 0x%x", at);
	for(i=0;i<(sizeof(buffer3)/4);i++){
		*p = at;
		at++;
		p++;
	}
	p--;
	*p = 0;
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer3,(sizeof(buffer3)-4));
	*p = sctp_csum_finalize(temp);
	printf(", csum is 0x%x\n", *p);

	p = (u_int32_t *)buffer4;
	printf("Buffer4 data value starts at 0x%x", at);
	for(i=0;i<(sizeof(buffer4)/4);i++){
		*p = at;
		at++;
		p++;
	}
	p--;
	*p = 0;
	temp = 0xffffffff;
	temp = update_crc32(temp,buffer4,(sizeof(buffer4)-4));
	*p = sctp_csum_finalize(temp);
	printf(", csum is 0x%x\n", *p);
}

static int
my_handle_notification(int fd,char *notify_buf) {
	union sctp_notification *snp;
	struct sctp_assoc_change *sac;
	struct sctp_paddr_change *spc;
	struct sctp_remote_error *sre;
	struct sctp_send_failed *ssf;
	struct sctp_shutdown_event *sse;
	int asocDown;
	char *str;
	char buf[256];
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	asocDown = 0;
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
			asocDown = 1;
			break;
		case SCTP_RESTART:
		        str = "RESTART";
			break;
		case SCTP_SHUTDOWN_COMP:
			str = "SHUTDOWN COMPLETE";
			asocDown = 1;
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
	if(asocDown){
		printf("Bring association back up\n");
		len = sizeof(to);
		if(connect(fd,(struct sockaddr *)&to,len) == -1){
			printf("Sorry connect fails %d\n",errno);
		}
		return(1);
	}
	return(0);
}


	
int
my_sctpReadInput(int fd)
{
	/* receive some number of datagrams and
	 * act on them.
	 */
	struct sctp_sndrcvinfo *s_info;
	int sz,i,disped;
	struct msghdr msg;
	struct iovec iov[2];
	unsigned char from[200];
	char readBuffer[65535];
	char controlVector[65535];
  
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
	sz = recvmsg(fd,&msg,0);
	if(sz <= 0){
		printf("Read returns %d errno:%d control len is %d msgflg:%x\n",
		       sz,errno,
		       msg.msg_controllen,msg.msg_flags);
	}
	if (msg.msg_flags & MSG_NOTIFICATION) {
		return(my_handle_notification(fd,readBuffer));
	}else{
		printf("Huh, I got data?.. ignored (%d bytes)\n",sz);
		return(0);
	}
}


int
clear_fds(int fd,int fd1)
{
	int felldown;
	int max,notdone;
	fd_set readfds,writefds,exceptfds;
	struct timeval tv;
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	felldown = 0;
	FD_SET(fd,&readfds);
	FD_SET(fd1,&readfds);
	if(fd > fd1){
		max = fd + 1;
	}else{
		max = fd1 + 1;
	}
	notdone = 1;
	while(notdone){
		select(max,&readfds,&writefds,&exceptfds,&tv);
		notdone = 0;
		if(FD_ISSET(fd,&readfds)){
			notdone++;
			printf("clearing fd:%d\n",fd);
			felldown += my_sctpReadInput(fd);
			notdone = 1;
		}
		if(FD_ISSET(fd1,&readfds)){
			notdone++;
			printf("clearing fd1:%d\n",fd1);
			felldown += my_sctpReadInput(fd1);
		}
	}
	return(felldown);
}

void
process_out_data(int fd,int fd1)
{
	int notdone,x,ret;
	while(1){
		/* Prepare the buffers */
		prepare_buffers();
		/* send out the 4 buffers */
		ret = sendto(fd,buffer1,sizeof(buffer1),0,
			     (struct sockaddr *)&to,sizeof(to));
		if(ret < sizeof(buffer1)){
			printf("Gak1, error:%d ret:%d\n",errno,ret);
		}
		printf("Sent buffer1 to fd %d\n", fd);

		ret = sendto(fd1,buffer2,sizeof(buffer1),0,
			     (struct sockaddr *)&to,sizeof(to));
		if(ret < sizeof(buffer2)){
			printf("Gak2, error:%d ret:%d\n",errno,ret);
		}
		printf("Sent buffer2 to fd1 %d\n", fd1);

		ret = sendto(fd,buffer3,sizeof(buffer1),0,
			     (struct sockaddr *)&to,sizeof(to));
		if(ret < sizeof(buffer3)){
			printf("Gak3, error:%d ret:%d\n",errno,ret);
		}
		printf("Sent buffer3 to fd %d\n", fd);

		ret = sendto(fd1,buffer4,sizeof(buffer1),0,
			     (struct sockaddr *)&to,sizeof(to));
		if(ret < sizeof(buffer4)){
			printf("Gak4, error:%d ret:%d\n",errno,ret);
		}
		printf("Sent buffer4 to fd1 %d\n", fd1);

		/*  now wait until we get a assoc failing */
		notdone = 1;
		while(notdone){
			x = clear_fds(fd,fd1);
			if(x){
				notdone = 0;
			}
			sleep(1);
		}
	}
}

int
main(int argc, char **argv)
{
	int i,fd,fd1;
	char *addr=NULL;
	u_int16_t port=0;
	int protocol_touse = IPPROTO_SCTP;
	while((i= getopt(argc,argv,"p:h:")) != EOF){
		switch(i){
		case 'h':
			addr = optarg;
			break;
		case 'p':
			port = (u_int16_t)strtol(optarg,NULL,0);
			break;
		};
	}
	memset(&to,0,sizeof(to));
	if((addr == NULL) || (port == 0)){
		printf("Sorry you must specify -h addr and -p port\n");
		return(-1);
	}
	if(inet_pton(AF_INET, addr, (void *) &to.sin_addr)){
		to.sin_len = sizeof(to);
		to.sin_family = AF_INET;
		printf("port selected is %d\n",port);
		printf("addr %x\n",(u_int)ntohl(to.sin_addr.s_addr));
		to.sin_port = htons(port);
	}else{
		printf("Can't translate the address\n");
		return(-1);
	}
	/**********************socket 1 *******************/
      	fd = socket(AF_INET, SOCK_SEQPACKET, protocol_touse);
	if(fd == -1){
		printf("can't open socket:%d\n",errno);
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	len = bindto.sin_len = sizeof(bindto);
	bindto.sin_family = AF_INET;
	bindto.sin_port = 0;
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

	/**********************socket 2 *******************/
      	fd1 = socket(AF_INET, SOCK_SEQPACKET, protocol_touse);
	if(fd1 == -1){
		printf("can't open socket:%d\n",errno);
		close(fd);
		return(-1);
	}
	memset(&bindto,0,sizeof(bindto));
	len = bindto.sin_len = sizeof(bindto);
	bindto.sin_family = AF_INET;
	bindto.sin_port = 0;
	if(bind(fd1,(struct sockaddr *)&bindto, len) < 0){
		printf("can't bind a socket:%d\n",errno);
		close(fd);
		close(fd1);
		return(-1);
	}
	if(getsockname(fd1,(struct sockaddr *)&got,&len) < 0){
		printf("get sockname failed err:%d\n",errno);
		close(fd);
		close(fd1);
		return(-1);
	}	
	printf("fd1 uses port %d\n",ntohs(got.sin_port));

	if(connect(fd,(struct sockaddr *)&to,len) == -1){
		printf("Sorry connect fails %d\n",errno);
		close(fd);
		return(-1);
	}
	if(connect(fd1,(struct sockaddr *)&to,len) == -1){
		printf("Sorry connect fails %d\n",errno);
		close(fd);
		return(-1);
	}
	printf("Connected\n");
	clear_fds(fd,fd1);
	process_out_data(fd,fd1);

	return(0);
}

