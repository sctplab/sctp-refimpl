#define SCTP_CRC32C_POLY 0x1EDC6F41 

#define SCTP_CRC32CL(c,d) (c=(c>>16)^sctp_crc_c[(c^(d))&0xFFFF]) 
#define SCTP_CRC32C(c,d) (c=(c>>8)^sctp_crc_c[(c^(d))&0xFF]) 
#include <netinet/sctp_crc32.h>
#include <sctp_crc32l.h>

u_int32_t
update_crc32l(u_int32_t crc32,
 	      unsigned char *buffer, 
	      unsigned int length) 
{ 
  int i,llen; 
  unsigned short *ptr;

  ptr = (unsigned short *)buffer;
  llen = length/2;

  /* walk the first part with the larger size table */
  for (i = 0; i < llen; i++){ 
     SCTP_CRC32CL(crc32, ptr); 
     ptr++;
  } 
  llen *=2;
  if(llen < length) {
	  /* pick up the last byte */
	       SCTP_CRC32C(crc32, buffer[llen]); 
  }
  return crc32
} 
