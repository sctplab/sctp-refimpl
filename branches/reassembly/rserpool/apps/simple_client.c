#include <sys/types.h>
#include <stdio.h>

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

#include <netinet/sctp.h>

#include <rserpool_lib.h>
#include <rserpool.h>
#include <rserpool_io.h>
#include <rserpool_util.h>


int
main(int argc, char **argv)
{
	char *logdir = NULL;
	struct rsp_info r;
	int i,sd, ret, len;
	int32_t op_scope;
	struct sctp_sndrcvinfo sinfo;
	char *name=NULL;
	char buffer[200], retname[200];
	size_t retnamelen;
	struct pe_address addr;
	socklen_t addrlen;

	while((i= getopt(argc,argv,"c:n:")) != EOF)
	{
		switch(i) {
		case 'c':
			logdir = optarg;
			break;
		case 'n':
			name = optarg;
			break;
		default:
			break;
		};
	}
	if(logdir == NULL) {
		printf("Error, no config dir specified -c dirname\n");
		return(-1);

	}
	if(name == NULL) {
		printf("Error, no send to name -n name\n");
		return(-1);
	}
	sprintf(r.rsp_prefix, "%s",  logdir);
	printf("Initialize with rsp_prefix set to %s\n",
	       r.rsp_prefix);
	op_scope = rsp_initialize(&r);
	if(op_scope < 0) {
		printf("Failed to init rsp errnor:%d\n", errno);
		return(-1);
	}
	printf("Operational scope is %d\n", op_scope);

	bzero((void *)&sinfo, sizeof(struct sctp_sndrcvinfo));
	sd = rsp_socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, op_scope);
	if(sd == -1) {
		printf("Can't init socket error:%d\n", errno);
		return(-1);
	}
	sprintf(buffer, "Hello.Echo Me");
	len = strlen(buffer) + 1;
	ret = rsp_sendmsg(sd, buffer, len, NULL, 0, name, strlen(name), &sinfo, 0);
	printf("rsp_sendmsg returns %d errno:%d\n", ret, errno);
	addrlen = sizeof(addr);
	memset(retname, 0, sizeof(retname));
	retnamelen = sizeof(retname);
	ret = rsp_rcvmsg(sd, buffer, sizeof(buffer), retname, 
			 &retnamelen, (struct sockaddr *)&addr, &addrlen, &sinfo, 0);
	printf("ret == %d from rsp_rcvmsg(errno=%d)\n", ret, errno);
	if(ret > 0) {
		printf("buffer=%s\n", buffer);
		if(retnamelen) {
			printf("returned name is %s\n", name);
		}
	}
	return(0);
}
