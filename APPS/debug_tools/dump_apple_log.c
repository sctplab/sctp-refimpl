#include <stdio.h>
#include <sys/types.h>
#include <sys/errno.h>

struct sctp_dump_log {
	u_int64_t timestamp;
	const char *descr;
	uint32_t subsys;
	uint32_t params[6];
};

int
main(int argc, char **argv)
{
	FILE *io;
	struct sctp_dump_log log;
	int cnt=0;
	if(argc < 2) {
		printf("Error use %s file\n", argv[0]);
		return (-1);
	}
	io = fopen(argv[1], "r");
	if(io == NULL) {
		printf("Can't open trace file %s err:%d\n", argv[1], errno);
	}
	while(fread(&log, sizeof(log), 1, io) > 0) {
		printf("%d %llu SCTP:%d[%d]:%x-%x-%x-%x\n",
		       cnt,
		       log.timestamp,
		       log.params[0],
		       log.params[1],
		       log.params[2],
		       log.params[3],
		       log.params[4],
		       log.params[5]);
	}
	fclose(io);
	return (0);
}
