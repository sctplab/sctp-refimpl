#ifndef __sctp_dtrace_declare_h__
#if defined(__FreeBSD__)
#include "opt_kdtrace.h"
#include <sys/kernel.h>
#include <sys/sdt.h>

/* Declare the SCTP provider */
SDT_PROVIDER_DECLARE(sctp);

/* The probes we have so far: */

/* One to track a net's cwnd */
SDT_PROBE_DECLARE(sctp, cwnd, net, val);

/* One to track an associations rwnd */
SDT_PROBE_DECLARE(sctp, rwnd, assoc, val);

/* One to track a net's flight size */ 
SDT_PROBE_DECLARE(sctp, flightsize, net, val);

/* One to track an associations flight size */ 
SDT_PROBE_DECLARE(sctp, flightsize, assoc, val);






#endif
#endif
