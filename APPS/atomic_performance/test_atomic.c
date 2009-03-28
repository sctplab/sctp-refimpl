#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

/* For apple */
#if defined(__Userspace_os_Darwin)
#include <libkern/OSAtomic.h>
#define atomic_add_int(addr, val)	OSAtomicAdd32Barrier(val, (int32_t *)addr)
#define atomic_fetchadd_int(addr, val)	OSAtomicAdd32Barrier(val, (int32_t *)addr)
#define atomic_subtract_int(addr, val)	OSAtomicAdd32Barrier(-val, (int32_t *)addr)
#define atomic_cmpset_int(dst, exp, src) OSAtomicCompareAndSwapIntBarrier(exp, src, (int *)dst)
#else 

/* Using gcc built-in functions for atomic memory operations
   Reference: http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html
   Requires gcc version 4.1.0
   compile with -march=i486
 */

/*Atomically add V to *P.*/
#define atomic_add_int(P, V)	 (void) __sync_fetch_and_add(P, V)

/*Atomically subtrace V from *P.*/
#define atomic_subtract_int(P, V) (void) __sync_fetch_and_sub(P, V)

/*
 * Atomically add the value of v to the integer pointed to by p and return
 * the previous value of *p.
 */
#define atomic_fetchadd_int(p, v) __sync_fetch_and_add(p, v)

/* Following explanation from src/sys/i386/include/atomic.h,
 * for atomic compare and set
 *
 * if (*dst == exp) *dst = src (all 32 bit words)
 *
 * Returns 0 on failure, non-zero on success
 */

#define atomic_cmpset_int(dst, exp, src) __sync_bool_compare_and_swap(dst, exp, src)

#endif


void
add_one_atomic(uint32_t *val)
{
	uint32_t i;
	for (i=0; i<3000000000; i++) {
		atomic_add_int(val, 1);
	}
}

void
add_one(uint32_t *val)
{
	uint32_t i;
	for (i=0; i<3000000000; i++) {
		*val += 1;
	}
}
