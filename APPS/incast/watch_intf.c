/*-
 * Copyright (c) 2011, by Randall Stewart.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of the authors nor the names of its 
 *    contributors may be used to endorse or promote products derived 
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <getopt.h>

int
main(int argc, char **argv)
{
	int i;
	size_t len;
	int headat, tailat;
	char *head=NULL, *tail=NULL;
	int ring_size=4096;
	int largest = -1, calc;
	int transmit = -1;
	while((i= getopt(argc,argv,"h:t:r:TR")) != EOF) {
		switch(i) {
		case 'T':
			transmit = 1;
			break;
		case 'R':
			transmit = 0;
			break;
		case 'r':
			ring_size = strtol(optarg, NULL, 0);
			break;
		case 'h':
			head = optarg;
			break;
		case 't':
			tail = optarg;
			break;
		case '?':
		default:
		out:
			printf("Use %s -T -R -h head.sys.ctl -t tail.sys.ctl (-r ringsize)\n",
			       argv[0]);
			return (-1);
		}
	}
	if ((head == NULL) || (tail == NULL) || (transmit == -1)) {
		goto out;
	}

	while (1) {
		len = sizeof(headat);
		if (sysctlbyname(head, &headat, 
				 &len, NULL, 0) < 0) {
			printf("Error %d (%s) sysctl failed '%s' \n", 
			       errno, strerror(errno), head);
			break;
		}
		len = sizeof(tailat);
		if (sysctlbyname(tail, &tailat, 
				 &len, NULL, 0) < 0) {
			printf("Error %d (%s) sysctl failed '%s' \n", 
			       errno, strerror(errno), tail);
			break;
		}
		if (transmit) {
			if (tailat >= headat) {
				calc = (tailat - headat);
			} else {
				calc = tailat + (ring_size - headat);
			}
		} else {
			if (headat > tailat) {
				calc = headat - (tailat+1);
			} else if (headat == tailat) {
				calc = 0; /* hit it while being removed */
			} else {
				calc = headat + (ring_size - (tailat+1));
			}
		}
		if (calc > 2000) {
			printf("?");
		}
		if (calc > largest) {
			largest = calc;
			printf("%d\r", largest);
			fflush(stdout);
		}
	}
	printf("\n");
	return (0);
}
