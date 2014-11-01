#!/bin/sh

source=~/Documents/KERN/darwin
destination=~/Documents
kernel=xnu-1699.24.8

files='bsd/conf/MASTER
       bsd/conf/files
       bsd/net/rtsock.c
       bsd/netinet/in_proto.c
       bsd/netinet6/in6_proto.c
       bsd/sys/mbuf.h
       bsd/sys/socket.h'

for file in $files
do
	echo "Copying " $file
	cp $source/$kernel/$file $destination/$kernel/$file
done

source=~/Documents/KERN/nke/sctp_nke/Lion
files='netinet/sctp.h
       netinet/sctp_asconf.c
       netinet/sctp_asconf.h
       netinet/sctp_auth.c
       netinet/sctp_auth.h
       netinet/sctp_bsd_addr.c
       netinet/sctp_bsd_addr.h
       netinet/sctp_callout.c
       netinet/sctp_callout.h
       netinet/sctp_cc_functions.c
       netinet/sctp_constants.h
       netinet/sctp_crc32.c
       netinet/sctp_crc32.h
       netinet/sctp_dtrace_declare.h
       netinet/sctp_dtrace_define.h
       netinet/sctp_header.h
       netinet/sctp_indata.c
       netinet/sctp_indata.h
       netinet/sctp_input.c
       netinet/sctp_input.h
       netinet/sctp_lock_apple_fg.h
       netinet/sctp_macosx.c
       netinet/sctp_os.h
       netinet/sctp_os_macosx.h
       netinet/sctp_output.c
       netinet/sctp_output.h
       netinet/sctp_pcb.c
       netinet/sctp_pcb.h
       netinet/sctp_peeloff.c
       netinet/sctp_peeloff.h
       netinet/sctp_ss_functions.c
       netinet/sctp_structs.h
       netinet/sctp_sysctl.c
       netinet/sctp_sysctl.h
       netinet/sctp_timer.c
       netinet/sctp_timer.h
       netinet/sctp_uio.h
       netinet/sctp_usrreq.c
       netinet/sctp_var.h
       netinet/sctputil.c
       netinet/sctputil.h
       netinet6/sctp6_usrreq.c
       netinet6/sctp6_var.h'
for file in $files
do
	echo "Copying " $file
	cp $source/$file $destination/$kernel/bsd/$file
done

