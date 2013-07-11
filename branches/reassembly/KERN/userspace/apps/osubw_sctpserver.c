#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> //for bzero
#include <arpa/inet.h> //for inet_aton
#include <unistd.h> //for getpid
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#if defined(SCTP_USERMODE)
#include <netinet/sctp_os.h>
#include <pthread.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/sctp_pcb.h>
#else
#include <netinet/sctp.h>
#include <sys/uio.h>
#endif


/* Bandwidth benchmark derived from OSU bw benchmark for userspace and kernel stacks
 * 
 */
    
#define MAX_ALIGNMENT 65536
#define MAX_MSG_SIZE (1<<17) //(1<<22)
#define MYBUFSIZE (MAX_MSG_SIZE + MAX_ALIGNMENT)


int loop = 100;
int window_size = 64;
int skip = 10;

int loop_large = 20;
int window_size_large = 64;
int skip_large = 2;

int large_message_size = 8192;

char s_buf1[MYBUFSIZE];
char r_buf1[MYBUFSIZE];



/* Prototypes*/
#if defined(SCTP_USERMODE)
extern void sctp_init(void);

extern int userspace_bind(struct socket *so, struct sockaddr *name, int namelen);
extern int userspace_listen(struct socket *so, int backlog);
extern struct socket * userspace_accept(struct socket *so, struct sockaddr * aname, socklen_t * anamelen);
extern  void userspace_close(struct socket *so);
extern ssize_t userspace_sctp_sendmsg(struct socket *so, 
                                      const void *data,
                                      size_t len,
                                      struct sockaddr *to,
                                      socklen_t tolen,
                                      u_int32_t ppid,
                                      u_int32_t flags,
                                      u_int16_t stream_no,
                                      u_int32_t timetolive,
                                      u_int32_t context);


extern ssize_t userspace_sctp_recvmsg(struct socket *so,
                                      void *dbuf,
                                      size_t len,
                                      struct sockaddr *from,
                                      socklen_t * fromlen,
                                      struct sctp_sndrcvinfo *sinfo,
                                      int *msg_flags);
extern int sctp_setopt(struct socket *so,
                        int optname,
                        void *optval,
                        size_t optsize,
                        void *p);

#endif


int main(int argc, char **argv) 
{
    int i, j;
    int size, align_size;
    char *s_buf, *r_buf;

    int n=-1, n_remaining, ncounter=0;;
    struct sctp_sndrcvinfo sri;
    socklen_t s=sizeof(struct sockaddr_in);
    int msg_flags=0, retval=-1;
    struct sockaddr_in src, cli;
    char mode[15];
    
    if (argc < 2) {
	printf("Usage: %s server-port\n", argv[0]);
	exit(1);
    }
    
#if defined(SCTP_USERMODE)
    uint32_t optval=1;
    struct socket *psock = NULL;
    struct socket *conn_sock = NULL;
    struct sockaddr_in dest;
    socklen_t dest_len = sizeof(struct sockaddr); 
    strcpy(mode, "Userspace");
#else //kernel mode
    int sock_fd;
    int conn_fd;
    strcpy(mode, "Kernel");
#endif

    
    align_size = getpagesize();
    assert(align_size <= MAX_ALIGNMENT);

    s_buf =
        (char *) (((unsigned long) s_buf1 + (align_size - 1)) /
                  align_size * align_size);
    r_buf =
        (char *) (((unsigned long) r_buf1 + (align_size - 1)) /
                  align_size * align_size);


#if defined(SCTP_USERMODE)
    sctp_init();
    SCTP_BASE_SYSCTL(sctp_udp_tunneling_for_client_enable)=0; 

    if( !(psock = userspace_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) ){
        printf("user_socket() returned NULL\n");
        exit(1);
    }
#else //Kernel mode
    if((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1) {
        printf("socket error\n");
        exit(1);
    }
#endif
    
    /* prepare source address */
    bzero(&src, sizeof(struct sockaddr_in));
    src.sin_family = AF_INET;
    src.sin_addr.s_addr = INADDR_ANY;
    src.sin_port = htons((unsigned short) atoi(argv[1]));
#if defined(__Userspace_os_FreeBSD)
    src.sin_len = sizeof(struct sockaddr);
#endif

#if defined(SCTP_USERMODE)    
    if(userspace_bind(psock, (struct sockaddr *)&src, sizeof(src)) == -1) {
        printf("userspace_bind failed.  exiting...\n");
        exit(1);
    }
    
    if(userspace_listen(psock, 10) == -1) {
        printf("userspace_listen failed.  exiting...\n");
        exit(1);        
    }
 
    printf("Calling accept in %s mode...\n", mode); 

    if( (conn_sock = userspace_accept(psock, (struct sockaddr *) &dest, &dest_len))== NULL) {
        printf("userspace_accept failed.  exiting...\n");
        exit(1);        
    }

    sctp_setopt(conn_sock, SCTP_NODELAY, &optval, sizeof(uint32_t), NULL);

#else //Kernel mode

    /* Setting the send and receive socket buffer sizes */
    const int maxbufsize = 65536 * 3;
    int sndrcvbufsize = maxbufsize;
    int sndrcvbufsize_len;
    int rc;
    sndrcvbufsize_len = sizeof(sndrcvbufsize);
    if((bind(sock_fd, (struct sockaddr *) &src, sizeof(src))) == -1) {
        perror("bind error\n");
        exit(1);
    }
  
    if((listen(sock_fd, 10)) == -1) {
        perror("listen error\n");
        exit(1);
    }

    printf("Calling accept in %s mode...\n", mode);
    
    if((conn_fd = accept(sock_fd, (struct sockaddr *)NULL, (socklen_t *) NULL)) == -1) {
        perror("accept error\n");
        exit(1);
    }

    rc = setsockopt(conn_fd, SOL_SOCKET, SO_SNDBUF, &sndrcvbufsize, sndrcvbufsize_len);
    
    if (rc == -1) {
        perror("can't set the socket send size to requested one");
        exit(1);
    }
    
    rc = setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &sndrcvbufsize, sndrcvbufsize_len);
    
    if (rc == -1) {
        perror("can't set the socket receive size to requested one");
        exit(1);
    }    

    /* turning nagle off */
    int no_nagle = 1;
    if (setsockopt(conn_fd, IPPROTO_SCTP,
                   SCTP_NODELAY, (char *) &no_nagle, sizeof(no_nagle)) < 0){
        perror("can't setsockopt to no_nagle\n");
        exit(1);
    }
    

    
#endif

    /* Bandwidth test */
    for(size = 1; size <= MAX_MSG_SIZE; size *= 2) {
        /* touch the data */
        for(i = 0; i < size; i++) {
            s_buf[i] = 'a';
            r_buf[i] = 'b';
        }
        
        if(size > large_message_size) {
            loop = loop_large;
            skip = skip_large;
            window_size = window_size_large;
        }

        for(i = 0; i < loop + skip; i++) {
            for(j = 0; j < window_size; j++) {

                n_remaining = size;
                ncounter = 0;
                do{
                
#if defined(SCTP_USERMODE)
                    if ((n = userspace_sctp_recvmsg(conn_sock, (void *)r_buf, size, (struct sockaddr *) &cli, &s, &sri, &msg_flags)) <=0 )
                        {
                            printf("userspace_sctp_recvmsg returned n=%d errno=%d\n", n, errno);
                            break;
                        }
#else  //Kernel mode
                    if ((n = sctp_recvmsg(conn_fd, (void *)r_buf, size, (struct sockaddr *) &cli, &s, &sri, &msg_flags)) <=0 )
                        {
                            printf("sctp_recvmsg returned n=%d errno=%d\n", n, errno);
                            break;
                        }                
#endif
                    n_remaining -= n;
                    ncounter += n;

                }while(n_remaining>0);

                assert(ncounter == size);

                
            }

#if defined(SCTP_USERMODE)         
            if ((retval = userspace_sctp_sendmsg(conn_sock /* struct socket *so */,
                                                 s_buf /* const void *data */,
                                                 4     /*sizeof(buf)*/ /* size_t len */,
                                                 (struct sockaddr *)&dest /* const struct sockaddr *to */,
                                                 dest_len /* socklen_t tolen */,
                                                 0 /* u_int32_t ppid */,
                                            0 /* u_int32_t flags */,
                                                 3 /* u_int16_t stream_no */,
                                                 0 /* u_int32_t timetolive */,
                                                 0 /* u_int32_t context */))<=0)
                {
                    printf("userspace_sctp_sendmsg returned retval=%d errno=%d\n", retval, errno);
                    exit(1);
                    
                }
              
#else //Kernel mode
            
            if ((retval = sctp_sendmsg(conn_fd, (void *)s_buf, 4, NULL, 0, 0, 0, 3, 0, 0)) <=0 )
                {
                    printf("sctp_sendmsg returned retval=%d errno=%d\n", retval, errno);
                    exit(1);
                }
#endif        

            
        }
        

    }

    
    printf("Server closing both sockets...errno=%d\n", errno);
#if defined(SCTP_USERMODE)
    userspace_close(conn_sock); 
    userspace_close(psock); 
    //    pthread_exit(NULL);
#else
    close(sock_fd);
    close(conn_fd);
    //    return 0;
#endif
    printf("Sleeping for 60 secs\n");
    sleep(60);
    return 0;

}


