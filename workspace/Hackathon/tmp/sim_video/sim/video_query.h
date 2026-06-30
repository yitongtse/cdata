/*-----------------------------------------------------------------------------
 * video_query.h - Video Query definitions
 *
 * February 2014
 *
 * Copyright (c) 2014-2015 by cisco Systems, Inc.
 * All rights reserved.
 *-----------------------------------------------------------------------------
 */
#ifndef __VIDEO_QUERY_H__
#define __VIDEO_QUERY_H__

#include "vidman_defs.h"

#define MSG_BUF_SIZE            2048
#define RSP_BUF_SIZE            4096

#define SNMP_GQI_SESSION_ID_LEN (22)

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
#define VIDMAN_SESSION_STATE_OFF               0x0
#define VIDMAN_SESSION_STATE_ACTIVE            0x1
#define VIDMAN_SESSION_STATE_IDLE              0x2
#define VIDMAN_SESSION_STATE_INIT              0x3

#define VIDMAN_SESSION_STATE_TRAFFIC_MASK      0x3

#define VIDMAN_SESSION_STATE_USED              0x4
#define VIDMAN_SESSION_STATE_ENABLED           0x8
#define VIDMAN_SESSION_STATE_PSI_READY         0x10
#define VIDMAN_SESSION_STATE_BLOCKED           0x20
#define VIDMAN_SESSION_STATE_NO_RESOURCES      0x40

// Video input statistics
//
typedef struct {
    uint8_t session_state;        // session state
    uint32_t session_id;          // session id
    uint32_t ip_cnt;              // input IPs
    uint32_t drop_ip_cnt;         // IPs dropped
    uint32_t tp_cnt;              // input TPs
    uint32_t rtp_cnt;             // input RTPs
    uint32_t null_tp_cnt;         // NULL TPs
    uint32_t unref_tp_cnt;        // unreferenced TPs
    uint32_t psi_tp_cnt;          // PSI TPs
    uint32_t pcr_tp_cnt;          // PCR TPs
    uint32_t non_pcr_tp_cnt;      // TPs not carrying PCR
    uint32_t sync_loss_cnt;       // TPs with sync loss
    uint32_t disc_cnt;            // discontinuity points
    uint32_t tb_disc_cnt;         // time base discontinuity points
    uint32_t cc_err_cnt;          // TPs with CC error
    uint32_t block_tp_cnt;        // NEW: TP dropped because session is blocked
    uint32_t new_pat_cnt;         // new PATs
    uint32_t new_pmt_cnt;         // new PMTs
    uint32_t pcr_jump_cnt;        // bad PCR jumps
    uint32_t pcr_xover_cnt;       // PCR crossovers
    uint32_t overflow_cnt;        // dejitter buffer overflows
    uint32_t underflow_cnt;       // dejitter buffer underflows
    uint32_t idle_cnt;            // number of idles detected
    uint32_t idle_time;           // total idle time in ms
    uint32_t measured_bitrate;    // measured bitrate
    uint32_t measured_min_bitrate;// min measured bitrate
    uint32_t measured_max_bitrate;// max measured bitrate
    uint32_t pcr_bitrate;         // pcr bitrate
    uint32_t pcr_min_bitrate;     // min pcr bitrate
    uint32_t pcr_max_bitrate;     // max pcr bitrate
    uint32_t stay_time;           // stay time (in ms)
    uint32_t jitter;              // measured jitter (in ms)
    uint64_t start_time;          // session start time
} vidman_video_in_stat_t;


// Video output statistics
//
typedef struct {
    uint8_t session_state;        // session state
    uint32_t tp_cnt;              // TPs processed from input
    uint32_t forward_tp_cnt;      // TPs forwarded to output
    uint32_t insert_tp_cnt;       // TPs inserted to output
    uint32_t psi_tp_cnt;          // PSI TPs
    uint32_t pcr_tp_cnt;          // PCR TPs
    uint32_t non_pcr_tp_cnt;      // non-PCR TPs
    uint32_t new_pat_cnt;         // new PATs from input
    uint32_t new_pmt_cnt;         // new PMTs from input
    uint32_t drop_tp_cnt;         // TPs dropped at output
    uint32_t block_tp_cnt;        // Blocked TPs dropped at output
    uint32_t overdue_tp_cnt;      // Overdue TPs dropped at output
    uint32_t info_overrun_cnt;    // TP info overruns
    uint32_t info_err_cnt;        // info errors
    uint32_t invalid_rate_cnt;    // PCR infos with invalid bitrate
    uint32_t overflow_cnt;        // dejitter buffer overflows
    uint32_t underflow_cnt;       // dejitter buffer underflows
    uint32_t measured_bitrate;    // measured bitrate
    uint64_t start_time;          // session start time
    uint32_t cmdbuf_maxhit_cnt;   // command buffer maxhit
    uint8_t  ca_desc_present;     // ca desc status on output pmt
} vidman_video_out_stat_t;


// Video QAM statistics
//
typedef struct {
    uint64_t mux_tp_cnt;          // TPs processed in mux
    uint64_t drop_tp_cnt;         // TPs dropped in mux
    uint32_t psi_tp_cnt;          // PAT TPs inserted
    uint32_t ecm_tp_cnt;          // ECM TPs inserted
    uint32_t crsl_tp_cnt;         // TPs from carousel inserted
    uint64_t bitrate;             // QAM averaged output bitrate
} vidman_video_qam_stat_t;


// Overall video statistics
//
typedef struct {
    uint16_t enable_cnt;          // number of sessions enabled
    uint16_t block_cnt;           // number of sessions blocked
    uint16_t active_cnt;          // number of active sessions
    uint16_t idle_cnt;            // number of idle sessions
    uint16_t init_cnt;            // number of init sessions
    uint16_t off_cnt;             // number of off sessions
    uint16_t psi_rdy_cnt;         // number of psi rdy sessions
    uint16_t asm_cnt;             // number of asm sessions
    uint16_t ssm_cnt;             // number of ssm sessions
    uint16_t udp_cnt;             // number of udp sessions
    uint16_t mux_cnt;             // number of mux sessions
    uint16_t passthru_cnt;        // number of passthru sessions
    uint16_t data_cnt;            // number of data sessions
    uint32_t measured_bitrate;    // measured birate
} vidman_video_stat_summary_t;

// PSI info (PAT or PAT)
//
typedef struct {
    uint16_t flags;
    uint16_t section_cnt;
    uint32_t psi_section[0];
} vidman_psi_info_t;

// PID Map
//
typedef struct {
    uint16_t in_pid;
    uint16_t out_pid;
} vidman_pid_map_t;

typedef struct {
    uint16_t num_pid;
    vidman_pid_map_t map[VIDEO_MAX_PID_MAP];
} vidman_video_pid_map_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint64_t item_value[50];
} vidman_snmp_input_ts_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t prog_index[50];
    uint16_t item_value_len;
    uint8_t  *item_value;
} vidman_snmp_input_prog_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t prog_index[50];
    uint32_t es_index[50];
    uint16_t item_value_len;
    uint8_t  *item_value;
} vidman_snmp_prog_es_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t item_value[50];
} vidman_snmp_input_stats_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t item_value[50];
} vidman_snmp_input_udp_org_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t insert_pkt_index[50];
    uint64_t item_value[50];
} vidman_snmp_insert_pkt_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint64_t item_value[50];
} vidman_snmp_output_stats_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint64_t item_value[50];
} vidman_snmp_output_ts_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint32_t output_prog_index[50];
    uint16_t item_value_len;
    uint8_t  *item_value;
} vidman_snmp_output_prog_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint32_t output_prog_index[50];
    uint32_t output_es_index[50];
    uint32_t item_value[50];
} vidman_snmp_output_prog_stats_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint32_t item_value[50];
} vidman_snmp_output_udp_dest_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint32_t item_value[50];
 } vidman_snmp_prog_map_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t output_ts_index[50];
    uint16_t item_value_len;
    uint8_t  *item_value;
} vidman_snmp_video_sess_info_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t output_ts_index[50];
    uint32_t input_prog_index[50];
    uint32_t output_prog_index[50];
    int32_t  item_value[50];
} vidman_snmp_video_sess_ptr_t;

typedef struct {
    uint16_t primary_id;
    uint32_t owner_id;
    uint32_t transaction_id;
    uint16_t no_of_sessions;
    uint32_t input_ts_index[50];
    uint32_t output_ts_index[50];
    uint64_t item_value[50];
} vidman_snmp_output_sess_info_t;

#endif /* __VIDEO_QUERY_H__ */
