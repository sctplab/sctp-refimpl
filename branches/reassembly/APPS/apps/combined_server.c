/*
 * Copyright (C) 2006 Michael Tuexen, tuexen@fh-muenster.de,
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

 
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <net/route.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#if defined (SOLARIS)
#include <strings.h>
#endif

#define ECHO_PORT    7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define CHARGEN_PORT 19

#define SECONDS_TO_CLEANUP 5
#define BUFFER_SIZE 1024
#define MAXIMUM_NUMBER_OF_LOCAL_ADDRESSES    1 /* do not change this */

static int verbose  = 0;
static int vverbose = 0;
static int tooManyAddresses = 0;
static int unknownCommand = 0;
static struct sockaddr_in local_addr[MAXIMUM_NUMBER_OF_LOCAL_ADDRESSES];
static unsigned short noOfLocalAddresses = 0;

struct server_data {
    int fd;
    void * (* connection_handler)(void *);
};



void printUsage(void)
{
    printf("Usage:   combined_server [options]\n");
    printf("options:\n");
    printf("-s       source address\n");
    printf("-v       verbose mode\n");   
    printf("-V       very verbose mode\n");   
}

void getArgs(int argc, char **argv)
{
    int c;
    extern char *optarg;
/*  extern int optind;*/

    while ((c = getopt(argc, argv, "hs:vV")) != -1)
    {
        switch (c) {
        case 'h':
            printUsage();
            exit(0);
        case 's':
            if (noOfLocalAddresses < MAXIMUM_NUMBER_OF_LOCAL_ADDRESSES) {
                    bzero(&local_addr[noOfLocalAddresses], sizeof(local_addr[0]));
#if !defined (LINUX) && !defined (SOLARIS)
                    local_addr[noOfLocalAddresses].sin_len    = sizeof(local_addr[0]);
#endif
                    local_addr[noOfLocalAddresses].sin_family = AF_INET;
                    if (inet_aton(optarg, &(local_addr[noOfLocalAddresses].sin_addr)) == 1) {
                        noOfLocalAddresses++;
                    }
            } else {
                tooManyAddresses = 1;
            }
            break;  
        case 'v':
            verbose = 1;
            break;
        case 'V':
            verbose = 1;
            vverbose = 1;
            break;
        default:
            unknownCommand = 1;
            break;
       }
    }
}

void checkArgs(void)
{
    int abortProgram;
    int printUsageInfo;
    
    abortProgram = 0;
    printUsageInfo = 0;
    
    if (noOfLocalAddresses == 0) {
      bzero(&local_addr[noOfLocalAddresses], sizeof(local_addr[0]));    
#if !defined (LINUX) && !defined (SOLARIS)
      local_addr[noOfLocalAddresses].sin_len    = sizeof(local_addr[0]);
#endif
      local_addr[noOfLocalAddresses].sin_family = AF_INET;
      inet_aton("0.0.0.0", &(local_addr[noOfLocalAddresses].sin_addr));
      noOfLocalAddresses = 1;
    }
    if (tooManyAddresses == 1) {
        printf("Error:   Too many source addresses were specified. Taking only the %d first ones.\n",
               MAXIMUM_NUMBER_OF_LOCAL_ADDRESSES);
    }
    if (unknownCommand ==1) {
         printf("Error:   Unkown options in command.\n");
         printUsageInfo = 1;
    }
    
    if (printUsageInfo == 1)
        printUsage();
    if (abortProgram == 1)
        exit(-1);
}

static void*
handle_chargen(void *arg)
{
    int fd;
    int n, length;
    char buffer[BUFFER_SIZE];
    struct sctp_status status;
    int len;
    
    fd = *((int *) arg);
    free(arg);
    
    pthread_detach(pthread_self());
    
    len = sizeof(struct sctp_status);
    getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, (void *)&status, &len);

    length = 1 + (random() % 512);
    memset(buffer, 'A', length);
    buffer[length-1] = '\n';

    while ((n = sctp_sendmsg(fd, buffer, length, NULL, 0, 0, 0, random()%status.sstat_outstrms, 0, 0)) >  0){
        if (vverbose) {
            printf("%-6d: sending %u bytes.\n", fd, n);
            fflush(stdout);
        }
        if (n != length) {
            printf("%-6d: Writing %u bytes failed.\n", fd, n);
        }
        length = 1 + (random() % 512);
        memset(buffer, 'A', length);
        buffer[length-1] = '\n';
    }
    close(fd);
    if (verbose) {
        printf("%-6d: Connection closed.\n", fd);
        fflush(stdout);
    }
    return(NULL);
}

static void*
handle_discard(void *arg)
{
    int fd;
    int n;
    char buffer[BUFFER_SIZE];
    
    fd = *((int *) arg);
    free(arg);
    
    pthread_detach(pthread_self());
        
    while ((n=read(fd, buffer, sizeof(buffer))) >  0){
        if (vverbose) {
            printf("%-6d: Discarding %u bytes.\n", fd, n);
            fflush(stdout);
        }
    }
    
    close(fd);
    if (verbose) {
        printf("%-6d: Connection closed.\n", fd);
        fflush(stdout);
    }
    return(NULL);
}

static void*
handle_echo(void *arg)
{
    int fd;
    int n;
    char buffer[BUFFER_SIZE];
    struct sctp_status status;
    struct sctp_sndrcvinfo sndrcvinfo;
    struct sockaddr_in from_addr;
    int len, flags;
    
    fd = *((int *) arg);
    free(arg);
    
    pthread_detach(pthread_self());
    
    len = sizeof(struct sctp_status);
    getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, (void *)&status, &len);
    
    len = sizeof(struct sockaddr_in);
    while ((n=sctp_recvmsg(fd, buffer, sizeof(buffer), (struct sockaddr *)&from_addr, &len, &sndrcvinfo, &flags)) >  0){
        if (vverbose) {
            printf("%-6d: Echoing %u bytes.\n", fd, n);
            fflush(stdout);
        }
        if (n != sctp_sendmsg(fd, buffer, n, NULL, 0, sndrcvinfo.sinfo_ppid, sndrcvinfo.sinfo_flags, sndrcvinfo.sinfo_stream % status.sstat_outstrms, 0, 0)) {
            printf("%-6d: Writing %u bytes failed.\n", fd, n);
        }
        len = sizeof(struct sockaddr_in);
    }
    close(fd);
    if (verbose) {
        printf("%-6d: Connection closed.\n", fd);
        fflush(stdout);
    }
    return(NULL);
}

static void*
handle_daytime(void *arg)
{
    int fd;
    int timeAsStringLen;
    char *timeAsString;
    time_t now;

    fd = *((int *) arg);
    free(arg);
    
    pthread_detach(pthread_self());
    
    time(&now);
    timeAsString = ctime(&now);
    timeAsStringLen = strlen(timeAsString);
    
    if (timeAsStringLen != write(fd, timeAsString, timeAsStringLen)) {
            printf("%-6d: Writing the daytime failed.\n", fd);
    }
    if (vverbose) {
        printf("%-6d: Daytime sent back: %s", fd, timeAsString);
        fflush(stdout);
    }

    close(fd);
    if (verbose) {
        printf("%-6d: Connection closed.\n", fd);
        fflush(stdout);
    }
    return(NULL);
}

static void*
start_server(void *arg)
{
    pthread_t tid;
    int *fdp;
    struct sockaddr_in remote_addr;
    int remote_addr_len;
    struct server_data data;
    
    data = *((struct server_data *) arg);
    free(arg);

    bzero(&remote_addr, sizeof(remote_addr));
    remote_addr_len = sizeof(remote_addr);
    for(;;){
        fdp = malloc(sizeof(int));
        if((*fdp=accept(data.fd, (struct sockaddr *) &remote_addr, &remote_addr_len)) < 0)
            perror("accept call failed");
        if (verbose) {
            printf("%-6d: Connection accepted from %s:%u.\n", *fdp, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
            fflush(stdout);
        }
        pthread_create(&tid, NULL, data.connection_handler, fdp);
    }
    return(NULL);
}

static pthread_t
init_server(struct sockaddr_in local_addr, int port, void *(connection_handler ()))
{
    pthread_t tid;
    int fd;
    struct server_data *server_data_p;
    
    if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
        perror("socket call failed");
    
    local_addr.sin_port= htons(port);
    if(bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");

    if(listen(fd, 1) < 0)
        perror("listen call failed");
        
    server_data_p = malloc(sizeof(struct server_data));
    server_data_p->fd = fd;
    server_data_p->connection_handler = connection_handler;
    if (verbose) {
        printf("Starting server on port %u.\n", port);
        fflush(stdout);
    }
    pthread_create(&tid, NULL, &start_server, (void *)server_data_p);
    return(tid);
}

int main (int argc, char * argv[])
{
    pthread_t echo_tid, discard_tid, daytime_tid, chargen_tid;
    
    getArgs(argc, argv);
    checkArgs();
    
    signal(SIGPIPE, SIG_IGN);
    echo_tid    = init_server(local_addr[0], ECHO_PORT,    &handle_echo);
    discard_tid = init_server(local_addr[0], DISCARD_PORT, &handle_discard);
    daytime_tid = init_server(local_addr[0], DAYTIME_PORT, &handle_daytime);
    chargen_tid = init_server(local_addr[0], CHARGEN_PORT, &handle_chargen);
            
    pthread_join(echo_tid, NULL);
    pthread_join(discard_tid, NULL);
    pthread_join(daytime_tid, NULL);
    pthread_join(chargen_tid, NULL);

    return 0;
}
