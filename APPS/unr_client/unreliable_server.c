#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PORT 30002
#define SIZE_OF_MESSAGE (512 * 1024)

int main() 
{
	int fd;
	struct sockaddr_in saddr_in;
	char buffer[SIZE_OF_MESSAGE];
	int saddrlen;
	int msg_flags=0;
	int received;
	int cnt;
	struct sctp_sndrcvinfo sri;

    	//Parameters to pass to the SCTP socket
    	struct sctp_initmsg initparams;

    	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
        	perror("Couldn't create socket");

    	bzero(&(saddr_in), sizeof(struct sockaddr_in));
    	saddr_in.sin_family = AF_INET;
    	saddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    	saddr_in.sin_port = htons(PORT);

    	bzero(&initparams, sizeof(initparams));
    	if(setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &initparams, sizeof(initparams))<0)
        	perror("SCTP_NODELAY");

        if (bind(fd, (struct sockaddr *)&(saddr_in),sizeof(struct sockaddr_in)) < 0) 
            	perror("bind");

        if(listen(fd, 5)<0)
            	perror("listen");
	
	bzero(&saddr_in, sizeof(struct sockaddr_in));
	bzero(&sri, sizeof(struct sctp_sndrcvinfo));
	saddrlen=sizeof(struct sockaddr_in);
	while(1){
		received=sctp_recvmsg(fd,                // socket descriptor
			buffer,                        // message received
	                SIZE_OF_MESSAGE,                             // bytes read
	                (struct sockaddr *) &saddr_in,       // remote address
	                (socklen_t *)&saddrlen,             // value-result parameter (remote address len)
	                &sri, &msg_flags              // struct sctp_sndrcvinfo and flags received
		       );
		cnt++;
		printf("%d\r",cnt);
		fflush(stdout);
		if (received <= 0)
			break;
	}
	printf("\n");

	if (close(fd) < 0)
		perror("close");

    	return 0;
}
