#ifndef __sctp_lock_apple_fg_h__
#define __sctp_lock_apple_fg_h__
/*-
 * Copyright (c) 2001-2006, Cisco Systems, Inc. All rights reserved.
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

/*
 * Appropriate macros are also provided for Apple Mac OS 10.4.x systems.
 * 10.3.x systems (SCTP_APPLE_PANTHER defined) builds use the emtpy macros.
 */

/* for now, all locks use this group and attributes */
#define SCTP_MTX_GRP sctppcbinfo.mtx_grp
#define SCTP_MTX_ATTR sctppcbinfo.mtx_attr

/* Lock for INFO stuff */
#define SCTP_INP_INFO_LOCK_INIT() \
	sctppcbinfo.ipi_ep_mtx = lck_rw_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_INP_INFO_RLOCK()
#define SCTP_INP_INFO_RUNLOCK() 
#define SCTP_INP_INFO_WLOCK() 
#define SCTP_INP_INFO_WUNLOCK()
#define SCTP_INP_INFO_LOCK_DESTROY() \
        lck_rw_free(sctppcbinfo.ipi_ep_mtx, SCTP_MTX_GRP)
#define SCTP_IPI_COUNT_INIT() \
	sctppcbinfo.ipi_count_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_IPI_COUNT_DESTROY() \
        lck_mtx_free(sctppcbinfo.ipi_count_mtx, SCTP_MTX_GRP)

#define SCTP_IPI_ADDR_INIT()
#define SCTP_IPI_ADDR_DESTROY(_inp)
#define SCTP_IPI_ADDR_LOCK()
#define SCTP_IPI_ADDR_UNLOCK()


#define SCTP_TCB_SEND_LOCK_INIT(_tcb)
#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb)
#define SCTP_TCB_SEND_LOCK(_tcb)
#define SCTP_TCB_SEND_UNLOCK(_tcb)



#define SCTP_STATLOG_INIT_LOCK() \
	sctppcbinfo.logging_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_STATLOG_LOCK() \
	lck_mtx_lock(sctppcbinfo.logging_mtx)
#define SCTP_STATLOG_UNLOCK() \
	lck_mtx_unlock(sctppcbinfo.logging_mtx)
#define SCTP_STATLOG_DESTROY() \
	lck_mtx_free(sctppcbinfo.logging_mtx, SCTP_MTX_GRP)


#define SCTP_STATLOG_GETREF(x) { \
        lck_mtx_lock(sctppcbinfo.logging_mtx); \
        x = global_sctp_cwnd_log_at; \
        global_sctp_cwnd_log_at++; \
        if(global_sctp_cwnd_log_at == SCTP_STAT_LOG_SIZE) { \
           global_sctp_cwnd_log_at = 0; \
           global_sctp_cwnd_log_rolled = 1; \
        } \
        lck_mtx_unlock(sctppcbinfo.logging_mtx); \
}



/* Lock for INP */
#define SCTP_INP_LOCK_INIT(_inp)
#define SCTP_INP_LOCK_DESTROY(_inp)

#define SCTP_INP_RLOCK(_inp)
#define SCTP_INP_RUNLOCK(_inp)
#define SCTP_INP_WLOCK(_inp)
#define SCTP_INP_WUNLOCK(_inp)
#define SCTP_INP_INCR_REF(_inp)
#define SCTP_INP_DECR_REF(_inp)

#define SCTP_ASOC_CREATE_LOCK_INIT(_inp)
#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp)
#define SCTP_ASOC_CREATE_LOCK(_inp)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)

#define SCTP_INP_READ_INIT(_inp)
#define SCTP_INP_READ_DESTROY(_inp)
#define SCTP_INP_READ_LOCK(_inp)
#define SCTP_INP_READ_UNLOCK(_inp)

/* Lock for TCB */

#define SCTP_TCB_LOCK_INIT(_tcb)
#define SCTP_TCB_LOCK_DESTROY(_tcb)
#define SCTP_TCB_LOCK(_tcb)
#define SCTP_TCB_TRYLOCK(_tcb) 1
#define SCTP_TCB_UNLOCK(_tcb)
#define SCTP_TCB_UNLOCK_IFOWNED(_tcb)
#define SCTP_TCB_LOCK_ASSERT(_tcb)

/* iterator locks */
#define SCTP_ITERATOR_LOCK_INIT() \
	sctppcbinfo.it_mtx = lck_mtx_alloc_init(SCTP_MTX_GRP, SCTP_MTX_ATTR)
#define SCTP_ITERATOR_LOCK() \
	lck_mtx_lock(sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_UNLOCK() \
	lck_mtx_unlock(sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY() \
	lck_mtx_free(sctppcbinfo.it_mtx, SCTP_MTX_GRP)

/* socket locks */
#define SOCK_LOCK(_so)
#define SOCK_UNLOCK(_so)
#define SOCKBUF_LOCK(_so_buf)
#define SOCKBUF_UNLOCK(_so_buf)
#define SOCKBUF_LOCK_ASSERT(_so_buf)


/***************BEGIN APPLE Tiger count stuff**********************/
#define I_AM_HERE \
                do { \
			printf("%s:%d at %s\n", __FILE__, __LINE__ , __FUNCTION__); \
		} while (0)

#define SAVE_I_AM_HERE(_inp) \
                do { \
			(_inp)->i_am_here_file = APPLE_FILE_NO; \
			(_inp)->i_am_here_line = __LINE__; \
		} while (0)

/* save caller pc and caller's caller pc */
#if defined (__i386__)
#define SAVE_CALLERS(a, b, c) { \
        unsigned int ebp = 0; \
        unsigned int prev_ebp = 0; \
        asm("movl %%ebp, %0;" : "=r"(ebp)); \
        a = *(unsigned int *)(*(unsigned int *)ebp + 4) - 4; \
        prev_ebp = *(unsigned int *)(*(unsigned int *)ebp); \
        b = *(unsigned int *)((char *)prev_ebp + 4) - 4; \
        prev_ebp = *(unsigned int *)prev_ebp; \
        c = *(unsigned int *)((char *)prev_ebp + 4) - 4; \
}
#else
#define SAVE_CALLERS(caller1, caller2, caller3)
#endif

#define SBLOCKWAIT(f)   (((f) & MSG_DONTWAIT) ? M_NOWAIT : M_WAITOK)

#define SCTP_INCR_EP_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_ep, 1); \
	        } while (0)

#define SCTP_DECR_EP_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_ep,-1); \
	        } while (0)

#define SCTP_INCR_ASOC_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_asoc, 1); \
	        } while (0)

#define SCTP_DECR_ASOC_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_asoc, -1); \
	        } while (0)

#define SCTP_INCR_LADDR_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_laddr, 1); \
	        } while (0)

#define SCTP_DECR_LADDR_COUNT() \
                do { \
	               atomic_add_int(&sctppcbinfo.ipi_count_laddr, -1); \
	        } while (0)

#define SCTP_INCR_RADDR_COUNT() \
                do { \
 	               atomic_add_int(&sctppcbinfo.ipi_count_raddr,1); \
	        } while (0)

#define SCTP_DECR_RADDR_COUNT() \
                do { \
 	               atomic_add_int(&sctppcbinfo.ipi_count_raddr,-1); \
	        } while (0)

#define SCTP_INCR_CHK_COUNT() \
                do { \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk, 1); \
	        } while (0)

#ifdef INVARIANTS

#define SCTP_DECR_CHK_COUNT() \
                do { \
                       if(sctppcbinfo.ipi_count_chunk == 0) \
                             panic("chunk count to 0?"); \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk,-1); \
	        } while (0)
#else

#define SCTP_DECR_CHK_COUNT() \
                do { \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk,-1); \
	        } while (0)
#endif

#define SCTP_INCR_READQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_readq,1); \
	        } while (0)

#define SCTP_DECR_READQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_readq, -1); \
	        } while (0)

#define SCTP_INCR_STRMOQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_strmoq, 1); \
	        } while (0)

#define SCTP_DECR_STRMOQ_COUNT() \
                do { \
		       atomic_add_int(&sctppcbinfo.ipi_count_strmoq,-1); \
	        } while (0)




#endif
