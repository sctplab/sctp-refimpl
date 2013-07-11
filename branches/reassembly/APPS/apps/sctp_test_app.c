/*******************************************************************
 *      
 *   Description: Test application for SCTP
 *
 *   Date:   06/15/99
 *
 */
/******************************************************************
*	Test application for SCTP
******************************************************************/

/******************************************************************
* EXTERNAL REFERENCES
*******************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>
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
#include "distributor.h"
#include <netdb.h>
#include "sctpAdaptor.h"
#include "userInputModule.h"

int destinationSet = 0;
int portSet = 0;
int sendOptions = 0;
int bulkInProgress = 0;
int bulkPingMode = 0;
int bindSpecific = 0;
int v6only = 0;
int v4only = 0;
extern int sctp_verbose;
int
main(int argc, char **argv)
{
	sctpAdaptorMod *sctpA;
	distributor *dist;
	int i;
	int rwnd=0;
	int swnd=0;
	int sctpPort,mySctpPort,scopeVal;
	char *addr;
	char *binda[100];
	int tcpmodel = 0;

	scopeVal = mySctpPort = sctpPort = 0;
	addr = binda[0] = NULL;
	while((i= getopt(argc,argv,"v46h:p:m:b:?s:tr:z:")) != EOF)
	{
		switch(i)
		{
		case 'v':
			sctp_verbose = 1;
			break;
		case 'z':
			swnd = strtol(optarg,NULL,0);
			break;
		case 't':
			tcpmodel = 1;
			break;
		case 'r':
			rwnd = strtol(optarg,NULL,0);
			break;
		case 's':
			scopeVal = strtol(optarg,NULL,0);
			break;
		case 'h':
			addr = optarg;
			destinationSet++;
			break;
		case '4':
			v4only = 1;
			SCTP_setV4only();
			break;
		case '6':
			v6only = 1;
			SCTP_setV6only();
			break;
		case 'p':
			sctpPort = strtol(optarg, NULL, 10);
			destinationSet++;
			break;
		case 'm':
			mySctpPort = (u_short)strtol(optarg, NULL, 0);
			break;
		case 'b':
			binda[bindSpecific] = optarg;
			bindSpecific++;
			break;
		default:
		case '?':
			printf("Usage: %s [-h destAddress [-s v6scope]] [-p destport]\n"
			       "       [-m myPort] [-b bindAddress] [-6] [-4]\n"
			       "  -6 : v6 only (default is v4 + v6)\n"
			       "  -4 : v4 only\n",
			       argv[0]);
			return(0);
			break;
		}
	}
	sendOptions = 0;
	if (v6only && v4only) {
		printf("Can't set to v4 only and v6 only at the same time!\n");
		return(-4);
	}
	/* init the distibutor */
	dist = createDistributor();
	if(dist == NULL){
		printf("Can't create distributor return is NULL?\n");
		return(-1);
	}

	/* set the v6 scope */
	if (scopeVal)
		SCTP_setIPv6scope((u_int)scopeVal);

	/* validate the bind address */
	if (bindSpecific) {
		int i;
		for (i=0; i<bindSpecific; i++) {
			struct in6_addr in6;
			struct in_addr in;
			if (inet_pton(AF_INET6, binda[i], (void *)&in6)) {
				SCTP_setBind6Addr((u_char *)&in6);
			} else if (inet_pton(AF_INET, binda[i], (void *) &in)) {
				SCTP_setBindAddr(in.s_addr);
			} else {
				printf("Invalid bind address format [%s]!\n", binda[i]);
				return(-3);
			}
		}
	}
	/* now init the SCTP adaptor */
	if(tcpmodel)
		sctpA = create_SCTP_adaptor(dist,mySctpPort,SCTP_TCP_TYPE,rwnd,swnd);
	else
		sctpA = create_SCTP_adaptor(dist,mySctpPort,SCTP_UDP_TYPE,rwnd,swnd);

	if(sctpA == NULL){
		printf("Can't init the SCTP adaptor for port %d\n",
		       mySctpPort);
		return(-2);
	}
	/* get the destination address */
	if (addr != NULL) {
		struct in6_addr in6;
		struct in_addr in;
		if (inet_pton(AF_INET6, addr, (void *)&in6)) {
			SCTP_setIPaddr6((u_char *)&in6);
			destinationSet = 1;
		} else if (inet_pton(AF_INET, addr, (void *) &in)) {
			SCTP_setIPaddr(in.s_addr);
			destinationSet = 1;
		} else {
			printf("Invalid destination address format [%s]!\n", addr);
			return(-3);
		}
	}
	if (sctpPort) {
		SCTP_setport(htons(sctpPort)); 
		portSet = 1;
	}

	/* now init the user interface module */
	initUserInterface(dist,sctpA);

	/* start the main loop */
	dist_process(dist);
  
	/* if we reach here someone quit ... cleanup */

	destroyUserInterface();

	/* kill the sctp adaptor */
	destroy_SCTP_adaptor(sctpA);

	/* kill the distributor */
	destroyDistributor(dist);
	/* we are done */
	return(0);
}
