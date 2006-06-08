#ifndef __rserpool_util_h__
#define __rserpool_util_h__
#include <rserpool_lib.h>

/* in rsp_timer.c */

void rsp_free_req(struct rsp_enrp_req *req);

struct rsp_enrp_req *rsp_aloc_req(char *name, int namelen, void *msg, int msglen);

void rsp_timer_check ( void );

void rsp_start_enrp_server_hunt(struct rsp_enrp_scope *sd, struct rsp_timer_entry *te, int non_blocking);


int
rsp_start_timer(struct rsp_enrp_scope *sd,
		struct rsp_socket_hash 	*sdata, 
		uint32_t time_out_ms, 
		struct rsp_enrp_req *msg,
		int type, 
		uint8_t want_cond, 
		uint16_t sleeper_cnt,
		struct rsp_timer_entry *ote);

#endif
