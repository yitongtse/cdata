///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#ifndef __SIM_CLI_VIDEO_H__
#define __SIM_CLI_VIDEO_H__


#define SIM_CLI_OWNER_ID                0


extern int video_msg_fd;                // socket to video message server


enum {
    CLI_VIDEO_OFF,
    CLI_VIDEO_ON
};

enum {
    CLI_VIDEO_QUERY_CONFIG,
    CLI_VIDEO_QUERY_PAT,
    CLI_VIDEO_QUERY_PMT,
    CLI_VIDEO_QUERY_STAT
};

enum {
    CLI_VIDEO_LCRED_NON_REDUNDANT,
    CLI_VIDEO_LCRED_PRIMARY_ACTIVE,
    CLI_VIDEO_LCRED_PRIMARY_STANDBY,
    CLI_VIDEO_LCRED_SECONDARY_ACTIVE,
    CLI_VIDEO_LCRED_SECONDARY_STANDBY
};

enum {
    CLI_VIDEO_LOG_SHOW,
    CLI_VIDEO_LOG_SHOW_ALL,
    CLI_VIDEO_LOG_LOCK,
    CLI_VIDEO_LOG_UNLOCK,
    CLI_VIDEO_LOG_RESET
};

enum {
    CLI_VIDEO_CA_SCG_ADD,
    CLI_VIDEO_CA_SCG_ADD_DELAY,
    CLI_VIDEO_CA_SCG_DELETE,
    CLI_VIDEO_CA_SCG_DELETE_DELAY,
    CLI_VIDEO_CA_DESC_PID,
    CLI_VIDEO_CA_DESC_PROG,
    CLI_VIDEO_CA_DESC_ALL_ES,
    CLI_VIDEO_CA_SID_ECM_PID,
    CLI_VIDEO_CA_RELEASE_PID,
    CLI_VIDEO_CA_ECM_PLAY_START,
    CLI_VIDEO_CA_ECM_PLAY_STOP,
    CLI_VIDEO_CA_SET_CWINDEX
};



// fooid for cli_video_add_session():
//   Bit 0-1: cast_type
//   Bit 2-3: process_type
//   Bit 4: pid range flag

#define CLI_VIDEO_CAST_TYPE_UDP         INPUT_TYPE_UNICAST
#define CLI_VIDEO_CAST_TYPE_ASM         INPUT_TYPE_MULTICAST_ASM
#define CLI_VIDEO_CAST_TYPE_SSM         INPUT_TYPE_MULTICAST_SSM

#define CLI_VIDEO_PROCESS_TYPE_REMAP    (PROCESS_TYPE_REMAP << 2)
#define CLI_VIDEO_PROCESS_TYPE_REMUX    (PROCESS_TYPE_REMUX << 2)
#define CLI_VIDEO_PROCESS_TYPE_PASSTHRU (PROCESS_TYPE_PASSTHRU << 2)
#define CLI_VIDEO_PROCESS_TYPE_DATA     (PROCESS_TYPE_DATA << 2)

#define CLI_VIDEO_PID_RANGE             (1 << 4)


int cli_video_show_config(cli_control_block *ccb);
int cli_video_show_default(cli_control_block *ccb);
int cli_video_add_session(cli_control_block*ccb);
int cli_video_set_qam_tsid_onid(cli_control_block *ccb);
int cli_video_set_qam_psi_period(cli_control_block *ccb);
int cli_video_set_qam_encrypt(cli_control_block *ccb);
int cli_video_set_out_sess_enable(cli_control_block *ccb);
int cli_video_set_out_sess_encrypt(cli_control_block *ccb);
int cli_video_set_qam_reserved_pid_range(cli_control_block *ccb);
int cli_video_add_crsl_insert(cli_control_block *ccb);
int cli_video_change_source(cli_control_block *ccb);
int cli_video_change_in_session(cli_control_block *ccb);
int cli_video_delete_session(cli_control_block *ccb);
int cli_video_add_crsl(cli_control_block *ccb);
int cli_video_delete_crsl(cli_control_block *ccb);
int cli_video_delete_owner(cli_control_block *ccb);

int cli_video_query_crsl(cli_control_block *ccb);
int cli_video_query_qam_pat(cli_control_block *ccb);
int cli_video_query_ses_in_pat(cli_control_block *ccb);
int cli_video_query_ses_in_pmt(cli_control_block *ccb);
int cli_video_query_ses_out_pmt(cli_control_block *ccb);
int cli_video_query_ses_stat(cli_control_block *ccb);

int cli_video_lcred_mode_role(cli_control_block *ccb);
int cli_video_lcred_go_hot(cli_control_block *ccb);
int cli_video_lcred_show_primary(cli_control_block *ccb);
int cli_video_lcred_set_primary(cli_control_block *ccb);

int cli_video_clear_context(cli_control_block *ccb);
int cli_video_clear_jibdq_stats(cli_control_block *ccb);

int cli_video_log(cli_control_block *ccb);

int cli_video_ca_update_scg(cli_control_block *ccb);

int cli_video_ca_add_desc(cli_control_block *ccb);
int cli_video_ca_delete_desc(cli_control_block *ccb);
int cli_video_ca_sid_ecm_pid (cli_control_block *ccb);
int cli_video_ca_ecm_playout (cli_control_block *ccb);
int cli_video_ca_release_pid (cli_control_block *ccb);
int cli_video_ca_set_cwindex (cli_control_block *ccb);

int cli_video_set_filter_pid(cli_control_block *ccb);
int cli_video_set_filter_prog(cli_control_block *ccb);
int cli_video_set_remap_prog(cli_control_block *ccb);
int cli_video_set_remap_pid(cli_control_block *ccb);

int cli_dump(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);

int cli_video_ca_get_pid(cli_control_block *ccb);
int cli_video_ca_get_sid(cli_control_block *ccb);
int cli_video_ca_request_ecm_pid(cli_control_block *ccb);
int cli_video_ca_request_pid(cli_control_block *ccb);
int cli_video_ca_reserve_pid(cli_control_block *ccb);
int cli_video_ca_release_pid(cli_control_block *ccb);
int cli_video_ca_set_cw_index(cli_control_block *ccb);

int cli_video_set_default_config(cli_control_block *ccb);
int cli_video_set_default_cr_mode(cli_control_block *ccb);
int cli_video_set_default_jitter_delay(cli_control_block *ccb);
int cli_video_set_default_timer(cli_control_block *ccb);
int cli_video_set_default_encrypt(cli_control_block *ccb);
int cli_video_set_default_ownerid(cli_control_block *ccb);
int cli_video_set_default_cas_sys_id(cli_control_block *ccb);

#endif // __SIM_CLI_VIDEO_H__

