#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "sctp_utilities.h"
#include "api_tests.h"

DEFINE_APITEST(sctp_gso_rtoinfo_1_1_defaults)
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

DEFINE_APITEST(sctp_gso_rtoinfo_1_M_defaults)
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

DEFINE_APITEST(sctp_gso_rtoinfo_1_1_bad_id)
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

DEFINE_APITEST(sctp_gso_rtoinfo_1_M_bad_id)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_good)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_good)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_bad_id)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_bad_id)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_init)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_init)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_max)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_max)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_min)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_min)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_same)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_same)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_1)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_2)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_3)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_4)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_5)
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

DEFINE_APITEST(sctp_sso_rtoinfo_ill_6)
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

DEFINE_APITEST(sctp_gso_rtoinfo_1_1_c_bad_id)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_c_bad_id)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_1_inherit)
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

DEFINE_APITEST(sctp_sso_rtoinfo_1_M_inherit)
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
