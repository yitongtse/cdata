/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_MESSAGES_H__
#define __VIDEO_MESSAGES_H__

#include "vidman_defs.h"
#include "video_query.h"
//#include <tdllib/tdllib.h>
//#include <vdman_led_config/vdman_led_config.h>

#define FLOWS_PER_QAM                   (MAX_VIDEO_FLOWS_PER_QAM + 2)
#define MAX_AVG_VIDEO_FLOWS_PER_QAM     20
#define MAX_AVG_CAROUSEL_PER_QAM        4
#define MAX_AVG_CAROUSEL_INSERT_PER_QAM 4
#define MAX_CAROUSEL_TP_PER_LC          1024

#define MAX_VIDEO_PROGRAM_NUM           65535
  
#define VIDEO_MSG_NO_LC_ID              0xFFFFFFFF
#define RES_NOT_ALLOCED                 0xFFFFFFFF
                                        // to be depricated!

// Session id generated from SUP has the following format 
//   MSB 16 bits - slot number
//   LSB 16 bits - session ids in the range of MAX_SESSIONS
//
#define SESSION_ID_MASK    0x0000FFFF

// Video session states
//
// The following definitions are used for session state
// for in session and/or out session:
//
//   bit 0-1 (in session only): traffic state (INIT, OFF, ACTIVE, or IDLE)
//   bit 2: whether the session is used (1) or not (0)
//   bit 3: whether the session is enabled
//   bit 4: whether the PAT/PMT are ready
//          (snooped for in session, built for out session)
//   bit 5: whether the session is blocked (1) or not (0)
//          In session can be blocked if number of programs (other than NIT)
//          exceeds the maximum (1 for SPTS, ? for MPTS) expected.
//          Out session can be blocked if PID or program number collision
//          is detected.
//
typedef enum {
    SESSION_STATE_OFF = VIDMAN_SESSION_STATE_OFF,
    SESSION_STATE_ACTIVE = VIDMAN_SESSION_STATE_ACTIVE,
    SESSION_STATE_IDLE = VIDMAN_SESSION_STATE_IDLE,
    SESSION_STATE_INIT = VIDMAN_SESSION_STATE_INIT,
    SESSION_STATE_MAX
} video_session_traffic_stat_t;

#define SESSION_STATE_TRAFFIC_MASK      VIDMAN_SESSION_STATE_TRAFFIC_MASK

#define SESSION_STATE_USED              VIDMAN_SESSION_STATE_USED
#define SESSION_STATE_ENABLED           VIDMAN_SESSION_STATE_ENABLED
#define SESSION_STATE_PSI_READY         VIDMAN_SESSION_STATE_PSI_READY
#define SESSION_STATE_BLOCKED           VIDMAN_SESSION_STATE_BLOCKED
#define SESSION_STATE_NO_RESOURCES      VIDMAN_SESSION_STATE_NO_RESOURCES


// Video message types
//   All messages are from SUP to LC except otherwise stated
//
typedef enum {
    MSG_TYPE_VIDEO_ADD_SESSION = 100,
    MSG_TYPE_VIDEO_DELETE_SESSION,
    MSG_TYPE_VIDEO_DELETE_SESS_LIST,
    MSG_TYPE_VIDEO_UPDATE_PID_FILTER,
    MSG_TYPE_VIDEO_UPDATE_PROG_FILTER,
    MSG_TYPE_VIDEO_UPDATE_PID_REMAP,
    MSG_TYPE_VIDEO_UPDATE_PROG_REMAP,
    MSG_TYPE_VIDEO_ADD_CAROUSEL,
    MSG_TYPE_VIDEO_DELETE_CAROUSEL,
    MSG_TYPE_VIDEO_CONFIG_QAM,
    MSG_TYPE_VIDEO_DELETE_OWNER,

    MSG_TYPE_VIDEO_QUERY_STAT,
    MSG_TYPE_VIDEO_QUERY_IN_PAT,
    MSG_TYPE_VIDEO_QUERY_QAM_PAT,
    MSG_TYPE_VIDEO_QUERY_IN_PMT,
    MSG_TYPE_VIDEO_QUERY_OUT_PMT,
    MSG_TYPE_VIDEO_QUERY_SESS_LIST,
    MSG_TYPE_VIDEO_QUERY_CAROUSEL_INSERT,

    MSG_TYPE_VIDEO_CHANGE_SOURCE,

    MSG_TYPE_VIDEO_LIST_OUT_SESSION,
    MSG_TYPE_VIDEO_SESSION_SUMMARY,
    MSG_TYPE_VIDEO_MULTICAST_SESSION_RESTART,

    MSG_TYPE_VIDEO_CONTEXT_RESET,

    // The following message types are used only within LC processes
    MSG_TYPE_VIDEO_LCRED_MODE_ROLE,
    MSG_TYPE_VIDEO_LCRED_GO_HOT,
    MSG_TYPE_VIDEO_CTRL_T_QUE_EVNT,
    MSG_TYPE_VIDEO_LCRED_LC_JOINED_GROUP,
    MSG_TYPE_VIDEO_LCRED_LC_LEFT_GROUP,

    // New message types to support Analyzer
    MSG_TYPE_VIDEO_STAT_UPDATE = 900,
    MSG_TYPE_VIDEO_GET_PID_STAT,
    MSG_TYPE_VIDEO_GET_PSI_TABLE,
    MSG_TYPE_VIDEO_GET_PROG_STAT
}  video_msg_type;


typedef enum video_encryption_type_e_ {
    VIDEO_ENCRYPT_NONE = 0,
    VIDEO_ENCRYPT_PKEY,
} video_encryption_type_e;


// Video session state
typedef struct {
    uint32_t session_id;           // input session ID
    uint32_t old_state;            // current input session status
    uint32_t new_state;            // next input session status
    uint16_t session_port;         // Input session GigE Port Number
    uint32_t src_ip;               // source IP address
    uint32_t dst_ip;               // destination IP address
    uint16_t dst_udp;              // destination UDP port
    uint16_t src_udp;              // source UDP port
} video_session_state_t;


// Item in video PID or program list
#define VIDEO_LIST_INVALID_ITEM         0


// PID range
typedef struct {
    uint16_t first;
    uint16_t last;
} video_pid_range_t;


// Common video message header for command, query and response 
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
} video_msg_t;


// Input session configuration
//
typedef struct {
    uint8_t  used_flag      : 1;  // whether the session is in use
    uint8_t  enable_flag    : 1;  // whether the session is enabled
    uint8_t  pid_remap_flag : 1;  // whether to remap pid
    uint8_t  parse_psi_flag : 1;  // whether to parse PSI
    uint8_t  dejitter_flag  : 1;  // whether to perform dejittering
    uint8_t  cbr_flag       : 1;  // whether session is constant bitrate
    uint8_t  ms_cr_flag     : 1;  // use master/slave clock recovery mode
                                  // (CBR only)
    uint8_t  mpts_flag      : 1;  // whether to accept MPTS

    uint8_t  owner_id;            // owner ID

    uint8_t  input_type;          // input type (unicast or multicast)
    uint8_t  input_port;          // input GE port
    uint8_t  ip_ver;              // IP version 
    uint16_t dst_udp;             // destination UDP port
    uint16_t src_udp;             // source UDP port
    uint32_t dst_ip[4];           // destination IP address for IPv6
                                  //   dst_ip[0] used for IPv4
    uint32_t src_ip[4];           // source IP address for IPv6
                                  //   src_ip[0] used for IPv4
    uint32_t bitrate_alloc;       // allocated bitrate
    uint16_t jitter_size;         // expected jitter size (in ms)
    uint16_t delay_target;        // target delay (in ms)
    uint16_t master_pcr_pid;      // preferred master PCR pid

    uint16_t init_thres;          // init threashold (in ms)
    uint16_t idle_thres;          // idle threashold (in ms)
    uint16_t off_timer;           // off timer (in sec)
    int32_t ref_cnt;              // number of out sessions referencing this
} in_session_config_t;


// Output session configuration
//
#define VIDEO_PID_LIST_SIZE     VIDEO_MAX_PID_FILTERS
#define VIDEO_PROG_LIST_SIZE    VIDEO_MAX_PROG_FILTERS

typedef struct {
    uint8_t  used_flag      : 1;  // whether the session is in use
    uint8_t  enable_flag    : 1;  // whether the session is enabled
    uint8_t  pid_remap_flag : 1;  // whether to remap pid
    uint8_t  parse_psi_flag : 1;  // whether to parse PSI
    uint8_t  dejitter_flag  : 1;  // whether to perform dejittering
    uint8_t  cbr_flag       : 1;  // whether session is constant bitrate
    uint8_t  mpts_flag      : 1;  // whether to accept MPTS
    uint8_t  encrypt_flag   : 1;  // whether the session is to be encrypted

    uint8_t  pending_flag   : 1;  // whether the session has pending config

    uint8_t  owner_id;            // owner ID

    uint8_t  flow_id;             // flow ID
    uint16_t qam_id;              // output qam channel ID
    uint16_t in_ses_id;           // corresponding input session ID
    uint32_t client_id;           // unique session id from client (VCP) 

    uint8_t pid_cnt;
    uint8_t prog_cnt;
    uint16_t in_pid_list[VIDEO_PID_LIST_SIZE];
    uint16_t out_pid_list[VIDEO_PID_LIST_SIZE];
    uint16_t in_prog_list[VIDEO_PROG_LIST_SIZE];
    uint16_t out_prog_list[VIDEO_PROG_LIST_SIZE];
    video_pid_range_t pid_range[VIDEO_PROG_LIST_SIZE];
} out_session_config_t;


// QAM video configuration
//
typedef struct {
    uint8_t enable_flag : 1;      // whether the qam is enabled
    uint8_t encrypt_flag : 1;     // whether the qam is encrypted
    uint8_t owner_id;             // owner ID
    uint16_t onid;                // original network ID
    uint16_t tsid;                // transport stream ID
    uint8_t flow_cnt;             // number of flows used in qam
    uint8_t crsl_cnt;             // number of carousels in qam
    uint32_t psi_period;          // PSI insert period (in ms)
} qam_config_t;


// Video input statistics
typedef vidman_video_in_stat_t video_in_stat_t;

// Video output statistics
typedef vidman_video_out_stat_t video_out_stat_t;

// Video QAM statistics
typedef vidman_video_qam_stat_t video_qam_stat_t;

// Overall video statistics
typedef vidman_video_stat_summary_t video_stat_summary_t;

// PSI info (PAT or PAT)
typedef vidman_psi_info_t video_psi_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t start_resource_id;
} video_list_cmd;

typedef struct {
    uint32_t ses_id;              // (output) session ID
    uint16_t session_state;        // session state
    uint16_t program_cnt;          // number of programs in the session
    uint32_t measured_bitrate;    // measured bitrate in bits per second
} sess_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t status;
    sess_info_t info[0];
} video_list_resp;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t status;
    video_stat_summary_t info;
} video_list_summary_resp;


// operation type used for PID_FILTER, PROGRAM_FILTER, PID_MAP and PROGRAM_MAP
// The types are defined in vdman_led_config.tdl
//
enum {
    VIDEO_LIST_OPER_ADD = 1,
    VIDEO_LIST_OPER_DELETE,
    VIDEO_LIST_OPER_CLEAR
};


typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t input_type;
    uint8_t  ip_addr_len;
    uint32_t src_ip_addr[4];
    uint32_t dst_ip_addr[4];
    uint16_t src_udp_port;
    uint16_t dst_udp_port;
    uint32_t stream_type;
    uint32_t pid_remap;
    uint32_t parse_psi;
    uint32_t dejitter;
    uint32_t jitter_size;
    uint32_t delay_target;
    uint32_t cbr;
    uint32_t bitrate_alloc;
    uint32_t idle_threshold;
    uint32_t init_threshold;
    uint32_t off_timer;
    uint32_t enable;
    uint32_t qam_id;
    uint32_t prog_num;
    uint32_t encrypt_flag;
    video_pid_range_t res_pid;
} video_add_session_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
} video_delete_session_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t num_of_sessions;
    uint32_t session_id_list[0];
} video_delete_session_list_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
} video_delete_owner_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t input_type;
    uint32_t input_port;
    uint32_t src_ip_addr[4];
    uint32_t src_ip_addr_new[4];
    uint32_t dst_ip_addr[4];
    uint32_t dst_ip_addr_new[4];
    uint32_t init_threshold;
} video_change_source_msg_t;

// QAM configuration field enum
typedef enum {
    qam_cfg_tsid,
    qam_cfg_onid,
    qam_cfg_psi_period,
    qam_cfg_encrypt,
    qam_cfg_enable
} qam_cfg_enum_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint8_t owner_id;
    uint32_t resource_id;
    uint32_t field_mask;
    uint32_t tsid;
    uint32_t onid;
    uint32_t psi_period;
    uint32_t encrypt_flag;
    uint32_t enable;
} video_config_qam_msg_t;


// This message struct is shared for the following message types:
//   - video_update_pid_filter_msg_t
//   - video_update_prog_filter_msg_t
//   - video_update_pid_map_msg_t
//
typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t enable;
    uint32_t encrypt_flag;
    uint16_t oper;
    uint16_t item_count;
    uint16_t list[0];
} video_update_output_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t enable;
    uint32_t encrypt_flag;
} video_update_out_config_msg_t;

// Message for program remap add/delete/cleanup
typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint16_t oper;
    uint16_t item_count;
    uint16_t in_prog_list[VIDEO_PROG_LIST_SIZE];
    uint16_t out_prog_list[VIDEO_PROG_LIST_SIZE];
    video_pid_range_t pid_range[VIDEO_PROG_LIST_SIZE];
} video_update_remap_prog_msg_t;

// Message for pid remap add/delete/cleanup
typedef struct {
    uint16_t primary_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint16_t oper;
    uint16_t item_count;
    uint16_t in_pid_list[VIDEO_PID_LIST_SIZE];
    uint16_t out_pid_list[VIDEO_PID_LIST_SIZE];
} video_update_remap_pid_msg_t;


typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t session_id[50];

} video_query_snmp_input_ts_info_msg_t;	//video_query_snmp_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t input_ts_index[50];
    uint32_t input_prog_index[50];
} video_query_snmp_input_prog_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t input_ts_index[50];
    uint32_t input_prog_index[50];
    uint32_t input_es_index;
} video_query_snmp_prog_es_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t input_ts_index[50];
} video_query_snmp_input_stats_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t input_ts_index[50];
} video_query_snmp_input_udp_org_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
} video_query_snmp_output_stats_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
} video_query_snmp_output_ts_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t output_prog_index[50];
} video_query_snmp_output_prog_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t output_prog_index[50];
    uint32_t output_es_index;
} video_query_snmp_output_prog_stats_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
} video_query_snmp_output_udp_dest_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
} video_query_snmp_video_sess_info_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
    int32_t  arg;
    uint16_t no_of_sessions;
    uint32_t session_id[50];
    uint32_t input_ts_index[50];
    uint32_t input_prog[50];
    uint32_t output_prog[50];
    uint32_t input_ts_conn_type;
    uint32_t output_ts_conn_type;
} video_query_snmp_video_sess_ptr_msg_t;

typedef vidman_snmp_input_ts_info_t video_query_snmp_input_ts_info_resp_t;
typedef vidman_snmp_input_prog_info_t video_query_snmp_input_prog_info_resp_t;
typedef vidman_snmp_prog_es_info_t video_query_snmp_prog_es_info_resp_t;
typedef vidman_snmp_input_stats_info_t video_query_snmp_input_stats_info_resp_t;
typedef vidman_snmp_input_udp_org_info_t video_query_snmp_input_udp_org_info_resp_t;
typedef vidman_snmp_output_stats_info_t video_query_snmp_output_stats_info_resp_t;
typedef vidman_snmp_output_ts_info_t video_query_snmp_output_ts_info_resp_t;
typedef vidman_snmp_output_prog_info_t video_query_snmp_output_prog_info_resp_t;
typedef vidman_snmp_output_prog_stats_info_t video_query_snmp_output_prog_stats_info_resp_t;
typedef vidman_snmp_output_udp_dest_info_t video_query_snmp_output_udp_dest_info_resp_t;
typedef vidman_snmp_video_sess_info_t video_query_snmp_video_sess_info_resp_t;
typedef vidman_snmp_video_sess_ptr_t video_query_snmp_video_sess_ptr_resp_t;

// Video statistic query message
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;     /* session id */
} video_query_stat_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;      /* session id */
    uint32_t status;
    video_in_stat_t in_stat;   /* in_stat */
    video_out_stat_t out_stat; /* out_stat */
    video_stat_summary_t stats_summary; /* overall stats summary */
                                        // TODO: where is the summary set??
} video_query_stat_resp_t;


// Video PAT query message
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;   // session id
} video_query_pat_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;   // session id
    uint32_t status;
    uint16_t total_section;     /* total number of sections in this session */
    uint16_t num_section;       /* number of sections in this response */
    uint16_t remaining_section; /* remaining # of sections for this query */
    uint16_t size;
    video_psi_info_t section[0];      /* buffer for PAT */
} video_query_pat_resp_t;


// Video PMT query message
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;   // session id
    uint32_t prog_idx;      // index of first program to query
} video_query_pmt_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t prog_idx;          // index of first program to query
    uint32_t status;
    uint16_t total_section;     /* total number of sections in this session */
    uint16_t num_section;       /* number of sections in this response */
    uint16_t remaining_section; /* remaining # of sections for this query */
    uint16_t size;
    video_psi_info_t section[0];      /* buffer for PMT */
} video_query_pmt_resp_t;

// Video PID Map query message
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;
} video_query_pid_map_msg_t;

// PID Map
typedef vidman_video_pid_map_t video_pid_map_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;
    uint32_t status;
    video_pid_map_t pid_map;
} video_query_pid_map_resp_t;


// NEW messages for packet insertion support
//
#define MAX_CRSL_NUM_TP         8
#define MAX_CRSL_INSERT         32


// Use to set up carousel
//
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;               // carousel resource ID
    int32_t  insert_count;              // -1 means continuous insersion
    int32_t  insert_period;             // in ms
    uint16_t num_tp;                    // must be <= MAX_CRSL_NUM_TP
    uint16_t num_qam;                   // must be <= MAX_CRSL_INSERT
    uint16_t qam_id[MAX_CRSL_INSERT];
    uint8_t  tp_data[0];                // (num_tp * TP_SIZE) bytes of data
} video_add_crsl_msg_t;

// Use to delete carousel from selected QAMs
// Carousl will also be deallocated if all its insertions have been deleted.
//
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;               // carousel resource ID
    uint16_t num_qam;                   // must be <= MAX_CRSL_INSERT
    uint16_t qam_id[0];
} video_delete_crsl_msg_t;

typedef struct {
    uint32_t resource_id;               // carousel resource ID
    uint16_t qam_id;
} crsl_insert_query_t;

typedef enum {
    CRSL_INSERT_ACTIVE = 1,
    CRSL_INSERT_INACTIVE,
    CRSL_INSERT_NOT_FOUND,
    CRSL_NOT_FOUND
} crsl_insert_state_t;

typedef struct {
    uint32_t resource_id;               // carousel resource ID
    uint32_t qam_id;
    crsl_insert_state_t state;
    int32_t insert_cnt;                 // no of time carousel has been inserted
} crsl_insert_resp_t;

// Used to query info of specified carousel ID
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t num_item;                  // must be <= MAX_CRSL_INSERT
    crsl_insert_query_t item[0];
} video_query_crsl_insert_t;

// Query response of given carousel ID
typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t status;                    // EOK, ENOTSUP, EINVAL
    uint16_t num_item;
    crsl_insert_resp_t item[0];
} video_reply_crsl_insert_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t resource_id;
} video_query_session_list_msg_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint32_t total_msg;
    uint32_t msg_num;
    uint32_t num_sessions;
    uint32_t sess_array[0];
} video_query_session_list_resp_t;

#endif  /* __VIDEO_MESSAGES_H__ */
