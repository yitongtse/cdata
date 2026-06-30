/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: rtsp.h
*    @brief Header file for ERMI RTSP related definitions
*    @author Yi Tong Tse
*/

#ifndef __RTSP_H__
#define __RTSP_H__

#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include "common.h"
#include "util.h"
#include "os_util_lnx.h"
#include "os_util.h"


//#define RTSP_TCP_PORT_DFT       554
#define RTSP_TCP_PORT_DFT       6070    /* to bypass permission issue */

#define RTSP_MAX_MSG_BYTES      4096  /* ?? */
#define RTSP_MSG_HDR_SIZE       16  /* ?? */

#define SPACE           ' '
#define TAB             '\t'
#define CRLF            "\r\n"


#define RTSP_VERSION                            "RTSP/1.0"
#define RTSP_ERMI_REQUIRE                       "com.cablelabs.ermi"

#define	RTSP_LOG(fmt, str...)           \
    printf("RTSP: "fmt"\n", ##str);
//    NULL


// Implementation specific defines
#define RTSP_MAX_TOKEN_LEN                      32
#define RTSP_MAX_TRANSPORTS                     4
#define RTSP_MAX_SESSIONS_PER_CONN              4
#define RTSP_MAX_SESSION_GROUPS_PER_CONN        4


// RTSP Methods
#define RTSP_METHOD_SETUP               "SETUP"
#define RTSP_METHOD_TEARDOWN            "TEARDOWN"
#define RTSP_METHOD_GET_PARAMETER       "GET_PARAMETER"
#define RTSP_METHOD_SET_PARAMETER       "SET_PARAMETER"
#define RTSP_METHOD_ANNOUNCE            "ANNOUNCE"


// RTSP Response Codes
typedef enum {
    RTSP_RESP_OK                                  = 200,
    RTSP_RESP_BAD_REQUEST                         = 400,
    RTSP_RESP_FORBIDDEN                           = 403,
    RTSP_RESP_NOT_FOUND                           = 404,
    RTSP_RESP_METHOD_NOT_ALLOWED                  = 405,
    RTSP_RESP_REQUEST_TIMEOUT                     = 408,
    RTSP_RESP_PRECONDITION_FAILED                 = 412,
    RTSP_RESP_PARAMETER_NOT_UNDERSTOOD            = 451,
    RTSP_RESP_NOT_ENOUGH_BANDWIDTH                = 453,
    RTSP_RESP_SESSION_NOT_FOUND                   = 454,
    RTSP_RESP_HEADER_FIELD_NOT_VALID_FOR_RESOURCE = 456,
    RTSP_RESP_INVALID_RANGE                       = 457,
    RTSP_RESP_UNSUPPORTED_TRANSPORT               = 461,
    RTSP_RESP_DESTINATION_UNREACHABLE             = 462,
    RTSP_RESP_INTERNAL_SERVER_ERROR               = 500,  // Not in ERMI spec!
    RTSP_RESP_NOT_IMPLEMENTED                     = 501,
    RTSP_RESP_SERVICE_UNAVAILABLE                 = 503,
    RTSP_RESP_VERSION_NOT_SUPPORTED               = 505,
    RTSP_RESP_OPTION_NOT_SUPPORTED                = 551,
} rtsp_resp_code_t;


// RTSP TEARDOWN Reason Codes
typedef enum {
    RTSP_REASON_USER_STOPPED                    = 200,
    RTSP_REASON_NO_USER_ACTIVITY                = 204,
    RTSP_REASON_SETTOP_CAP_MISMATCH             = 205,
    RTSP_REASON_INSUFFICIENT_PRIORITY           = 206,
    RTSP_REASON_NETWORK_DELIVERY_FAILURE        = 207,
    RTSP_REASON_FAIL_TO_TUNE                    = 400,
    RTSP_REASON_LOSS_OF_TUNE                    = 401,
    RTSP_REASON_RTSP_FAILURE                    = 403,
    RTSP_REASON_CHANNEL_FAILURE                 = 404,
    RTSP_REASON_NO_RTSP_SERVER                  = 405,
    RTSP_REASON_UNKNOWN                         = 408,
    RTSP_REASON_NETWORK_RESOURCE_FAILURE        = 409,
    RTSP_REASON_SETTOP_HEARTBEAT_TIMEOUT        = 420,
    RTSP_REASON_SETTOP_INACTIVITY_TIMEOUT       = 421,
    RTSP_REASON_CONTENT_UNAVAILBLE              = 422,
    RTSP_REASON_STREAMING_FAILURE               = 423,
    RTSP_REASON_QAM_FAILURE                     = 424,
    RTSP_REASON_VOLUME_FAILURE                  = 425,
    RTSP_REASON_STREAM_CONTROL_ERROR            = 426,
    RTSP_REASON_STREAM_CONTROL_TIMEOUT          = 427,
    RTSP_REASON_SESSION_LIST_MISMATCH           = 428,
    RTSP_REASON_QAM_PARAMETER_MISMATCH          = 502,
    RTSP_REASON_SESSION_TIMEOUT                 = 550,
} rtsp_reason_code_t;


// RTSP clab-Notice Codes
typedef enum {
    RTSP_CLAB_NOTICE_DELIVERY_SUCCEED           = 2104,
    RTSP_CLAB_NOTICE_PID_CONFLICT               = 4400,
    RTSP_CLAB_NOTICE_INPUT_TS_INVALID           = 4401,
    RTSP_CLAB_NOTICE_PROGRAM_NUMBER_CONFLICT    = 4402,
    RTSP_CLAB_NOTICE_DOWNSTREAM_FAILURE         = 5401,
    RTSP_CLAB_NOTICE_SERVER_RESOURCE_UNAVIL     = 5200,
    RTSP_CLAB_NOTICE_UNABLE_TO_JOIN             = 5404,
    RTSP_CLAB_NOTICE_INPUT_INTERFACE_FAILURE    = 5405,
    RTSP_CLAB_NOTICE_REDUNDANT_SOURCE_FAILOVER  = 5406,
    RTSP_CLAB_NOTICE_INTERNAL_SERVER_ERROR      = 5502,
    RTSP_CLAB_NOTICE_BANDWIDTH_EXCEEDED_LIMIT   = 5602,
    RTSP_CLAB_NOTICE_SESSION_IN_PROGRESS        = 5700,
    RTSP_CLAB_NOTICE_RECLAIM_SESSION            = 5701,
} rtsp_clab_notice_code_t;


/// clab-MPTSMode
#define RTSP_CLAB_MPTS_MODE_PASSTHROUGH         "passthrough"
#define RTSP_CLAB_MPTS_MODE_MULTIPLEX           "multiplex"

enum {
    RTSP_CLAB_MPTS_MODE_IDX_PASSTHROUGH,
    RTSP_CLAB_MPTS_MODE_IDX_MULTIPLEX,
};

extern const char* rtsp_clab_mpts_modes[];


// ERMI RTSP parameters supported in GET_PARAMETER and SET_PARAMETER
#define RTSP_PARAM_CLAB_SESSION_LIST           "clab-session-list"
#define RTSP_PARAM_CLAB_CONNECTION_TIMEOUT     "clab-connection-timeout"
#define RTSP_PARAM_CLAB_SESSION_GROUP_LIST     "clab-sessiongroup-list"

enum {
    RTSP_PARAM_IDX_CLAB_SESSION_LIST,
    RTSP_PARAM_IDX_CLAB_CONNECTION_TIMEOUT,
    RTSP_PARAM_IDX_CLAB_SESSION_GROUP_LIST,
};


/// RTSP transport parameter strings
#define RTSP_TRANS_PAR_UNICAST                  "unicast"
#define RTSP_TRANS_PAR_MULTICAST                "multicast"
#define RTSP_TRANS_PAR_BITRATE                  "bit_rate"
#define RTSP_TRANS_PAR_DEPI_MODE                "depi_mode"
#define RTSP_TRANS_PAR_SRC                      "source"
#define RTSP_TRANS_PAR_SRC_PORT                 "source_port"
#define RTSP_TRANS_PAR_DST                      "destination"
#define RTSP_TRANS_PAR_DST_PORT                 "destination_port"
#define RTSP_TRANS_PAR_MULTICAST_ADDR           "multicast_address"
#define RTSP_TRANS_PAR_RANK                     "rank"
#define RTSP_TRANS_PAR_MPTS_PROG                "mpts_program"
#define RTSP_TRANS_PAR_QAM_TSID                 "qam_tsid"
#define RTSP_TRANS_PAR_FIBER_NODE               "fiber_node"
#define RTSP_TRANS_PAR_FREQ_RANGE               "frequency_range"
#define RTSP_TRANS_PAR_QAM_NAME                 "qam_name"
#define RTSP_TRANS_PAR_QAM_DST                  "qam_destination"
#define RTSP_TRANS_PAR_MODULATION               "modulation"
#define RTSP_TRANS_PAR_J83_ANNEX                "j83_annex"
#define RTSP_TRANS_PAR_TAPS                     "taps"
#define RTSP_TRANS_PAR_CHANNEL_WIDTH            "channe_width"
#define RTSP_TRANS_PAR_SYMBOL_RATE              "symbol_rate"

/// Bit masks for RTSP transport parameters
#define RTSP_TRANS_PAR_MASK_UNICAST             0x00000001
#define RTSP_TRANS_PAR_MASK_MULTICAST           0x00000002
#define RTSP_TRANS_PAR_MASK_BITRATE             0x00000004
#define RTSP_TRANS_PAR_MASK_DEPI_MODE           0x00000008
#define RTSP_TRANS_PAR_MASK_SRC                 0x00000010
#define RTSP_TRANS_PAR_MASK_SRC_PORT            0x00000020
#define RTSP_TRANS_PAR_MASK_DST                 0x00000040
#define RTSP_TRANS_PAR_MASK_DST_PORT            0x00000080
#define RTSP_TRANS_PAR_MASK_MULTICAST_ADDR      0x00000100
#define RTSP_TRANS_PAR_MASK_RANK                0x00000200
#define RTSP_TRANS_PAR_MASK_MPTS_PROG           0x00000400
#define RTSP_TRANS_PAR_MASK_QAM_TSID            0x00000800
#define RTSP_TRANS_PAR_MASK_FIBER_NODE          0x00001000
#define RTSP_TRANS_PAR_MASK_FREQ_RANGE          0x00002000
#define RTSP_TRANS_PAR_MASK_QAM_NAME            0x00004000
#define RTSP_TRANS_PAR_MASK_QAM_DST             0x00008000
#define RTSP_TRANS_PAR_MASK_MODULATION          0x00010000
#define RTSP_TRANS_PAR_MASK_J83_ANNEX           0x00020000
#define RTSP_TRANS_PAR_MASK_TAPS                0x00040000
#define RTSP_TRANS_PAR_MASK_CHANNEL_WIDTH       0x00080000
#define RTSP_TRANS_PAR_MASK_SYMBOL_RATE         0x00100000

#define RTSP_CONTENT_TYPE_TEXT_PARAM            "text/parameters"
#define RTSP_CONTENT_TYPE_TEXT_XML              "text/xml"

enum {
    RTSP_CONTENT_TYPE_IDX_TEXT_PARAM,
    RTSP_CONTENT_TYPE_IDX_TEXT_XML,
};

extern const char* rtsp_content_types[];


/// ERMI Transport Profiles
#define RTSP_TRANS_PROFILE_DOCSIS_QAM           "clab-DOCSIS/QAM"
#define RTSP_TRANS_PROFILE_MP2T_QAM             "clab-MP2T/DVBC/QAM"
#define RTSP_TRANS_PROFILE_MP2T_UDP             "clab-MP2T/DVBC/UDP"

enum {
    RTSP_TRANS_PROFILE_IDX_DOCSIS_QAM,
    RTSP_TRANS_PROFILE_IDX_MP2T_QAM,
    RTSP_TRANS_PROFILE_IDX_MP2T_UDP
};

extern const char* rtsp_trans_profiles[];


/// ERMI DEPI Modes
#define RTSP_TRANS_DEPI_MODE_MPT                "docsis_mpt"
#define RTSP_TRANS_DEPI_MODE_PSP                "docsis_psp"

enum {
    RTSP_TRANS_DEPI_MODE_IDX_MPT,
    RTSP_TRANS_DEPI_MODE_IDX_PSP,
};

extern const char* rtsp_trans_depi_modes[];


/// ERMI Annexes
#define RTSP_TRANS_J83_ANNEX_A                  "Annex_A"
#define RTSP_TRANS_J83_ANNEX_B                  "Annex_B"
#define RTSP_TRANS_J83_ANNEX_C                  "Annex_C"

enum {
    RTSP_TRANS_J83_ANNEX_IDX_A,
    RTSP_TRANS_J83_ANNEX_IDX_B,
    RTSP_TRANS_J83_ANNEX_IDX_C,
};

extern const char* rtsp_trans_j83_annexes[];


typedef struct {
    char *spec;
    uint32 par_mask;
    int profile_idx;
    int depi_mode_idx;
    int bitrate;
    uint32 src_ip;
    uint16 src_udp;    
    uint32 dst_ip;
    uint16 dst_udp;
    uint32 mcast_ip;
    uint32 rank;
    uint16 mpts_prog;
    uint16 tsid;
    char *fiber_nodes;
    uint32 freq_low;
    uint32 freq_high;
    char *service_group;
    uint32 freq;
    uint16 prog_num;
    int modulation;
    int j83_annex_idx;
    int taps;
    int incr;
    int channel_width;
    int symbol_rate;
} rtsp_trans_t;


/// RTSP header string
#define RTSP_HDR_CSEQ                   "CSeq"
#define RTSP_HDR_SESSION                "Session"
#define RTSP_HDR_REQUIRE                "Require"
#define RTSP_HDR_TRANSPORT              "Transport"
#define RTSP_HDR_CONTENT_TYPE           "Content-Type"
#define RTSP_HDR_CONTENT_LENGTH         "Content-Length"
#define RTSP_HDR_CLAB_CLIENT_SESSION_ID "clab-ClientSessionId"
#define RTSP_HDR_CLAB_NOTICE            "clab-Notice"
#define RTSP_HDR_CLAB_REASON            "clab-Reason"
#define RTSP_HDR_CLAB_SESSION_GROUP     "clab-SessionGroup"
#define RTSP_HDR_CLAB_PRIORITY          "clab-Priority"
#define RTSP_HDR_CLAB_SETUP_TYPE        "clab-SetupType"
#define RTSP_HDR_CLAB_PID_REMAP         "clab-PidRemap"
#define RTSP_HDR_CLAB_MPTS_MODE         "clab-MPTSMode"
#define RTSP_HDR_CLAB_STATMUX_GROUP     "clab-StatmuxGroup"

/// Bit masks for RTSP headers
#define RTSP_HDR_MASK_CSEQ                      0x00000001
#define RTSP_HDR_MASK_SESSION                   0x00000002
#define RTSP_HDR_MASK_REQUIRE                   0x00000004
#define RTSP_HDR_MASK_TRANSPORT                 0x00000008
#define RTSP_HDR_MASK_CONTENT_TYPE              0x00000010
#define RTSP_HDR_MASK_CONTENT_LENGTH            0x00000020
#define RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID    0x00000040
#define RTSP_HDR_MASK_CLAB_NOTICE               0x00000080
#define RTSP_HDR_MASK_CLAB_REASON               0x00000100
#define RTSP_HDR_MASK_CLAB_SESSION_GROUP        0x00000200
#define RTSP_HDR_MASK_CLAB_PRIORITY             0x00000400
#define RTSP_HDR_MASK_CLAB_SETUP_TYPE           0x00000800
#define RTSP_HDR_MASK_CLAB_PID_REMAP            0x00001000
#define RTSP_HDR_MASK_CLAB_MPTS_MODE            0x00002000
#define RTSP_HDR_MASK_CLAB_STATMUX_GROUP        0x00004000

/// Bit masks for mandatory headers
#define RTSP_HDR_MASK_REQUIRED_SETUP_REQ                        \
                ( RTSP_HDR_MASK_CSEQ                    |       \
                  RTSP_HDR_MASK_REQUIRE                 |       \
                  RTSP_HDR_MASK_TRANSPORT               |       \
                  RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID  |       \
                  RTSP_HDR_MASK_MPTS_MODE )

extern char *rtsp_method[];


/// RTSP Session
typedef struct {
    boolean used;
    char ses_id[RTSP_MAX_TOKEN_LEN];    /// RTSP session ID
    char ses_grp[RTSP_MAX_TOKEN_LEN];   /// session group
    uint64 clab_cln_id;                 /// client ID (in clab-ClientSessionId)
    uint32 clab_ses_id;                 /// session ID (in clab-ClientSessionId)
} rtsp_session_t;


/// RTSP Request
typedef struct {
    uint32 hdr_mask;            ///< header bit mask
    char *method;               /// method (in request)
    char *url;                  /// URL (in request)
    char *version;              /// RTSP version
    uint32 cseq;
    int rc;                     ///< response code (in responses)
    char *rc_phrase;            ///< response phrase (in responses)
    char *ses_id;
    int timeout;                ///< timeout included in Session header
    char require[RTSP_MAX_TOKEN_LEN];
    int16 content_type_idx;
    uint32 content_length;
    uint64 clab_cln_id;
    uint32 clab_ses_id;
    uint32 notice_code;
    char *notice_desc;
    char *date_time;
    uint32 reason_code;
    char *reason_phrase;
    char *session_group;
    uint8 priority;     // 1DIGIT
    uint8 setup_type;   // 1DIGIT
    uint8 pid_remap;
    int16 mpts_mode_idx;    
    char *statmux_group_id;
    uint32 statmux_group_bitrate;
    int transport_cnt;      ///< number of transport specs
    rtsp_trans_t transport[RTSP_MAX_TRANSPORTS];  ///< transport specs
    char *msg_body;
} rtsp_msg_t;


/// Information of an RTSP Connection
typedef struct {
    int id;
    int fd;                     ///< socket descriptor
    boolean connected;          ///< whether the connection is up
    timer_t hold_timer_id;      ///< Hold timer ID

    char *in_msg_buf;           ///< incoming message buffer

    boolean active_flag;        /// first request already received?
    uint32 tx_cseq;             /// CSeq for next out message
    uint32 rx_cseq;             /// expected CSeq for next in message
    uint32 ses_id;              /// next session ID to use

    rtsp_session_t* session[RTSP_MAX_SESSIONS_PER_CONN];
    char session_group[RTSP_MAX_SESSION_GROUPS_PER_CONN][RTSP_MAX_TOKEN_LEN];
} rtsp_conn_t;


void skip_ws(char **ptr);
void rtsp_get_token(char **ptr, const char *delim, char **token, int *rc);

void rtsp_conn_init(rtsp_conn_t *conn);

char *rtsp_resp_txt(uint32 resp_code);
void rtsp_get_request(char **ptr, rtsp_msg_t *req, int *rc);
void rtsp_get_response(char **ptr, rtsp_msg_t *resp, int* rc);

int rtsp_put_request(char **ptr, rtsp_msg_t *req, char *method, char *uri);
void rtsp_msg_print(rtsp_msg_t *req);

int rtsp_put_response(char **ptr, rtsp_msg_t *rsp);

int rtsp_check_request_setup(rtsp_msg_t *req, rtsp_conn_t *conn);
int rtsp_check_request_teardown(rtsp_msg_t *req, rtsp_conn_t *conn);
int rtsp_check_request_get_parameter(rtsp_msg_t *req, rtsp_conn_t *conn);
int rtsp_check_request_set_parameter(rtsp_msg_t *req, rtsp_conn_t *conn);

void rtsp_parse_transport_spec(rtsp_trans_t *trans, int *rc);


int rtsp_get_peer_msg(rtsp_conn_t *conn, uint16 *msg_len);

char *rtsp_reason_txt(uint32 reason_code);
char *rtsp_clab_notice_txt(uint32 notice_code);


#endif  // __RTSP_H__
