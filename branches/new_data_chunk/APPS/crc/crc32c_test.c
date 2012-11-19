#include <stdio.h>
#include "crc32cr.h"

struct SCTP_Header
{
short int src_port;
short int dst_port;
unsigned long verification;
unsigned long checksum;
};

typedef struct
{
struct SCTP_Header common_header;
unsigned long chunks[];
}        SCTP_message;


#define NMIN 12
#define NMAX 65536

#define _LITTLE_ENDIAN 1

#ifdef _LITTLE_ENDIAN

#define ntohl(a) ((((a) & 0xff) << 24) | (((a) & 0xff00) << 8) | \
			(((a) & 0xff0000) >> 8) | ((a) >> 24))

#define htonl(a) ((((a) & 0xff) << 24) | (((a) & 0xff00) << 8) | \
			(((a) & 0xff0000) >> 8) | ((a) >> 24))
#else

#define ntohl(a) (a)

#define htonl(a) (a)

#endif

unsigned char t_0[32];


unsigned char t_1[44] =
{
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
0x00, 0x01, 0x02, 0x03,
0x04, 0x05, 0x06, 0x07,
0x08, 0x09, 0x0a, 0x0b,
0x0c, 0x0d, 0x0e, 0x0f,
0x10, 0x11, 0x12, 0x13,
0x14, 0x15, 0x16, 0x17,
0x18, 0x19, 0x1a, 0x1b,
0x1c, 0x1d, 0x1e, 0x1f,
};




/* Example of crc insertion */


int
insert_crc32(unsigned char *buffer, unsigned int length)
{
SCTP_message *message;
unsigned int i;
unsigned long crc32 = ~0L;

   /* check packet length */
if (length > NMAX || length < NMIN)
   return -1;

message = (SCTP_message *) buffer;
message->common_header.checksum = 0L;

for (i = 0; i < length; i++)
   {
   CRC32C(crc32, buffer[i]);
   }

   /* and insert it into the message */
message->common_header.checksum = htonl(crc32);
return 1;
}

/* Example of crc validation */
/* Test of 32 zeros should yield 0x756EC955 placed in network order */
/* 13 zeros followed by bytes values of 1 - 0x1F should yield 0x5B988D47 */

int
validate_crc32(unsigned char *buffer, unsigned int length)
{
SCTP_message *message;
unsigned int i;
unsigned long original_crc32;
unsigned long crc32 = ~0L;

   /* check packet length */
if (length > NMAX || length < NMIN)
   return -1;

   /* save and zero checksum */
message = (SCTP_message *) buffer;
original_crc32 = ntohl(message->common_header.checksum);
message->common_header.checksum = 0L;

for (i = 0; i < length; i++)
   {
   CRC32C(crc32, buffer[i]);
   }

return ((original_crc32 == crc32) ? 1 : -1);
}



main()
{
printf("\nTesting SCTP CRC-32c\n");

insert_crc32(t_0, 32);

printf(" %02X %02X %02X %02X should be 75 6E C9 55\n",
       t_0[8], t_0[9], t_0[10], t_0[11]);


insert_crc32(t_1, 44);

printf(" %02X %02X %02X %02X should be 5B 98 8D 47\n",
       t_1[8], t_1[9], t_1[10], t_1[11]);

if (validate_crc32(t_0, 32) == 1)
   printf("T_0 Value confirmed\n");

if (validate_crc32(t_1, 44) == 1)
   printf("T_1 Value confirmed\n");

}
