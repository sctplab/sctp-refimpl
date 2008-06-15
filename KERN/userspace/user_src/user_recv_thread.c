#include <netinet/sctp_os.h>
#include <pthread.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_pcb.h>


/* extern __Userspace__ variable in user_recv_thread.h */
int userspace_rawsctp = -1; /* needs to be declared = -1 */
int userspace_rawroute = -1;

void *user_recv_thread(void * threadname);

void * (*recvFunction)(void *) = {&user_recv_thread};

static int raw_setReceiveBufferSize(int sfd,int new_size)
{
    int ch = new_size;
    if (setsockopt (sfd, SOL_SOCKET, SO_RCVBUF, (void*)&ch, sizeof(ch)) < 0) {
        perror("raw_setReceiveBufferSize setsockopt: SO_RCVBUF failed !\n");
        exit(1);
    }
    printf("raw_setReceiveBufferSize set receive buffer size to : %d bytes\n",ch);
    return 0;
}

static int raw_setSendBufferSize(int sfd,int new_size)
{
    int ch = new_size;
    if (setsockopt (sfd, SOL_SOCKET, SO_SNDBUF, (void*)&ch, sizeof(ch)) < 0) {
        perror("raw_setSendBufferSize setsockopt: SO_RCVBUF failed !\n");
        exit(1);
    }
    printf("raw_setSendBufferSize set send buffer size to : %d bytes\n",ch);
    return 0;
}


void recv_thread_init()
{
    pthread_t recvthread;
    int rc;
    const int hdrincl = 1;

    /* use raw socket, create if not initialized */
    if (userspace_rawsctp == -1) {
        if ((userspace_rawsctp = socket(AF_INET, SOCK_RAW, IPPROTO_SCTP)) < 0) {
            perror("raw socket failure\n");
            exit(1);
        }
        if (setsockopt(userspace_rawsctp, IPPROTO_IP, IP_HDRINCL, &hdrincl, sizeof(int)) < 0) {
            perror("raw setsockopt failure\n");
            exit(1);
        }
        raw_setReceiveBufferSize(userspace_rawsctp, SB_RAW); /* 128K */
        raw_setSendBufferSize(userspace_rawsctp, SB_RAW); /* 128K Is this setting net.inet.raw.maxdgram value? Should it be set to 64K? */
    }

    
    /* start a thread here for receiving incoming messages */    
    
    rc = pthread_create(&recvthread, NULL, recvFunction, (void *)NULL);
    if (rc){
        printf("ERROR; return code from recvthread pthread_create() is %d\n", rc);
        exit(1);
    }

}

void *user_recv_thread(void * threadname)
{
    const int MAXLEN_MBUF_CHAIN = 32; /* What should this value be? */
    struct mbuf *recvmbuf[MAXLEN_MBUF_CHAIN];
    struct iovec recv_iovec[MAXLEN_MBUF_CHAIN];
    int iovcnt = MAXLEN_MBUF_CHAIN;
    struct ip ipheader;
    int last_start_index = 0;
    /*Initially the entire set of mbufs is to be allocated.
      to_fill indicates this amount. */
    int to_fill = MAXLEN_MBUF_CHAIN;
    /* iovlen is the size of each mbuf in the chain */
    int i, n, ncounter;
    int iovlen = MCLBYTES;
    int want_ext = (iovlen > MLEN)? 1 : 0;
    int want_header = 0;
    
    while(1) {

        for( i=last_start_index; i<to_fill; i++ ){

            /* Not getting the packet header. Tests with chain of one run
               as usual without having the packet header.
               Have tried both sending and receiving
             */

            recvmbuf[i] = sctp_get_mbuf_for_msg(iovlen, want_header, M_DONTWAIT, want_ext, MT_DATA);
            recv_iovec[i].iov_base = (caddr_t)recvmbuf[i]->m_data;
            recv_iovec[i].iov_len = iovlen;
        }
        to_fill = 0;
        
        ncounter = n = readv(userspace_rawsctp, recv_iovec, iovcnt);
        assert (n <= (MAXLEN_MBUF_CHAIN * iovlen));

        
        SCTP_HEADER_LEN(recvmbuf[last_start_index]) = n; /* length of total packet */
        
        if(n <= iovlen){
            SCTP_BUF_LEN(recvmbuf[last_start_index]) = n;
            (to_fill)++;
                
        }
        else {
            printf("%s: n=%d > iovlen=%d\n", __func__, n, iovlen);
            i=last_start_index;
            SCTP_BUF_LEN(recvmbuf[i]) = iovlen;

            ncounter -= iovlen;
            (to_fill)++;
            do{
                recvmbuf[i]->m_next = recvmbuf[i+1];
                SCTP_BUF_LEN(recvmbuf[i]->m_next) = min(ncounter, iovlen);
                i++;
                ncounter -= iovlen;
                (to_fill)++;
                
            }while(ncounter > 0);
        }
        
        assert(to_fill <= MAXLEN_MBUF_CHAIN);
        
        SCTPDBG(SCTP_DEBUG_INPUT1, "%s: Received %d bytes.", __func__, n);
        SCTPDBG(SCTP_DEBUG_INPUT1, " - calling sctp_input with off=%d\n", sizeof(ipheader));
        
        /* process incoming data */
        /* sctp_input frees this mbuf. */
        sctp_input(recvmbuf[last_start_index], sizeof(ipheader));
        
    }

    return NULL;
}


