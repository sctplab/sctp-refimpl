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

int 
sctp_one2one(unsigned short port, int should_listen)
{
	int fd;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) 
		return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}
	if(should_listen) {
		if (listen(fd, 1) < 0) {
			close(fd);
			return -1;
		}
	}
	return(fd);
}



int sctp_socketpair(int *fds)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	

	/* Get any old port, but listen */
	fd = sctp_one2one(0, 1);
	if (fd  < 0) {
		return -1;
	}

	/* Get any old port, but no listen */
	fds[0] = sctp_one2one(0, 0);
	if (fds[0] < 0) {
		close(fd);
		return -1;
	}
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

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

int sctp_socketpair_reuse(int fd, int *fds)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	

	/* Get any old port, but no listen */
	fds[0] = sctp_one2one(0, 0);
	if (fds[0] < 0) {
		close(fd);
		return -1;
	}
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

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
	return 0;
}


int sctp_socketstar(int *fd, int *fds, unsigned int n)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	unsigned int i, j;
	
	if ((*fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    	return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(*fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(*fd);
		return -1;
	}
	
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (*fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(*fd);
		return -1;
	}

	if (listen(*fd, 1) < 0) {
		close(*fd);
		return -1;
	}

	for (i = 0; i < n; i++){
		if ((fds[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
			close(*fd);
			for (j = 0; j < i; j++ )
				close(fds[j]);
			return -1;
		}

		if (connect(fds[i], (struct sockaddr *) &addr, addr_len) < 0) {
			close(*fd);
			for (j = 0; j <= i; j++ )
				close(fds[j]);
			return -1;
		}
	}

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



int 
sctp_one2many(unsigned short port)
{
	int fd;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}
	return(fd);
}



/* If fds[0] != -1 its a valid 1-2-M socket already open
 * that is to be used with the new association 
 */



int 
sctp_socketpair_1tom(int *fds, sctp_assoc_t *ids)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;

	fd = sctp_one2many(0);
	if (fd == -1) {
		return -1;
	}

	if(fds[0] == -1) {
		fds[0] = sctp_one2many(0);
		if (fds[0]  < 0) {
			close(fd);
			return -1;
		}
	}
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return -1;
	}

	if (sctp_connectx(fds[0], (struct sockaddr *) &addr, 1, NULL) < 0) {
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
		    uint32_t *cookie_life)
{
	struct sctp_assocparams asocinfo;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(asocinfo);
	bzero((void *)&asocinfo, sizeof(asocinfo));
	asocinfo.sasoc_assoc_id = assoc_id;
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO, (void *)&asocinfo, &len);

	if(asoc_maxrxt) 
		*asoc_maxrxt = asocinfo.sasoc_asocmaxrxt;
	if (peer_dest_cnt) 
		*peer_dest_cnt = asocinfo.sasoc_number_peer_destinations;
	if (peer_rwnd) 
		*peer_rwnd = asocinfo.sasoc_peer_rwnd;
	if (local_rwnd)
		*local_rwnd = asocinfo.sasoc_local_rwnd;
	if (cookie_life)
		*cookie_life = asocinfo.sasoc_cookie_life;
	return result;
}

int
sctp_set_assoc_info(int fd, sctp_assoc_t assoc_id, 
		    uint16_t asoc_maxrxt,
		    uint16_t peer_dest_cnt, 
		    uint32_t peer_rwnd,
		    uint32_t local_rwnd,
		    uint32_t cookie_life)
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
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO, (void *)&asocinfo, len);
	return result;
}

int 
sctp_set_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t max)
{
	return(sctp_set_assoc_info(fd, asoc, max, 0, 0, 0, 0));
}

int 
sctp_get_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t *max)
{
	return(sctp_get_assoc_info(fd, asoc, max, NULL, NULL, NULL, NULL));
}

int 
sctp_set_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t dstcnt)
{
	return(sctp_set_assoc_info(fd, asoc, 0, dstcnt, 0, 0, 0));
}

int 
sctp_get_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t *dst)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, dst, NULL, NULL, NULL));
}

int 
sctp_set_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t rwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, rwnd, 0, 0));
}

int 
sctp_get_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t *rwnd)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, rwnd, NULL, NULL));
}


int 
sctp_set_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t lrwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, lrwnd, 0));
}

int 
sctp_get_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t *lrwnd)
{

	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, lrwnd, NULL));
}

int 
sctp_set_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t life)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, 0, life));
}

int 
sctp_get_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t *life)
{
	return(sctp_get_assoc_info(fd, asoc, NULL, NULL, NULL, NULL, life));
}


uint32_t
sctp_get_number_of_associations(int fd)
{
	uint32_t number;
	socklen_t len;
	
	len = (socklen_t) sizeof(uint32_t);
	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_NUMBER, (void *)&number, &len) < 0)
		return -1;
	else
		return number;
}

uint32_t
sctp_get_association_identifiers(int fd, sctp_assoc_t ids[], unsigned int n)
{
	socklen_t len;
	
	len = (socklen_t) (n * sizeof(sctp_assoc_t));
	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST, (void *)ids, &len) < 0)
		return -1;
	else
		return (len / sizeof(sctp_assoc_t));
}


int 
sctp_get_initmsg(int fd, 
		 uint32_t *ostreams,
		 uint32_t *istreams,
		 uint16_t *maxattempt,
		 uint16_t *max_init_timeo)

{
	struct sctp_initmsg initmsg;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(initmsg);
	bzero((void *)&initmsg, sizeof(initmsg));
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG, 
			    (void *)&initmsg, &len);

	if(ostreams) 
		*ostreams = initmsg.sinit_num_ostreams;
	if (istreams)
		*istreams = initmsg.sinit_max_instreams;
	if (maxattempt) 
		*maxattempt = initmsg.sinit_max_attempts;
	if (max_init_timeo)
		*max_init_timeo = initmsg.sinit_max_init_timeo;
	return result;
}

int 
sctp_set_initmsg(int fd, 
		 uint32_t ostreams,
		 uint32_t istreams,
		 uint16_t maxattempt,
		 uint16_t max_init_timeo)

{
	struct sctp_initmsg initmsg;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(initmsg);
	bzero((void *)&initmsg, sizeof(initmsg));
	initmsg.sinit_num_ostreams = ostreams;
	initmsg.sinit_max_instreams = istreams;
	initmsg.sinit_max_attempts = maxattempt;
	initmsg.sinit_max_init_timeo = max_init_timeo;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_INITMSG, 
			    (void *)&initmsg, len);

	return result;
}
int sctp_set_im_ostream(int fd, uint32_t ostream)
{
	return (sctp_set_initmsg(fd, ostream, 0, 0, 0));
}
int sctp_set_im_istream(int fd, uint32_t istream)
{
	return (sctp_set_initmsg(fd, 0, istream, 0, 0));
}
int sctp_set_im_maxattempt(int fd, uint16_t max)
{
	return (sctp_set_initmsg(fd, 0, 0, max, 0));
}
int sctp_set_im_maxtimeo(int fd, uint16_t timeo)
{
	return (sctp_set_initmsg(fd, 0, 0, 0, timeo));
}

int sctp_get_ndelay(int fd, uint32_t *val)
{
	int result;
	socklen_t len;
	len = sizeof(*val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, 
			    val, &len);
	return (result);
}

int sctp_set_ndelay(int fd, uint32_t val)
{
	int result;
	socklen_t len;
	len = sizeof(val);

	result = setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, 
			    &val, len);
	return(result);
}
