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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	
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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	result = sctp_get_rto_info(fd, 1, NULL, NULL, NULL);
	
	close(fd);
	
	if (result)
		return strerror(errno);

	return NULL;
}

DEFINE_APITEST(sctp_gso_rtoinfo_1_M_bad_id)
{
	int fd, result;
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	
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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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
	
	fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

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

