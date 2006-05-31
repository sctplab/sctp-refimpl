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
 * you get back a pointer to a struct xxx
 *
 */

/* The extern socket hash */
extern HashedTbl *sd_pool;
/* do I need a mutex? */

struct rsp_socket_hash {
	int	 	sd;			/* sctp socket */
	HashedTbl	*cache;			/* cache of names */
	HashedTbl	*vtagHash;		/* assoc id-> rsp_pool */
	HashedTbl	*ipaddrPortHash		/* ipadd -> rsp_pool_element */
	uint32_t 	refcnt;			/* number of names in use */
	dlist_t 	*enrpAddrList;		/* Home ENRP server */
	char 		*registeredName;	/* our name if registered */
	uint16_t	port;			/* our port number */
	uint8_t		registered;		/* boolean flag */
};

/* A pool entry */
struct rsp_pool {
	char 		*name;
	uint32_t 	name_len;
	dlist_t 	*peList;
	uint32_t 	refcnt;
	uint32_t	regType;
	uint32_t	policy_value;
	uint32_t	policy_actvalue;
	uint8_t         auto_update;
};

/* Each entry aka the actual PE */
struct rsp_pool_element {
	char 		*name; 		/* pointer to pool name */
	struct rsp_pool *pool;		/* pointer to pool entry */
	dlist_t 	*addrList;	/* list of addresses */
	sctp_assoc_t	asocid;		/* sctp asoc id */
	int		sd;		/* sctp socket */
};

/* An address entry in the list */
struct pe_address {
	union {
		struct sockaddr_in  sin;
		struct sockaddr_in6 sin6;
	}sa;
};


#endif
