#include <sys/types.h>
#include <stdio.h>
#include "crcmodel.h"

int 
main(int argc, char **argv)
{
  int i,x;
  int refin,refout;
  char string[20]="123456789";
  char test1[38];
  char test2[48];
  cm_t crc32c,*p_crc32c,*p_cm;
  u_int32_t crc;

  refin = refout = 0;
  if(argc < 3){
    printf("Need args [TRUE/FALSE TRUE/FALSE]\n");
    return(-1);
  }
  if((strcmp(argv[1],"true") == 0) ||
     (strcmp(argv[1],"TRUE") == 0)){
    refin = 1;
  }
  if((strcmp(argv[2],"true") == 0) ||
     (strcmp(argv[2],"TRUE") == 0)){
    refout = 1;
  }
  memset(test1,0,sizeof(test1));
  memset(test2,0,sizeof(test2));
  x = 1;
  for(i=13;i<sizeof(test2);i++){
    test2[i] = (char)x;
  }
 p_crc32c = &crc32c;
  /*
    CRC32-c
  p_crc32c->cm_width = 32;
  p_crc32c->cm_poly  = 0x1EDC6F41;
  p_crc32c->cm_init  = 0xFFFFFFFF;
  p_crc32c->cm_refin = refin;
  p_crc32c->cm_refot = refout;
  p_crc32c->cm_xorot = 0x00000000;
  */
  /*
   Name   : "CRC-32"
   Width  : 32
   Poly   : 04C11DB7
   Init   : FFFFFFFF
   RefIn  : True
   RefOut : True
   XorOut : FFFFFFFF
   Check  : CBF43926

   */
  p_crc32c->cm_width = 32;
  p_crc32c->cm_poly  = 0x04C11DB7;
  p_crc32c->cm_init  = 0xFFFFFFFF;
  p_crc32c->cm_refin = refin;
  p_crc32c->cm_refot = refout;
  p_crc32c->cm_xorot = 0xFFFFFFFF;
  cm_ini(p_crc32c);
  p_cm = p_crc32c;
  for(i=0;i<9;i++){
    printf("%c",string[i]);
    cm_nxt(p_cm,string[i]);
  }
  crc = cm_crc(p_cm);
  printf("\ncrc value is %x\n",crc);
  return(0);
}
