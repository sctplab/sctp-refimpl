#ifndef __sctp_dtrace_define_h__
#if defined(__FreeBSD__)
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>

SDT_PROVIDER_DEFINE(sctp);

/********************************************************/
/* Cwnd probe - tracks changes in the congestion window on a netp */
/********************************************************/
SDT_PROBE_DEFINE(sctp, cwnd, net, val, val);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, val, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, val, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, val, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, val, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, val, 4, "int");

/********************************************************/
/* Rwnd probe - tracks changes in the receiver window for an assoc */
/********************************************************/
SDT_PROBE_DEFINE(sctp, rwnd, assoc, val, val);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 1, "uint32_t");
/* The up/down amount */
SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 2, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, rwnd, assoc, val, 3, "int");

/********************************************************/
/* flight probe - tracks changes in the flight size on a net or assoc */
/********************************************************/
SDT_PROBE_DEFINE(sctp, flightsize, net, val, val);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 2, "uintptr_t");
/* The up/down amount */
SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, flightsize, net, val, 4, "int");
/********************************************************/
/* The total flight version */
/********************************************************/
SDT_PROBE_DEFINE(sctp, flightsize, assoc, val, val);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 1, "uint32_t");
/* The up/down amount */
SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 2, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, flightsize, assoc, val, 3, "int");

#endif
#endif
