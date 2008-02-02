#include <stdio.h>
#include <sys/types.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <string.h>
#ifndef SCTP_MAX_LOGGING_SIZE
#define SCTP_MAX_LOGGING_SIZE 0
#define NO_LOGGING_PRINT 1}
/* this means the logging option is disabled */
#endif
int
main(int argc, char **argv)
{
#ifndef NO_LOGGING_PRINT
	struct sctp_log log;
	unsigned int i;
	size_t len = sizeof(struct sctp_log);


	if (sysctlbyname("net.inet.sctp.log", &log, &len, NULL, 0) < 0) {
		printf("Error %d (%s) could not get the log.\n", errno, strerror(errno));
		return(0);
	}

	for (i = 0; i < SCTP_MAX_LOGGING_SIZE; i++) {
		printf("%d %ju SCTP:%d[%d]:%x-%x-%x-%x\n",
		       i,
		       log.entry[i].timestamp,
		       log.entry[i].params[0],
		       log.entry[i].params[1],
		       log.entry[i].params[2],
		       log.entry[i].params[3],
		       log.entry[i].params[4],
		       log.entry[i].params[5]);
	}
#else
        printf("Apple dump logging not supported\n");
#endif
	return (0);
}
