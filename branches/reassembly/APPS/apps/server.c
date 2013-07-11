#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>

#define USE_BIND

int main (int argc, const char * argv[]) {

  int fd, nfd;
  socklen_t remote_addr_length;
  struct sockaddr_in local_addr, remote_addr;
  struct sockaddr_storage addr_storage[2];

  if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    perror("socket call");

  bzero(&addr_storage, sizeof(addr_storage));

  /* fill in data for fist address: 192.168.1.16:2345 */
  bzero(&local_addr, sizeof(local_addr));
#ifndef linux
  local_addr.sin_len         = sizeof(local_addr);
#endif
  local_addr.sin_family      = AF_INET;
  local_addr.sin_addr.s_addr = inet_addr("192.168.1.16");
  local_addr.sin_port        = htons(2345);
  
  /* copy in storage array */
  bcopy((const void *) &local_addr, (void *) &(addr_storage[0]), sizeof(local_addr));

  /* fill in data for second address: 127.0.0.1:2345 */
  bzero(&local_addr, sizeof(local_addr));
#ifndef linux
  local_addr.sin_len         = sizeof(local_addr);
#endif
  local_addr.sin_family      = AF_INET;
  local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  local_addr.sin_port        = htons(2345);
  
  /* copy in storage array */
  bcopy((const void *) &local_addr, (void *) &(addr_storage[1]), sizeof(local_addr));

  /* fill in data for wildcard address: 0.0.0.0:2345 */
  bzero(&local_addr, sizeof(local_addr));
#ifndef linux
  local_addr.sin_len         = sizeof(local_addr);
#endif
  local_addr.sin_family      = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port        = htons(2345);

#ifdef USE_BIND
  if (bind(fd, (const struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
    perror("bind");
#else
  if (sctp_bindx(fd, (struct sockaddr_storage *)&addr_storage, 2, SCTP_BINDX_ADD_ADDR) < 0)
    perror("sctp_bindx");
#endif

  if (listen(fd, 1) < 0)
    perror("listen");

  bzero(&remote_addr, sizeof(remote_addr));
  remote_addr_length = sizeof(remote_addr);

  if ((nfd = accept(fd, (struct sockaddr *)&remote_addr, &remote_addr_length)) < 0)
    perror("accept");

  sleep(5);

  if (close(nfd) < 0)
    perror("close 1");
    
  if (close(fd) < 0)
    perror("close 2");

  return(0);
}
