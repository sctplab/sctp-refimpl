#ifndef __pdapi_req_h__
#define __pdapi_req_h__

#include "sctp_csum.h"


#define PDAPI_DATA_BLOCK_SIZE 16000

#define PDAPI_DATA_MESSAGE    1
#define PDAPI_REQUEST_MESSAGE 2
#define PDAPI_END_MESSAGE     3


struct data_block {
	struct data_block *next;
	struct sctp_sndrcvinfo info;
	int last;
	int sz;
	unsigned char data[4];
};

struct requests {
	sctp_assoc_t assoc_id;
	uint32_t msg_cnt;
	struct sockaddr_in who;
	struct requests *next;
	struct requests *prev;
	struct data_block *first;
	struct data_block *tail;
};


struct pdapi_request {
	unsigned char request;
	unsigned char filler;
	uint16_t seq;
	union {  
		uint32_t checksum;
		int size;
		unsigned char data[0];
	}msg;
};


#endif
