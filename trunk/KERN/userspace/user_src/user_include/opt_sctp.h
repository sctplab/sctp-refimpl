#define SCTP 1
#define SCTP_DEBUG 1

/* SCTP_PROCESS_LEVEL_LOCKS uses sctp_process_lock.h within sctp_pcb.h 
 *  otherwise if undefined (i.e. below is commented out), we will use 
 *  sctp_lock_userspace.h .
 */
#define SCTP_PROCESS_LEVEL_LOCKS 1

#if defined(__Userspace_os_FreeBSD)
/* our FreeBSD machines are typically configured to have the CRC32c disabled */
#define SCTP_WITH_NO_CSUM 1
#endif
