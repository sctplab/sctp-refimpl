/*	$KAME: sctp_var.h,v 1.24 2005/03/06 16:04:19 itojun Exp $	*/

/*
 * Copyright (c) 2001-2005 Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETINET_SCTP_VAR_H_
#define _NETINET_SCTP_VAR_H_

#ifndef __OpenBSD__
#include <sys/socketvar.h>
#endif
#include <netinet/sctp_uio.h>

/* SCTP Kernel structures */

/*
 * Names for SCTP sysctl objects
 */
#ifndef __APPLE__
#define	SCTPCTL_MAXDGRAM	    1	/* max datagram size */
#define	SCTPCTL_RECVSPACE	    2	/* default receive buffer space */
#define SCTPCTL_AUTOASCONF          3   /* auto asconf enable/disable flag */
#define SCTPCTL_ECN_ENABLE          4	/* Is ecn allowed */
#define SCTPCTL_ECN_NONCE           5   /* Is ecn nonce allowed */
#define SCTPCTL_STRICT_SACK         6	/* strictly require sack'd TSN's to be
					 * smaller than sndnxt.
					 */
#define SCTPCTL_NOCSUM_LO           7   /* Require that the Loopback NOT have
				         * the crc32 checksum on packets routed over
					 * it.
				         */
#define SCTPCTL_STRICT_INIT         8
#define SCTPCTL_PEER_CHK_OH         9
#define SCTPCTL_MAXBURST            10
#define SCTPCTL_MAXCHUNKONQ         11
#define SCTPCTL_DELAYED_SACK        12
#define SCTPCTL_HB_INTERVAL         13
#define SCTPCTL_PMTU_RAISE          14
#define SCTPCTL_SHUTDOWN_GUARD      15
#define SCTPCTL_SECRET_LIFETIME     16
#define SCTPCTL_RTO_MAX             17
#define SCTPCTL_RTO_MIN             18
#define SCTPCTL_RTO_INITIAL         19
#define SCTPCTL_INIT_RTO_MAX        20
#define SCTPCTL_COOKIE_LIFE         21
#define SCTPCTL_INIT_RTX_MAX        22
#define SCTPCTL_ASSOC_RTX_MAX       23
#define SCTPCTL_PATH_RTX_MAX        24
#define SCTPCTL_NR_OUTGOING_STREAMS 25
#define SCTPCTL_CMT_ON_OFF          26
#define SCTPCTL_CWND_MAXBURST       27
#define SCTPCTL_EARLY_FR            28
#define SCTPCTL_RTTVAR_CC           29
#define SCTPCTL_DEADLOCK_DET        30
#define SCTPCTL_EARLY_FR_MSEC       31
#define SCTPCTL_AUTH_DISABLE        32
#define SCTPCTL_AUTH_HMAC_ID        33
#define SCTPCTL_ABC_L_VAR           34
#ifdef SCTP_DEBUG
#define SCTPCTL_DEBUG               35
#define SCTPCTL_MAXID		    36
#else
#define SCTPCTL_MAXID		    35
#endif

#define SCTPCTL_CMT_USE_DAC         37
#endif

#ifdef SCTP_DEBUG
#define SCTPCTL_NAMES { \
	{ 0, 0 }, \
	{ "sendspace", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
	{ "autoasconf", CTLTYPE_INT }, \
	{ "ecn_enable", CTLTYPE_INT }, \
	{ "ecn_nonce", CTLTYPE_INT }, \
	{ "strict_sack", CTLTYPE_INT }, \
	{ "looback_nocsum", CTLTYPE_INT }, \
	{ "strict_init", CTLTYPE_INT }, \
	{ "peer_chkoh", CTLTYPE_INT }, \
	{ "maxburst", CTLTYPE_INT }, \
	{ "maxchunks", CTLTYPE_INT }, \
	{ "delayed_sack_time", CTLTYPE_INT }, \
	{ "heartbeat_interval", CTLTYPE_INT }, \
	{ "pmtu_raise_time", CTLTYPE_INT }, \
	{ "shutdown_guard_time", CTLTYPE_INT }, \
	{ "secret_lifetime", CTLTYPE_INT }, \
	{ "rto_max", CTLTYPE_INT }, \
	{ "rto_min", CTLTYPE_INT }, \
	{ "rto_initial", CTLTYPE_INT }, \
	{ "init_rto_max", CTLTYPE_INT }, \
	{ "valid_cookie_life", CTLTYPE_INT }, \
	{ "init_rtx_max", CTLTYPE_INT }, \
	{ "assoc_rtx_max", CTLTYPE_INT }, \
	{ "path_rtx_max", CTLTYPE_INT }, \
	{ "nr_outgoing_streams", CTLTYPE_INT }, \
	{ "cmt_on_off", CTLTYPE_INT }, \
	{ "cmt_use_dac", CTLTYPE_INT }, \
	{ "cwnd_maxburst", CTLTYPE_INT }, \
        { "early_fast_retran", CTLTYPE_INT }, \
        { "use_rttvar_congctrl", CTLTYPE_INT }, \
        { "deadlock_detect", CTLTYPE_INT }, \
        { "early_fast_retran_msec", CTLTYPE_INT }, \
	{ "auth_disable", CTLTYPE_INT }, \
	{ "auth_hmac_id", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
}
#else
#define SCTPCTL_NAMES { \
	{ 0, 0 }, \
	{ "sendspace", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
	{ "autoasconf", CTLTYPE_INT }, \
	{ "ecn_enable", CTLTYPE_INT }, \
	{ "ecn_nonce", CTLTYPE_INT }, \
	{ "strict_sack", CTLTYPE_INT }, \
	{ "looback_nocsum", CTLTYPE_INT }, \
	{ "strict_init", CTLTYPE_INT }, \
	{ "peer_chkoh", CTLTYPE_INT }, \
	{ "maxburst", CTLTYPE_INT }, \
	{ "maxchunks", CTLTYPE_INT }, \
	{ "delayed_sack_time", CTLTYPE_INT }, \
	{ "heartbeat_interval", CTLTYPE_INT }, \
	{ "pmtu_raise_time", CTLTYPE_INT }, \
	{ "shutdown_guard_time", CTLTYPE_INT }, \
	{ "secret_lifetime", CTLTYPE_INT }, \
	{ "rto_max", CTLTYPE_INT }, \
	{ "rto_min", CTLTYPE_INT }, \
	{ "rto_initial", CTLTYPE_INT }, \
	{ "init_rto_max", CTLTYPE_INT }, \
	{ "valid_cookie_life", CTLTYPE_INT }, \
	{ "init_rtx_max", CTLTYPE_INT }, \
	{ "assoc_rtx_max", CTLTYPE_INT }, \
	{ "path_rtx_max", CTLTYPE_INT }, \
	{ "nr_outgoing_streams", CTLTYPE_INT }, \
	{ "cmt_on_off", CTLTYPE_INT }, \
	{ "cmt_use_dac", CTLTYPE_INT }, \
	{ "cwnd_maxburst", CTLTYPE_INT }, \
        { "early_fast_retran", CTLTYPE_INT }, \
        { "use_rttvar_congctrl", CTLTYPE_INT }, \
        { "deadlock_detect", CTLTYPE_INT }, \
        { "early_fast_retran_msec", CTLTYPE_INT }, \
	{ "auth_disable", CTLTYPE_INT }, \
	{ "auth_hmac_id", CTLTYPE_INT }, \
}
#endif



#if (defined(__APPLE__) && defined(KERNEL))
#ifndef _KERNEL
#define _KERNEL
#endif
#endif

#if defined(_KERNEL)

#if defined(__FreeBSD__) || defined(__APPLE__)
#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_sctp);
#endif
extern struct	pr_usrreqs sctp_usrreqs;
#elif defined(__NetBSD__)
int sctp_usrreq __P((struct socket *, int, struct mbuf *, struct mbuf *,
		      struct mbuf *, struct proc *));
#else
int sctp_usrreq __P((struct socket *, int, struct mbuf *, struct mbuf *,
		      struct mbuf *));
#endif
#if defined(HAVE_SCTP_SORECEIVE)
#define	sctp_sbspace(asoc, sb) ((long) (((sb)->sb_hiwat > (asoc)->sb_cc) ? ((sb)->sb_hiwat - (asoc)->sb_cc) : 0))
#else
#define	sctp_sbspace(asoc, sb) ((long) (((sb)->sb_hiwat > (sb)->sb_cc) ? ((sb)->sb_hiwat - (sb)->sb_cc) : 0))
#endif

#define	sctp_sbspace_failedmsgs(sb) ((long) (((sb)->sb_hiwat > (sb)->sb_cc) ? ((sb)->sb_hiwat - (sb)->sb_cc) : 0))

#define sctp_sbspace_sub(a,b) ((a > b) ? (a - b) : 0)

#if defined(__FreeBSD__) && __FreeBSD_version > 500000
#define sctp_sbfree(stcb, sb, m) { \
        if((sb)->sb_cc >= (m)->m_len) { \
  	   (sb)->sb_cc -= (m)->m_len; \
        } else { \
           panic("sb_cc would go negative"); \
           (sb)->sb_cc = 0; \
        } \
        if(stcb) {\
          if((stcb)->asoc.sb_cc >= (m)->m_len) {\
             (stcb)->asoc.sb_cc -= (m)->m_len; \
          } else  {\
              panic("assoc sb_cc would go negative"); \
             (stcb)->asoc.sb_cc = 0; \
          } \
        } \
	if ((m)->m_type != MT_DATA && (m)->m_type != MT_HEADER && \
	    (m)->m_type != MT_OOBDATA) \
		(sb)->sb_ctl -= (m)->m_len; \
        if((sb)->sb_mbcnt >= MSIZE) { \
           (sb)->sb_mbcnt -= MSIZE; \
	    if ((m)->m_flags & M_EXT) { \
		if((sb)->sb_mbcnt >= (m)->m_ext.ext_size) { \
		   (sb)->sb_mbcnt -= (m)->m_ext.ext_size; \
                } else  { \
                   panic("assoc sb_mbcnt would go negative"); \
		   (sb)->sb_mbcnt = 0; \
                } \
            } \
        } else  { \
            panic("sb_mbcnt would go negative"); \
            (sb)->sb_mbcnt = 0; \
        } \
}

#define sctp_sballoc(stcb, sb, m)  { \
	(sb)->sb_cc += (m)->m_len; \
        if(stcb) \
  	  (stcb)->asoc.sb_cc += (m)->m_len; \
	if ((m)->m_type != MT_DATA && (m)->m_type != MT_HEADER && \
	    (m)->m_type != MT_OOBDATA) \
		(sb)->sb_ctl += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt += (m)->m_ext.ext_size; \
}

#else
#if defined(HAVE_SCTP_SORECEIVE)
#define sctp_sbfree(stcb, sb, m) { \
        if((sb)->sb_cc >= (uint32_t)(m)->m_len) { \
  	   (sb)->sb_cc -= (m)->m_len; \
        } else { \
           (sb)->sb_cc = 0; \
        } \
        if(stcb) {\
          if((stcb)->asoc.sb_cc >= (uint32_t)(m)->m_len) {\
             (stcb)->asoc.sb_cc -= (m)->m_len; \
          } else  {\
             (stcb)->asoc.sb_cc = 0; \
          } \
        } \
        if((sb)->sb_mbcnt >= MSIZE) { \
           (sb)->sb_mbcnt -= MSIZE; \
	    if ((m)->m_flags & M_EXT) { \
		if((sb)->sb_mbcnt >= (uint32_t)(m)->m_ext.ext_size) { \
		   (sb)->sb_mbcnt -= (m)->m_ext.ext_size; \
                } else  { \
		   (sb)->sb_mbcnt = 0; \
                } \
            } \
        } else  { \
            (sb)->sb_mbcnt = 0; \
        } \
}

#define sctp_sballoc(stcb, sb, m)  { \
	(sb)->sb_cc += (m)->m_len; \
        if(stcb) \
  	  (stcb)->asoc.sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt += (m)->m_ext.ext_size; \
}

#else

#define sctp_sbfree(stcb, sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt -= (m)->m_ext.ext_size; \
}

#define sctp_sballoc(stcb, sb, m)  { \
	(sb)->sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_flags & M_EXT) \
		(sb)->sb_mbcnt += (m)->m_ext.ext_size; \
}

#endif
#endif

#define sctp_ucount_incr(val) { \
             val++; \
} 
#ifdef INVARIANTS_SCTP
#define sctp_ucount_decr(val) { \
             if(val > 0) { \
                val--; \
             } else {  \
                panic("decr would be negative"); \
             } \
}
#else
#define sctp_ucount_decr(val) { \
             if(val > 0) { \
                val--; \
             } else {  \
                val = 0; \
             } \
}
#endif
 
extern int	sctp_sendspace;
extern int	sctp_recvspace;
extern int      sctp_ecn_enable;
extern int      sctp_ecn_nonce;
extern int      sctp_use_cwnd_based_maxburst;
extern unsigned int sctp_cmt_on_off;
extern unsigned int sctp_cmt_use_dac;
extern unsigned int sctp_cmt_sockopt_on_off;
struct sctp_nets;
struct sctp_inpcb;
struct sctp_tcb;
struct sctphdr;

#if defined(__OpenBSD__)
void sctp_fasttim(void);
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
void	sctp_ctlinput __P((int, struct sockaddr *, void *));
int	sctp_ctloutput __P((struct socket *, struct sockopt *));
void	sctp_input __P((struct mbuf *, int));
#else
void*	sctp_ctlinput __P((int, struct sockaddr *, void *));
int	sctp_ctloutput __P((int, struct socket *, int, int, struct mbuf **));
void	sctp_input __P((struct mbuf *, ... ));
#endif
void	sctp_drain __P((void));
void	sctp_init __P((void));
int	sctp_shutdown __P((struct socket *));
void	sctp_notify __P((struct sctp_inpcb *, int, struct sctphdr *,
			 struct sockaddr *, struct sctp_tcb *,
			 struct sctp_nets *));
int sctp_usr_recvd __P((struct socket *, int));

#if defined(INET6)
void ip_2_ip6_hdr __P((struct ip6_hdr *, struct ip *));
#endif

int sctp_bindx(struct socket *, int, struct sockaddr_storage *,
	int, int, struct proc *);

/* can't use sctp_assoc_t here */
int sctp_peeloff(struct socket *, struct socket *, int, caddr_t, int *);


sctp_assoc_t sctp_getassocid(struct sockaddr *);



int sctp_ingetaddr(struct socket *,
#if defined(__FreeBSD__) || defined(__APPLE__)
		   struct sockaddr **
#else
		   struct mbuf *
#endif
);

int sctp_peeraddr(struct socket *,
#if defined(__FreeBSD__) || defined(__APPLE__)
		  struct sockaddr **
#else
		  struct mbuf *
#endif
);

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
#if __FreeBSD_version >= 600000
int sctp_listen(struct socket *, int,  struct thread *);
#else
int sctp_listen(struct socket *, struct thread *);
#endif
#else
int sctp_listen(struct socket *, struct proc *);
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
int sctp_accept(struct socket *, struct sockaddr **);
#else
int sctp_accept(struct socket *, struct mbuf *);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
int sctp_sysctl(int *, u_int, void *, size_t *, void *, size_t);
#endif

#ifdef __OpenBSD__
/*
 * compatibility defines for OpenBSD, Apple
 */

/* map callout into timeout for OpenBSD */

#ifndef callout_init
#define callout_init(args)
#define callout_reset(c, ticks, func, arg) \
do { \
	timeout_set((c), (func), (arg)); \
	timeout_add((c), (ticks)); \
} while (0)
#define callout_stop(c) timeout_del(c)
#define callout_pending(c) timeout_pending(c)
#define callout_active(c) timeout_initialized(c)
#endif
#endif


#if defined(__APPLE__)

/* XXX: Hopefully temporary until APPLE changes to newer defs like other BSDs */
#define if_addrlist	if_addrhead
#define if_list		if_link
#define ifa_list	ifa_link
#endif /* __APPLE__ **/

/* additional protosw entries for Mac OS X 10.4 */
#if defined(__APPLE__) && !defined(SCTP_APPLE_PANTHER)
int sctp_lock (struct socket *so, int refcount, int lr);
int sctp_unlock (struct socket *so, int refcount, int lr);
#ifdef _KERN_LOCKS_H_
lck_mtx_t *sctp_getlock(struct socket *so, int locktype);
#else
void * sctp_getlock(struct socket *so, int locktype);
#endif /* _KERN_LOCKS_H_ */
#endif /* __APPLE__ */

#endif /* _KERNEL */

#endif /* !_NETINET_SCTP_VAR_H_ */
