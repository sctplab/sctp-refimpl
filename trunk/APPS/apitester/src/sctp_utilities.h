int  sctp_one2one(unsigned short port, int should_listen);
int sctp_socketpair(int *);
int sctp_socketpair_reuse(int fd, int *fds);
int sctp_socketstar(int *, int *, unsigned int);
int sctp_shutdown(int);
int sctp_abort(int);
int sctp_enable_non_blocking(int);
int sctp_disable_non_blocking_blocking(int);
int sctp_set_rto_info(int, sctp_assoc_t, uint32_t, uint32_t, uint32_t);
int sctp_set_initial_rto(int , sctp_assoc_t, uint32_t);
int sctp_set_maximum_rto(int , sctp_assoc_t, uint32_t);
int sctp_set_minimum_rto(int , sctp_assoc_t, uint32_t);
int sctp_get_rto_info(int, sctp_assoc_t, uint32_t *, uint32_t *, uint32_t *);
int sctp_get_initial_rto(int fd, sctp_assoc_t, uint32_t *);
int sctp_get_maximum_rto(int fd, sctp_assoc_t, uint32_t *);
int sctp_get_minimum_rto(int fd, sctp_assoc_t, uint32_t *);

int sctp_one2many(unsigned short port);
int sctp_socketpair_1tom(int *fds, sctp_assoc_t *asocids);
int sctp_get_assoc_info(int fd, sctp_assoc_t assoc_id, 
			uint16_t *asoc_maxrxt,
			uint16_t *peer_dest_cnt, 
			uint32_t *peer_rwnd,
			uint32_t *local_rwnd,
			uint32_t *cookie_life);
int sctp_set_assoc_info(int fd, sctp_assoc_t assoc_id, 
			uint16_t asoc_maxrxt,
			uint16_t peer_dest_cnt, 
			uint32_t peer_rwnd,
			uint32_t local_rwnd,
			uint32_t cookie_life);
int sctp_set_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t max);
int sctp_get_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t *max);

int sctp_set_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t dstcnt);
int sctp_get_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t *dst);

int sctp_set_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t rwnd);
int sctp_get_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t *rwnd);

int sctp_set_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t lrwnd);
int sctp_get_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t *lrwnd);

int sctp_set_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t life);
int sctp_get_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t *life);

uint32_t sctp_get_number_of_associations(int);
uint32_t sctp_get_association_identifiers(int, sctp_assoc_t [], unsigned int);

int 
sctp_get_initmsg(int fd, 
		 uint32_t *ostreams,
		 uint32_t *istreams,
		 uint16_t *maxattempt,
		 uint16_t *max_init_timeo);

int 
sctp_set_initmsg(int fd, 
		 uint32_t ostreams,
		 uint32_t istreams,
		 uint16_t maxattempt,
		 uint16_t max_init_timeo);

int sctp_set_im_ostream(int fd, uint32_t ostream);
int sctp_set_im_istream(int fd, uint32_t istream);
int sctp_set_im_maxattempt(int fd, uint16_t max);
int sctp_set_im_maxtimeo(int fd, uint16_t timeo);
int sctp_get_ndelay(int fd, uint32_t *val);
int sctp_set_ndelay(int fd, uint32_t val);

