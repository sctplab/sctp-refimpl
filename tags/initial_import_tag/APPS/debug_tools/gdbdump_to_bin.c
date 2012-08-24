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

void
collect_four(FILE *out, char *str, int line)
{
	/* collect 4 unsigned long ints
	 * from the string and write them out.
	 */
	int len, got=0, maybe=0,i;
	uint32_t collected;
	char *begin=NULL;

	len = strlen(str);
	for(i=0;i<len;i++) {
		if(maybe == 0) {
			if((str[i] == ' ') || (str[i] == 0x09)) {
				continue;
			}
		} else {
			if((str[i] == ' ') || (str[i] == 0x09) || (str[i] == '\n')) {
				str[i] = 0;
				collected = strtoul(begin, NULL, 0);
				fwrite(&collected, sizeof(collected), 1, out);
				got++;
				if(got == 4) {
					/* done */
					fflush(out);
					return;
				}
				maybe = 0;
			}
			continue;
		}
		maybe = 1;
		begin = &str[i];
	}
	printf("hmm only collected %d at line %d?\n", got, line);
}


int
main(int argc, char **argv)
{
	int i,cnt=0, len;
	FILE *in, *out;
	uint32_t initial_address, position;
	int initial_set=0;
	char linebuffer[200];
	char *infile=NULL, *outfile=NULL, *pos;

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
		position = strtoul(linebuffer, NULL, 0);
		if(position == 0) {
			continue;
		}
		if(initial_set == 0) {
			initial_address = position;
			printf("Starting collection at address %x line:%d\n",
			       initial_address, cnt);
			initial_set = 1;
		} else {
			if ((initial_address + 16) != position) {
				printf("Next postion is 0x%x not expected %x line:%d -- aborting\n",
				       position, (initial_address+16), cnt);
			out_now:
				fclose(in);
				fclose(out);
				return(-4);
			} else {
			        initial_address = position;
			}
			
		}
		pos = strtok(linebuffer, ":");
		if(pos == NULL) {
			printf("Can't find ':' at line %d? - bye\n",
			       cnt);
			goto out_now;
		}
		len = strlen(pos);
		collect_four(out, &linebuffer[(len+1)], cnt);
	}
	fclose(in);
	fclose(out);
	printf("Collection ends at line %d address %x\n", cnt, (initial_address+16));
	return (0);
}
