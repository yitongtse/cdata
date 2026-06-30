///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp.h
///  @brief     Header file for ERRP (ERMI-1) related definitions
///  @author    Yi Tong Tse


#ifndef __ERRP_H__
#define __ERRP_H__

#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include "common.h"
#include "util.h"
#include "os_util_lnx.h"
#include "os_util.h"


#define ERRP_LOG(fmt, str...)        \
    NULL
//    printf("ERRP: "fmt"\n", ##str);


// Defines accroding to ERRP spec
//
#define ERRP_MSG_HDR_SIZE         3
#define ERRP_PARAM_HDR_SIZE       4
#define ERRP_CAP_INFO_HDR_SIZE    4
#define ERRP_ATTR_HDR_SIZE        4

#define ERRP_TCP_PORT_DFT         6069

#define ERRP_MAX_MSG_BYTES        4096
#define ERRP_HEADER_BYTES         3     

#define ERRP_RESERVED             0

#define ERRP_PROTOCOL_VERSION     3
#define ERRP_HOLD_TIME_MIN        3

#define ERRP_ADDR_DOMAIN_ANY      0

#define ERRP_ADDR_FAMILY          32769  /* RTSP URL */

#define ERRP_VERSION              1

#define ERRP_ATTR_FLAG_NOT_WELL_KNOWN 0x80

#define ERRP_SOC_WINDOW           32768

//#define ERRP_CONNECT_TIME_DFT     120
#define ERRP_CONNECT_TIME_DFT     10

//#define ERRP_KEEPALIVE_TIME_DFT   3
//#define ERRP_HOLD_TIME_DFT        9

// For debugging only
#define ERRP_KEEPALIVE_TIME_DFT   60
#define ERRP_HOLD_TIME_DFT        90


// Implementation specific defines below
//
#define ERRP_MAX_NAME_LEN                 64
#define ERRP_MAX_ERR_LEN                  128
#define ERRP_MAX_ROUTE_TYPES              2
#define ERRP_MAX_ROUTES_PER_ATTR          1
#define ERRP_MAX_EDGE_INPUTS_PER_ATTR     1
#define ERRP_MAX_UDP_MAPS_PER_ATTR        1
#define ERRP_MAX_ROUTES                   512
#define ERRP_MAX_MAPS_PER_ROUTE           32
#define ERRP_MAX_ALT_SERVERS              2
#define ERRP_MAX_FIBER_NODES_PER_ROUTE    2
#define ERRP_MAX_INPUTS_PER_ROUTE         2
#define ERRP_MAX_EDGE_INPUTS              2


// ERRP error codes
#define ERRP_OK                           0
#define ERRP_ERR_MSG_HEADER               1
#define ERRP_ERR_OPEN_MSG                 2
#define ERRP_ERR_UPDATE_MSG               3
#define ERRP_ERR_HOLD_TIMER_EXPIRED       4
#define ERRP_ERR_FSM                      5
#define ERRP_ERR_CEASE                    6

#define ERRP_ERR_SUB_UNSPECIFIED          0

// ERRP message header error subcode
#define ERRP_ERR_SUB_BAD_MSG_LEN          1
#define ERRP_ERR_SUB_BAD_MSG_TYPE         2

// ERRP OPEN message error subcode
#define ERRP_ERR_SUB_UNSUP_VERSION        1
#define ERRP_ERR_SUB_BAD_PEER_ADDR_DOMAIN 2
#define ERRP_ERR_SUB_BAD_ERRP_ID          3
#define ERRP_ERR_SUB_UNSUP_OPT_PARAM      4
#define ERRP_ERR_SUB_UNSUP_HOLD_TIME      5
#define ERRP_ERR_SUB_UNSUP_CAP            6
#define ERRP_ERR_SUB_CAP_MISMATCH         7

// ERRP UPDATE message error subcode
#define ERRP_ERR_SUB_MALFORMED_ATTR_LIST  1
#define ERRP_ERR_SUB_UNRECOG_ATTR         2
#define ERRP_ERR_SUB_MISSING_ATTR         3
#define ERRP_ERR_SUB_ATTR_FLAGS           4
#define ERRP_ERR_SUB_ATTR_LEN             5
#define ERRP_ERR_SUB_INVALID_ATTR         6


/// Check well-known attribute flag
static inline
int errp_is_attr_well_known (uint8 attr_flags)
{
    return !(attr_flags & ERRP_ATTR_FLAG_NOT_WELL_KNOWN);
}


/// ERRP Message Types
typedef enum {
    ERRP_OPEN = 1,
    ERRP_UPDATE = 2,
    ERRP_NOTIFICATION = 3,
    ERRP_KEEPALIVE = 4,
} errp_message_type_t;


/// ERRP Parameter Types in OPEN message
typedef enum {
    ERRP_PARAM_CAP_INFO = 1,
    ERRP_PARAM_STREAMING_ZONE = 2,
    ERRP_PARAM_COMPONENT_NAME = 3,
    ERRP_PARAM_VENDOR_STRING = 4,
} errp_param_t;


/// ERRP Capability Codes
typedef enum {
    ERRP_CAP_ROUTE_TYPES_SUPPORTED = 1,
    ERRP_CAP_SEND_RECEIVE = 2,
    ERRP_CAP_ERRP_VERSION = 32768,
} errp_cap_code_t;


/// ERRP Application Protocols
typedef enum {
    ERRP_PROT_NONE_STATIC = 32766,
    ERRP_PROT_RTSP_DYNAMIC = 32768,
    ERRP_PROT_RTSP_DYNAMIC_PROVISION = 32770,
} errp_app_protocol_t;


/// ERRP Send Receive Capability
typedef enum {
    ERRP_SEND_AND_RECEIVE = 1,
    ERRP_SEND_ONLY = 2,
    ERRP_RECEIVE_ONLY = 3,
} errp_send_receive_t;


/// ERRP Attribute Type Codes
typedef enum {
    ERRP_ATTR_WITHDRAWN_ROUTE = 1,
    ERRP_ATTR_REACHABLE_ROUTE = 2,
    ERRP_ATTR_NEXT_HOP_SERVER = 3,
    ERRP_ATTR_QAM_NAMES = 232,
    ERRP_ATTR_CAS_CAP = 233,
    ERRP_ATTR_TOTAL_BANDWIDTH = 234,
    ERRP_ATTR_AVAIL_BANDWIDTH = 235,
    ERRP_ATTR_COST = 236,
    ERRP_ATTR_EDGE_INPUT = 237,
    ERRP_ATTR_QAM_CFG = 238,
    ERRP_ATTR_UDP_MAP = 239,
    ERRP_ATTR_SERVICE_STATUS = 241,
    ERRP_ATTR_MAX_MPEG_FLOWS = 242,
    ERRP_ATTR_NEXT_HOP_ALT = 243,
    ERRP_ATTR_PORT_ID = 244,
    ERRP_ATTR_FIBER_NODE = 245,
    ERRP_ATTR_QAM_CAP = 247,
    ERRP_ATTR_INPUT_MAP = 249,
    ERRP_ATTR_UNKNOWN = -1
} errp_attr_t;


/// Attribute bit masks for UPDATE message
#define ERRP_ATTR_MASK_WITHDRAWN_ROUTE  0x00000001
#define ERRP_ATTR_MASK_REACHABLE_ROUTE  0x00000002
#define ERRP_ATTR_MASK_NEXT_HOP_SERVER  0x00000004
#define ERRP_ATTR_MASK_QAM_NAMES        0x00000008
#define ERRP_ATTR_MASK_CAS_CAP          0x00000010
#define ERRP_ATTR_MASK_TOTAL_BANDWIDTH  0x00000020
#define ERRP_ATTR_MASK_AVAIL_BANDWIDTH  0x00000040
#define ERRP_ATTR_MASK_COST             0x00000080
#define ERRP_ATTR_MASK_EDGE_INPUT       0x00000100
#define ERRP_ATTR_MASK_QAM_CFG          0x00000200
#define ERRP_ATTR_MASK_UDP_MAP          0x00000400
#define ERRP_ATTR_MASK_SERVICE_STATUS   0x00000800
#define ERRP_ATTR_MASK_MAX_MPEG_FLOWS   0x00001000
#define ERRP_ATTR_MASK_NEXT_HOP_ALT     0x00002000
#define ERRP_ATTR_MASK_PORT_ID          0x00004000
#define ERRP_ATTR_MASK_FIBER_NODE       0x00008000
#define ERRP_ATTR_MASK_QAM_CAP          0x00010000
#define ERRP_ATTR_MASK_INPUT_MAP        0x00020000

/// Bit mask of required attributes for a reachable route
#define ERRP_ATTR_MASK_REQUIRED_REACHABLE               \
              ( ERRP_ATTR_MASK_NEXT_HOP_SERVER |        \
                ERRP_ATTR_MASK_QAM_NAMES       |        \
                ERRP_ATTR_MASK_TOTAL_BANDWIDTH |        \
                ERRP_ATTR_MASK_QAM_CFG         |        \
                ERRP_ATTR_MASK_UDP_MAP         |        \
                ERRP_ATTR_MASK_PORT_ID         |        \
                ERRP_ATTR_MASK_FIBER_NODE      |        \
                ERRP_ATTR_MASK_QAM_CAP )

/// Bit mask of required attributes for a withdrawn route
#define ERRP_ATTR_MASK_REQUIRED_WITHDRAWN               \
              ( ERRP_ATTR_MASK_NEXT_HOP_SERVER |        \
                ERRP_ATTR_MASK_QAM_NAMES       |        \
                ERRP_ATTR_MASK_FIBER_NODE      |        \
                ERRP_ATTR_MASK_QAM_CAP )


// ERRP QAM configuration related definitios
//

#define ERRP_QAM_CFG_ERR        255

/// QAM Modulation Modes
enum {
  ERRP_QAM_CFG_MODULATION_64_QAM = 3,
  ERRP_QAM_CFG_MODULATION_256_QAM = 4,
  ERRP_QAM_CFG_MODULATION_128_QAM = 5,
  ERRP_QAM_CFG_MODULATION_512_QAM = 6,
  ERRP_QAM_CFG_MODULATION_1024_QAM = 7,
};

/// QAM Interleaver Settings
enum {
  ERRP_QAM_CFG_FEC_I8_J16 = 3,
  ERRP_QAM_CFG_FEC_I16_J8 = 4,
  ERRP_QAM_CFG_FEC_I32_J4 = 5,
  ERRP_QAM_CFG_FEC_I64_J2 = 6,
  ERRP_QAM_CFG_FEC_I128_J1 = 7,
  ERRP_QAM_CFG_FEC_I12_J7 = 8,
  ERRP_QAM_CFG_FEC_I128_J2 = 9,
  ERRP_QAM_CFG_FEC_I128_J3 = 10,
  ERRP_QAM_CFG_FEC_I128_J4 = 11,
  ERRP_QAM_CFG_FEC_I128_J5 = 12,
  ERRP_QAM_CFG_FEC_I128_J6 = 13,
  ERRP_QAM_CFG_FEC_I128_J7 = 14,
  ERRP_QAM_CFG_FEC_I128_J8 = 15,
};

/// QAM Annex Modes
enum {
  ERRP_QAM_CFG_ANNEX_A = 0,
  ERRP_QAM_CFG_ANNEX_B = 1,
  ERRP_QAM_CFG_ANNEX_C = 2,
};

/// QAM Channel Bandwidth Types
enum {
  ERRP_QAM_CFG_CHAN_BW_6MHZ = 0,
  ERRP_QAM_CFG_CHAN_BW_7MHZ = 2,
  ERRP_QAM_CFG_CHAN_BW_8MHZ = 1,
};


/// CAS Encryption Types
enum {
  ERRP_CAS_ENC_TYPE_NONE = 0,
  ERRP_CAS_ENC_TYPE_NON_SESSION = 1,
  ERRP_CAS_ENC_TYPE_SESSION = 2,
};

/// CAS Encryption Schemes
enum {
  ERRP_CAS_ENC_SCHEME_DES = 0,
  ERRP_CAS_ENC_SCHEME_3DES = 1,
  ERRP_CAS_ENC_SCHEME_AES = 2,
  ERRP_CAS_ENC_SCHEME_DVB_CSA = 3,
  ERRP_CAS_ENC_SCHEME_PKEY = 4,
  ERRP_CAS_ENC_SCHEME_MEDIAC = 5,
  ERRP_CAS_ENC_SCHEME_DVS042 = 6,
};


/// Service Status
enum {
  ERRP_SERVICE_STATUS_OPERATIONAL = 1,
  ERRP_SERVICE_STATUS_SHUTTING_DOWN = 2,
  ERRP_SERVICE_STATUS_STANDBY = 3,
};


// ERRP QAM Capability related definiitions
// Note: In ERMI spec, bit 0 is the most significant bit!
//

/// QAM Channel Bandwidth Capability Bits
typedef struct {
    uint16 reserved : 5;
    uint16 _8mhz : 1;
    uint16 _7mhz : 1;
    uint16 _6mhz : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_chan_bw_cap_t;


/// QAM J83 Capability Bits
typedef struct {
    uint16 reserved : 5;
    uint16 annex_C : 1;
    uint16 annex_B : 1;
    uint16 annex_A : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_j83_cap_t;


/// QAM Interleaver Capability Bits
typedef struct {
    uint16 reserved : 11;
    uint32 i12_j7 : 1;
    uint32 i128_j8 : 1;
    uint32 i128_j7 : 1;
    uint32 i128_j6 : 1;
    uint32 i128_j5 : 1;
    uint32 i128_j4 : 1;
    uint32 i128_j3 : 1;
    uint32 i128_j2 : 1;
    uint32 i128_j1 : 1;
    uint32 i64_j2 : 1;
    uint32 i32_j4 : 1;
    uint32 i16_j8 : 1;
    uint32 i8_j16 : 1;
    uint32 tsid_grp_id: 7;
    uint32 lock : 1;
} errp_qam_interleaver_cap_t;


/// QAM DOCSIS/Video Capabilities Bits
typedef struct {
    uint16 reserved : 11;
    uint32 stream_redundancy : 1;
    uint32 mixed_static_dynamic : 1;
    uint32 docsis_PSP : 1;
    uint32 docsis_MPT : 1;
    uint32 video : 1;
    uint32 reserved2 : 6;
    uint32 mixed_docsis : 1;
    uint32 mixed_video_data : 1;
    uint32 tsid_grp_id: 7;
    uint32 lock : 1;
} errp_qam_docsis_video_cap_t;


/// QAM Modulation Capability Bits
typedef struct {
    uint16 reserved : 6;
    uint16 _256_qam : 1;
    uint16 _64_qam : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_modulation_cap_t;


/// QAM Capabilities
typedef struct {
    errp_qam_chan_bw_cap_t chan_bw;
    errp_qam_j83_cap_t j83;
    errp_qam_interleaver_cap_t interleaver;
    errp_qam_docsis_video_cap_t docsis_video;
    errp_qam_modulation_cap_t modulation;
} errp_qam_cap_t;


/// ERRP Edge Input Attribute
typedef struct {
    uint32 if_id;
    uint32 subnet_mask;
    uint32 max_bandwidth;
    char host_name[ERRP_MAX_NAME_LEN];
    char group_name[ERRP_MAX_NAME_LEN];
} errp_edge_input_t;


/// ERRP QAM configuration attributes
typedef struct {
    uint32 freq;
    uint8 modulation;
    uint8 interleaver;
    uint16 tsid;
    uint8 annex;
    uint8 chan_bw;
} errp_qam_cfg_t;


/// UDP Port Map types
typedef enum
{
    ERRP_UDP_MAP_TYPE_STATIC_PORT = 1,
    ERRP_UDP_MAP_TYPE_STATIC_RANGE = 2,
    ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE = 3,
} errp_udp_map_type_t;

/// ERRP UDP Map Attribute for Static Port
typedef struct {
    uint16 udp_port;
    uint16 prog_num;
} errp_udp_map_static_port_t;

/// ERRP UDP Map Attribute for Static Port Range
typedef struct {
    uint16 start_udp_port;
    uint16 start_prog_num;
    uint16 count;
} errp_udp_map_static_range_t;

/// ERRP UDP Map Attribute for Dynamic Port Range
typedef struct {
    uint16 start_udp_port;
    uint16 count;
} errp_udp_map_dynamic_range_t;

/// ERRP UDP Map Attribute
typedef struct {
    errp_udp_map_type_t type;
    union {
        errp_udp_map_static_port_t static_port;
        errp_udp_map_static_range_t static_range;
        errp_udp_map_dynamic_range_t dynamic_range;
    };
} errp_udp_map_t;


// QAM Capability related definitions
//

/// ERRP QAM Channel Bandwidth Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 _6mhz : 1;
    uint16 _7mhz : 1;
    uint16 _8mhz : 1;
} qam_chan_bw_cap_t;


/// ERRP QAM J83 Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 annexA : 1;
    uint16 annexB : 1;
    uint16 annexC : 1;
} qam_j83_cap_t;


/// ERRP QAM Interleaver Capability Bits
typedef struct {
    uint32 lock : 1;
    uint32 tsid_grp_id: 7;
    uint32 i8_j16 : 1;
    uint32 i16_j8 : 1;
    uint32 i32_j4 : 1;
    uint32 i64_j2 : 1;
    uint32 i128_j1 : 1;
    uint32 i128_j2 : 1;
    uint32 i128_j3 : 1;
    uint32 i128_j4 : 1;
    uint32 i128_j5 : 1;
    uint32 i128_j6 : 1;
    uint32 i128_j7 : 1;
    uint32 i128_j8 : 1;
    uint32 i12_j7 : 1;
} qam_interleaver_cap_t;

/// ERRP QAM DOCSIS/Video Capability Bits
typedef struct {
    uint32 lock : 1;
    uint32 tsid_grp_id: 7;
    uint32 mixed_video_data : 1;
    uint32 mixed_docsis : 1;
    uint32 reserved : 6;
    uint32 video : 1;
    uint32 docsis_MPT : 1;
    uint32 docsis_PSP : 1;
    uint32 mixed_static_dynamic : 1;
    uint32 stream_redundancy : 1;
} qam_docsis_video_cap_t;


/// ERRP QAM Modulation Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 _64qam : 1;
    uint16 _256qam : 1;
} qam_modulation_cap_t;


/// ERRP CAS Capablity
typedef struct {
    uint8 enc_type;
    uint8 enc_scheme;
    uint16 key_len;
    uint16 cas_id;
} errp_cas_cap_t;


/// ERRP Route Type
typedef struct {
    uint16 addr_family;         ///< address family
    uint16 app_protocol;        ///< application protocol
} errp_route_type_t;


/// ERRP Route
typedef struct {
    errp_route_type_t type;
    char addr[ERRP_MAX_NAME_LEN];
} errp_route_t;


/// ERRP Server (for IP signalling)
typedef struct {
    uint32 addr_domain;
    char component_addr[ERRP_MAX_NAME_LEN];
    char streaming_zone[ERRP_MAX_NAME_LEN];
} errp_server_t;


/// ERRP Resource
typedef struct {
    uint32 attr_mask;           ///< attribute bit mask
    errp_route_t withdrawn_route;
    errp_route_t reachable_route;
    errp_server_t next_hop_server;
    uint16 alt_server_cnt;
    errp_server_t next_hop_server_alt[ERRP_MAX_ALT_SERVERS];
    char qam_name[ERRP_MAX_NAME_LEN];
    int total_bandwidth;
    int avail_bandwidth;
    int edge_input_cnt;
    errp_edge_input_t edge_input[ERRP_MAX_EDGE_INPUTS];
    errp_qam_cfg_t qam_cfg;
    int udp_map_cnt;
    errp_udp_map_t udp_map[ERRP_MAX_MAPS_PER_ROUTE];
    uint32 port_id;
    errp_qam_cap_t qam_cap;
    uint8 cost;
    uint32 service_status;
    uint32 max_flows;
    errp_cas_cap_t cas_cap;
    int fiber_node_cnt;
    char fiber_node[ERRP_MAX_FIBER_NODES_PER_ROUTE][ERRP_MAX_NAME_LEN];
    int input_cnt;
    char input_map[ERRP_MAX_INPUTS_PER_ROUTE][ERRP_MAX_NAME_LEN];
} errp_resource_t;


/// Information of an ERRP Node
typedef struct {
    uint8 version;              ///< protocol version
    uint16 hold_time;           ///< hold time
    uint32 addr_domain;         ///< address domain
    uint32 errp_id;             ///< ERRP identifier

    // Mandatory parameters
    char streaming_zone[ERRP_MAX_NAME_LEN];
    char component_name[ERRP_MAX_NAME_LEN];

    // Optional parameters
    char vendor_string[ERRP_MAX_NAME_LEN];

    // Capabilities
    int route_type_cnt;
    errp_route_type_t route_type[ERRP_MAX_ROUTE_TYPES];
                                ///< route types supported
    uint32 send_receive;        ///< send receive capability
    uint32 errp_version;        ///< ERRP version
} errp_node_t;


/// Information of an ERRP connection
typedef struct {
    int fd;                     ///< socket descriptor
    uint32 fsm_state;           ///< FSM state

    boolean connected;          ///< whether the connection is up

    struct sockaddr_in addr;    ///< socket address of peer node
    uint32 connect_time;        ///< ConnectRetry timer value
    uint32 keepalive_time;      ///< Keepalive timer value
    uint32 hold_time;           ///< Hold timer value

    TIMER_T connect_timer_id;   ///< ConnectRetry timer ID
    TIMER_T keepalive_timer_id; ///< Keepalive timer ID
    TIMER_T hold_timer_id;      ///< Hold timer ID
    SEMA_T conn_sema;           ///< Semaphore used for connect

    int avail_len;              ///< length of available data in message buffer
    int in_msg_offset;          ///< offset into message buffer for the next msg
    char *in_msg_buf;           ///< incoming message buffer

    errp_node_t *node;          ///< pointer to ERRP node info of this node
    errp_node_t *peer;          ///< ERRP node info of the peer
} errp_conn_t;


/// ERRP Error Report
typedef struct {
    uint8 err;          ///< error code
    uint8 err_sub;      ///< error subcode
    int len;            ///< number of bytes in error data
    char data[0];       ///< array to hold error data
} errp_error_report_t;


int errp_put_msg_hdr(char **buf, int msg_type, int len);
int errp_put_open_msg(char **buf, const errp_node_t *node);
int errp_put_update_msg(char **buf, const errp_resource_t *resource);
int errp_put_notification_msg(char **buf, const errp_error_report_t *err_rpt);
int errp_put_keepalive_msg(char **buf);

void errp_get_msg_hdr(parser_t *psr, uint8 *msg_type, uint16 *msg_len);
void errp_get_open_msg(parser_t *psr, errp_conn_t *conn,
                       errp_error_report_t *err_rpt);
void errp_get_update_msg(parser_t *psr, errp_resource_t *resource,
                         errp_error_report_t *err_rpt);
void errp_get_notification_msg(parser_t *psr, errp_error_report_t *err_rpt);
void errp_get_keepalive_msg(parser_t *psr);


// ERRP utilities
void errp_node_init(errp_node_t *node, uint16 hold_time, uint32 addr_domain,
                    uint32 errp_id);
void errp_node_print(const errp_node_t *node);

void errp_conn_init(errp_conn_t *conn, errp_node_t *node, errp_node_t *peer);
void errp_conn_print(const errp_conn_t *conn);

void errp_set_route_type(errp_route_type_t *route_type,
                         uint16 address_family, uint16 app_protocol);
void errp_route_print(const errp_route_t *route);
void errp_resource_print(const errp_resource_t *resource);
void errp_error_report_print(const errp_error_report_t *rpt);

uint8 errp_find_missing_attr(uint32 attr_mask);

int errp_get_peer_msg(errp_conn_t *conn, parser_t *psr, uint8 *msg_type,
                      uint16 *msg_len);

void errp_set_err_bad_msg_len(errp_error_report_t *rpt, uint16 msg_len);
void errp_set_err_bad_msg_type(errp_error_report_t *rpt, uint8 msg_type);
void errp_set_err_mismatch_cap(errp_error_report_t *rpt);
void errp_add_err_mismatch_cap(errp_error_report_t *rpt, uint16 cap_code,
                               uint16 cap_len, void* cap_value);

// ERRP Callback Functions
boolean errp_check_cap_param(errp_conn_t *conn, errp_error_report_t *rpt);
boolean errp_is_addr_domain_valid(uint32 addr_domain);
boolean errp_is_errp_id_present(uint32 addr_domain, uint32 errp_id);

// Timer event handlers
void errp_connect_timer_event_handler(union sigval value);
void errp_keepalive_timer_event_handler(union sigval value);
void errp_hold_timer_event_handler(union sigval value);

// General utilities
char* errp_fsm_state_txt(uint32 state);
char* errp_fsm_event_txt(uint32 event);

#endif  // __ERRP_H__
