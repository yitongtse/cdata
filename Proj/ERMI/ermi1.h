/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: ermi1.h
*    @brief Header file for ERRP related definitions
*    @author Yi Tong Tse
*/

// Defines accroding to ERRP spec
//
#define ERRP_MSG_HDR_SIZE         3
#define ERRP_PARAM_HDR_SIZE       4
#define ERRP_CAP_INFO_HDR_SIZE    4
#define ERRP_ATTR_HDR_SIZE        4

#define ERRP_TCP_PORT_DFT         6069

#define ERRP_MSG_BYTES_MAX        4096
#define ERRP_HEADER_BYTES         3     

#define ERRP_RESERVED             0

#define ERRP_OPEN_MSG_VERSION     3
#define ERRP_HOLD_TIME_MIN        3

#define ERRP_ADDR_DOMAIN_ANY      0

#define ERRP_ADDR_FAMILY          32769  /* RTSP URL */

#define ERRP_VERSION              1

#define ERRP_ATTR_FLAG_WELL_KNOWN 0x80


// Implementation specific defines below
//
#define ERRP_MAX_NAME_LEN                 64
#define ERRP_MAX_ROUTES_PER_ATTR          1
#define ERRP_MAX_EDGE_INPUTS_PER_ATTR     1
#define ERRP_MAX_UDP_MAPS_PER_ATTR        1


// ERRP error codes
#define ERRP_OK                           0

#define ERRP_ERR_MSG_HEADER               1
#define ERRP_ERR_OPEN_MSG                 2
#define ERRP_ERR_UPDATE_MSG               3
#define ERRP_ERR_HOLD_TIMER_EXPIRED       4
#define ERRP_ERR_FSM                      5
#define ERRP_ERR_CEASE                    6

#define ERRP_SUB_ERR_UNSPECIFIED          0

// ERRP message header error subcode
#define ERRP_SUB_ERR_BAD_MSG_LEN          1
#define ERRP_SUB_ERR_BAD_MSG_TYPE         2

// ERRP OPEN message error subcode
#define ERRP_SUB_ERR_UNSUP_VERSION        1
#define ERRP_SUB_ERR_BAD_PEER_ADDR_DOMAIN 2
#define ERRP_SUB_ERR_BAD_ERRP_ID          3
#define ERRP_SUB_ERR_UNSUP_OPT_PARAM      4
#define ERRP_SUB_ERR_UNSUP_HOLD_TIME      5
#define ERRP_SUB_ERR_UNSUP_CAP            6
#define ERRP_SUB_ERR_CAP_MISMATCH         7

// ERRP UPDATE message error subcode
#define ERRP_SUB_ERR_MALFORMED_ATTR_LIST  1
#define ERRP_SUB_ERR_UNRECOG_ATTR         2
#define ERRP_SUB_ERR_MISSING_ATTR         3
#define ERRP_SUB_ERR_ATTR_FLAGS           4
#define ERRP_SUB_ERR_ATTR_LEN             5
#define ERRP_SUB_ERR_INVALID_ATTR         6


//! ERRP Message Types
typedef enum {
    ERRP_OPEN = 1,
    ERRP_UPDATE = 2,
    ERRP_NOTIFICATION = 3,
    ERRP_KEEPALIVE = 4,
} errp_message_type_t;


//! ERRP Parameter Types in OPEN message
typedef enum {
    ERRP_PARAM_CAP_INFO = 1,
    ERRP_PARAM_STREAMING_ZONE = 2,
    ERRP_PARAM_COMPONENT_NAME = 3,
    ERRP_PARAM_VENDOR_STRING = 4,
} errp_param_t;


//! ERRP Capability Codes
typedef enum {
    ERRP_CAP_ROUTE_TYPES_SUPPORTED = 1,
    ERRP_CAP_SEND_RECEIVE = 2,
    ERRP_CAP_ERRP_VERSION = 32768,
} errp_cap_code_t;


//! ERRP Application Protocols
typedef enum {
    ERRP_PROT_STATIC = 32766,
    ERRP_PROT_DYNAMIC = 32768,
    ERRP_PROT_DYNAMIC_PROVISION = 32770,
} errp_app_protocol_t;


//! ERRP Send Receive Capability
typedef enum {
    ERRP_SEND_AND_RECEIVE = 1,
    ERRP_SEND_ONLY = 2,
    ERRP_RECEIVE_ONLY = 3,
} errp_send_receive_t;


//! ERRP Attribute Type Codes
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
} errp_attr_t;


// ERRP QAM configuration related definitios
//

#define ERRP_QAM_CFG_ERR        255

//! QAM Modulation Modes
enum {
  ERRP_QAM_CFG_MODULATION_64_QAM = 3,
  ERRP_QAM_CFG_MODULATION_256_QAM = 4,
  ERRP_QAM_CFG_MODULATION_128_QAM = 5,
  ERRP_QAM_CFG_MODULATION_512_QAM = 6,
  ERRP_QAM_CFG_MODULATION_1024_QAM = 7,
};

//! QAM Interleaver Settings
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

//! QAM Annex Modes
enum {
  ERRP_QAM_CFG_ANNEX_A = 0,
  ERRP_QAM_CFG_ANNEX_B = 1,
  ERRP_QAM_CFG_ANNEX_C = 2,
};

//! QAM Channel Bandwidth Types
enum {
  ERRP_QAM_CFG_CHAN_BW_6MHZ = 0,
  ERRP_QAM_CFG_CHAN_BW_7MHZ = 2,
  ERRP_QAM_CFG_CHAN_BW_8MHZ = 1,
};


// ERRP QAM Capability related definiitions
// Note: In ERMI spec, bit 0 is the most significant bit!
//

//! QAM Channel Bandwidth Capability Bits
typedef struct {
    uint16 reserved : 5;
    uint16 _8mhz : 1;
    uint16 _7mhz : 1;
    uint16 _6mhz : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_chan_bw_cap_t;


//! QAM J83 Capability Bits
typedef struct {
    uint16 reserved : 5;
    uint16 annex_C : 1;
    uint16 annex_B : 1;
    uint16 annex_A : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_j83_cap_t;


//! QAM Interleaver Capability Bits
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


//! QAM DOCSIS/Video Capabilities Bits
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


//! QAM Modulation Capability Bits
typedef struct {
    uint16 reserved : 6;
    uint16 _256_qam : 1;
    uint16 _64_qam : 1;
    uint16 tsid_grp_id: 7;
    uint16 lock : 1;
} errp_qam_modulation_cap_t;


//! QAM Capabilities
typedef struct {
    errp_qam_chan_bw_cap_t chan_bw;
    errp_qam_j83_cap_t j83;
    errp_qam_interleaver_cap_t interleaver;
    errp_qam_docsis_video_cap_t docsis_video;
    errp_qam_modulation_cap_t modulation;
} errp_qam_cap_t;


//! ERRP Edge Input Attribute
typedef struct {
    uint32 if_id;
    uint32 subnet_mask;
    uint32 max_bandwidth;
    char* host_name;
    char* group_name;
} errp_edge_input_t;


//! ERRP Route
typedef struct {
    uint16 addr_family;
    uint16 app_protocol;
    char addr[ERRP_MAX_NAME_LEN];
} errp_route_t;


//! Information of an ERRP Node
typedef struct {
    uint16 hold_time;      //!< hold time
    uint32 addr_domain;    //!< address domain
    uint32 errp_id;
    uint16 addr_family;
    uint16 app_protocol;
    uint32 send_receive;
    uint32 errp_version;
    char streaming_zone[ERRP_MAX_NAME_LEN];
    char component_name[ERRP_MAX_NAME_LEN];
    char vendor_string[ERRP_MAX_NAME_LEN];
} errp_node_t;


//! ERRP QAM configuration attributes
typedef struct {
    uint32 freq;
    uint8 modulation;
    uint8 interleaver;
    uint16 tsid;
    uint8 annex;
    uint8 chan_bw;
} errp_qam_cfg_t;


//! UDP Port Map types
typedef enum
{
    ERRP_UDP_MAP_TYPE_STATIC_PORT = 1,
    ERRP_UDP_MAP_TYPE_STATIC_RANGE = 2,
    ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE = 3,
} errp_udp_map_type_t;

//! ERRP UDP Map Attribute for Static Port
typedef struct {
    uint16 udp_port;
    uint16 prog_num;
} errp_udp_map_static_port_t;

//! ERRP UDP Map Attribute for Static Port Range
typedef struct {
    uint16 start_udp_port;
    uint16 start_prog_num;
    uint16 count;
} errp_udp_map_static_range_t;

//! ERRP UDP Map Attribute for Dynamic Port Range
typedef struct {
    uint16 start_udp_port;
    uint16 count;
} errp_udp_map_dynamic_range_t;

//! ERRP UDP Map Attribute
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

//! ERRP QAM Channel Bandwidth Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 _6mhz : 1;
    uint16 _7mhz : 1;
    uint16 _8mhz : 1;
} qam_chan_bw_cap_t;


//! ERRP QAM J83 Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 annexA : 1;
    uint16 annexB : 1;
    uint16 annexC : 1;
} qam_j83_cap_t;


//! ERRP QAM Interleaver Capability Bits
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

//! ERRP QAM DOCSIS/Video Capability Bits
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


//! ERRP QAM Modulation Capability Bits
typedef struct {
    uint16 lock : 1;
    uint16 tsid_grp_id: 7;
    uint16 _64qam : 1;
    uint16 _256qam : 1;
} qam_modulation_cap_t;


//! ERRP QAM Capabilities
typedef struct {
    qam_chan_bw_cap_t chan_bw_cap;
    qam_j83_cap_t j83_cap;
    qam_interleaver_cap_t interleaver_cap;
    qam_docsis_video_cap_t docsis_video_cap;
    qam_modulation_cap_t modulation_cap;
} qam_cap_t;


void errp_node_init(errp_node_t *node);
void errp_node_print(const errp_node_t *node);
void errp_route_print(const errp_route_t *route);

char* errp_skip_msg_hdr(char **buf);
int errp_put_msg_hdr(char **buf, int msg_type, int len);
int errp_put_open_msg(char **buf, const errp_node_t *node);
int errp_put_attr_withdrawn_routes(char **buf, int num_routes,
                                   const errp_route_t *route);
int errp_put_attr_reachable_routes(char **buf, int num_routes,
                                   const errp_route_t *route);
int errp_put_attr_next_hop_server(char **buf, uint32 addr_domain,
                                  const char* component_addr,
                                  const char* streaming_zone);
int errp_put_attr_qam_names(char **buf, const char *qam_name);
int errp_put_attr_total_bandwidth(char **buf, int bandwidth);
int errp_put_attr_edge_input(char **buf, int num_inputs,
                             const errp_edge_input_t *input);
int errp_put_attr_qam_cfg(char **buf, const errp_qam_cfg_t *cfg);
int errp_put_attr_udp_map(char **buf, int num_maps, const errp_udp_map_t *map);
int errp_put_attr_qam_cap(char **buf, const errp_qam_cap_t *cap);



int errp_get_msg(const char **buf, errp_node_t *node);




