/*-
 * Copyright (C) 2002-2007 Cisco Systems Inc,
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2006 Apple Computer, Inc. All Rights Reserved.
 * 
 * @APPLE_LICENSE_OSREFERENCE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code 
 * as defined in and that are subject to the Apple Public Source License 
 * Version 2.0 (the 'License'). You may not use this file except in 
 * compliance with the License.  The rights granted to you under the 
 * License may not be used to create, or enable the creation or 
 * redistribution of, unlawful or unlicensed copies of an Apple operating 
 * system, or to circumvent, violate, or enable the circumvention or 
 * violation of, any terms of an Apple operating system software license 
 * agreement.
 *
 * Please obtain a copy of the License at 
 * http://www.opensource.apple.com/apsl/ and read it before using this 
 * file.
 *
 * The Original Code and all software distributed under the License are 
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * Please see the License for the specific language governing rights and 
 * limitations under the License.
 *
 * @APPLE_LICENSE_OSREFERENCE_HEADER_END@
 */

#include <sctp.h>

#include <sys/param.h>
#include <sys/domain.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/route.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#include <netinet/in_pcb.h>
#include <netinet/sctp_os.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_var.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_peeloff.h>

#define APPLE_FILE_NO 5

/* sctp_peeloff() support via socket option */
#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>

extern struct fileops socketops;

#ifndef SCTP_APPLE_PANTHER
#include <sys/proc_internal.h>
#include <sys/file_internal.h>
#endif /* !SCTP_APPLE_PANTHER */
#endif /* HAVE_SCTP_PEELOFF_SOCKOPT */

#ifdef SCTP_DEBUG
extern uint32_t sctp_debug_on;
#endif /* SCTP_DEBUG */

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)

/*
 * NOTE!! sctp_peeloff_option() MUST be kept in sync with the Apple accept()
 * call.
 */
#ifdef SCTP_APPLE_PANTHER
int
sctp_peeloff_option(struct proc *p, struct sctp_peeloff_opt *uap)
{
	struct file *fp;
	int error, s;
	struct socket *head, *so;
	int fd;
	short fflag;		/* type must match fp->f_flag */

	/* AUDIT_ARG(fd, uap->s); */
	error = getsock(p->p_fd, uap->s, &fp);
	if (error)
		return (error);
	s = splnet();
	head = (struct socket *)fp->f_data;
	if (head == NULL) {
		splx(s);
		return (EBADF);
	}
	error = sctp_can_peel_off(head, uap->assoc_id);
	if (error) {
		splx(s);
		return (error);
	}
	fflag = fp->f_flag;
	thread_funnel_switch(NETWORK_FUNNEL, KERNEL_FUNNEL);
	error = falloc(p, &fp, &fd);
	thread_funnel_switch(KERNEL_FUNNEL, NETWORK_FUNNEL);
	if (error) {
		/*
		 * Probably ran out of file descriptors. Put the unaccepted
		 * connection back onto the queue and do another wakeup so
		 * some other process might have a chance at it.
		 */
		/* SCTP will NOT put the connection back onto queue */
		splx(s);
		return (error);
	} else {
		*fdflags(p, fd) &= ~UF_RESERVED;
		/* return the new descriptor to caller */
		uap->new_sd = fd;
	}

	so = sctp_get_peeloff(head, uap->assoc_id, &error);
	if (so == NULL) {
		/* Either someone else peeled it off or can't get a socket */
		splx(s);
		return (error);
	}
	so->so_state &= ~SS_COMP;
	so->so_state &= ~SS_NOFDREF;
	so->so_head = NULL;
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = fflag;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;

	splx(s);
	return (error);
}

#else
/* TIGER */

#define f_flag f_fglob->fg_flag
#define f_type f_fglob->fg_type
#define f_msgcount f_fglob->fg_msgcount
#define f_cred f_fglob->fg_cred
#define f_ops f_fglob->fg_ops
#define f_offset f_fglob->fg_offset
#define f_data f_fglob->fg_data

int
sctp_peeloff_option(struct proc *p, struct sctp_peeloff_opt *uap)
{
	struct fileproc *fp;
	int error;
	struct socket *head, *so = NULL;
	lck_mtx_t *mutex_held;
	int fd = uap->s;
	int newfd;
	short fflag;		/* type must match fp->f_flag */
	int dosocklock = 0;

	/* AUDIT_ARG(fd, uap->s); */
	error = fp_getfsock(p, fd, &fp, &head);
	if (error) {
		if (error == EOPNOTSUPP)
			error = ENOTSOCK;
		return (error);
	}
	if (head == NULL) {
		error = EBADF;
		goto out;
	}

	if (head->so_proto->pr_getlock != NULL)  {
		mutex_held = (*head->so_proto->pr_getlock)(head, 0);
		dosocklock = 1;
	}
	else {
		mutex_held = head->so_proto->pr_domain->dom_mtx;
		dosocklock = 0;
	}

	error = sctp_can_peel_off(head, uap->assoc_id);
	if (error) {
		return (error);
	}

	lck_mtx_assert(mutex_held, LCK_MTX_ASSERT_OWNED);
        socket_unlock(head, 0); /* unlock head to avoid deadlock with select, keep a ref on head */
	fflag = fp->f_flag;
	proc_fdlock(p);
	error = falloc_locked(p, &fp, &newfd, 1);
	if (error) {
		/*
		 * Probably ran out of file descriptors. Put the
		 * unaccepted connection back onto the queue and
		 * do another wakeup so some other process might
		 * have a chance at it.
		 */
		/* SCTP will NOT put the connection back onto queue */
		proc_fdunlock(p);
		socket_lock(head, 0);
		goto out;
	}
	*fdflags(p, newfd) &= ~UF_RESERVED;
	uap->new_sd = newfd;	/* return the new descriptor to the caller */
	/* sctp_get_peeloff() does sonewconn() which expects head to be locked */
	socket_lock(head, 0);
	so = sctp_get_peeloff(head, uap->assoc_id, &error);
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = fflag;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;
	fp_drop(p, newfd, fp, 1);
	proc_fdunlock(p);
        if (dosocklock)
                socket_lock(so, 1);
        so->so_state &= ~SS_COMP;
        so->so_head = NULL;
        if (dosocklock)
                socket_unlock(so, 1);
out:
	file_drop(fd);
	return (error);
}
#endif				/* SCTP_APPLE_PANTHER */
#endif				/* HAVE_SCTP_PEELOFF_SOCKOPT */


/* socket lock pr_xxx functions */
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
/* Tiger only */
int
sctp_lock(struct socket *so, int refcount, int lr)
{
	if (so->so_pcb) {
		lck_mtx_assert(((struct inpcb *)so->so_pcb)->inpcb_mtx,
			       LCK_MTX_ASSERT_NOTOWNED);
		lck_mtx_lock(((struct inpcb *)so->so_pcb)->inpcb_mtx);
	} else {
		panic("sctp_lock: so=%x NO PCB!\n", so);
		lck_mtx_assert(so->so_proto->pr_domain->dom_mtx,
			       LCK_MTX_ASSERT_NOTOWNED);
		lck_mtx_lock(so->so_proto->pr_domain->dom_mtx);
	}

	if (so->so_usecount < 0)
		panic("sctp_lock: so=%x so_pcb=%x ref=%x\n",
		    so, so->so_pcb, so->so_usecount);

	if (refcount)
		so->so_usecount++;

	SAVE_CALLERS(((struct sctp_inpcb *)so->so_pcb)->lock_caller1,
		     ((struct sctp_inpcb *)so->so_pcb)->lock_caller2,
		     ((struct sctp_inpcb *)so->so_pcb)->lock_caller3);
#if defined(__ppc__)
	((struct sctp_inpcb *)so->so_pcb)->lock_caller1 = lr;
	((struct sctp_inpcb *)so->so_pcb)->lock_caller2 = refcount;
#endif
	((struct sctp_inpcb *)so->so_pcb)->lock_gen_count = ((struct sctp_inpcb *)so->so_pcb)->gen_count++;
	return (0);
}

int
sctp_unlock(struct socket *so, int refcount, int lr)
{
	if (so->so_pcb) {	
		SAVE_CALLERS(((struct sctp_inpcb *)so->so_pcb)->unlock_caller1,
			     ((struct sctp_inpcb *)so->so_pcb)->unlock_caller2,
			     ((struct sctp_inpcb *)so->so_pcb)->unlock_caller3);
#if defined(__ppc__)
		((struct sctp_inpcb *)so->so_pcb)->unlock_caller1 = lr;
		((struct sctp_inpcb *)so->so_pcb)->unlock_caller2 = refcount;
#endif
		((struct sctp_inpcb *)so->so_pcb)->unlock_gen_count = ((struct sctp_inpcb *)so->so_pcb)->gen_count++;
	}
	
	if (refcount)
		so->so_usecount--;

	if (so->so_usecount < 0)
		panic("sctp_unlock: so=%x usecount=%x\n", so, so->so_usecount);

	if (so->so_pcb == NULL) {
		panic("sctp_unlock: so=%x NO PCB!\n", so);
		lck_mtx_assert(so->so_proto->pr_domain->dom_mtx,
			       LCK_MTX_ASSERT_OWNED);
		lck_mtx_unlock(so->so_proto->pr_domain->dom_mtx);
	} else {
		lck_mtx_assert(((struct inpcb *)so->so_pcb)->inpcb_mtx,
			       LCK_MTX_ASSERT_OWNED);
		lck_mtx_unlock(((struct inpcb *)so->so_pcb)->inpcb_mtx);
	}
	return (0);
}

lck_mtx_t *
sctp_getlock(struct socket *so, int locktype)
{
	/* WARNING: we do not own the socket lock here... */
	/* We do not have always enough callers */
	/*
	SAVE_CALLERS(((struct sctp_inpcb *)so->so_pcb)->getlock_caller1,
		     ((struct sctp_inpcb *)so->so_pcb)->getlock_caller2,
		     ((struct sctp_inpcb *)so->so_pcb)->getlock_caller3);
	((struct sctp_inpcb *)so->so_pcb)->getlock_gen_count = ((struct sctp_inpcb *)so->so_pcb)->gen_count++;
	*/
	if (so->so_pcb) {
		if (so->so_usecount < 0)
			panic("sctp_getlock: so=%x usecount=%x\n",
			      so, so->so_usecount);
		return (((struct inpcb *)so->so_pcb)->inpcb_mtx);
	} else {
		panic("sctp_getlock: so=%x NULL so_pcb\n", so);
		return (so->so_proto->pr_domain->dom_mtx);
	}
}

void
sctp_lock_assert(struct socket *so)
{
	if (so->so_pcb) {
		lck_mtx_assert(((struct inpcb *)so->so_pcb)->inpcb_mtx,
			       LCK_MTX_ASSERT_OWNED);
	} else {
		panic("sctp_lock_assert: so=%p has sp->so_pcb==NULL.\n", so);
	}
}

void
sctp_unlock_assert(struct socket *so)
{
	if (so->so_pcb) {
		lck_mtx_assert(((struct inpcb *)so->so_pcb)->inpcb_mtx,
			       LCK_MTX_ASSERT_NOTOWNED);
	} else {
		panic("sctp_unlock_assert: so=%p has sp->so_pcb==NULL.\n", so);
	}
}
#endif /* SCTP_APPLE_FINE_GRAINED_LOCKING */

/*
 * timer functions
 */
int sctp_main_timer_ticks = 0;

void
sctp_start_main_timer(void) {
	/* bound the timer (in msec) */
	if ((int)sctp_main_timer <= 1000/hz)
		sctp_main_timer = 1000/hz;
	sctp_main_timer_ticks = MSEC_TO_TICKS(sctp_main_timer);
/*  printf("start main timer: interval %d\n", sctp_main_timer_ticks); */
	timeout(sctp_fasttim, NULL, sctp_main_timer_ticks);
}

void
sctp_stop_main_timer(void) {
/* printf("stop main timer\n"); */
	untimeout(sctp_fasttim, NULL);
}


/*
 * locks
 */
#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
#ifdef _KERN_LOCKS_H_
lck_rw_t *sctp_calloutq_mtx;
#else
void *sctp_calloutq_mtx;
#endif
#endif



/*
 * here we hack in a fix for Apple's m_copym for the case where the first
 * mbuf in the chain is a M_PKTHDR and the length is zero.
 * NOTE: this is possibly fixed in Leopard
 */
static void
sctp_pkthdr_fix(struct mbuf *m)
{
	struct mbuf *m_nxt;

	if ((SCTP_BUF_GET_FLAGS(m) & M_PKTHDR) == 0) {
		/* not a PKTHDR */
		return;
	}
	if (SCTP_BUF_LEN(m) != 0) {
		/* not a zero length PKTHDR mbuf */
		return;
	}
	/* let's move in a word into the first mbuf... yes, ugly! */
	m_nxt = SCTP_BUF_NEXT(m);
	if (m_nxt == NULL) {
		/* umm... not a very useful mbuf chain... */
		return;
	}
	if ((size_t)SCTP_BUF_LEN(m_nxt) > sizeof(int)) {
		/* move over an int */
		bcopy(mtod(m_nxt, caddr_t), mtod(m, caddr_t), sizeof(int));
		/* update mbuf data pointers and lengths */
		SCTP_BUF_LEN(m) += sizeof(int);
		SCTP_BUF_RESV_UF(m_nxt, sizeof(int));
		SCTP_BUF_LEN(m_nxt) -= sizeof(int);
	}
}

inline struct mbuf *
sctp_m_copym(struct mbuf *m, int off0, int len, int wait)
{
	struct mbuf *n, **np;
	int off = off0;
	struct mbuf *top;
	int copyhdr = 0;

	sctp_pkthdr_fix(m);

/*	return (m_copym(m, off, len, wait));*/
	if (off < 0 || len < 0)
		panic("sctp_m_copym: bad offset or length");
	if (off == 0 && m->m_flags & M_PKTHDR)
		copyhdr = 1;

	while (off >= m->m_len) {
		if (m == 0)
			panic("sctp_m_copym: null m");
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;

	MBUF_LOCK();

	while (len > 0) {
/*
		m_range_check(mfree);
		m_range_check(mclfree);
		m_range_check(mbigfree);*/

		if (m == 0) {
			if (len != M_COPYALL)
				panic("sctp_m_copym: not M_COPYALL");
			break;
		}
		if ((n = mfree)) {
			MCHECK(n);
			++mclrefcnt[mtocl(n)];
			mbstat.m_mtypes[MT_FREE]--;
			mbstat.m_mtypes[m->m_type]++;
			mfree = n->m_next;
			n->m_next = n->m_nextpkt = 0;
			n->m_type = m->m_type;
			n->m_data = n->m_dat;
			n->m_flags = 0;
		} else {
		        MBUF_UNLOCK();
		        n = m_retry(wait, m->m_type);
		        MBUF_LOCK();
		}
		*np = n;

		if (n == 0)
			goto nospace;
		if (copyhdr) {
			M_COPY_PKTHDR(n, m);
			if (len == M_COPYALL)
				n->m_pkthdr.len -= off0;
			else
				n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		if (len == M_COPYALL) {
		    if (min(len, (m->m_len - off)) == len) {
			printf("m->m_len %d - off %d = %d, %d\n", 
			       m->m_len, off, m->m_len - off,
			       min(len, (m->m_len - off)));
		    }
		}
		n->m_len = min(len, (m->m_len - off));
		if (n->m_len == M_COPYALL) {
		    printf("n->m_len == M_COPYALL, fixing\n");
		    n->m_len = MHLEN;
		}
		if (m->m_flags & M_EXT) {
			n->m_ext = m->m_ext;
			insque((queue_t)&n->m_ext.ext_refs, (queue_t)&m->m_ext.ext_refs);
			n->m_data = m->m_data + off;
			n->m_flags |= M_EXT;
		} else {
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		}
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	MBUF_UNLOCK();

/*	if (top == 0)
		MCFail++;
*/

	return (top);
nospace:
	MBUF_UNLOCK();

	m_freem(top);
/*	MCFail++;*/
	return (0);
}


/*
 * here we fix up Apple's m_prepend() and m_prepend_2().
 * See FreeBSD uipc_mbuf.c, version 1.170.
 */
struct mbuf *
sctp_m_prepend(struct mbuf *m, int len, int how)
{
	struct mbuf *mn;

	MGET(mn, how, m->m_type);
	if (mn == (struct mbuf *)NULL) {
		m_freem(m);
		return ((struct mbuf *)NULL);
	}
	if (m->m_flags & M_PKTHDR) {
		M_COPY_PKTHDR(mn, m);
		m->m_flags &= ~M_PKTHDR;
	}
	mn->m_next = m;
	m = mn;
	if (m->m_flags & M_PKTHDR) {
		if ((size_t)len < MHLEN)
			MH_ALIGN(m, len);
	} else {
		if ((size_t)len < MLEN)
			M_ALIGN(m, len);
	}
	m->m_len = len;
	return (m);
}

struct mbuf *
sctp_m_prepend_2(struct mbuf *m, int len, int how)
{
        if (M_LEADINGSPACE(m) >= len) {
                m->m_data -= len;
                m->m_len += len;
        } else {
		m = sctp_m_prepend(m, len, how);
        }
        if ((m) && (m->m_flags & M_PKTHDR))
                m->m_pkthdr.len += len;
        return (m);
}

#if defined(SCTP_APPLE_FINE_GRAINED_LOCKING)
void
sctp_slowtimo()
{
	struct inpcb *inp, *inp_next;
	struct socket *so;
#ifdef SCTP_DEBUG
	unsigned int n = 0;
#endif
	lck_rw_lock_exclusive(sctppcbinfo.ipi_ep_mtx);

	inp = LIST_FIRST(&sctppcbinfo.inplisthead);
	while (inp) {
		inp_next = LIST_NEXT(inp, inp_list);
#ifdef SCTP_DEBUG
		n++;
#endif
		if ((inp->inp_wantcnt == WNT_STOPUSING) && (lck_mtx_try_lock(inp->inpcb_mtx))) {
			so = inp->inp_socket;
			if ((so->so_usecount != 0) || (inp->inp_state != INPCB_STATE_DEAD)) {
				lck_mtx_unlock(inp->inpcb_mtx);
			} else {
				LIST_REMOVE(inp, inp_list);
				inp->inp_socket = NULL;
				so->so_pcb      = NULL;
				lck_mtx_unlock(inp->inpcb_mtx);
				lck_mtx_free(inp->inpcb_mtx, sctppcbinfo.mtx_grp);
				SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_ep, inp);
				sodealloc(so);
				SCTP_DECR_EP_COUNT();
			}
		}
		inp = inp_next;
	}
	lck_rw_unlock_exclusive(sctppcbinfo.ipi_ep_mtx);
#ifdef SCTP_DEBUG
	if ((sctp_debug_on & SCTP_DEBUG_PCB2) && (n > 0)) {
		printf("sctp_slowtimo: inps: %u\n", n);
	}
#endif
}
#endif

#if defined(SCTP_APPLE_AUTO_ASCONF)
socket_t sctp_address_monitor_so = 0;

#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))
#define NEXT_SA(sa) sa = (struct sockaddr *) \
	((caddr_t) sa + (sa->sa_len ? ROUNDUP(sa->sa_len, sizeof(unsigned long)) : sizeof(unsigned long)))

static void
sctp_get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
    int i;
    for (i = 0; i < RTAX_MAX; i++) {
	if (addrs & (1 << i)) {
	    rti_info[i] = sa;
	    NEXT_SA(sa);
	} else {
	    rti_info[i] = NULL;
	}
    }
}

static void print_address(struct sockaddr *sa)
{
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;
    ushort port = 0;
    int len = 0;
    void *src;
    char str[128];
    const char *ptr = NULL;

    switch(sa->sa_family) {
    case AF_INET:
	sin = (struct sockaddr_in *)sa;
	src = (void *)&sin->sin_addr.s_addr;
	port = ntohs(sin->sin_port);
#ifdef HAVE_SA_LEN
	len = sa->sa_len;
#else
	len = sizeof(*sin);
#endif
	break;
    case AF_INET6:
	sin6 = (struct sockaddr_in6 *)sa;
	src = (void *)&sin6->sin6_addr;
	port = ntohs(sin6->sin6_port);
#ifdef HAVE_SA_LEN
	len = sa->sa_len;
#else
	len = sizeof(*sin6);
#endif
	break;
    default:
	/* TSNH: unknown family */
	printf("[unknown address family]");
	return;
    }
    ptr = inet_ntop(sa->sa_family, src, str, sizeof(str));
    if (ptr != NULL) 
	printf("%s:%u", ptr, port);
    else
	printf("[cannot display address]:%u", port);
}

static void sctp_handle_ifamsg(struct ifa_msghdr *ifa_msg) {
    struct sockaddr *sa;
    struct sockaddr *rti_info[RTAX_MAX];
    struct ifnet *ifn, *found_ifn = NULL;
    struct ifaddr *ifa, *found_ifa = NULL;

    /* handle only the types we want */
    if ((ifa_msg->ifam_type != RTM_NEWADDR) &&
	(ifa_msg->ifam_type != RTM_DELADDR))
	return;
					  
    /* parse the list of addreses reported */
    sa = (struct sockaddr *)(ifa_msg + 1);
    sctp_get_rtaddrs(ifa_msg->ifam_addrs, sa, rti_info);

    /* we only want the interface address */
    sa = rti_info[RTAX_IFA];

/* tempy prints */
    if (ifa_msg->ifam_type == RTM_NEWADDR)
	printf("if_index %u: adding ", ifa_msg->ifam_index);
    else
	printf("if_index %u: deleting ", ifa_msg->ifam_index);
    print_address(sa);
    printf("\n");
/* end tempy prints */

    /*
     * find the actual kernel ifa/ifn for this address.
     * we need this primarily for the v6 case to get the ifa_flags.
     */
    TAILQ_FOREACH(ifn, &ifnet, if_list) {
	/* find the interface by index */
	if (ifa_msg->ifam_index == ifn->if_index) {
	    found_ifn = ifn;
	    break;
	}
    }
    if (found_ifn == NULL) {
	/* TSNH */
	printf("if_index %u not found?!", ifa_msg->ifam_index);
	return;
    }

    /* verify the address on the interface */
    TAILQ_FOREACH(ifa, &found_ifn->if_addrlist, ifa_list) {
	if (ifa->ifa_addr == NULL)
	    continue;

#if defined(INET6)
	if (ifa->ifa_addr->sa_family == AF_INET6) {
	    if (SCTP6_ARE_ADDR_EQUAL(&((struct sockaddr_in6 *)sa)->sin6_addr, 
				     &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr)) {
		found_ifa = ifa;
		break;
	    }
	} else
#endif
	if (ifa->ifa_addr->sa_family == AF_INET) {
	    if (((struct sockaddr_in *)sa)->sin_addr.s_addr ==
		((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr) {
		found_ifa = ifa;
		break;
	    }
	}
    }
    if (found_ifa == NULL) {
	/* TSNH */
	printf("ifa not found?!");
	return;
    }

    /* relay the appropriate address change to the base code */
    if (ifa_msg->ifam_type == RTM_NEWADDR) {
	sctp_addr_change(found_ifa, RTM_ADD);
    } else {
	sctp_addr_change(found_ifa, RTM_DELETE);
    }
}

void sctp_address_monitor_cb(socket_t rt_sock, void *cookie, int watif)
{
    struct msghdr msg;
    struct iovec iov;
    int flags;
    size_t length;
    errno_t error;
    struct rt_msghdr *rt_msg;
    char rt_buffer[1024];

    /* setup the receive iovec and msghdr */
    iov.iov_base = rt_buffer;
    iov.iov_len = sizeof(rt_buffer);
    bzero(&msg, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    flags = 0;
    length = 0;
    /* read the routing socket */
    error = sock_receive(rt_sock, &msg, flags, &length);
    if (error != 0) {
	printf("Routing socket read error: length %d, errno %d\n",
	       length, error);
	return;
    }
    if (length == 0) {
	printf("Routing socket closed.\n");
	return;
    }

    /* process the routing event */
    rt_msg = (struct rt_msghdr *)rt_buffer;
    printf("Got routing event 0x%x, %u bytes\n", rt_msg->rtm_type, length);
    switch (rt_msg->rtm_type) {
    case RTM_DELADDR:
    case RTM_NEWADDR:
	sctp_handle_ifamsg((struct ifa_msghdr *)rt_buffer);
	break;
    default:
	/* ignore this routing event */
	break;
    }
}

void sctp_address_monitor_start(void)
{
    errno_t error;

    /* open an in kernel routing socket client */
    error = sock_socket(PF_ROUTE, SOCK_RAW, 0, sctp_address_monitor_cb,
			NULL, &sctp_address_monitor_so);
    if (error < 0) {
	printf("Failed to create routing socket\n");
	/* FIX ME: try again later?? */
    }
}

void sctp_address_monitor_destroy(void)
{
    if (sctp_address_monitor_so != NULL) {
	sock_close(sctp_address_monitor_so);
	sctp_address_monitor_so = 0;
    }
}
#else
void sctp_address_monitor_start(void)
{
}

void sctp_address_monitor_destroy(void)
{
}
#endif
