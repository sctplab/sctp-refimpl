#ifndef __sctp_csum_h__
#define __sctp_csum_h__

u_int32_t
update_crc32(u_int32_t crc32,
	     unsigned char *buffer, unsigned int length);

u_int32_t
sctp_csum_finalize(u_int32_t crc32);

#endif
