#ifndef __cmp_msg__h__
#define __cmp_msg__h__


struct txfr_msg{
	uint32_t type;
	uint32_t seqno;
	char data[0];
};

#define SCTP_TRANSFER_DATA       0x00000001
#define SCTP_TRANSFER_COMLETE    0x00000002
#define SCTP_REQ_CLR_ERROR_STAT  0x00000003
#define SCTP_ERROR_STAT_RESP     0x00000004
#define SCTP_ALL_DONE_CLOSE      0x00000005

struct txfr_request{
	uint32_t type;
	int num_of_blocks;
	int block_size;
	int snd_window;
	int rcv_window;
	u_int8_t tos_value;
};

struct txfr_complete {
	uint32_t type;
	int num_of_blocks;
};

struct txfr_request_info {
	uint32_t type;
};

struct txfr_request_info_resp {
	uint32_t type;
	uint32_t fr_errors;
	uint32_t fr_inwindow;
	uint32_t retrans;
	uint32_t num_t3;
	uint32_t tsn_sent;
	uint32_t tsn_recvd;
	uint32_t blk_by_rwnd;
	uint32_t blk_by_cwnd;
	uint32_t max_burst_applied;
};

/*
 * Client                      Server
 *                            sd = accept();
 * ------txfr_request--->
 * <------ DATA---(as defined)---
 * <------ txfr_complete ---
 * -------request_info(ERRORS)--->
 * <------info_resp(ERRORS)------
 * -------request_info(SCTP_ALL_DONE_CLOSE)--->
 *                              close(sd)
 */

#endif
