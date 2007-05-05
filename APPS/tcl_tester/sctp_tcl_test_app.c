#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>

#include <netinet/sctp.h>

void
useage(char *who)
{
	printf("Usage: %s [ options ]\n", who);
	printf("Current options:\n");
	printf("-m myport       - Set my port to bind to (defaults to 2222)\n");
	printf("-B ip-address   - Bind the specified address (else bindall)\n");
	printf("-s size         - send data at start of an assoc size is data write size (def none)\n");
	printf("-c cnt          - count of data to send at start (default 10)\n");
	printf("-4              - bind v4 socket\n");
	printf("-6              - bind v6 socket (default)\n");
	printf("-l              - don't connect, listen only (default)\n");
	printf("-h host-address - address of host to connect to (required if no -l)\n");
	printf("-p port         - of host to connect to (defaults to 2222)\n");
	printf("-L              - send LOOP requests\n");
	printf("-v              - Verbose please\n");
	printf("-S              - send SIMPLE data (default)\n");
}

union sctp_sockstore addr;
union sctp_sockstore bindto;
int verbose = 0;
int listen_only = 1;
uint16_t remote_port, local_port;
int size_to_send = 0;
int cnt_to_send = 10;
int use_v6=1;
int send_loop_request = 0;
char *conn_string = NULL;
char *bind_string = NULL;

int
main (int argc, char **argv)
{
	int i;

	remote_port = local_port = htons(2222);
	memset(&addr, 0, sizeof(addr));
	memset(&bindto, 0, sizeof(bindto));
	while((i= getopt(argc, argv,"lSLs:c:46m:p:vB:h:?")) != EOF) {
		switch(i) {
		case 'l':
			listen_only = 1;
			break;
		case 'S':
			send_loop_request = 0;
			break;
		case 'L':
			send_loop_request = 1;
			break;

		case 's':
			size_to_send = strtol(optarg, NULL, 0);
			break;
		case 'c':
			cnt_to_send = strtol(optarg, NULL, 0);
			break;
		case '4':
			use_v6 = 0;
			break;
		case '6':
			use_v6 = 1;
			break;

		case 'm':
			local_port = htons(((uint16_t)strtol(optarg, NULL, 0)));
			break;
		case 'p':
			remote_port = htons(((uint16_t)strtol(optarg, NULL, 0)));
			break;

		case 'v':
			verbose = 1;
			break;
		case 'B':
		{
			bind_string = optarg;
			if (inet_pton(AF_INET6, optarg, (void *)&bindto.sin6.sin6_addr) != 1) {
				if (inet_pton(AF_INET, optarg, (void *)&bindto.sin.sin_addr) != 1) {
					printf("-B Un-parsable address not AF_INET6/or/AF_INET4 %s\n",
					       optarg);
					printf("Default to bind-all\n");
					memset(&bindto, 0, sizeof(bindto));
					bind_string = NULL;
				}
			}			 
		}
		break;
		case 'h':
		{
			conn_string = optarg;
			if (inet_pton(AF_INET6, optarg, (void *)&addr.sin6.sin6_addr) != 1) {
				if (inet_pton(AF_INET, optarg, (void *)&addr.sin.sin_addr) != 1) {
					printf("-h Un-parsable address not AF_INET6/or/AF_INET4 %s\n",
					       optarg);
					listen_only = 1;
					printf("Will listen only\n");
				} else {
					listen_only = 0;
				}
			} else {
				listen_only = 0;
			}
		}
		break;
		default:
		case '?':
			useage(argv[0]);
			return(0);
			break;
		}
	}
	if(verbose) {
		if(listen_only) {
			printf("I will listen to port %d and not associate\n", ntohs(remote_port));
		} else {
			printf("I will try to keep a association up to address:%s port:%d until ctl-c\n",
			       conn_string, ntohs(remote_port));
		}
		if(bind_string == NULL) {
			printf("I will bind %s to addr:INADDR_ANY port:%d\n", 
			       ((use_v6) ? "IPv6" : "IPv4"),
			       ntohs(local_port));
		} else {
			printf("I will bind %s to addr:%s port:%d\n", 
			       ((use_v6) ? "IPv6" : "IPv4"),
			       bind_string, ntohs(local_port));
		}
		if(size_to_send) {
			printf("When an association begins, I will send %d %s records of %d bytes\n",
			       cnt_to_send, (send_loop_request ? "loop request" : "simple"  ),size_to_send);
		} else {
			printf("I will NOT send any data at association startup\n");
		}
	}

}
