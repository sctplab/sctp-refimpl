#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <ext_socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#if defined (LINUX)
    #include <time.h>
#endif


#define ECHO_PORT    7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define CHARGEN_PORT 19

#define BUFFER_SIZE  3000

int main (int argc, const char * argv[]) {
  int chargen_fd, daytime_fd, discard_fd, echo_fd, backlog, remote_addr_size, length, idle_time, nfds;
  struct sockaddr_in local_addr, remote_addr;
  char buffer[BUFFER_SIZE];
  fd_set rset;
  time_t now;
  char *time_as_string;
  struct sctp_event_subscribe evnts; 

  /* get sockets */
  if ((chargen_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((daytime_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((discard_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((echo_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");

  /* disable all event notifications */
  bzero(&evnts, sizeof(evnts));
  evnts.sctp_data_io_event = 1;
  ext_setsockopt(chargen_fd,IPPROTO_SCTP, SCTP_EVENTS,
		 &evnts, sizeof(evnts));

  ext_setsockopt(daytime_fd,IPPROTO_SCTP, SCTP_EVENTS,
		 &evnts, sizeof(evnts));

  ext_setsockopt(discard_fd,IPPROTO_SCTP, SCTP_EVENTS,
		 &evnts, sizeof(evnts));

  ext_setsockopt(echo_fd,IPPROTO_SCTP, SCTP_EVENTS,
		 &evnts, sizeof(evnts));

  /* bind the sockets to INADDRANY */
  bzero(&local_addr, sizeof(local_addr));
#if !defined (LINUX)
  local_addr.sin_len    = sizeof(local_addr);
#endif
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port   = htons(CHARGEN_PORT);
  if(ext_bind(chargen_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(DAYTIME_PORT);
  if(ext_bind(daytime_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(DISCARD_PORT);
  if(ext_bind(discard_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(ECHO_PORT);
  if(ext_bind(echo_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");

  /* make the sockets 'active' */
  backlog = 1; /* it is ignored */
  if (ext_listen(chargen_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(daytime_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(discard_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(echo_fd, backlog) < 0)
    perror("listen call failed");
  	
  /* set autoclose feature: close idle assocs after 5 seconds */
  idle_time = 5;
  if (ext_setsockopt(chargen_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(daytime_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(discard_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(echo_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");

  nfds = FD_SETSIZE;
  FD_ZERO(&rset);
  /* Handle now incoming requests */
  while (1) {
    FD_SET(chargen_fd, &rset);
    FD_SET(daytime_fd, &rset);
    FD_SET(discard_fd, &rset);
    FD_SET(echo_fd,    &rset);

    ext_select(nfds, &rset, NULL, NULL, NULL);

    if (FD_ISSET(chargen_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));    
      if ((length =ext_recvfrom(chargen_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      length = 1 + (random() % 512);
      memset(buffer, 'A', length);
      buffer[length-1] = '\n';
      if (ext_sendto(chargen_fd, buffer, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
        perror("sendto call failed.\n");
    }
    
    if (FD_ISSET(daytime_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));    
      if ((length =ext_recvfrom(daytime_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      time(&now);
      time_as_string = ctime(&now);
      length = strlen(time_as_string);
      if (ext_sendto(daytime_fd, time_as_string, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
        perror("sendto call failed.\n");
    }
   
    if (FD_ISSET(discard_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));    
      if ((length =ext_recvfrom(discard_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
    }
    
    if (FD_ISSET(echo_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));    
      if ((length =ext_recvfrom(echo_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      if (ext_sendto(echo_fd, buffer, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
        perror("sendto call failed.\n");
    }
  }
    
  return 0;
}
