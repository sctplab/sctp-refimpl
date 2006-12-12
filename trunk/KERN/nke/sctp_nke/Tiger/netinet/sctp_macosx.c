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

#include <sctp.h>

#include <sys/param.h>
#include <sys/domain.h>
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
	socket_unlock(head, 0);

	/* unlock head to avoid deadlock with select,
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
