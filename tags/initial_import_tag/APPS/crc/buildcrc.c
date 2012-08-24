/* Example of table build routine */ 
#include <stdio.h> 
#include <stdlib.h> 
     
#define OUTPUT_FILE   "crc32cr.h" 
#define CRC32C_POLY    0x1EDC6F41L 
#define CRC_TYPE  "\ 
/* Castagnoli93                                                  */\n\ 
/* x^32+x^28+x^27+x^26+x^25+x^23+x^22+x^20+x^19+x^18+x^14+x^13+  */\n\ 
/* x^11+x^10+x^9+x^8+x^6+x^0                                     */\n\ 
/* Guy Castagnoli Stefan Braeuer and Martin Herrman              */\n\ 
/* \"Optimization of Cyclic Redundancy-Check Codes                */\n\ 
/* with 24 and 32 Parity Bits\",                                  */\n\ 
/* IEEE Transactions on Communications, Vol.41, No.6, June 1993  */\n" 
     
FILE *tf; 
     
unsigned long 
reflect_32 (unsigned long b)
{ 
  int i; 
  unsigned long rw = 0L; 
  for (i = 0; i < 32; i++){ 
    if (b & 1) 
      rw |= 1 << (31 - i); 
    b >>= 1; 
  } 
  return (rw); 
} 
     
unsigned long 
build_crc_table (int index) 
{ 
  int i; 
  unsigned long rb; 
     
  rb = reflect_32 (index); 
     
  for (i = 0; i < 8; i++){ 
    if (rb & 0x80000000L) 
      rb = (rb << 1) ^ CRC32C_POLY; 
    else 
      rb <<= 1; 
  } 
  return (reflect_32 (rb)); 
} 
 
main () 
{ 
  int i; 
     
  printf ("\nGenerating CRC-32c table file <%s>\n", OUTPUT_FILE); 
  if ((tf = fopen (OUTPUT_FILE, "w")) == NULL){ 
    printf ("Unable to open %s\n", OUTPUT_FILE); 
    exit (1); 
  } 
  fprintf (tf, "#ifndef __crc32cr_table_h__\n"); 
  fprintf (tf, "#define __crc32cr_table_h__\n\n"); 
  fprintf (tf, "#define CRC32C_POLY 0x%08lX\n", CRC32C_POLY); 
  fprintf (tf, "#define CRC32C(c,d) (c=(c>>8)^crc_c[(c^(d))&0xFF])\n"); 
  fprintf (tf, "\nunsigned long  crc_c[256] =\n{\n"); 
  for (i = 0; i < 256; i++){ 
    fprintf (tf, "0x%08lXL, ", build_crc_table (i)); 
    if ((i & 3) == 3) 
      fprintf (tf, "\n"); 
  } 
     
  fprintf (tf, "};\n\n#endif\n"); 
     
  if (fclose (tf) != 0) 
    printf ("Unable to close <%s>." OUTPUT_FILE); 
  else 
    printf ("\nThe CRC-32c table has been written to <%s>.\n", 
	    OUTPUT_FILE); 
} 
     
     
     
