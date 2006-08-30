#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <netinet/sctp.h>

main()
{
	int sd;
	int e,f=0;
	char buffer[1000];

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(sd == -1) {
		printf("socket failed %d\n", errno);
	}
	listen(sd, 2);
	e = sctp_recvmsg(sd, buffer, sizeof(buffer),
			 (NULL), 0, NULL, &f);
	printf("Returned %d errno:%d\n", e, errno);
}
