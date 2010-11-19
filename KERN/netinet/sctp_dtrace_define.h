#ifndef __sctp_dtrace_define_h__
#if defined(__FreeBSD__)
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>

SDT_PROVIDER_DEFINE(sctp);

/********************************************************/
/* Cwnd probe - tracks changes in the congestion window on a netp */
/********************************************************/
/* Initial */
SDT_PROBE_DEFINE(sctp, cwnd, net, init, init);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, init, 4, "int");


/* ACK-INCREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, ack, ack);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ack, 4, "int");

/* FastRetransmit-DECREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, fr, fr);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, fr, 4, "int");


/* TimeOut-DECREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, to, to);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, to, 4, "int");


/* BurstLimit-DECREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, bl, bl);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, bl, 4, "int");


/* ECN-DECREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, ecn, ecn);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, ecn, 4, "int");


/* PacketDrop-DECREASE */
SDT_PROBE_DEFINE(sctp, cwnd, net, pd, pd);
/* The Vtag for this end */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 0, "uint32_t");
/* The port number of the local side << 16 | port number of remote 
 * in network byte order.
 */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 1, "uint32_t");
/* The pointer to the struct sctp_nets * changing */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 2, "uintptr_t");
/* The old value of the cwnd  */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 3, "int");
/* The new value of the cwnd */
SDT_PROBE_ARGTYPE(sctp, cwnd, net, pd, 4, "int");



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
#else /* to #if Freebsd */
/* All other platforms not defining dtrace probes */
#ifndef SDT_PROBE
#define SDT_PROBE(a, b, c, d, e, f, g, h, i) 
#endif

#endif

#endif 
