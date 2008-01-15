#ifndef __Rserpool_io_h__
#define __Rserpool_io_h__
#include <sys/types.h>
#include <netinet/sctp.h>
#include <netinet/in.h>

/*
 * RSERPOOL parameters on the wire.
 */

#define RSP_SKIP_PARAM     0x8000
#define RSP_REPORT_PARAM   0x4000

struct rsp_paramhdr {
	uint16_t param_type;		/* parameter type */
	uint16_t param_length;		/* parameter length */
};


#define RSP_PARAM_RSVD                  0x0000
#define RSP_PARAM_IPV4_ADDR		        0x0001
#define RSP_PARAM_IPV6_ADDR             0x0002
#define RSP_PARAM_DCCP_TRANSPORT        0x0003
#define RSP_PARAM_SCTP_TRANSPORT        0x0004
#define RSP_PARAM_TCP_TRANSPORT         0x0005
#define RSP_PARAM_UDP_TRANSPORT         0x0006
#define RSP_PARAM_UDPLITE_TRANSPORT     0x0007
#define RSP_PARAM_SELECT_POLICY         0x0008
#define RSP_PARAM_POOL_HANDLE           0x0009
#define RSP_PARAM_POOL_ELEMENT          0x000a
#define RSP_PARAM_SERVER_INFO           0x000b
#define RSP_PARAM_OPERATION_ERROR       0x000c
#define RSP_PARAM_COOKIE                0x000d
#define RSP_PARAM_PE_IDENTIFIER         0x000e
#define RSP_PARAM_PE_CHECKSUM           0x000f

/* align to 32-bit sizes */
#define RSP_SIZE32(x)	((((x)+3) >> 2) << 2)

struct rsp_ipv4_address {
	struct rsp_paramhdr ph;
	struct in_addr      in;
};

struct rsp_ipv6_address {
	struct rsp_paramhdr ph;
	struct in6_addr     in6;
};

union rsp_address_union {
	struct rsp_paramhdr ph;
	struct rsp_ipv4_address ipv4;
	struct rsp_ipv6_address ipv6;
};

#define RSP_USE_DATA_ONLY    0x0000
#define RSP_USE_DATA_AND_CTL 0x0001

struct rsp_sctp_transport {
	struct rsp_paramhdr      ph;
	uint16_t	         port;  /* Is this in network order?? */
	uint16_t                 transport_use;
	union rsp_address_union  address[0];
};

struct rsp_tcp_transport {
	struct rsp_paramhdr      ph;
	uint16_t	         port;  /* Is this in network order?? */
	uint16_t                 transport_use;
	union rsp_address_union  address;
};

struct rsp_udp_transport {
	struct rsp_paramhdr      ph;
	uint16_t	         port;  /* Is this in network order?? */
	uint16_t                 reserved;
	union rsp_address_union  address;
};

#define RSP_POLICY_RESV                 0x00
#define RSP_POLICY_ROUND_ROBIN          0x01
#define RSP_POLICY_LEAST_USED           0x02
#define RSP_POLICY_LEAST_USED_DEGRADES  0x03
#define RSP_POLICY_WEIGHTED_ROUND_ROBIN 0x04

struct rsp_select_policy {
	struct rsp_paramhdr      ph;
	uint32_t                 policy_type;
	//uint8_t                  reserved[3];
};

/* Round robin uses the rsp_select_policy 
 * all others use rsp_select_policy_value, at
 * least of the initial draft delcared type. Future
 * policy's may need different values.
 */

struct rsp_select_policy_value {
	struct rsp_paramhdr      ph;
	uint32_t                 policy_type;
	/*
	uint8_t                  policy_type;
	uint8_t                  reserved[3];
	*/
	uint32_t                 policy_value;
};


struct rsp_pool_handle {
	struct rsp_paramhdr      ph;
	uint8_t                  pool_handle[0]; /* variable length */
};


struct rsp_pool_element {
	struct rsp_paramhdr      ph;
	uint32_t                 id;
	uint32_t                 home_enrp_id;
	int32_t                  registration_life;
	struct rsp_paramhdr      user_transport; /* variable from here
						  * on out. 
                                                  */
	/* After the life is:
	 * struct rsp_xxx_transport x;
	 * struct rsp_select_policy_xxx y;
	 * struct rsp_sctp_transport asap;
	 */
};

#define RSP_MULTICAST_FLAG      0x80000000

struct rsp_server_info {
	struct rsp_paramhdr        ph;
	uint32_t                   server_id;
	uint32_t		   flags;
	struct rsp_sctp_transport  transport;   
};

#define RSP_UNSPECIFIED_ERROR       0x0000
#define RSP_UNRECOGNIZED_PARAM      0x0001
#define RSP_UNRECOGNIZED_MESG       0x0002
#define RSP_INVALID_VALUES          0x0003
#define RSP_NONUNIQUE_PE_ID         0x0004
#define RSP_INCONSISTENT_POLICY     0x0005
#define RSP_LACK_OF_RESOURCE        0x0006
#define RSP_INCONSISTENT_TRANSPORT  0x0007
#define RSP_INCONSISTENT_DATA_CTRL  0x0008
#define RSP_UNKNOWN_POOL_HANDLE     0x0009

struct rsp_error_cause {
	u_int16_t code;
	u_int16_t length;
	/* optional cause-specific info may follow */
	void *ocause;
};

struct rsp_operational_error {
	struct rsp_paramhdr        ph;
	struct rsp_error_cause     cause[1];
};

struct rsp_cookie {
	struct rsp_paramhdr        ph;
	uint8_t                    cookie[4]; /* variable lenght */
};

struct rsp_pe_identifier {
	struct rsp_paramhdr       ph;
	uint32_t                  id;
};

struct rsp_checksum {
	struct rsp_paramhdr       ph;
 	uint16_t                  sum;
	uint16_t                  pad;
};

/* Base message structure */
struct asap_message {
	uint8_t		asap_type;
	uint8_t		asap_flags;
	uint16_t	asap_length;
};


/* ASAP Messages */
#define ASAP_REGISTRATION   		0x01
#define ASAP_DEREGISTRATION 		0x02
#define ASAP_REGISTRATION_RESPONSE	0x03
#define ASAP_DEREGISTRATION_RESPONSE	0x04
#define ASAP_HANDLE_RESOLUTION		0x05
#define ASAP_HANDLE_RESOLUTION_RESPONSE	0x06
#define ASAP_ENDPOINT_KEEP_ALIVE     	0x07
#define ASAP_ENDPOINT_KEEP_ALIVE_ACK	0x08
#define ASAP_ENDPOINT_UNREACHABLE	0x09
#define ASAP_SERVER_ANNOUNCE		0x0a
#define ASAP_COOKIE			0x0b
#define ASAP_COOKIE_ECHO		0x0c
#define ASAP_BUSINESS_CARD		0x0d
#define ASAP_ERROR			0x0e


#define RSERPOOL_ASAP_PPID 11
#define RSERPOOL_ENRP_PPID 12

/* largest message we read must fit in here */
#define RSERPOOL_STACK_BUF_SPACE   65535
#endif

