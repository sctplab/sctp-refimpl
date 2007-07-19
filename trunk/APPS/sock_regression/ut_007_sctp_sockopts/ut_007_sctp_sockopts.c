/*-
 * Copyright (c) 2001-2007 by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2001-2007, by Michael Tuexen, tuexen@fh-muenster.de. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define SCTP_SLEEP_MS	100

static void
sctp_delay(int ms)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = (ms * 1000 * 1000);
	(void)nanosleep(&ts, NULL);
}

static int 
sctp_one2many(unsigned short port, int bindall)
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
	if (bindall) {
		addr.sin_addr.s_addr = 0;
	} else {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	
	if (bind(fd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}
	
	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}
	
	return(fd);
}

static uint32_t
sctp_get_association_identifiers(int fd, sctp_assoc_t ids[], unsigned int n)
{
	socklen_t len;
	
	len = (socklen_t) (n * sizeof(sctp_assoc_t));
	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST,
		(void *)ids, &len) < 0)
		return -1;
	else
		return (len / sizeof(sctp_assoc_t));
}

static sctp_assoc_t 
__get_assoc_id (int fd, struct sockaddr *addr)
{
	struct sctp_paddrinfo sp;
	socklen_t siz;
	socklen_t sa_len;
	int cnt = 0;

	/* First get the assoc id */
 try_again:
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
		if (cnt < 1) {
			cnt++;
			sctp_delay(SCTP_SLEEP_MS);
			goto try_again;
		}
		return ((sctp_assoc_t)0);
	}
	/* BSD: We depend on the fact that 0 can never be returned */
	return (sp.spinfo_assoc_id);
}

/*
 * If fds[0] != -1 its a valid 1-2-M socket already open
 * that is to be used with the new association 
 */
static int 
sctp_socketpair_1tom(int *fds, sctp_assoc_t *ids, int bindall)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	int set=0;
	sctp_assoc_t aid;
	
	fd = sctp_one2many(0, bindall);
	if (fd == -1) {
		printf("Can't get socket\n");
		return -1;
	}
	
	if(fds[0] == -1) {
		fds[0] = sctp_one2many(0, bindall);
		if (fds[0]  < 0) {
			close(fd);
			return -1;
		}
	}
	set = 1;
	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		if(set)
			close(fds[0]);
		close(fd);
		return -1;
	}
	if (bindall) {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	if (sctp_connectx(fds[0], (struct sockaddr *) &addr, 1, &aid) < 0) {
		close(fd);
		if(set)
			close(fds[0]);
		return -1;
	}
	fds[1] = fd;
	/* Now get the assoc-id's if the caller wants them */
	if(ids == NULL)
		return 0;
	
	ids[0] = aid;
	
	if (getsockname (fds[0], (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		printf("Can't get socket name2\n");
		if (set) 
			close (fds[0]);
		return -1;
	}
	if (bindall) {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	ids[1] = __get_assoc_id (fds[1], (struct sockaddr *)&addr);

	return 0;
}

static int
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
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO,
	    (void *)&asocinfo, &len);
	
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

static int
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
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_ASSOCINFO,
	    (void *)&asocinfo, len);

	return result;
}

static int 
sctp_set_asoc_maxrxt(int fd, sctp_assoc_t asoc, uint16_t max)
{
	return(sctp_set_assoc_info(fd, asoc, max, 0, 0, 0, 0));
}

static int 
sctp_set_asoc_peerdest_cnt(int fd, sctp_assoc_t asoc, uint16_t dstcnt)
{
	return(sctp_set_assoc_info(fd, asoc, 0, dstcnt, 0, 0, 0));
}

static int 
sctp_set_asoc_peer_rwnd(int fd, sctp_assoc_t asoc, uint32_t rwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, rwnd, 0, 0));
}

static int 
sctp_set_asoc_local_rwnd(int fd, sctp_assoc_t asoc, uint32_t lrwnd)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, lrwnd, 0));
}

static int 
sctp_set_asoc_cookie_life(int fd, sctp_assoc_t asoc, uint32_t life)
{
	return(sctp_set_assoc_info(fd, asoc, 0, 0, 0, 0, life));
}

static int 
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

static int 
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

static int
sctp_set_im_ostream(int fd, uint32_t ostream)
{
	return (sctp_set_initmsg(fd, ostream, 0, 0, 0));
}

static int
sctp_set_im_istream(int fd, uint32_t istream)
{
	return (sctp_set_initmsg(fd, 0, istream, 0, 0));
}

static int
sctp_set_im_maxattempt(int fd, uint16_t max)
{
	return (sctp_set_initmsg(fd, 0, 0, max, 0));
}

static int
sctp_set_im_maxtimeo(int fd, uint16_t timeo)
{
	return (sctp_set_initmsg(fd, 0, 0, 0, timeo));
}

static int
sctp_get_paddr_param(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint32_t *hbinterval,
    uint16_t *maxrxt,
    uint32_t *pathmtu,
    uint32_t *flags,
    uint32_t *ipv6_flowlabel,
    uint8_t *ipv4_tos)
{
	struct sctp_paddrparams param;
	socklen_t len;
	int result;
	memset(&param, 0, sizeof(param));
	param.spp_assoc_id = id;
	if(sa) {
		if (sa->sa_family == AF_INET) {
			memcpy(&param.spp_address, sa,
			    sizeof(struct sockaddr_in));
		} else if (sa->sa_family == AF_INET6) {
			memcpy(&param.spp_address, sa,
			    sizeof(struct sockaddr_in6));
		} else {
			errno = EINVAL;
			return -1;
		}
	}
	len = sizeof(param);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
	    &param, &len);
	if (result < 0) {
		return (result);
	}
	if (hbinterval) {
		*hbinterval = param.spp_hbinterval;
	}
	if (maxrxt) {
		*maxrxt = param.spp_pathmaxrxt;
	}
	if (pathmtu) {
		*pathmtu  = param.spp_pathmtu;
	}
	if (flags) {
		*flags = param.spp_flags;
	}
	if (ipv6_flowlabel) {
		*ipv6_flowlabel = param.spp_ipv6_flowlabel;
	}
	if (ipv4_tos) {
		*ipv4_tos = param.spp_ipv4_tos;
	}

	return (result);
}

static int
sctp_set_paddr_param(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint32_t hbinterval,
    uint16_t maxrxt,
    uint32_t pathmtu,
    uint32_t flags,
    uint32_t ipv6_flowlabel,
    uint8_t ipv4_tos)
{
	struct sctp_paddrparams param;
	socklen_t len;
	int result;
	
	memset(&param, 0, sizeof(param));
	param.spp_assoc_id = id;
	if(sa) {
		if (sa->sa_family == AF_INET) {
			memcpy(&param.spp_address, sa,
			    sizeof(struct sockaddr_in));
		} else if (sa->sa_family == AF_INET6) {
			memcpy(&param.spp_address, sa,
			    sizeof(struct sockaddr_in6));
		} else {
			errno = EINVAL;
			return -1;
		}
	}
	param.spp_hbinterval = hbinterval;
	param.spp_pathmaxrxt = maxrxt;
	param.spp_pathmtu = pathmtu;
	param.spp_flags = flags;
	param.spp_ipv6_flowlabel = ipv6_flowlabel;
	param.spp_ipv4_tos = ipv4_tos;
	len = sizeof(param);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS,
	    &param, len);

	return(result);
}

static int
sctp_set_hbint(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint16_t hbinterval)
{
	int result;
	uint32_t flags;

	flags = SPP_HB_ENABLE;
	result  = sctp_set_paddr_param(fd, id, sa,
	    hbinterval,
	    0,
	    0,
	    flags,
	    0,
	    0);

	return result;	
}

static int
sctp_set_hbzero(int fd, sctp_assoc_t id, 
    struct sockaddr *sa)
{
	int result;
	uint32_t flags;
	
	flags = SPP_HB_ENABLE | SPP_HB_TIME_IS_ZERO;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    0,
	    0,
	    flags,
	    0,
	    0);

	return result;
}

static int
sctp_set_hbdisable(int fd, sctp_assoc_t id, 
    struct sockaddr *sa)
{
	int result;
	uint32_t flags;

	flags = SPP_HB_DISABLE;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    0,
	    0,
	    flags,
	    0,
	    0);

	return result;
}

static int
sctp_set_hbenable(int fd, sctp_assoc_t id, 
    struct sockaddr *sa)
{
	int result;
	uint32_t flags;

	flags = SPP_HB_ENABLE;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    0,
	    0,
	    flags,
	    0,
	    0);

	return result;
}

static int
sctp_set_maxrxt(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint32_t maxrxt)
{
	int result;
	uint32_t flags;
	
	flags = 0;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    maxrxt,
	    0,
	    flags,
	    0,
	    0);

	return (result);
}

static int
sctp_set_flow(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint32_t ipv6_flowlabel)
{
	int result;
	uint32_t flags;
	
	flags = SPP_IPV6_FLOWLABEL;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    0,
	    0,
	    flags,
	    ipv6_flowlabel,
	    0);

	return (result);
}

static int
sctp_set_tos(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint8_t ipv4_tos)
{
	int result;
	uint32_t flags;
	flags = SPP_IPV4_TOS;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0, 0, 0, flags, 0, ipv4_tos);
	return (result);
}

static int
sctp_set_pmtu(int fd, sctp_assoc_t id, 
    struct sockaddr *sa,
    uint32_t pathmtu)
{
	int result;
	uint32_t flags;
	flags = SPP_PMTUD_DISABLE;
	result  = sctp_set_paddr_param(fd, id, sa,
	    0,
	    0,
	    pathmtu,
	    flags,
	    0,
	    0);

	return (result);	
}

static int
sctp_set_disfrag( int fd, int val)
{
	socklen_t len;
	int result;
	
	len = sizeof(val);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS,
	    &val, len);

	return(result);
}

static int
sctp_set_primary(int fd, sctp_assoc_t id, struct sockaddr *sa)
{
	struct sctp_setprim prim;
	socklen_t len;
	int result;

	len = sizeof(prim);
	prim.ssp_assoc_id = id;
	if (sa->sa_family == AF_INET) {
		memcpy(&prim.ssp_addr, sa, sizeof(struct sockaddr_in));
	} else if (sa->sa_family == AF_INET6) {
		memcpy(&prim.ssp_addr, sa, sizeof(struct sockaddr_in6));
	} else {
		errno = EINVAL;
		return -1;
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_PRIMARY_ADDR, 
			    &prim, len);
	
	return(result);
}

static int 
sctp_get_primary(int fd, sctp_assoc_t id, struct sockaddr *sa, socklen_t *alen)
{
	struct sctp_setprim prim;
	socklen_t len, clen;
	int result;
	struct sockaddr *lsa;

	len = sizeof(prim);
	memset(&prim, 0, sizeof(prim));
	prim.ssp_assoc_id = id;
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_PRIMARY_ADDR, 
			    &prim, &len);
	lsa = (struct sockaddr *)&prim.ssp_addr;
	if(lsa->sa_family == AF_INET)
		clen = sizeof(struct sockaddr_in);
	else if (lsa->sa_family == AF_INET6)
		clen = sizeof(struct sockaddr_in6);
	else {
		errno = EFAULT;
		return -1;
	}
	if(*alen > clen) 
		len = clen;
	else
		len = *alen;

	memcpy(sa, lsa, len);
	*alen = clen;

	return(result);
}

static int
sctp_set_peer_prim(int fd, sctp_assoc_t id,  struct sockaddr *sa)
{

	struct sctp_setpeerprim prim;	
	int result;
	socklen_t len;

	if(sa == NULL) {
		errno = EINVAL;
		return (-1);
	}
	if(sa->sa_family == AF_INET) {
		memcpy(&prim.sspp_addr, sa, sizeof(struct sockaddr_in));
	}else if (sa->sa_family == AF_INET6) {
		memcpy(&prim.sspp_addr, sa, sizeof(struct sockaddr_in6));
	} else {
		errno = EINVAL;
		return (-1);
	}
	prim.sspp_assoc_id = id;
	len = sizeof(prim);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_SET_PEER_PRIMARY_ADDR,
			    &prim, len);
	return (result);
}

static int
sctp_set_autoclose(int fd, uint32_t val)
{
	int result;
	socklen_t len;

	len = sizeof(val);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, 
	    &val, len);

	return(result);
}

static int
sctp_get_autoclose(int fd, uint32_t *val)
{
	int result;
	socklen_t len;

	len = sizeof(*val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, 
	    val, &len);

	return (result);
}

static int
sctp_get_ndelay(int fd, uint32_t *val)
{
	int result;
	socklen_t len;

	len = sizeof(*val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, 
			    val, &len);

	return (result);
}

static int
sctp_set_ndelay(int fd, uint32_t val)
{
	int result;
	socklen_t len;

	len = sizeof(val);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, 
	    &val, len);

	return(result);
}

static int 
sctp_get_adaptation( int fd, uint32_t *val)
{
	struct sctp_setadaptation adapt;
	socklen_t len;
	int result;

	len = sizeof(adapt);
	memset(&adapt, 0, sizeof(adapt));

	result = getsockopt(fd, IPPROTO_SCTP, SCTP_ADAPTATION_LAYER,
	    &adapt, &len);
	*val = adapt.ssb_adaptation_ind;

	return(result);
}

static int
sctp_set_adaptation( int fd, uint32_t val)
{
	struct sctp_setadaptation adapt;
	socklen_t len;
	int result;

	len = sizeof(adapt);
	memset(&adapt, 0, sizeof(adapt));
	adapt.ssb_adaptation_ind = val;

	result = setsockopt(fd, IPPROTO_SCTP, SCTP_ADAPTATION_LAYER,
	    &adapt, len);

	return(result);
}

static int
sctp_get_disfrag( int fd, int *val)
{
	socklen_t len;
	int result;
	
	len = sizeof(*val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS,
	    val, &len);

	return(result);
}

static int
sctp_set_dsack(int fd, sctp_assoc_t id, uint32_t delay, uint32_t freq)
{
	int result;
	socklen_t len;
	struct sctp_sack_info sack;
	
	len = sizeof(sack);
	sack.sack_assoc_id = id;
	sack.sack_delay = delay;
	sack.sack_freq = freq;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_DELAYED_SACK,
	    &sack, len);

	return(result);
}

static int
sctp_set_ddelay(int fd, sctp_assoc_t id, uint32_t delay)
{
	return (sctp_set_dsack(fd, id, delay, 0));
}

static int
sctp_set_dfreq(int fd, sctp_assoc_t id, uint32_t freq)
{
	return (sctp_set_dsack(fd, id, 0, freq));
}

static int
sctp_get_dsack(int fd, sctp_assoc_t id,uint32_t *delay, uint32_t *freq)
{
	int result;
	socklen_t len;
	struct sctp_sack_info sack;
	memset(&sack, 0, sizeof(sack));
	sack.sack_assoc_id = id;
	len = sizeof(sack);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_DELAYED_SACK,
	    &sack, &len);
	if (delay) {
		*delay = sack.sack_delay;
	}
	if (freq) {
		*freq = sack.sack_freq;
	}

	return(result);
}

static int
sctp_get_interleave(int fd, int *inter)
{
	int result;
	socklen_t len;
	
	len = sizeof(*inter);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE,
	    inter, &len);

	return(result);
}

static int
sctp_get_delete_key(int fd, sctp_assoc_t assoc_id, uint16_t *keyid)
{
	socklen_t len;
	struct sctp_authkeyid akey;
	int result;
	
	len = sizeof(akey);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_DELETE_KEY,
	    &akey, &len);
	if (result >= 0) {
		/* This should always fail */
		*keyid = akey.scact_keynumber;
	}
	
	return (result);
}

static int
sctp_set_delete_key(int fd, sctp_assoc_t assoc_id, uint16_t keyid)
{
	socklen_t len;
	struct sctp_authkeyid akey;
	int result;
	
	len = sizeof(akey);
	akey.scact_assoc_id = assoc_id;
	akey.scact_keynumber = keyid;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_DELETE_KEY,
	    &akey, len);
	return (result);
}

static int
sctp_set_interleave(int fd, int inter)
{
	int result;
	socklen_t len;
	
	len = sizeof(inter);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE,
	    &inter, len);

	return(result);
}

static int
sctp_get_auth_chunk_id(int fd, uint8_t *fill)
{
	int result;
	socklen_t len;
	struct sctp_authchunk ch;
	
	len = sizeof(ch);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_CHUNK,
	    &ch, &len);
	if(result >= 0) {
		/* We really expect this to ALWAYS fail */
		*fill = ch.sauth_chunk;
	}

	return (result);
}

static int
sctp_set_auth_chunk_id(int fd, uint8_t chk)
{
	int result;
	socklen_t len;
	struct sctp_authchunk ch;
	
	len = sizeof(ch);
	ch.sauth_chunk = chk;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_CHUNK,
	    &ch, len);

	return (result);	
}

static int
sctp_v4_address_mapping_enabled(int fd)
{
	int onoff;
	socklen_t length;
	
	length = (socklen_t)sizeof(int);
	(void)getsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
	    &onoff, &length);

	return (onoff);
}

static int
sctp_set_events(int fd, struct sctp_event_subscribe *ev)
{
	socklen_t len;
	int result;
	len = sizeof(*ev);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS,
	    ev, len);

	return (result);
}

static int
sctp_get_events(int fd, struct sctp_event_subscribe *ev)
{
	socklen_t len;
	int result;
	len = sizeof(*ev);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS,
	    ev, &len);

	return (result);
}

static int
sctp_get_defsend(int fd, sctp_assoc_t id, struct sctp_sndrcvinfo *s)
{
	socklen_t len;
	int result;
	s->sinfo_assoc_id = id;
	
	len = sizeof(*s);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_DEFAULT_SEND_PARAM,
	    s, &len);
	
	return (result);
}

static int
sctp_set_defsend(int fd, sctp_assoc_t id, struct sctp_sndrcvinfo *s)
{
	socklen_t len;
	int result;
	s->sinfo_assoc_id = id;
	
	len = sizeof(*s);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_DEFAULT_SEND_PARAM,
	    s, len);

	return (result);
}

static int
sctp_get_auth_key(int fd, sctp_assoc_t assoc_id, uint16_t *keyid,
    uint16_t *keylen, uint8_t *keytext)
{
	socklen_t len;
	struct sctp_authkey *akey;
	int result;
	
	len = sizeof(akey) + *keylen;
	akey = (struct sctp_authkey *)alloca(len);
	if (akey == NULL) {
		printf("could not get memory for akey\n");
		return (-1);
	}
	akey->sca_assoc_id = assoc_id;
	akey->sca_keynumber = *keyid;
	bcopy(keytext, akey->sca_key, *keylen);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_KEY, akey, &len);
	if (result >= 0) {
		/* This should always fail */
		*keyid = akey->sca_keynumber;
		*keylen = len - sizeof(*akey);
		bcopy(akey->sca_key, keytext, *keylen);
	}
	
	return (result);
}

static int sctp_set_auth_key(int fd, sctp_assoc_t assoc_id, uint16_t keyid,
    uint16_t keylen, uint8_t *keytext)
{
	socklen_t len;
	struct sctp_authkey *akey;
	int result;
	
	len = sizeof(*akey) + keylen;
	akey = (struct sctp_authkey *)alloca(len);
	if (akey == NULL) {
		printf("could not get memory for akey\n");
		return (-1);
	}
	akey->sca_assoc_id = assoc_id;
	akey->sca_keynumber = keyid;
	bcopy(keytext, akey->sca_key, keylen);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_KEY, akey, len);
	
	return (result);
}

static int
sctp_get_active_key(int fd, sctp_assoc_t assoc_id, uint16_t *keyid)
{
	socklen_t len;
	struct sctp_authkeyid akey;
	int result;
	
	len = sizeof(akey);
	akey.scact_assoc_id = assoc_id;
	akey.scact_keynumber = *keyid;
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_ACTIVE_KEY,
	    &akey, &len);
	if (result >= 0) {
		*keyid = akey.scact_keynumber;
	}
	return (result);
}

static int
sctp_set_active_key(int fd, sctp_assoc_t assoc_id, uint16_t keyid)
{
	socklen_t len;
	struct sctp_authkeyid akey;
	int result;
	
	len = sizeof(akey);
	akey.scact_assoc_id = assoc_id;
	akey.scact_keynumber = keyid;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTH_ACTIVE_KEY,
	    &akey, len);
	return (result);
}

static int
sctp_get_pdapi_point(int fd, int *point)
{
	int result;
	socklen_t len;
	
	len = sizeof(*point);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_PARTIAL_DELIVERY_POINT,
	    point, &len);

	return(result);
}

static int
sctp_set_pdapi_point(int fd, int point)
{
	int result;
	socklen_t len;
	
	len = sizeof(point);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_PARTIAL_DELIVERY_POINT,
	    &point, len);

	return(result);
}

static int
sctp_set_context(int fd, sctp_assoc_t id, uint32_t context)
{
	int result;
	socklen_t len;
	struct sctp_assoc_value av;
	
	len = sizeof(av);
	av.assoc_id = id;
	av.assoc_value = context;
	
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_CONTEXT,
	    &av, len);
	return(result);
	
}

static int
sctp_get_context(int fd, sctp_assoc_t id, uint32_t *context)
{
	int result;
	socklen_t len;
	struct sctp_assoc_value av;
	
	len = sizeof(av);
	av.assoc_id = id;
	av.assoc_value = 0;
	
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_CONTEXT,
	    &av, &len);
	*context = av.assoc_value;

	return(result);
}

static int
sctp_set_rto_info(int fd, sctp_assoc_t assoc_id, uint32_t init,
    uint32_t max, uint32_t min)
{
	struct sctp_rtoinfo rtoinfo;
	socklen_t len;
	
	len = (socklen_t)sizeof(struct sctp_rtoinfo);
	bzero((void *)&rtoinfo, sizeof(struct sctp_rtoinfo));
	
	rtoinfo.srto_assoc_id = assoc_id;
	rtoinfo.srto_initial  = init;
	rtoinfo.srto_max      = max;
	rtoinfo.srto_min      = min;

	return setsockopt(fd, IPPROTO_SCTP, SCTP_RTOINFO,
	    (const void *)&rtoinfo, len);
}

static int
sctp_set_initial_rto(int fd, sctp_assoc_t assoc_id, uint32_t init)
{
	return sctp_set_rto_info(fd, assoc_id, init, 0, 0);
}

static int
sctp_set_maximum_rto(int fd, sctp_assoc_t assoc_id, uint32_t max)
{
	return sctp_set_rto_info(fd, assoc_id, 0, max, 0);
}

static int
sctp_set_minimum_rto(int fd, sctp_assoc_t assoc_id, uint32_t min)
{
	return sctp_set_rto_info(fd, assoc_id, 0, 0, min);
}

static int
sctp_get_rto_info(int fd, sctp_assoc_t assoc_id, uint32_t *init,
    uint32_t *max, uint32_t *min)
{
	struct sctp_rtoinfo rtoinfo;
	socklen_t len;
	int result;
	
	len = (socklen_t)sizeof(struct sctp_rtoinfo);
	bzero((void *)&rtoinfo, sizeof(struct sctp_rtoinfo));
	rtoinfo.srto_assoc_id = assoc_id;
	
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_RTOINFO,
	    (void *)&rtoinfo, &len);	

	if (init)
		*init = rtoinfo.srto_initial;
	if (max)
		*max = rtoinfo.srto_max;
	if (min)
		*min = rtoinfo.srto_min;

	return result;
}

static int
sctp_get_initial_rto(int fd, sctp_assoc_t assoc_id, uint32_t *init)
{
	return sctp_get_rto_info(fd, assoc_id, init, NULL, NULL);
}

static int
sctp_get_minimum_rto(int fd, sctp_assoc_t assoc_id, uint32_t *min)
{
	return sctp_get_rto_info(fd, assoc_id, NULL, NULL, min);
}

static int
sctp_get_maximum_rto(int fd, sctp_assoc_t assoc_id, uint32_t *max)
{
	return sctp_get_rto_info(fd, assoc_id, NULL, max, NULL);
}

static int 
sctp_one2one(unsigned short port, int should_listen, int bindall)
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
	if (bindall) {
		addr.sin_addr.s_addr = 0;
	} else {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

	if (bind(fd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
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

static int
sctp_socketpair(int *fds, int bindall)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;
	

	/* Get any old port, but listen */
	fd = sctp_one2one(0, 1, bindall);
	if (fd  < 0) {
		return -1;
	}

	/* Get any old port, but no listen */
	fds[0] = sctp_one2one(0, 0, bindall);
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
	if (bindall) {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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

static uint32_t
sctp_get_number_of_associations(int fd)
{
	uint32_t number;
	socklen_t len;
	
	len = (socklen_t) sizeof(uint32_t);
	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_NUMBER,
		(void *)&number, &len) < 0)
		return -1;
	else
		return number;
}

int
sctp_socketstar(int *fd, int *fds, unsigned int n)
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

	if (bind(*fd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
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
		if ((fds[i] = socket(AF_INET, SOCK_SEQPACKET,
			    IPPROTO_SCTP)) < 0) {
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

static int
sctp_get_maxseg(int fd, sctp_assoc_t id, int *val)
{
	socklen_t len;
	struct sctp_assoc_value av;
	int result;
	
	av.assoc_id = id;
	av.assoc_value = 0;
	
	len = sizeof(av);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAXSEG,
	    &av, &len);
	*val = av.assoc_value;
	return(result);
	
}

static int
sctp_set_maxseg(int fd, sctp_assoc_t id, int val)
{
	socklen_t len;
	int result;
	struct sctp_assoc_value av;
	len = sizeof(av);
	av.assoc_id = id;
	av.assoc_value = val;
	
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_MAXSEG,
	    &av, len);
	return(result);
}

static int
sctp_socketpair_reuse(int fd, int *fds, int bindall)
{
	struct sockaddr_in addr;
	socklen_t addr_len;

	/* Get any old port, but no listen */
	fds[0] = sctp_one2one(0, 0, bindall);
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
	if (bindall) {
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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

/*
 * TEST-TITLE rtoinfo/gso_1_1_defaults
 * TEST-DESCR: will open a 1-1 socket, get the endpoint
 * TEST-DESCR: rto info and validate it conforms
 * TEST-DESCR: to recommended default values
 * TEST-DESCR: found in rfc4960.
 */
static void
test_rtoinfo_gso_1_1_defaults(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != 3000) || (min != 1000) || (max != 60000))
		errx(-1, "Default not RFC 4960 compliant");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_M_defaults
 * TEST-DESCR: will open a 1-M socket, get the endpoint
 * TEST-DESCR: rto info and validate it conforms
 * TEST-DESCR: to recommended default values
 * TEST-DESCR: found in rfc4960.
 */
static void
test_rtoinfo_gso_1_M_defaults(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != 3000) || (min != 1000) || (max != 60000))
		errx(-1, "Default not RFC 4960 compliant");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_1_badid
 * TEST-DESCR: will open a 1-1 socket, and attempt to
 * TEST-DESCR: get association level rto information  using a bad 
 * TEST-DESCR: association id. It expects the call to
 * TEST-DESCR: fail.
 */
static void
test_rtoinfo_gso_1_1_bad_id(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
	
	result = sctp_get_rto_info(fd, 1, NULL, NULL, NULL);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_M_badid
 * TEST-DESCR: will open a 1-M socket, and attempt to
 * TEST-DESCR: association level get rto information  using a bad 
 * TEST-DESCR: association id. It expects the call to
 * TEST-DESCR: fail.
 */
static void
test_rtoinfo_gso_1_M_bad_id(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));
	
	result = sctp_get_rto_info(fd, 1, NULL, NULL, NULL);
	
	close(fd);
	
	if (!result)
		errx(-1, "getsockopt was successful");

	if (errno != ENOENT)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_1_good
 * TEST-DESCR: will open a 1-1 socket, get the rto info
 * TEST-DESCR: double it, and attempt to set the doubled
 * TEST-DESCR: information back on the ep. It expects
 * TEST-DESCR: the call to succeed
 */
static void
test_rtoinfo_sso_1_1_good(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(fd, 0, init, max, min);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != new_init) || (min != new_min) || (max != new_max))
		errx(-1, "Values could not be set correctly");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_M_good
 * TEST-DESCR: will open a 1-M socket, get the rto info
 * TEST-DESCR: double it, and attempt to set the doubled
 * TEST-DESCR: information back on the ep. It expects
 * TEST-DESCR: the call to succeed
 */
static void
test_rtoinfo_sso_1_M_good(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(fd, 0, init, max, min);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != new_init) || (min != new_min) || (max != new_max))
		errx(-1, "Values could not be set correctly");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_1_bad_id
 * TEST-DESCR: will open a 1-1 socket, set rto information
 * TEST-DESCR: using a bad association id. It expects the
 * TEST-DESCR: call to fail.
 */
static void
test_rtoinfo_sso_1_1_bad_id(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_set_rto_info(fd, 1, init, max, min);
	close(fd);

	if (result) {
		errx(-1, "%s", strerror(errno));
	}
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_M_bad_id
 * TEST-DESCR: will open a 1-M socket, set rto information
 * TEST-DESCR: using a bad association id. It expects the
 * TEST-DESCR: call to fail.
 */
static void
test_rtoinfo_sso_1_M_bad_id(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_set_rto_info(fd, 1, init, max, min);
	close(fd);

	if (!result) {
		errx(-1, "setsockopt was successful");
	}

	if (errno != ENOENT)
		errx(-1, "%s", strerror(errno));

	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_init
 * TEST-DESCR: opens a 1-1 socket, gets the 
 * TEST-DESCR: current values on an endpoint adds 10
 * TEST-DESCR: to initial value but leaves max/min 
 * TEST-DESCR: unchanged (0 values). Then it validates
 * TEST-DESCR: that the initial value changed but NOT
 * TEST-DESCR: max and min.
 */
static void
test_rtoinfo_sso_1_1_init(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	init  += 10;
	
	result = sctp_set_initial_rto(fd, 0, init);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (init != new_init)
		errx(-1, "Value could not be set correctly");
		
	if ((max != new_max) || (min != new_min))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_M_init
 * TEST-DESCR: opens a 1-1 socket, gets the 
 * TEST-DESCR: current values on an endpoint adds 10
 * TEST-DESCR: to initial value but leaves max/min 
 * TEST-DESCR: unchanged (0 values). Then it validates
 * TEST-DESCR: that the initial value changed but NOT
 * TEST-DESCR: max and min.
 */
static void
test_rtoinfo_sso_1_M_init(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	init  += 10;
	
	result = sctp_set_initial_rto(fd, 0, init);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (init != new_init)
		errx(-1, "Value could not be set correctly");
		
	if ((max != new_max) || (min != new_min))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_max
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can set only the max value and other
 * TEST-DESCR: values are not effected.
 */
static void
test_rtoinfo_sso_1_1_max(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	max  += 10;
	
	result = sctp_set_maximum_rto(fd, 0, max);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (max != new_max)
		errx(-1, "Value could not be set correctly");
		
	if ((init != new_init) || (min != new_min))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_M_max
 * TEST-DESCR: Validates on a 1-M model socket that
 * TEST-DESCR: you can set only the max value and other
 * TEST-DESCR: values are not effected.
 */
static void
test_rtoinfo_sso_1_M_max(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	max  += 10;
	
	result = sctp_set_maximum_rto(fd, 0, max);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (max != new_max)
		errx(-1, "Value could not be set correctly");
		
	if ((init != new_init) || (min != new_min))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_min
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can set only the min value and other
 * TEST-DESCR: values are not effected.
 */
static void
test_rtoinfo_sso_1_1_min(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	min  += 10;
	
	result = sctp_set_minimum_rto(fd, 0, min);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (min != new_min)
		errx(-1, "Value could not be set correctly");
		
	if ((init != new_init) || (max != new_max))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_M_min
 * TEST-DESCR: Validates on a 1-M model socket that
 * TEST-DESCR: you can set only the min value and other
 * TEST-DESCR: values are not effected.
 */
static void
test_rtoinfo_sso_1_M_min(void)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	min  += 10;
	
	result = sctp_set_minimum_rto(fd, 0, min);

	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	if (min != new_min)
		errx(-1, "Value could not be set correctly");
		
	if ((init != new_init) || (max != new_max))
		errx(-1, "Values have been changed");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_same
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can set all values to the same setting
 * TEST-DESCR: (100).
 */
static void
test_rtoinfo_sso_1_1_same(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_set_rto_info(fd, 0, 100, 100, 100);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

		
	if ((init != 100) || (max != 100) || (min != 100))
		errx(-1, "Values could not be set correclty");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_M_same
 * TEST-DESCR: Validates on a 1-M model socket that
 * TEST-DESCR: you can set all values to the same setting
 * TEST-DESCR: (100).
 */
static void
test_rtoinfo_sso_1_M_same(void)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_set_rto_info(fd, 0, 100, 100, 100);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		errx(-1, "%s", strerror(errno));

		
	if ((init != 100) || (max != 100) || (min != 100))
		errx(-1, "Values could not be set correclty");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_1
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the initial RTO less than
 * TEST-DESCR: the minimum. This should fail.
 */
static void
test_rtoinfo_sso_ill_1(void)
{
	int fd, result;
	uint32_t min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_minimum_rto(fd, 0, &min);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_initial_rto(fd, 0, min - 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.init smaller than RTO.min");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_2
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the initial RTO greater than
 * TEST-DESCR: the maximum. This should fail.
 */
static void
test_rtoinfo_sso_ill_2(void)
{
	int fd, result;
	uint32_t max;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_maximum_rto(fd, 0, &max);
	
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_initial_rto(fd, 0, max + 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.init greater than RTO.max");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_3
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the minimum RTO greater than
 * TEST-DESCR: the initial RTO. This should fail.
 */
static void
test_rtoinfo_sso_ill_3(void)
{
	int fd, result;
	uint32_t init;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_initial_rto(fd, 0, &init);
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_minimum_rto(fd, 0, init + 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.min greater than RTO.init");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_4
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the minimum RTO greater than
 * TEST-DESCR: the maximum RTO. This should fail.
 */
static void
test_rtoinfo_sso_ill_4(void)
{
	int fd, result;
	uint32_t max;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_maximum_rto(fd, 0, &max);
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_minimum_rto(fd, 0, max + 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.min greater than RTO.max");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_5
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the maximum RTO smaller than
 * TEST-DESCR: the initial RTO. This should fail.
 */
static void
test_rtoinfo_sso_ill_5(void)
{
	int fd, result;
	uint32_t init;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_initial_rto(fd, 0, &init);
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_maximum_rto(fd, 0, init - 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.max smaller than RTO.init");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_ill_6
 * TEST-DESCR: On a 1-1 socket we attempt to
 * TEST-DESCR: set the maximum RTO smaller than
 * TEST-DESCR: the minimum RTO. This should fail.
 */
static void
test_rtoinfo_sso_ill_6(void)
{
	int fd, result;
	uint32_t min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_minimum_rto(fd, 0, &min);
	if (result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
		
	result = sctp_set_maximum_rto(fd, 0, min - 10);

	close(fd);
	
	if (!result)
		errx(-1, "Can set RTO.max smaller than RTO.min");
	
	if (errno != EDOM)
		errx(-1, "%s", strerror(errno));
	
	return;
}

/*
 * TEST-TITLE rtoinfo/gso_1_1_c_bad_id
 * TEST-DESCR: On a 1-1 socket we create an association
 * TEST-DESCR: and attempt to get information using one
 * TEST-DESCR: of the file descriptors using a random (1)
 * TEST-DESCR: value in the association id field. We expect
 * TEST-DESCR: this to succeed since the 1-1 model does
 * TEST-DESCR: not need an association id and should ignore it.
 */
static void
test_rtoinfo_gso_1_1_c_bad_id(void)
{
	int fd[2], result;

	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "%s", strerror(errno));
		
	result = sctp_get_rto_info(fd[0], 1, NULL, NULL, NULL);
	
	close(fd[0]);
	close(fd[1]);
	
	if (result)
		errx(-1, "%s", strerror(errno));

	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_c_bad_id
 * TEST-DESCR: On a 1-1 socket we create an association
 * TEST-DESCR: and attempt to set information using one
 * TEST-DESCR: of the file descriptors using a random (1)
 * TEST-DESCR: value in the association id field. We expect
 * TEST-DESCR: this to succeed since the 1-1 model does
 * TEST-DESCR: not need an association id and should ignore it.
 */
static void
test_rtoinfo_sso_1_1_c_bad_id(void)
{
	int fd[2], result;
	uint32_t init, max, min;
	
	if (sctp_socketpair(fd, 0) < 0)
		errx(-1, "%s", strerror(errno));

	result = sctp_get_rto_info(fd[0], 0, &init, &max, &min);
	
	if (result) {
		close(fd[0]);
		close(fd[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_set_rto_info(fd[0], 1, init, max, min);
	close(fd[0]);
	close(fd[1]);

	if (result) {
		errx(-1, "%s", strerror(errno));
	}
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_1_inherit
 * TEST-DESCR: We open a 1-1 socket, change the
 * TEST-DESCR: default values of the endpoint. We
 * TEST-DESCR: then create an association by having
 * TEST-DESCR: our endpoint listen and a seperate
 * TEST-DESCR: 1-1 socket connect to us. We then validate
 * TEST-DESCR: that the set values on the endpoint inherited
 * TEST-DESCR: to the association correctly.
 */
static void
test_rtoinfo_sso_1_1_inherit(void)
{
	int cfd, afd, lfd, result;
	struct sockaddr_in addr;
	socklen_t addr_len;
	uint32_t init, min, max, new_init, new_max, new_min;

	if ((lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		errx(-1, "%s", strerror(errno));

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(lfd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}

	if (listen(lfd, 1) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_rto_info(lfd, 0, &init, &max, &min);
	
	if (result) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(lfd, 0, init, max, min);

	if (result) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}
	
	if ((cfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(cfd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (lfd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	if (connect(cfd, (struct sockaddr *) &addr, addr_len) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	if ((afd = accept(lfd, NULL, 0)) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}
	close(lfd);

	result = sctp_get_rto_info(afd, 0, &new_init, &new_max, &new_min);
	
	close(cfd);
	close(afd);

	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != new_init) || (min != new_min) || (max != new_max))
		errx(-1, "Values are not inherited correctly");
	
	return;
}

/*
 * TEST-TITLE rtoinfo/sso_1_M_inherit
 * TEST-DESCR: We open a 1-M socket, change the
 * TEST-DESCR: default values of the endpoint. We
 * TEST-DESCR: then create an association by having
 * TEST-DESCR: our endpoint listen and a seperate
 * TEST-DESCR: 1-M socket connect to us. We then validate
 * TEST-DESCR: that the set values on the endpoint inherited
 * TEST-DESCR: to the association correctly.
 */
static void
test_rtoinfo_sso_1_M_inherit(void)
{
	int cfd, pfd, afd, lfd, result;
	struct sockaddr_in addr;
	socklen_t addr_len;
	uint32_t init, min, max, new_init, new_max, new_min;
	sctp_assoc_t assoc_id;
	
	if ((lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) 
	   	errx(-1, "%s", strerror(errno));

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(lfd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}

	if (listen(lfd, 1) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}
	
	if ((cfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		close(lfd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_rto_info(cfd, 0, &init, &max, &min);
	
	if (result) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(cfd, 0, init, max, min);

	if (result) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(cfd, (struct sockaddr *)&addr,
		(socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (lfd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	if (sctp_connectx(cfd, (struct sockaddr *) &addr, 1, &assoc_id) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}

	if ((afd = accept(lfd, NULL, 0)) < 0) {
		close(lfd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}
	close(lfd);

	if ((pfd = sctp_peeloff(cfd, assoc_id)) < 0) {
		close(afd);
		close(cfd);
		errx(-1, "%s", strerror(errno));
	}
	close(cfd);

	result = sctp_get_rto_info(pfd, 0, &new_init, &new_max, &new_min);
	
	close(afd);
	close(pfd);

	if (result)
		errx(-1, "%s", strerror(errno));

	if ((init != new_init) || (min != new_min) || (max != new_max))
		errx(-1, "Values are not inherited correctly");

	return;
}

/********************************************************
 *
 * SCTP_ASSOCLIST tests
 *
 ********************************************************/

/*
 * TEST-TITLE assoclist/gso_numbers_zero
 * TEST-DESCR: Open a 1-1 socket and validate that
 * TEST-DESCR: it has no associations.
 */
static void
test_assoclist_gso_numbers_zero(void)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
	    	errx(-1, "%s", strerror(errno));
		
	result = sctp_get_number_of_associations(fd);
	
	close(fd);

	if (result == 0)
		return;
	else
		errx(-1, "Wrong number of associations");
}

#define NUMBER_OF_ASSOCS 12

/*
 * TEST-TITLE assoclist/gso_numbers_pos
 * TEST-DESCR: Open a 1-M socket, and create
 * TEST-DESCR: a number of associations (using seperate fd's) to
 * TEST-DESCR: it. Validate that the number of associations
 * TEST-DESCR: returned is the number we created.
 */
static void
test_assoclist_gso_numbers_pos(void)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	unsigned int i;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		errx(-1, "%s", strerror(errno));
	
	sctp_delay(SCTP_SLEEP_MS);
	result = sctp_get_number_of_associations(fd);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS)
		return;
	else
		errx(-1, "Wrong number of associations");
}

/*
 * TEST-TITLE assoclist/gso_ids_no_assoc
 * TEST-DESCR: Open a 1-1 socket, and get the 
 * TEST-DESCR: assocation list. Verify that no
 * TEST-DESCR: association id's are returned.
 */
static void
test_assoclist_gso_ids_no_assoc(void)
{
	int fd, result;
	sctp_assoc_t id;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
	    	errx(-1, "%s", strerror(errno));
		
	if (sctp_get_number_of_associations(fd) != 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_association_identifiers(fd, &id, 1);
	close(fd);
	if (result == 0)
		return;
	else
		errx(-1, "Wrong number of identifiers");
}

/*
 * TEST-TITLE assoclist/gso_ids_buf_fit
 * TEST-DESCR: Open a 1-M socket and create a
 * TEST-DESCR: number of assocaitions connected to
 * TEST-DESCR: the 1-M socket. Get the association 
 * TEST-DESCR: identifiers and validate that they are not
 * TEST-DESCR: duplicated.
 */
static void
test_assoclist_gso_ids_buf_fit(void)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i, j;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		errx(-1, "%s", strerror(errno));
	sctp_delay(SCTP_SLEEP_MS);

	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);
		errx(-1, "Wrong number of associations");
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS) {
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			for (j = 0; j < NUMBER_OF_ASSOCS; j++)
				if ((i != j) && (ids[i] == ids[j]))
					errx(-1, "Same identifier for different associations");
		return;
	} else
		errx(-1, "Wrong number of identifiers");
}

/*
 * TEST-TITLE assoclist/gso_ids_buf_large
 * TEST-DESCR: Create a number of associations connected
 * TEST-DESCR: to our 1-M socket. Get the number of 
 * TEST-DESCR: assocations passing in a larger buffer
 * TEST-DESCR: then needed i.e. 1 extra id. Then validate
 * TEST-DESCR: that no duplicate association id is given.
 */
static void
test_assoclist_gso_ids_buf_large(void)
{
	int fd, fds[NUMBER_OF_ASSOCS + 1], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i, j;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		errx(-1, "%s", strerror(errno));
	sctp_delay(SCTP_SLEEP_MS);
	
	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);	
		errx(-1, "Wrong number of associations");
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS + 1);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS) {
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			for (j = 0; j < NUMBER_OF_ASSOCS; j++)
				if ((i != j) && (ids[i] == ids[j]))
					errx(-1, "Same identifier for different associations");
		return;
	} else
		errx(-1, "Wrong number of identifiers");
}

/*
 * TEST-TITLE assoclist/gso_ids_buf_small
 * TEST-DESCR: Create a number of associations
 * TEST-DESCR: on a 1-M socket, then request the
 * TEST-DESCR: association id's but give too small
 * TEST-DESCR: of a list. Validate that we can retrieve
 * TEST-DESCR: the list, even though we do not get all
 * TEST-DESCR: of them.
 */
static void
test_assoclist_gso_ids_buf_small(void)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		errx(-1, "%s", strerror(errno));
	sctp_delay(SCTP_SLEEP_MS);
	
	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS - 1);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result > 0)
		errx(-1, "getsockopt successful");
	else
		return;
}


/********************************************************
 *
 * SCTP_ASSOCINFO tests
 *
 ********************************************************/
static char error_buffer[128];

/*
 * TEST-TITLE associnfo/gso_1_1_defaults
 * TEST-DESCR: Open a 1-1 socket and validate that
 * TEST-DESCR: the default settings conform to RFC4960.
 */
static void
test_associnfo_gso_1_1_defaults(void)
{
	int fd, result;
	uint16_t asoc_maxrxt, peer_dest_cnt;
	uint32_t peer_rwnd, local_rwnd, cookie_life;
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt,
				     &peer_dest_cnt, 
				     &peer_rwnd,
				     &local_rwnd,
				     &cookie_life);
	close(fd);
	if (result)
		errx(-1, "%s", strerror(errno));

	if (asoc_maxrxt != 10) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "max_rxt:%d Not compliant with RFC4960", asoc_maxrxt);
		errx(-1, "%s", error_buffer);
	}
	if (local_rwnd < 4096) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "local_rwnd:%d Not compliant with RFC4960", local_rwnd);
		errx(-1, "%s", error_buffer);
	}
	if (cookie_life != 60000) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "cookie_life:%d Not compliant with RFC4960", cookie_life);
		errx(-1, "%s", error_buffer);
	}
	if(peer_rwnd != 0) {
		errx(-1, "Peer rwnd with no peer?");
	}

	if(peer_dest_cnt != 0) {
		errx(-1, "Peer destination count with no peer?");
	}
	return;
}

/*
 * TEST-TITLE associnfo/gso_1_M_defaults
 * TEST-DESCR: Open a 1-1 socket and validate that
 * TEST-DESCR: the default settings conform to RFC4960.
 */
static void
test_associnfo_gso_1_M_defaults(void)
{
	int fd, result;
	uint16_t asoc_maxrxt, peer_dest_cnt;
	uint32_t peer_rwnd, local_rwnd, cookie_life;
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt,
				     &peer_dest_cnt, 
				     &peer_rwnd,
				     &local_rwnd,
				     &cookie_life);
	close(fd);
	if (result)
		errx(-1, "%s", strerror(errno));

	if (asoc_maxrxt != 10) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "max_rxt:%d Not compliant with RFC4960", asoc_maxrxt);
		errx(-1, "%s", error_buffer);
	}
	if (local_rwnd < 4096) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "local_rwnd:%d Not compliant with RFC4960", local_rwnd);
		errx(-1, "%s", error_buffer);
	}
	if (cookie_life != 60000) {
		snprintf(error_buffer, sizeof(error_buffer),
		    "cookie_life:%d Not compliant with RFC4960", cookie_life);
		errx(-1, "%s", error_buffer);
	}
	if(peer_rwnd != 0) {
		errx(-1, "Peer rwnd with no peer?");
	}

	if(peer_dest_cnt != 0) {
		errx(-1, "Peer destination count with no peer?");
	}
	return;
}

/*
 * TEST-TITLE associnfo/sso_rxt_1_1
 * TEST-DESCR: Open a 1-1 socket and validate that
 * TEST-DESCR: you can set the maximum rxt and no
 * TEST-DESCR: other associnfo value is changed.
 */
static void
test_associnfo_sso_rxt_1_1(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = asoc_maxrxt[0] * 2;
	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}

	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value";
		goto out;
	}
 out:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_rxt_1_M
 * TEST-DESCR: Open a 1-M socket and validate that
 * TEST-DESCR: you can set the maximum rxt and no
 * TEST-DESCR: other associnfo value is changed.
 */
static void
test_associnfo_sso_rxt_1_M(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2many(0, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = asoc_maxrxt[0] * 2;

	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}

	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value";
		goto out;
	}
 out:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_rxt_asoc_1_1
 * TEST-DESCR: Open a 1-1 socket, create an association,
 * TEST-DESCR  using that listener, and validate that
 * TEST-DESCR: you can set the maximum rxt on the association and no
 * TEST-DESCR: other associnfo value are changed on the association.
 */
static void
test_associnfo_sso_rxt_asoc_1_1(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[1], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_rxt_asoc_1_M
 * TEST-DESCR: Open a 1-M socket, create an association,
 * TEST-DESCR  using the endpoint, and validate that
 * TEST-DESCR: you can set the maximum rxt on the association and no
 * TEST-DESCR: other associnfo value is changed on the assocaition.
 */
static void
test_associnfo_sso_rxt_asoc_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;
	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[0], ids[0], newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);
	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_rxt_asoc_1_1_inherit
 * TEST-DESCR: Create a 1-1 socket, change a value
 * TEST-DESCR: (maxrxt) and then create an association
 * TEST-DESCR: using that socket. Validate that the new
 * TEST-DESCR: value was inherited to the association.
 */
static void
test_associnfo_sso_rxt_asoc_1_1_inherit(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out_nopair;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[2] != newval) {
		retstring = "maxrxt did not inherit";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_rxt_asoc_1_M_inherit
 * TEST-DESCR: Create a 1-M socket, change a value
 * TEST-DESCR: (maxrxt) and then create an association
 * TEST-DESCR: using that socket. Validate that the new
 * TEST-DESCR: value was inherited to the association.
 */
static void
test_associnfo_sso_rxt_asoc_1_M_inherit(void)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0, 0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[0], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out_nopair;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[2] != newval) {
		retstring = "maxrxt did not inherit";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_1_1
 * TEST-DESCR: Open a 1-1 socket, double the cookie
 * TEST-DESCR: life and validate it does not change
 * TEST-DESCR: any other values in the associnfo list.
 */
static void
test_associnfo_sso_clife_1_1(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = cookie_life[0] * 2;
	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of cookie-life";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "max rxt changed on set of cookie-life";
		goto out;
	}

	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value";
		goto out;
	}
 out:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_1_M
 * TEST-DESCR: Open a 1-M socket, double the cookie
 * TEST-DESCR: life and validate it does not change
 * TEST-DESCR: any other values in the associnfo list.
 */
static void
test_associnfo_sso_clife_1_M(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2many(0, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = cookie_life[0] * 2;

	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of cookie-life";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed with cookie_life";
		goto out;
	}

	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value";
		goto out;
	}
 out:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_asoc_1_1
 * TEST-DESCR: Open a 1-1 socket, create an association,
 * TEST-DESCR  double the cookie life on the association
 * TEST-DESCR: and validate it does not change
 * TEST-DESCR: any other association values in the associnfo list.
 */
static void
test_associnfo_sso_clife_asoc_1_1(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[1], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of cookie-life";
		goto out;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie-life did not change to correct value"
			" on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of asoc clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwnd ep changed on set of asoc clife";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep during set of asoc clife";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_asoc_1_M
 * TEST-DESCR: Open a 1-M socket, create an association,
 * TEST-DESCR  double the cookie life on the association
 * TEST-DESCR: and validate it does not change
 * TEST-DESCR: any other association values in the associnfo list.
 */
static void
test_associnfo_sso_clife_asoc_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0, 0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_1tom(fds, ids,0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;
	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[0], ids[0], newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change on set asoc clife set";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of asoc clife";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of asoc clife";
		goto out;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value"
			" on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);
	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life on ep changed on set of asoc clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwdn ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep during set of asoc clife";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_asoc_1_1_inherit
 * TEST-DESCR: Open a 1-1 socket, double its cookie life.
 * TEST-DESCR  Use the socket to create an association and
 * TEST-DESCR: validate that the new cookie life inherited to
 * TEST-DESCR: the association.
 */
static void
test_associnfo_sso_clife_asoc_1_1_inherit(void)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of clife";
		goto out_nopair;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt ep changed on set of clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[2] != newval) {
		retstring = "clife did not inherit";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_clife_asoc_1_M_inherit
 * TEST-DESCR: Open a 1-1 socket, double its cookie life.
 * TEST-DESCR  Use the socket to create an association and
 * TEST-DESCR: validate that the new cookie life inherited to
 * TEST-DESCR: the association.
 */
static void
test_associnfo_sso_clife_asoc_1_M_inherit(void)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[0], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of clife";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie_lifet did not change";
		goto out_nopair;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value on ep";
		goto out_nopair;
	}


	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt asoc changed on set of clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd asoc changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[2] != newval) {
		retstring = "cookie_life did not inherit";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_lrwd_ep_1_1
 * TEST-DESCR: Attempt to set the local rwnd using
 * TEST-DESCR: the associnfo structure on a 1-1 socket. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the endpoint.
 */
static void
test_associnfo_sso_lrwnd_ep_1_1(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fd;
	char *retstring=NULL;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fd, 0,  (2*local_rwnd[0]));
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_lrwd_ep_1_M
 * TEST-DESCR: Attempt to set the local rwnd using
 * TEST-DESCR: the associnfo structure on a 1-M socket. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the endpoint.
 */
static void
test_associnfo_sso_lrwnd_ep_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fd;
	char *retstring=NULL;
	fd = sctp_one2many(0, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fd, 0,  (2*local_rwnd[0]));

	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fd);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_lrwd_asoc_1_1
 * TEST-DESCR: Using a 1-1 socket create an assocation.
 * TEST-DESCR: Attempt to set the local rwnd using
 * TEST-DESCR: the associnfo structure on the assocation. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the association.
 */
static void
test_associnfo_sso_lrwnd_asoc_1_1(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fds[0], 0,  (2*local_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_lrwd_asoc_1_M
 * TEST-DESCR: Using a 1-M socket create an assocation.
 * TEST-DESCR: Attempt to set the local rwnd using
 * TEST-DESCR: the associnfo structure on the assocation. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the association.
 */
static void
test_associnfo_sso_lrwnd_asoc_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fds[0], 0,  (2*local_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}



/*
 * TEST-TITLE associnfo/sso_lrwd_asoc_1_1
 * TEST-DESCR: Using a 1-1 socket create an assocation.
 * TEST-DESCR: Attempt to set the peers rwnd using
 * TEST-DESCR: the associnfo structure on the assocation. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the association.
 */
static void
test_associnfo_sso_prwnd_asoc_1_1(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peer_rwnd(fds[0], 0,  (2*peer_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_rwnd[0] != peer_rwnd[1]) {
		retstring = "peers rwnd changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_lrwd_asoc_1_M
 * TEST-DESCR: Using a 1-M socket create an assocation.
 * TEST-DESCR: Attempt to set the peers rwnd using
 * TEST-DESCR: the associnfo structure on the assocation. We validate
 * TEST-DESCR: that the set option did not effect
 * TEST-DESCR: the values on the association.
 */
static void
test_associnfo_sso_prwnd_asoc_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peer_rwnd(fds[0], 0,  (2*peer_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if (peer_rwnd[0] != peer_rwnd[1]) {
		retstring = "peers rwnd changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_pdest_asoc_1_1
 * TEST-DESCR: Create an assocation with the 1-1 model.
 * TEST-DESCR: Then attempt to set the number of peer
 * TEST-DESCR: destinations. Validate that the set
 * TEST-DESCR: had no effect.
 */
static void
test_associnfo_sso_pdest_asoc_1_1(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peerdest_cnt(fds[0], 0,  (2*peer_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_dest_cnt[0] != peer_dest_cnt[1]) {
		retstring = "peers dest count changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE associnfo/sso_pdest_asoc_1_M
 * TEST-DESCR: Create an assocation with the 1-M model.
 * TEST-DESCR: Then attempt to set the number of peer
 * TEST-DESCR: destinations. Validate that the set
 * TEST-DESCR: had no effect.
 */
static void
test_associnfo_sso_pdest_asoc_1_M(void)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids, 0);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peerdest_cnt(fds[0], 0,  (2*peer_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_dest_cnt[0] != peer_dest_cnt[1]) {
		retstring = "peers dest count changed";
	}

 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}


/********************************************************
 *
 * SCTP_initmsg tests
 *
 ********************************************************/
/*
 * TEST-TITLE initmsg/gso_1_1_defaults
 * TEST-DESCR: Create a 1-1 socket, and gather
 * TEST-DESCR: the initmsg defaults. Compare these
 * TEST-DESCR: to those required by RFC4960.
 */
static void
test_initmsg_gso_1_1_defaults(void)
{
	int fd, result;
	uint32_t ostreams, istreams;
	uint16_t max, timeo;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams, &istreams,
				  &max, &timeo);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (max != 8) {
		errx(-1, "Default not RFC 4960 compliant (max_attempts)");
	}
	if (timeo != 60000) {
		errx(-1, "Default not RFC 4960 compliant (max_init_timeo)");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_M_defaults
 * TEST-DESCR: Create a 1-M socket, and gather
 * TEST-DESCR: the initmsg defaults. Compare these
 * TEST-DESCR: to those required by RFC4960.
 */
static void
test_initmsg_gso_1_M_defaults(void)
{
	int fd,result;
	uint32_t ostreams, istreams;
	uint16_t max, timeo;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams, &istreams,
				  &max, &timeo);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (max != 8) {
		errx(-1, "Default not RFC 4960 compliant (max_attempts)");
	}
	if (timeo != 60000) {
		errx(-1, "Default not RFC 4960 compliant (max_init_timeo)");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_1_set_ostrm
 * TEST-DESCR: Create a 1-1 socket, set the
 * TEST-DESCR: ostrm's and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_1_set_ostrm(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * ostreams[0];
	result = sctp_set_im_ostream(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[1] != newval) {
		errx(-1, "failed to set new ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "ostream set changed istream value");
	}
	if (max[0] != max[1]) {
		errx(-1, "ostream set changed max attempts value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "ostream set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_1_set_istrm
 * TEST-DESCR: Create a 1-1 socket, set the
 * TEST-DESCR: istrm's and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_1_set_istrm(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * istreams[0];
	result = sctp_set_im_istream(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "istream set changed ostream value");
	}
	if (newval != istreams[1]) {
		errx(-1, "failed to set new istream value");
	}
	if (max[0] != max[1]) {
		errx(-1, "istream set changed max attempts value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "istream set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_1_set_max
 * TEST-DESCR: Create a 1-1 socket, set the
 * TEST-DESCR: max retrans and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_1_set_max(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxattempt(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "max attempt set changed ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "max attempt set changed max attempts value");
	}
	if (newval != max[1]) {
		errx(-1, "failed to set new max attempt value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "max attempt set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_1_set_max
 * TEST-DESCR: Create a 1-1 socket, set the
 * TEST-DESCR: timeo and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_1_set_timeo(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxtimeo(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "max timeo set changed ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "max timeo set changed max attempts value");
	}
	if (max[0] != max[1]) {
		errx(-1, "max timo set changed max attempts value");
	}
	if (newval != timeo[1]) {
		errx(-1, "failed to set new max timeo value");
	}
}


/*
 * TEST-TITLE initmsg/gso_1_M_set_ostrm
 * TEST-DESCR: Create a 1-M socket, set the
 * TEST-DESCR: ostrm's and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_M_set_ostrm(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * ostreams[0];
	result = sctp_set_im_ostream(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[1] != newval) {
		errx(-1, "failed to set new ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "ostream set changed istream value");
	}
	if (max[0] != max[1]) {
		errx(-1, "ostream set changed max attempts value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "ostream set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_M_set_istrm
 * TEST-DESCR: Create a 1-M socket, set the
 * TEST-DESCR: istrm's and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_M_set_istrm(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * istreams[0];
	result = sctp_set_im_istream(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "istream set changed ostream value");
	}
	if (newval != istreams[1]) {
		errx(-1, "failed to set new istream value");
	}
	if (max[0] != max[1]) {
		errx(-1, "istream set changed max attempts value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "istream set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_M_set_max
 * TEST-DESCR: Create a 1-M socket, set the
 * TEST-DESCR: max retrans and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_M_set_max(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxattempt(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "max attempt set changed ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "max attempt set changed max attempts value");
	}
	if (newval != max[1]) {
		errx(-1, "failed to set new max attempt value");
	}
	if (timeo[0] != timeo[1]) {
		errx(-1, "max attempt set changed max init timeout value");
	}
}

/*
 * TEST-TITLE initmsg/gso_1_M_set_max
 * TEST-DESCR: Create a 1-M socket, set the
 * TEST-DESCR: timeo and validate that no other
 * TEST-DESCR: initmsg parameters change.
 */
static void
test_initmsg_gso_1_M_set_timeo(void)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxtimeo(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		errx(-1, "max timeo set changed ostream value");
	}
	if (istreams[0] != istreams[1]) {
		errx(-1, "max timeo set changed max attempts value");
	}
	if (max[0] != max[1]) {
		errx(-1, "max timo set changed max attempts value");
	}
	if (newval != timeo[1]) {
		errx(-1, "failed to set new max timeo value");
	}
}

/********************************************************
 *
 * SCTP_NODELAY tests
 *
 ********************************************************/
/*
 * TEST-TITLE nodelay/gso_1_1_def_ndelay
 * TEST-DESCR: Create a 1-1 socket, and get
 * TEST-DESCR: the nodelay option. Validate that
 * TEST-DESCR: it is off i.e.the Nagle algorithim is enabled by
 * TEST-DESCR: default.
 */
static void
test_nodelay_gso_1_1_def_ndelay(void)
{
	uint32_t val;
	int fd, result;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val);
	close(fd);
	if(result) {
		errx(-1, "%s", strerror(errno));
	}
	if(val == 1) {
		errx(-1, "non-compliant NO-Delay should default to off");
	}
	return;
}

/*
 * TEST-TITLE nodelay/gso_1_M_def_ndelay
 * TEST-DESCR: Create a 1-M socket, and get
 * TEST-DESCR: the nodelay option. Validate that
 * TEST-DESCR: it is off i.e.the Nagle algorithim is enabled by
 * TEST-DESCR: default.
 */
static void
test_nodelay_gso_1_M_def_ndelay(void)
{
	uint32_t val;
	int fd, result;
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val);
	close(fd);
	if(result) {
		errx(-1, "%s", strerror(errno));
	}
	if(val == 1) {
		errx(-1, "non-compliant NO-Delay should default to off");
	}
	return;
}

/*
 * TEST-TITLE nodelay/gso_1_1_set_ndelay
 * TEST-DESCR: Create a 1-1 socket, and change
 * TEST-DESCR: the nodelay option. Validate that
 * TEST-DESCR: it changes correctly.
 */
static void
test_nodelay_gso_1_1_set_ndelay(void)
{
	uint32_t val[3];
	int fd, result;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[0]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = !val[0];
	result = sctp_set_ndelay(fd, val[1]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[2]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if(val[1] != val[2]) {
		errx(-1, "could not toggle the value");
	}
	return;
}

/*
 * TEST-TITLE nodelay/gso_1_M_set_ndelay
 * TEST-DESCR: Create a 1-M socket, and change
 * TEST-DESCR: the nodelay option. Validate that
 * TEST-DESCR: it changes correctly.
 */
static void
test_nodelay_gso_1_M_set_ndelay(void)
{
	uint32_t val[3];
	int fd, result;
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[0]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = !val[0];
	result = sctp_set_ndelay(fd, val[1]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[2]);
	if(result) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if(val[1] != val[2]) {
		errx(-1, "could not toggle the value");
	}
	return;
}

/********************************************************
 *
 * SCTP_autoclose tests
 *
 ********************************************************/
/*
 * TEST-TITLE autoclose/gso_1_1_def_autoclose
 * TEST-DESCR: Open a 1-1 socket and get the value
 * TEST-DESCR: set on autoclose. Validate it is
 * TEST-DESCR: off by default.
 */
static void
test_autoclose_gso_1_1_def_autoclose(void)
{
	uint32_t aclose;
	int result, fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose);
	close(fd);
	if (result < 0) {
		return;
	}
	if(aclose) {
		errx(-1, "autoclose enabled on 1-2-1?");
	}
	return;
}

/*
 * TEST-TITLE autoclose/gso_1_M_def_autoclose
 * TEST-DESCR: Open a 1-M socket and get the value
 * TEST-DESCR: set on autoclose. Validate it is
 * TEST-DESCR: off by default.
 */
static void
test_autoclose_gso_1_M_def_autoclose(void)
{
	uint32_t aclose;
	int result, fd;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(aclose) {
		errx(-1, "autoclose is enabled by default");
	}
	return;
}

/*
 * TEST-TITLE autoclose/gso_1_1_set_autoclose
 * TEST-DESCR: Open a 1-1 socket and get the value
 * TEST-DESCR: set on autoclose. Toggle it and change
 * TEST-DESCR: its setting. Validate that the setting
 * TEST-DESCR: changes.
 */
static void
test_autoclose_gso_1_1_set_autoclose(void)
{
	uint32_t aclose[3];
	int result, fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[0]);
	if (result < 0) {
		close(fd);
		return;
	}
	aclose[1] = 40;
	result = sctp_set_autoclose(fd, aclose[1]);
	if (result < 0) {
		close(fd);
		return;
	}
	result = sctp_get_autoclose(fd, &aclose[2]);
	close(fd);
	if (result < 0) {
		return;
	}
	if(aclose[1] == aclose[2]) {
		errx(-1, "1-2-1 allowed set of auto close");
	}
	return;
}

/*
 * TEST-TITLE autoclose/gso_1_M_set_autoclose
 * TEST-DESCR: Open a 1-M socket and get the value
 * TEST-DESCR: set on autoclose. Toggle it and change
 * TEST-DESCR: its setting. Validate that the setting
 * TEST-DESCR: changes.
 */
static void
test_autoclose_gso_1_M_set_autoclose(void)
{
	uint32_t aclose[3];
	int result, fd;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[0]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	aclose[1] = 40;
	result = sctp_set_autoclose(fd, aclose[1]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[2]);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(aclose[1] != aclose[2]) {
		errx(-1, "Can't set auto close to new value");
	}
	return;
}


/********************************************************
 *
 * SCTP_SET_PEER_PRIMARY tests
 *
 ********************************************************/
/*
 * TEST-TITLE setpeerprim/sso_1_1_good_peerprim
 * TEST-DESCR: Create an assocaition using a 1-1 socket.
 * TEST-DESCR: Validate that you can get the peers primary address and
 * TEST-DESCR: succesfully request the peer to changes its primary.
 * TEST-DESCR: Note this test requires more than one address
 * TEST-DESCR: on the local host and a working getladdr()/getpaddr()
 * TEST-DESCR: functions. It also depends on free[lp]addr() to work.
 */
static void
test_setpeerprim_sso_1_1_good_peerprim(void)
{
	int fds[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa=NULL;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	num = sctp_getladdrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	result = sctp_set_peer_prim(fds[0], 0,  sa);
	if (result < 0) {
		retstring = strerror(errno);
	}
	sctp_freeladdrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setpeerprim/sso_1_1_bad_peerprim
 * TEST-DESCR: Create an assocaition using a 1-1 socket.
 * TEST-DESCR: Validate that you can get the peers primary address and
 * TEST-DESCR: ask to change the peer primary address to an invalid
 * TEST-DESCR: address on this host. We expect the call to fail.
 * TEST-DESCR: Note this test requires more than one address
 * TEST-DESCR: on the local host and a working getladdr()/getpaddr()
 * TEST-DESCR: functions. It also depends on free[lp]addr() to work.
 */
static void
test_setpeerprim_sso_1_1_bad_peerprim(void)
{
	int fds[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sin;
	struct sockaddr *sa=NULL;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	num = sctp_getladdrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	sin = *((struct sockaddr_in *)sa);
	sctp_freeladdrs(sa);
	sin.sin_addr.s_addr = 0x12345678;
	result = sctp_set_peer_prim(fds[0], 0,  (struct sockaddr *)&sin);
	if (result >= 0) {
		retstring = "set peer primary for bad address allowed";
	}

 out:
	close(fds[0]);
	close(fds[1]);
	
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setpeerprim/sso_1_M_good_peerprim
 * TEST-DESCR: Create an association using a 1-M socket.
 * TEST-DESCR: Validate that you can get the peers primary address and
 * TEST-DESCR: succesfully request the peer to changes its primary.
 * TEST-DESCR: Note this test requires more than one address
 * TEST-DESCR: on the local host and a working getladdr()/getpaddr()
 * TEST-DESCR: functions. It also depends on free[lp]addr() to work.
 */
static void
test_setpeerprim_sso_1_M_good_peerprim(void)
{
	int fds[2];
	sctp_assoc_t ids[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa=NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	result =  sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	num = sctp_getladdrs(fds[0], ids[0], &sa);
	if(num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	cnt = 0;
 try_again:
	result = sctp_set_peer_prim(fds[0], ids[0],  sa);
	if (result < 0) {
		if(cnt < 1) {
			sctp_delay(250);
			cnt++;
			goto try_again;
		}
		retstring = strerror(errno);
	}
	sctp_freeladdrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE setpeerprim/sso_1_M_bad_peerprim
 * TEST-DESCR: Create an assocaition using a 1-M socket.
 * TEST-DESCR: Validate that you can get the peers primary address and
 * TEST-DESCR: ask to change the peer primary address to an invalid
 * TEST-DESCR: address on this host. We expect the call to fail.
 * TEST-DESCR: Note this test requires more than one address
 * TEST-DESCR: on the local host and a working getladdr()/getpaddr()
 * TEST-DESCR: functions. It also depends on free[lp]addr() to work.
 */
static void
test_setpeerprim_sso_1_M_bad_peerprim(void)
{
	int fds[2];
	sctp_assoc_t ids[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sin;
	struct sockaddr *sa=NULL;
	int cnt = 0;

	fds[0] = fds[1] = -1;
	result =  sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getladdrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	sin = *((struct sockaddr_in *)sa);
	sctp_freeladdrs(sa);
	sin.sin_addr.s_addr = 0x12345678;
	result = sctp_set_peer_prim(fds[0], 0,  (struct sockaddr *)&sin);
	if (result >= 0) {
		retstring = "set peer primary for bad address allowed";
	}
 out:
	close(fds[0]);
	close(fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/********************************************************
 *
 * SCTP_PRIMARY tests
 *
 ********************************************************/

/*
 * TEST-TITLE setprim/gso_1_1_get_prim
 * TEST-DESCR: On a 1-1 socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Validate
 * TEST-DESCR: the primary is in the retrieved address list.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_gso_1_1_get_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at;
	union sctp_sockstore store;
	socklen_t len;
	int cnt = 0;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:			
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	sctp_freepaddrs(sa);
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setprim/gso_1_M_get_prim
 * TEST-DESCR: On a 1-M socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Validate
 * TEST-DESCR: the primary is in the retrieved address list.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_gso_1_M_get_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at;
	union sctp_sockstore store;
	socklen_t len;
	sctp_assoc_t ids[2];
	int cnt=0;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:			
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	sctp_freepaddrs(sa);
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setprim/gso_1_1_set_prim
 * TEST-DESCR: On a 1-1 socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Pick a new primary
 * TEST-DESCR: and set it. Then retrieve the primary address. Validate
 * TEST-DESCR: the primary is the new one.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_sso_1_1_set_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt = 0;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	if(cnt == 0) {
		/* we want the second one */
		if (sa->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else
			len = sizeof(struct sockaddr_in6);
		setit = (struct sockaddr *)((caddr_t)sa  + len);
	} else {
		/* we want the first one */
		setit = sa;
	}
	/* now do the set */
 set_again:
	cnt = 0;
	result = sctp_set_primary(fds[0], 0, setit);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (setit->sa_family != store.sa.sa_family) {
		found = 1;
	} else {
		if(setit->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)setit;
			if(sin->sin_addr.s_addr !=
			   store.sin.sin_addr.s_addr) {
				found = 1;
			}
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)setit;
			if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
			}
		}
	}
	if (found) {
		if (cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto set_again;
		}
		retstring = "set to new primary failed";
	}
	
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setprim/gso_1_M_set_prim
 * TEST-DESCR: On a 1-M socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Pick a new primary
 * TEST-DESCR: and set it. Then retrieve the primary address. Validate
 * TEST-DESCR: the primary is the new one.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_sso_1_M_set_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt=0;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	if(cnt == 0) {
		/* we want the second one */
		if (sa->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else
			len = sizeof(struct sockaddr_in6);
		setit = (struct sockaddr *)((caddr_t)sa  + len);
	} else {
		/* we want the first one */
		setit = sa;
	}
	/* now do the set */
	cnt = 0;
 set_again:
	result = sctp_set_primary(fds[0], ids[0], setit);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (setit->sa_family != store.sa.sa_family) {
		found = 1;
	} else {
		if(setit->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)setit;
			if(sin->sin_addr.s_addr !=
			   store.sin.sin_addr.s_addr) {
				found = 1;
			}
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)setit;
			if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
			}
		}
	}
	if (found) {
		if (cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto set_again;
		}
		retstring = "set to new primary failed";
	}
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}
/*
 * TEST-TITLE setprim/gso_1_1_bad_prim
 * TEST-DESCR: On a 1-1 socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Pick a new primary
 * TEST-DESCR: but corrupt it to be a non-valid address.
 * TEST-DESCR: Set it. Validate that it fails OR the 
 * TEST-DESCR: primary has not changed.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_sso_1_1_bad_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	struct sockaddr_in sinc;
	socklen_t len;
	int cnt=0;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;
	} 
	/* ok we now know that cnt is the current primary */
	sinc = *((struct sockaddr_in *)sa);
	sinc.sin_addr.s_addr = 0x12345678;
	setit = (struct sockaddr *)&sinc;

	result = sctp_set_primary(fds[0], 0, setit);
	if (result < 0) {
		/* good we rejected it */
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if(setit->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)setit;
		if(sin->sin_addr.s_addr !=
		   store.sin.sin_addr.s_addr) {
			found = 1;
		}
	} else {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)setit;
		if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
				       &store.sin6.sin6_addr)) {
			found = 1;
		}
	}
	if (found == 0) {
		retstring = "set to corrupt primary allowed";
	}
	
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE setprim/gso_1_M_bad_prim
 * TEST-DESCR: On a 1-M socket, create an association.
 * TEST-DESCR: After creating, get the address list and
 * TEST-DESCR: retrieve the primary address. Pick a new primary
 * TEST-DESCR: but corrupt it to be a non-valid address.
 * TEST-DESCR: Set it. Validate that it fails OR the 
 * TEST-DESCR: primary has not changed.
 * TEST-DESCR: Note we must be multi-homed for this to work.
 */
static void
test_setprim_sso_1_M_bad_prim(void)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sinc;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt=0;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	sinc = *((struct sockaddr_in *)sa);
	sinc.sin_addr.s_addr = 0x12345678;
	setit = (struct sockaddr *)&sinc;

	/* now do the set */
	result = sctp_set_primary(fds[0], ids[0], setit);
	if (result < 0) {
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (found) {
		retstring = "set to new primary failed";
	}

	found = 0;
	if(setit->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)setit;
		if(sin->sin_addr.s_addr !=
		   store.sin.sin_addr.s_addr) {
			found = 1;
		}
	} else {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)setit;
		if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
				       &store.sin6.sin6_addr)) {
			found = 1;
		}
	}
	if (found == 0) {
		retstring = "set to corrupt primary allowed";
	}
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}


/********************************************************
 *
 * SCTP_ADAPTATION tests
 *
 ********************************************************/
/*
 * TEST-TITLE adaptation/gso_1_1
 * TEST-DESCR: On a 1-1 socket, and retrieve the
 * TEST-DESCR: adaptation layer indication set on 
 * TEST-DESCR: the endpoint.
 */
static void
test_adaptation_gso_1_1(void)
{
	int fd, result;
	uint32_t val;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;
}
/*
 * TEST-TITLE adaptation/gso_1_M
 * TEST-DESCR: On a 1-1 socket, and retrieve the
 * TEST-DESCR: adaptation layer indication set on 
 * TEST-DESCR: the endpoint.
 */
static void
test_adaptation_gso_1_M(void)
{
	int fd, result;
	uint32_t val;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;
}

/*
 * TEST-TITLE adaptation/sso_1_1
 * TEST-DESCR: On a 1-1 socket, and retrieve the
 * TEST-DESCR: adaptation layer indication set on 
 * TEST-DESCR: the endpoint add one to it and set it.
 * TEST-DESCR: Validate that we set the new value.
 */
static void
test_adaptation_sso_1_1(void)
{
	int fd, result;
	uint32_t val, val1;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	val1 =  val + 1;
	result = sctp_set_adaptation(fd, val1);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if(val1 != val) {
		errx(-1, "Set failed, not new value that was set");
	}
	return;

}

/*
 * TEST-TITLE adaptation/sso_1_M
 * TEST-DESCR: On a 1-M socket, and retrieve the
 * TEST-DESCR: adaptation layer indication set on 
 * TEST-DESCR: the endpoint add one to it and set it.
 * TEST-DESCR: Validate that we set the new value.
 */
static void
test_adaptation_sso_1_M(void)
{
	int fd, result;
	uint32_t val, val1;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	val1 =  val + 1;
	result = sctp_set_adaptation(fd, val1);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if(val1 != val) {
		printf("%x was what I set, val:%x\n", 
		       val1, val);
		errx(-1, "Set failed, not new value that was set");
	}
	return;
}


/********************************************************
 *
 * SCTP_DISABLE_FRAGMENTS tests
 *
 ********************************************************/
/*
 * TEST-TITLE disfrag/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket, get the disable 
 * TEST-DESCR: fragmentation setting. Validate it
 * TEST-DESCR: is not enabled (sctp will fragment messages).
 */
static void
test_disfrag_gso_def_1_1(void)
{
	int fd, result;
	int val=0;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(val) {
		errx(-1, "Incorrect default, SCTP fragmentation is disabled");
	}
	return;
}

/*
 * TEST-TITLE disfrag/gso_def_1_M
 * TEST-DESCR: On a 1-M socket, get the disable 
 * TEST-DESCR: fragmentation setting. Validate it
 * TEST-DESCR: is not enabled (sctp will fragment messages).
 */
static void
test_disfrag_gso_def_1_M(void)
{
	int fd, result;
	int val=0;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(val) {
		errx(-1, "Incorrect default, SCTP fragmentation is disabled");
	}
	return;
}


/*
 * TEST-TITLE disfrag/sso_1_1
 * TEST-DESCR: On a 1-1 socket, get the disable 
 * TEST-DESCR: fragmentation setting. Change it to the 
 * TEST-DESCR: opposite. Validate that our set was
 * TEST-DESCR: successful.
 */
static void
test_disfrag_sso_1_1(void)
{
	int fd, result;
	int val=0,nval;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	nval = !val;
	result = sctp_set_disfrag(fd, nval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if (val != nval) {
		errx(-1, "disable fragments not changed");
	}
	return;
}

/*
 * TEST-TITLE disfrag/sso_1_M
 * TEST-DESCR: On a 1-1 socket, get the disable 
 * TEST-DESCR: fragmentation setting. Change it to the 
 * TEST-DESCR: opposite. Validate that our set was
 * TEST-DESCR: successful.
 */
static void
test_disfrag_sso_1_M(void)
{
	int fd, result;
	int val=0,nval;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	nval = !val;
	result = sctp_set_disfrag(fd, nval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	if (val != nval) {
		errx(-1, "disable fragments not changed");
	}
	return;
}

/********************************************************
 *
 * SCTP_PEER_ADDR_PARAMS tests
 *
 ********************************************************/

/*
 * TEST-TITLE paddrpara/gso_1_1
 * TEST-DESCR: On a 1-1 socket, retrieve the
 * TEST-DESCR: endpoint default settings and
 * TEST-DESCR: validate they conform to RFC 4960.
 */
static void
test_paddrpara_gso_def_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval;
	uint16_t maxrxt;
	uint32_t pathmtu;
	uint32_t flags;
	uint32_t ipv6_flowlabel;
	uint8_t ipv4_tos;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval,
				      &maxrxt,
				      &pathmtu,
				      &flags,
				      &ipv6_flowlabel,
				      &ipv4_tos);
	close(fd);
	if (result< 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (hbinterval != 30000) {
		errx(-1, "HB Interval not compliant to RFC 4960");
	}
	if (maxrxt != 5) {
		errx(-1, "Path Max RXT not compliant to RFC 4960");
	}
	if (ipv6_flowlabel) {
		errx(-1, "IPv6 Flow label something other than 0 by default");
	}
	if (ipv4_tos) {
		errx(-1, "IPv4 TOS something other than 0 by default");
	}
	if (flags & SPP_PMTUD_DISABLE) {
		errx(-1, "Path MTU not enabled by default");
	}
	if (flags & SPP_HB_DISABLE) {
		errx(-1, "HB not enabled by default");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/gso_1_M
 * TEST-DESCR: On a 1-M socket, retrieve the
 * TEST-DESCR: endpoint default settings and
 * TEST-DESCR: validate they conform to RFC 4960.
 */
static void
test_paddrpara_gso_def_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval;
	uint16_t maxrxt;
	uint32_t pathmtu;
	uint32_t flags;
	uint32_t ipv6_flowlabel;
	uint8_t ipv4_tos;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval,
				      &maxrxt,
				      &pathmtu,
				      &flags,
				      &ipv6_flowlabel,
				      &ipv4_tos);
	close(fd);
	if (result< 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (hbinterval != 30000) {
		errx(-1, "HB Interval not compliant to RFC4960");
	}
	if (maxrxt != 5) {
		errx(-1, "Path Max RXT not compliant to RFC4960");
	}
	if (ipv6_flowlabel) {
		errx(-1, "IPv6 Flow label something other than 0 by default");
	}
	if (ipv4_tos) {
		errx(-1, "IPv4 TOS something other than 0 by default");
	}
	if (flags & SPP_PMTUD_DISABLE) {
		errx(-1, "Path MTU not enabled by default");
	}
	if (flags & SPP_HB_DISABLE) {
		errx(-1, "HB not enabled by default");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_hb_int_1_1
 * TEST-DESCR: On a 1-1 socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: hb interval, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_int_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = (hbinterval[0] * 2);

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_hb_int_1_M
 * TEST-DESCR: On a 1-M socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: hb interval, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_int_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = (hbinterval[0] * 2);

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_hb_zero_1_1
 * TEST-DESCR: On a 1-1 socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: hb interval to zero, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_zero_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_zero_1_M
 * TEST-DESCR: On a 1-M socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: hb interval to zero, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_zero_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_off_1_1
 * TEST-DESCR: On a 1-1 socket, Retreive
 * TEST-DESCR: the current values, 
 * TEST-DESCR: turn off hb's, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_off_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_off_1_M
 * TEST-DESCR: On a 1-M socket, Retreive
 * TEST-DESCR: the current values, 
 * TEST-DESCR: turn off hb's, assure the change
 * TEST-DESCR: happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_off_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_on_1_1
 * TEST-DESCR: On a 1-1 socket, Retreive
 * TEST-DESCR: the current values, 
 * TEST-DESCR: turn off hb's then turn them back on, 
 * TEST-DESCR: assure the change happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_on_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fd);
		errx(-1, "%s", retstring);
	}
	result = sctp_set_hbenable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }
        close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_on_1_M
 * TEST-DESCR: On a 1-M socket, Retreive
 * TEST-DESCR: the current values, 
 * TEST-DESCR: turn off hb's then turn them back on, 
 * TEST-DESCR: assure the change happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_hb_on_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[3];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto outquick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto outquick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto outquick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto outquick;
	}
        if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB could not disable";
	outquick:
                close(fd);
		if (retstring != NULL)
		errx(-1, "%s", retstring);
	}


	result = sctp_set_hbenable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
	}
	close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_pmrxt_int_1_1
 * TEST-DESCR: On a 1-1 socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: path max retransmit.
 * TEST-DESCR: assure the change happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_pmrxt_int_1_1(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_pmrxt_int_1_M
 * TEST-DESCR: On a 1-M socket, Retreive
 * TEST-DESCR: the current values, change the
 * TEST-DESCR: path max retransmit.
 * TEST-DESCR: assure the change happens and no other parameters
 * TEST-DESCR: are effected.
 */
static void
test_paddrpara_sso_pmrxt_int_1_M(void)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_bad_hb_en_1_1
 * TEST-DESCR: On a 1-1 socket, Set the
 * TEST-DESCR: flags to both enable and disable
 * TEST-DESCR: HB at the same time, we expect
 * TEST-DESCR: the call to fail.
 */
static void
test_paddrpara_sso_bad_hb_en_1_1(void)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	flags = SPP_HB_ENABLE | SPP_HB_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		errx(-1, "Able to enable and disable HB");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_bad_hb_en_1_M
 * TEST-DESCR: On a 1-M socket, Set the
 * TEST-DESCR: flags to both enable and disable
 * TEST-DESCR: HB at the same time, we expect
 * TEST-DESCR: the call to fail.
 */
static void
test_paddrpara_sso_bad_hb_en_1_M(void)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	flags = SPP_HB_ENABLE | SPP_HB_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		errx(-1, "Able to enable and disable HB");
	}
	return;
}


/*
 * TEST-TITLE paddrpara/sso_bad_pmtud_en_1_1
 * TEST-DESCR: On a 1-1 socket, Set the
 * TEST-DESCR: flags to both enable and disable
 * TEST-DESCR: PMTU discovery at the same time, we expect
 * TEST-DESCR: the call to fail.
 */
static void
test_paddrpara_sso_bad_pmtud_en_1_1(void)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	flags = SPP_PMTUD_ENABLE | SPP_PMTUD_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		errx(-1, "Able to enable and disable HB");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_bad_pmtud_en_1_M
 * TEST-DESCR: On a 1-M socket, Set the
 * TEST-DESCR: flags to both enable and disable
 * TEST-DESCR: PMTU discovery at the same time, we expect
 * TEST-DESCR: the call to fail.
 */
static void
test_paddrpara_sso_bad_pmtud_en_1_M(void)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	flags = SPP_PMTUD_ENABLE | SPP_PMTUD_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		errx(-1, "Able to enable and disable HB");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_ahb_int_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: set the hb interval on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_int_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 10;

	result = sctp_set_hbint(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);

	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ahb_int_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: set the hb interval on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_int_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3], newval;
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 10;

	result = sctp_set_hbint(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}


/*
 * TEST-TITLE paddrpara/sso_ahb_zero_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: set the hb interval to zero on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_zero_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_ahb_zero_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: set the hb interval to zero on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_zero_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3], newval;
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ahb_off_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: turn heartbeats off on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_off_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_set_hbdisable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ahb_off_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: turn heartbeats off on the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_off_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_set_hbdisable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}

	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ahb_on_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: turn heartbeats off and then back on for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_on_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_set_hbdisable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", retstring);
	}
	result = sctp_set_hbenable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }
	close(fds[0]);
	close(fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_ahb_on_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: turn heartbeats off and then back on for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_ahb_on_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[4];
	uint16_t maxrxt[4];
	uint32_t pathmtu[4];
	uint32_t flags[4];
	uint32_t ipv6_flowlabel[4];
	uint8_t ipv4_tos[4];
	char *retstring = NULL;
	uint32_t munflags[2];
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_set_hbdisable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", retstring);
	}
	result = sctp_set_hbenable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[3],
				      &maxrxt[3],
				      &pathmtu[3],
				      &flags[3],
				      &ipv6_flowlabel[3],
				      &ipv4_tos[3]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);

	if(hbinterval[3] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[3] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }

	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_apmrxt_int_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: set the path max retransmit for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_apmrxt_int_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_apmrxt_int_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: set the path max retransmit for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_apmrxt_int_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint16_t new_maxrxt;
	sctp_assoc_t ids[3];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], ids[0], NULL, new_maxrxt);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_apmtu_dis_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: disable the path mtu algorithm for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_apmtu_dis_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = pathmtu[0] / 2;
	if(newval < 1024)
		newval = 1024;

	result = sctp_set_pmtu(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] & SPP_PMTUD_ENABLE) {
		errx(-1, "failed, pmtu still enabled");
	}
	if (pathmtu[1] != newval) {
		errx(-1, "failed, pmtu not set to correct value");
	}
	muflags[0] = flags[0] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	muflags[1] = flags[1] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	if(muflags[0] != muflags[1]) {
		errx(-1, "flag settings changed");
		
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings changed");
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		errx(-1, "ipv6 flowlabel settings changed");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_apmtu_dis_1_M
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: disable the path mtu algorithm for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_apmtu_dis_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = pathmtu[0] / 2;
	if(newval < 1024)
		newval = 1024;

	result = sctp_set_pmtu(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] & SPP_PMTUD_ENABLE) {
		errx(-1, "failed, pmtu still enabled");
	}
	if (pathmtu[1] != newval) {
		errx(-1, "failed, pmtu not set to correct value");
	}
	muflags[0] = flags[0] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	muflags[1] = flags[1] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	if(muflags[0] != muflags[1]) {
		errx(-1, "flag settings changed");
		
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings changed");
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		errx(-1, "ipv6 flowlabel settings changed");
	}
	return;

}

/*
 * TEST-TITLE paddrpara/sso_av6_flow_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: set the v6 flowlabel for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_av6_flo_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 1;
	result = sctp_set_flow(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		errx(-1, "failed, flags changed");
	}
	if (ipv6_flowlabel[1] != newval) {
		errx(-1, "failed, flowlabel not set to correct value");
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings changed");
	}
        return;

}

/*
 * TEST-TITLE paddrpara/sso_av6_flow_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: set the v6 flowlabel for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_av6_flo_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 1;
	result = sctp_set_flow(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		errx(-1, "failed, flags changed");
	}
	if (ipv6_flowlabel[1] != newval) {
		errx(-1, "failed, flowlabel not set to correct value");
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings changed");
	}
        return;
}

/*
 * TEST-TITLE paddrpara/sso_av4_tos_1_1
 * TEST-DESCR: On a 1-1 socket, Create an assoc and
 * TEST-DESCR: set the v4 tos (dscp)  for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_av4_tos_1_1(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint8_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 4;
	result = sctp_set_tos(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		errx(-1, "failed, flags changed");
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		errx(-1, "ipv6 flow label settings changed");
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(newval != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings not changed");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_av4_tos_1_M
 * TEST-DESCR: On a 1-M socket, Create an assoc and
 * TEST-DESCR: set the v4 tos (dscp)  for the association.
 * TEST-DESCR: Assure its set and no other values changed.
 */
static void
test_paddrpara_sso_av4_tos_1_M(void)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint8_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	newval = 4;
	result = sctp_set_tos(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		errx(-1, "failed, flags changed");
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		errx(-1, "ipv6 flow label settings changed");
	}
	if (hbinterval[1] != hbinterval[0]) {
		errx(-1, "hb interval settings changed");
	}

	if (maxrxt[1] != maxrxt[0]) {
		errx(-1, "maxrxt settings changed");
	}
	if(newval != ipv4_tos[1]) {
		errx(-1, "ipv4 tos settings not changed");
	}
	return;
}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_int_1_1
 * TEST-DESCR: On a 1-1 socket, set the heartbeat interval
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_int_1_1(void)
{
	int fd, result;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = (hbinterval[0] * 2);

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_int_1_M
 * TEST-DESCR: On a 1-M socket, set the heartbeat interval
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_int_1_M(void)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint32_t newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = (hbinterval[0] * 2);

	/* Set the assoc value */
	result = sctp_set_hbint(fds[0], 0, NULL, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_zero_1_1
 * TEST-DESCR: On a 1-1 socket, set the heartbeat interval to 0
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_zero_1_1(void)
{
	int fd, result;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_zero_1_M
 * TEST-DESCR: On a 1-M socket, set the heartbeat interval to 0
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_zero_1_M(void)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint32_t newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = 0;
	result = sctp_set_hbzero(fds[0], 0, NULL);
	/* Set the assoc value */

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_off_1_1
 * TEST-DESCR: On a 1-1 socket, set heartbeat to off
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_off_1_1(void)
{
	int fd, result;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	muflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB ep did not disable HB";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changecd";
	}
	if (flags[2] & SPP_HB_ENABLE) {
		retstring = "HB did not stay disabled";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ainhhb_off_1_M
 * TEST-DESCR: On a 1-M socket, set heartbeat to off
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhhb_off_1_M(void)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	result = sctp_set_hbdisable(fds[0], 0, NULL);
	/* Set the assoc value */
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	muflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
	}
	if(flags[1] & SPP_HB_ENABLE) {
		retstring = "HB ep did not disable HB";
	}
	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], ids[0], NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}

	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);

	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}
	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changed";
	}
	if(flags[2] & SPP_HB_ENABLE) {
		retstring = "HB did not stay disabled";
	}

 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_ainhpmrxt_int_1_1
 * TEST-DESCR: On a 1-1 socket, set path max retransmit
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhpmrxt_int_1_1(void)
{
	int fd, result;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint16_t new_maxrxt;

	fd = sctp_one2one(0, 1, 0);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);

	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed ep failed";
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change on ep";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed on ep";
	}
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(new_maxrxt != maxrxt[2]) {
		retstring = "maxrxt did NOT change on assoc";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changed";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_ainhpmrxt_int_1_M
 * TEST-DESCR: On a 1-M socket, set path max retransmit
 * TEST-DESCR: on the socket. Create an assoc and
 * TEST-DESCR: assure the new value inherited to the association.
 */
static void
test_paddrpara_sso_ainhpmrxt_int_1_M(void)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;
	uint16_t new_maxrxt;

	fds[0] = sctp_one2many(0,0);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, NULL, new_maxrxt);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(new_maxrxt != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}

	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);

	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}
	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changecd";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dhb_int_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and set
 * TEST-DESCR: the heartbeat interval for the association.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_int_1_1(void)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	newval = (hbinterval[0] * 2) + 10;
	result = sctp_set_hbint(fds[0], 0, sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dhb_int_1_m
 * TEST-DESCR: On a 1-M socket, create an assocation and set
 * TEST-DESCR: the heartbeat interval for the association.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_int_1_M(void)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;
	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	newval = (hbinterval[0] * 2) + 10;
	result = sctp_set_hbint(fds[0], ids[0], sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dhb_zero_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and set
 * TEST-DESCR: the heartbeat interval for the association to zero.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_zero_1_1(void)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], 0, sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dhb_zero_1_M
 * TEST-DESCR: On a 1-M socket, create an assocation and set
 * TEST-DESCR: the heartbeat interval for the association to zero.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_zero_1_M(void)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], ids[0], sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dhb_off_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and turn
 * TEST-DESCR: heartbeating or the association.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_off_1_1(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	result = sctp_set_hbdisable(fds[0], 0, sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	muflags[0] = (flags[0] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	muflags[1] = (flags[1] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	if (muflags[0] != muflags[1]) {
		retstring = "flags changed";
		goto out;

	}
	if( flags[1] & SPP_HB_ENABLE) {
		retstring = "HB not disabled";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_dhb_off_1_M
 * TEST-DESCR: On a 1-M socket, create an assocation and turn
 * TEST-DESCR: heartbeating or the association.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dhb_off_1_M(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint32_t muflags[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;
	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result = sctp_set_hbdisable(fds[0],ids[0], sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	muflags[0] = (flags[0] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	muflags[1] = (flags[1] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	if (muflags[0] != muflags[1]) {
		retstring = "flags changed";
		goto out;

	}
	if( flags[1] & SPP_HB_ENABLE) {
		retstring = "HB not disabled";
	}

 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}


/*
 * TEST-TITLE paddrpara/sso_dpmrxt_int_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and 
 * TEST-DESCR: set the path max retransmit.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dpmrxt_int_1_1(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2], new_maxrxt;
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, sa, new_maxrxt);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dpmrxt_int_1_M
 * TEST-DESCR: On a 1-M socket, create an assocation and 
 * TEST-DESCR: set the path max retransmit.
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dpmrxt_int_1_M(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2], new_maxrxt;
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], ids[0], sa, new_maxrxt);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_dpmrxt_int_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and 
 * TEST-DESCR: set the ipv4 tos (DSCP).
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dav4_tos_1_1(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2], newval;
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 4;
	result = sctp_set_tos(fds[0], 0, sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(newval != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_dpmrxt_int_1_M
 * TEST-DESCR: On a 1-M socket, create an assocation and 
 * TEST-DESCR: set the ipv4 tos (DSCP).
 * TEST-DESCR: assure the new value is on the association and
 * TEST-DESCR: no other values have changed.
 */
static void
test_paddrpara_sso_dav4_tos_1_M(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2], newval;
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 4;
	result = sctp_set_tos(fds[0], ids[0], sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(newval != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE paddrpara/sso_hb_demand_1_1
 * TEST-DESCR: On a 1-1 socket, create an assocation and 
 * TEST-DESCR: demand a heartbeat.
 * TEST-DESCR: assure that no settings on the association
 * TEST-DESCR: have changed.
 */
static void
test_paddrpara_sso_hb_demand_1_1(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result =  sctp_set_paddr_param(fds[0], 0, 
				       sa, 0, 0, 0,
				       SPP_HB_DEMAND,
				       0, 0);


	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE paddrpara/sso_hb_demand_1_M
 * TEST-DESCR: On a 1-M socket, create an assocation and 
 * TEST-DESCR: demand a heartbeat.
 * TEST-DESCR: assure that no settings on the association
 * TEST-DESCR: have changed.
 */
static void
test_paddrpara_sso_hb_demand_1_M(void)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sctp_delay(SCTP_SLEEP_MS);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result =  sctp_set_paddr_param(fds[0], ids[0], 
				       sa, 0, 0, 0,
				       SPP_HB_DEMAND,
				       0, 0);


	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}
/********************************************************
 *
 * SCTP_DEFAULT_SEND_PARAM tests
 *
 ********************************************************/

/*
 * TEST-TITLE defsend/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Get the default send parameters
 * TEST-DESCR: and validate they are all 0.
 */
static void
test_defsend_gso_def_1_1(void)
{
	struct sctp_sndrcvinfo sinfo;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (sinfo.sinfo_stream != 0) {
		errx(-1, "Default is not to send on stream 0");
	}
	if (sinfo.sinfo_flags != 0) {
		errx(-1, "Default sends with options set");
	}
	if (sinfo.sinfo_ppid != 0) {
		errx(-1, "Default sends with non-zero ppid");
	}
	if (sinfo.sinfo_context != 0) {
		errx(-1, "Default sends with non-zero context");
	}
	return;
}

/*
 * TEST-TITLE defsend/gso_def_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Get the default send parameters
 * TEST-DESCR: and validate they are all 0.
 */
static void
test_defsend_gso_def_1_M(void)
{
	struct sctp_sndrcvinfo sinfo;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (sinfo.sinfo_stream != 0) {
		errx(-1, "Default is not to send on stream 0");
	}
	if (sinfo.sinfo_flags != 0) {
		errx(-1, "Default sends with options set");
	}
	if (sinfo.sinfo_ppid != 0) {
		errx(-1, "Default sends with non-zero ppid");
	}
	if (sinfo.sinfo_context != 0) {
		errx(-1, "Default sends with non-zero context");
	}
	return;
}

/*
 * TEST-TITLE defsend/sso_on_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Change the default stream.
 * TEST-DESCR: Validate that it changed and
 * TEST_DESCR: all other settings remain unchanged.
 */
static void
test_defsend_sso_on_1_1(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_defsend(fd, 0, &sinfo[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;
}

/*
 * TEST-TITLE defsend/sso_on_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Change the default stream.
 * TEST-DESCR: Validate that it changed and
 * TEST_DESCR: all other settings remain unchanged.
 */
static void
test_defsend_sso_on_1_M(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_defsend(fd, 0, &sinfo[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;
}

/*
 * TEST-TITLE defsend/sso_asc_1_1
 * TEST-DESCR: On a 1-1 socket create an assocaition.
 * TEST-DESCR: Change default stream on the assocation.
 * TEST-DESCR: Validate that it changed and
 * TEST_DESCR: all other settings remain unchanged.
 */
static void
test_defsend_sso_asc_1_1(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;
}

/*
 * TEST-TITLE defsend/sso_asc_1_M
 * TEST-DESCR: On a 1-M socket create an assocaition.
 * TEST-DESCR: Change default stream on the assocation.
 * TEST-DESCR: Validate that it changed and
 * TEST_DESCR: all other settings remain unchanged.
 */
static void
test_defsend_sso_asc_1_M(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;
}

/*
 * TEST-TITLE defsend/sso_inherit_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Change default stream on the endpoint.
 * TEST-DESCR: Create an assocation and
 * TEST-DESCR: validate that the change was
 * TEST_DESCR: inherited to the association.
 */
static void
test_defsend_sso_inherit_1_1(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;

}

/*
 * TEST-TITLE defsend/sso_inherit_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Change default stream on the endpoint.
 * TEST-DESCR: Create an assocation and
 * TEST-DESCR: validate that the change was
 * TEST_DESCR: inherited to the association.
 */
static void
test_defsend_sso_inherit_1_M(void)
{
	struct sctp_sndrcvinfo sinfo[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		errx(-1, "Def send stream did not change");
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		errx(-1, "sinfo_flags changed");
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		errx(-1, "sinfo context changed");
	}
	return;


}

/*
 * TEST-TITLE defsend/sso_inherit_ncep_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Change default stream on the endpoint.
 * TEST-DESCR: Create an assocation and
 * TEST-DESCR: validate that the change was
 * TEST_DESCR: inherited to the association. Then.
 * TEST_DESCR: change the association value. Validate
 * TEST_DESCR: it changed and the endpoint did not change.
 */
static void
test_defsend_sso_inherit_ncep_1_1(void)
{
	struct sctp_sndrcvinfo sinfo[3];
	int fd, result;
	char *retstring = NULL;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[1], 0, &sinfo[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	/* Now get the listener value, it should NOT have changed */
	result = sctp_get_defsend(fd, 0, &sinfo[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
 out:
 	close (fd);
 	close (fds[0]);
 	close (fds[1]);
 	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE defsend/sso_inherit_ncep_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Change default stream on the endpoint.
 * TEST-DESCR: Create an assocation and
 * TEST-DESCR: validate that the change was
 * TEST_DESCR: inherited to the association. Then.
 * TEST_DESCR: change the association value. Validate
 * TEST_DESCR: it changed and the endpoint did not change.
 */
static void
test_defsend_sso_inherit_ncep_1_M(void)
{
	struct sctp_sndrcvinfo sinfo[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE defsend/sso_nc_other_asc_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Change default stream on the endpoint.
 * TEST-DESCR: Create two assocation and
 * TEST-DESCR: validate that the change was
 * TEST_DESCR: inherited to the association. Then.
 * TEST_DESCR: change one of the association values. Validate
 * TEST_DESCR: it changed and the endpoint and other
 * TEST_DESCR: association did not change.
 */
static void
test_defsend_sso_nc_other_asc_1_M(void)
{
	struct sctp_sndrcvinfo sinfo[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	int fds2[2];
	sctp_assoc_t ids[2], ids2[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* Create a second assoc for fds[0] */
	fds2[0] = fds[0];
	result = sctp_socketpair_1tom(fds2, ids2,  1);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_defsend(fds[0], 0, &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
	/* check other asoc */
	result = sctp_get_defsend(fds[0], ids2[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected other assoc";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	close (fds2[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/********************************************************
 *
 * SCTP_EVENTS tests
 *
 ********************************************************/

/*
 * TEST-TITLE events/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Get the default events and 
 * TEST-DESCR: validate that no notification is
 * TEST-DESCR: enabled.
 */
static void
test_events_gso_def_1_1(void)
{
	struct sctp_event_subscribe ev;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	memset(&ev, 0, sizeof(ev));
	result = sctp_get_events(fd, &ev);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(ev.sctp_data_io_event)
		errx(-1, "data_io_event not defaulted to off");

	if (ev.sctp_association_event)
		errx(-1, "association_event not defaulted to off");
	if (ev.sctp_address_event)
		errx(-1, "address_event not defaulted to off");
	if (ev.sctp_send_failure_event)
		errx(-1, "send_failure_event not defaulted to off");
	if (ev.sctp_peer_error_event)
		errx(-1, "peer_error_event not defaulted to off");
	if (ev.sctp_shutdown_event)
		errx(-1, "shutdown_event not defaulted to off");
	if (ev.sctp_partial_delivery_event)
		errx(-1, "pdap_event not defaulted to off");
	if (ev.sctp_adaptation_layer_event)
		errx(-1, "adaptation_event not defaulted to off");
	if (ev.sctp_authentication_event)
		errx(-1, "authentication_event not defaulted to off");
	return;
}

/*
 * TEST-TITLE events/gso_def_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Get the default events and 
 * TEST-DESCR: validate that no notification is
 * TEST-DESCR: enabled.
 */
static void
test_events_gso_def_1_M(void)
{
	struct sctp_event_subscribe ev;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	memset(&ev, 0, sizeof(ev));
	result = sctp_get_events(fd, &ev);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(ev.sctp_data_io_event)
		errx(-1, "data_io_event not defaulted to off");

	if (ev.sctp_association_event)
		errx(-1, "association_event not defaulted to off");
	if (ev.sctp_address_event)
		errx(-1, "address_event not defaulted to off");
	if (ev.sctp_send_failure_event)
		errx(-1, "send_failure_event not defaulted to off");
	if (ev.sctp_peer_error_event)
		errx(-1, "peer_error_event not defaulted to off");
	if (ev.sctp_shutdown_event)
		errx(-1, "shutdown_event not defaulted to off");
	if (ev.sctp_partial_delivery_event)
		errx(-1, "pdap_event not defaulted to off");
	if (ev.sctp_adaptation_layer_event)
		errx(-1, "adaptation_event not defaulted to off");
	if (ev.sctp_authentication_event)
		errx(-1, "authentication_event not defaulted to off");
	return;

}

/*
 * TEST-TITLE events/sso_def_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Change two of the settings to the
 * TEST-DESCR: opposite value with the set option.
 * TEST-DESCR: validate that the new settings are
 * TEST-DESCR: correct and no other un-changed settings
 * TEST-DESCR: changed.
 */
static void
test_events_sso_1_1(void)
{
	struct sctp_event_subscribe ev[2];
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	memset(&ev, 0, sizeof(ev));
	result = sctp_get_events(fd, &ev[0]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(ev[0].sctp_data_io_event == 0) {
		ev[0].sctp_data_io_event = 1;
	} else {
		ev[0].sctp_data_io_event = 0;
	}
	if(ev[0].sctp_association_event == 0) {
		ev[0].sctp_association_event = 1;
	} else {
		ev[0].sctp_association_event = 0;
	}
	result = sctp_set_events(fd, &ev[0]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_events(fd, &ev[1]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(ev[0].sctp_data_io_event != ev[1].sctp_data_io_event)
		errx(-1, "data_io_event set failed");

	if (ev[0].sctp_association_event != ev[1].sctp_association_event)
		errx(-1, "association_event set failed");

	if (ev[0].sctp_address_event != ev[1].sctp_address_event)
		errx(-1, "address_event changed");

	if (ev[0].sctp_send_failure_event != ev[1].sctp_send_failure_event)
		errx(-1, "send_failure_event changed");

	if (ev[0].sctp_peer_error_event != ev[1].sctp_peer_error_event)
		errx(-1, "peer_error_event changed");

	if (ev[0].sctp_shutdown_event != ev[1].sctp_shutdown_event) 
		errx(-1, "shutdown_event changed");
	if (ev[0].sctp_partial_delivery_event !=ev[1].sctp_partial_delivery_event)
		errx(-1, "pdap_event changed");
	if (ev[0].sctp_adaptation_layer_event != ev[1].sctp_adaptation_layer_event)
		errx(-1, "adaptation_event changed");
	if (ev[0].sctp_authentication_event != ev[1].sctp_authentication_event)
		errx(-1, "authentication_event changed");
	return;
}

/*
 * TEST-TITLE events/sso_def_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Change two of the settings to the
 * TEST-DESCR: opposite value with the set option.
 * TEST-DESCR: validate that the new settings are
 * TEST-DESCR: correct and no other un-changed settings
 * TEST-DESCR: changed.
 */
static void
test_events_sso_1_M(void)
{
	struct sctp_event_subscribe ev[2];
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	memset(&ev, 0, sizeof(ev));
	result = sctp_get_events(fd, &ev[0]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(ev[0].sctp_data_io_event == 0) {
		ev[0].sctp_data_io_event = 1;
	} else {
		ev[0].sctp_data_io_event = 0;
	}
	if(ev[0].sctp_association_event == 0) {
		ev[0].sctp_association_event = 1;
	} else {
		ev[0].sctp_association_event = 0;
	}
	result = sctp_set_events(fd, &ev[0]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_events(fd, &ev[1]);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(ev[0].sctp_data_io_event != ev[1].sctp_data_io_event)
		errx(-1, "data_io_event set failed");

	if (ev[0].sctp_association_event != ev[1].sctp_association_event)
		errx(-1, "association_event set failed");

	if (ev[0].sctp_address_event != ev[1].sctp_address_event)
		errx(-1, "address_event changed");

	if (ev[0].sctp_send_failure_event != ev[1].sctp_send_failure_event)
		errx(-1, "send_failure_event changed");

	if (ev[0].sctp_peer_error_event != ev[1].sctp_peer_error_event)
		errx(-1, "peer_error_event changed");

	if (ev[0].sctp_shutdown_event != ev[1].sctp_shutdown_event) 
		errx(-1, "shutdown_event changed");
	if (ev[0].sctp_partial_delivery_event !=ev[1].sctp_partial_delivery_event)
		errx(-1, "pdap_event changed");
	if (ev[0].sctp_adaptation_layer_event != ev[1].sctp_adaptation_layer_event)
		errx(-1, "adaptation_event changed");
	if (ev[0].sctp_authentication_event != ev[1].sctp_authentication_event)
		errx(-1, "authentication_event changed");
	return;
}


/********************************************************
 *
 * SCTP_I_WANT_MAPPED_V4_ADDR tests
 *
 ********************************************************/
/*
 * TEST-TITLE mapped/gso_1_1_def
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Get the setting and validate the default.
 */
static void
test_mapped_gso_1_1_def(void)
{
	int fd, onoff;
	
	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	onoff = sctp_v4_address_mapping_enabled(fd);
	close(fd);
	
	if (onoff) {
		errx(-1, "Option enabled by default");
	} else {
		return;
	}
}

/*
 * TEST-TITLE mapped/gso_1_M_def
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Get the setting and validate the default.
 */
static void
test_mapped_gso_1_M_def(void)
{
	int fd, onoff;
	
	fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	onoff = sctp_v4_address_mapping_enabled(fd);
	close(fd);
	
	if (onoff) {
		errx(-1, "Option enabled by default");
	} else {
		return;
	}
}

/*
 * TEST-TITLE mapped/sso_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Toggle the setting to the opposite
 * TEST-DESCR: value and validate it can be set.
 */
static void
test_mapped_sso_1_1(void)
{
	socklen_t len;
	int val, result, val2;
	int fd;

	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, &len);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if (val == 0) {
		val = 1;
	} else {
		val = 0;
	}
	len = sizeof(val);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, len);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val2);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val2, &len);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(val != val2) {
		errx(-1, "Could not change mapped v4 address");
	}
	return;
}

/*
 * TEST-TITLE mapped/sso_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Toggle the setting to the opposite
 * TEST-DESCR: value and validate it can be set.
 */
static void
test_mapped_sso_1_M(void)
{
	socklen_t len;
	int val, result, val2;
	int fd;

	fd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, &len);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if (val == 0) {
		val = 1;
	} else {
		val = 0;
	}
	len = sizeof(val);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, len);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val2);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val2, &len);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(val != val2) {
		errx(-1, "Could not change mapped v4 address");
	}
	return;
}

/*
 * TEST-TITLE mapped/sso_bad_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Open a v4 socket and attempt to
 * TEST-DESCR: turn on mapped v4 addresses. Validate
 * TEST-DESCR: it fails.
 */
static void
test_mapped_sso_bad_1_1(void)
{
	socklen_t len;
	int val, result;
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	len = sizeof(val);
	val = 1;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, len);
	close(fd);
	if(result >= 0) {
		errx(-1, "mapped v4 setting allowed on non v6 socket");
	}
	return;
}

/*
 * TEST-TITLE mapped/sso_bad_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Open a v4 socket and attempt to
 * TEST-DESCR: turn on mapped v4 addresses. Validate
 * TEST-DESCR: it fails.
 */
static void
test_mapped_sso_bad_1_M(void)
{
	socklen_t len;
	int val, result;
	int fd;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}

	len = sizeof(val);
	val = 1;
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
			    &val, len);
	close(fd);
	if(result >= 0) {
		errx(-1, "mapped v4 setting allowed on non v6 socket");
	}
	return;
}

/********************************************************
 *
 * SCTP_MAXSEG tests
 *
 ********************************************************/
/*
 * TEST-TITLE maxseg/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Validate that the default setting
 * TEST-DESCR: for maxseg is 0, i.e., no user
 * TEST-DESCR: defined fragmentaton point.
 */
static void
test_maxseg_gso_def_1_1(void)
{
	int fd, val, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if (val != 0) {
		errx(-1, "maxseg not unlimited (i.e. 0)");
	}

	return;
}

/*
 * TEST-TITLE maxseg/sso_def_1_1
 * TEST-DESCR: On a 1-1 socket. Get
 * TEST-DESCR: the max seg, reduce it by
 * TEST-DESCR: 100 bytes and set it. Validate
 * TEST-DESCR: that the new setting worked.
 */
static void
test_maxseg_sso_set_1_1(void)
{
	int fd, val[3], result;
	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	if ((val[0] == 0) || (val[0] > 1452)) {
		val[1] = 1452;
	} else {
		val[1] = val[0] - 100;
	}
	result = sctp_set_maxseg(fd, 0, val[1]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[2]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(val[1] < val[2]) {
		errx(-1, "Set did not work");
	}
	return;
}

/*
 * TEST-TITLE maxseg/gso_def_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Validate that the default setting
 * TEST-DESCR: for maxseg is 0, i.e., no user
 * TEST-DESCR: defined fragmentaton point.
 */
static void
test_maxseg_gso_def_1_M(void)
{
	int fd, val, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if (val != 0) {
		errx(-1, "maxseg not unlimited (i.e. 0)");
	}

	return;
}

/*
 * TEST-TITLE maxseg/sso_def_1_M
 * TEST-DESCR: On a 1-M socket. Get
 * TEST-DESCR: the max seg, reduce it by
 * TEST-DESCR: 100 bytes and set it. Validate
 * TEST-DESCR: that the new setting worked.
 */
static void
test_maxseg_sso_set_1_M(void)
{
	int fd, val[3], result;
	fd = sctp_one2many(0,1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}

	if ((val[0] == 0) || (val[0] > 1452)) {
		val[1] = 1452;
	} else {
		val[1] = val[0] - 100;
	}
	result = sctp_set_maxseg(fd, 0, val[1]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[2]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(val[1] < val[2]) {
		errx(-1, "Set did not work");
	}
	return;
}

/*
 * TEST-TITLE maxseg/sso_asc_1_1
 * TEST-DESCR: On a 1-1 socket, create an association
 * TEST-DESCR: set the max seg on the assoc to 1200 bytes.
 * TEST-DESCR: Validate that the new setting worked.
 */
static void
test_maxseg_sso_asc_1_1(void)
{
	int val[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fds[1], 0, val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);

 	if(val[1] != 1200)
 		errx(-1, "maxseg on assoc not expected value");

 	return;
}

/*
 * TEST-TITLE maxseg/sso_asc_1_M
 * TEST-DESCR: On a 1-M socket, create an association
 * TEST-DESCR: set the max seg on the assoc to 1200 bytes.
 * TEST-DESCR: Validate that the new setting worked.
 */
static void
test_maxseg_sso_asc_1_M(void)
{
	int val[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fds[0], ids[0], val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if(val[1] == val[0])
		errx(-1, "maxseg on assoc did not change");

 	if(val[1] != 1200)
 		errx(-1, "maxseg on assoc not expected value");
	return;
}

/*
 * TEST-TITLE maxseg/sso_inherit_1_1
 * TEST-DESCR: On a 1-1 socket, Set the max seg on the endpoint to 1200 bytes.
 * TEST-DESCR: Create an assocation and validate that the new
 * TEST-DESCR: value gets inherited to the new assocaition.
 */
static void
test_maxseg_sso_inherit_1_1(void)
{
	int val[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fd, 0, val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	if(val[1] != 1200) {
		errx(-1, "maxseg did not inherit to assoc");
	}
	return;

}

/*
 * TEST-TITLE maxseg/sso_inherit_1_M
 * TEST-DESCR: On a 1-M socket, Set the max seg on the endpoint to 1200 bytes.
 * TEST-DESCR: Create an assocation and validate that the new
 * TEST-DESCR: value gets inherited to the new assocaition.
 */
static void
test_maxseg_sso_inherit_1_M(void)
{
	int val[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fds[0], 0, val[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if(val[1] != 1200) {
		errx(-1, "maxseg did not inherit to assoc");
	}
	return;
}

/*
 * TEST-TITLE maxseg/sso_inherit_ncep_1_1
 * TEST-DESCR: On a 1-1 socket, 
 * TEST-DESCR: Set the max seg to 1200 bytes.
 * TEST-DESCR: Start an association and validate it inherits
 * TEST-DESCR: the new value. Then set a different value on 
 * TEST-DESCR: the association. Validate the endpoint value is
 * TEST-DESCR: unchanged.
 */
static void
test_maxseg_sso_inherit_ncep_1_1(void)
{
	int val[3];
	int fd, result;
	char *retstring = NULL;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fd, 0, val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (val[1] != 1200) {
		retstring = "maxseg did not change on asoc";
		goto out;
        }
	/* Change the assoc value */
	val[2] = val[1] + 100;
	result = sctp_set_maxseg(fds[1], 0, val[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	/* Now get the listener value, it should NOT have changed */
	result = sctp_get_maxseg(fd, 0, &val[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected ep";
	}
 out:
 	close (fd);
 	close (fds[0]);
 	close (fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);

}
/*
 * TEST-TITLE maxseg/sso_inherit_ncep_1_M
 * TEST-DESCR: On a 1-M socket, 
 * TEST-DESCR: Set the max seg to 1200 bytes.
 * TEST-DESCR: Start an association and validate it inherits
 * TEST-DESCR: the new value. Then set a different value on 
 * TEST-DESCR: the association. Validate the endpoint value is
 * TEST-DESCR: unchanged.
 */
static void
test_maxseg_sso_inherit_ncep_1_M(void)
{
	int  val[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fds[0], 0, val[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (val[1] != 1200) {
		retstring = "maxseg did not change";
		goto out;
	}
	/* Change the assoc value */
	val[2] = val[1] + 100;
	result = sctp_set_maxseg(fds[0], ids[0], val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], 0, &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (val[2] != val[1]) {
		retstring = "change on assoc, effected ep";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE maxseg/sso_nc_other_asoc_1_M
 * TEST-DESCR: On a 1-M socket, 
 * TEST-DESCR: Set the max seg to 1200 bytes.
 * TEST-DESCR: Start two associations and validate they inherit
 * TEST-DESCR: the new value. Then set a different value on 
 * TEST-DESCR: the association. Validate the endpoint and other
 * TEST-DESCR: associations  value's are unchanged.
 */
static void
test_maxseg_sso_nc_other_asc_1_M(void)
{
	int val[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	int fds2[2];
	sctp_assoc_t ids[2], ids2[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_maxseg(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	val[1] = 1200;
	result = sctp_set_maxseg(fds[0], 0, val[1]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* Create a second assoc for fds[0] */
	fds2[0] = fds[0];
	result = sctp_socketpair_1tom(fds2, ids2,  1);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_maxseg(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (1200 != val[1]) {
		retstring = "maxseg did not change in assoc";
		goto out;
	}
	/* Change the assoc value */
	val[2] = val[1] + 100;;
	result = sctp_set_maxseg(fds[0], ids[0], val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_maxseg(fds[0], 0, &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected ep";
	}
	/* check other asoc */
	result = sctp_get_maxseg(fds[0], ids2[0], &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected other assoc";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	close (fds2[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/********************************************************
 *
 * SCTP_AUTH_CHUNK tests
 *
 ********************************************************/
/*
 * TEST-TITLE authchk/gso_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Attempt to get an auth-chunk id.
 * TEST-DESCR: This should fail since its a set 
 * TEST-DESCR: only option.
 */
static void
test_authchk_gso_1_1(void)
{
	int result;
	int fd;
	uint8_t chk;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_auth_chunk_id(fd, &chk);
	close (fd);
	if (result >= 0) {
		errx(-1, "allowed get of auth chunk id");
	}
	return;
}

/*
 * TEST-TITLE authchk/gso_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Attempt to get an auth-chunk id.
 * TEST-DESCR: This should fail since its a set 
 * TEST-DESCR: only option.
 */
static void
test_authchk_gso_1_M(void)
{
	int result;
	int fd;
	uint8_t chk;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_auth_chunk_id(fd, &chk);
	close (fd);
	if (result >= 0) {
		errx(-1, "allowed get of auth chunk id");
	}
	return;
}

/*
 * TEST-TITLE authchk/sso_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Attempt to add a data chunk (0)
 * TEST-DESCR: to the list of authenticated chunk.
 * TEST-DESCR: This should succeed.
 */
static void
test_authchk_sso_1_1(void)
{
	int result;
	int fd;
	uint8_t chk;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/*
	 * Set to auth a data chunk.
	 */
	chk = 0;
	result = sctp_set_auth_chunk_id(fd, chk);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;
}

/*
 * TEST-TITLE authchk/sso_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Attempt to add a data chunk (0)
 * TEST-DESCR: to the list of authenticated chunk.
 * TEST-DESCR: This should succeed.
 */
static void
test_authchk_sso_1_M(void)
{
	int result;
	int fd;
	uint8_t chk;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/*
	 * Set to auth a data chunk.
	 */
	chk = 0;
	result = sctp_set_auth_chunk_id(fd, chk);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;

}

/********************************************************
 *
 * SCTP_HMAC_IDENT tests
 *
 ********************************************************/
/*
 * TEST-TITLE hmacid/sso_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set in to prefer sha256 then sha1. If
 * TEST-DESCR: that succeeds verify our settings. If it
 * TEST-DESCR: fails then set just sha1 and verify it
 * TEST-DESCR: is set correctly.
 */
static void
test_hmacid_sso_1_1(void)
{
	int result, fd, i;
	socklen_t len;
	int check=2;
	uint16_t ary[2];
	uint16_t ary2[100];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = SCTP_AUTH_HMAC_ID_SHA256;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	if (result < 0) {
		/* no sha256, retry with just sha1 */
		ary[0] = SCTP_AUTH_HMAC_ID_SHA1;
		check = 1;
		len = sizeof(uint16_t);
		result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
				    algo, len);
		if (result < 0) {
			close (fd);
			errx(-1, "%s", strerror(errno));
		}
	}
	memset(ary2, 0, sizeof(ary2));
	algo = (struct sctp_hmacalgo *)ary2;
	len = sizeof(ary2);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, &len);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	for (i=0; i<check; i++) {
		if(ary[i] != ary2[i]) {
			errx(-1, "Did not get back the expected list");
		}
	}
	
	if((len/sizeof(uint16_t)) != check) {
		errx(-1, "Did not get back the expected list - size wrong");
	}
	return;
}
/*
 * TEST-TITLE hmacid/sso_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set in to prefer sha256 then sha1. If
 * TEST-DESCR: that succeeds verify our settings. If it
 * TEST-DESCR: fails then set just sha1 and verify it
 * TEST-DESCR: is set correctly.
 */
static void
test_hmacid_sso_1_M(void)
{
	int result, fd, i;
	socklen_t len;
	int check=2;
	uint16_t ary[2];
	uint16_t ary2[100];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = SCTP_AUTH_HMAC_ID_SHA256;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	if (result < 0) {
		/* no sha256, retry with just sha1 */
		ary[0] = SCTP_AUTH_HMAC_ID_SHA1;
		check = 1;
		len = sizeof(uint16_t);
		result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
				    algo, len);
		if (result < 0) {
			close (fd);
			errx(-1, "%s", strerror(errno));
		}
	}
	memset(ary2, 0, sizeof(ary2));
	algo = (struct sctp_hmacalgo *)ary2;
	len = sizeof(ary2);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, &len);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	for (i=0; i<check; i++) {
		if(ary[i] != ary2[i]) {
			errx(-1, "Did not get back the expected list");
		}
	}
	
	if((len/sizeof(uint16_t)) != check) {
		errx(-1, "Did not get back the expected list - size wrong");
	}
	return;
}

/*
 * TEST-TITLE hmacid/sso_bad_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set in to prefer id 2960 (bogus) then sha1.
 * TEST-DESCR: Validate that the request is rejected.
 */
static void
test_hmacid_sso_bad_1_1(void)
{
	int result, fd;
	socklen_t len;
	uint16_t ary[2];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = 2960;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to set bogus hmac id 2960");
	}
	return;
}

/*
 * TEST-TITLE hmacid/sso_bad_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set in to prefer id 2960 (bogus) then sha1.
 * TEST-DESCR: Validate that the request is rejected.
 */
static void
test_hmacid_sso_bad_1_M(void)
{
	int result, fd;
	socklen_t len;
	uint16_t ary[2];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = 2960;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to set bogus hmac id 2960");
	}
	return;
}

/*
 * TEST-TITLE hmacid/sso_nosha1_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set to prefer only sha256 without 
 * TEST-DESCR: including sha1. The test should fail
 * TEST-DESCR: since sha1 is required to be in the list.
 */
static void
test_hmacid_sso_nosha1_1_1(void)
{
	int result, fd;
	socklen_t len;
	uint16_t ary[2];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = SCTP_AUTH_HMAC_ID_SHA256;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	if (result < 0) {
		/* no sha256, retry with just sha1 */
		close (fd);
		errx(-1, "Can't run test SHA256 not supported");
	}
	ary[1] = 0;
	len = sizeof(uint16_t);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	close (fd);
	if (result >= 0) {
		errx(-1, "Was allowed to set only SHA256");
	}
	return;

}

/*
 * TEST-TITLE hmacid/sso_nosha1_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set to prefer only sha256 without 
 * TEST-DESCR: including sha1. The test should fail
 * TEST-DESCR: since sha1 is required to be in the list.
 */
static void
test_hmacid_sso_nosha1_1_M(void)
{
	int result, fd;
	socklen_t len;
	uint16_t ary[2];
	struct sctp_hmacalgo *algo;
	algo = (struct sctp_hmacalgo *)ary;
	/* First try 256 */
	ary[0] = SCTP_AUTH_HMAC_ID_SHA256;
	ary[1] = SCTP_AUTH_HMAC_ID_SHA1;
	len = sizeof(ary);

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	if (result < 0) {
		/* no sha256, retry with just sha1 */
		close (fd);
		errx(-1, "Can't run test SHA256 not supported");
	}
	ary[1] = 0;
	len = sizeof(uint16_t);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_HMAC_IDENT,
			    algo, len);
	close (fd);
	if (result >= 0) {
		errx(-1, "Was allowed to set only SHA256");
	}
	return;
}

/********************************************************
 *
 * SCTP_AUTH_KEY tests
 *
 ********************************************************/
/* endpoint tests */
/*
 * TEST-TITLE authkey/gso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot set the default endpoint keynumber (0)
 * TEST-DESCR: using getsockopt.
 */
static void
test_authkey_gso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_get_auth_key(fd, 0, &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to get auth key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/gso_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot set the default endpoint keynumber (0)
 * TEST-DESCR: using getsockopt.
 */
static void
test_authkey_gso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_get_auth_key(fd, 0, &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to get auth key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/gso_new_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot get/set a new endpoint keynumber using getsockopt.
 */
static void
test_authkey_gso_new_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0x1234;
	result = sctp_get_auth_key(fd, 0, &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to get auth key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/gso_new_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot get/set a new endpoint keynumber using getsockopt.
 */
static void
test_authkey_gso_new_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0x1234;
	result = sctp_get_auth_key(fd, 0, &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to get auth key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can overwrite the endpoint default keynumber 0,
 * TEST-DESCR: which should have been the NULL key
 */
static void
test_authkey_sso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* overwrite the default key */
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can overwrite the endpoint default keynumber 0,
 * TEST-DESCR: which should have been the NULL key
 */
static void
test_authkey_sso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* overwrite the default key */
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_new_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can add a new endpoint keynumber 0xFFFF
 */
static void
test_authkey_sso_new_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key id */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_new_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new endpoint keynumber 0xFFFF
 */
static void
test_authkey_sso_new_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;	
	char *keytext = "This is my new key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key id */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_newnul_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can add a new endpoint NULL keynumber 0xFFFF
 */
static void
test_authkey_sso_newnul_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext = 0xff;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new NULL key id */
	keylen = 0;
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, &keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/*
 * TEST-TITLE authkey/sso_newnul_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new endpoint NULL keynumber 0xFFFF
 */
static void
test_authkey_sso_newnul_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	uint8_t keytext = 0xff;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new NULL key id */
	keylen = 0;
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, &keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	return;
}

/* assoc tests */
/*
 * TEST-TITLE authkey/gso_a_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot get the SCTP_AUTH_KEY option on an assoc
 */
static void
test_authkey_gso_a_def_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fds[0] = fds[1] = -1;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_get_auth_key(fds[1], 0, &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fd);
		close(fds[0]);
		close(fds[1]);
		errx(-1, "was able to get auth key");
	}
	close(fd);
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/gso_a_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot get the SCTP_AUTH_KEY option on an assoc
 */
static void
test_authkey_gso_a_def_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, keylen;
	uint8_t keytext[128];

	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_get_auth_key(fds[0], ids[0], &keyid, &keylen, keytext);
	if (result >= 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "was able to get auth key");
	}
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can overwrite the assoc default keynumber 0,
 * TEST-DESCR: which should have been the NULL key
 * TEST-DESCR: and inherited from the endpoint
 */
static void
test_authkey_sso_a_def_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, keylen;
	char *keytext = "This is my key";

	fds[0] = fds[1] = -1;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	/* overwrite the default key */
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_set_auth_key(fds[1], 0, keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can overwrite the assoc default keynumber 0,
 * TEST-DESCR: which should have been the NULL key
 * TEST-DESCR: and inherited from the endpoint
 */
static void
test_authkey_sso_a_def_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, keylen;
	char *keytext = "This is my key";

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* overwrite the default key */
	keylen = sizeof(keytext);
	keyid = 0;
	result = sctp_set_auth_key(fds[0], ids[0], keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_new_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can add a new assoc keynumber 0xFFFF
 */
static void
test_authkey_sso_a_new_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fds[0] = fds[1] = -1;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key id */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fds[1], 0, keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_new_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new assoc keynumber 0xFFFF
 */
static void
test_authkey_sso_a_new_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key id */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fds[0], ids[0], keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_newnul_1_1
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new assoc NULL keynumber 0xFFFF
 */
static void
test_authkey_sso_a_newnul_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, keylen;
	uint8_t keytext = 0xff;

	fds[0] = fds[1] = -1;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	/* add a new NULL key id */
	keylen = 0;
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fds[1], 0, keyid, keylen, &keytext);
	if (result < 0) {
		close(fd);
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fd);
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE authkey/sso_a_newnul_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new assoc NULL keynumber 0xFFFF
 */
static void
test_authkey_sso_a_newnul_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, keylen;
	uint8_t keytext = 0xff;

	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* add a new NULL key id */
	keylen = 0;
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fds[0], ids[0], keyid, keylen, &keytext);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "failed to set auth key");
	}
	/* No way to tell if it was really written ok */
	close(fds[0]);
	close(fds[1]);
	return;
}

/********************************************************
 *
 * SCTP_AUTH_ACTIVE_KEY tests
 *
 ********************************************************/
/*
 * TEST-TITLE actkey/gso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can get the default active endpoint keynumber
 * TEST-DESCR: which should be keynumber 0
 */
static void
test_actkey_gso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0xff;
	result = sctp_get_active_key(fd, 0, &keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (keyid != 0) {
		errx(-1, "default key not key 0");
	}
	return;
}

/*
 * TEST-TITLE actkey/gso_def_1_M
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can get the default active endpoint keynumber
 * TEST-DESCR: which should be keynumber 0
 */
static void
test_actkey_gso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0xff;
	result = sctp_get_active_key(fd, 0, &keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (keyid != 0) {
		errx(-1, "default key not key 0");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can set the default endpoint keynumber active again
 */
static void
test_actkey_sso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid, verify_keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0;
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set key active");
	}
	result = sctp_get_active_key(fd, 0, &verify_keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (verify_keyid != keyid) {
		errx(-1, "active key was not set");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can set the default endpoint keynumber active again
 */
static void
test_actkey_sso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid, verify_keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0;
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		errx(-1, "was unable to set key active");
	}
	result = sctp_get_active_key(fd, 0, &verify_keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (verify_keyid != keyid) {
		errx(-1, "active key was not set");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_inval_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot set an unknown keynumber to be active
 */
static void
test_actkey_sso_inval_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0x1234;
	result = sctp_set_active_key(fd, 0, keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to set unknown key active");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_inval_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot set an unknown keynumber to be active
 */
static void
test_actkey_sso_inval_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0x1234;
	result = sctp_set_active_key(fd, 0, keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to set unknown key active");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_new_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can add a new keynumber and set it active.
 * TEST-DESCR: Validates you can also get the new active keynumber.
 */
static void
test_actkey_sso_new_1_1(void)
{
	int fd, result;
	uint16_t keyid, verify_keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {	
		close(fd);
		errx(-1, "was unable to set new key active");
	}
	result = sctp_get_active_key(fd, 0, &verify_keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (verify_keyid != keyid) {
		errx(-1, "new active key was not set");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_new_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can add a new keynumber and set it active.
 * TEST-DESCR: Validates you can also get the new active keynumber.
 */
static void
test_actkey_sso_new_1_M(void)
{
	int fd, result;
	uint16_t keyid, verify_keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set new key active");
	}
	result = sctp_get_active_key(fd, 0, &verify_keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	if (verify_keyid != keyid) {
		errx(-1, "new active key was not set");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_inhdef_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: an assoc inherits the endpoint default active keynumber.
 */
static void
test_actkey_sso_inhdef_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, a_keyid;

	/* does the new assoc inherit the default active key from the ep? */
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_active_key(fd, 0, &keyid);
	if (result < 0) {
		errx(-1, "was unable to get active key");
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_active_key(fds[1], 0, &a_keyid);
	close(fd);
	if (result < 0) {
		errx(-1, "was unable to get assoc active key");
	}
	if (a_keyid != keyid) {
		errx(-1, "did not inherit from ep");
	}
	return;
}

/*
 * TEST-TITLE actkey/sso_inhdef_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: an assoc inherits the endpoint default active keynumber.
 */
static void
test_actkey_sso_inhdef_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, a_keyid;
	char *ret = NULL;

	/* does the new assoc inherit the default active key from the ep? */
	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_active_key(fds[0], 0, &keyid);
	if (result < 0) {
		errx(-1, "was unable to get ep active key");
	}

	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_active_key(fds[0], ids[0], &a_keyid);	
	if (result < 0) {
		ret = "was unable to get assoc active key";
		goto out;
	}
	if (a_keyid != keyid) {
		ret = "did not inherit default active key";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
	if (ret != NULL)
		errx(-1, "%s", ret);
}

/*
 * TEST-TITLE actkey/sso_inhnew_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: an assoc inherits the endpoint active keynumber.
 */
static void
test_actkey_sso_inhnew_1_1(void)
{
	int fd, fds[2], result;
	uint16_t keyid, a_keyid, keylen;
	char *keytext = "This is my new key";
	char *ret = NULL;

	/* does the new assoc inherit the new active key from the ep? */
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key to the ep */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set new key active");
	}
	/* create a new assoc */
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	/* verify the assoc inherits the ep active key */
	result = sctp_get_active_key(fds[1], 0, &a_keyid);
	if (result < 0) {
		ret = "was unable to get active key";
		goto out;
	}
	if (a_keyid != keyid) {
		ret = "new active key was not set";
		goto out;
	}
 out:
	close(fd);
	close(fds[0]);
	close(fds[1]);
	if (ret != NULL)
		errx(-1, "%s", ret);
}

/*
 * TEST-TITLE actkey/sso_inhnew_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: an assoc inherits the endpoint active keynumber.
 */
static void
test_actkey_sso_inhnew_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t keyid, a_keyid, keylen;
	char *keytext = "This is my new key";
	char *ret = NULL;

	/* does the new assoc inherit the new active key from the ep? */
	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key to the ep */
	keylen = sizeof(keytext);
	keyid = 0xFFFF;
	result = sctp_set_auth_key(fds[0], 0, keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "failed to set auth key");
	}
	result = sctp_set_active_key(fds[0], 0, keyid);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "was unable to set new key active");
	}
	/* create a new assoc */
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* verify the assoc inherits the ep active key */
	result = sctp_get_active_key(fds[0], ids[0], &a_keyid);
	if (result < 0) {
		ret = "was unable to get assoc active key";
		goto out;
	}
	if (a_keyid != keyid) {
		ret = "did not inherit default active key";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
	if (ret != NULL)
		errx(-1, "%s", ret);
}

/*
 * TEST-TITLE actkey/sso_achg_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: changing the assoc active keynumber leaves the ep active
 * TEST-DESCR: keynumber the same.
 */
static void
test_actkey_sso_achg_1_1(void)
{
	int fd, fds[2], result;
	uint16_t def_keyid, keyid, a_keyid, keylen;
	char *keytext = "This is my new key";
	char *ret = NULL;

	/* does changing the assoc active key leave the ep alone? */
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* get the default key */
	result = sctp_get_active_key(fd, 0, &def_keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to get default active key");
	}
	/* add a new key */
	keylen = sizeof(keytext);
	keyid = 1;
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "failed to set auth key");
	}
	/* create a new assoc */
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	/* get assoc's active key, should be default key */
	result = sctp_get_active_key(fds[1], 0, &a_keyid);
	if (result < 0) {
		ret = "was unable to get active key";
		goto out;
	}
	if (a_keyid != def_keyid) {
		ret = "new active key was not set";
		goto out;
	}
	/* set assoc's active key */
	result = sctp_set_active_key(fds[1], 0, keyid);
	if (result < 0) {
		ret = "was unable to set assoc active key";
		goto out;
	}
	result = sctp_get_active_key(fds[1], 0, &a_keyid);
	if (result < 0) {
		ret = "was unable to get active key";
		goto out;
	}
	if (a_keyid != keyid) {
		ret = "new active key was not set";
		goto out;
	}
	/* make sure ep active key didn't change */
	result = sctp_get_active_key(fd, 0, &keyid);
	if (result < 0) {
		ret = "was unable to get ep active key back";
		goto out;
	}
	if (keyid != def_keyid) {
		ret = "ep active key changed";
		goto out;
	}

 out:
	close(fd);
	close(fds[0]);
	close(fds[1]);
	if (ret != NULL)
		errx(-1, "%s", ret);
}

/*
 * TEST-TITLE actkey/sso_achg_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: changing the assoc active keynumber leaves the ep active
 * TEST-DESCR: keynumber the same.
 */
static void
test_actkey_sso_achg_1_M(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	uint16_t def_keyid, keyid, a_keyid, keylen;
	char *keytext = "This is my new key";
	char *ret = NULL;

	/* does changing the assoc active key leave the ep alone? */
	fds[0] = fds[1] = -1;
	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* get the default key */
	result = sctp_get_active_key(fds[0], 0, &def_keyid);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "was unable to geet default active key");
	}
	/* add a new key */
	keylen = sizeof(keytext);
	keyid = 1;
	result = sctp_set_auth_key(fds[0], 0, keyid, keylen,
				   (uint8_t *)keytext);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "failed to set auth key");
	}
	/* create a new assoc */
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		close(fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* get assoc's active key, should be default key */
	result = sctp_get_active_key(fds[0], ids[0], &a_keyid);
	if (result < 0) {
		ret = "was unable to get active key";
		goto out;
	}
	if (a_keyid != def_keyid) {
		ret = "new active key was not set";
		goto out;
	}
	/* set assoc's active key */
	result = sctp_set_active_key(fds[0], ids[0], keyid);
	if (result < 0) {
		ret = "was unable to set assoc active key";
		goto out;
	}
	result = sctp_get_active_key(fds[0], ids[0], &a_keyid);
	if (result < 0) {
		ret = "was unable to get active key";
		goto out;
	}
	if (a_keyid != keyid) {
		ret = "new active key was not set";
		goto out;
	}
	/* make sure ep active key didn't change */
	result = sctp_get_active_key(fds[0], 0, &keyid);
	if (result < 0) {
		ret = "was unable to get ep active key back";
		goto out;
	}
	if (keyid != def_keyid) {
		ret = "ep active key changed";
		goto out;
	}

 out:
	close(fds[0]);
	close(fds[1]);
	if (ret != NULL)
		errx(-1, "%s", ret);
}

/********************************************************
 *
 * SCTP_AUTH_DELETE_KEY tests
 *
 ********************************************************/
/*
 * NOTE: These tests assume SCTP_AUTH_KEY and SCTP_AUTH_ACTIVE_KEY socket
 * options are WORKING.  Any failure in AUTH_KEY or AUTH_ACTIVE_KEY will
 * likely fail additional tests in this suite.
 */

/*
 * TEST-TITLE delkey/gso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot delete the default endpoint key using getsockopt.
 */
static void
test_delkey_gso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0;
	result = sctp_get_delete_key(fd, 0, &keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to get delete key?");
	}
	return;
}

/*
 * TEST-TITLE delkey/gso_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot delete the default endpoint key using getsockopt.
 */
static void
test_delkey_gso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0;
	result = sctp_get_delete_key(fd, 0, &keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to get delete key?");
	}
	return;
}

/*
 * TEST-TITLE delkey/gso_inval_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot delete an unknown keynumber using getsockopt.
 */
static void
test_delkey_gso_inval_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0x1234;
	result = sctp_get_delete_key(fd, 0, &keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to get delete key?");
	}
	return;
}

/*
 * TEST-TITLE delkey/gso_inval_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot delete an unknown keynumber using getsockopt.
 */
static void
test_delkey_gso_inval_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	keyid = 0x1234;
	result = sctp_get_delete_key(fd, 0, &keyid);
	close(fd);
	if (result >= 0) {
		errx(-1, "was able to get delete key?");
	}
	return;
}

/*
 * TEST-TITLE delkey/sso_def_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot delete the default endpoint key because it is
 * TEST-DESCR: the current active keynumber.
 */
static void
test_delkey_sso_def_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* delete the default, active key */
	keyid = 0;
	result = sctp_get_delete_key(fd, 0, &keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete default active key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_def_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot delete the default endpoint key because it is
 * TEST-DESCR: the current active keynumber.
 */
static void
test_delkey_sso_def_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* delete the default, active key */
	keyid = 0;
	result = sctp_get_delete_key(fd, 0, &keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete default active key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_inval_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot delete a keynumber that has not been previously
 * TEST-DESCR: added using SCTP_AUTH_KEY.
 */
static void
test_delkey_sso_inval_1_1(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* delete a non-existant key */
	keyid = 1234;
	result = sctp_get_delete_key(fd, 0, &keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete non-existant key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_inval_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot delete a keynumber that has not been previously
 * TEST-DESCR: added using SCTP_AUTH_KEY.
 */
static void
test_delkey_sso_inval_1_M(void)
{
	int fd, result;
	uint16_t keyid;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* delete a non-existant key */
	keyid = 1234;
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete non-existant key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_new_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can delete a newly added keynumber that has not been
 * TEST-DESCR: made active. Note this tries to delete the deleted keynumber
 * TEST-DESCR: to make sure the delete actually occurred.
 */
static void
test_delkey_sso_new_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key */
	keyid = 1;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}

	/* delete the key */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to delete key");
	}
	/* delete again to make sure it's really gone */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to re-delete key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_new_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can delete a newly added keynumber that has not been
 * TEST-DESCR: made active. Note this tries to delete the deleted keynumber
 * TEST-DESCR: to make sure the delete actually occurred.
 */
static void
test_delkey_sso_new_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add a new key */
	keyid = 1;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}

	/* delete the key */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to delete key");
	}
	/* delete again to make sure it's really gone */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to re-delete key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_newact_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you cannot delete a newly added keynumber that has been
 * TEST-DESCR: made active.
 */
static void
test_delkey_sso_newact_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add and activate a new key */
	keyid = 0xFFFF;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set active key");
	}

	/* delete the key */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete an active key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_newact_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you cannot delete a newly added keynumber that has been
 * TEST-DESCR: made active.
 */
static void
test_delkey_sso_newact_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add and activate a new key */
	keyid = 1;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set active key");
	}

	/* delete the key */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to delete an active key");
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_zero_1_1
 * TEST-DESCR: Validates on a 1-1 model socket that
 * TEST-DESCR: you can delete the default endpoint keynumber 0 after a
 * TEST-DESCR: new keynumber that has been added to the endpoint.
 * TEST-DESCR: Note this tries to delete the deleted keynumber
 * TEST-DESCR: to make sure the delete actually occurred.
 */
static void
test_delkey_sso_zero_1_1(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add and activate a new key */
	keyid = 1;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set active key");
	}

	/* delete default key 0 */
	keyid = 0;
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to delete key");
	}
	/* delete again to make sure it's really gone */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to re-delete key");
	}

	close(fd);
	return;
}

/*
 * TEST-TITLE delkey/sso_zero_1_M
 * TEST-DESCR: Validates on a 1-many model socket that
 * TEST-DESCR: you can delete the default endpoint keynumber 0 after a
 * TEST-DESCR: new keynumber that has been added to the endpoint.
 * TEST-DESCR: Note this tries to delete the deleted keynumber
 * TEST-DESCR: to make sure the delete actually occurred.
 */
static void
test_delkey_sso_zero_1_M(void)
{
	int fd, result;
	uint16_t keyid, keylen;
	char *keytext = "This is my new key";

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	/* add and activate a new key */
	keyid = 1;
	keylen = sizeof(keytext);
	result = sctp_set_auth_key(fd, 0, keyid, keylen, (uint8_t *)keytext);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to add key");
	}
	result = sctp_set_active_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to set active key");
	}

	/* delete default key 0 */
	keyid = 0;
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result < 0) {
		close(fd);
		errx(-1, "was unable to delete key");
	}
	/* delete again to make sure it's really gone */
	result = sctp_set_delete_key(fd, 0, keyid);
	if (result >= 0) {
		close(fd);
		errx(-1, "was able to re-delete key");
	}
	close(fd);
	return;
}

/********************************************************
 *
 * SCTP_DELAYED_SACK tests
 *
 ********************************************************/

/*
 * TEST-TITLE dsack/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Validate that the default settings
 * TEST-DESCR: for delayed sack conform to RFC4960.
 */
static void
test_dsack_gso_def_1_1(void)
{
	uint32_t delay, freq;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay, &freq);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if ((delay < 200) || (delay > 500)) {
		errx(-1, "Delay non-compliant to RFC4960 - 200ms <= x <= 500ms");
	}
	if (freq != 2) {
		errx(-1, "Sack Freq non-compliant to RFC4960 its != 2");
	}
	return;
}

/*
 * TEST-TITLE dsack/gso_def_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Validate that the default settings
 * TEST-DESCR: for delayed sack conform to RFC4960.
 */
static void
test_dsack_gso_def_1_M(void)
{
	uint32_t delay, freq;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay, &freq);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if ((delay < 200) || (delay > 500)) {
		errx(-1, "Delay non-compliant to RFC4960 - 200ms <= x <= 500ms");
	}
	if (freq != 2) {
		errx(-1, "Sack Freq non-compliant to RFC4960 its != 2");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_delay_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set the delay to a new value.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_delay_1_1(void)
{
	uint32_t delay[2];
	uint32_t freq[2];
	uint32_t newval;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if( newval > 500 ) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[1], &freq[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != delay[1]) {
		errx(-1, "Could not change delay");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "Set of delay changed frequency");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_delay_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set the delay to a new value.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_delay_1_M(void)
{
	uint32_t delay[2];
	uint32_t freq[2];
	uint32_t newval;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if( newval > 500 ) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[1], &freq[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != delay[1]) {
		errx(-1, "Could not change delay");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "Set of delay changed frequency");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_freq_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set the freq to a new value.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_freq_1_1(void)
{
	uint32_t delay[2];
	uint32_t freq[2];
	uint32_t newval;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = freq[0]++;
	result = sctp_set_dfreq(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[1], &freq[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (delay[0] != delay[1]) {
		errx(-1, "Set of freq changed delay");
	}
	if (newval != freq[1]) {
		errx(-1, "Could not change freq");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_freq_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set the freq to a new value.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_freq_1_M(void)
{
	uint32_t delay[2], freq[2];
	uint32_t newval;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = freq[0]+1;

	result = sctp_set_dfreq(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[1], &freq[1]);
	close (fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (delay[0] != delay[1]) {
		errx(-1, "Set of frequency changed delay");
	}
	if (newval != freq[1]) {
		errx(-1, "Could not change frequency");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_asc_1_1
 * TEST-DESCR: On a 1-1 socket, create an association
 * TEST-DESCR: Set the delay to a new value for the assoc.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_asc_1_1(void)
{
	uint32_t delay[2], freq[2], newval;
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fds[1], 0, newval);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[1], 0, &delay[1], &freq[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if (newval != delay[1]) {
		errx(-1, "Could not set delay");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "freq changed on delay set");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_asc_1_M
 * TEST-DESCR: On a 1-M socket, create an association
 * TEST-DESCR: Set the delay to a new value for the assoc.
 * TEST-DESCR: Validate that it happens and
 * TEST-DESCR: no other values change.
 */
static void
test_dsack_sso_asc_1_M(void)
{
	uint32_t delay[2], freq[2], newval;
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], ids[0], &delay[1], &freq[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if (newval != delay[1]) {
		errx(-1, "Could not set delay");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "freq changed on delay set");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_inherit_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Set the delay to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value.
 */
static void
test_dsack_sso_inherit_1_1(void)
{
	uint32_t delay[2], freq[2], newval;
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[1], 0, &delay[1], &freq[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	if (newval != delay[1]) {
		errx(-1, "New delay did not inherit");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "freq changed on delay set");
	}

	return;
}

/*
 * TEST-TITLE dsack/sso_inherit_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the delay to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value.
 */
static void
test_dsack_sso_inherit_1_M(void)
{
	uint32_t delay[2], freq[2], newval;
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], ids[0], &delay[1], &freq[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if (newval != delay[1]) {
		errx(-1, "New delay did not inherit");
	}
	if (freq[0] != freq[1]) {
		errx(-1, "freq changed on delay set");
	}
	return;
}

/*
 * TEST-TITLE dsack/sso_inherit_ncep_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Set the delay to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value. Now
 * TEST-DESCR: change the assocation value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint.
 */
static void
test_dsack_sso_inherit_ncep_1_1(void)
{
	uint32_t delay[3], freq[3], newval;
	int fd, result;
	char *retstring = NULL;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fd, 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[1], 0, &delay[1], &freq[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (delay[1] != newval) {
		retstring = "Inheritance failed";
		goto out;
	}
	if (freq[0] != freq[1]) {
		retstring = "freq changed";
		goto out;
	}
	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_ddelay(fds[1], 0, newval);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	/* Now get the listener value, it should NOT have changed */
	result = sctp_get_dsack(fd, 0, &delay[2], &freq[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (delay[2] != delay[1]) {
		retstring = "Change of assoc, effected ep";
	}
 out:
 	close (fd);
 	close (fds[0]);
 	close (fds[1]);
 	if (retstring != NULL)
		errx(-1, "%s", retstring);

}

/*
 * TEST-TITLE dsack/sso_inherit_ncep_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the delay to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value. Now
 * TEST-DESCR: change the assocation value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint.
 */
static void
test_dsack_sso_inherit_ncep_1_M(void)
{
	uint32_t delay[3], freq[3], newval;
	int result;
	char *retstring = NULL;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_dsack(fds[0], ids[0], &delay[1], &freq[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (newval != delay[1]) {
		retstring = "Delay did not change";
		goto out;
	}
	if (freq[0] != freq[1]) {
		retstring =  "freq changed";
		goto out;
	}

	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_ddelay(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_dsack(fds[0], 0, &delay[2], &freq[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (delay[2] != delay[1]) {
		retstring = "Change of assoc, effected ep";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE dsack/sso_nc_other_asc_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the delay to a new value for the ep.
 * TEST-DESCR: create two associations validate that both
 * TEST-DESCR: association inherit the new value. Now
 * TEST-DESCR: change one assocation value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint or
 * TEST-DESCR: other association.
 */
static void
test_dsack_sso_nc_other_asc_1_M(void)
{
	uint32_t delay[3], freq[3], newval;
	int result;
	char *retstring = NULL;
	int fds[2];
	int fds2[2];
	sctp_assoc_t ids[2], ids2[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_dsack(fds[0], 0, &delay[0], &freq[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = delay[0] + 100;
	if (newval > 500) {
		newval = delay[0] - 100;
	}
	result = sctp_set_ddelay(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}


	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* Create a second assoc for fds[0] */
	fds2[0] = fds[0];
	result = sctp_socketpair_1tom(fds2, ids2,  1);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_dsack(fds[0], ids[0], &delay[1], &freq[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (newval != delay[1]) {
		retstring = "Did not change delay on asoc";
		goto out;
	}

	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_ddelay(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_dsack(fds[0], 0, &delay[2], &freq[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (delay[2] != delay[1]) {
		retstring = "Change of assoc, effected ep";
	}
	/* check other asoc */
	result = sctp_get_dsack(fds[0], ids2[0], &delay[2], &freq[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (delay[2] != delay[1]) {
		retstring = "Change of assoc, effected other assoc";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	close (fds2[1]);
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/********************************************************
 *
 * SCTP_FRAGMENT_INTERLEAVE tests
 *
 ********************************************************/
/*
 * TEST-TITLE fraginter/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Get the default fragment interleave and
 * TEST-DESCR: validate it is set to 1.
 */
static void
test_fraginter_gso_def_1_1(void)
{
	int inter;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_interleave(fd, &inter);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (inter != 1) {
		errx(-1, "Default fragment interleave not 1");
	}
	return;
}

/*
 * TEST-TITLE fraginter/gso_def_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Get the default fragment interleave and
 * TEST-DESCR: validate it is set to 1.
 */
static void
test_fraginter_gso_def_1_M(void)
{
	int inter;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_interleave(fd, &inter);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (inter != 1) {
		errx(-1, "Default fragment interleave not 1");
	}
	return;
}

/*
 * TEST-TITLE fraginter/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that we can set levels 0, 1 and 2
 */
static void
test_fraginter_sso_1_1(void)
{
	int inter[2],newval;
	int fd, result;
	int cnt, i;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	cnt = 0;
	for(cnt=0; cnt<2; cnt++) {
		for(i=0; i<3; i++) {
			newval = i;
			result = sctp_set_interleave(fd, newval);
			if(result < 0) {
				close(fd);
				errx(-1, "%s", strerror(errno));
			}
		
			result = sctp_get_interleave(fd, &inter[1]);
			if(result < 0) {
				close(fd);
				errx(-1, "%s", strerror(errno));
			} 
			if(inter[1] != newval) {
				close(fd);
				errx(-1, "failed to set fragment interleave");
			}

		}
	}
	close(fd);
	return;
}

/*
 * TEST-TITLE fraginter/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Validate that we can set levels 0, 1 and 2
 */
static void
test_fraginter_sso_1_M(void)
{
	int inter[2],newval;
	int fd, result;
	int cnt, i;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_interleave(fd, &inter[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	cnt = 0;
	for(cnt=0; cnt<2; cnt++) {
		for(i=0; i<3; i++) {
			newval = i;
			result = sctp_set_interleave(fd, newval);
			if(result < 0) {
				close(fd);
				errx(-1, "%s", strerror(errno));
			}
		
			result = sctp_get_interleave(fd, &inter[1]);
			if(result < 0) {
				close(fd);
				errx(-1, "%s", strerror(errno));
			} 
			if(inter[1] != newval) {
				close(fd);
				errx(-1, "failed to set fragment interleave");
			}

		}
	}
	close(fd);
	return;
}


/*
 * TEST-TITLE fraginter/sso_bad_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that we fail to set a
 * TEST-DESCR: bad fragment interleave level (42).
 */
static void
test_fraginter_sso_bad_1_1(void)
{
	int inter[2],newval;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_interleave(fd, &inter[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	} 
	newval = 42;
	result = sctp_set_interleave(fd, newval);
	if(result < 0) {
		close(fd);
		return;
	}
	result = sctp_get_interleave(fd, &inter[1]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	} 
	if(inter[1] != inter[0]) {
		close(fd);
		errx(-1, "bogus set changed interleave value");
	}
	close(fd);	
	return;
}

/*
 * TEST-TITLE fraginter/sso_bad_1_M
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that we fail to set a
 * TEST-DESCR: bad fragment interleave level (42).
 */
static void
test_fraginter_sso_bad_1_M(void)
{
	int inter[2],newval;
	int fd, result;

	fd = sctp_one2many(0,1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_interleave(fd, &inter[0]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	} 
	newval = 42;
	result = sctp_set_interleave(fd, newval);
	if(result < 0) {
		close(fd);
		return;
	}
	result = sctp_get_interleave(fd, &inter[1]);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	} 
	if(inter[1] != inter[0]) {
		close(fd);
		errx(-1, "bogus set changed interleave value");
	}
	close(fd);	
	return;
}

/********************************************************
 *
 * SCTP_PARTIAL_DELIVERY_POINT tests
 *
 ********************************************************/
/*
 * TEST-TITLE paapi/gso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that we can retrieve
 * TEST-DESCR: the pd-api point.
 */
static void
test_pdapi_gso_1_1(void)
{
	int point;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;
}

/*
 * TEST-TITLE paapi/gso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Validate that we can retrieve
 * TEST-DESCR: the pd-api point.
 */
static void
test_pdapi_gso_1_M(void)
{
	int point;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	close(fd);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	return;

}

/*
 * TEST-TITLE paapi/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that we can set
 * TEST-DESCR: the pd-api point.
 */
static void
test_padapi_sso_1_1(void)
{
	int point, newval;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if (point > 100 ){
		newval = point/2;
	} else {
		newval = point *2;
	}
	result = sctp_set_pdapi_point(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(newval != point) {
		errx(-1, "Could not set pdapi point");
	}
	return;

}

/*
 * TEST-TITLE paapi/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Validate that we can set
 * TEST-DESCR: the pd-api point.
 */
static void
test_pdapi_sso_1_M(void)
{
	int point, newval;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if (point > 100 ){
		newval = point/2;
	} else {
		newval = point *2;
	}
	result = sctp_set_pdapi_point(fd, newval);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_pdapi_point(fd, &point);
	if(result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	close(fd);
	if(newval != point) {
		errx(-1, "Could not set pdapi point");
	}
	return;

}

/********************************************************
 *
 * SCTP_USE_EXT_RCVINFO tests
 *
 ********************************************************/
/*
 * TEST-TITLE xrcvinfo/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that you can get and set
 * TEST-DESCR: the SCTP_USE_EXT_RECVINFO flags.
 */
static void
test_xrcvinfo_sso_1_1(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of ext_rcvinfo failed");
	}
	return;
	
}
/*
 * TEST-TITLE xrcvinfo/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Validate that you can get and set
 * TEST-DESCR: the SCTP_USE_EXT_RECVINFO flags.
 */
static void
test_xrcvinfo_sso_1_M(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of ext_rcvinfo failed");
	}
	return;

}
/********************************************************
 *
 * SCTP_AUTO_ASCONF tests
 *
 ********************************************************/

/*
 * TEST-TITLE aasconf/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Validate that the auto-asconf 
 * TEST-DESCR: option can be retrieved and changed.
 */
static void
test_aasconf_sso_1_1(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of auto-asconf failed");
	}
	return;
	
}

/*
 * TEST-TITLE aasconf/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Validate that the auto-asconf 
 * TEST-DESCR: option can be retrieved and changed.
 */
static void
test_aasconf_sso_1_M(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_AUTO_ASCONF,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of auto-asconf failed");
	}
	return;
}


/********************************************************
 *
 * SCTP_MAX_BURST tests
 *
 ********************************************************/
/*
 * TEST-TITLE maxburst/gso_def_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Retrieve the max burst setting and
 * TEST-DESCR: validate it conforms to RFC4960.
 */
static void
test_maxburst_gso_def_1_1(void)
{
	uint8_t val=0;
	int fd, result;
	socklen_t len;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &val, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (val != 4) {
		errx(-1, "Max burst not compliant to RFC4960 - 4");
	}
	return;
}

/*
 * TEST-TITLE maxburst/gso_def_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Retrieve the max burst setting and
 * TEST-DESCR: validate it conforms to RFC4960.
 */
static void
test_maxburst_gso_def_1_M(void)
{
	uint8_t val=0;
	int fd, result;
	socklen_t len;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &val, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (val != 4) {
		errx(-1, "Max burst not compliant to RFC4960 - 4");
	}
	return;
}

/*
 * TEST-TITLE maxburst/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Retrieve the max burst lower this
 * TEST-DESCR: by one, and then set it. Validate
 * TEST-DESCR: we can set it.
 */
static void
test_maxburst_sso_1_1(void)
{
	uint8_t val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val >= 2)
		newval = val -1;
	else
		newval = val +1;

	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of max-burst failed");
	}
	return;
	
}

/*
 * TEST-TITLE maxburst/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Retrieve the max burst lower this
 * TEST-DESCR: by one, and then set it. Validate
 * TEST-DESCR: we can set it.
 */
static void
test_maxburst_sso_1_M(void)
{
	uint8_t val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val >= 2)
		newval = val -1;
	else
		newval = val +1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_MAX_BURST,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of max-burst failed");
	}
	return;
}

/********************************************************
 *
 * SCTP_CONTEXT tests
 *
 ********************************************************/

/*
 * TEST-TITLE context/sso_1_1
 * TEST-DESCR: On a 1-1 socket.
 * TEST-DESCR: Set the context to a new value.
 * TEST-DESCR: Validate that it is set.
 */
static void
test_context_sso_1_1(void)
{
	uint32_t val, newval, finalval;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &val);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val == 0)
		newval = 4960;
	else
		newval = 0;

	result = sctp_set_context(fd, 0, newval);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &finalval);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of context failed");
	}
	return;
	
}

/*
 * TEST-TITLE context/sso_1_M
 * TEST-DESCR: On a 1-M socket.
 * TEST-DESCR: Set the context to a new value.
 * TEST-DESCR: Validate that it is set.
 */
static void
test_context_sso_1_M(void)
{
	uint32_t val, newval, finalval;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &val);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val == 0)
		newval = 4960;
	else
		newval = 0;

	result = sctp_set_context(fd, 0, newval);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &finalval);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of context failed");
	}
	return;
	
}


/*
 * TEST-TITLE context/sso_asc_1_1
 * TEST-DESCR: On a 1-1 socket, create an association
 * TEST-DESCR: Set the context to a new value for the assoc.
 * TEST-DESCR: Validate that it happens 
 */
static void
test_context_sso_asc_1_1(void)
{
	uint32_t val[2], newval;
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fds[1], 0, newval);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if (newval != val[1]) {
		errx(-1, "Could not set context");
	}
	return;
}

/*
 * TEST-TITLE context/sso_asc_1_M
 * TEST-DESCR: On a 1-M socket, create an association
 * TEST-DESCR: Set the context to a new value for the assoc.
 * TEST-DESCR: Validate that it happens 
 */
static void
test_context_sso_asc_1_M(void)
{
	uint32_t val[2],newval;
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if (newval != val[1]) {
		errx(-1, "Could not set context");
	}
	return;
}

/*
 * TEST-TITLE context/sso_inherit_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Set the context to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value.
 */
static void
test_context_sso_inherit_1_1(void)
{
	uint32_t val[2], newval;
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	if (newval != val[1]) {
		errx(-1, "New context did not inherit");
	}
	return;
}

/*
 * TEST-TITLE context/sso_inherit_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the context to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value.
 */
static void
test_context_sso_inherit_1_M(void)
{
	uint32_t val[2], newval;
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	if (newval != val[1]) {
		errx(-1, "New context did not inherit");
	}
	return;
}

/*
 * TEST-TITLE context/sso_inherit_ncep_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Set the context to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value. Now
 * TEST-DESCR: change the assocation value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint.
 */
static void
test_context_sso_inherit_ncep_1_1(void)
{
	uint32_t val[3], newval;
	int fd, result;
	char *retstring = NULL;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fd, 0, &val[0]);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fd, 0, newval);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[1], 0, &val[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (val[1] != newval) {
		retstring = "Inheritance failed";
		goto out;
	}
	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_context(fds[1], 0, newval);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	/* Now get the listener value, it should NOT have changed */
	result = sctp_get_context(fd, 0, &val[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected ep";
	}
 out:
 	close (fd);
 	close (fds[0]);
 	close (fds[1]);
	
	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE context/sso_inherit_ncep_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the context to a new value for the ep.
 * TEST-DESCR: create an association validate that the
 * TEST-DESCR: association inherits the new value. Now
 * TEST-DESCR: change the assocation value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint.
 */
static void
test_context_sso_inherit_ncep_1_M(void)
{
	uint32_t val[3], newval;
	int result;
	char *retstring = NULL;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_context(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (newval != val[1]) {
		retstring = "Context did not change";
		goto out;
	}

	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_context(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_context(fds[0], 0, &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected ep";
	}
 out:
	close (fds[0]);
	close (fds[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}

/*
 * TEST-TITLE context/sso_nc_other_asc_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Set the context to a new value for the ep.
 * TEST-DESCR: create two associations validate that both
 * TEST-DESCR: association inherit the new value. Now
 * TEST-DESCR: change one assocations value, and validate
 * TEST-DESCR: the new value is set but not in the endpoint or
 * TEST-DESCR: other association.
 */
static void
test_context_sso_nc_other_asc_1_M(void)
{
	uint32_t val[3], newval;
	int result;
	char *retstring = NULL;
	int fds[2];
	int fds2[2];
	sctp_assoc_t ids[2], ids2[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		errx(-1, "%s", strerror(errno));
	}
	result = sctp_get_context(fds[0], 0, &val[0]);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	newval = val[0] + 100;
	result = sctp_set_context(fds[0], 0, newval);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		errx(-1, "%s", strerror(errno));
	}
	/* Create a second assoc for fds[0] */
	fds2[0] = fds[0];
	result = sctp_socketpair_1tom(fds2, ids2,  1);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_context(fds[0], ids[0], &val[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	
	if (newval != val[1]) {
		retstring = "Did not change context on asoc";
		goto out;
	}

	/* Change the assoc value */
	newval -= 50;
	result = sctp_set_context(fds[0], ids[0], newval);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	result = sctp_get_context(fds[0], 0, &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}

	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected ep";
	}
	/* check other asoc */
	result = sctp_get_context(fds[0], ids2[0], &val[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		errx(-1, "%s", strerror(errno));
	}
	if (val[2] != val[1]) {
		retstring = "Change of assoc, effected other assoc";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	close (fds2[1]);

	if (retstring != NULL)
		errx(-1, "%s", retstring);
}


/********************************************************
 *
 * SCTP_EXPLICIT_EOR tests
 *
 ********************************************************/
/*
 * TEST-TITLE eeor/sso_1_1
 * TEST-DESCR: On a 1-1 socket. 
 * TEST-DESCR: Get the eeor mode setting, Change it
 * TEST-DESCR: and validate the change occurs.
 */
static void
test_eeor_sso_1_1(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of auto-asconf failed");
	}
	return;
	
}

/*
 * TEST-TITLE eeor/sso_1_M
 * TEST-DESCR: On a 1-M socket. 
 * TEST-DESCR: Get the eeor mode setting, Change it
 * TEST-DESCR: and validate the change occurs.
 */
static void
test_eeor_sso_1_M(void)
{
	int val, newval, finalval;
	int fd, result;
	socklen_t len;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(val);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &val, &len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	if(val)
		newval = 0;
	else
		newval = 1;
	len = sizeof(newval);
	result = setsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &newval, len);
	if (result < 0) {
		close(fd);
		errx(-1, "%s", strerror(errno));
	}
	len = sizeof(finalval);
	result = getsockopt(fd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR,
			    &finalval, &len);
	close(fd);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if (newval != finalval) {
		errx(-1, "Set of auto-asconf failed");
	}
	return;
}

/********************************************************
 *
 * Read only options on associations.
 *
 ********************************************************/
/*
 * TEST-TITLE read/status
 * TEST-DESCR: Setup an association on a 1-M socket
 * TEST-DESCR: and get the SCTP_STATUS, validate 
 * TEST-DESCR: there is no error.
 */
static void
test_read_status(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	struct sctp_status stat;
	socklen_t len;

	fds[0] = fds[1] = -1;
	memset(&stat, 0, sizeof(stat));
	result = sctp_socketpair_1tom(fds, ids, 1);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	stat.sstat_assoc_id = ids[0];
	len = sizeof(stat);
	result = getsockopt(fds[0], IPPROTO_SCTP, SCTP_STATUS,
			    &stat, &len);
	close(fds[0]);
	close(fds[1]);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(len != sizeof(stat)) {
		errx(-1, "Did not get back a full stat structure");
	} 
	return;
}

/*
 * TEST-TITLE read/status
 * TEST-DESCR: Setup an association on a 1-M socket
 * TEST-DESCR: and get the SCTP_STATUS, use the status
 * TEST-DESCR: to get the paddrinfo of the primary address.
 */
static void
test_read_paddrinfo(void)
{
	int fds[2], result;
	sctp_assoc_t ids[2];
	struct sctp_status stat;
	struct sctp_paddrinfo pa;
	socklen_t len;

	fds[0] = fds[1] = -1;
	memset(&stat, 0, sizeof(stat));
	result = sctp_socketpair_1tom(fds, ids, 1);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	stat.sstat_assoc_id = ids[0];
	len = sizeof(stat);
	result = getsockopt(fds[0], IPPROTO_SCTP, SCTP_STATUS,
			    &stat, &len);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	memcpy(&pa, &stat.sstat_primary, sizeof(pa));
	pa.spinfo_assoc_id = ids[0];
	len = sizeof(pa);
	result = getsockopt(fds[0], IPPROTO_SCTP, SCTP_GET_PEER_ADDR_INFO,
			    &pa, &len);
	close(fds[0]);
	close(fds[1]);
	if (result < 0) {
		errx(-1, "%s", strerror(errno));
	}
	if(len != sizeof(pa)) {
		errx(-1, "Did not get back a full structure");
	} 
	return;
}

/*
 * TEST-TITLE read/auth_p_chklist
 * TEST-DESCR: Setup an association on a 1-M socket
 * TEST-DESCR: and get the peer authentication chunk lists. Validate
 * TEST-DESCR: that asconf and asconf-ack are in the list. Note
 * TEST-DESCR: that this test will fail if the peer does not support
 * TEST-DESCR: the add-ip extension.
 */
static void
test_read_auth_p_chklist(void)
{
	int fds[2], result, i, j;
	sctp_assoc_t ids[2];
	uint8_t asconf=0, asconf_ack=0;
	uint8_t buffer[260];
	struct sctp_authchunks *auth;
	socklen_t len;
	int cnt = 0;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
 try_again:
	memset(buffer, 0, sizeof(buffer));
	auth = (struct sctp_authchunks *)buffer;
	auth->gauth_assoc_id = ids[0];
	len = sizeof(buffer);
	result = getsockopt(fds[0], IPPROTO_SCTP, SCTP_PEER_AUTH_CHUNKS,
			    auth, &len);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	j = len - sizeof(sctp_assoc_t);
	if(j > 260)
		j = 256;

	for (i=0; i<j; i++) {
		if(auth->gauth_chunks[i] == SCTP_ASCONF) {
			asconf = 1;
		}
		if(auth->gauth_chunks[i] == SCTP_ASCONF_ACK) {
			asconf_ack = 1;
		}
	}
	if ((asconf_ack == 0) || (asconf == 0)) {
		if (cnt < 1) {
			cnt++;
			sctp_delay(SCTP_SLEEP_MS);
			goto try_again;
		}
		close(fds[0]);
		close(fds[1]);
		errx(-1, "Did not see ASCONF/ASCONF-ACK in list");
	}
	close(fds[0]);
	close(fds[1]);
	return;
}

/*
 * TEST-TITLE read/auth_p_chklist
 * TEST-DESCR: Setup an association on a 1-M socket
 * TEST-DESCR: and get the local authentication chunk lists. Validate
 * TEST-DESCR: that asconf and asconf-ack are in the list.
 */
static void
test_read_auth_l_chklist(void)
{
	int fds[2], result, i,j;
	sctp_assoc_t ids[2];
	uint8_t buffer[260];
	uint8_t asconf=0, asconf_ack=0;
	struct sctp_authchunks *auth;
	socklen_t len;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if(result < 0) {
		errx(-1, "%s", strerror(errno));		
	}
	memset(buffer, 0, sizeof(buffer));
	auth = (struct sctp_authchunks *)buffer;
	auth->gauth_assoc_id = ids[0];
	len = sizeof(buffer);
	result = getsockopt(fds[0], IPPROTO_SCTP, SCTP_LOCAL_AUTH_CHUNKS,
			    auth, &len);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		errx(-1, "%s", strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	j = len - sizeof(sctp_assoc_t);
	if(j > 260)
		j = 256;

	for (i=0; i<j; i++) {
		if(auth->gauth_chunks[i] == SCTP_ASCONF) {
			asconf = 1;
		}
		if(auth->gauth_chunks[i] == SCTP_ASCONF_ACK) {
			asconf_ack = 1;
		}
	}
	if ((asconf_ack == 0) || (asconf == 0)) {
		errx(-1, "Did not see ASCONF/ASCONF-ACK in list");
	}
	return;
}

int
main(int argc, char *argv[])
{
	test_rtoinfo_gso_1_1_defaults();
	test_rtoinfo_gso_1_M_defaults();
	test_rtoinfo_gso_1_1_bad_id();
	test_rtoinfo_gso_1_M_bad_id();
	test_rtoinfo_sso_1_1_good();
	test_rtoinfo_sso_1_M_good();
	test_rtoinfo_sso_1_1_bad_id();
	test_rtoinfo_sso_1_M_bad_id();
	test_rtoinfo_sso_1_1_init();
	test_rtoinfo_sso_1_M_init();
	test_rtoinfo_sso_1_1_max();
	test_rtoinfo_sso_1_M_max();
	test_rtoinfo_sso_1_1_min();
	test_rtoinfo_sso_1_M_min();
	test_rtoinfo_sso_1_1_same();
	test_rtoinfo_sso_1_M_same();
	test_rtoinfo_sso_ill_1();
	test_rtoinfo_sso_ill_2();
	test_rtoinfo_sso_ill_3();
	test_rtoinfo_sso_ill_4();
	test_rtoinfo_sso_ill_5();
	test_rtoinfo_sso_ill_6();
	test_rtoinfo_gso_1_1_c_bad_id();
	test_rtoinfo_sso_1_1_c_bad_id();
	test_rtoinfo_sso_1_1_inherit();
	test_rtoinfo_sso_1_M_inherit();
	test_assoclist_gso_numbers_zero();
	test_assoclist_gso_numbers_pos();
	test_assoclist_gso_ids_no_assoc();
	test_assoclist_gso_ids_buf_fit();
	test_assoclist_gso_ids_buf_large();
	test_assoclist_gso_ids_buf_small();
	test_associnfo_gso_1_1_defaults();
	test_associnfo_gso_1_M_defaults();
	test_associnfo_sso_rxt_1_1();
	test_associnfo_sso_rxt_1_M();
	test_associnfo_sso_rxt_asoc_1_1();
	test_associnfo_sso_rxt_asoc_1_M();
	test_associnfo_sso_rxt_asoc_1_1_inherit();
	test_associnfo_sso_rxt_asoc_1_M_inherit();
	test_associnfo_sso_clife_1_1();
	test_associnfo_sso_clife_1_M();
	test_associnfo_sso_clife_asoc_1_1();
	test_associnfo_sso_clife_asoc_1_M();
	test_associnfo_sso_clife_asoc_1_1_inherit();
	test_associnfo_sso_clife_asoc_1_M_inherit();
	test_associnfo_sso_lrwnd_ep_1_1();
	test_associnfo_sso_lrwnd_ep_1_M();
	test_associnfo_sso_lrwnd_asoc_1_1();
	test_associnfo_sso_lrwnd_asoc_1_M();
	test_associnfo_sso_prwnd_asoc_1_1();
	test_associnfo_sso_prwnd_asoc_1_M();
	test_associnfo_sso_pdest_asoc_1_1();
	test_associnfo_sso_pdest_asoc_1_M();
	test_initmsg_gso_1_1_defaults();
	test_initmsg_gso_1_M_defaults();
	test_initmsg_gso_1_1_set_ostrm();
	test_initmsg_gso_1_1_set_istrm();
	test_initmsg_gso_1_1_set_max();
	test_initmsg_gso_1_1_set_timeo();
	test_initmsg_gso_1_M_set_ostrm();
	test_initmsg_gso_1_M_set_istrm();
	test_initmsg_gso_1_M_set_max();
	test_initmsg_gso_1_M_set_timeo();
	test_nodelay_gso_1_1_def_ndelay();
	test_nodelay_gso_1_M_def_ndelay();
	test_nodelay_gso_1_1_set_ndelay();
	test_nodelay_gso_1_M_set_ndelay();
	test_autoclose_gso_1_1_def_autoclose();
	test_autoclose_gso_1_M_def_autoclose();
	test_autoclose_gso_1_1_set_autoclose();
	test_autoclose_gso_1_M_set_autoclose();
	test_setpeerprim_sso_1_1_good_peerprim();
	test_setpeerprim_sso_1_1_bad_peerprim();
	test_setpeerprim_sso_1_M_good_peerprim();
	test_setpeerprim_sso_1_M_bad_peerprim();
	test_setprim_gso_1_1_get_prim();
	test_setprim_gso_1_M_get_prim();
	test_setprim_sso_1_1_set_prim();
	test_setprim_sso_1_M_set_prim();
	test_setprim_sso_1_1_bad_prim();
	test_setprim_sso_1_M_bad_prim();
	test_adaptation_gso_1_1();
	test_adaptation_gso_1_M();
	test_adaptation_sso_1_1();
	test_adaptation_sso_1_M();
	test_disfrag_gso_def_1_1();
	test_disfrag_gso_def_1_M();
	test_disfrag_sso_1_1();
	test_disfrag_sso_1_M();
	test_paddrpara_gso_def_1_1();
	test_paddrpara_gso_def_1_M();
	test_paddrpara_sso_hb_int_1_1();
	test_paddrpara_sso_hb_int_1_M();
	test_paddrpara_sso_hb_zero_1_1();
	test_paddrpara_sso_hb_zero_1_M();
	test_paddrpara_sso_hb_off_1_1();
	test_paddrpara_sso_hb_off_1_M();
	test_paddrpara_sso_hb_on_1_1();
	test_paddrpara_sso_hb_on_1_M();
	test_paddrpara_sso_pmrxt_int_1_1();
	test_paddrpara_sso_pmrxt_int_1_M();
	test_paddrpara_sso_bad_hb_en_1_1();
	test_paddrpara_sso_bad_hb_en_1_M();
	test_paddrpara_sso_bad_pmtud_en_1_1();
	test_paddrpara_sso_bad_pmtud_en_1_M();
	test_paddrpara_sso_ahb_int_1_1();
	test_paddrpara_sso_ahb_int_1_M();
	test_paddrpara_sso_ahb_zero_1_1();
	test_paddrpara_sso_ahb_zero_1_M();
	test_paddrpara_sso_ahb_off_1_1();
	test_paddrpara_sso_ahb_off_1_M();
	test_paddrpara_sso_ahb_on_1_1();
	test_paddrpara_sso_ahb_on_1_M();
	test_paddrpara_sso_apmrxt_int_1_1();
	test_paddrpara_sso_apmrxt_int_1_M();
	test_paddrpara_sso_apmtu_dis_1_1();
	test_paddrpara_sso_apmtu_dis_1_M();
	test_paddrpara_sso_av6_flo_1_1();
	test_paddrpara_sso_av6_flo_1_M();
	test_paddrpara_sso_av4_tos_1_1();
	test_paddrpara_sso_av4_tos_1_M();
	test_paddrpara_sso_ainhhb_int_1_1();
	test_paddrpara_sso_ainhhb_int_1_M();
	test_paddrpara_sso_ainhhb_zero_1_1();
	test_paddrpara_sso_ainhhb_zero_1_M();
	test_paddrpara_sso_ainhhb_off_1_1();
	test_paddrpara_sso_ainhhb_off_1_M();
	test_paddrpara_sso_ainhpmrxt_int_1_1();
	test_paddrpara_sso_ainhpmrxt_int_1_M();
	test_paddrpara_sso_dhb_int_1_1();
	test_paddrpara_sso_dhb_int_1_M();
	test_paddrpara_sso_dhb_zero_1_1();
	test_paddrpara_sso_dhb_zero_1_M();
	test_paddrpara_sso_dhb_off_1_1();
	test_paddrpara_sso_dhb_off_1_M();
	test_paddrpara_sso_dpmrxt_int_1_1();
	test_paddrpara_sso_dpmrxt_int_1_M();
	test_paddrpara_sso_dav4_tos_1_1();
	test_paddrpara_sso_dav4_tos_1_M();
	test_paddrpara_sso_hb_demand_1_1();
	test_paddrpara_sso_hb_demand_1_M();
	test_defsend_gso_def_1_1();
	test_defsend_gso_def_1_M();
	test_defsend_sso_on_1_1();
	test_defsend_sso_on_1_M();
	test_defsend_sso_asc_1_1();
	test_defsend_sso_asc_1_M();
	test_defsend_sso_inherit_1_1();
	test_defsend_sso_inherit_1_M();
	test_defsend_sso_inherit_ncep_1_1();
	test_defsend_sso_inherit_ncep_1_M();
	test_defsend_sso_nc_other_asc_1_M();
	test_events_gso_def_1_1();
	test_events_gso_def_1_M();
	test_events_sso_1_1();
	test_events_sso_1_M();
	test_mapped_gso_1_1_def();
	test_mapped_gso_1_M_def();
	test_mapped_sso_1_1();
	test_mapped_sso_1_M();
	test_mapped_sso_bad_1_1();
	test_mapped_sso_bad_1_M();
	test_maxseg_gso_def_1_1();
	test_maxseg_sso_set_1_1();
	test_maxseg_gso_def_1_M();
	test_maxseg_sso_set_1_M();
	test_maxseg_sso_asc_1_1();
	test_maxseg_sso_asc_1_M();
	test_maxseg_sso_inherit_1_1();
	test_maxseg_sso_inherit_1_M();
	test_maxseg_sso_inherit_ncep_1_1();
	test_maxseg_sso_inherit_ncep_1_M();
	test_maxseg_sso_nc_other_asc_1_M();
	test_authchk_gso_1_1();
	test_authchk_gso_1_M();
	test_authchk_sso_1_1();
	test_authchk_sso_1_M();
	test_hmacid_sso_1_1();
	test_hmacid_sso_1_M();
	test_hmacid_sso_bad_1_1();
	test_hmacid_sso_bad_1_M();
	test_hmacid_sso_nosha1_1_1();
	test_hmacid_sso_nosha1_1_M();
	test_authkey_gso_def_1_1();
	test_authkey_gso_def_1_M();
	test_authkey_gso_new_1_1();
	test_authkey_gso_new_1_M();
	test_authkey_sso_def_1_1();
	test_authkey_sso_def_1_M();
	test_authkey_sso_new_1_1();
	test_authkey_sso_new_1_M();
	test_authkey_sso_newnul_1_1();
	test_authkey_sso_newnul_1_M();
	test_authkey_gso_a_def_1_1();
	test_authkey_gso_a_def_1_M();
	test_authkey_sso_a_def_1_1();
	test_authkey_sso_a_def_1_M();
	test_authkey_sso_a_new_1_1();
	test_authkey_sso_a_new_1_M();
	test_authkey_sso_a_newnul_1_1();
	test_authkey_sso_a_newnul_1_M();
	test_actkey_gso_def_1_1();
	test_actkey_gso_def_1_M();
	test_actkey_sso_def_1_1();
	test_actkey_sso_def_1_M();
	test_actkey_sso_inval_1_1();
	test_actkey_sso_inval_1_M();
	test_actkey_sso_new_1_1();
	test_actkey_sso_new_1_M();
	test_actkey_sso_inhdef_1_1();
	test_actkey_sso_inhdef_1_M();
	test_actkey_sso_inhnew_1_1();
	test_actkey_sso_inhnew_1_M();
	test_actkey_sso_achg_1_1();
	test_actkey_sso_achg_1_M();
	test_delkey_gso_def_1_1();
	test_delkey_gso_def_1_M();
	test_delkey_gso_inval_1_1();
	test_delkey_gso_inval_1_M();
	test_delkey_sso_def_1_1();
	test_delkey_sso_def_1_M();
	test_delkey_sso_inval_1_1();
	test_delkey_sso_inval_1_M();
	test_delkey_sso_new_1_1();
	test_delkey_sso_new_1_M();
	test_delkey_sso_newact_1_1();
	test_delkey_sso_newact_1_M();
	test_delkey_sso_zero_1_1();
	test_delkey_sso_zero_1_M();
	test_dsack_gso_def_1_1();
	test_dsack_gso_def_1_M();
	test_dsack_sso_delay_1_1();
	test_dsack_sso_delay_1_M();
	test_dsack_sso_freq_1_1();
	test_dsack_sso_freq_1_M();
	test_dsack_sso_asc_1_1();
	test_dsack_sso_asc_1_M();
	test_dsack_sso_inherit_1_1();
	test_dsack_sso_inherit_1_M();
	test_dsack_sso_inherit_ncep_1_1();
	test_dsack_sso_inherit_ncep_1_M();
	test_dsack_sso_nc_other_asc_1_M();
	test_fraginter_gso_def_1_1();
	test_fraginter_gso_def_1_M();
	test_fraginter_sso_1_1();
	test_fraginter_sso_1_M();
	test_fraginter_sso_bad_1_1();
	test_fraginter_sso_bad_1_M();
	test_pdapi_gso_1_1();
	test_pdapi_gso_1_M();
	test_padapi_sso_1_1();
	test_pdapi_sso_1_M();
	test_xrcvinfo_sso_1_1();
	test_xrcvinfo_sso_1_M();
	test_aasconf_sso_1_1();
	test_aasconf_sso_1_M();
	test_maxburst_gso_def_1_1();
	test_maxburst_gso_def_1_M();
	test_maxburst_sso_1_1();
	test_maxburst_sso_1_M();
	test_context_sso_1_1();
	test_context_sso_1_M();
	test_context_sso_asc_1_1();
	test_context_sso_asc_1_M();
	test_context_sso_inherit_1_1();
	test_context_sso_inherit_1_M();
	test_context_sso_inherit_ncep_1_1();
	test_context_sso_inherit_ncep_1_M();
	test_context_sso_nc_other_asc_1_M();
	test_eeor_sso_1_1();
	test_eeor_sso_1_M();
	test_read_status();
	test_read_paddrinfo();
	test_read_auth_p_chklist();
	test_read_auth_l_chklist();

	printf("0");
	return EX_OK;
}
