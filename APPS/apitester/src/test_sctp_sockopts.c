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

	if (sctp_socketpair(fd, 0) < 0)
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
	
	if (sctp_socketpair(fd, 0) < 0)
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

	fd = sctp_one2one(0,1, 0);

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

	fd = sctp_one2many(0, 0);

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

	fd = sctp_one2one(0,1, 0);
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
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
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

	fds[0] = sctp_one2many(0,0);
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
	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
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

	fd = sctp_one2one(0,1, 0);
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


	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
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

	fds[0] = sctp_one2many(0, 0);
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


	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
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

	fd = sctp_one2one(0,1, 0);

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

	fd = sctp_one2many(0, 0);

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

	fd = sctp_one2one(0,1, 0);
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
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
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

	fds[0] = sctp_one2many(0, 0);
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
	if (sctp_socketpair_1tom(fds, ids,0) < 0) {
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

	fd = sctp_one2one(0,1, 0);
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


	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
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

	fds[0] = sctp_one2many(0,0);
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


	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
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
	fd = sctp_one2one(0,1, 0);
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
	fd = sctp_one2many(0, 0);
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
	result = sctp_socketpair(fds, 0);
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

	result = sctp_socketpair_1tom(fds, ids, 0);
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
	result = sctp_socketpair(fds, 0);
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

	result = sctp_socketpair_1tom(fds, ids, 0);
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
	result = sctp_socketpair(fds, 0);
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

	result = sctp_socketpair_1tom(fds, ids, 0);
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

DEFINE_APITEST(nodelay, gso_1_1_def_ndelay)
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
DEFINE_APITEST(nodelay, gso_1_M_def_ndelay)
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
DEFINE_APITEST(nodelay, gso_1_1_set_ndelay)
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

DEFINE_APITEST(nodelay, gso_1_M_set_ndelay)
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

/********************************************************
 *
 * SCTP_autoclose tests
 *
 ********************************************************/

DEFINE_APITEST(autoclose, gso_1_1_def_autoclose)
{
	uint32_t aclose;
	int result, fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose);
	close(fd);
	if (result < 0) {
		return NULL;
	}
	if(aclose) {
		return "autoclose enabled on 1-2-1?";
	}
	return NULL;
}

DEFINE_APITEST(autoclose, gso_1_M_def_autoclose)
{
	uint32_t aclose;
	int result, fd;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose);
	close(fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if(aclose) {
		return "autoclose is enabled by default";
	}
	return NULL;
}

DEFINE_APITEST(autoclose, gso_1_1_set_autoclose)
{
	uint32_t aclose[3];
	int result, fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[0]);
	if (result < 0) {
		close(fd);
		return NULL;
	}
	aclose[1] = 40;
	result = sctp_set_autoclose(fd, aclose[1]);
	if (result < 0) {
		close(fd);
		return NULL;
	}
	result = sctp_get_autoclose(fd, &aclose[2]);
	close(fd);
	if (result < 0) {
		return NULL;
	}
	if(aclose[1] == aclose[2]) {
		return "1-2-1 allowed set of auto close";
	}
	return NULL;
}


DEFINE_APITEST(autoclose, gso_1_M_set_autoclose)
{
	uint32_t aclose[3];
	int result, fd;

	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if(fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[0]);
	if (result < 0) {
		close(fd);
		return(strerror(errno));
	}
	aclose[1] = 40;
	result = sctp_set_autoclose(fd, aclose[1]);
	if (result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_autoclose(fd, &aclose[2]);
	close(fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if(aclose[1] != aclose[2]) {
		return "Can't set auto close to new value";
	}
	return NULL;
}


/********************************************************
 *
 * SCTP_SET_PEER_PRIMARY tests
 *
 ********************************************************/

DEFINE_APITEST(setpeerprim, sso_1_1_good_peerprim)
{
	int fds[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa=NULL;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	num = sctp_getladdrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	result = sctp_set_peer_prim(fds[0], 0,  sa);
	if (result < 0) {
		retstring = strerror(errno);
	}
	sctp_freeladdrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setpeerprim, sso_1_1_bad_peerprim)
{
	int fds[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sin;
	struct sockaddr *sa=NULL;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	num = sctp_getladdrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	sin = *((struct sockaddr_in *)sa);
	sctp_freeladdrs(sa);
	sin.sin_addr.s_addr = 0x12345678;
	result = sctp_set_peer_prim(fds[0], 0,  (struct sockaddr *)&sin);
	if (result >= 0) {
		retstring = "set peer primary for bad address allowed";
	}

 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}


DEFINE_APITEST(setpeerprim, sso_1_M_good_peerprim)
{
	int fds[2];
	sctp_assoc_t ids[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa=NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	result =  sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
 try_again:
	num = sctp_getladdrs(fds[0], ids[0], &sa);
	if(num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	result = sctp_set_peer_prim(fds[0], ids[0],  sa);
	if (result < 0) {
		retstring = strerror(errno);
	}
	sctp_freeladdrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);

}


DEFINE_APITEST(setpeerprim, sso_1_M_bad_peerprim)
{
	int fds[2];
	sctp_assoc_t ids[2];
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sin;
	struct sockaddr *sa=NULL;
	int cnt = 0;

	fds[0] = fds[1] = -1;
	result =  sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
 try_again:
	num = sctp_getladdrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getladdr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freeladdrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	sin = *((struct sockaddr_in *)sa);
	sctp_freeladdrs(sa);
	sin.sin_addr.s_addr = 0x12345678;
	result = sctp_set_peer_prim(fds[0], 0,  (struct sockaddr *)&sin);
	if (result >= 0) {
		retstring = "set peer primary for bad address allowed";
	}
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

/********************************************************
 *
 * SCTP_PRIMARY tests
 *
 ********************************************************/

DEFINE_APITEST(setprim, gso_1_1_get_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at;
	union sctp_sockstore store;
	socklen_t len;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:			
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	sctp_freepaddrs(sa);
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setprim, gso_1_M_get_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at;
	union sctp_sockstore store;
	socklen_t len;
	sctp_assoc_t ids[2];
	int cnt=0;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:			
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	sctp_freepaddrs(sa);
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setprim, sso_1_1_set_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	if(cnt == 0) {
		/* we want the second one */
		if (sa->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else
			len = sizeof(struct sockaddr_in6);
		setit = (struct sockaddr *)((caddr_t)sa  + len);
	} else {
		/* we want the first one */
		setit = sa;
	}
	/* now do the set */
	result = sctp_set_primary(fds[0], 0, setit);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (setit->sa_family != store.sa.sa_family) {
		found = 1;
	} else {
		if(setit->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)setit;
			if(sin->sin_addr.s_addr !=
			   store.sin.sin_addr.s_addr) {
				found = 1;
			}
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)setit;
			if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
			}
		}
	}
	if (found) {
		retstring = "set to new primary failed";
	}
	
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setprim, sso_1_M_set_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt=0;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	if(cnt == 0) {
		/* we want the second one */
		if (sa->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else
			len = sizeof(struct sockaddr_in6);
		setit = (struct sockaddr *)((caddr_t)sa  + len);
	} else {
		/* we want the first one */
		setit = sa;
	}
	/* now do the set */
	result = sctp_set_primary(fds[0], ids[0], setit);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (setit->sa_family != store.sa.sa_family) {
		found = 1;
	} else {
		if(setit->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)setit;
			if(sin->sin_addr.s_addr !=
			   store.sin.sin_addr.s_addr) {
				found = 1;
			}
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)setit;
			if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
			}
		}
	}
	if (found) {
		retstring = "set to new primary failed";
	}
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setprim, sso_1_1_bad_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	struct sockaddr_in sinc;
	socklen_t len;
	int cnt;

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;
	} 
	/* ok we now know that cnt is the current primary */
	sinc = *((struct sockaddr_in *)sa);
	sinc.sin_addr.s_addr = 0x12345678;
	setit = (struct sockaddr *)&sinc;

	result = sctp_set_primary(fds[0], 0, setit);
	if (result < 0) {
		/* good we rejected it */
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], 0,  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if(setit->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)setit;
		if(sin->sin_addr.s_addr !=
		   store.sin.sin_addr.s_addr) {
			found = 1;
		}
	} else {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)setit;
		if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
				       &store.sin6.sin6_addr)) {
			found = 1;
		}
	}
	if (found == 0) {
		retstring = "set to corrupt primary allowed";
	}
	
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}

DEFINE_APITEST(setprim, sso_1_M_bad_prim)
{
	int fds[2], i, found;
	int result, num;
	char *retstring = NULL;
	struct sockaddr_in sinc;
	struct sockaddr *sa, *at, *setit;
	union sctp_sockstore store;
	socklen_t len;
	int cnt=0;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}
	len = sizeof(store);
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
	}
	/* validate its in the list of addresses */
	at = sa;
	cnt = found = 0;
	for (i=0; i<num; i++) {
		if(store.sa.sa_family != at->sa_family) {
			goto skip_forward;
		}
		if (at->sa_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)at;
			if(sin->sin_addr.s_addr ==
			   store.sin.sin_addr.s_addr) {
				found = 1;
				break;
			}
		} else if (at->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)at;
			if (IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
					      &store.sin6.sin6_addr)) {
				found = 1;
				break;
			}
		} else {
			break;
		}
	skip_forward:
		cnt++;
		if (at->sa_family == AF_INET)
			len = sizeof(struct sockaddr_in);
		else if (at->sa_family == AF_INET6)
			len = sizeof(struct sockaddr_in6);
		else
			break;
		at = (struct sockaddr *)((caddr_t)at  + len);
	}
	if(found == 0) {
		retstring = "Bad primary - not in peer addr list";
		sctp_freepaddrs(sa);
		goto out;

	}
	/* ok we now know that cnt is the current primary */
	sinc = *((struct sockaddr_in *)sa);
	sinc.sin_addr.s_addr = 0x12345678;
	setit = (struct sockaddr *)&sinc;

	/* now do the set */
	result = sctp_set_primary(fds[0], ids[0], setit);
	if (result < 0) {
		sctp_freepaddrs(sa);
		goto out;
	}
	/* now did we actually set it? */
	result = sctp_get_primary(fds[0], ids[0],  &store.sa, &len);
	if (result < 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	found = 0;
	if (found) {
		retstring = "set to new primary failed";
	}

	found = 0;
	if(setit->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *)setit;
		if(sin->sin_addr.s_addr !=
		   store.sin.sin_addr.s_addr) {
			found = 1;
		}
	} else {
		struct sockaddr_in6 *sin6;
		sin6 = (struct sockaddr_in6 *)setit;
		if (!IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, 
				       &store.sin6.sin6_addr)) {
			found = 1;
		}
	}
	if (found == 0) {
		retstring = "set to corrupt primary allowed";
	}
	sctp_freepaddrs(sa);
 out:
	close(fds[0]);
	close(fds[1]);
	return (retstring);
}


/********************************************************
 *
 * SCTP_ADAPTATION tests
 *
 ********************************************************/

DEFINE_APITEST(adaptation, gso_1_1)
{
	int fd, result;
	uint32_t val;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	close(fd);
	if(result < 0) {
		return(strerror(errno));
	}
	return NULL;
}

DEFINE_APITEST(adaptation, gso_1_M)
{
	int fd, result;
	uint32_t val;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	close(fd);
	if(result < 0) {
		return(strerror(errno));
	}
	return NULL;
}

DEFINE_APITEST(adaptation, sso_1_1)
{
	int fd, result;
	uint32_t val, val1;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	val1 =  val + 1;
	result = sctp_set_adaptation(fd, val1);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if(val1 != val) {
		return "Set failed, not new value that was set";
	}
	return NULL;

}

DEFINE_APITEST(adaptation, sso_1_M)
{
	int fd, result;
	uint32_t val, val1;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	val1 =  val + 1;
	result = sctp_set_adaptation(fd, val1);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_adaptation(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if(val1 != val) {
		printf("%x was what I set, val:%x\n", 
		       val1, val);
		return "Set failed, not new value that was set";
	}
	return NULL;
}


/********************************************************
 *
 * SCTP_DISABLE_FRAGMENTS tests
 *
 ********************************************************/

DEFINE_APITEST(disfrag, gso_def_1_1)
{
	int fd, result;
	int val=0;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(val) {
		return "Incorrect default, SCTP fragmentation is disabled";
	}
	return NULL;
}

DEFINE_APITEST(disfrag, gso_def_1_M)
{
	int fd, result;
	int val=0;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(val) {
		return "Incorrect default, SCTP fragmentation is disabled";
	}
	return NULL;
}


DEFINE_APITEST(disfrag, sso_1_1)
{
	int fd, result;
	int val=0,nval;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	nval = !val;
	result = sctp_set_disfrag(fd, nval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if (val != nval) {
		return "disable fragments not changed";
	}
	return NULL;
}

DEFINE_APITEST(disfrag, sso_1_M)
{
	int fd, result;
	int val=0,nval;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	nval = !val;
	result = sctp_set_disfrag(fd, nval);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_disfrag(fd, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close (fd);
	if (val != nval) {
		return "disable fragments not changed";
	}
	return NULL;
}

/********************************************************
 *
 * SCTP_PEER_ADDR_PARAMS tests
 *
 ********************************************************/

DEFINE_APITEST(paddrpara, gso_def_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval;
	uint16_t maxrxt;
	uint32_t pathmtu;
	uint32_t flags;
	uint32_t ipv6_flowlabel;
	uint8_t ipv4_tos;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval,
				      &maxrxt,
				      &pathmtu,
				      &flags,
				      &ipv6_flowlabel,
				      &ipv4_tos);
	close(fd);
	if (result< 0) {
		return(strerror(errno));
	}
	if (hbinterval != 30000) {
		return "HB Interval not compliant to RFC2960";
	}
	if (maxrxt != 5) {
		return "Path Max RXT not compliant to RFC2960";
	}
	if (ipv6_flowlabel) {
		return "IPv6 Flow label something other than 0 by default";
	}
	if (ipv4_tos) {
		return "IPv4 TOS something other than 0 by default";
	}
	if (flags & SPP_PMTUD_DISABLE) {
		return "Path MTU not enabled by default";
	}
	if (flags & SPP_HB_DISABLE) {
		return "HB not enabled by default";
	}
	return NULL;
}


DEFINE_APITEST(paddrpara, gso_def_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval;
	uint16_t maxrxt;
	uint32_t pathmtu;
	uint32_t flags;
	uint32_t ipv6_flowlabel;
	uint8_t ipv4_tos;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval,
				      &maxrxt,
				      &pathmtu,
				      &flags,
				      &ipv6_flowlabel,
				      &ipv4_tos);
	close(fd);
	if (result< 0) {
		return(strerror(errno));
	}
	if (hbinterval != 30000) {
		return "HB Interval not compliant to RFC2960";
	}
	if (maxrxt != 5) {
		return "Path Max RXT not compliant to RFC2960";
	}
	if (ipv6_flowlabel) {
		return "IPv6 Flow label something other than 0 by default";
	}
	if (ipv4_tos) {
		return "IPv4 TOS something other than 0 by default";
	}
	if (flags & SPP_PMTUD_DISABLE) {
		return "Path MTU not enabled by default";
	}
	if (flags & SPP_HB_DISABLE) {
		return "HB not enabled by default";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_hb_int_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 1;

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_hb_int_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 1;

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_hb_zero_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;

}
DEFINE_APITEST(paddrpara, sso_hb_zero_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;

}


DEFINE_APITEST(paddrpara, sso_hb_off_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	return retstring;

}
DEFINE_APITEST(paddrpara, sso_hb_off_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	return retstring;

}

DEFINE_APITEST(paddrpara, sso_hb_on_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fd);
		return(retstring);
	}
	result = sctp_set_hbenable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }
        close(fd);
	return retstring;

}

DEFINE_APITEST(paddrpara, sso_hb_on_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[3];

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto outquick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto outquick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto outquick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto outquick;
	}
        if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB could not disable";
	outquick:
                close(fd);
		return retstring;
	}


	result = sctp_set_hbenable(fd, 0, NULL);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
	}
	close(fd);
	return retstring;

}

DEFINE_APITEST(paddrpara, sso_pmrxt_int_1_1)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_pmrxt_int_1_M)
{
	int fd;
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fd, 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_bad_hb_en_1_1)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	flags = SPP_HB_ENABLE | SPP_HB_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		return "Able to enable and disable HB";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_bad_hb_en_1_M)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	flags = SPP_HB_ENABLE | SPP_HB_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		return "Able to enable and disable HB";
	}
	return NULL;
}



DEFINE_APITEST(paddrpara, sso_bad_pmtud_en_1_1)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	flags = SPP_PMTUD_ENABLE | SPP_PMTUD_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		return "Able to enable and disable HB";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_bad_pmtud_en_1_M)
{
	int fd, result;
	uint32_t flags;
	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	flags = SPP_PMTUD_ENABLE | SPP_PMTUD_DISABLE;
	result  = sctp_set_paddr_param(fd, 0, NULL,
				       0,
				       0,
				       0,
				       flags,
				       0,
				       0);
	close(fd);
	if (result != -1) {
		return "Able to enable and disable HB";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_ahb_int_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 1;

	result = sctp_set_hbint(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);

	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_ahb_int_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3], newval;
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = (hbinterval[0] * 2) + 1;

	result = sctp_set_hbint(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}


DEFINE_APITEST(paddrpara, sso_ahb_zero_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2], newval;
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;

}
DEFINE_APITEST(paddrpara, sso_ahb_zero_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3], newval;
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_ahb_off_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint32_t munflags[2];

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_set_hbdisable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_ahb_off_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_set_hbdisable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}

	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB still enabled";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_ahb_on_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint32_t munflags[2];

	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_set_hbdisable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fds[0]);
		close(fds[1]);
		return(retstring);
	}
	result = sctp_set_hbenable(fds[0], 0, NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }
	close(fds[0]);
	close(fds[1]);
	return retstring;

}
DEFINE_APITEST(paddrpara, sso_ahb_on_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[4];
	uint16_t maxrxt[4];
	uint32_t pathmtu[4];
	uint32_t flags[4];
	uint32_t ipv6_flowlabel[4];
	uint8_t ipv4_tos[4];
	char *retstring = NULL;
	uint32_t munflags[2];
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_set_hbdisable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
		goto out_quick;
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
		goto out_quick;
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
		goto out_quick;
	}
	munflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	munflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(munflags[0] != munflags[1]) {
		retstring = "flag settings changed";
		goto out_quick;
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB can't be disabled";
	out_quick:
		close(fds[0]);
		close(fds[1]);
		return(retstring);
	}
	result = sctp_set_hbenable(fds[0], ids[0], NULL);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[3],
				      &maxrxt[3],
				      &pathmtu[3],
				      &flags[3],
				      &ipv6_flowlabel[3],
				      &ipv4_tos[3]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);

	if(hbinterval[3] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[3] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
	}
	if(pathmtu[0] != pathmtu[2]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[2]) {
		retstring = "HB did not re-enable";
        }

	return retstring;

}

DEFINE_APITEST(paddrpara, sso_apmrxt_int_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	char *retstring = NULL;
	uint16_t new_maxrxt;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, NULL, new_maxrxt);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_apmrxt_int_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	char *retstring = NULL;
	uint16_t new_maxrxt;
	sctp_assoc_t ids[3];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], ids[0], NULL, new_maxrxt);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}

	close(fds[0]);
	close(fds[1]);
	if(hbinterval[2] != hbinterval[0]) {
		retstring = "EP hb changed too";
	}
	if (maxrxt[2] != maxrxt[0]) {
		retstring = "EP max rxt changed too";
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change to new value";
	}
	if(pathmtu[0] != pathmtu[1]) {
		retstring = "pathmtu changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	return retstring;
}

DEFINE_APITEST(paddrpara, sso_apmtu_dis_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = pathmtu[0] / 2;
	if(newval < 1024)
		newval = 1024;

	result = sctp_set_pmtu(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] & SPP_PMTUD_ENABLE) {
		return "failed, pmtu still enabled";
	}
	if (pathmtu[1] != newval) {
		return "failed, pmtu not set to correct value";
	}
	muflags[0] = flags[0] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	muflags[1] = flags[1] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	if(muflags[0] != muflags[1]) {
		return "flag settings changed";
		
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		return "ipv4 tos settings changed";
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		return "ipv6 flowlabel settings changed";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_apmtu_dis_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = pathmtu[0] / 2;
	if(newval < 1024)
		newval = 1024;

	result = sctp_set_pmtu(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] & SPP_PMTUD_ENABLE) {
		return "failed, pmtu still enabled";
	}
	if (pathmtu[1] != newval) {
		return "failed, pmtu not set to correct value";
	}
	muflags[0] = flags[0] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	muflags[1] = flags[1] & ~(SPP_PMTUD_ENABLE|SPP_PMTUD_DISABLE);
	if(muflags[0] != muflags[1]) {
		return "flag settings changed";
		
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		return "ipv4 tos settings changed";
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		return "ipv6 flowlabel settings changed";
	}
	return NULL;

}

DEFINE_APITEST(paddrpara, sso_av6_flo_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 1;
	result = sctp_set_flow(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		return "failed, flags changed";
	}
	if (ipv6_flowlabel[1] != newval) {
		return "failed, flowlabel not set to correct value";
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		return "ipv4 tos settings changed";
	}
        return NULL;

}

DEFINE_APITEST(paddrpara, sso_av6_flo_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint32_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 1;
	result = sctp_set_flow(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		return "failed, flags changed";
	}
	if (ipv6_flowlabel[1] != newval) {
		return "failed, flowlabel not set to correct value";
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		return "ipv4 tos settings changed";
	}
        return NULL;
}

DEFINE_APITEST(paddrpara, sso_av4_tos_1_1)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint8_t newval;

	fds[0] = fds[1] = -1;
	result = sctp_socketpair(fds, 1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 4;
	result = sctp_set_tos(fds[0], 0, NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		return "failed, flags changed";
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		return "ipv6 flow label settings changed";		
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(newval != ipv4_tos[1]) {
		return "ipv4 tos settings not changed";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_av4_tos_1_M)
{
	int fds[2];
	int result;
	struct sockaddr *sa = NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	uint8_t newval;
	sctp_assoc_t ids[2];

	fds[0] = fds[1] = -1;
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result < 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	newval = 4;
	result = sctp_set_tos(fds[0], ids[0], NULL, newval);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	if (result< 0) {
		close(fds[0]);
		close(fds[1]);
		return(strerror(errno));
	}
	close(fds[0]);
	close(fds[1]);
	if(flags[1] != flags[0]) {
		return "failed, flags changed";
	}
	if (ipv6_flowlabel[1] != ipv6_flowlabel[0]) {
		return "ipv6 flow label settings changed";		
	}
	if (hbinterval[1] != hbinterval[0]) {
		return "hb interval settings changed";
	}

	if (maxrxt[1] != maxrxt[0]) {
		return "maxrxt settings changed";
	}
	if(newval != ipv4_tos[1]) {
		return "ipv4 tos settings not changed";
	}
	return NULL;
}

DEFINE_APITEST(paddrpara, sso_ainhhb_int_1_1)
{
	int fd, result;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0,1, 0);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = (hbinterval[0] * 2) + 1;

	result = sctp_set_hbint(fd, 0, NULL, newval);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_ainhhb_int_1_M)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint32_t newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = (hbinterval[0] * 2) + 1;

	/* Set the assoc value */
	result = sctp_set_hbint(fds[0], 0, NULL, newval);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);

}

DEFINE_APITEST(paddrpara, sso_ainhhb_zero_1_1)
{
	int fd, result;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0,1, 0);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = 0;

	result = sctp_set_hbzero(fd, 0, NULL);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_ainhhb_zero_1_M)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint32_t newval;
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	newval = 0;
	result = sctp_set_hbzero(fds[0], 0, NULL);
	/* Set the assoc value */

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != newval) {
		retstring = "HB interval set on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}

	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != newval) {
		retstring = "HB interval did not inherit";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_ainhhb_off_1_1)
{
	int fd, result;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];

	fd = sctp_one2one(0,1, 0);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_set_hbdisable(fd, 0, NULL);
	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	muflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
	}
	if (flags[1] & SPP_HB_ENABLE) {
		retstring = "HB ep did not disable HB";
	}

	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changecd";
	}
	if (flags[2] & SPP_HB_ENABLE) {
		retstring = "HB did not stay disabled";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_ainhhb_off_1_M)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;

	fds[0] = sctp_one2many(0,0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	result = sctp_set_hbdisable(fds[0], ids[0], NULL);
	/* Set the assoc value */

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(maxrxt[0] != maxrxt[1]) {
		retstring = "maxrxt changed";
	}
	muflags[0] = flags[0] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[1] & ~(SPP_HB_ENABLE|SPP_HB_DISABLE);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
	}
	if(flags[1] & SPP_HB_ENABLE) {
		retstring = "HB ep did not disable HB";
	}

	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(maxrxt[0] != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}

	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS|SPP_HB_ENABLE|SPP_HB_DISABLE);

	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}
	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changecd";
	}
	if(flags[2] & SPP_HB_ENABLE) {
		retstring = "HB did not stay disabled";
	}

 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_ainhpmrxt_int_1_1)
{
	int fd, result;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	uint16_t new_maxrxt;

	fd = sctp_one2one(0,1, 0);
	if(fd < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fd, 0, NULL, new_maxrxt);

	if (result< 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	result = sctp_get_paddr_param(fd, 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed ep failed";
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change on ep";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed on ep";
	}
	if (sctp_socketpair_reuse(fd, fds, 0) < 0) {
		retstring = strerror(errno);
		close(fd);
		goto out_nopair;
	}

	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[1], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(new_maxrxt != maxrxt[2]) {
		retstring = "maxrxt did NOT change on assoc";
		goto out;
	}
	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}

	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changed";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	close(fd);
	return (retstring);

}


DEFINE_APITEST(paddrpara, sso_ainhpmrxt_int_1_M)
{
	int result;
	uint32_t hbinterval[3];
	uint16_t maxrxt[3];
	uint32_t pathmtu[3];
	uint32_t flags[3];
	uint32_t ipv6_flowlabel[3];
	uint8_t ipv4_tos[3];
	uint32_t muflags[2];
	int fds[2];
	sctp_assoc_t ids[2];
	char *retstring=NULL;
	uint16_t new_maxrxt;

	fds[0] = sctp_one2many(0,0);
	if(fds[0] < 0) {
		return(strerror(errno));
	}
	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);
	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}

	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, NULL, new_maxrxt);

	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);


	if (result) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	/* Validate it set */
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "HB interval changed on ep failed";
	}
	if(new_maxrxt != maxrxt[1]) {
		retstring = "maxrxt did not change";
	}
	if(flags[0] != flags[1]) {
		retstring = "flag settings changed";
	}
	if (sctp_socketpair_1tom(fds, ids, 0) < 0) {
		retstring = strerror(errno);
		close(fds[0]);
		goto out_nopair;
	}
	/* Now what about on ep? */
	/* Now what about on ep? */
	result = sctp_get_paddr_param(fds[0], 0, NULL, &hbinterval[2],
				      &maxrxt[2],
				      &pathmtu[2],
				      &flags[2],
				      &ipv6_flowlabel[2],
				      &ipv4_tos[2]);

	if(new_maxrxt != maxrxt[2]) {
		retstring = "maxrxt changed";
		goto out;
	}

	muflags[0] = flags[0] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);
	muflags[1] = flags[2] & ~(SPP_IPV6_FLOWLABEL|SPP_IPV4_TOS);

	if(muflags[0] != muflags[1]) {
		retstring = "flag settings changed";
		goto out;
	}
	if (hbinterval[2] != hbinterval[1]) {
		retstring = "HB interval changecd";
	}
 out:
	close(fds[1]);
 out_nopair:
	close(fds[0]);
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_dhb_int_1_1)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	newval = (hbinterval[0] * 2) + 1;
	result = sctp_set_hbint(fds[0], 0, sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_dhb_int_1_M)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;
	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	newval = (hbinterval[0] * 2) + 1;
	result = sctp_set_hbint(fds[0], ids[0], sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_dhb_zero_1_1)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], 0, sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_dhb_zero_1_M)
{
	int result, num;
	uint32_t newval;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 0;

	result = sctp_set_hbzero(fds[0], ids[0], sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != newval) {
		retstring = "hb interval did not change";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_dhb_off_1_1)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t muflags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}

	result = sctp_set_hbdisable(fds[0], 0, sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	muflags[0] = (flags[0] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	muflags[1] = (flags[1] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	if (muflags[0] != muflags[1]) {
		retstring = "flags changed";
		goto out;

	}
	if( flags[1] & SPP_HB_ENABLE) {
		retstring = "HB not disabled";
	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);

}


DEFINE_APITEST(paddrpara, sso_dhb_off_1_M)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint32_t muflags[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;
	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result = sctp_set_hbdisable(fds[0],ids[0], sa);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	muflags[0] = (flags[0] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	muflags[1] = (flags[1] & ~(SPP_HB_DISABLE|SPP_HB_ENABLE));
	if (muflags[0] != muflags[1]) {
		retstring = "flags changed";
		goto out;

	}
	if( flags[1] & SPP_HB_ENABLE) {
		retstring = "HB not disabled";
	}

 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);

}

DEFINE_APITEST(paddrpara, sso_dpmrxt_int_1_1)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2], new_maxrxt;
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], 0, sa, new_maxrxt);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_dpmrxt_int_1_M)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2], new_maxrxt;
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids,  1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	new_maxrxt = 2 * maxrxt[0];
	result = sctp_set_maxrxt(fds[0], ids[0], sa, new_maxrxt);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != new_maxrxt) {
		retstring = "maxrxt did not change";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}

DEFINE_APITEST(paddrpara, sso_dav4_tos_1_1)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2], newval;
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 4;
	result = sctp_set_tos(fds[0], 0, sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(newval != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);

}

DEFINE_APITEST(paddrpara, sso_dav4_tos_1_M)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2], newval;
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	newval = 4;
	result = sctp_set_tos(fds[0], ids[0], sa, newval);
	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(newval != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);

}

DEFINE_APITEST(paddrpara, sso_hb_demand_1_1)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	struct sockaddr *sa = NULL;

	if (sctp_socketpair(fds, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
	num = sctp_getpaddrs(fds[0], 0, &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result =  sctp_set_paddr_param(fds[0], 0, 
				       sa, 0, 0, 0,
				       SPP_HB_DEMAND,
				       0, 0);


	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], 0, sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}


DEFINE_APITEST(paddrpara, sso_hb_demand_1_M)
{
	int result, num;
	int fds[2];
	char *retstring=NULL;
	uint32_t hbinterval[2];
	uint16_t maxrxt[2];
	uint32_t pathmtu[2];
	uint32_t flags[2];
	uint32_t ipv6_flowlabel[2];
	uint8_t ipv4_tos[2];
	sctp_assoc_t ids[2];
	struct sockaddr *sa = NULL;
	int cnt=0;

	fds[0] = fds[1] = -1;
	if (sctp_socketpair_1tom(fds, ids, 1) < 0) {
		retstring = strerror(errno);
		goto out_nopair;
	}
 try_again:
	num = sctp_getpaddrs(fds[0], ids[0], &sa);
	if( num < 0) {
		retstring = "sctp_getpaddr failed";
		goto out;
	}
	if (num < 2) {
		sctp_freepaddrs(sa);
		if(cnt < 1) {
			sleep(1);
			cnt++;
			goto try_again;
		}
		retstring = "host is not multi-homed can't run test";
		goto out;
	}

	/* Get all the values for assoc info on ep */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[0],
				      &maxrxt[0],
				      &pathmtu[0],
				      &flags[0],
				      &ipv6_flowlabel[0],
				      &ipv4_tos[0]);

	if (result) {
		sctp_freepaddrs(sa);
		retstring = strerror(errno);
		goto out;
	}
	result =  sctp_set_paddr_param(fds[0], ids[0], 
				       sa, 0, 0, 0,
				       SPP_HB_DEMAND,
				       0, 0);


	if (result< 0) {
		retstring = strerror(errno);
		sctp_freepaddrs(sa);
		goto out;
	}
	/* Now what got set? */
	result = sctp_get_paddr_param(fds[0], ids[0], sa, &hbinterval[1],
				      &maxrxt[1],
				      &pathmtu[1],
				      &flags[1],
				      &ipv6_flowlabel[1],
				      &ipv4_tos[1]);
	sctp_freepaddrs(sa);
	if (result < 0) {
		retstring = strerror(errno);
		goto out;
	}
	if(maxrxt[1] != maxrxt[0]) {
		retstring = "maxrxt changed";
		goto out;
	}
	if(hbinterval[1] != hbinterval[0]) {
		retstring = "hb interval changed";
		goto out;
	}
	if(ipv6_flowlabel[0] != ipv6_flowlabel[1]) {
		retstring = "v6 flowlabel changed";
		goto out;
	}
	if(ipv4_tos[0] != ipv4_tos[1]) {
		retstring = "v4 tos changed";
		goto out;
	}
	if (flags[0] != flags[1]) {
		retstring = "flags changed";
		goto out;

	}
 out:
	close(fds[0]);
	close(fds[1]);
 out_nopair:
	return (retstring);
}
/********************************************************
 *
 * SCTP_DEFAULT_SEND_PARAM tests
 *
 ********************************************************/
DEFINE_APITEST(defsend, gso_def_1_1)
{
	struct sctp_sndrcvinfo sinfo;
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo);
	close(fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if (sinfo.sinfo_stream != 0) {
		return "Default is not to send on stream 0";
	}
	if (sinfo.sinfo_flags != 0) {
		return "Default sends with options set";
	}
	if (sinfo.sinfo_ppid != 0) {
		return "Default sends with non-zero ppid";
	}
	if (sinfo.sinfo_context != 0) {
		return "Default sends with non-zero context";
	}
	return NULL;
}

DEFINE_APITEST(defsend, gso_def_1_M)
{
	struct sctp_sndrcvinfo sinfo;
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo);
	close(fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if (sinfo.sinfo_stream != 0) {
		return "Default is not to send on stream 0";
	}
	if (sinfo.sinfo_flags != 0) {
		return "Default sends with options set";
	}
	if (sinfo.sinfo_ppid != 0) {
		return "Default sends with non-zero ppid";
	}
	if (sinfo.sinfo_context != 0) {
		return "Default sends with non-zero context";
	}
	return NULL;
}

DEFINE_APITEST(defsend, sso_on_1_1)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}

	result = sctp_get_defsend(fd, 0, &sinfo[1]);
	close (fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;
}

DEFINE_APITEST(defsend, sso_on_1_M)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;

	fd = sctp_one2many(0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}

	result = sctp_get_defsend(fd, 0, &sinfo[1]);
	close (fd);
	if (result < 0) {
		return(strerror(errno));
	}
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;
}

DEFINE_APITEST(defsend, sso_asc_1_1)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;
}

DEFINE_APITEST(defsend, sso_asc_1_M)
{
	struct sctp_sndrcvinfo sinfo[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;
}
DEFINE_APITEST(defsend, sso_inherit_1_1)
{
	struct sctp_sndrcvinfo sinfo[2];
	int fd, result;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	close (fd);
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;

}

DEFINE_APITEST(defsend, sso_inherit_1_M)
{
	struct sctp_sndrcvinfo sinfo[2];
	int result;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	close (fds[0]);
	close (fds[1]);
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		return "Def send stream did not change";
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		return "sinfo_flags changed";
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		return "sinfo context changed";
	}
	return NULL;


}

DEFINE_APITEST(defsend, sso_inherit_ncep_1_1)
{
	struct sctp_sndrcvinfo sinfo[3];
	int fd, result;
	char *retstring = NULL;
	int fds[2];
	fds[0] = fds[1] = -1;

	fd = sctp_one2one(0, 1, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fd, 0, &sinfo[0]);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fd, 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	result = sctp_socketpair_reuse(fd, fds, 1);
	if (result < 0) {
		close (fd);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[1], 0, &sinfo[1]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[1], 0, &sinfo[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	/* Now get the listener value, it should NOT have changed */
	result = sctp_get_defsend(fd, 0, &sinfo[2]);
	if (result < 0) {
		close (fd);
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
 out:
 	close (fd);
 	close (fds[0]);
 	close (fds[1]);
 	return retstring;

}

DEFINE_APITEST(defsend, sso_inherit_ncep_1_M)
{
	struct sctp_sndrcvinfo sinfo[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	sctp_assoc_t ids[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}

	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	return retstring;
}

DEFINE_APITEST(defsend, sso_nc_other_asc_1_M)
{
	struct sctp_sndrcvinfo sinfo[3];
	int result;
	char *retstring = NULL;
	int fds[2];
	int fds2[2];
	sctp_assoc_t ids[2], ids2[2];
	fds[0] = fds[1] = -1;

	fds[0] = sctp_one2many(0, 1);
	if (fds[0] < 0) {
		return(strerror(errno));
	}
	result = sctp_get_defsend(fds[0], 0, &sinfo[0]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	sinfo[1] = sinfo[0];
	sinfo[1].sinfo_stream++;
	result = sctp_set_defsend(fds[0], 0, &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}

	result = sctp_socketpair_1tom(fds, ids,  1);
	if (result < 0) {
		close (fds[0]);
		return(strerror(errno));
	}
	/* Create a second assoc for fds[0] */
	fds2[0] = fds[0];
	result = sctp_socketpair_1tom(fds2, ids2,  1);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		return(strerror(errno));
	}

	result = sctp_get_defsend(fds[0], ids[0], &sinfo[1]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		return(strerror(errno));
	}
	
	if ((sinfo[0].sinfo_stream+1) != sinfo[1].sinfo_stream) {
		retstring = "Def send stream did not change";
		goto out;
	}
	if (sinfo[0].sinfo_flags != sinfo[1].sinfo_flags) {
		retstring =  "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_ppid != sinfo[1].sinfo_ppid) {
		retstring = "sinfo_flags changed";
		goto out;
	}
	if (sinfo[0].sinfo_context != sinfo[1].sinfo_context) {
		retstring = "sinfo context changed";
		goto out;
	}
	/* Change the assoc value */
	sinfo[2] = sinfo[1];
	sinfo[2].sinfo_stream++;
	result = sctp_set_defsend(fds[0], ids[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		return(strerror(errno));
	}

	result = sctp_get_defsend(fds[0], 0, &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		return(strerror(errno));
	}

	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected ep";
	}
	/* check other asoc */
	result = sctp_get_defsend(fds[0], ids2[0], &sinfo[2]);
	if (result < 0) {
		close (fds[0]);
		close (fds[1]);
		close (fds2[1]);
		return(strerror(errno));
	}
	if (sinfo[2].sinfo_stream != sinfo[1].sinfo_stream) {
		retstring = "Change of assoc, effected other assoc";
	}
 out:
	close (fds[0]);
	close (fds[1]);
	close (fds2[1]);
	return retstring;
}

/********************************************************
 *
 * SCTP_MAXSEG tests
 *
 ********************************************************/

DEFINE_APITEST(maxseg, gso_def_1_1)
{
	int fd, val, result;

	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if (val != 0) {
		return "maxseg not unlimited (i.e. 0)";
	}

	return NULL;
}

DEFINE_APITEST(maxseg, sso_set_1_1)
{
	int fd, val[3], result;
	fd = sctp_one2one(0, 0, 1);
	if (fd < 0) {
		return(strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[0]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}

	if ((val[0] == 0) || (val[0] > 1452)) {
		val[1] = 1452;
	} else {
		val[1] = val[0] - 100;
	}
	result = sctp_set_maxseg(fd, 0, val[1]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	result = sctp_get_maxseg(fd, 0, &val[2]);
	if(result < 0) {
		close(fd);
		return(strerror(errno));
	}
	close(fd);
	if(val[1] < val[2]) {
		return "Set did not work";
	}
	return NULL;
}

