#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int
main(int argc, char **argv)
{
	int i, cnt;
	char *infile=NULL;
	char *outfile=NULL;
	FILE *in, *out;
	char buffer[1024];

	while ((i = getopt(argc, argv, "i:o:?")) != EOF) {
		switch (i) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case '?':
			printf("%s [-i inputfile -o outfile]\n", argv[0]);	
			return (0);
		};
	};
	if (infile == NULL) {
		in = stdin;
	} else {
		in = fopen(infile, "r");
		if (in == NULL) {
			printf("Can't open %s\n", infile);
			return (-1);
		}
	}
	if (outfile == NULL) {
		out = stdout;
	} else {
		out = fopen(outfile, "w+");
		if (in == NULL) {
			printf("Can't open %s\n", outfile);
			return (-1);
		}
	}
	cnt = 0;
	while(fgets(buffer, sizeof(buffer), in) != NULL) {
		cnt++;
		if (buffer[0] == '\n')
			continue;
		if (fputs(buffer, out) == EOF) {
			printf("Write error at line:%d\n", cnt);
			return(-1);
		}
	}
	if (infile) {
		fclose(in);
	}
	if (outfile) {
		fclose(out);
	}
	return (0);
}
