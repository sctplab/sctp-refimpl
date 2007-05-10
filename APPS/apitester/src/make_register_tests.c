
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#define IO_BUFFER_SIZE 65535
#define TEXT_BUFFER_SIZE 65535
#define HEADER1 "#include \"api_tests.h\"\n\n"
#define HEADER2 "\nstruct test all_tests[] = {\n"
#define HEADER3 "};\nunsigned int number_of_tests = (sizeof(all_tests) / sizeof(all_tests[0]));\n"

static char io_buffer[IO_BUFFER_SIZE];
static char text_buffer[TEXT_BUFFER_SIZE];
FILE *registerfile;

char* myread(FILE *stream) {
	char *text = text_buffer;
	*text = '\0';
	while(fgets(io_buffer, IO_BUFFER_SIZE, stream)) {
		strcat(text, io_buffer);
	}
	return text;
}

void fill_file(char* test, int mode)
{
char header[100], declare[100];
char *tests, *line;
FILE *testfile;
testfile = fopen("tests.txt", "r");
	if (!testfile) 
	{
		fprintf(stderr, "Cannot open file [%s] for writing: %s\n", "tests.txt", strerror(errno));
		exit(-1);
	}
	else
	{
		tests = myread(testfile);
		fclose (testfile);
		strcpy(header, test);
		fwrite(header, strlen(header),1,registerfile);
		line = strtok (tests, "(");
		while (line)
		{	
			line = strtok(NULL, ")");
			if (line)
			{
				declare[0] = '\0';
				if (mode==1)
				{
					strcat(declare,"DECLARE_APITEST(");
					strcat(declare,line);
					strcat(declare,");\n");
				}
				else
				{
					strcat(declare,"\tREGISTER_APITEST(");
					strcat(declare,line);
					strcat(declare,"),\n");
				}
				fwrite(declare, strlen(declare), 1, registerfile);
				line = strtok(NULL, "(");
			}
			else
				break;	
		}
		testfile = NULL;
	}
}


int main()
{

	char header[100];
	
	system("grep \"DEFINE\" test* > tmptests.txt");
	registerfile = fopen("register_tests.c","w");
	if (!registerfile) 
	{
		fprintf(stderr, "Cannot open file [%s] for writing: %s\n", "register.c", strerror(errno));
		exit(-1);
	}
	fill_file(HEADER1, 1);
	fill_file(HEADER2,0);
	strcpy(header, HEADER3);
	fwrite(header, strlen(header),1,registerfile);
	fclose (registerfile);
	system("rm tmptests.txt");
	return 0;
}

