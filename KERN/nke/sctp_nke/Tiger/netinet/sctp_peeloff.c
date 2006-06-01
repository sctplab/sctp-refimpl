/*-
 * Copyright (C) 2002-2006 Cisco Systems Inc,
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

/* $KAME: sctp_peeloff.c,v 1.13 2005/03/06 16:04:18 itojun Exp $	 */

#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD:$");
#endif

#if !(defined(__OpenBSD__) || defined(__APPLE__))
#include "opt_ipsec.h"
#endif
#if defined(__FreeBSD__)
#include "opt_inet6.h"
#include "opt_inet.h"
#include "opt_global.h"
#endif
#if defined(__NetBSD__)
#include "opt_inet.h"
#endif

#ifdef __APPLE__
#include <sctp.h>
#elif !defined(__OpenBSD__)
#include "opt_sctp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_peeloff.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_auth.h>

#ifdef IPSEC
#ifndef __OpenBSD__
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#else
#undef IPSEC
#endif
#endif				/* IPSEC */

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#include <sys/file.h>
#include <sys/filedesc.h>

#ifdef __APPLE__
extern struct fileops socketops;

#ifndef SCTP_APPLE_PANTHER
#include <sys/proc_internal.h>
#include <sys/file_internal.h>
#endif				/* !SCTP_APPLE_PANTHER */
#endif				/* __APPLE__ */

#endif				/* HAVE_SCTP_PEELOFF_SOCKOPT */

#ifdef SCTP_DEBUG
extern uint32_t sctp_debug_on;

#endif				/* SCTP_DEBUG */


int
sctp_can_peel_off(struct socket *head, sctp_assoc_t assoc_id)
{
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;

	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL) {
		return (EFAULT);
	}
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL) {
		return (ENOTCONN);
	}
	SCTP_TCB_UNLOCK(stcb);
	/* We are clear to peel this one off */
	return (0);
}

int
sctp_do_peeloff(struct socket *head, struct socket *so, sctp_assoc_t assoc_id)
{
	struct sctp_inpcb *inp, *n_inp;
	struct sctp_tcb *stcb;

	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL)
		return (EFAULT);
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL)
		return (ENOTCONN);

	n_inp = (struct sctp_inpcb *)so->so_pcb;
	n_inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
	    SCTP_PCB_FLAGS_CONNECTED |
	    SCTP_PCB_FLAGS_IN_TCPPOOL |	/* Turn on Blocking IO */
	    (SCTP_PCB_COPY_FLAGS & inp->sctp_flags));
	n_inp->sctp_socket = so;

	/*
	 * Now we must move it from one hash table to another and get the
	 * stcb in the right place.
	 */
	sctp_move_pcb_and_assoc(inp, n_inp, stcb);

	sctp_pull_off_control_to_new_inp(inp, n_inp, stcb);

	SCTP_TCB_UNLOCK(stcb);
	return (0);
}

struct socket *
sctp_get_peeloff(struct socket *head, sctp_assoc_t assoc_id, int *error)
{
	struct socket *newso;
	struct sctp_inpcb *inp, *n_inp;
	struct sctp_tcb *stcb;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PEEL1) {
		printf("SCTP peel-off called\n");
	}
#endif				/* SCTP_DEBUG */

	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL) {
		*error = EFAULT;
		return (NULL);
	}
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL) {
		*error = ENOTCONN;
		return (NULL);
	}
	newso = sonewconn(head, SS_ISCONNECTED
#if defined(__APPLE__) && !defined(SCTP_APPLE_PANTHER)
	    ,NULL
#endif
	    );
	if (newso == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_PEEL1) {
			printf("sctp_peeloff:sonewconn failed err\n");
		}
#endif				/* SCTP_DEBUG */
		*error = ENOMEM;
		SCTP_TCB_UNLOCK(stcb);
		return (NULL);
#ifndef SCTP_APPLE_FINE_GRAINED_LOCKING
	}
#else
	} else
		socket_lock(newso, 1);
#endif
	n_inp = (struct sctp_inpcb *)newso->so_pcb;
	SOCK_LOCK(head);
	SCTP_INP_WLOCK(inp);
	SCTP_INP_WLOCK(n_inp);
	n_inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
	    SCTP_PCB_FLAGS_CONNECTED |
	    SCTP_PCB_FLAGS_IN_TCPPOOL |	/* Turn on Blocking IO */
	    (SCTP_PCB_COPY_FLAGS & inp->sctp_flags));
	n_inp->sctp_features = inp->sctp_features;
	/* copy in the authentication parameters from the original endpoint */
	if (n_inp->sctp_ep.local_hmacs)
		sctp_free_hmaclist(n_inp->sctp_ep.local_hmacs);
	n_inp->sctp_ep.local_hmacs =
	    sctp_copy_hmaclist(inp->sctp_ep.local_hmacs);
	if (n_inp->sctp_ep.local_auth_chunks)
		sctp_free_chunklist(n_inp->sctp_ep.local_auth_chunks);
	n_inp->sctp_ep.local_auth_chunks =
	    sctp_copy_chunklist(inp->sctp_ep.local_auth_chunks);
	(void)sctp_copy_skeylist(&inp->sctp_ep.shared_keys,
	    &n_inp->sctp_ep.shared_keys);

	n_inp->sctp_socket = newso;
	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_AUTOCLOSE)) {
		sctp_feature_off(n_inp, SCTP_PCB_FLAGS_AUTOCLOSE);
		n_inp->sctp_ep.auto_close_time = 0;
		sctp_timer_stop(SCTP_TIMER_TYPE_AUTOCLOSE, n_inp, stcb, NULL);
	}
	/* Turn off any non-blocking semantic. */
	newso->so_state &= ~SS_NBIO;
	newso->so_state |= SS_ISCONNECTED;
	/* We remove it right away */
#if defined(__FreeBSD__) || defined(__APPLE__)
#ifdef SCTP_LOCK_LOGGING
	sctp_log_lock(inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_SOCK);
#endif
	TAILQ_REMOVE(&head->so_comp, newso, so_list);
	head->so_qlen--;
	SOCK_UNLOCK(head);
#else

#if defined(__NetBSD__) || defined(__OpenBSD__)
	newso = TAILQ_FIRST(&head->so_q);
#else
	newso = head->so_q;
#endif
	if (soqremque(newso, 1) == 0) {
#ifdef INVARIENTS
		panic("sctp_peeloff");
#else
		printf("soremque failed, peeloff-fails (invarients would panic)\n");
		SCTP_INP_WUNLOCK(inp);
		SCTP_INP_WUNLOCK(n_inp);
		SCTP_TCB_UNLOCK(stcb);
		*error = ENOTCONN;
		return (NULL);

#endif
	}
#endif				/* __FreeBSD__ */
	/*
	 * Now we must move it from one hash table to another and get the
	 * stcb in the right place.
	 */
	SCTP_INP_WUNLOCK(n_inp);
	SCTP_INP_WUNLOCK(inp);
	sctp_move_pcb_and_assoc(inp, n_inp, stcb);
	/*
	 * And now the final hack. We move data in the pending side i.e.
	 * head to the new socket buffer. Let the GRUBBING begin :-0
	 */
	sctp_pull_off_control_to_new_inp(inp, n_inp, stcb);

	SCTP_TCB_UNLOCK(stcb);
	return (newso);
}

#if defined(HAVE_SCTP_PEELOFF_SOCKOPT)
#ifdef __APPLE__
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
	head = (struct socket *)fp->f_data;
	if (head == NULL) {
		error = EBADF;
		goto out;
	}
	mutex_held = (*head->so_proto->pr_getlock) (head, 0);

	error = sctp_can_peel_off(head, uap->assoc_id);
	if (error) {
		return (error);
	}
	lck_mtx_assert(mutex_held, LCK_MTX_ASSERT_OWNED);
	socket_unlock(head, 0);	/* unlock head to avoid deadlock with select,
				 * keep a ref on head */
	fflag = fp->f_flag;
	proc_fdlock(p);
	error = falloc_locked(p, &fp, &newfd, 1);
	if (error) {
		/*
		 * Probably ran out of file descriptors. Put the unaccepted
		 * connection back onto the queue and do another wakeup so
		 * some other process might have a chance at it.
		 */
		/* SCTP will NOT put the connection back onto queue */
		proc_fdunlock(p);
		goto out;
	}
	*fdflags(p, newfd) &= ~UF_RESERVED;
	uap->new_sd = newfd;	/* return the new descriptor to the caller */
	socket_lock(head, 0);
	so = sctp_get_peeloff(head, uap->assoc_id, &error);
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = fflag;
	fp->f_ops = &socketops;
	fp->f_data = (caddr_t)so;
	fp_drop(p, newfd, fp, 1);
	proc_fdunlock(p);
	so->so_state &= ~SS_COMP;
	so->so_state &= ~SS_NOFDREF;
	so->so_head = NULL;
	socket_unlock(so, 1);
out:
	file_drop(fd);
	return (error);
}

#endif				/* SCTP_APPLE_PANTHER */
#endif				/* __APPLE__ */

#endif				/* HAVE_SCTP_PEELOFF_SOCKOPT */
