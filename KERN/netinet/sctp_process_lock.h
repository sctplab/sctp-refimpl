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
#ifndef __sctp_process_lock_h__
#define __sctp_process_lock_h__



#define SCTP_IPI_COUNT_INIT()

#define SCTP_STATLOG_INIT_LOCK() 
#define SCTP_STATLOG_LOCK()
#define SCTP_STATLOG_UNLOCK()
#define SCTP_STATLOG_DESTROY()

#define SCTP_STATLOG_GETREF(x) { \
        x = atomic_fetchadd_int(&global_sctp_cwnd_log_at, 1); \
        if(x == SCTP_STAT_LOG_SIZE) { \
           global_sctp_cwnd_log_at = 1; \
           x = 0; \
           global_sctp_cwnd_log_rolled = 1; \
        } \
}

#define SCTP_INP_INFO_LOCK_INIT() \
        pthread_mutex_init(&sctppcbinfo.ipi_ep_mtx, NULL)


#define SCTP_INP_INFO_RLOCK()	do { 					\
             ptrhead_mutex_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)


#define SCTP_INP_INFO_WLOCK()	do { 					\
             ptrhead_mutex_lock(&sctppcbinfo.ipi_ep_mtx);                         \
} while (0)



#define SCTP_IPI_ADDR_INIT() \
        pthread_mutex_init(&sctppcbinfo.ipi_addr_mtx, NULL)

#define SCTP_IPI_ADDR_DESTROY() \
	pthread_mutex_destroy(&sctppcbinfo.ipi_addr_mtx)

#define SCTP_IPI_ADDR_LOCK()	do { 					\
             pthread_mutex_lock(&sctppcbinfo.ipi_addr_mtx);                         \
} while (0)

#define SCTP_IPI_ADDR_UNLOCK()		pthead_mutex_unlock(&sctppcbinfo.ipi_addr_mtx)

#define SCTP_INP_INFO_RUNLOCK()		pthread_mutex_unlock(&sctppcbinfo.ipi_ep_mtx)
#define SCTP_INP_INFO_WUNLOCK()		pthread_mutex_unlock(&sctppcbinfo.ipi_ep_mtx)

/*
 * The INP locks we will use for locking an SCTP endpoint, so for example if
 * we want to change something at the endpoint level for example random_store
 * or cookie secrets we lock the INP level.
 */

#define SCTP_INP_READ_INIT(_inp) \
	pthread_mutex_init(&(_inp)->inp_rdata_mtx, NULL)

#define SCTP_INP_READ_DESTROY(_inp) \
	pthread_mutex_destroy(&(_inp)->inp_rdata_mtx)

#define SCTP_INP_READ_LOCK(_inp)	do { \
        pthread_mutex_lock(&(_inp)->inp_rdata_mtx);    \
} while (0)


#define SCTP_INP_READ_UNLOCK(_inp) pthread_mutex_unlock(&(_inp)->inp_rdata_mtx)


#define SCTP_INP_LOCK_INIT(_inp) \
	pthread_mutex_init(&(_inp)->inp_mtx, NULL)
#define SCTP_ASOC_CREATE_LOCK_INIT(_inp) \
	pthread_mutex_init(&(_inp)->inp_create_mtx, NULL)

#define SCTP_INP_LOCK_DESTROY(_inp) pthread_mutex_destroy(&(_inp)->inp_mtx)

#define SCTP_ASOC_CREATE_LOCK_DESTROY(_inp) pthread_mutex_destroy(&(_inp)->inp_create_mtx)


#ifdef SCTP_LOCK_LOGGING
#define SCTP_INP_RLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
        pthread_mutex_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
	sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_INP);\
        pthread_mutex_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#else

#define SCTP_INP_RLOCK(_inp)	do { 					\
        pthread_mutex_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#define SCTP_INP_WLOCK(_inp)	do { 					\
        pthread_mutex_lock(&(_inp)->inp_mtx);                                     \
} while (0)

#endif


#define SCTP_TCB_SEND_LOCK_INIT(_tcb) \
	pthread_mutex_init(&(_tcb)->tcb_send_mtx, NULL)

#define SCTP_TCB_SEND_LOCK_DESTROY(_tcb) pthread_mutex_destroy(&(_tcb)->tcb_send_mtx)

#define SCTP_TCB_SEND_LOCK(_tcb)  do { pthread_mutex_lock(&(_tcb)->tcb_send_mtx); \
} while (0)

#define SCTP_TCB_SEND_UNLOCK(_tcb) pthread_mutex_unlock(&(_tcb)->tcb_send_mtx)


#define SCTP_INP_INCR_REF(_inp) atomic_add_int(&((_inp)->refcount), 1)
#define SCTP_INP_DECR_REF(_inp) atomic_add_int(&((_inp)->refcount), -1)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_ASOC_CREATE_LOCK(_inp) \
	do {								\
                sctp_log_lock(_inp, (struct sctp_tcb *)NULL, SCTP_LOG_LOCK_CREATE); \
		pthread_mutex_lock(&(_inp)->inp_create_mtx);			\
	} while (0)
#else

#define SCTP_ASOC_CREATE_LOCK(_inp) \
	do {								\
		pthread_mutex_lock(&(_inp)->inp_create_mtx);			\
	} while (0)
#endif

#define SCTP_INP_RUNLOCK(_inp)		pthread_mutex_unlock(&(_inp)->inp_mtx)
#define SCTP_INP_WUNLOCK(_inp)		pthread_mutex_unlock(&(_inp)->inp_mtx)
#define SCTP_ASOC_CREATE_UNLOCK(_inp)	pthread_mutex_unlock(&(_inp)->inp_create_mtx)

/*
 * For the majority of things (once we have found the association) we will
 * lock the actual association mutex. This will protect all the assoiciation
 * level queues and streams and such. We will need to lock the socket layer
 * when we stuff data up into the receiving sb_mb. I.e. we will need to do an
 * extra SOCKBUF_LOCK(&so->so_rcv) even though the association is locked.
 */

#define SCTP_TCB_LOCK_INIT(_tcb) \
	pthread_mutex_init(&(_tcb)->tcb_mtx, NULL)

#define SCTP_TCB_LOCK_DESTROY(_tcb)	pthread_mutex_destroy(&(_tcb)->tcb_mtx)

#ifdef SCTP_LOCK_LOGGING
#define SCTP_TCB_LOCK(_tcb)  do {					\
        sctp_log_lock(_tcb->sctp_ep, _tcb, SCTP_LOG_LOCK_TCB);          \
	pthread_mutex_lock(&(_tcb)->tcb_mtx);                                     \
} while (0)

#else
#define SCTP_TCB_LOCK(_tcb)  do {					\
	pthread_mutex_lock(&(_tcb)->tcb_mtx);                                     \
} while (0)

#endif


#define SCTP_TCB_TRYLOCK(_tcb) 	pthread_mutex_trylock(&(_tcb)->tcb_mtx)

#define SCTP_TCB_UNLOCK(_tcb)		pthread_mutex_unlock(&(_tcb)->tcb_mtx)

#define SCTP_TCB_LOCK_ASSERT(_tcb)

#define SCTP_ITERATOR_LOCK_INIT() \
        pthread_mutex_init(&sctppcbinfo.it_mtx, NULL)

#define SCTP_ITERATOR_LOCK() \
	do {								\
		pthread_mutex_lock(&sctppcbinfo.it_mtx);				\
	} while (0)

#define SCTP_ITERATOR_UNLOCK()	        pthread_mutex_unlock(&sctppcbinfo.it_mtx)
#define SCTP_ITERATOR_LOCK_DESTROY()	pthrad_mutex_destroy(&sctppcbinfo.it_mtx)


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

#define SCTP_DECR_CHK_COUNT() \
                do { \
  	               atomic_add_int(&sctppcbinfo.ipi_count_chunk,-1); \
	        } while (0)

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
