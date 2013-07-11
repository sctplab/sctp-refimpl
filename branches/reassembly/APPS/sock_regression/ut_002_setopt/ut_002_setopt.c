/*-
 * Copyright (c) 2001-2007, by Weongyo Jeong. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the distribution.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <netinet/sctp.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static void
test_SCTP_NODELAY(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_NODELAY, &on, sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_AUTOCLOSE(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &on, sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &on, sizeof(on));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	close(sd);
}

static void
test_SCTP_AUTO_ASCONF(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTO_ASCONF, &on, sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_EXPLICIT_EOR(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_EXPLICIT_EOR, &on, sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_DISABLE_FRAGMENTS(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS, &on,
	    sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_USE_EXT_RCVINFO(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_USE_EXT_RCVINFO, &on,
	    sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_I_WANT_MAPPED_V4_ADDR(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR, &on,
	    sizeof(on));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	close(sd);

	sd = socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR, &on,
	    sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_PARTIAL_DELIVERY_POINT(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_PARTIAL_DELIVERY_POINT, &on,
	    sizeof(on));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_FRAGMENT_INTERLEAVE(void)
{
	int ret, sd;
	uint32_t level;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	level = SCTP_FRAG_LEVEL_2;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE, &level,
	    sizeof(level));
	if (ret == -1)
		errx(-1, "setsockopt error");

	level = 0x199999;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE, &level,
	    sizeof(level));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	close(sd);
}

static void
test_SCTP_CMT_ON_OFF(void)
{
	int ret, sd;
	uint32_t value;
	size_t len = sizeof(uint32_t);
	struct sctp_assoc_value av;

	if (sysctlbyname("net.inet.sctp.cmt_on_off", &value, &len, 0, 0) < 0)
		errx(-1, "sysctlbyname error");

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	av.assoc_id = 0;
	av.assoc_value = 1;
	if (value) {
		ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CMT_ON_OFF, &av,
		    sizeof(av));
		if (!(ret == -1 && errno == ENOTCONN))
			errx(-1, "sctp_connectx: ENOPROTOOPT expected");
	} else {
		ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CMT_ON_OFF, &av,
		    sizeof(av));
		if (!(ret == -1 && errno == ENOPROTOOPT))
			errx(-1, "sctp_connectx: ENOPROTOOPT expected");
	}

	close(sd);
}

static void
test_SCTP_CLR_STAT_LOG(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CLR_STAT_LOG, &on, sizeof(on));
	if (!(ret == -1 && errno == EOPNOTSUPP))
		errx(-1, "sctp_connectx: EOPNOTSUPP expected");

	close(sd);
}

static void
test_SCTP_CONTEXT(void)
{
	int ret, sd;
	struct sctp_assoc_value av;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	av.assoc_id = 0;
	av.assoc_value = 1;

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CONTEXT, &av, sizeof(av));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_VRF_ID(void)
{
	int ret, sd;
	uint32_t vrfid;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	vrfid = 1;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_VRF_ID, &vrfid, sizeof(vrfid));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	vrfid = -1;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_VRF_ID, &vrfid, sizeof(vrfid));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "sctp_connectx: EINVAL expected");

	vrfid = 0;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_VRF_ID, &vrfid, sizeof(vrfid));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_DEL_VRF_ID(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DEL_VRF_ID, &on, sizeof(on));
	if (!(ret == -1 && errno == EOPNOTSUPP))
		errx(-1, "sctp_connectx: EOPNOTSUPP expected");

	close(sd);
}

static void
test_SCTP_ADD_VRF_ID(void)
{
	int ret, sd, on = 1;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_ADD_VRF_ID, &on, sizeof(on));
	if (!(ret == -1 && errno == EOPNOTSUPP))
		errx(-1, "sctp_connectx: EOPNOTSUPP expected");

	close(sd);
}

static void
test_SCTP_DELAYED_SACK(void)
{
	int ret, sd;
	struct sctp_sack_info sack;

	sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	sack.sack_assoc_id = -1;
	sack.sack_delay = 10;
	sack.sack_freq = 10;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DELAYED_SACK, &sack,
	    sizeof(sack));
	if (!(ret == -1 && errno == ENOENT))
		errx(-1, "sctp_connectx: ENOENT expected");

	sack.sack_assoc_id = 0;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DELAYED_SACK, &sack,
	    sizeof(sack));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	sack.sack_assoc_id = -1;
	sack.sack_delay = 10;
	sack.sack_freq = 10;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DELAYED_SACK, &sack,
	    sizeof(sack));
	if (ret == -1)
		errx(-1, "setsockopt error");

	sack.sack_assoc_id = 0;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_DELAYED_SACK, &sack,
	    sizeof(sack));
	if (ret == -1)
		errx(-1, "setsockopt error");

	close(sd);
}

static void
test_SCTP_AUTH_CHUNK(void)
{
	int i, ret, sd;
	struct sctp_authchunk sauth;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	for (i = 0; i < 256; i++) {
		sauth.sauth_chunk = i;
		ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTH_CHUNK, &sauth,
		    sizeof(sauth));
		switch(i) {
		case SCTP_INITIATION:
		case SCTP_INITIATION_ACK:
		case SCTP_SHUTDOWN_COMPLETE:
		case SCTP_AUTHENTICATION:
			if (!(ret == -1 && errno == EINVAL))
				errx(-1, "setsockopt: EINVAL expected");
			break;
		default:
			if (ret == -1)
		errx(-1, "setsockopt: error");
		}
	}

	close(sd);
}

static void
test_SCTP_AUTH_KEY(void)
{
	/* do nothing.  */
}

static void
test_SCTP_HMAC_IDENT(void)
{
	/* do nothing.  */
}

static void
test_SCTP_AUTH_ACTIVE_KEY(void)
{
	int ret, sd;
	struct sctp_authkeyid scact;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	scact.scact_assoc_id = -1;
	scact.scact_keynumber = 999;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTH_ACTIVE_KEY, &scact,
	    sizeof(scact));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	close(sd);
}

static void
test_SCTP_AUTH_DELETE_KEY(void)
{
	int ret, sd;
	struct sctp_authkeyid scdel;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	scdel.scact_assoc_id = -1;
	scdel.scact_keynumber = 999;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_AUTH_DELETE_KEY, &scdel,
	    sizeof(scdel));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	close(sd);
}

static void
test_SCTP_RESET_STREAMS(void)
{
	int ret, sd;
	struct sctp_stream_reset strrst;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	strrst.strrst_assoc_id = -1;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_RESET_STREAMS, &strrst,
	    sizeof(strrst));
	if (!(ret == -1 && errno == ENOENT))
		errx(-1, "setsockopt: ENOENT expected");

	close(sd);
}

static void
test_SCTP_CONNECT_X(void)
{
	int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CONNECT_X, &ret, sizeof(ret));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	close(sd);
}

static void
test_SCTP_CONNECT_X_DELAYED(void)
{
	int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CONNECT_X_DELAYED, &ret,
	    sizeof(ret));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	close(sd);
}

static void
test_SCTP_CONNECT_X_COMPLETE(void)
{
	int sd, ret;
	struct sockaddr sa;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_CONNECT_X_COMPLETE, &sa,
	    sizeof(sa));
	if (!(ret == -1 && errno == ENOENT))
		errx(-1, "setsockopt: ENOENT expected");

	close(sd);
}

static void
test_SCTP_MAX_BURST(void)
{
	uint8_t burst = 1;
	int sd, ret;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_MAX_BURST, &burst,
	    sizeof(burst));
	if (ret == -1)
		errx(-1, "setsockopt error");
}

static void
test_SCTP_MAXSEG(void)
{
	/* do nothing.  */
}

static void
test_SCTP_EVENTS(void)
{
	/* do nothing.  */
}

static void
test_SCTP_ADAPTATION_LAYER(void)
{
	/* do nothing.  */
}

#ifdef SCTP_DEBUG
static void
test_SCTP_SET_INITIAL_DBG_SEQ(void)
{
	/* do nothing.  */
}
#endif

static void
test_SCTP_DEFAULT_SEND_PARAM(void)
{
	/* do nothing.  */
}

static void
test_SCTP_PEER_ADDR_PARAMS(void)
{
	/* do nothing.  */
}

static void
test_SCTP_RTOINFO(void)
{
	/* do nothing.  */
}

static void
test_SCTP_ASSOCINFO(void)
{
	/* do nothing.  */
}

static void
test_SCTP_INITMSG(void)
{
	/* do nothing.  */
}

static void
test_SCTP_PRIMARY_ADDR(void)
{
	/* do nothing.  */
}

static void
test_SCTP_SET_DYNAMIC_PRIMARY(void)
{
	int sd, ret;
	uid_t euid = geteuid();
	union sctp_sockstore ss;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	bzero(&ss, sizeof(ss));
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_SET_DYNAMIC_PRIMARY, &ss,
	    sizeof(ss));

	if (euid != 0) {
		if (!(ret == -1 && errno == EPERM))
			errx(-1, "setsockopt: EPERM expected");
	} else {
		if (!(ret == -1 && errno == EADDRNOTAVAIL))
			errx(-1, "setsockopt: EADDRNOTAVAIL expected");
	}
			
	close(sd);
}

static void
test_SCTP_SET_PEER_PRIMARY_ADDR(void)
{
	int sd, ret;
	struct sctp_setpeerprim sspp;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_SET_PEER_PRIMARY_ADDR, &sspp,
	    sizeof(sspp));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	close(sd);
}

static void
test_SCTP_BINDX_ADD_ADDR(void)
{
	int sd, ret;
	struct sctp_getaddresses addrs;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));
	
	bzero(&addrs, sizeof(addrs));
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_BINDX_ADD_ADDR, &addrs,
	    sizeof(addrs));
	if (!(ret == -1 && errno == EAFNOSUPPORT))
		errx(-1, "setsockopt: EAFNOSUPPORT expected");
	
	addrs.sget_assoc_id = 1;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_BINDX_REM_ADDR, &addrs,
	    sizeof(addrs));
	if (ret == -1)
		errx(-1, "SCTP_BINDX_ADD_ADDR is implemented.");

	close(sd);
}

static void
test_SCTP_BINDX_REM_ADDR(void)
{
	int sd, ret;
	struct sctp_getaddresses addrs;

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sd == -1)
		errx(-1, "socket: %s", strerror(errno));

	bzero(&addrs, sizeof(addrs));
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_BINDX_REM_ADDR, &addrs,
	    sizeof(addrs));
	if (!(ret == -1 && errno == EINVAL))
		errx(-1, "setsockopt: EINVAL expected");

	addrs.sget_assoc_id = 1;
	ret = setsockopt(sd, IPPROTO_SCTP, SCTP_BINDX_REM_ADDR, &addrs,
	    sizeof(addrs));
	if (ret == -1)
		errx(-1, "SCTP_BINDX_REM_ADDR is implemented.");

	close(sd);
}

int
main(int argc, char *argv[])
{
	test_SCTP_NODELAY();
	test_SCTP_AUTOCLOSE();
	test_SCTP_AUTO_ASCONF();
	test_SCTP_EXPLICIT_EOR();
	test_SCTP_DISABLE_FRAGMENTS();
	test_SCTP_USE_EXT_RCVINFO();
	test_SCTP_I_WANT_MAPPED_V4_ADDR();
	test_SCTP_PARTIAL_DELIVERY_POINT();
	test_SCTP_FRAGMENT_INTERLEAVE();
	test_SCTP_CMT_ON_OFF();
	test_SCTP_CLR_STAT_LOG();
	test_SCTP_CONTEXT();
	test_SCTP_VRF_ID();
	test_SCTP_DEL_VRF_ID();
	test_SCTP_ADD_VRF_ID();
	test_SCTP_DELAYED_SACK();
	test_SCTP_AUTH_CHUNK();
	test_SCTP_AUTH_KEY();
	test_SCTP_HMAC_IDENT();
	test_SCTP_AUTH_ACTIVE_KEY();
	test_SCTP_AUTH_DELETE_KEY();
	test_SCTP_RESET_STREAMS();
	test_SCTP_CONNECT_X();
	test_SCTP_CONNECT_X_DELAYED();
	test_SCTP_CONNECT_X_COMPLETE();
	test_SCTP_MAX_BURST();
	test_SCTP_MAXSEG();
	test_SCTP_EVENTS();
	test_SCTP_ADAPTATION_LAYER();
#ifdef SCTP_DEBUG
	test_SCTP_SET_INITIAL_DBG_SEQ();
#endif
	test_SCTP_DEFAULT_SEND_PARAM();
	test_SCTP_PEER_ADDR_PARAMS();
	test_SCTP_RTOINFO();
	test_SCTP_ASSOCINFO();
	test_SCTP_INITMSG();
	test_SCTP_PRIMARY_ADDR();
	test_SCTP_SET_DYNAMIC_PRIMARY();
	test_SCTP_SET_PEER_PRIMARY_ADDR();
	test_SCTP_BINDX_ADD_ADDR();
	test_SCTP_BINDX_REM_ADDR();

	printf("0");
	return EX_OK;
}
