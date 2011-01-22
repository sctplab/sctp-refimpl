#ifndef __incast_fmt_h__
#define __incast_fmt_h__

#define MAX_SINGLE_MSG 9000 /* Largest 1 mtu normally 1448*/
#define DEFAULT_MSG_SIZE 1448 /* TCP header and TS and IP hdr - 1500 */

struct incast_msg_req {
	int number_of_packets;
	int size;
};

#define DEFAULT_SVR_PORT 9902

#define SRV_STATE_NEW       1
#define SRV_STATE_REQ_SENT  2
#define SRV_STATE_COMPLETE  3

struct incast_peer {
	int fd;
	int state;
	struct timeval start;
	struct timeval end;
	struct sockaddr_in addr;
	int msg_cnt;
	int size;
	int total_size;
};


#endif
