#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "sctp_utilities.h"
#include "api_tests.h"

DEFINE_APITEST(rtoinfo, gso_1_1_defaults)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if ((init != 3000) || (min != 1000) || (max != 60000))
		return "Default not RFC 2960 compliant";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, gso_1_M_defaults)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if ((init != 3000) || (min != 1000) || (max != 60000))
		return "Default not RFC 2960 compliant";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, gso_1_1_bad_id)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	result = sctp_get_rto_info(fd, 1, NULL, NULL, NULL);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	return NULL;
}

DEFINE_APITEST(rtoinfo, gso_1_M_bad_id)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);
	
	result = sctp_get_rto_info(fd, 1, NULL, NULL, NULL);
	
	close(fd);
	
	if (!result)
		return "getsockopt was successful";

	if (errno != ENOENT)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_good)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(fd, 0, init, max, min);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if ((init != new_init) || (min != new_min) || (max != new_max))
		return "Values could not be set correctly";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_good)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(fd, 0, init, max, min);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if ((init != new_init) || (min != new_min) || (max != new_max))
		return "Values could not be set correctly";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_bad_id)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_set_rto_info(fd, 1, init, max, min);
	close(fd);

	if (result) {
		return strerror(errno);
	}
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_bad_id)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_set_rto_info(fd, 1, init, max, min);
	close(fd);

	if (!result) {
		return "setsockopt was successful";
	}

	if (errno != ENOENT)
		return strerror(errno);

	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_init)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	init  += 10;
	
	result = sctp_set_initial_rto(fd, 0, init);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (init != new_init)
		return "Value could not be set correctly";
		
	if ((max != new_max) || (min != new_min))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_init)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	init  += 10;
	
	result = sctp_set_initial_rto(fd, 0, init);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (init != new_init)
		return "Value could not be set correctly";
		
	if ((max != new_max) || (min != new_min))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_max)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	max  += 10;
	
	result = sctp_set_maximum_rto(fd, 0, max);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (max != new_max)
		return "Value could not be set correctly";
		
	if ((init != new_init) || (min != new_min))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_max)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	max  += 10;
	
	result = sctp_set_maximum_rto(fd, 0, max);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (max != new_max)
		return "Value could not be set correctly";
		
	if ((init != new_init) || (min != new_min))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_min)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	min  += 10;
	
	result = sctp_set_minimum_rto(fd, 0, min);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (min != new_min)
		return "Value could not be set correctly";
		
	if ((init != new_init) || (max != new_max))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_min)
{
	int fd, result;
	uint32_t init, max, min, new_init, new_max, new_min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	min  += 10;
	
	result = sctp_set_minimum_rto(fd, 0, min);

	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &new_init, &new_max, &new_min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	if (min != new_min)
		return "Value could not be set correctly";
		
	if ((init != new_init) || (max != new_max))
		return "Values have been changed";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_same)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_set_rto_info(fd, 0, 100, 100, 100);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

		
	if ((init != 100) || (max != 100) || (min != 100))
		return "Values could not be set correclty";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_same)
{
	int fd, result;
	uint32_t init, max, min;
	
	if ((fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_set_rto_info(fd, 0, 100, 100, 100);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_rto_info(fd, 0, &init, &max, &min);
	
	close(fd);
	
	if (result)
		return strerror(errno);

		
	if ((init != 100) || (max != 100) || (min != 100))
		return "Values could not be set correclty";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_1)
{
	int fd, result;
	uint32_t min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_minimum_rto(fd, 0, &min);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_initial_rto(fd, 0, min - 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.init smaller than RTO.min";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_2)
{
	int fd, result;
	uint32_t max;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_maximum_rto(fd, 0, &max);
	
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_initial_rto(fd, 0, max + 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.init greater than RTO.max";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_3)
{
	int fd, result;
	uint32_t init;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_initial_rto(fd, 0, &init);
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_minimum_rto(fd, 0, init + 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.min greater than RTO.init";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_4)
{
	int fd, result;
	uint32_t max;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_maximum_rto(fd, 0, &max);
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_minimum_rto(fd, 0, max + 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.min greater than RTO.max";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_5)
{
	int fd, result;
	uint32_t init;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_initial_rto(fd, 0, &init);
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_maximum_rto(fd, 0, init - 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.max smaller than RTO.init";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_ill_6)
{
	int fd, result;
	uint32_t min;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		return strerror(errno);

	result = sctp_get_minimum_rto(fd, 0, &min);
	if (result) {
		close(fd);
		return strerror(errno);
	}
		
	result = sctp_set_maximum_rto(fd, 0, min - 10);

	close(fd);
	
	if (!result)
		return "Can set RTO.max smaller than RTO.min";
	
	if (errno != EDOM)
		return strerror(errno);
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, gso_1_1_c_bad_id)
{
	int fd[2], result;

	if (sctp_socketpair(fd) < 0)
		return strerror(errno);
		
	result = sctp_get_rto_info(fd[0], 1, NULL, NULL, NULL);
	
	close(fd[0]);
	close(fd[1]);
	
	if (result)
		return strerror(errno);

	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_c_bad_id)
{
	int fd[2], result;
	uint32_t init, max, min;
	
	if (sctp_socketpair(fd) < 0)
		return strerror(errno);

	result = sctp_get_rto_info(fd[0], 0, &init, &max, &min);
	
	if (result) {
		close(fd[0]);
		close(fd[1]);
		return strerror(errno);
	}
	
	result = sctp_set_rto_info(fd[0], 1, init, max, min);
	close(fd[0]);
	close(fd[1]);

	if (result) {
		return strerror(errno);
	}
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_1_inherit)
{
	int cfd, afd, lfd, result;
	struct sockaddr_in addr;
	socklen_t addr_len;
	uint32_t init, min, max, new_init, new_max, new_min;

	if ((lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return strerror(errno);

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(lfd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		return strerror(errno);
	}

	if (listen(lfd, 1) < 0) {
		close(lfd);
		return strerror(errno);
	}

	result = sctp_get_rto_info(lfd, 0, &init, &max, &min);
	
	if (result) {
		close(lfd);
		return strerror(errno);
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(lfd, 0, init, max, min);

	if (result) {
		close(lfd);
		return strerror(errno);
	}
	
	if ((cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
		close(lfd);
		return strerror(errno);
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(cfd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (lfd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	if (connect(cfd, (struct sockaddr *) &addr, addr_len) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	if ((afd = accept(lfd, NULL, 0)) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}
	close(lfd);

	result = sctp_get_rto_info(afd, 0, &new_init, &new_max, &new_min);
	
	close(cfd);
	close(afd);

	if (result)
		return strerror(errno);

	if ((init != new_init) || (min != new_min) || (max != new_max))
		return "Values are not inherited correctly";
	
	return NULL;
}

DEFINE_APITEST(rtoinfo, sso_1_M_inherit)
{
	int cfd, pfd, afd, lfd, result;
	struct sockaddr_in addr;
	socklen_t addr_len;
	uint32_t init, min, max, new_init, new_max, new_min;
	sctp_assoc_t assoc_id;
	
	if ((lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return strerror(errno);

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(lfd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		return strerror(errno);
	}

	if (listen(lfd, 1) < 0) {
		close(lfd);
		return strerror(errno);
	}
	
	if ((cfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) {
		close(lfd);
		return strerror(errno);
	}

	result = sctp_get_rto_info(cfd, 0, &init, &max, &min);
	
	if (result) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}
	
	init *= 2;
	min  *= 2;
	max  *= 2;
	
	result = sctp_set_rto_info(cfd, 0, init, max, min);

	if (result) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
	addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(cfd, (struct sockaddr *)&addr, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	addr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (getsockname (lfd, (struct sockaddr *) &addr, &addr_len) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	if (sctp_connectx(cfd, (struct sockaddr *) &addr, 1, &assoc_id) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}

	if ((afd = accept(lfd, NULL, 0)) < 0) {
		close(lfd);
		close(cfd);
		return strerror(errno);
	}
	close(lfd);

	if ((pfd = sctp_peeloff(cfd, assoc_id)) < 0) {
		close(afd);
		close(cfd);
		return strerror(errno);
	}
	close(cfd);

	result = sctp_get_rto_info(pfd, 0, &new_init, &new_max, &new_min);
	
	close(afd);
	close(pfd);

	if (result)
		return strerror(errno);

	if ((init != new_init) || (min != new_min) || (max != new_max))
		return "Values are not inherited correctly";

	return NULL;
}

DEFINE_APITEST(assoclist, gso_numbers_zero)
{
	int fd, result;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return strerror(errno);
		
	result = sctp_get_number_of_associations(fd);
	
	close(fd);

	if (result == 0)
		return NULL;
	else
		return "Wrong number of associations";
}

#define NUMBER_OF_ASSOCS 12

DEFINE_APITEST(assoclist, gso_numbers_pos)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	unsigned int i;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		return strerror(errno);
		
	result = sctp_get_number_of_associations(fd);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS)
		return NULL;
	else
		return "Wrong number of associations";
}

DEFINE_APITEST(assoclist, gso_ids_no_assoc)
{
	int fd, result;
	sctp_assoc_t id;
	
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
    	return strerror(errno);
		
	if (sctp_get_number_of_associations(fd) != 0) {
		close(fd);
		return strerror(errno);
	}
	
	result = sctp_get_association_identifiers(fd, &id, 1);
	close(fd);
	if (result == 0)
		return NULL;
	else
		return "Wrong number of identifiers";
}

DEFINE_APITEST(assoclist, gso_ids_buf_fit)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i, j;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		return strerror(errno);
		
	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);
		return strerror(errno);
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS) {
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			for (j = 0; j < NUMBER_OF_ASSOCS; j++)
				if ((i != j) && (ids[i] == ids[j]))
					return "Same identifier for different associations";
		return NULL;
	} else
		return "Wrong number of identifiers";
}

DEFINE_APITEST(assoclist, gso_ids_buf_large)
{
	int fd, fds[NUMBER_OF_ASSOCS + 1], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i, j;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		return strerror(errno);
		
	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);	
		return strerror(errno);
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS + 1);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result == NUMBER_OF_ASSOCS) {
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			for (j = 0; j < NUMBER_OF_ASSOCS; j++)
				if ((i != j) && (ids[i] == ids[j]))
					return "Same identifier for different associations";
		return NULL;
	} else
		return "Wrong number of identifiers";
}

DEFINE_APITEST(assoclist, gso_ids_buf_small)
{
	int fd, fds[NUMBER_OF_ASSOCS], result;
	sctp_assoc_t ids[NUMBER_OF_ASSOCS];
	unsigned int i;
	
	if (sctp_socketstar(&fd, fds, NUMBER_OF_ASSOCS) < 0)
		return strerror(errno);
		
	if (sctp_get_number_of_associations(fd) != NUMBER_OF_ASSOCS) {
		close(fd);
		for (i = 0; i < NUMBER_OF_ASSOCS; i++)
			close(fds[i]);
		return strerror(errno);
	}
	
	result = sctp_get_association_identifiers(fd, ids, NUMBER_OF_ASSOCS - 1);
	
	close(fd);
	for (i = 0; i < NUMBER_OF_ASSOCS; i++)
		close(fds[i]);
	
	if (result > 0)
		return "getsockopt successful";
	else
		return NULL;
}


/********************************************************
 *
 * SCTP_ASSOCINFO tests
 *
 ********************************************************/
static char error_buffer[128];

DEFINE_APITEST(associnfo, gso_1_1_defaults)
{
	int fd, result;
	uint16_t asoc_maxrxt, peer_dest_cnt;
	uint32_t peer_rwnd, local_rwnd, cookie_life;
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt,
				     &peer_dest_cnt, 
				     &peer_rwnd,
				     &local_rwnd,
				     &cookie_life);
	close(fd);
	if (result)
		return strerror(errno);

	if (asoc_maxrxt != 10) {
		sprintf(error_buffer, "max_rxt:%d Not compliant with RFC4960", asoc_maxrxt);
		return(error_buffer);
	}
	if (local_rwnd < 4096) {
		sprintf(error_buffer, "local_rwnd:%d Not compliant with RFC4960", local_rwnd);
		return(error_buffer);
	}
	if (cookie_life != 60000) {
		sprintf(error_buffer, "cookie_life:%d Not compliant with RFC4960", cookie_life);
		return(error_buffer);
	}
	if(peer_rwnd != 0) {
		return "Peer rwnd with no peer?";
	}

	if(peer_dest_cnt != 0) {
		return "Peer destination count with no peer?";
	}
	return NULL;
}

DEFINE_APITEST(associnfo, gso_1_M_defaults)
{
	int fd, result;
	uint16_t asoc_maxrxt, peer_dest_cnt;
	uint32_t peer_rwnd, local_rwnd, cookie_life;
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt,
				     &peer_dest_cnt, 
				     &peer_rwnd,
				     &local_rwnd,
				     &cookie_life);
	close(fd);
	if (result)
		return strerror(errno);

	if (asoc_maxrxt != 10) {
		sprintf(error_buffer, "max_rxt:%d Not compliant with RFC4960", asoc_maxrxt);
		return(error_buffer);
	}
	if (local_rwnd < 4096) {
		sprintf(error_buffer, "local_rwnd:%d Not compliant with RFC4960", local_rwnd);
		return(error_buffer);
	}
	if (cookie_life != 60000) {
		sprintf(error_buffer, "cookie_life:%d Not compliant with RFC4960", cookie_life);
		return(error_buffer);
	}
	if(peer_rwnd != 0) {
		return "Peer rwnd with no peer?";
	}

	if(peer_dest_cnt != 0) {
		return "Peer destination count with no peer?";
	}
	return NULL;
}


DEFINE_APITEST(associnfo, sso_rxt_1_1)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2one(0,1);

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = asoc_maxrxt[0] * 2;
	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}

	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value";
		goto out;
	}
 out:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_rxt_1_M)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2many(0);

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = asoc_maxrxt[0] * 2;

	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}

	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value";
		goto out;
	}
 out:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_rxt_asoc_1_1)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0,1);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_reuse(fd, fds) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[1], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_rxt_asoc_1_M)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0);
	if(fds[0] < 0) {
		return (strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_1tom(fds, ids) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;
	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[0], ids[0], newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);
	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);

}

DEFINE_APITEST(associnfo, sso_rxt_asoc_1_1_inherit)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0,1);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out_nopair;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_reuse(fd, fds) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[2] != newval) {
		retstring = "maxrxt did not inherit";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_rxt_asoc_1_M_inherit)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = asoc_maxrxt[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_maxrxt(fds[0], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie-life changed on set of maxrxt";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] == asoc_maxrxt[1]) {
		retstring = "maxrxt did not change";
		goto out_nopair;
	}
	if(asoc_maxrxt[1] != newval) {
		retstring = "maxrxt did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_1tom(fds, ids) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of maxrxt";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[2] != newval) {
		retstring = "maxrxt did not inherit";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);


}


/* ************************************* */
DEFINE_APITEST(associnfo, sso_clife_1_1)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2one(0,1);

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = cookie_life[0] * 2;
	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of cookie-life";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "max rxt changed on set of cookie-life";
		goto out;
	}

	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value";
		goto out;
	}
 out:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_clife_1_M)
{
	int fd, result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int newval;
	char *retstring=NULL;

	fd = sctp_one2many(0);

	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}

	newval = cookie_life[0] * 2;

	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;

	}
	/* Get all the values for assoc info on ep again */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set of cookie-life";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed with cookie_life";
		goto out;
	}

	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value";
		goto out;
	}
 out:
	close(fd);
	return (retstring);


}


DEFINE_APITEST(associnfo, sso_clife_asoc_1_1)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0,1);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_reuse(fd, fds) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[1], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of cookie-life";
		goto out;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie-life did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life ep changed on set of asoc clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwnd ep changed on set of asoc clife";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep during set of asoc clife";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_clife_asoc_1_M)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0);
	if(fds[0] < 0) {
		return (strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	if (sctp_socketpair_1tom(fds, ids) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;
	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[0], ids[0], newval);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change on set asoc clife set";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of asoc clife";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of asoc clife";
		goto out;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value on assoc";
		goto out;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);
	if(cookie_life[0] != cookie_life[2]) {
		retstring = "cookie-life on ep changed on set of asoc clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwdn ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt changed on ep during set of asoc clife";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);

}

DEFINE_APITEST(associnfo, sso_clife_asoc_1_1_inherit)
{
	int fd, result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	char *retstring=NULL;

	fd = sctp_one2one(0,1);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fd, 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie-life did not change";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of clife";
		goto out_nopair;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value on assoc";
		goto out_nopair;
	}


	if (sctp_socketpair_reuse(fd, fds) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[1], 0, 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt ep changed on set of clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local_rwnd ep changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[2] != newval) {
		retstring = "clife did not inherit";
		goto out;
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_clife_asoc_1_M_inherit)
{
	int result;
	uint16_t asoc_maxrxt[3], peer_dest_cnt[3];
	uint32_t peer_rwnd[3], local_rwnd[3], cookie_life[3];
	int newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	newval = cookie_life[0] * 2;

	/* Set the assoc value */
	result = sctp_set_asoc_cookie_life(fds[0], 0, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);

	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of clife";
		goto out_nopair;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local_rwnd changed on set of maxrxt";
		goto out_nopair;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] == cookie_life[1]) {
		retstring = "cookie_lifet did not change";
		goto out_nopair;
	}
	if(cookie_life[1] != newval) {
		retstring = "cookie_life did not change to correct value on ep";
		goto out_nopair;
	}


	if (sctp_socketpair_1tom(fds, ids) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	result = sctp_get_assoc_info(fds[0], ids[0], 
				     &asoc_maxrxt[2],
				     &peer_dest_cnt[2], 
				     &peer_rwnd[2],
				     &local_rwnd[2],
				     &cookie_life[2]);

	if(asoc_maxrxt[0] != asoc_maxrxt[2]) {
		retstring = "maxrxt asoc changed on set of clife";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[2]) {
		retstring = "local-rwnd asoc changed on set of maxrxt";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[2] != newval) {
		retstring = "cookie_life did not inherit";
		goto out;
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);


}

DEFINE_APITEST(associnfo, sso_lrwnd_ep_1_1)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fd;
	char *retstring=NULL;
	fd = sctp_one2one(0,1);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fd, 0,  (2*local_rwnd[0]));
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fd);
	return(retstring);
}

DEFINE_APITEST(associnfo, sso_lrwnd_ep_1_M)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fd;
	char *retstring=NULL;
	fd = sctp_one2many(0);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fd, 0,  (2*local_rwnd[0]));

	result = sctp_get_assoc_info(fd, 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fd);
	return(retstring);
}

DEFINE_APITEST(associnfo, sso_lrwnd_asoc_1_1)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fds[0], 0,  (2*local_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}


DEFINE_APITEST(associnfo, sso_lrwnd_asoc_1_M)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_local_rwnd(fds[0], 0,  (2*local_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}




DEFINE_APITEST(associnfo, sso_prwnd_asoc_1_1)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peer_rwnd(fds[0], 0,  (2*peer_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_rwnd[0] != peer_rwnd[1]) {
		retstring = "peers rwnd changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}


DEFINE_APITEST(associnfo, sso_prwnd_asoc_1_M)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peer_rwnd(fds[0], 0,  (2*peer_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if (peer_rwnd[0] != peer_rwnd[1]) {
		retstring = "peers rwnd changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}

DEFINE_APITEST(associnfo, sso_pdest_asoc_1_1)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	char *retstring=NULL;
	result = sctp_socketpair(fds);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peerdest_cnt(fds[0], 0,  (2*peer_rwnd[0]));
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_dest_cnt[0] != peer_dest_cnt[1]) {
		retstring = "peers dest count changed";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}


DEFINE_APITEST(associnfo, sso_pdest_asoc_1_M)
{
	int result;
	uint16_t asoc_maxrxt[2], peer_dest_cnt[2];
	uint32_t peer_rwnd[2], local_rwnd[2], cookie_life[2];
	int fds[2];
	sctp_assoc_t ids[2];	
	char *retstring=NULL;
	fds[0] = fds[1] = -1;

	result = sctp_socketpair_1tom(fds, ids);
	if(result < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[0],
				     &peer_dest_cnt[0], 
				     &peer_rwnd[0],
				     &local_rwnd[0],
				     &cookie_life[0]);
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	/* We don't care about the return.. error or
	 * whatever. We care about if it changed.
	 */
	(void)sctp_set_asoc_peerdest_cnt(fds[0], 0,  (2*peer_rwnd[0]));

	result = sctp_get_assoc_info(fds[0], 0, 
				     &asoc_maxrxt[1],
				     &peer_dest_cnt[1], 
				     &peer_rwnd[1],
				     &local_rwnd[1],
				     &cookie_life[1]);
	
	if (result) {
		retstring = strerror(errno);
		goto out;
	}
	if(asoc_maxrxt[0] != asoc_maxrxt[1]) {
		retstring = "maxrxt changed on set of lrwnd";
		goto out;
	}

	if(local_rwnd[0] != local_rwnd[1]) {
		retstring = "local-rwnd changed on set!";
		goto out;
	}
	/* don't check peer_rwnd or peer_dest_cnt  we have no peer */
	if(cookie_life[0] != cookie_life[1]) {
		retstring = "cookie_life changed on set of lrwnd";
		goto out;
	}
	if(peer_dest_cnt[0] != peer_dest_cnt[1]) {
		retstring = "peers dest count changed";
	}

 out:
	close (fds[0]);
	close (fds[1]);
	return(retstring);
}


/********************************************************
 *
 * SCTP_initmsg tests
 *
 ********************************************************/

DEFINE_APITEST(initmsg, gso_1_1_defaults)
{
	int fd, result;
	uint32_t ostreams, istreams;
	uint16_t max, timeo;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams, &istreams,
				  &max, &timeo);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (max != 8) {
		return "Default not RFC 2960 compliant (max_attempts)";
	}
	if (timeo != 60000) {
		return "Default not RFC 2960 compliant (max_init_timeo)";
	}
	return(NULL);
}


DEFINE_APITEST(initmsg, gso_1_M_defaults)
{
	int fd,result;
	uint32_t ostreams, istreams;
	uint16_t max, timeo;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams, &istreams,
				  &max, &timeo);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (max != 8) {
		return "Default not RFC 2960 compliant (max_attempts)";
	}
	if (timeo != 60000) {
		return "Default not RFC 2960 compliant (max_init_timeo)";
	}
	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_1_set_ostrm)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * ostreams[0];
	result = sctp_set_im_ostream(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[1] != newval) {
		return "failed to set new ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "ostream set changed istream value";
	}
	if (max[0] != max[1]) {
		return "ostream set changed max attempts value";
	}
	if (timeo[0] != timeo[1]) {
		return "ostream set changed max init timeout value";
	}

	return(NULL);
}
DEFINE_APITEST(initmsg, gso_1_1_set_istrm)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * istreams[0];
	result = sctp_set_im_istream(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "istream set changed ostream value";
	}
	if (newval != istreams[1]) {
		return "failed to set new istream value";
	}
	if (max[0] != max[1]) {
		return "istream set changed max attempts value";
	}
	if (timeo[0] != timeo[1]) {
		return "istream set changed max init timeout value";
	}
	return(NULL);
}


DEFINE_APITEST(initmsg, gso_1_1_set_max)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxattempt(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "max attempt set changed ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "max attempt set changed max attempts value";
	}
	if (newval != max[1]) {
		return "failed to set new max attempt value";
	}
	if (timeo[0] != timeo[1]) {
		return "max attempt set changed max init timeout value";
	}
	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_1_set_timeo)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxtimeo(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "max timeo set changed ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "max timeo set changed max attempts value";
	}
	if (max[0] != max[1]) {
		return "max timo set changed max attempts value";
	}
	if (newval != timeo[1]) {
		return "failed to set new max timeo value";
	}
	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_M_set_ostrm)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * ostreams[0];
	result = sctp_set_im_ostream(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[1] != newval) {
		return "failed to set new ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "ostream set changed istream value";
	}
	if (max[0] != max[1]) {
		return "ostream set changed max attempts value";
	}
	if (timeo[0] != timeo[1]) {
		return "ostream set changed max init timeout value";
	}

	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_M_set_istrm)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint32_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * istreams[0];
	result = sctp_set_im_istream(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "istream set changed ostream value";
	}
	if (newval != istreams[1]) {
		return "failed to set new istream value";
	}
	if (max[0] != max[1]) {
		return "istream set changed max attempts value";
	}
	if (timeo[0] != timeo[1]) {
		return "istream set changed max init timeout value";
	}
	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_M_set_max)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxattempt(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "max attempt set changed ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "max attempt set changed max attempts value";
	}
	if (newval != max[1]) {
		return "failed to set new max attempt value";
	}
	if (timeo[0] != timeo[1]) {
		return "max attempt set changed max init timeout value";
	}
	return(NULL);
}

DEFINE_APITEST(initmsg, gso_1_M_set_timeo)
{
	int fd, result;
	uint32_t ostreams[2], istreams[2];
	uint16_t max[2], timeo[2];
	uint16_t newval;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	
	result = sctp_get_initmsg(fd, &ostreams[0], &istreams[0],
				  &max[0], &timeo[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	newval = 2 * max[0];
	result = sctp_set_im_maxtimeo(fd, newval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));		
	}
	result = sctp_get_initmsg(fd, &ostreams[1], &istreams[1],
				  &max[1], &timeo[1]);
	close(fd);
	if(result < 0) {
		return(strerror(errno));		
	}
	if (ostreams[0] != ostreams[1]) {
		return "max timeo set changed ostream value";
	}
	if (istreams[0] != istreams[1]) {
		return "max timeo set changed max attempts value";
	}
	if (max[0] != max[1]) {
		return "max timo set changed max attempts value";
	}
	if (newval != timeo[1]) {
		return "failed to set new max timeo value";
	}
	return(NULL);
}

/********************************************************
 *
 * SCTP_NODELAY tests
 *
 ********************************************************/

DEFINE_APITEST(initmsg, gso_1_1_def_ndelay)
{
	uint32_t val;
	int fd, result;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val);
	close(fd);
	if(result) {
		return(strerror(errno));
	}
	if(val == 1) {
		return "non-compliant NO-Delay should default to off";
	}
	return NULL;
}
DEFINE_APITEST(initmsg, gso_1_M_def_ndelay)
{
	uint32_t val;
	int fd, result;
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val);
	close(fd);
	if(result) {
		return(strerror(errno));
	}
	if(val == 1) {
		return "non-compliant NO-Delay should default to off";
	}
	return NULL;
}
DEFINE_APITEST(initmsg, gso_1_1_set_ndelay)
{
	uint32_t val[3];
	int fd, result;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[0]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	val[1] = !val[0];
	result = sctp_set_ndelay(fd, val[1]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[2]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if(val[1] != val[2]) {
		return "could not toggle the value";
	}
	return NULL;
}

DEFINE_APITEST(initmsg, gso_1_M_set_ndelay)
{
	uint32_t val[3];
	int fd, result;
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[0]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	val[1] = !val[0];
	result = sctp_set_ndelay(fd, val[1]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_ndelay(fd, &val[2]);
	if(result) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if(val[1] != val[2]) {
		return "could not toggle the value";
	}
	return NULL;
}
