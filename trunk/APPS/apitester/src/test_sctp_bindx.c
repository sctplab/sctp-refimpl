#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/sctp.h>
#include "sctp_utilities.h"
#include "api_tests.h"

DEFINE_APITEST(bindx, port_w_a_w_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(0);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, port_s_a_w_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(0);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, port_w_a_s_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, port_s_a_s_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, zero_flag)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, 0);
	close(fd);
		
	if (result)
		return NULL;
	else
		return "sctp_bindx() was successful";
}

DEFINE_APITEST(bindx, add_zero_addresses)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 0, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, rem_zero_addresses)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 0, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, add_zero_addresses_NULL)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	result = sctp_bindx(fd, NULL, 0, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, rem_zero_addresses_NULL)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	result = sctp_bindx(fd, NULL, 0, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, dup_add_s_a_s_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR);
	close(fd);
		
	if (result)
		return strerror(errno);
	else
		return NULL;
}

DEFINE_APITEST(bindx, rem_last_s_a_s_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		return NULL;
	else
		return "Can remove last address";
}

DEFINE_APITEST(bindx, rem_s_a_s_p)
{
	int fd, result;
	struct sockaddr_in address;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
		
	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_ADD_ADDR) < 0) {
		close(fd);
		return strerror(errno);
	}

	bzero((void *)&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	address.sin_len = sizeof(struct sockaddr_in);
#endif
	address.sin_port = htons(1234);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	result = sctp_bindx(fd, (struct sockaddr *)&address, 1, SCTP_BINDX_REM_ADDR);
	close(fd);
		
	if (result)
		return NULL;
	else
		return "Allowed to remove a boundall address";

}

/*
 * packed/unpacked.
 * v6only
 * multiple values.
 */
