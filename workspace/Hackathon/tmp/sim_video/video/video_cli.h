/*
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_CLI_H__
#define __VIDEO_CLI_H__


struct cli_control_block_;


// TODO: show move PD function IDs to the platform side!

// Video CLI function ID
//
enum {
    CLI_VIDEO_SHOW_VER = 1,
    CLI_VIDEO_SHOW_BUFFER,
    CLI_VIDEO_SHOW_CONTEXTS,
    CLI_VIDEO_SHOW_LCRED,
    CLI_VIDEO_SHOW_CR,
    CLI_VIDEO_SHOW_CAPTURE,
    CLI_VIDEO_SHOW_CAPTURE_OUTPUT,

    CLI_VIDEO_SHOW_CONFIG_MAP_SESSION_ID,
    CLI_VIDEO_SHOW_CONFIG_MAP_CRSL_ID,
    CLI_VIDEO_SHOW_CONFIG_MAP_SESSION,
    CLI_VIDEO_SHOW_CONFIG_MAP_QAM,
    CLI_VIDEO_SHOW_CONFIG_SESSIONS,
    CLI_VIDEO_SHOW_CONFIG_SESSION,
    CLI_VIDEO_SHOW_CONFIG_IN_SESSIONS,
    CLI_VIDEO_SHOW_CONFIG_OUT_SESSIONS,
    CLI_VIDEO_SHOW_CONFIG_QAMS,
    CLI_VIDEO_SHOW_CONFIG_CRSLS,
    CLI_VIDEO_SHOW_CONFIG_CRSL,
    CLI_VIDEO_SHOW_CONFIG_OWNER,

    CLI_VIDEO_SHOW_MAP_SESSION_ID,
    CLI_VIDEO_SHOW_MAP_CRSL_ID,
    CLI_VIDEO_SHOW_MAP_SESSION_GROUPS,
    CLI_VIDEO_SHOW_MAP_SESSION_GROUP,
    CLI_VIDEO_SHOW_MAP_QAM_GROUPS,
    CLI_VIDEO_SHOW_MAP_QAM_GROUP,
    CLI_VIDEO_SHOW_MAP_SESSION,
    CLI_VIDEO_SHOW_MAP_SESSION_SUMMARY,
    CLI_VIDEO_SHOW_MAP_QAM,

    CLI_VIDEO_SHOW_IN_SESSIONS,
    CLI_VIDEO_SHOW_IN_SESSIONS_ACTIVE,
    CLI_VIDEO_SHOW_IN_SESSIONS_IDLE,
    CLI_VIDEO_SHOW_IN_SESSIONS_OFF,
    CLI_VIDEO_SHOW_IN_SESSIONS_SUMMARY,
    CLI_VIDEO_SHOW_IN_SESSION,

    CLI_VIDEO_SHOW_OUT_SESSIONS,
    CLI_VIDEO_SHOW_OUT_SESSIONS_SUMMARY,
    CLI_VIDEO_SHOW_OUT_SESSION,
    CLI_VIDEO_SHOW_OUT_SESSION_CA_INFO,

    CLI_VIDEO_SHOW_SESSIONS,
    CLI_VIDEO_SHOW_SESSION_SUMMARY,
    CLI_VIDEO_SHOW_SESSION_RANGE,
    CLI_VIDEO_SHOW_SESSION,
    CLI_VIDEO_SHOW_SESSION_IN_PCR,
    CLI_VIDEO_SHOW_SESSION_IN_PSI,
    CLI_VIDEO_SHOW_SESSION_IN_PID,
    CLI_VIDEO_SHOW_SESSION_OUT_PSI,
    CLI_VIDEO_SHOW_SESSION_OUT_PID,

    CLI_VIDEO_SHOW_CRSLS,
    CLI_VIDEO_SHOW_CRSL,

    CLI_VIDEO_SHOW_QAMS,
    CLI_VIDEO_SHOW_QAM_RANGE,
    CLI_VIDEO_SHOW_QAM_SUMMARY,
    CLI_VIDEO_SHOW_QAM,
    CLI_VIDEO_SHOW_QAM_CRSL,
    CLI_VIDEO_SHOW_QAM_FLOWS,
    CLI_VIDEO_SHOW_QAM_PID_MAP,
    CLI_VIDEO_SHOW_QAM_PID,
    CLI_VIDEO_SHOW_QAM_PROG,
    CLI_VIDEO_SHOW_QAM_PROG_MAP,
    CLI_VIDEO_SHOW_QAM_PROG_PMT,
    CLI_VIDEO_SHOW_QAM_PROG_PMT_PKT,
    CLI_VIDEO_SHOW_QAM_PROG_PRIV,
    CLI_VIDEO_SHOW_QAM_PROG_PRIV_PKT,
    CLI_VIDEO_SHOW_QAM_PAT,
    CLI_VIDEO_SHOW_QAM_PAT_PKT,
    CLI_VIDEO_SHOW_ERROR,
//    CLI_VIDEO_SHOW_MONITOR_IN,
    CLI_VIDEO_SHOW_RID_SESSION,

    CLI_VIDEO_SHOW_OWNER,

    CLI_VIDEO_SHOW_STAT,
    CLI_VIDEO_SHOW_STAT_DELTA,
    CLI_VIDEO_SHOW_STAT_MESSAGE,
    CLI_VIDEO_SHOW_STAT_THREAD,

    CLI_VIDEO_SHOW_PERF,
    CLI_VIDEO_SHOW_PERF_QAMS,
    CLI_VIDEO_SHOW_PERF_QAM,
    CLI_VIDEO_SHOW_PERF_SESSIONS,
    CLI_VIDEO_SHOW_PERF_SESSION,

    CLI_VIDEO_SHOW_JIBDQ,
    CLI_VIDEO_SHOW_JIBDQ_STATE,
    CLI_VIDEO_SHOW_JIBDQ_QAM,
    CLI_VIDEO_SHOW_JIBDQ_ERR,
    CLI_VIDEO_SHOW_JIBDQ_ERR_SUM,

    CLI_VIDEO_SHOW_QUE_DEPTH,

    CLI_VIDEO_CLEAR_STAT = 101,
    CLI_VIDEO_CLEAR_STAT_DELTA,
    CLI_VIDEO_CLEAR_STAT_MESSAGE,
    CLI_VIDEO_CLEAR_STAT_THREAD,

    CLI_VIDEO_CLEAR_JIBDQ_ALL,
    CLI_VIDEO_CLEAR_JIBDQ_RNG,

    CLI_VIDEO_CAPTURE_OFF = 201,
    CLI_VIDEO_CAPTURE_ANY,
    CLI_VIDEO_CAPTURE_DROP,
    CLI_VIDEO_CAPTURE_SYNCLOSS,
    CLI_VIDEO_CAPTURE_CCERR,
    CLI_VIDEO_CAPTURE_OUTPUT_QAM,

    CLI_VIDEO_CR_PLL = 301,
    CLI_VIDEO_CR_LIMIT,
    CLI_VIDEO_CR_REVIEW,
    CLI_VIDEO_CR_RECORD_SET,
    CLI_VIDEO_CR_RECORD_START,
    CLI_VIDEO_CR_RECORD_STOP,
    CLI_VIDEO_CR_RECORD_STATUS,
    CLI_VIDEO_CR_RESTART,

    CLI_VIDEO_PSI = 320,
    CLI_VIDEO_PSI_RECORD_SET,
    CLI_VIDEO_PSI_RECORD_START,
    CLI_VIDEO_PSI_RECORD_STATUS,

    CLI_VIDEO_SET_IN_IDLE = 401,
    CLI_VIDEO_SET_IN_JITTER,
    CLI_VIDEO_SET_IN_MASTER_PCR_PID,

//    CLI_VIDEO_SET_MONITOR_IN,
//    CLI_VIDEO_SET_MONITOR_IN_OFF,
    CLI_VIDEO_SET_MONITOR_SRC,
    CLI_VIDEO_SET_MONITOR_SRC_OFF,
    CLI_VIDEO_SET_MONITOR_QAM,
    CLI_VIDEO_SET_MONITOR_QAM_OFF,
    CLI_VIDEO_SET_MONITOR_TIMING,
    CLI_VIDEO_SET_MONITOR_TIMING_OFF,

    CLI_VIDEO_SET_QAM_DIAG_ON,
    CLI_VIDEO_SET_QAM_DIAG_OFF,

    CLI_VIDEO_SET_DISC_INSERT_ENABLE,
    CLI_VIDEO_SET_DISC_INSERT_ENABLE_OFF,

    CLI_VIDEO_DEBUG_MSG = 501,
    CLI_VIDEO_DEBUG_MSG_OFF,
    CLI_VIDEO_DEBUG_EVENT,
    CLI_VIDEO_DEBUG_EVENT_OFF,
    CLI_VIDEO_DEBUG_IN,
    CLI_VIDEO_DEBUG_IN_OFF,
    CLI_VIDEO_DEBUG_OUT,
    CLI_VIDEO_DEBUG_OUT_OFF,
    CLI_VIDEO_DEBUG_PSI,
    CLI_VIDEO_DEBUG_PSI_OFF,
    CLI_VIDEO_DEBUG_CLKREC,
    CLI_VIDEO_DEBUG_CLKREC_OFF,
    CLI_VIDEO_DEBUG_THREAD,
    CLI_VIDEO_DEBUG_THREAD_OFF,
    CLI_VIDEO_DEBUG_CA,
    CLI_VIDEO_DEBUG_CA_OFF,
    CLI_VIDEO_DEBUG_ELCARO_IN_CHECK,
    CLI_VIDEO_DEBUG_ELCARO_OUT_CHECK,
    CLI_VIDEO_DEBUG_ELCARO_QAM_CHECK,
    CLI_VIDEO_DEBUG_ELCARO_CRSL_CHECK,
    CLI_VIDEO_DEBUG_DEMUX,
    CLI_VIDEO_DEBUG_DEMUX_OFF,

    CLI_VIDEO_DISABLE_INPUT = 601,
    CLI_VIDEO_TERMINATE_SEGV,
    CLI_VIDEO_TERMINATE_FPE,
    CLI_VIDEO_TERMINATE_ABRT,
    CLI_VIDEO_TERMINATE_TERM,
    CLI_VIDEO_EXPORT_CONFIG,

    CLI_VIDEO_CA_INIT = 701,
    CLI_VIDEO_CA_SHOW_ECM,
    CLI_VIDEO_CA_SHOW_IPC,

    CLI_VIDEO_GET_PID_STAT = 801,

    CLI_VIDEO_RESET = 999
};


int cli_video_init(void);
int cli_video_config(struct cli_control_block_ *ccb);
int cli_video_show_config(struct cli_control_block_ *ccb);
int cli_video_show_list(struct cli_control_block_ *ccb);
int cli_video_show_summary(struct cli_control_block_ *ccb);
int cli_video_set_qam_enable(struct cli_control_block_ *ccb);
int cli_video_set_qam_encrypt(struct cli_control_block_ *ccb);
int cli_video_set_out_sess_enable(struct cli_control_block_ *ccb);
int cli_video_set_out_sess_encrypt(struct cli_control_block_ *ccb);
int cli_video_set_qam_tsid_onid(struct cli_control_block_ *ccb);
int cli_video_add_crsl(struct cli_control_block_ *ccb);
int cli_video_add_crsl_insert(struct cli_control_block_ *ccb);
int cli_video_change_in_session(struct cli_control_block_ *ccb);
int cli_video_delete_session(struct cli_control_block_ *ccb);
int cli_video_delete_crsl(struct cli_control_block_ *ccb);
int cli_video_delete_crsl_insert(struct cli_control_block_ *ccb);

int cli_video_query(struct cli_control_block_ *ccb);
int cli_video_query_in_session(struct cli_control_block_ *ccb);
int cli_video_query_in_session_all(struct cli_control_block_ *ccb);
int cli_video_query_out_session(struct cli_control_block_ *ccb);
int cli_video_query_out_session_all(struct cli_control_block_ *ccb);
int cli_video_query_qam(struct cli_control_block_ *ccb);
int cli_video_query_qam_all(struct cli_control_block_ *ccb);
int cli_video_query_crsl(struct cli_control_block_ *ccb);

int cli_video_lcred_mode_role(struct cli_control_block_ *ccb);
int cli_video_lcred_go_hot(struct cli_control_block_ *ccb);
int cli_video_lcred_show_primary(struct cli_control_block_ *ccb);
int cli_video_lcred_set_primary(struct cli_control_block_ *ccb);

int cli_video_clear_context(struct cli_control_block_ *ccb);

int cli_video_log(struct cli_control_block_ *ccb);

int cli_video_ca_update_scg(struct cli_control_block_ *ccb);

int cli_video_ca_add_desc(struct cli_control_block_ *ccb);
int cli_video_ca_delete_desc(struct cli_control_block_ *ccb);

int cli_dump(struct cli_control_block_ *ccb);
int cli_exit(struct cli_control_block_ *ccb); 

#endif // __VIDEO_CLI_H__

