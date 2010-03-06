#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h> //for bzero
#include <arpa/inet.h> //for inet_aton
#include <unistd.h> //for getpid
#include <math.h>
#include <errno.h>
#include <assert.h>
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

/* Bandwidth benchamark. basic algo from osu bandwidth for userspace and kernel stacks
 * 
 */

struct dp{
    int msg_size;
    double tottime;
    double bw;
};
#define PRINTARRAY 100


#define FIELD_WIDTH 20
#define FLOAT_PRECISION 2
    
#define MAX_ALIGNMENT 65536
#define MAX_MSG_SIZE (1<<17) //(1<<22)
#define MYBUFSIZE (MAX_MSG_SIZE + MAX_ALIGNMENT)
#define DATAPOINTS (int)ceil(log(MAX_MSG_SIZE)/log(2))+1

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
extern void sctp_finish(void);

extern int userspace_connect(struct socket *so, struct sockaddr *name, int namelen);
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


int main(int argc, char **argv) {
    int i, j;
    int size, align_size;
    char *s_buf, *r_buf;
    double t_start = 0.0, t_end = 0.0, t = 0.0;


    struct sockaddr_in *dest = malloc(sizeof(struct sockaddr_in));
    struct sctp_sndrcvinfo sri;
    struct sockaddr_in cli;
    socklen_t s = sizeof(struct sockaddr_in);
    int msg_flags = 0;
    int n=-1;
    char mode[15];


    struct timeval start, end;
    struct dp datapoints[DATAPOINTS];
    int num_dp = 0;
    FILE *file;
    char filename[PRINTARRAY];
    char ascii_size[PRINTARRAY];
    int name[] = {CTL_KERN, KERN_OSTYPE};
    int namelen = 2;
    char oldval[15];
    size_t len = sizeof(oldval);
    int  error;    


    if (argc < 3) {
	printf("Usage: %s dst-ip dest-port\n", argv[0]);
	exit(1);
    }
    
#if defined(SCTP_USERMODE)
    uint32_t optval=1;
    struct socket *psock = NULL;
    strcpy(mode, "Userspace");
#else //kernel mode
    int sock_fd;
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

    
    error = sysctl (name, namelen,
                    (void *)oldval, &len, NULL
                    /* newval */, 0 /* newlen */);
    if (error) {
        printf("sysctl() error\n");
        exit(1);
    }

    strcpy(filename, "BWbenchmark");
    sprintf(ascii_size,"%s%smode",oldval,mode);
    strcat(filename, ascii_size);
    strcat(filename,".txt");

    
#if defined(SCTP_USERMODE)
    sctp_init(); 
    SCTP_BASE_SYSCTL(sctp_udp_tunneling_for_client_enable)=1; 
    
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
    
    /* prepare destination adddress */
    bzero(dest, sizeof(struct sockaddr_in));
    dest->sin_family = AF_INET;
    dest->sin_addr.s_addr = inet_addr(argv[1]);
    dest->sin_port = htons((unsigned short) atoi(argv[2]));
#if defined(__Userspace_os_FreeBSD)
    dest->sin_len = sizeof(struct sockaddr);
#endif

#if defined(SCTP_USERMODE)
    /* call userspace_connect which eventually calls sctp_send_initiate */
    if( userspace_connect(psock, (struct sockaddr *) dest, sizeof(struct sockaddr_in)) == -1 ) {
        printf("userspace_connect failed.  exiting...\n");
        exit(1);        
    }

    sctp_setopt(psock, SCTP_NODELAY, &optval, sizeof(uint32_t), NULL);

#else //Kernel mode
    if((connect(sock_fd, (struct sockaddr *) dest, sizeof(struct sockaddr_in))) == -1) {
        perror("connect error\n");
        exit(1);
    }
    /* Setting the send and receive socket buffer sizes */
    const int maxbufsize = 65536 * 3;
    int sndrcvbufsize = maxbufsize;
    int sndrcvbufsize_len;
    int rc;
    sndrcvbufsize_len = sizeof(sndrcvbufsize);
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sndrcvbufsize, sndrcvbufsize_len);
    
    if (rc == -1) {
        perror("can't set the socket send size to requested one");
        exit(1);
    }
    
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sndrcvbufsize, sndrcvbufsize_len);
    
    if (rc == -1) {
        perror("can't set the socket receive size to requested one");
        exit(1);
    }
    
    /* turning nagle off */
    int no_nagle = 1;
    if (setsockopt(sock_fd, IPPROTO_SCTP,
                   SCTP_NODELAY, (char *) &no_nagle, sizeof(no_nagle)) < 0){
        perror("can't setsockopt to no_nagle\n");
        exit(1);
    }
#endif



    num_dp = 0;
    int retval = 0;

    printf("\nRunning in %s mode\nStoring results in file %s\n\n", mode, filename);

    fprintf(stdout, "# %s %s\n", mode, "Mode: Bandwidth Benchmark Similar to OSU");
    fprintf(stdout, "%-*s%*s\n", 10, "# Size", FIELD_WIDTH,
            "Bandwidth (MB/s)");
    fflush(stdout);


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
            if(i == skip) {
                gettimeofday(&start, NULL);
                t_start = (double) start.tv_sec + .000001 * (double) start.tv_usec;
            }
            
            for(j = 0; j < window_size; j++) {
#if defined(SCTP_USERMODE)
                if((retval = userspace_sctp_sendmsg(psock /* struct socket *so */,
                                                    s_buf /* const void *data */,
                                                    size /* size_t len */,
                                                    (struct sockaddr *)dest /* const struct sockaddr *to */,
                                                    sizeof(struct sockaddr_in) /* socklen_t tolen */,
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
                
                if ((retval = sctp_sendmsg(sock_fd, (void *)s_buf, (size_t)size, NULL, 0, 0, 0, 3, 0, 0)) <=0 )
                    {
                        printf("sctp_sendmsg returned retval=%d errno=%d\n", retval, errno);
                        exit(1);
                    }
#endif        
                
                
            }


#if defined(SCTP_USERMODE)
            if ((n = userspace_sctp_recvmsg(psock, r_buf, 4, (struct sockaddr *) &cli, &s, &sri, &msg_flags)) <=0 )
                {
                    printf(".....userspace_sctp_recvmsg returned n=%d errno=%d\n", n, errno);
                    break;
                }
#else  //Kernel mode
            if ((n = sctp_recvmsg(sock_fd, r_buf, 4, (struct sockaddr *) &cli, &s, &sri, &msg_flags)) <=0 )
                {
                    printf("sctp_recvmsg returned n=%d errno=%d\n", n, errno);
                    break;
                }
#endif
            
        }
        
        gettimeofday(&end, NULL);
        t_end = (double) end.tv_sec + .000001 * (double) end.tv_usec;
        
        t = t_end - t_start;
        
        double tmp = size / 1e6 * loop * window_size;
        
        fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                FLOAT_PRECISION, tmp / t);
        fflush(stdout);
        datapoints[num_dp].msg_size = size;
        datapoints[num_dp].tottime = t;
        datapoints[num_dp].bw = tmp/t;
        num_dp++;
        
    }
    
    
    printf("Client closing socket...errno=%d\n", errno);
    free(dest);
#if defined(SCTP_USERMODE)
    userspace_close(psock);
#else
    close(sock_fd);
#endif
    
    printf("Sleeping for 60 secs\n");
    sleep(60);
    
    file = fopen(filename, "w+");
    if(file == NULL) {
        perror("could not open results file");
    } else {        
        for (j=0; j<num_dp; j++)
        {
            fprintf(file,"%d , %.2f , %.2f\n",datapoints[j].msg_size, datapoints[j].tottime, datapoints[j].bw); /*write datapoints to file*/
        }
        fclose(file);
    }


#if defined(SCTP_USERMODE)
    sctp_finish();
    //    pthread_exit(NULL);
#endif

    return 0;
        
}


