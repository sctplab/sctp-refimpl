#ifndef __sctp_dtrace_declare_h__
#if defined(__FreeBSD__)
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>

/* Declare the SCTP provider */
SDT_PROVIDER_DECLARE(sctp);

/* The probes we have so far: */

/* One to track a net's cwnd */
/* initial */
SDT_PROBE_DECLARE(sctp, cwnd, net, init);
/* update at a ack -- increase */
SDT_PROBE_DECLARE(sctp, cwnd, net, ack);
/* update at a fast retransmit -- decrease */
SDT_PROBE_DECLARE(sctp, cwnd, net, fr);
/* update at a time-out -- decrease */
SDT_PROBE_DECLARE(sctp, cwnd, net, to);
/* update at a burst-limit -- decrease */
SDT_PROBE_DECLARE(sctp, cwnd, net, bl);
/* update at a ECN -- decrease */
SDT_PROBE_DECLARE(sctp, cwnd, net, ecn);
/* update at a Packet-Drop -- decrease */
SDT_PROBE_DECLARE(sctp, cwnd, net, pd);

/* One to track an associations rwnd */
SDT_PROBE_DECLARE(sctp, rwnd, assoc, val);

/* One to track a net's flight size */ 
SDT_PROBE_DECLARE(sctp, flightsize, net, val);

/* One to track an associations flight size */ 
SDT_PROBE_DECLARE(sctp, flightsize, assoc, val);






#else
/* All other platforms not defining dtrace probes */
#ifndef SDT_PROBE
#define SDT_PROBE(a, b, c, d, e, f, g, h, i) 
#endif
#endif
#endif
