#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/sctp_os.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_pcb.h>


/* extern __Userspace__ variable in user_recv_thread.h */
int userspace_rawsctp = -1; /* needs to be declared = -1 */
int userspace_udpsctp = -1; 
int userspace_rawroute = -1;

static void *
recv_function_raw(void *arg)
{
	const int MAXLEN_MBUF_CHAIN = 32; /* What should this value be? */
	struct mbuf *recvmbuf[MAXLEN_MBUF_CHAIN];
	struct iovec recv_iovec[MAXLEN_MBUF_CHAIN];
	int iovcnt = MAXLEN_MBUF_CHAIN;
	/*Initially the entire set of mbufs is to be allocated.
	  to_fill indicates this amount. */
	int to_fill = MAXLEN_MBUF_CHAIN;
	/* iovlen is the size of each mbuf in the chain */
	int i, n, ncounter;
	int iovlen = MCLBYTES;
	int want_ext = (iovlen > MLEN)? 1 : 0;
	int want_header = 0;
	
	while (1) {
		for (i = 0; i < to_fill; i++) {
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
		SCTP_HEADER_LEN(recvmbuf[0]) = n; /* length of total packet */
		
		if (n <= iovlen) {
			SCTP_BUF_LEN(recvmbuf[0]) = n;
			(to_fill)++;
		} else {
			printf("%s: n=%d > iovlen=%d\n", __func__, n, iovlen);
			i = 0;
			SCTP_BUF_LEN(recvmbuf[0]) = iovlen;

			ncounter -= iovlen;
			(to_fill)++;
			do {
				recvmbuf[i]->m_next = recvmbuf[i+1];
				SCTP_BUF_LEN(recvmbuf[i]->m_next) = min(ncounter, iovlen);
				i++;
				ncounter -= iovlen;
				(to_fill)++;
			} while (ncounter > 0);
		}
		assert(to_fill <= MAXLEN_MBUF_CHAIN);
		SCTPDBG(SCTP_DEBUG_INPUT1, "%s: Received %d bytes.", __func__, n);
		SCTPDBG(SCTP_DEBUG_INPUT1, " - calling sctp_input with off=%d\n", (int)sizeof(struct ip));
		
		/* process incoming data */
		/* sctp_input frees this mbuf. */
		sctp_input_with_port(recvmbuf[0], sizeof(struct ip), 0);
	}
	return NULL;
}

static void *
recv_function_udp(void *arg)
{
	const int MAXLEN_MBUF_CHAIN = 32; /* What should this value be? */
	struct mbuf *recvmbuf[MAXLEN_MBUF_CHAIN];
	struct iovec iov[MAXLEN_MBUF_CHAIN];
	/*Initially the entire set of mbufs is to be allocated.
	  to_fill indicates this amount. */
	int to_fill = MAXLEN_MBUF_CHAIN;
	/* iovlen is the size of each mbuf in the chain */
	int i, n, ncounter;
	int iovlen = MCLBYTES;
	int want_ext = (iovlen > MLEN)? 1 : 0;
	int want_header = 0;
	struct ip *ip;
	struct mbuf *ip_m;
	struct msghdr msg;
	struct sockaddr_in src, dst;
	char cmsgbuf[CMSG_SPACE(sizeof (struct in_addr))];
	struct cmsghdr *cmsg;

	
	while (1) {
		for (i = 0; i < to_fill; i++) {
			/* Not getting the packet header. Tests with chain of one run
			   as usual without having the packet header.
			   Have tried both sending and receiving
			 */
			recvmbuf[i] = sctp_get_mbuf_for_msg(iovlen, want_header, M_DONTWAIT, want_ext, MT_DATA);
			iov[i].iov_base = (caddr_t)recvmbuf[i]->m_data;
			iov[i].iov_len = iovlen;
		}
		to_fill = 0;
		bzero((void *)&msg, sizeof(struct msghdr));
		bzero((void *)&src, sizeof(struct sockaddr_in));
		bzero((void *)&dst, sizeof(struct sockaddr_in));
		bzero((void *)cmsgbuf, CMSG_SPACE(sizeof (struct in_addr)));
		
		msg.msg_name = (void *)&src;
		msg.msg_namelen = sizeof(struct sockaddr_in);
		msg.msg_iov = iov;
		msg.msg_iovlen = MAXLEN_MBUF_CHAIN;
		msg.msg_control = (void *)cmsgbuf;
		msg.msg_controllen = CMSG_LEN(sizeof (struct in_addr));
		msg.msg_flags = 0;

		ncounter = n = recvmsg(userspace_udpsctp, &msg, 0);

		assert (n <= (MAXLEN_MBUF_CHAIN * iovlen));
		SCTP_HEADER_LEN(recvmbuf[0]) = n; /* length of total packet */
		
		if (n <= iovlen) {
			SCTP_BUF_LEN(recvmbuf[0]) = n;
			(to_fill)++;
		} else {
			printf("%s: n=%d > iovlen=%d\n", __func__, n, iovlen);
			i = 0;
			SCTP_BUF_LEN(recvmbuf[0]) = iovlen;

			ncounter -= iovlen;
			(to_fill)++;
			do {
				recvmbuf[i]->m_next = recvmbuf[i+1];
				SCTP_BUF_LEN(recvmbuf[i]->m_next) = min(ncounter, iovlen);
				i++;
				ncounter -= iovlen;
				(to_fill)++;
			} while (ncounter > 0);
		}
		assert(to_fill <= MAXLEN_MBUF_CHAIN);

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if ((cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_RECVDSTADDR)) {
				dst.sin_family = AF_INET;
				dst.sin_len = sizeof(struct sockaddr_in);
				dst.sin_port = htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port));
				memcpy((void *)&dst.sin_addr, (const void *)CMSG_DATA(cmsg), sizeof(struct in_addr));
			}
		}
		
		ip_m = sctp_get_mbuf_for_msg(sizeof(struct ip), 1, M_DONTWAIT, 1, MT_DATA);
		
		ip = mtod(ip_m, struct ip *);
		bzero((void *)ip, sizeof(struct ip));
		ip->ip_v = IPVERSION;
		ip->ip_len = n;
		ip->ip_src = src.sin_addr;
		ip->ip_dst = dst.sin_addr;
		SCTP_HEADER_LEN(ip_m) = sizeof(struct ip) + n;
		SCTP_BUF_LEN(ip_m) = sizeof(struct ip);
		SCTP_BUF_NEXT(ip_m) = recvmbuf[0];
		
		SCTPDBG(SCTP_DEBUG_INPUT1, "%s: Received %d bytes.", __func__, n);
		SCTPDBG(SCTP_DEBUG_INPUT1, " - calling sctp_input with off=%d\n", (int)sizeof(struct ip));
		
		/* process incoming data */
		/* sctp_input frees this mbuf. */
		sctp_input_with_port(ip_m, sizeof(struct ip), src.sin_port);
	}
	return NULL;
}

static int
setReceiveBufferSize(int sfd, int new_size)
{
	int ch = new_size;
	if (setsockopt (sfd, SOL_SOCKET, SO_RCVBUF, (void*)&ch, sizeof(ch)) < 0) {
		perror("raw_setReceiveBufferSize setsockopt: SO_RCVBUF failed !\n");
		exit(1);
	}
	printf("raw_setReceiveBufferSize set receive buffer size to : %d bytes\n",ch);
	return 0;
}

static int
setSendBufferSize(int sfd, int new_size)
{
	int ch = new_size;
	if (setsockopt (sfd, SOL_SOCKET, SO_SNDBUF, (void*)&ch, sizeof(ch)) < 0) {
		perror("raw_setSendBufferSize setsockopt: SO_RCVBUF failed !\n");
		exit(1);
	}
	printf("raw_setSendBufferSize set send buffer size to : %d bytes\n",ch);
	return 0;
}

void 
recv_thread_init()
{
	pthread_t recvthreadraw , recvthreadudp;
	int rc;
	const int hdrincl = 1;
	const int on = 1;
	struct sockaddr_in addr_ipv4;

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
		setReceiveBufferSize(userspace_rawsctp, SB_RAW); /* 128K */
		setSendBufferSize(userspace_rawsctp, SB_RAW); /* 128K Is this setting net.inet.raw.maxdgram value? Should it be set to 64K? */
	}

	 /* use UDP socket, create if not initialized */
	if (userspace_udpsctp == -1) {
		if ((userspace_udpsctp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("UDP socket failure");
			exit(1);
		}
		if (setsockopt(userspace_udpsctp, IPPROTO_IP, IP_RECVDSTADDR, (const void *)&on, (int)sizeof(int)) < 0) {
			perror("setsockopt: IP_RECVDSTADDR");
			exit(1);
		}
		memset((void *)&addr_ipv4, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_SIN_LEN
		addr_ipv4.sin_len         = sizeof(struct sockaddr_in);
#endif
		addr_ipv4.sin_family      = AF_INET;
		addr_ipv4.sin_port        = htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port));
		addr_ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(userspace_udpsctp, (const struct sockaddr *)&addr_ipv4, sizeof(struct sockaddr_in)) < 0) {
			perror("bind");
			exit(1);
		}
		setReceiveBufferSize(userspace_rawsctp, SB_RAW); /* 128K */
		setSendBufferSize(userspace_rawsctp, SB_RAW); /* 128K Is this setting net.inet.raw.maxdgram value? Should it be set to 64K? */
	}

	/* start threads here for receiving incoming messages */
	if (userspace_rawsctp != -1) {
		if ((rc = pthread_create(&recvthreadraw, NULL, &recv_function_raw, (void *)NULL))) {
			printf("ERROR; return code from recvthread pthread_create() is %d\n", rc);
			exit(1);
		}
	}
	if (userspace_udpsctp != -1) {
		if ((rc = pthread_create(&recvthreadudp, NULL, &recv_function_udp, (void *)NULL))) {
			printf("ERROR; return code from recvthread pthread_create() is %d\n", rc);
			exit(1);
		}
	}
}
