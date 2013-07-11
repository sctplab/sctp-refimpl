    /* Example of table build routine */ 
     
#include <stdio.h> 
#include <stdlib.h> 
     
#define CRC32C_POLY    0x1EDC6F41L 
     
unsigned long 
reflect_32 (unsigned long b) 
{ 
	int i; 
	unsigned long rw = 0L; 
     
	for (i = 0; i < 32; i++) { 
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
     
	    for (i = 0; i < 8; i++) { 
		    if (rb & 0x80000000L) 
			    rb = (rb << 1) ^ CRC32C_POLY; 
		    else 
			    rb <<= 1; 
	    } 
	    return (reflect_32 (rb)); 
} 

int
main (int argc, char **argv) 
{ 
	int i,sztbl; 
	FILE *tf;
	
	if(argc < 2) {
		printf("Use %s file-to-create\n",argv[0]);
		return(-1);
	} 
	sztbl = 256;
	if(argc > 2) {
		sztbl = strtol(argv[2],NULL,0);
	}
	if( !(sztbl == 256) &&
	    !(sztbl == 65536)) {
		printf("Invalid table size 256 or 65536\n");
		return(-1);
	}

	printf ("\nGenerating CRC-32c table file <%s> size %d\n", argv[1], sztbl); 
	if ((tf = fopen (argv[1], "w")) == NULL) { 
		printf ("Unable to open %s\n", argv[1]); 
		return(-1);
        }
	fprintf (tf, "\nunsigned long  crc_c[%d] =\n{\n", sztbl); 
	for (i = 0; i < sztbl; i++) { 
		fprintf (tf, "0x%08lXL, ", build_crc_table (i)); 
		if ((i & 3) == 3) 
			fprintf (tf, "\n"); 
        } 
	fprintf (tf, "};\n\n#endif\n"); 
	fclose (tf);
	return(0);
} 
