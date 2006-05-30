#ifndef __rserpool_lib_h__
#define __rserpool_lib_h__

#include <dlist.h>

/*
 * We first need to have the base structure
 * of a Pool that is kept inside the library.
 * This is what will be the holder for all
 * rserpool stuff. We next have the stucture
 * for the actual PE's themselves. These will
 * need to point back to the Pool and reference
 * count it. Each one will also need to be hashed
 * via the asocid and all the addresses/ports so
 * we can figure out from a received message what
 * the actual Pool.
 *
 * So we end up with:
 *
 * assoc-id    --maps--> PE
 * port+addr's --maps--> PE
 * name        --maps--> Pool
 * Pool has a list of PE's
 *
 * We map into a global process wide
 * table based on the socket descriptor.
 * This table determines which PE/PU we are.
 *
 * sd --sd2pool--> hashes out a hash table for the socket
 *
 */

extern HashedTbl *sd_pool;

/* first the pool */
struct rsp_pool {
	char *name;
	uint32_t name_len;
	dlist_t *pe_list;
	int sd; 
	uint32_t refcount;
};



#endif
