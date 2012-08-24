#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <string.h>
#include <stdarg.h>


int
main(int argc, char **argv)
{
	int i,cnt=0, len, times, ccnt=0;
	FILE *in, *out;
	char *infile=NULL, *outfile=NULL;
	char buf[3], linebuffer[200];
	uint8_t val;

	while((i= getopt(argc,argv,"i:o:")) != EOF)
	{
		switch(i) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;

		default:
			break;
		};
	}

	if ((infile == NULL) || (outfile == NULL)) {
		printf("Use %s -i input-from-gdb -o binary-output-file\n",
		       argv[0]);
		return(-1);
	}
	in = fopen(infile, "r");
	if(in == NULL) {
		printf("Can't open input file %s error:%d -- goodbye\n",
		       infile, errno);
		return(-2);
	}
	out = fopen(outfile, "w+");
	if(out == NULL) {
		printf("Can't open output file %s error:%d -- goodbye\n",
		       outfile, errno);
		return(-3);
	}

	while(fgets(linebuffer, sizeof(linebuffer), in)) {
		cnt++;
		if(linebuffer[0] == '\n')
			continue;
		len = strlen(linebuffer);
		if(linebuffer[(len-1)] == '\n') {
			linebuffer[(len-1)] = 0;
		}
		len = strlen(linebuffer);
		if (len % 2) {
			printf("Line %d not an even number of char's (%d) - skip\n", cnt, len);
			continue;
		}
		times = len/2;
		for(i=0; i<times; i++) {
			strncpy(buf, &linebuffer[(i*2)], 2);
			ccnt++;
			buf[2] = 0;
			val = strtoul(buf, NULL, 16);
			fwrite(&val, 1, 1, out);
		}
	}
	fclose(in);
	fclose(out);
	printf("Collection end lines %d, byte-cnt %d\n", cnt, ccnt);
	return (0);
}
