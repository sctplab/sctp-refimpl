#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/sctp.h>

int sctp_socketpair(int *fds)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return -1;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}

	if ((fds[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(fd);
		return -1;
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fds[0], (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (fd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(fd);
		return -1;
	}

	if (connect(fds[0], (struct sockaddr *) &addr, addr_len) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	if ((fds[1] = accept(fd, NULL, 0)) < 0) {
		close(fd);
		close(fds[0]);
		return -1;
	}

	close(fd);
	return 0;
}

int sctp_shutdown(int fd) {
	return shutdown(fd, SHUT_WR);
}

int sctp_abort(int fd) {
    struct linger l;
    
    l.l_onoff  = 1;
    l.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof (struct linger)) < 0)
    	return -1;
    return close(fd);
 }

int sctp_enable_non_blocking(int fd)
{
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags  | O_NONBLOCK);
}

int sctp_disable_non_blocking_blocking(int fd)
{
	int flags;
	
	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags  & ~O_NONBLOCK);
}
