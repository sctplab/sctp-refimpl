#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/sctp.h>

int sctp_socketpair(int *fds)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	unsigned short port1, port2, port3;

	port1 = port2 = port3 = 0;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	getsockname (fd, (struct sockaddr *) &addr, &addr_len);
	port1 = ntohs(addr.sin_port);

	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}

	if ((fds[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fd);
		return -1;
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fds[0], (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	getsockname (fds[0], (struct sockaddr *) &addr, &addr_len);
	port2 = ntohs(addr.sin_port);

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	port3 = ntohs(addr.sin_port);

	if (connect(fds[0], (struct sockaddr *) &addr, addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	if ((fds[1] = accept(fd, NULL, 0)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	close(fd);
	return 0;
}

int sctp_shutdown(int fd) {
	return shutdown(fd, SHUT_WR);
}

int sctp_abort(int fd) {
    struct linger l;
    
    l.l_onoff  = 1;
    l.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof (struct linger)) < 0)
    	return -1;
    return close(fd);
 }

int sctp_enable_non_blocking(int fd)
{
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags  | O_NONBLOCK);
}

int sctp_disable_non_blocking_blocking(int fd)
{
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags  & ~O_NONBLOCK);
}

int sctp_set_rto_info(int fd, sctp_assoc_t assoc_id, uint32_t init, uint32_t max, uint32_t min)
{
	struct sctp_rtoinfo rtoinfo;
	socklen_t len;
	
	len = (socklen_t)sizeof(struct sctp_rtoinfo);
	bzero((void *)&rtoinfo, sizeof(struct sctp_rtoinfo));
	
	rtoinfo.srto_assoc_id = assoc_id;
	rtoinfo.srto_initial  = init;
	rtoinfo.srto_max      = max;
	rtoinfo.srto_min      = min;

	return setsockopt(fd, IPPROTO_SCTP, SCTP_RTOINFO, (const void *)&rtoinfo, len);
}

int sctp_set_initial_rto(int fd, sctp_assoc_t assoc_id, uint32_t init)
{
	return sctp_set_rto_info(fd, assoc_id, init, 0, 0);
}

int sctp_set_maximum_rto(int fd, sctp_assoc_t assoc_id, uint32_t max)
{
	return sctp_set_rto_info(fd, assoc_id, 0, max, 0);
}

int sctp_set_minimum_rto(int fd, sctp_assoc_t assoc_id, uint32_t min)
{
	return sctp_set_rto_info(fd, assoc_id, 0, 0, min);
}

int sctp_get_rto_info(int fd, sctp_assoc_t assoc_id, uint32_t *init, uint32_t *max, uint32_t *min)
{
	struct sctp_rtoinfo rtoinfo;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(struct sctp_rtoinfo);
	bzero((void *)&rtoinfo, sizeof(struct sctp_rtoinfo));
	rtoinfo.srto_assoc_id = assoc_id;
	
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_RTOINFO, (void *)&rtoinfo, &len);	

	if (init)
		*init = rtoinfo.srto_initial;
	if (max)
		*max = rtoinfo.srto_max;
	if (min)
		*min = rtoinfo.srto_min;

	return result;
}

int sctp_get_initial_rto(int fd, sctp_assoc_t assoc_id, uint32_t *init)
{
	return sctp_get_rto_info(fd, assoc_id, init, NULL, NULL);
}

int sctp_get_minimum_rto(int fd, sctp_assoc_t assoc_id, uint32_t *min)
{
	return sctp_get_rto_info(fd, assoc_id, NULL, NULL, min);
}

int sctp_get_maximum_rto(int fd, sctp_assoc_t assoc_id, uint32_t *max)
{
	return sctp_get_rto_info(fd, assoc_id, NULL, max, NULL);
}





static sctp_assoc_t 
__get_assoc_id (int fd, struct sockaddr *addr)
{
	struct sctp_paddrinfo sp;
	socklen_t siz;
	struct sockaddr *sa;
	socklen_t sa_len;

	/* First get the assoc id */
	siz = sizeof(sp);
	memset(&sp,0,sizeof(sp));
	if(addr->sa_family == AF_INET) {
		sa_len = sizeof(struct sockaddr_in);
	} else if (addr->sa_family == AF_INET6) {
		sa_len = sizeof(struct sockaddr_in6);
	} else {
		return ((sctp_assoc_t)0);
	}
	memcpy((caddr_t)&sp.spinfo_address, addr, sa_len);
	if(getsockopt(fd, IPPROTO_SCTP, SCTP_GET_PEER_ADDR_INFO,
		      &sp, &siz) != 0) {
		return ((sctp_assoc_t)0);
	}
	/* BSD: We depend on the fact that 0 can never be returned */
	return (sp.spinfo_assoc_id);

}

int sctp_socketpair_1tom(int *fds, sctp_assoc_t *ids)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}


	if ((fds[0] = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		close(fd);
		return -1;
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fds[0], (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	if (listen(fds[0], 1) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return -1;
	}

	if (connect(fds[0], (struct sockaddr *) &addr, addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}
	fds[1] = fd;
	/* Now get the assoc-id's if the caller wants them */
	if(ids == NULL)
		return 0;

	ids[0] = __get_assoc_id (fds[0], (struct sockaddr *)&addr);

	if (getsockname (fds[0], (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return -1;
	}

	ids[1] = __get_assoc_id (fds[1], (struct sockaddr *)&addr);
	return 0;
}

int 
sctp_get_assoc_info(int fd, sctp_assoc_t assoc_id, 
		    uint16_t *asoc_maxrxt,
		    uint16_t *peer_dest_cnt, 
		    uint32_t *peer_rwnd,
		    uint32_t *local_rwnd,
		    uint32_t *cookie_life,
		    uint32_t *sack_delay,
		    uint32_t *sack_freq)
{
	struct sctp_assocparams asocinfo;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(asocinfo);
	bzero((void *)&asocinfo, sizeof(asocinfo));
	asocinfo.sasoc_assoc_id = assoc_id;
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO, (void *)&asocinfo, &len);

	if(*asoc_maxrxt) 
		*asoc_maxrxt = asocinfo.sasoc_asocmaxrxt;
	if (peer_dest_cnt) 
		*peer_dest_cnt = asocinfo.sasoc_number_peer_destinations;
	if (peer_rwnd) 
		*peer_rwnd = asocinfo.sasoc_peer_rwnd;
	if (local_rwnd)
		*local_rwnd = asocinfo.sasoc_local_rwnd;
	if (cookie_life)
		*cookie_life = asocinfo.sasoc_cookie_life;
	if (sack_delay) 
		*sack_delay = asocinfo.sasoc_sack_delay;
	if (sack_freq)
		*sack_freq = asocinfo.sasoc_sack_freq;
	return result;
}

int
sctp_set_assoc_info(int fd, sctp_assoc_t assoc_id, 
		    uint16_t asoc_maxrxt,
		    uint16_t peer_dest_cnt, 
		    uint32_t peer_rwnd,
		    uint32_t local_rwnd,
		    uint32_t cookie_life,
		    uint32_t sack_delay,
		    uint32_t sack_freq)
{
	struct sctp_assocparams asocinfo;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(asocinfo);
	bzero((void *)&asocinfo, sizeof(asocinfo));
	asocinfo.sasoc_assoc_id = assoc_id;
	asocinfo.sasoc_asocmaxrxt = asoc_maxrxt;
	asocinfo.sasoc_number_peer_destinations = peer_dest_cnt;
	asocinfo.sasoc_peer_rwnd = peer_rwnd;
	asocinfo.sasoc_local_rwnd = local_rwnd;
	asocinfo.sasoc_cookie_life = cookie_life;
	asocinfo.sasoc_sack_delay = sack_delay;
	asocinfo.sasoc_sack_freq = sack_freq;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO, (void *)&asocinfo, len);
	return result;
}

int 
sctp_set_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t max)
{
	return(sctp_set_assoc_info(fd, asoc, max, 0, 0, 0, 0, 0, 0));
}

int 
sctp_get_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t *max)
{
	return(sctp_get_assoc_info(fd, asoc, max, NULL, NULL, NULL, NULL, NULL, NULL));
}

int 
sctp_set_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t dstcnt)
{
	return(sctp_set_assoc_info(fd, asoc, 0, dstcnt, 0, 0, 0, 0, 0));
}

int 
sctp_get_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t *dst)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, dst, NULL, NULL, NULL, NULL, NULL));
}

int 
sctp_set_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t rwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, rwnd, 0, 0, 0, 0));
}

int 
sctp_get_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t *rwnd)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, rwnd, NULL, NULL, NULL, NULL));
}


int 
sctp_set_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t lrwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, lrwnd, 0, 0, 0));
}

int 
sctp_get_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t *lrwnd)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, lrwnd, NULL, NULL, NULL));
}

int 
sctp_set_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t life)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, 0, life, 0, 0));
}

int 
sctp_get_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t *life)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, NULL, life, NULL, NULL));
}

int 
sctp_set_asoc_sack_delay(int fd, sctp_assoc_t asoc, uint32_t delay)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, 0, 0, delay, 0));
}

int 
sctp_get_asoc_sack_delay(int fd, sctp_assoc_t asoc, uint32_t *delay)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, NULL, NULL, delay, NULL));
}

int 
sctp_set_asoc_sack_freq(int fd, sctp_assoc_t asoc, uint32_t freq)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, 0, 0, 0, freq));
}

int 
sctp_get_asoc_(int fd, sctp_assoc_t asoc, uint32_t *freq)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, NULL, NULL, NULL, freq));
}
