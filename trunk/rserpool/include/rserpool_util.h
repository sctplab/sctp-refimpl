#ifndef __rserpool_util_h__
#define __rserpool_util_h__


/* in rsp_timer.c */

void rsp_free_req(struct rsp_enrp_req *req);

struct rsp_enrp_req *rsp_aloc_req(char *name, int namelen, void *msg, int msglen)

void rsp_timer_check ( void );

#endif
