/* Example of crc insertion */ 
#include "crc32cr.h" 
     

unsigned int 
insert_crc32(unsigned char *buffer, unsigned int length) 
{ 
  unsigned int i; 
  unsigned long crc32 = ~0L; 
     
  for (i = 0; i < length; i++){ 
    CRC32C(crc32, buffer[i]); 
  } 
     
  /* and return it */
  return(crc32); 
} 
     
 
