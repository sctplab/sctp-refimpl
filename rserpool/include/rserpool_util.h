#ifndef __rserpool_util_h__
#define __rserpool_util_h__
#include <rserpool_lib.h>

/* in rsp_timer.c */

void rsp_free_req(struct rsp_enrp_req *req);

struct rsp_enrp_req *rsp_aloc_req(char *name, int namelen, void *msg, int msglen, int type);

void rsp_timer_check ( void );

void rsp_start_enrp_server_hunt(struct rsp_enrp_scope *sd, int non_blocking);


struct rsp_timer_entry *
asap_find_req(struct rsp_enrp_scope *scp,
	      char *name, 
	      int name_len, 
	      int type,
	      int leave_locked);


int
rsp_start_timer(struct rsp_enrp_scope *sd,
		struct rsp_socket_hash 	*sdata, 
		uint32_t time_out_ms, 
		struct rsp_enrp_req *msg,
		int type, 
		uint8_t want_cond, 
		struct rsp_timer_entry **ote);

int rsp_stop_timer(struct rsp_timer_entry *te);

void rsp_process_fds(int poll_ret);



void
handle_enrpserver_notification (struct rsp_enrp_scope *scp, 
				char *buf, 
				struct sctp_sndrcvinfo *sinfo, 
				ssize_t sz,
				struct sockaddr *from, 
				socklen_t from_len);

void
handle_asapmsg_fromenrp (struct rsp_enrp_scope *scp, 
			 char *buf, 
			 struct sctp_sndrcvinfo *sinfo, 
			 ssize_t sz,
			 struct sockaddr *from, 
			 socklen_t from_len);
#endif
