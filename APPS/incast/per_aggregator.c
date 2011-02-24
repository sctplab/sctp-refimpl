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
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/errno.h>
struct list {
	int txfr;
	int indx;
	int gain;
	double gainpercentage;
};

int
main(int argc, char **argv)
{
	int smallest, index_of_small;
	char buffer[1024];
	FILE *io;
	char *file=NULL, *stop;
	uint64_t bytes;
	int txfr, i, j;
	struct list *listp=NULL;

	bytes = 0;

	while ((i = getopt(argc, argv, "h:i:?")) != EOF) {
		switch (i) {
		case 'h':
			txfr = strtol(optarg, NULL,0);
			listp = malloc((sizeof(struct list)) * txfr);
			if (listp == NULL) {
				printf("Malloc of %d entries fails\n",
				       txfr);
			}
			break;
		case 'i':
			file = optarg;
			break;
		case '?':
		default:
		use:
			printf("Use %s -i file-to-sum -h how-many\n",
			       argv[0]);
		exit(-1);
		break;
		};
	};
	if (file == NULL) {
		goto use;
	}
	if (listp == NULL) {
		goto use;
	}

	io = fopen(file, "r");
	if (io == NULL) {
		printf("Can't open '%s' err:%d\n", file, errno);
		return (-1);
	}
	stop = NULL;
	i = 0;
	while (fgets(buffer, sizeof(buffer), io) != NULL) {
		listp[i].indx = strtol(buffer, &stop, 0);
		if (stop == NULL) {
			printf("Huh\n");
			return (-1);
		}
		stop++;
		listp[i].txfr = strtol(stop, NULL, 0);
		i++;
	}
	fclose(io);
	smallest = listp[0].txfr;
	index_of_small = 0;
	for(j=0; j<i; j++) {
		if(smallest > listp[j].txfr) {
			index_of_small = j;
			smallest = listp[j].txfr;
		}
	}
	/* Now we have the smallest calculate the gains */
	for(j=0; j<i; j++) {
		listp[j].gain = listp[j].txfr - smallest;
	}
	/* Now generate the percentages */
	for(j=0; j<i; j++) {
		if (listp[j].gain) {
			listp[j].gainpercentage = (100.0 * listp[j].gain)/(listp[j].txfr * 1.0);
		} else {
			listp[j].gainpercentage = 0.0;
		}
	}
	for (j=0; j<i; j++) {
		printf("%d %d %f\n",
		       listp[j].indx,
		       listp[j].txfr,
		       listp[j].gainpercentage);
	}
	return (0);
}
