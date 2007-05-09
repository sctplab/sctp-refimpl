/* 
 * This file will be generated automatically in the future.
 * Up to now you must insert one line per test manually.
 *
 */
 
#include "api_tests.h"

DECLARE_APITEST(sctp_sendmsg_c_p_c_a);
DECLARE_APITEST(sctp_sendmsg_c_p_c_a_over);
DECLARE_APITEST(sctp_sendmsg_w_p_c_a);
DECLARE_APITEST(sctp_sendmsg_w_p_c_a_over);
DECLARE_APITEST(sctp_sendmsg_c_p_w_a);
DECLARE_APITEST(sctp_sendmsg_c_p_w_a_over);
DECLARE_APITEST(sctp_sendmsg_w_p_w_a);
DECLARE_APITEST(sctp_sendmsg_w_p_w_a_over);
DECLARE_APITEST(sctp_sendmsg_b_p_c_a);
DECLARE_APITEST(sctp_sendmsg_b_p_c_a_over);
DECLARE_APITEST(sctp_sendmsg_c_p_b_a);
DECLARE_APITEST(sctp_sendmsg_c_p_b_a_over);
DECLARE_APITEST(sctp_sendmsg_b_p_b_a);
DECLARE_APITEST(sctp_sendmsg_b_p_b_a_over);
DECLARE_APITEST(sctp_sendmsg_w_p_b_a);
DECLARE_APITEST(sctp_sendmsg_w_p_b_a_over);
DECLARE_APITEST(sctp_sendmsg_b_p_w_a);
DECLARE_APITEST(sctp_sendmsg_b_p_w_a_over);
DECLARE_APITEST(sctp_sendmsg_non_null_zero);
DECLARE_APITEST(sctp_sendmsg_non_null_zero_over);
DECLARE_APITEST(sctp_sendmsg_null_zero);
DECLARE_APITEST(sctp_sendmsg_null_zero_over);
DECLARE_APITEST(sctp_sendmsg_null_non_zero);
DECLARE_APITEST(sctp_sendmsg_null_non_zero_over);
DECLARE_APITEST(sctp_gso_rtoinfo_1_1_defaults);
DECLARE_APITEST(sctp_gso_rtoinfo_1_M_defaults);
DECLARE_APITEST(sctp_gso_rtoinfo_1_1_bad_id);
DECLARE_APITEST(sctp_gso_rtoinfo_1_M_bad_id);
DECLARE_APITEST(sctp_sso_rtoinfo_1_1_good);
DECLARE_APITEST(sctp_sso_rtoinfo_1_M_good);
DECLARE_APITEST(sctp_sso_rtoinfo_1_1_bad_id);
DECLARE_APITEST(sctp_sso_rtoinfo_1_M_bad_id);
DECLARE_APITEST(sctp_sso_rtoinfo_1_1_init);
DECLARE_APITEST(sctp_sso_rtoinfo_1_M_init);
DECLARE_APITEST(sctp_sso_rtoinfo_1_1_max);
DECLARE_APITEST(sctp_sso_rtoinfo_1_M_max);
DECLARE_APITEST(sctp_sso_rtoinfo_1_1_min);
DECLARE_APITEST(sctp_sso_rtoinfo_1_M_min);

struct test all_tests[] = {
	REGISTER_APITEST(sctp_sendmsg_c_p_c_a),
	REGISTER_APITEST(sctp_sendmsg_c_p_c_a_over),
	REGISTER_APITEST(sctp_sendmsg_w_p_c_a),
	REGISTER_APITEST(sctp_sendmsg_w_p_c_a_over),
	REGISTER_APITEST(sctp_sendmsg_c_p_w_a),
	REGISTER_APITEST(sctp_sendmsg_c_p_w_a_over),
	REGISTER_APITEST(sctp_sendmsg_w_p_w_a),
	REGISTER_APITEST(sctp_sendmsg_w_p_w_a_over),
	REGISTER_APITEST(sctp_sendmsg_b_p_c_a),
	REGISTER_APITEST(sctp_sendmsg_b_p_c_a_over),
	REGISTER_APITEST(sctp_sendmsg_c_p_b_a),
	REGISTER_APITEST(sctp_sendmsg_c_p_b_a_over),
	REGISTER_APITEST(sctp_sendmsg_b_p_b_a),
	REGISTER_APITEST(sctp_sendmsg_b_p_b_a_over),
	REGISTER_APITEST(sctp_sendmsg_w_p_b_a),
	REGISTER_APITEST(sctp_sendmsg_w_p_b_a_over),
	REGISTER_APITEST(sctp_sendmsg_b_p_w_a),
	REGISTER_APITEST(sctp_sendmsg_b_p_w_a_over),
	REGISTER_APITEST(sctp_sendmsg_non_null_zero),
	REGISTER_APITEST(sctp_sendmsg_non_null_zero_over),
	REGISTER_APITEST(sctp_sendmsg_null_zero),
	REGISTER_APITEST(sctp_sendmsg_null_zero_over),
	REGISTER_APITEST(sctp_sendmsg_null_non_zero),
	REGISTER_APITEST(sctp_sendmsg_null_non_zero_over),
	REGISTER_APITEST(sctp_gso_rtoinfo_1_1_defaults),
	REGISTER_APITEST(sctp_gso_rtoinfo_1_M_defaults),
	REGISTER_APITEST(sctp_gso_rtoinfo_1_1_bad_id),
	REGISTER_APITEST(sctp_gso_rtoinfo_1_M_bad_id),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_1_good),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_M_good),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_1_bad_id),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_M_bad_id),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_1_init),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_M_init),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_1_max),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_M_max),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_1_min),
	REGISTER_APITEST(sctp_sso_rtoinfo_1_M_min),
};

unsigned int number_of_tests = (sizeof(all_tests) / sizeof(all_tests[0]));
