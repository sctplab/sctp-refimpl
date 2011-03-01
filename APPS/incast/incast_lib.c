/*-
 * Copyright (c) 2011, by Randall Stewart.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * a) Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of the authors nor the names of its 
 *    contributors may be used to endorse or promote products derived 
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <incast_fmt.h>
#include <net/if_dl.h>
#include <arpa/inet.h>

#ifndef STRING_BUF_SZ
#define STRING_BUF_SZ 1024
#endif



struct incast_peer_outrec32 {
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	int peerno;
	int state;
};

struct incast_peer_outrec64 {
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	int peerno;
	int state;
};


int
read_peer_rec(struct incast_peer_outrec *rec, FILE *io, int infile_size)
{
	int ret;
	struct incast_peer_outrec32 bit32;
	struct incast_peer_outrec64 bit64;
	if (sizeof(time_t) == infile_size) {
		return (fread(rec, sizeof(struct incast_peer_outrec), 1, io));
	}
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		rec->start.tv_sec = (time_t)bit32.start_tv_sec;
		rec->start.tv_nsec = (long)bit32.start_tv_nsec;
		rec->end.tv_sec = (time_t)bit32.end_tv_sec;
		rec->end.tv_nsec = (long)bit32.end_tv_nsec;
		rec->peerno = bit32.peerno;
		rec->state = bit32.state;
	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 32 bit */
		rec->start.tv_sec = (time_t)bit64.start_tv_sec;
		rec->start.tv_nsec = (long)bit64.start_tv_nsec;
		rec->end.tv_sec = (time_t)bit64.end_tv_sec;
		rec->end.tv_nsec = (long)bit64.end_tv_nsec;
		rec->peerno = bit64.peerno;
		rec->state = bit64.state;
	}
	return(1);
}

struct ele_lead_hdr32 {
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	uint32_t number_of_bytes;
	uint32_t number_servers;
};

struct ele_lead_hdr64 {
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	uint32_t number_of_bytes;
	uint32_t number_servers;
};


int
read_ele_hdr(struct elephant_lead_hdr *hdr, FILE *io, int infile_size)
{
	int ret;
	struct ele_lead_hdr32 bit32;
	struct ele_lead_hdr64 bit64;
	if (sizeof(time_t) == infile_size) {
		return (fread(hdr, sizeof(struct elephant_lead_hdr), 1, io));
	}
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		hdr->start.tv_sec = (time_t)bit32.start_tv_sec;
		hdr->start.tv_nsec = (long)bit32.start_tv_nsec;
		hdr->end.tv_sec = (time_t)bit32.end_tv_sec;
		hdr->end.tv_nsec = (long)bit32.end_tv_nsec;
		hdr->number_servers = bit32.number_servers;
		hdr->number_of_bytes = bit32.number_of_bytes;

	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 32 bit */
		hdr->start.tv_sec = (time_t)bit64.start_tv_sec;
		hdr->start.tv_nsec = (long)bit64.start_tv_nsec;
		hdr->end.tv_sec = (time_t)bit64.end_tv_sec;
		hdr->end.tv_nsec = (long)bit64.end_tv_nsec;
		hdr->number_servers = bit64.number_servers;
		hdr->number_of_bytes = bit64.number_of_bytes;
	}
	return (1);
}




struct incast_lead_hdr32 {
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t connected_tv_sec;
	uint32_t connected_tv_nsec;
	uint32_t sending_tv_sec;
	uint32_t sending_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	uint32_t number_servers;
	uint32_t passcnt;
};

struct incast_lead_hdr64 {
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t connected_tv_sec;
	uint64_t connected_tv_nsec;
	uint64_t sending_tv_sec;
	uint64_t sending_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	uint32_t number_servers;
	uint32_t passcnt;
};


int
read_incast_hdr(struct incast_lead_hdr *hdr, FILE *io, int infile_size)
{
	int ret;
	struct incast_lead_hdr32 bit32;
	struct incast_lead_hdr64 bit64;
	if (sizeof(time_t) == infile_size) {
		return (fread(hdr, sizeof(struct incast_lead_hdr), 1, io));
	}
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		hdr->start.tv_sec = (time_t)bit32.start_tv_sec;
		hdr->start.tv_nsec = (long)bit32.start_tv_nsec;
		hdr->connected.tv_sec = (time_t)bit32.connected_tv_sec;
		hdr->connected.tv_nsec = (long)bit32.connected_tv_nsec;
		hdr->sending.tv_sec = (time_t)bit32.sending_tv_sec;
		hdr->sending.tv_nsec = (long)bit32.sending_tv_nsec;
		hdr->end.tv_sec = (time_t)bit32.end_tv_sec;
		hdr->end.tv_nsec = (long)bit32.end_tv_nsec;
		hdr->number_servers = bit32.number_servers;
		hdr->passcnt = bit32.passcnt;

	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 32 bit */
		hdr->start.tv_sec = (time_t)bit64.start_tv_sec;
		hdr->start.tv_nsec = (long)bit64.start_tv_nsec;
		hdr->connected.tv_sec = (time_t)bit64.connected_tv_sec;
		hdr->connected.tv_nsec = (long)bit64.connected_tv_nsec;
		hdr->sending.tv_sec = (time_t)bit64.sending_tv_sec;
		hdr->sending.tv_nsec = (long)bit64.sending_tv_nsec;
		hdr->end.tv_sec = (time_t)bit64.end_tv_sec;
		hdr->end.tv_nsec = (long)bit64.end_tv_nsec;
		hdr->number_servers = bit64.number_servers;
		hdr->passcnt = bit64.passcnt;
	}
	return (1);
}




struct elephant_sink_rec64 {
	struct sockaddr_in from;
	uint64_t start_tv_sec;
	uint64_t start_tv_nsec;
	uint64_t end_tv_sec;
	uint64_t end_tv_nsec;
	uint64_t mono_start_tv_sec;
	uint64_t mono_start_tv_nsec;
	uint64_t mono_end_tv_sec;
	uint64_t mono_end_tv_nsec;
	int number_bytes;
};

struct elephant_sink_rec32 {
	struct sockaddr_in from;
	uint32_t start_tv_sec;
	uint32_t start_tv_nsec;
	uint32_t end_tv_sec;
	uint32_t end_tv_nsec;
	uint32_t mono_start_tv_sec;
	uint32_t mono_start_tv_nsec;
	uint32_t mono_end_tv_sec;
	uint32_t mono_end_tv_nsec;
	int number_bytes;
};

int no_cc_change=0;

int 
read_a_sink_rec(struct elephant_sink_rec *sink, FILE *io, int infile_size)
{
	int ret;
	struct elephant_sink_rec32 bit32;
	struct elephant_sink_rec64 bit64;

	if (sizeof(time_t) == infile_size) {
		return(fread(sink, sizeof(struct elephant_sink_rec), 1, io));
	} 
	/* Ok we have a size mis-match */
	if (sizeof(time_t) == 8) {
		/* We are reading from a 32 bit into 
		 * a 64 bit record. 
		 */
		ret = fread(&bit32, sizeof(bit32), 1, io);
		if (ret != 1) {
			return (ret);
		}
		/* now convert to 64 bit */
		memcpy(&sink->from, &bit32.from, sizeof(sink->from));
		sink->number_bytes = bit32.number_bytes;
		sink->start.tv_sec = (time_t)bit32.start_tv_sec;
		sink->start.tv_nsec = (long)bit32.start_tv_nsec;
		sink->end.tv_sec = (time_t)bit32.end_tv_sec;
		sink->end.tv_nsec = (long)bit32.end_tv_nsec;
		sink->mono_start.tv_sec = (time_t)bit32.mono_start_tv_sec;
		sink->mono_start.tv_nsec = (long)bit32.mono_start_tv_nsec;
		sink->mono_end.tv_sec = (time_t)bit32.mono_end_tv_sec;
		sink->mono_end.tv_nsec = (long)bit32.mono_end_tv_nsec;
	} else {
		/* We are on a 32 bit reading a 64 bit record */
		ret = fread(&bit64, sizeof(bit64), 1, io);
		if (ret != 1) {
			return (ret);
		}
		memcpy(&sink->from, &bit64.from, sizeof(sink->from));
		sink->number_bytes = bit64.number_bytes;
		sink->start.tv_sec = (time_t)bit64.start_tv_sec;
		sink->start.tv_nsec = (long)bit64.start_tv_nsec;
		sink->end.tv_sec = (time_t)bit64.end_tv_sec;
		sink->end.tv_nsec = (long)bit64.end_tv_nsec;
		sink->mono_start.tv_sec = (time_t)bit64.mono_start_tv_sec;
		sink->mono_start.tv_nsec = (long)bit64.mono_start_tv_nsec;
		sink->mono_end.tv_sec = (time_t)bit64.mono_end_tv_sec;
		sink->mono_end.tv_nsec = (long)bit64.mono_end_tv_nsec;
	}
	return (1);

}

void
print_an_address(struct sockaddr *a, int cr)
{
	char stringToPrint[STRING_BUF_SZ];
	u_short prt;
	char *srcaddr, *txt;

	if (a == NULL) {
		printf("NULL");
		if (cr) printf("\n");
		return;
	}
	if (a->sa_family == AF_INET) {
		srcaddr = (char *)&((struct sockaddr_in *)a)->sin_addr;
		txt = "IPv4 Address: ";
		prt = ntohs(((struct sockaddr_in *)a)->sin_port);
	} else if (a->sa_family == AF_INET6) {
		srcaddr = (char *)&((struct sockaddr_in6 *)a)->sin6_addr;
		prt = ntohs(((struct sockaddr_in6 *)a)->sin6_port);
		txt = "IPv6 Address: ";
	} else if (a->sa_family == AF_LINK) {
		int i;
		char tbuf[STRING_BUF_SZ];
		u_char adbuf[STRING_BUF_SZ];
		struct sockaddr_dl *dl;

		dl = (struct sockaddr_dl *)a;
		strncpy(tbuf, dl->sdl_data, dl->sdl_nlen);
		tbuf[dl->sdl_nlen] = 0;
		printf("Intf:%s (len:%d)Interface index:%d type:%x(%d) ll-len:%d ",
		    tbuf,
		    dl->sdl_nlen,
		    dl->sdl_index,
		    dl->sdl_type,
		    dl->sdl_type,
		    dl->sdl_alen
		    );
		memcpy(adbuf, LLADDR(dl), dl->sdl_alen);
		for (i = 0; i < dl->sdl_alen; i++) {
			printf("%2.2x", adbuf[i]);
			if (i < (dl->sdl_alen - 1))
				printf(":");
		}
		if (cr) printf("\n");
		return;
	} else {
		return;
	}
	if (inet_ntop(a->sa_family, srcaddr, stringToPrint, sizeof(stringToPrint))) {
		if (a->sa_family == AF_INET6) {
			printf("%s%s:%d scope:%d",
			    txt, stringToPrint, prt,
			    ((struct sockaddr_in6 *)a)->sin6_scope_id);
			if (cr) printf("\n");
		} else {
			printf("%s%s:%d", txt, stringToPrint, prt);
			if (cr) printf("\n");
		}

	} else {
		printf("%s unprintable?", txt);
		if (cr) printf("\n");
	}
}


#undef STRING_BUF_SZ

int
translate_ip_address(char *host, struct sockaddr_in *sa)
{
	struct hostent *hp;
	int len, cnt, i;

	if (sa == NULL) {
		return (-1);
	}
	len = strlen(host);
	cnt = 0;
	for (i = 0; i < len; i++) {
		if (host[i] == '.')
			cnt++;
	}
	if (cnt < 3) {
		/* make it treat it like a host name */
		sa->sin_addr.s_addr = 0xffffffff;
	}
	sa->sin_len = sizeof(struct sockaddr_in);
	sa->sin_family = AF_INET;
	if (sa->sin_addr.s_addr == 0xffffffff) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			return (htonl(strtoul(host, NULL, 0)));
		}
		memcpy(&sa->sin_addr, hp->h_addr_list[0], sizeof(sa->sin_addr));
	} else {
		sa->sin_addr.s_addr = htonl(inet_network(host));
	}
	if (sa->sin_addr.s_addr == 0xffffffff) {
		return (-1);
	}
	return (0);
}

static int
send_req_to_servers(struct incast_control *ctrl)
{
	struct incast_peer *peer;
	struct incast_msg_req req;

	req.number_of_packets = htonl(ctrl->cnt_req);
	req.size = htonl(ctrl->size);
	ctrl->completed_server_cnt = 0;
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		if(send(peer->sd, (void *)&req, sizeof(req), 0) < sizeof(req)) {
			printf("Send error:%d to", errno);
			print_an_address((struct sockaddr *)&peer->addr, 1);
			return (-1);
		}
		peer->state = SRV_STATE_REQ_SENT;
	}
	return (0);
}

static int
build_conn_into_kq(int kq, struct incast_control *ctrl)
{
	struct kevent ke;
	struct incast_peer *peer;
	int proto, sockopt, optval;
	socklen_t optlen;
	/* First pass open up all the client sockets
	 * and prepare them.
	 */
	if (ctrl->sctp_on) {
		proto = IPPROTO_SCTP;
		sockopt = SCTP_NODELAY;
	} else {
		proto = IPPROTO_TCP;
		sockopt = TCP_NODELAY;
	}
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		peer->sd = socket(AF_INET, SOCK_STREAM, proto);
		if (peer->sd == -1) {
			printf("Can't open a socket for a peer err:%d\n",
			       errno);
			return(-1);
		}
		if ((proto == IPPROTO_SCTP) && (no_cc_change == 0)) {
			struct sctp_assoc_value av;
			socklen_t optlen;
			av.assoc_id = 0;
			av.assoc_value = SCTP_CC_RTCC;
			optlen = sizeof(av);
			if (setsockopt(peer->sd, proto,  SCTP_PLUGGABLE_CC,
			       &av, optlen) == -1) {
				printf("Can't turn on RTCC cc err:%d\n",
				       errno);
				exit(-1);
			}
		}
		/* Set no delay */
		optval = 1;
		optlen = sizeof(optval);
		if (setsockopt(peer->sd, proto, sockopt, 
			       &optval, optlen) == -1) {
			printf("Can't set no-delay err:%d\n", errno);
			return (-1);
		}
		/* Now bind the local address */
		optlen = sizeof(struct sockaddr_in);
		if (bind(peer->sd, (struct sockaddr *)&ctrl->bind_addr, 
			 optlen)) {
			printf("Can't bind err:%d\n", errno);
			return (-1);
		}
	}
	/* Ok all connections are built, time to connect */
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		optlen = sizeof(struct sockaddr_in);
		if(connect(peer->sd, (struct sockaddr *)&peer->addr,
			   optlen)) {
			printf("Connect err:%d for address", errno);
			print_an_address((struct sockaddr *)&peer->addr, 1);
			return (-1);
		}
	}
	/* Now stick it all in the kqueue */
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		EV_SET(&ke, peer->sd, EVFILT_READ, (EV_ADD|EV_CLEAR),
		       0, 0, (void *)peer);
		if (kevent(kq, &ke, 1, NULL, 0, NULL) < 0) {
			printf("Failed to add kqueue event for peer err:%d\n",
				errno);
			printf("Peer ");
			print_an_address((struct sockaddr *)&peer->addr, 1);
			return(-1);
		}
	}
	return (0);
}

static void
clean_up_conn(struct incast_control *ctrl)
{
	struct incast_peer *peer;
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		if (peer->sd != -1) {
			close(peer->sd);
			peer->sd =-1;
		}
		peer->state = SRV_STATE_NEW;
		peer->msg_cnt = 0; 
		peer->byte_cnt = 0;
	}
}

static void
incast_read_from(struct incast_control *ctrl, 
		 struct incast_peer *peer, int to_read)
{
	char buf[MAX_SINGLE_MSG];
	int read_am, r_ret, tot_read;
	tot_read = 0;
again:
	if ((to_read-tot_read) > MAX_SINGLE_MSG) {
		read_am = MAX_SINGLE_MSG;
	} else {
		read_am = (to_read-tot_read);
	}
	r_ret = recv(peer->sd, buf, read_am, 0);
	if (r_ret < 1) {
		printf("Error in socket read err:%d (r_ret:%d sd:%d)\n", errno, r_ret, peer->sd);
		return;
	}
	if (peer->state == SRV_STATE_REQ_SENT) {
		/* First read get start time */
		if(clock_gettime(CLOCK_MONOTONIC_PRECISE, 
				 &peer->start)) {
			printf("Warning - can't get clock err:%d\n", errno);
			peer->state = SRV_STATE_ERROR;
			ctrl->completed_server_cnt++;
			close(peer->sd);
			peer->sd = -1;
		} else {
			peer->state = SRV_STATE_READING;
		}
	}
	peer->msg_cnt++;
	peer->byte_cnt += r_ret;
	tot_read += r_ret;
	if (tot_read < to_read) {
		/* Consume all kqueue told us to */
		goto again;
	}
	return;
}

static int
gather_kq_results(int kq, struct incast_control *ctrl)
{
	/* Question:
	 *  Should we run a guard timer and leave behind
	 *  connections that don't respond?
	 */
	int read_cmpl;
	struct kevent ke;
	struct incast_peer *peer;
	int not_done, kret;

	read_cmpl = (ctrl->size * ctrl->cnt_req);
	not_done = 1;
	while (not_done) {
		kret = kevent(kq, NULL, 0, &ke, 1, NULL);
		if (kret < 1) {
			printf("Kq returned %d errno:%d\n",
			       kret, errno);
			return (-1);
		}
		if (ke.filter != EVFILT_READ) {
			printf("Kevent filter is %d errno:%d ??\n - aborting",
			       ke.filter, errno);
			return (-1);
		}
		peer = (struct incast_peer *)ke.udata;
		if (ke.data) { 
			/*(number of bytes)*/
			incast_read_from(ctrl, peer, ke.data);
		}
		if (ke.flags & EV_EOF) {
			close(peer->sd);
			peer->sd = -1;
			ctrl->completed_server_cnt++;
			if (peer->state == SRV_STATE_READING) {
				if(clock_gettime(CLOCK_MONOTONIC_PRECISE, 
						 &peer->end)) {
					printf("Clock get fails\n");
					peer->state = SRV_STATE_ERROR;
				} else {
					if (peer->byte_cnt >= read_cmpl) {
						peer->state = SRV_STATE_COMPLETE;
					} else {
						peer->state = SRV_STATE_ERROR;
					}
				}
			} else  if ((peer->state != SRV_STATE_ERROR) &&
				    (peer->state != SRV_STATE_COMPLETE)){
				/* peer closes without any data coming in? */
				printf("Peer EV_EOF and no data? state:%d ke.data:%d\n", 
				       peer->state, (int)ke.data);
				peer->state = SRV_STATE_ERROR;
			}
		} else {
			/* Are we done? */
			if (peer->byte_cnt >= read_cmpl) {
				peer->state = SRV_STATE_COMPLETE;
				if(clock_gettime(CLOCK_MONOTONIC_PRECISE, 
						 &peer->end)) {
					printf("Clock get fails\n");
					peer->state = SRV_STATE_ERROR;
				}
				close(peer->sd);
				peer->sd = -1;
				ctrl->completed_server_cnt++;
			}
		}
		/* Are we done with all servers? */
		if (ctrl->completed_server_cnt >= ctrl->number_server) {
			not_done = 0;
		}
	}
	return (0);
}

void
display_results(struct incast_control *ctrl, int pass, int no_time)
{
	int peerno;
	struct incast_peer *peer;
	peerno = 0;
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		peerno++;
		if(peer->state == SRV_STATE_ERROR) {
			printf("Peer:%d(", peerno);
			print_an_address((struct sockaddr *)&peer->addr, 0);
			printf(") Pass:%d - Peer Error\n", pass);
			printf(" -- read_cnt:%d byte_cnt:%d\n",
			       peer->msg_cnt, peer->byte_cnt);
		} else if (peer->state != SRV_STATE_COMPLETE) {
			printf("Peer:%d(", peerno);
			print_an_address((struct sockaddr *)&peer->addr, 0);
			printf(") Pass:%d - Peer left in state:%d?\n", 
			       pass, peer->state);
			printf(" -- read_cnt:%d byte_cnt:%d\n",
			       peer->msg_cnt, peer->byte_cnt);
		} else {
			timespecsub(&peer->end, &peer->start);
			if ((no_time == 0) && ((peer->end.tv_sec) || (peer->end.tv_nsec > 300000000))) {
				/* More than 300ms */
				printf("Peer:%d(", peerno);
				print_an_address((struct sockaddr *)&peer->addr, 0);
				printf(") Pass:%d %ld.%9.9ld\n",
				       pass, (long int)peer->end.tv_sec, 
				       peer->end.tv_nsec);
			} else 	if (ctrl->verbose) {
				printf("Peer:%d(", peerno);
				print_an_address((struct sockaddr *)&peer->addr, 0);
				printf(") Pass:%d %ld.%9.9ld\n",
				       pass, (long int)peer->end.tv_sec, 
				       peer->end.tv_nsec);
				printf(" -- read_cnt:%d byte_cnt:%d\n",
				       peer->msg_cnt, peer->byte_cnt);
			}
		}
	}
}

static int
store_results(struct incast_control *ctrl, struct incast_lead_hdr *hdr)
{
	FILE *out;
	struct incast_peer_outrec orec;
	int peer_no=0;
	out = fopen(ctrl->file_to_store_results, "a+");
	struct incast_peer *peer;
	if (out == NULL) {
		printf("Can't open file %s to store results err:%d\n", 
		       ctrl->file_to_store_results,
		       errno);
		return (-1);
	}
	if (fwrite(hdr, sizeof(struct incast_lead_hdr), 1, out) != 1) {
		printf("write fails err:%d\n", errno);
		fclose(out);
		return (-1);
	}
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		orec.start = peer->start;
		orec.end = peer->end;
		orec.state = peer->state;
		orec.peerno = peer_no;
		peer_no++;
		if (fwrite(&orec, sizeof(struct incast_peer_outrec), 1, out) != 1) {
			printf("write fails err:%d\n", errno);
			fclose(out);
			return (-1);
		}
	}
	fclose(out);
	return (0);
}

void 
incast_run_clients(struct incast_control *ctrl)
{
	int kq;
	struct incast_lead_hdr hdr;
	kq = kqueue();
	if (kq == -1) {
		printf("Fatal error - can't open kqueue err:%d\n", errno);
		return;
	}
	memset(&hdr, 0, sizeof(hdr));

	hdr.passcnt = 0;
	hdr.number_servers = ctrl->number_server;

	while (ctrl->cnt_of_times > 0) {
		ctrl->cnt_of_times -= ctrl->decrement_amm;
		hdr.passcnt++;
		/* Build the connections */
		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.start)) {
			printf("Can't get clock, errno:%d\n", errno);
			break;
		}
		if(build_conn_into_kq(kq, ctrl)) {
			printf("Can't build all the connections\n");
			clean_up_conn(ctrl);
			break;
		}
		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.connected)) {
			printf("Can't get clock, errno:%d\n", errno);
			clean_up_conn(ctrl);
			break;
		}

		/* Send out the request */
		if(send_req_to_servers(ctrl)) {
			printf("Can't send all results - exit cleanup\n");
			clean_up_conn(ctrl);
			break;
		}

		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.sending)) {
			printf("Can't get clock, errno:%d\n", errno);
			clean_up_conn(ctrl);
			break;
		}

		/* Now gather responses of data from all */
		if(gather_kq_results(kq, ctrl) ){
			printf("Gather results fails!!\n");
			clean_up_conn(ctrl);
			break;
		}

		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.end)) {
			printf("Can't get clock, errno:%d\n", errno);
			clean_up_conn(ctrl);
			break;
		}


		/* Display results */
		if (ctrl->file_to_store_results == NULL) {
			display_results(ctrl, hdr.passcnt, 0);
		} else {
			if(store_results(ctrl, &hdr)) {
				clean_up_conn(ctrl);
				break;
			}
		}
		/* Now assure everyone is back to the new state */
		clean_up_conn(ctrl);
		
		/* Now did the user specify a quiet time? */
		if (ctrl->nap_time) {
			struct timespec nap;
			nap.tv_sec = 0;
			nap.tv_nsec = ctrl->nap_time;
			nanosleep(&nap, NULL);
		}
	}
	close(kq);
	return;
}

static int
distribute_to_each_peer(struct incast_control *ctrl)
{
	int proto, sockopt;
	struct incast_peer *peer;
	int i, *p, ret, optval;
	socklen_t optlen;
	char buffer[MAX_SINGLE_MSG];
	
	/* prepare the buffer */
	p = (int *)buffer;
	for(i=0; i<(MAX_SINGLE_MSG/4); i++) {
		*p = (i);
		p++;
	}
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		/* Get the right socket */
		if (ctrl->sctp_on) {
			proto = IPPROTO_SCTP;
			sockopt = SCTP_NODELAY;
		} else {
			proto = IPPROTO_TCP;
			sockopt = TCP_NODELAY;
		}
		peer->sd = socket(AF_INET, SOCK_STREAM, proto);
		if (peer->sd == -1) {
			printf("Can't open a socket for a peer err:%d\n",
			       errno);
			
			return (-1);
		}
		if ((proto == IPPROTO_SCTP) && (no_cc_change == 0)) {
			struct sctp_assoc_value av;
			socklen_t optlen;
			av.assoc_id = 0;
			av.assoc_value = SCTP_CC_RTCC;
			optlen = sizeof(av);
			if (setsockopt(peer->sd, proto,  SCTP_PLUGGABLE_CC,
			       &av, optlen) == -1) {
				printf("Can't turn on RTCC cc err:%d\n",
				       errno);
				exit(-1);
			}
		}
		/* Set no delay */
		optval = 1;
		optlen = sizeof(optval);
		if (setsockopt(peer->sd, proto, sockopt, 
			       &optval, optlen) == -1) {
			printf("Can't set no-delay err:%d\n", errno);
			return (-1);
		}
		/* bind */
		optlen = sizeof(struct sockaddr_in);
		if (bind(peer->sd, (struct sockaddr *)&ctrl->bind_addr, 
			 optlen)) {
			printf("Can't bind err:%d\n", errno);
			return (-1);
		}
		
		/* connect */
		if(connect(peer->sd, (struct sockaddr *)&peer->addr,
			   optlen)) {
			printf("Connect err:%d for address", errno);
			print_an_address((struct sockaddr *)&peer->addr, 1);
			return (-1);
		}
		/* grab the time */
		if(clock_gettime(CLOCK_MONOTONIC_PRECISE, 
				 &peer->start)) {
			printf("Warning start time not gotten\n");
		}

		/* send it all */
		for(i=0; i<ctrl->cnt_req; i++) {
			if((ret = send(peer->sd, buffer, ctrl->size, 0)) < 1) {
				printf("Send failed err:%d ret:%d peer ", errno, ret);
				print_an_address((struct sockaddr *)&peer->addr, 1);
				break;
			}
		}
		/* close */
		peer->state = SRV_STATE_COMPLETE;
		close(peer->sd);
		peer->sd = -1;

		/* grab the time */
		if(clock_gettime(CLOCK_MONOTONIC_PRECISE, 
				 &peer->end)) {
			printf("Warning start time not gotten\n");
		}
	}
	return (0);
}

static int
store_elephant_peer(struct incast_control *ctrl, struct elephant_lead_hdr *hdr)
{
	FILE *out;
	struct incast_peer_outrec orec;
	int peer_no=0;
	out = fopen(ctrl->file_to_store_results, "a+");
	struct incast_peer *peer;
	if (out == NULL) {
		printf("Can't open file %s to store results err:%d\n", 
		       ctrl->file_to_store_results,
		       errno);
		return (-1);
	}
	if (fwrite(hdr, sizeof(struct elephant_lead_hdr), 1, out) != 1) {
		printf("write fails err:%d\n", errno);
		fclose(out);
		return (-1);
	}
	LIST_FOREACH(peer, &ctrl->master_list, next) {
		orec.start = peer->start;
		orec.end = peer->end;
		orec.state = peer->state;
		orec.peerno = peer_no;
		peer_no++;
		if (fwrite(&orec, sizeof(struct incast_peer_outrec), 1, out) != 1) {
			printf("write fails err:%d\n", errno);
			fclose(out);
			return (-1);
		}
	}
	fclose(out);
	return (0);
}

void 
elephant_run_clients(struct incast_control *ctrl, int first_size)
{
	int pass=0;
	struct timespec ts;
	long randval;
	struct elephant_lead_hdr hdr;

	memset(&hdr, 0, sizeof(hdr));
	hdr.number_servers = ctrl->number_server;	

	/* Seed the random number generator */
	clock_gettime(CLOCK_MONOTONIC_PRECISE, &ts);
	srandom((unsigned long)ts.tv_nsec);
	
	while (ctrl->cnt_of_times > 0) {
		ctrl->cnt_of_times -= ctrl->decrement_amm;
		pass++;
		/* Select some number of bytes to send 
		 * we need a number between 1Meg - 100Meg
		 */
		if (first_size) {
			ctrl->byte_cnt_req = first_size;
			first_size = 0;
			goto size_set;
		}
	choose_again:
		randval = (random() & 0x1fffffff);
		if (randval < 100000000) {
			goto choose_again;
		} else {
			ctrl->byte_cnt_req = randval;
		}
	size_set:
		ctrl->cnt_req = (ctrl->byte_cnt_req/ctrl->size) + 1;

		hdr.number_of_bytes = ctrl->byte_cnt_req;

		/* Now for each peer we must connect, send and close */
		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.start)) {
			printf("Can't get clock, errno:%d\n", errno);
			break;
		}
		distribute_to_each_peer(ctrl);
		/* Display results */
		if (clock_gettime(CLOCK_REALTIME_PRECISE, &hdr.end)) {
			printf("Can't get clock, errno:%d\n", errno);
			clean_up_conn(ctrl);
			break;
		}
		if (ctrl->file_to_store_results == NULL) {
			display_results(ctrl, pass, 1);
		} else {
			if (store_elephant_peer(ctrl, &hdr)) {
				printf("Can't store results\n");
				display_results(ctrl, pass, 1);
			}
		}
		/* Clean up everything */
		clean_up_conn(ctrl);

		/* Now did the user specify a quiet time? */
		if (ctrl->nap_time) {
			struct timespec nap;
			nap.tv_sec = 0;
			nap.tv_nsec = ctrl->nap_time;
			nanosleep(&nap, NULL);
		}
	}
	return;
}


static void
incast_add_peer(struct incast_control *ctrl, char *body, int linecnt,
	uint16_t def_port)
{
	struct incast_peer *peer;
	char *ipaddr, *portstr=NULL, *tok, *hostname, *max;
	int long_len;
	int len;

	if (body == NULL) {
		printf("adding peer - error line:%d no body\n",
		       linecnt);
		return;
	}
	len = strlen(body);
	max = body + len + 1;

	 
	ipaddr = strtok(body, ":") ;
	if (ipaddr == NULL) {
		printf("Parse error for peer line %d - no port?\n", linecnt);
		return;
	}
	portstr = strtok(NULL, ":");
	if (portstr == NULL) {
		printf("Parse error no port string %d \n", linecnt);
		return;
	}
	hostname = strtok(NULL, ":");
	if (hostname == NULL) {
		printf("Parse error for peer line %d - no hostname string\n", 
		       linecnt);
		return;
	}
	len = strlen(hostname);
	tok = &hostname[len+1];
	if (tok >= max) {
		printf("Parse error for peer line %d - no long length\n",
			linecnt);
		return;
	}
	long_len = strtol(tok, NULL, 0);
	if ((long_len != 4) && (long_len != 8)) {
		printf("Long length not set to 4 or 8 - error line %d\n", 
		       linecnt);
		return;
	}
	peer = malloc(sizeof(struct incast_peer));
	memset(peer, 0, sizeof(struct incast_peer));
	
	peer->long_size = long_len;
	if (translate_ip_address(ipaddr, &peer->addr)) {
		printf("line:%d unrecognizable peer address '%s'\n",
		       linecnt, ipaddr);
		free(peer);
		return;
	}
	if(portstr == NULL) {
		peer->addr.sin_port = htons(DEFAULT_SVR_PORT);
	} else {
		int x;
		x = strtol(portstr, NULL, 0);
		if ((x < 1) || (x > 65535)) {
			if (x) {
				printf("Invalid port number %d - using default\n",
				       x);
			}
			peer->addr.sin_port = htons(def_port); 
		} else {
			peer->addr.sin_port = htons(x);
		}
	}
	peer->state = SRV_STATE_NEW;
	peer->sd = -1;
	peer->peer_name = malloc(strlen(hostname)+1);
	strcpy(peer->peer_name, hostname);

	LIST_INSERT_HEAD(&ctrl->master_list, peer, next);
	ctrl->number_server++;
}

static void
incast_set_bind(struct incast_control *ctrl, char *body, int linecnt)
{
	char *ipaddr, *port, *host, *max, *tok;
	int long_size, len;
	if (body == NULL) {
		printf("setting bind - error line:%d no body\n",
		       linecnt);
		return;
	}
	len = strlen(body);
	max = body + len + 1;
	ipaddr = strtok(body, ":");
	if (ipaddr == NULL) {
		printf("Parse error bind - no ip addr field - line:%d\n",
		       linecnt);
		return;
	}
	port = strtok(NULL, ":");
	if (port == NULL) {
		printf("Parse error bind - no port field - line:%d\n",
		       linecnt);
		return;
	}
	host = strtok(NULL, ":");
	if (host == NULL) {
		printf("Parse error bind - no hostname field - line:%d\n",
		       linecnt);
		return;
	}
	len = strlen(host);
	tok = host + len + 1;
	if (tok > max) {
		printf("No long size found on end line:%d bind\n", linecnt);
		return;
	}
	long_size = strtol(tok, NULL, 0);
	if ((long_size != 4) && (long_size != 8) && (long_size != 0)) {
		printf("Invalid long size in bind line %d!\n", linecnt);
		return;
	}
	if (long_size) {
		ctrl->long_size = long_size;
	} else {
		ctrl->long_size = sizeof(long);
	}
	len = strlen(host);
	ctrl->hostname = malloc(len+1);
	strcpy(ctrl->hostname, host);
	if (translate_ip_address(ipaddr, &ctrl->bind_addr)) {
		printf("line:%d unrecognizable bind address '%s'\n",
		       linecnt, body);
		return;
	}
	ctrl->bind_addr.sin_port = htons((uint16_t)strtol(port, NULL, 0));
}

void
parse_config_file(struct incast_control *ctrl, char *configfile, 
		  uint16_t def_port)
{
	char buffer[1025];
	FILE *io;
	char *token, *body;
	int linecnt, len, olen;
	/* 
	 * Here we parse the configuration
	 * file. An entry in the file has
	 * the form:
	 * keyword:information
	 *
	 * Allowable keywords and from are:
	 * sctp:  - switch sctp on tcp off
	 * tcp:   - switch tcp on sctp off - defaults to TCP on.
	 * peer:ip.dot.addr.or.name:port   - create a peer entry for this guy
	 * bind:ip.dot.addr.or.name        - specify the bind to address
	 * times:number - Number of passes, if 0 never stop. (default 1)
	 * sends:number - Number of bytes per send (1-9000 - default 1448)
	 * sendc:number - Total number of sends (1-N) - default 3.
	 *
	 * When the format is an address leaving off the port or setting
	 * it to 0 gains the default address.
	 */

	memset(ctrl, 0, sizeof(struct incast_control));
	LIST_INIT(&ctrl->master_list);
	ctrl->decrement_amm = 1;
	ctrl->cnt_of_times = 1;
	ctrl->size = DEFAULT_MSG_SIZE;
	ctrl->cnt_req = DEFAULT_NUMBER_SENDS;


	io = fopen(configfile, "r");
	if (io == NULL) {
		printf("Can't open config file '%s':%d\n", configfile, errno);
		return;
	}
	linecnt = 1;
	memset(buffer, 0, sizeof(buffer));
	while (fgets(buffer, (sizeof(buffer)-1), io) != NULL) {
		/* Get rid of cr */
		olen = strlen(buffer);
		if (olen == 1) {
			linecnt++;
			continue;
		}
		if (buffer[olen-1] == '\n') {
			buffer[olen-1] = 0;
			olen--;
		}
		if (buffer[0] == '#') {
			/* commented out */
			linecnt++;
			continue;
		}
		/* tokenize the token and body */
		token = strtok(buffer, ":");
		if (token == NULL) {
			linecnt++;
			continue;
		}
		len = strlen(token);
		if ((len+1) == olen) {
			/* No body */
			body = NULL;
		} else {
			/* past the null */
			body = &token[len+1];
		}
		if (strcmp(token, "sctp") == 0) {
			ctrl->sctp_on = 1;
		} else if (strcmp(token, "tcp") == 0) {
			ctrl->sctp_on = 0;
		} else if (strcmp(token, "peer") == 0) {
			incast_add_peer(ctrl, body, linecnt, def_port);
		} else if (strcmp(token, "bind") == 0) {
			incast_set_bind(ctrl, body, linecnt);
		} else if (strcmp(token, "times") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d times with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if (cnt) {
				ctrl->cnt_of_times = cnt;
			} else {
				ctrl->cnt_of_times = 1;
				ctrl->decrement_amm = 0;
			}
		} else if (strcmp(token, "sends") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d sends with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if ((cnt) && ((cnt > 0) && (cnt < MAX_SINGLE_MSG))) {
				ctrl->size = cnt;
			} else {
				printf("Warning line:%d sizes invalid ( 0 > %d < %d)\n",
				       linecnt, cnt, MAX_SINGLE_MSG);
				printf("Using default %d\n", DEFAULT_MSG_SIZE);
			}

		} else if (strcmp(token, "sendc") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d sendc with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if ((cnt) && (cnt > 0)) {
				ctrl->cnt_req = cnt;
			} else {
				printf("Warning line:%d sizes invalid ( 0 > %d)\n",
				       linecnt, cnt);
				printf("Using default %d\n", DEFAULT_NUMBER_SENDS);
				ctrl->cnt_req = DEFAULT_NUMBER_SENDS;
			}
		} else {
			printf("Unknown token '%s' at line %d\n",
			       token, linecnt);
		}
		linecnt++;
	}
	fclose(io);
}
