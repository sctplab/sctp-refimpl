#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE (1<<16)

static unsigned short port      = 0;
static unsigned int   number    = 1;
static unsigned short errornous = 0;
static unsigned short drop      = 0;
static unsigned short verbose   = 0;
static unsigned short showUsage = 0;
static unsigned short offset    = 0;

struct common_header {
  unsigned short source_port;
  unsigned short destination_port;
  unsigned long  verification_tag;
  unsigned long  checksum;
};

struct pktdrp_header {
  unsigned char  type;
  unsigned char  flags;
  unsigned short length;
  unsigned long  bandwidth;
  unsigned long  size;
  unsigned short truncated_length;
  unsigned short reservered;
  unsigned char  packet[0];
};

void printUsage(void)
{
    printf("Usage:   emulator -p port [options]\n");
    printf("options:\n");
    printf("-d        discard and send packet drop report (default no)\n");
    printf("-e        introduce CRC/checksum errors (default: no)\n");
    printf("-h        show this message\n");
    printf("-n number send each packet number times (default: 1 )\n");
    printf("-o offset modify the byte with the given offset (default 0)\n");
    printf("-v        verbose mode                  (default: no)\n");   
}

void getArgs(int argc, const char * argv[])
{
    int c;
    extern char *optarg;
/*  extern int optind;*/

    while ((c = getopt(argc, (char * const *)argv, "ehn:o:p:v")) != -1)
    {
        switch (c) {
        case 'd':
            drop =1;
            break;
        case 'e':
            errornous = 1;
            break;
        case 'h':
            showUsage = 1;
            break;
        case 'p':
            port = atoi(optarg);
            break;  
        case 'n':
            number = atoi(optarg);
            break;  
        case 'o':
            offset = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            showUsage = 1;
            break;
       }
    }
}

void checkArgs(void)
{    
  if (port == 0) {
    fprintf(stderr, "Error: No port number specified.\n");
    printUsage();
    exit(-1);
  }
  if (showUsage) {
    printUsage();
    exit(1);
  }
}

int main(int argc, const char * argv[])
{
  int fd, n, i, addr_len;
  struct sockaddr_in addr;
  char buffer[BUFFER_SIZE];
  struct ip *ip_header;
  struct common_header *common_header;
  struct pktdrp_header *pktdrp_header;

  
  getArgs(argc, argv);
  checkArgs();
  
  if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT)) < 0)
    perror("socket call failed");

  bzero(&addr, sizeof(addr));
  addr.sin_len         = sizeof(addr);
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port        = htons(port);
  
  if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    perror("bind call failed");
  
  bzero(&addr, sizeof(addr));
  addr_len = sizeof(addr);
  ip_header = (struct ip *)buffer;
  while ((n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addr_len)) >  0){
    if (verbose) {
      printf("Received %u bytes from %s.\n", n, inet_ntoa(addr.sin_addr));
      fflush(stdout);
    }
    if ((errornous) && ((offset + (ip_header->ip_hl << 2)) < n)) {
      buffer[offset + (ip_header->ip_hl << 2)]++;
      if (verbose) {
        printf("Modified the packet\n");
        fflush(stdout);
      }
    }
   
    /*
    if (drop) {
        memmove(buffer + (ip_header->ip_hl << 2) + 
                         sizeof(struct common_header) + 
                         sizeof(struct pktdrp_header), buffer + (ip_header->ip_hl << 2), n - (ip_header->ip_hl << 2));
        common_header =  (struct common_header *) (buffer + (ip_header->ip_hl << 2));
        pktdrp_header =  (struct pktdrp_header *) (buffer + (ip_header->ip_hl << 2) + sizeof(struct common_header));
        common_header->source_port = ;
        
    }
    */
    for(i = 0; i < number; i++) {
      if (n != sendto(fd, buffer, n, 0, (struct sockaddr *) &addr, addr_len)) {
        fprintf(stderr, "Writing %u bytes failed.\n", n);
        fflush(stderr);
      }
    }
    if ((verbose) && (number > 0)) {
      printf("Sent %u bytes from %s %u times.\n", n, inet_ntoa(addr.sin_addr), number);
      fflush(stdout);
    }      
  }
  if (close(fd) < 0)
    perror("close call failed");
  return 0;
}
