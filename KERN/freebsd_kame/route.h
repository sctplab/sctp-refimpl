/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)route.h	8.4 (Berkeley) 1/9/95
 * $FreeBSD: src/sys/net/route.h,v 1.36.2.4 2001/12/20 10:30:17 ru Exp $
 */

#ifndef _NET_ROUTE_H_
#define _NET_ROUTE_H_

#include <sys/queue.h>
/*
 * Kernel resident routing tables.
 *
 * The routing tables are initialized when interface addresses
 * are set by making entries for all directly connected interfaces.
 */

/*
 * A route consists of a destination address and a reference
 * to a routing entry.  These are often held by protocols
 * in their control blocks, e.g. inpcb.
 */
#if 1 /* def NEW_STRUCT_ROUTE */
struct route {
	struct	rtentry *ro_rt;
	union {
		struct sockaddr_storage rou_dst_ss;
		struct sockaddr ro_dst_sa;
	} ro_u;
};
#define ro_dst ro_u.ro_dst_sa
#else
struct route {
	struct	rtentry *ro_rt;
	struct	sockaddr ro_dst;
};
#endif

/*
 * These numbers are used by reliable protocols for determining
 * retransmission behavior and are included in the routing structure.
 */
struct rt_metrics {
	u_long	rmx_locks;	/* Kernel must leave these values alone */
	u_long	rmx_mtu;	/* MTU for this path */
	u_long	rmx_hopcount;	/* max hops expected */
	u_long	rmx_expire;	/* lifetime for route, e.g. redirect */
	u_long	rmx_recvpipe;	/* inbound delay-bandwidth product */
	u_long	rmx_sendpipe;	/* outbound delay-bandwidth product */
	u_long	rmx_ssthresh;	/* outbound gateway buffer limit */
	u_long	rmx_rtt;	/* estimated round trip time */
	u_long	rmx_rttvar;	/* estimated rtt variance */
	u_long	rmx_pksent;	/* packets sent using this route */
	u_long	rmx_filler[4];	/* will be used for T/TCP later */
};

/*
 * rmx_rtt and rmx_rttvar are stored as microseconds;
 * RTTTOPRHZ(rtt) converts to a value suitable for use
 * by a protocol slowtimo counter.
 */
#define	RTM_RTTUNIT	1000000	/* units for rtt, rttvar, as units per sec */
#define	RTTTOPRHZ(r)	((r) / (RTM_RTTUNIT / PR_SLOWHZ))

/*
 * XXX kernel function pointer `rt_output' is visible to applications.
 */
struct mbuf;

/*
 * We distinguish between routes to hosts and routes to networks,
 * preferring the former if available.  For each route we infer
 * the interface to use from the gateway address supplied when
 * the route was entered.  Routes that forward packets through
 * gateways are marked so that the output routines know to address the
 * gateway rather than the ultimate destination.
 */
#ifndef RNF_NORMAL
#include <net/radix.h>
#endif
struct rtentry {
	struct	radix_node rt_nodes[2];	/* tree glue, and other values */
#define	rt_key(r)	((struct sockaddr *)((r)->rt_nodes->rn_key))
#define	rt_mask(r)	((struct sockaddr *)((r)->rt_nodes->rn_mask))
	struct	sockaddr *rt_gateway;	/* value */
	long	rt_refcnt;		/* # held references */
	u_long	rt_flags;		/* up/down?, host/net */
	struct	ifnet *rt_ifp;		/* the answer: interface to use */
	struct	ifaddr *rt_ifa;		/* the answer: interface to use */
	struct	sockaddr *rt_genmask;	/* for generation of cloned routes */
	caddr_t	rt_llinfo;		/* pointer to link level info cache */
	struct	rt_metrics rt_rmx;	/* metrics used by rx'ing protocols */
	struct	rtentry *rt_gwroute;	/* implied entry for gatewayed routes */
	int	(*rt_output) __P((struct ifnet *, struct mbuf *,
				  struct sockaddr *, struct rtentry *));
					/* output routine for this (rt,if) */
	struct	rtentry *rt_parent; 	/* cloning parent of this route */
	void	*rt_filler2;		/* more filler */
	LIST_HEAD(, rttimer) rt_timer;  /* queue of timeouts for misc funcs */

	/* the following entries are for statistics*/
	time_t rt_createtime;	/* timestamp at creation of this route */
	time_t rt_lastreftime;	/* timestamp when the latest reference time */
	u_long rt_usehist[16];	/* histogram of references for every 5 min */
	u_long rt_reusehist[16]; /* histogram of re-use of this route */
	u_long rt_releasehist[16]; /* histogram of release of this route*/
};

/*
 * Following structure necessary for 4.3 compatibility;
 * We should eventually move it to a compat file.
 */
struct ortentry {
	u_long	rt_hash;		/* to speed lookups */
	struct	sockaddr rt_dst;	/* key */
	struct	sockaddr rt_gateway;	/* value */
	short	rt_flags;		/* up/down?, host/net */
	short	rt_refcnt;		/* # held references */
	u_long	rt_use;			/* raw # packets forwarded */
	struct	ifnet *rt_ifp;		/* the answer: interface to use */
};

#define rt_use rt_rmx.rmx_pksent

#define	RTF_UP		0x1		/* route usable */
#define	RTF_GATEWAY	0x2		/* destination is a gateway */
#define	RTF_HOST	0x4		/* host entry (net otherwise) */
#define	RTF_REJECT	0x8		/* host or net unreachable */
#define	RTF_DYNAMIC	0x10		/* created dynamically (by redirect) */
#define	RTF_MODIFIED	0x20		/* modified dynamically (by redirect) */
#define RTF_DONE	0x40		/* message confirmed */
/*			0x80		   unused, was RTF_DELCLONE */
#define RTF_CLONING	0x100		/* generate new routes on use */
#define RTF_XRESOLVE	0x200		/* external daemon resolves name */
#define RTF_LLINFO	0x400		/* generated by link layer (e.g. ARP) */
#define RTF_STATIC	0x800		/* manually added */
#define RTF_BLACKHOLE	0x1000		/* just discard pkts (during updates) */
#define RTF_PROTO2	0x4000		/* protocol specific routing flag */
#define RTF_PROTO1	0x8000		/* protocol specific routing flag */

#define RTF_PRCLONING	0x10000		/* protocol requires cloning */
#define RTF_WASCLONED	0x20000		/* route generated through cloning */
#define RTF_PROTO3	0x40000		/* protocol specific routing flag */
/*			0x80000		   unused */
#define RTF_PINNED	0x100000	/* future use */
#define	RTF_LOCAL	0x200000 	/* route represents a local address */
#define	RTF_BROADCAST	0x400000	/* route represents a bcast address */
#define	RTF_MULTICAST	0x800000	/* route represents a mcast address */

#define	RTF_CACHE	0x1000000	/* cached route in general */
					/* 0x2000000 and up unassigned */

/*
 * Routing statistics.
 */
struct	rtstat {
	short	rts_badredirect;	/* bogus redirect calls */
	short	rts_dynamic;		/* routes created by redirects */
	short	rts_newgateway;		/* routes modified by redirects */
	short	rts_unreach;		/* lookups which failed */
	short	rts_wildcard;		/* lookups satisfied by a wildcard */
};
/*
 * Structures for routing messages.
 */
struct rt_msghdr {
	u_short	rtm_msglen;	/* to skip over non-understood messages */
	u_char	rtm_version;	/* future binary compatibility */
	u_char	rtm_type;	/* message type */
	u_short	rtm_index;	/* index for associated ifp */
	int	rtm_flags;	/* flags, incl. kern & message, e.g. DONE */
	int	rtm_addrs;	/* bitmask identifying sockaddrs in msg */
	pid_t	rtm_pid;	/* identify sender */
	int	rtm_seq;	/* for sender to identify action */
	int	rtm_errno;	/* why failed */
	int	rtm_use;	/* from rtentry */
	u_long	rtm_inits;	/* which metrics we are initializing */
	struct	rt_metrics rtm_rmx; /* metrics themselves */
};

#define RTM_VERSION	5	/* Up the ante and ignore older versions */

/*
 * Message types.
 */
#define RTM_ADD		0x1	/* Add Route */
#define RTM_DELETE	0x2	/* Delete Route */
#define RTM_CHANGE	0x3	/* Change Metrics or flags */
#define RTM_GET		0x4	/* Report Metrics */
#define RTM_LOSING	0x5	/* Kernel Suspects Partitioning */
#define RTM_REDIRECT	0x6	/* Told to use different route */
#define RTM_MISS	0x7	/* Lookup failed on this address */
#define RTM_LOCK	0x8	/* fix specified metrics */
#define RTM_OLDADD	0x9	/* caused by SIOCADDRT */
#define RTM_OLDDEL	0xa	/* caused by SIOCDELRT */
#define RTM_RESOLVE	0xb	/* req to resolve dst to LL addr */
#define RTM_NEWADDR	0xc	/* address being added to iface */
#define RTM_DELADDR	0xd	/* address being removed from iface */
#define RTM_IFINFO	0xe	/* iface going up/down etc. */
#define	RTM_NEWMADDR	0xf	/* mcast group membership being added to if */
#define	RTM_DELMADDR	0x10	/* mcast group membership being deleted */

/*
 * Bitmask values for rtm_inits and rmx_locks.
 */
#define RTV_MTU		0x1	/* init or lock _mtu */
#define RTV_HOPCOUNT	0x2	/* init or lock _hopcount */
#define RTV_EXPIRE	0x4	/* init or lock _expire */
#define RTV_RPIPE	0x8	/* init or lock _recvpipe */
#define RTV_SPIPE	0x10	/* init or lock _sendpipe */
#define RTV_SSTHRESH	0x20	/* init or lock _ssthresh */
#define RTV_RTT		0x40	/* init or lock _rtt */
#define RTV_RTTVAR	0x80	/* init or lock _rttvar */

/*
 * Bitmask values for rtm_addrs.
 */
#define RTA_DST		0x1	/* destination sockaddr present */
#define RTA_GATEWAY	0x2	/* gateway sockaddr present */
#define RTA_NETMASK	0x4	/* netmask sockaddr present */
#define RTA_GENMASK	0x8	/* cloning mask sockaddr present */
#define RTA_IFP		0x10	/* interface name sockaddr present */
#define RTA_IFA		0x20	/* interface addr sockaddr present */
#define RTA_AUTHOR	0x40	/* sockaddr for author of redirect */
#define RTA_BRD		0x80	/* for NEWADDR, broadcast or p-p dest addr */

/*
 * Index offsets for sockaddr array for alternate internal encoding.
 */
#define RTAX_DST	0	/* destination sockaddr present */
#define RTAX_GATEWAY	1	/* gateway sockaddr present */
#define RTAX_NETMASK	2	/* netmask sockaddr present */
#define RTAX_GENMASK	3	/* cloning mask sockaddr present */
#define RTAX_IFP	4	/* interface name sockaddr present */
#define RTAX_IFA	5	/* interface addr sockaddr present */
#define RTAX_AUTHOR	6	/* sockaddr for author of redirect */
#define RTAX_BRD	7	/* for NEWADDR, broadcast or p-p dest addr */
#define RTAX_MAX	8	/* size of array to allocate */

struct rt_addrinfo {
	int	rti_addrs;
	struct	sockaddr *rti_info[RTAX_MAX];
	int	rti_flags;
	struct	ifaddr *rti_ifa;
	struct	ifnet *rti_ifp;
};

struct route_cb {
	int	ip_count;
	int	ip6_count;
	int	ipx_count;
	int	ns_count;
	int	any_count;
};

/* 
 * This structure, and the prototypes for the rt_timer_{init,remove_all,
 * add,timer} functions all used with the kind permission of BSDI.
 * These allow functions to be called for routes at specific times.
 */

struct rttimer {
	TAILQ_ENTRY(rttimer)	rtt_next;  /* entry on timer queue */
	LIST_ENTRY(rttimer) 	rtt_link;  /* multiple timers per rtentry */
	struct rttimer_queue	*rtt_queue;/* back pointer to queue */
	struct rtentry  	*rtt_rt;   /* Back pointer to the route */
	void            	(*rtt_func) __P((struct rtentry *, 
						 struct rttimer *));
	time_t          	rtt_time; /* When this timer was registered */
};

struct rttimer_queue {
	long				rtq_timeout;
	unsigned long			rtq_count;
	TAILQ_HEAD(, rttimer)		rtq_head;
	LIST_ENTRY(rttimer_queue)	rtq_link;
};

#ifdef _KERNEL
#define	RTFREE(rt) \
	do { \
		if ((rt)->rt_refcnt <= 1) \
			rtfree(rt); \
		else \
			(rt)->rt_refcnt--; \
	} while (0)

#define	RTUSE(rt) \
	do { \
		int i; \
		(rt)->rt_use++; \
		(rt)->rt_lastreftime = time_second; \
		i = ((rt)->rt_lastreftime - (rt)->rt_createtime) / 300; \
		if (i > 12) \
			i = 12; \
		(rt)->rt_usehist[i]++; \
	} while (0)

#define	RTREUSE(rt) \
	do { \
		int i; \
		i = (time_second - (rt)->rt_createtime) / 300; \
		if (i > 12) \
			i = 12; \
		(rt)->rt_reusehist[i]++; \
	} while (0)

#define	RTRELEASE(rt) \
	do { \
		int i; \
		i = (time_second - (rt)->rt_createtime) / 300; \
		if (i > 12) \
			i = 12; \
		(rt)->rt_releasehist[i]++; \
	} while (0)

extern struct route_cb route_cb;
extern struct radix_node_head *rt_tables[AF_MAX+1];

struct ifmultiaddr;
struct proc;

void	 route_init __P((void));
int	 rt_getifa __P((struct rt_addrinfo *));
void	 rt_ifmsg __P((struct ifnet *));
void	 rt_missmsg __P((int, struct rt_addrinfo *, int, int));
void	 rt_newaddrmsg __P((int, struct ifaddr *, int, struct rtentry *));
void	 rt_newmaddrmsg __P((int, struct ifmultiaddr *));
int	 rt_setgate __P((struct rtentry *,
	    struct sockaddr *, struct sockaddr *));
int	 rt_timer_add __P((struct rtentry *,
			  void(*)(struct rtentry *, struct rttimer *),
			  struct rttimer_queue *));
struct	 rttimer_queue *rt_timer_queue_create __P((u_int));
void	 rt_timer_queue_change __P((struct rttimer_queue *, long));
void	 rt_timer_queue_destroy __P((struct rttimer_queue *, int));
void	 rt_timer_remove_all __P((struct rtentry *));
unsigned long rt_timer_count __P((struct rttimer_queue *));
void	 rt_timer_timer __P((void *));
void	 rt_add_cache __P((struct rtentry *,
			   void(*) __P((struct rtentry *, struct rttimer *))));
void	 rtalloc __P((struct route *));
void	 rtalloc_ign __P((struct route *, u_long));
struct rtentry *
	 rtalloc1 __P((struct sockaddr *, int, u_long));
void	 rtfree __P((struct rtentry *));
int	 rtinit __P((struct ifaddr *, int, int));
int	 rtioctl __P((u_long, caddr_t, struct proc *));
void	 rtredirect __P((struct sockaddr *, struct sockaddr *,
	    struct sockaddr *, int, struct sockaddr *, struct rtentry **));
int	 rtrequest __P((int, struct sockaddr *,
	    struct sockaddr *, struct sockaddr *, int, struct rtentry **));
int	 rtrequest1 __P((int, struct rt_addrinfo *, struct rtentry **));
#endif

#endif
