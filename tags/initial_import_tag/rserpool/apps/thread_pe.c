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

#include <pthread.h>
#include <stdio.h>

#define NUM_THREADS     5
#define MAX_BUFFER      512
#define MY_PORT_NUM     9000
#define LOCALTIME_STREAM    0
#define GMT_STREAM      1

char *name;
char *logdir;
pthread_t threads[NUM_THREADS];

void *
pe_thread(void *thrid) {
    int tid;
    tid = (int)thrid;
    int listenSock, connSock, ret;
    struct sockaddr_in servaddr;
    struct sockaddr_in fromaddr;
    socklen_t socklen;
    struct sctp_initmsg initmsg;
    char buffer[MAX_BUFFER];
    char rcvbuf[MAX_BUFFER];

    printf("\t\t\tHello World! It's me, thread #%d!\n", tid);

    bzero((void *)rcvbuf, MAX_BUFFER);
    socklen = sizeof(struct sockaddr_in);
    /* Create SCTP TCP-Style Socket */
    listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

    /* Accept connections from any interface */
    bzero( (void *)&servaddr, sizeof(servaddr) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
    servaddr.sin_port = htons(MY_PORT_NUM);

    ret = bind( listenSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

    printf("Listening on port: 0x%x\n", servaddr.sin_port);
    /* Specify that a maximum of 5 streams will be available per socket */
    memset( &initmsg, 0, sizeof(initmsg) );
    initmsg.sinit_num_ostreams = 5;
    initmsg.sinit_max_instreams = 5;
    initmsg.sinit_max_attempts = 4;
    ret = setsockopt( listenSock, IPPROTO_SCTP, SCTP_INITMSG,
                       &initmsg, sizeof(initmsg) );

    /* Place the server socket into the listening state */
    listen( listenSock, 5 );

    /* Server loop... */
    while( 1 ) {

        /* Await a new client connection */
        printf("Awaiting a new connection\n");
        connSock = accept(listenSock, (struct sockaddr *)NULL, (socklen_t *)NULL);
        printf("Client Connected \n");
        /* New client socket has connected */

        ret = sctp_recvmsg(connSock, rcvbuf, MAX_BUFFER,
                           (struct sockaddr *)&fromaddr, &socklen,
                           NULL, 0);
        if (ret < 0) {
            perror("Error getting data");
            exit(1);
        }
        rcvbuf[ret] = '\0';
        printf("Got [%s]\n", rcvbuf);

        /* Send local time on stream 0 (local time stream) */
        snprintf(buffer, MAX_BUFFER, "I got [%s] from you", rcvbuf);
        printf("Sending [%s]\n",buffer);
        ret = sctp_sendmsg(connSock, (void *)buffer, (size_t)strlen(buffer),
                           NULL, 0, 0, 0, LOCALTIME_STREAM, 0, 0 );
        if (ret < 0) {
            perror("Error sending msg");
            exit(1);
        }

        /* Close the client connection */
        sleep(10);
        close( connSock );
    }
    pthread_exit(NULL);
}

void *
pe_register_sctp (void *thrid) {
    int tid;
    struct rsp_info r;
    int sd, ret, status=0;
    int32_t op_scope;
    uint32_t peid;
    struct asap_error_cause cause;
    struct sctp_sndrcvinfo sinfo;
    char buffer[200], retname[200];
    size_t retnamelen;
    socklen_t addrlen;

    struct sockaddr_in addr;
    tid = (int)thrid;

    printf("\nRegistering pe with the enrp server");
    sprintf(r.rsp_prefix, "%s",  logdir);
    printf("Initialize with rsp_prefix set to %s\n",
           r.rsp_prefix);
    op_scope = rsp_initialize(&r);
    if (op_scope < 0) {
        printf("Failed to init rsp errnor:%d\n", errno);
        return NULL;
    }
    printf("Operational scope is %d\n", op_scope);

    sd = rsp_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP, op_scope);
    if(sd == -1) {
        printf("Can't init socket error:%d\n", errno);
        return NULL;
    }

    peid = rsp_register_sctp(sd, name, strlen(name), 1, 0);
    printf("\n%s: return from register:0x%x\n", __FUNCTION__, peid);

    /* FIX Doesn't Work. The new assoc request from client is treated as assoc change from enrp server */
    /* Its one-to-many sctp socket. We can reuse the socket for client to pe also */
    ret = rsp_rcvmsg(sd, buffer, sizeof(buffer), retname,
             &retnamelen, (struct sockaddr *)&addr, &addrlen, &sinfo, 0);

    sleep(20);

    ret = rsp_deregister(sd, name, strlen(name), peid, &cause);
    printf("\n%s: return from deregister:%d\n", __FUNCTION__, ret);
    pthread_join(threads[1], (void **)&status);

    printf("\n%s: exiting register thread", __FUNCTION__);
    pthread_exit(NULL);
}

void *
pe_register_thread (void *thrid) {
    int tid;
    struct rsp_info r;
    int sd, ret, status=0;
    int32_t op_scope;
    uint32_t peid;

    struct asap_error_cause cause;
    struct rsp_register_params rparams;
    struct sockaddr_in addr;
    tid = (int)thrid;

    printf("\nRegistering pe with the enrp server");
    sprintf(r.rsp_prefix, "%s",  logdir);
    printf("Initialize with rsp_prefix set to %s\n",
           r.rsp_prefix);
    op_scope = rsp_initialize(&r);
    if (op_scope < 0) {
        printf("Failed to init rsp errnor:%d\n", errno);
        return NULL;
    }
    printf("Operational scope is %d\n", op_scope);

    sd = rsp_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP, op_scope);
    if(sd == -1) {
        printf("Can't init socket error:%d\n", errno);
        return NULL;
    }




    ((struct sockaddr *)&addr)->sa_family = AF_INET;
    addr.sin_port  = htons(MY_PORT_NUM);
    inet_aton("172.19.140.20", &addr.sin_addr);
    //inet_aton("172.19.211.249", &addr.sin_addr);
    rparams.protocol_type = RSP_PARAM_SCTP_TRANSPORT;
    rparams.transport_use = 1;
    rparams.taddr = (struct sockaddr *)&addr;
    rparams.socklen = sizeof(struct sockaddr_in);
    rparams.cnt_of_addr = 1;
    rparams.reglife = -1; /* infinite */
    rparams.policy = 1; /* Round Robin */
    rparams.policy_value = 0; /* undefined/not used */

    peid = rsp_register(sd, name, strlen(name), &rparams, &cause);
    printf("\n%s: return from register:0x%x\n", __FUNCTION__, peid);

    sleep(20);

    ret = rsp_deregister(sd, name, strlen(name), peid, &cause);
    printf("\n%s: return from deregister:%d\n", __FUNCTION__, ret);
    pthread_join(threads[1], (void **)&status);

    printf("\n%s: exiting register thread", __FUNCTION__);
    pthread_exit(NULL);

}

int
main(int argc, char **argv)
{
    int i=0;
    int sctp = 0;
    int rc, status=0;

	while((i= getopt(argc,argv,"c:n:s")) != EOF)
	{
		switch(i) {
		case 'c':
			logdir = strdup(optarg);
			break;
		case 'n':
			name = strdup(optarg);
			break;
		case 's':
		    sctp = 1;
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

	printf("In main: creating pe register thread \n");
	if (sctp) {
	    rc = pthread_create(&threads[0], NULL, pe_register_sctp, (void *)i);
	    if (rc) {
	        printf("ERROR; return code from pthread_create() is %d\n", rc);
	        exit(-1);
	    }
	} else {
	    rc = pthread_create(&threads[0], NULL, pe_register_thread, (void *)i);
	    if (rc) {
	        printf("ERROR; return code from pthread_create() is %d\n", rc);
	        exit(-1);
	    }
	}

    printf("In main: creating pe data thread \n");
    rc = pthread_create(&threads[1], NULL, pe_thread, (void *)i);
    if (rc) {
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    pthread_join(threads[0], (void **)&status);
    pthread_exit(NULL);
	return(0);
}
