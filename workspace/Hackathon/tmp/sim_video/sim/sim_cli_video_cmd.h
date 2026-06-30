///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#ifndef __SIM_CLI_VIDEO_CMD_H__
#define __SIM_CLI_VIDEO_CMD_H__

#include "sim_video.h"


/**** Video CLIs ***********************************************
video get pid

video show buffer
video show capture
video show lcred

video show map id {session | carousel}
video show map session
video show map qam <id>

video show config
video show config <slot> map id {session | carousel}
video show config <slot> map session
video show config <slot> map qam <id>
video show config <slot> session
video show config <slot> session <id>
video show config <slot> in
video show config <slot> out
video show config <slot> qam
video show config <slot> carousel
video show config <slot> carousel <id>
video show config <slot> owner <owner_id>

video show statistics
video show statistics thread

video show session
video show session summary
video show session <id_range>
video show session <id>
video show session <id> in {pcr | psi | pid}
video show session <id> out {psi | pid}

video show qam
video show qam summary
video show qam <qam_id>
video show qam <qam_id> flow
video show qam <qam_id> pat
video show qam <qam_id> pid
video show qam <qam_id> program
video show qam <qam_id> carousel

video show owner <id>

vidoe show cr
video show default 

video add session <start_rid>
            { ip <dst_ip> udp <dst_udp_range>
              | [ source-ip <src_ip> ] group-ip <grp_ip_range> }
            bitrate <bps> qam <qam_range>
            { remap <prog_num_range> pid <pid-range> | remux | passthru | data }

video delete session <rid_range>
video delete owner <owner-id>

video set in <in_id> master-pcr <pid>
video set qam <qam_id_range> tsid <tsid_range> onid <onid>
video set qam <qam_id_range> psi-period <ms>
video set qam <qam_id_range> encrypt {on | off}
video set qam <qam_id_range> enable {on | off}

video set default config <slot>
video set default cr-mode {vbr | cbr | master-slave}
video set default jitter <jitter-ms> delay <delay-ms>
video set default timer <init_timer_in_ms> <idle_timer_in_ms> <off_timer_in_sec>
video set default encrypt { on | off }
video set default owner-id <id>

video set disc-insert-enable {on | off}

video cr pll <lf_par> <loop_gain>
video cr limit <freq_offset_in_ppm> <drift_rate_in_ppm_per_hour>
video cr review <monitor_period> <drift_thres_in_ms> <adjust_thres_in_ms>
video cr restart <in_id>

video cr monitor <out_id> <sec>
video cr monitor off

video cr record set <filename> <max_size> <in_id>
video cr record start
video cr record stop
video cr record status

video psi record set <filename> <max_tps>
video psi record start <in_id>
video psi record status

// CA related CLIs
video cas qam <qam_id> add ca-desc ca-pid <ca_pid> program <sid>
video cas qam <qam_id> add ca-desc ca-pid <ca_pid> program <sid> all-es
video cas qam <qam_id> add ca-desc ca-pid <ca_pid> pid <pid>
video cas qam <qam_id> delete ca-desc ca-pid <ca_pid> program <sid>
video cas qam <qam_id> delete ca-desc ca-pid <ca_pid> program <sid> all-es
video cas qam <qam_id> delete ca-desc ca-pid <ca_pid> pid <sid>
video cas qam <qam_id> set cw-index <cw_idx> pid <pid>
video cas qam <qam_id> query program <sid>
video cas qam <qam_id> query pid <pid>
video cas qam <qam_id> request ecm-pid program <sid>
video cas qam <qam_id> request pid {<pid>}
video cas qam <qam_id> release pid <pid>

video capture { any | drop | l2-drop | sync-loss | cc-err }
video capture off

video debug message
video debug event
video debug in
video debug out
video debug psi
video debug cr
video debug ca

// lcred 
video lcred mode-role {non-redundant | primary | secondary} {active | standby}
video lcred go-hot <primary_id>

// PID filtering for passthru
video modify session <ses-id-range> add filter pid <pid-list>
video modify session <ses-id-range> delete filter pid <pid-list>
video modify session <ses-id-range> clear filter pid

// Program filtering for passthru
video modify session <ses-id-range> add filter program <prog-list>
video modify session <ses-id-range> delete filter program <prog-list>
video modify session <ses-id-range> clear filter program

// Program select for remux
video modify session <ses-id-range> add remap program <in-prog-list> <out-prog-list> pid <pid-range>
video modify session <ses-id-range> delete remap program <in-prog-list>
video modify session <ses-id-range> clear remap program

// Reset all configuration and state data
video reset

// Carousel insertion
video add carousel <id> qam <qam_list>
                        tp <num-tp> pid <pid> payload <hex-pattern> 
                        count <insert_cnt> period <insert_period> 
video delete carousel <id> [qam <qam_list>]
video query carousel <id> qam <qam_list>

video show carousel
video show carousel <id>
video show qam <qam_id> carousel
video show config <slot> carousel
video show config <slot> carousel <id>

// query CLIs
video query ses <ses_id> statistics
video query ses <ses_id> in pat
video query ses <ses_id> in pmt
video query ses <ses_id> out pmt
video query qam <qam_id> pat

***************************************************************/


#define VIDEO_CLI_SOCK                   0

CLI_ARR_START(cli_video_elem)

CLI_ARR_CMD (0, video,
             CLI_NULL(),
             0, "Video commands")

// Server get commands
//
CLI_ARR_CMD (1, get,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_GET_PID_STAT),
             0, "Video get pid stat")

// Video show commands
//
CLI_ARR_CMD (1, show,
             CLI_NULL(),
             0, "Video show commands")

CLI_ARR_CMD (2, buffer,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_BUFFER),
             0, "Show buffer info")

CLI_ARR_CMD (2, capture,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CAPTURE),
             0, "Show video capture")

CLI_ARR_CMD (2, config,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONTEXTS),
             0, "Show video contexts")

CLI_ARR_NUM (3, MIN_NG_SLOT_ID, MAX_NG_SLOT_ID,
             CLI_NULL(),
             0, "Slot number")

CLI_ARR_CMD (4, map,
             CLI_NULL(),
             0, "Map")

CLI_ARR_CMD (5, id,
             CLI_NULL(),
             0, "Resource ID maps")

CLI_ARR_CMD (6, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_MAP_SESSION_ID),
             0, "Session ID map")

CLI_ARR_CMD (6, carousel,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_MAP_CRSL_ID),
             0, "Carousel resource ID map")

CLI_ARR_CMD (5, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_MAP_SESSION),
             0, "Session map")

CLI_ARR_CMD (5, qam,
             CLI_NULL(),
             0, "Qam flow map")

CLI_ARR_NUM (6, 0, NUM_QAMS-1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_MAP_QAM),
             0, "QAM id")

CLI_ARR_CMD (4, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_SESSIONS),
             0, "Show configured video session IDs")

CLI_ARR_NUM (5, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_SESSION),
             0, "Session ID")

CLI_ARR_CMD (4, in,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_IN_SESSIONS),
             0, "Show video in session configuration")

CLI_ARR_CMD (4, out,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_OUT_SESSIONS),
             0, "Show video out session configuration")

CLI_ARR_CMD (4, qam,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_QAMS),
             0, "Show qam configuration")

CLI_ARR_CMD (4, carousel,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_CRSLS),
             0, "Show carousel configurations")

CLI_ARR_NUM (5, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_CRSL),
             0, "Show carousel configuration")

CLI_ARR_CMD (4, owner,
             CLI_NULL(),
             0, "Show video resource owned by a owner")

CLI_ARR_NUM (5, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CONFIG_OWNER),
             0, "Owner ID")


CLI_ARR_CMD (2, lcred,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_LCRED),
             0, "Show lcred info")

CLI_ARR_CMD (2, statistics,
             CLI_SOCK(VIDEO_CLI_SOCK, CLI_VIDEO_SHOW_STAT),
             0, "Show video statistics")

CLI_ARR_CMD (3, thread,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_STAT_THREAD),
             0, "Show video thread statistics")

CLI_ARR_CMD (2, map,
             CLI_NULL(),
             0, "Map")

CLI_ARR_CMD (3, id,
             CLI_NULL(),
             0, "Resource ID maps")

CLI_ARR_CMD (4, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_MAP_SESSION_ID),
             0, "Session ID map")

CLI_ARR_CMD (4, carousel,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_MAP_CRSL_ID),
             0, "Carousel resource ID map")

CLI_ARR_CMD (3, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_MAP_SESSION),
             0, "Session map")

CLI_ARR_CMD (3, qam,
             CLI_NULL(),
             0, "QAM flow map")

CLI_ARR_NUM (4, 0, NUM_QAMS-1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_MAP_QAM),
             0, "QAM id")

// video show session ...
//
CLI_ARR_CMD (2, session,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSIONS),
             0, "Show video sessions")

CLI_ARR_CMD (3, summary,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_SUMMARY),
             0, "Show video session summary")

CLI_ARR_STR (3,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_RANGE),
             0, "Session id range")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION),
             0, "Session id")

CLI_ARR_CMD (4, in,
             CLI_NULL(),
             0, "Show video session input")

CLI_ARR_CMD (5, pcr,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_IN_PCR),
             0, "Show video session input PCR table")

CLI_ARR_CMD (5, psi,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_IN_PSI),
             0, "Show video session input PSI")

CLI_ARR_CMD (5, pid,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_IN_PID),
             0, "Show video session input pid table")

CLI_ARR_CMD (4, out,
             CLI_NULL(),
             0, "Show video session output")

CLI_ARR_CMD (5, psi,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_OUT_PSI),
             0, "Show video session output PSI")

CLI_ARR_CMD (5, pid,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_SESSION_OUT_PID),
             0, "Show video session output pid table")

CLI_ARR_CMD (2, qam,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAMS),
             0, "Show qam info")

CLI_ARR_CMD (3, summary,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_SUMMARY),
             0, "Show summary of qam channels")

CLI_ARR_NUM (3, 0, NUM_QAMS - 1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM),
             0, "QAM id")

CLI_ARR_CMD (4, flow,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_FLOWS),
             0, "Show flows in qam")

CLI_ARR_CMD (4, pat,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_PAT),
             0, "Show qam PAT")

CLI_ARR_CMD (4, pid,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_PID_MAP),
             0, "Show PID usage map")

CLI_ARR_NUM (5, 0, 0x1FFF,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_PID),
             0, "PID value")

CLI_ARR_CMD (4, program,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_PROG_MAP),
             0, "Show program number usage map")

CLI_ARR_NUM (5, 0, 65535,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_PROG),
             0, "Program number")

CLI_ARR_CMD (4, carousel,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_QAM_CRSL),
             0, "Show carousels in qam")

// video show carousel ...
CLI_ARR_CMD (2, carousel,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CRSLS),
             0, "Show video data carousels")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CRSL),
             0, "Show video data carousel")

CLI_ARR_CMD (2, owner,
             CLI_NULL(),
             0, "Show video resource owned by a owner")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_OWNER),
             0, "Owner ID")

// video show cr
CLI_ARR_CMD (2, cr,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SHOW_CR),
             0, "Show video clock recovery parameters")

// video show default
CLI_ARR_CMD (2, default,
             CLI_FUN(cli_video_show_default, 0),
             0, "Show sim_cli video default setting")

// Video config commands
//
CLI_ARR_CMD (1, add,
             CLI_NULL(),
             0, "Add video session")

CLI_ARR_CMD (2, session,
             CLI_NULL(),
             0, "Session")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_NULL(),
             0, "Start session ID")

CLI_ARR_CMD (4, ip,
             CLI_NULL(),
             0, "Unicast destination IP")

CLI_ARR_STR (5,
             CLI_NULL(),
             0, "IP address")

CLI_ARR_CMD (6, udp,
             CLI_NULL(),
             0, "Destination UDP ports")

CLI_ARR_STR (7,
             CLI_NULL(),
             0, "UDP port range")

CLI_ARR_CMD (8, bitrate,
             CLI_NULL(),
             0, "Allocated bitrate")

CLI_ARR_NUM (9, 0, 52000000,
             CLI_NULL(),
             0, "Bitrate in bps")

CLI_ARR_CMD (10, qam,
             CLI_NULL(),
             0, "QAM channels")

CLI_ARR_STR (11,
             CLI_NULL(),
             0, "QAM channels ID range")

CLI_ARR_CMD (12, remap,
             CLI_NULL(),
             0, "Remap session")

CLI_ARR_STR (13,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_UDP | CLI_VIDEO_PROCESS_TYPE_REMAP),
             0, "Program number range")

CLI_ARR_CMD (14, pid,
             CLI_NULL(),
             0, "PID range")

CLI_ARR_STR (15,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_UDP | CLI_VIDEO_PROCESS_TYPE_REMAP |
                     CLI_VIDEO_PID_RANGE),
             0, "PID range (e.g. 48-79)")

CLI_ARR_CMD (12, remux,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_UDP | CLI_VIDEO_PROCESS_TYPE_REMUX),
             0, "Remux session")

CLI_ARR_CMD (12, passthru,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_UDP | CLI_VIDEO_PROCESS_TYPE_PASSTHRU),
             0, "Passthrough session")

CLI_ARR_CMD (12, data,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_UDP | CLI_VIDEO_PROCESS_TYPE_DATA),
             0, "Data-piping session")

CLI_ARR_CMD (4, group-ip,
             CLI_NULL(),
             0, "Group IP range")

CLI_ARR_STR (5,
             CLI_NULL(),
             0, "IP address range")

CLI_ARR_CMD (6, bitrate,
             CLI_NULL(),
             0, "Allocated bitrate")

CLI_ARR_NUM (7, 0, 52000000,
             CLI_NULL(),
             0, "Bitrate in bps")

CLI_ARR_CMD (8, qam,
             CLI_NULL(),
             0, "QAM channels")

CLI_ARR_STR (9,
             CLI_NULL(),
             0, "QAM channels ID range")

CLI_ARR_CMD (10, remap,
             CLI_NULL(),
             0, "Remap session")

CLI_ARR_STR (11,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_ASM | CLI_VIDEO_PROCESS_TYPE_REMAP),
             0, "Program number range")

CLI_ARR_CMD (12, pid,
             CLI_NULL(),
             0, "PID range")

CLI_ARR_STR (13,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_ASM | CLI_VIDEO_PROCESS_TYPE_REMAP |
                     CLI_VIDEO_PID_RANGE),
             0, "PID range (e.g. 48-79)")

CLI_ARR_CMD (10, remux,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_ASM | CLI_VIDEO_PROCESS_TYPE_REMUX),
             0, "Remux session")

CLI_ARR_CMD (10, passthru,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_ASM | CLI_VIDEO_PROCESS_TYPE_PASSTHRU),
             0, "Passthrough session")

CLI_ARR_CMD (10, data,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_ASM | CLI_VIDEO_PROCESS_TYPE_DATA),
             0, "Data-piping session")

CLI_ARR_CMD (4, source-ip,
             CLI_NULL(),
             0, "Source IP")

CLI_ARR_STR (5,
             CLI_NULL(),
             0, "IP address")

CLI_ARR_CMD (6, group-ip,
             CLI_NULL(),
             0, "Group IP range")

CLI_ARR_STR (7,
             CLI_NULL(),
             0, "IP address range")

CLI_ARR_CMD (8, bitrate,
             CLI_NULL(),
             0, "Allocated bitrate")

CLI_ARR_NUM (9, 0, 52000000,
             CLI_NULL(),
             0, "Bitrate in bps")

CLI_ARR_CMD (10, qam,
             CLI_NULL(),
             0, "QAM channels")

CLI_ARR_STR (11,
             CLI_NULL(),
             0, "QAM channels ID range")

CLI_ARR_CMD (12, remap,
             CLI_NULL(),
             0, "Remap session")

CLI_ARR_STR (13,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_SSM | CLI_VIDEO_PROCESS_TYPE_REMAP),
             0, "Program number range")

CLI_ARR_CMD (14, pid,
             CLI_NULL(),
             0, "PID range")

CLI_ARR_STR (15,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_SSM | CLI_VIDEO_PROCESS_TYPE_REMAP |
                     CLI_VIDEO_PID_RANGE),
             0, "PID range (e.g. 48-79)")

CLI_ARR_CMD (12, remux,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_SSM | CLI_VIDEO_PROCESS_TYPE_REMUX),
             0, "Passthrough session")

CLI_ARR_CMD (12, passthru,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_SSM | CLI_VIDEO_PROCESS_TYPE_PASSTHRU),
             0, "Passthrough session")

CLI_ARR_CMD (12, data,
             CLI_FUN(cli_video_add_session,
                     CLI_VIDEO_CAST_TYPE_SSM | CLI_VIDEO_PROCESS_TYPE_DATA),
             0, "Data-piping session")

// CLI: video add carousel ...
CLI_ARR_CMD (2, carousel,
             CLI_NULL(),
             0, "Carousel")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_NULL(),
             0, "Carousel ID")

CLI_ARR_CMD (4, qam,
             CLI_NULL(),
             0, "Qam channels")

CLI_ARR_STR (5,
             CLI_NULL(),
             0, "QAM list (e.g. 0,1,2,35)")

CLI_ARR_CMD (6, tp,
             CLI_NULL(),
             0, "Carousel TPs")

CLI_ARR_NUM (7, 0, MAX_CRSL_NUM_TP - 1,
             CLI_NULL(),
             0, "Number of TPs in carousel")

CLI_ARR_CMD (8, pid,
             CLI_NULL(),
             0, "PID")

CLI_ARR_NUM (9, 1, NUM_PIDS - 1,
             CLI_NULL(),
             0, "PID value")

CLI_ARR_CMD (10, payload,
             CLI_NULL(),
             0, "Carousel TP payload")

CLI_ARR_STR (11,
             CLI_NULL(),
             0, "Carousel data pattern in hex (e.g. 0102abcdef)")

CLI_ARR_CMD (12, count,
             CLI_NULL(),
             0, "Times to insert")

CLI_ARR_NUM (13, -1, 1000000,
             CLI_NULL(),
             0, "Times to insert (-1 means continuous insertion)")

CLI_ARR_CMD (14, period,
             CLI_NULL(),
             0, "Insertion period")

CLI_ARR_NUM (15, 1, 60000,
             CLI_FUN(cli_video_add_crsl, 0),
             0, "Insertion period in ms")


// Delete CLIs: video delete ...
//
CLI_ARR_CMD (1, delete,
             CLI_NULL(),
             0, "Delete video resources")

CLI_ARR_CMD (2, session,
             CLI_NULL(),
             0, "Delete video session")

CLI_ARR_STR (3,
             CLI_FUN(cli_video_delete_session, 0),
             0, "Out session ID range (e.g. 0-65535)")

CLI_ARR_CMD (2, carousel,
             CLI_NULL(),
             0, "Delete carousel")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_FUN(cli_video_delete_crsl, 0),
             0, "Carousel ID")

CLI_ARR_CMD (4, qam,
             CLI_NULL(),
             0, "Qam channels")

CLI_ARR_STR (5,
             CLI_FUN(cli_video_delete_crsl, 0),
             0, "QAM list (e.g. 0,1,2,35)")

CLI_ARR_CMD (2, owner,
             CLI_NULL(),
             0, "Delete sessions/carousels based on owner")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_FUN(cli_video_delete_owner, 0),
             0, "Owner ID")


// video modify ...
//
CLI_ARR_CMD (1, modify,
             CLI_NULL(),
             0, "Modify video session")

CLI_ARR_CMD (2, session,
             CLI_NULL(),
             0, "Session")

CLI_ARR_STR (3,
             CLI_NULL(),
             0, "Out session ID range (e.g. 0-65535)")

CLI_ARR_CMD (4, add,
             CLI_NULL(),
             0, "Add")

CLI_ARR_CMD (5, filter,
             CLI_NULL(),
             0, "Add filter")

CLI_ARR_CMD (6, pid,
             CLI_NULL(),
             0, "Add PID filter")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_filter_pid, VIDEO_LIST_OPER_ADD),
             0, "PID list (e.g. 10,22,15)")

CLI_ARR_CMD (6, program,
             CLI_NULL(),
             0, "Add program filter")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_filter_prog, VIDEO_LIST_OPER_ADD),
             0, "Program list (e.g. 1,12,5)")

CLI_ARR_CMD (5, remap,
             CLI_NULL(),
             0, "Add remapped programs or pids")

CLI_ARR_CMD (6, program,
             CLI_NULL(),
             0, "Add remapped programs")

CLI_ARR_STR (7,
             CLI_NULL(),
             0, "Input program list (e.g. 1,12,5)")

CLI_ARR_STR (8,
             CLI_FUN(cli_video_set_remap_prog, VIDEO_LIST_OPER_ADD),
             0, "Output program list (e.g. 1,12,5)")

CLI_ARR_CMD (9, pid,
             CLI_NULL(),
             0, "PID range")

CLI_ARR_STR (10,
             CLI_FUN(cli_video_set_remap_prog,
                     VIDEO_LIST_OPER_ADD | CLI_VIDEO_PID_RANGE),
             0, "PID range (e.g. 48-79)")

CLI_ARR_CMD (6, pid,
             CLI_NULL(),
             0, "Add remapped pids")

CLI_ARR_STR (7,
             CLI_NULL(),
             0, "Input pid list (e.g. 1,12,5)")

CLI_ARR_STR (8,
             CLI_FUN(cli_video_set_remap_pid, VIDEO_LIST_OPER_ADD),
             0, "Output pid list (e.g. 1,12,5)")

CLI_ARR_CMD (4, delete,
             CLI_NULL(),
             0, "Delete")

CLI_ARR_CMD (5, filter,
             CLI_NULL(),
             0, "Delete filter")

CLI_ARR_CMD (6, pid,
             CLI_NULL(),
             0, "Delete PID filter")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_filter_pid, VIDEO_LIST_OPER_DELETE),
             0, "PID list (e.g. 10,22,15)")

CLI_ARR_CMD (6, program,
             CLI_NULL(),
             0, "Delete program filter")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_filter_prog, VIDEO_LIST_OPER_DELETE),
             0, "Program list (e.g. 1,12,5)")

CLI_ARR_CMD (5, remap,
             CLI_NULL(),
             0, "Delete remapped programs or pids")

CLI_ARR_CMD (6, program,
             CLI_NULL(),
             0, "Delete remapped programs")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_remap_prog, VIDEO_LIST_OPER_DELETE),
             0, "Input program list (e.g. 1,12,5)")

CLI_ARR_CMD (6, pid,
             CLI_NULL(),
             0, "Delete remapped pids")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_remap_pid, VIDEO_LIST_OPER_DELETE),
             0, "Input pid list (e.g. 1,12,5)")

CLI_ARR_CMD (4, clear,
             CLI_NULL(),
             0, "Clear")

CLI_ARR_CMD (5, filter,
             CLI_NULL(),
             0, "Clear filter")

CLI_ARR_CMD (6, pid,
             CLI_FUN(cli_video_set_filter_pid, VIDEO_LIST_OPER_CLEAR),
             0, "Clear all PID filters")

CLI_ARR_CMD (6, program,
             CLI_FUN(cli_video_set_filter_prog, VIDEO_LIST_OPER_CLEAR),
             0, "Clear all program filters")

CLI_ARR_CMD (5, remap,
             CLI_NULL(),
             0, "Clear remapped programs or pids")

CLI_ARR_CMD (6, program,
             CLI_FUN(cli_video_set_remap_prog, VIDEO_LIST_OPER_CLEAR),
             0, "Clear all remapped programs")

CLI_ARR_CMD (6, pid,
             CLI_FUN(cli_video_set_remap_pid, VIDEO_LIST_OPER_CLEAR),
             0, "Clear all remapped pids")


// Video queries: video query ...
//
CLI_ARR_CMD (1, query,
             CLI_NULL(),
             0, "Query")

CLI_ARR_CMD (2, carousel,
             CLI_NULL(),
             0, "Carousel query")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_NULL(),
             0, "Carousel resource ID")

CLI_ARR_CMD (4, qam,
             CLI_NULL(),
             0, "Qam channels")

CLI_ARR_STR (5,
             CLI_FUN(cli_video_query_crsl, 0),
             0, "QAM list (e.g. 0,1,2,35)")

CLI_ARR_CMD (2, qam,
             CLI_NULL(),
             0, "QAM query")

CLI_ARR_NUM (3, 0, NUM_QAMS - 1,
             CLI_NULL(),
             0, "QAM ID")

CLI_ARR_CMD (4, pat,
             CLI_FUN(cli_video_query_qam_pat, 0),
             0, "Query QAM PAT")

CLI_ARR_CMD (2, session,
             CLI_NULL(),
             0, "Session query")

CLI_ARR_NUM (3, 0, INT_MAX,
             CLI_NULL(),
             0, "Session ID")

CLI_ARR_CMD (4, in,
             CLI_NULL(),
             0, "Session input query")

CLI_ARR_CMD (5, pat,
             CLI_FUN(cli_video_query_ses_in_pat, 0),
             0, "Query session input PAT")

CLI_ARR_CMD (5, pmt,
             CLI_FUN(cli_video_query_ses_in_pmt, 0),
             0, "Query session input PMT")

CLI_ARR_CMD (4, out,
             CLI_NULL(),
             0, "Session output query")

CLI_ARR_CMD (5, pmt,
             CLI_FUN(cli_video_query_ses_out_pmt, 0),
             0, "Query session output PMT")

CLI_ARR_CMD (4, statistics,
             CLI_FUN(cli_video_query_ses_stat, 0),
             0, "Query session statistics")


// Set commands
//
CLI_ARR_CMD (1, set,
             CLI_NULL(),
             0, "Set")

CLI_ARR_CMD (2, in,
             CLI_NULL(),
             0, "Input session")

CLI_ARR_NUM (3, 0, MAX_SESSIONS - 1,
             CLI_NULL(),
             0, "In session ID")

CLI_ARR_CMD (4, master-pcr,
             CLI_NULL(),
             0, "Set preferred master PCR pid")

CLI_ARR_NUM (5, 0, NULL_PID,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SET_IN_MASTER_PCR_PID),
             0, "Preferred master PCR PID")

CLI_ARR_CMD (2, qam,
             CLI_NULL(),
             0, "Configure QAM")

CLI_ARR_STR (3,
             CLI_NULL(),
             0, "QAM ID range")

CLI_ARR_CMD (4, tsid,
             CLI_NULL(),
             0, "Configure TSID")

CLI_ARR_STR (5,
             CLI_NULL(),
             0, "TSID range")

CLI_ARR_CMD (6, onid,
             CLI_NULL(),
             0, "Configure ONID")

CLI_ARR_STR (7,
             CLI_FUN(cli_video_set_qam_tsid_onid, 0),
             0, "ONID")

CLI_ARR_CMD (4, encrypt,
             CLI_NULL(),
             0, "Encryption")

CLI_ARR_CMD (5, on,
             CLI_FUN(cli_video_set_qam_encrypt, CLI_VIDEO_ON),
             0, "Encryption ON")

CLI_ARR_CMD (5, off,
             CLI_FUN(cli_video_set_qam_encrypt, CLI_VIDEO_OFF),
             0, "Encryption OFF")

CLI_ARR_CMD (4, enable,
             CLI_NULL(),
             0, "Set QAM enable flag")

CLI_ARR_CMD (5, on,
             CLI_FUN(cli_video_set_qam_enable, CLI_VIDEO_ON),
             0, "Enable QAM")

CLI_ARR_CMD (5, off,
             CLI_FUN(cli_video_set_qam_enable, CLI_VIDEO_OFF),
             0, "Disable QAM")

CLI_ARR_CMD (4, psi-period,
             CLI_NULL(),
             0, "Configure PSI insertion period")

CLI_ARR_NUM (5, 1, 1000,
             CLI_FUN(cli_video_set_qam_psi_period, 0),
             0, "PSI period in ms")

// video set default ...
CLI_ARR_CMD (2, default,
             CLI_NULL(),
             0, "Set default parameters in sim_cli")

CLI_ARR_CMD (3, config,
             CLI_NULL(),
             0, "Set default config context")

CLI_ARR_NUM (4, MIN_NG_SLOT_ID, MAX_NG_SLOT_ID,
             CLI_FUN(cli_video_set_default_config, 0),
             0, "Slot number")

CLI_ARR_CMD (3, cr-mode,
             CLI_NULL(),
             0, "Set default clock recovery mode in sim_cli")

CLI_ARR_CMD (4, vbr,
             CLI_FUN(cli_video_set_default_cr_mode, VIDEO_CR_MODE_UNIFIED_VBR),
             0, "Unified VBR clock recovery mode")

CLI_ARR_CMD (4, cbr,
             CLI_FUN(cli_video_set_default_cr_mode, VIDEO_CR_MODE_UNIFIED_CBR),
             0, "Unified CBR clock recovery mode")

CLI_ARR_CMD (4, master-slave,
             CLI_FUN(cli_video_set_default_cr_mode, VIDEO_CR_MODE_MASTER_SLAVE),
             0, "Master-slave clock recovery mode (CBR)")

CLI_ARR_CMD (3, jitter,
             CLI_NULL(),
             0, "Set default network jitter in sim_cli")

CLI_ARR_NUM (4, 0, 300,
             CLI_NULL(),
             0, "Network jitter in ms")

CLI_ARR_CMD (5, delay,
             CLI_NULL(),
             0, "Set default system delay in sim_cli")

CLI_ARR_NUM (6, 0, 300,
             CLI_FUN(cli_video_set_default_jitter_delay, 0),
             0, "System delay in ms")

CLI_ARR_CMD (3, timer,
             CLI_NULL(),
             0, "Set default timer parameters in sim_cli")

CLI_ARR_NUM (4, 1, 5000,
             CLI_NULL(),
             0, "Default init timer in ms")

CLI_ARR_NUM (5, 1, 5000,
             CLI_NULL(),
             0, "Default idle timer in ms")

CLI_ARR_NUM (6, 1, 300,
             CLI_FUN(cli_video_set_default_timer, 0),
             0, "Default off timer in second")

CLI_ARR_CMD (3, encrypt,
             CLI_NULL(),
             0, "Set default session encryption flag in sim_cli")

CLI_ARR_CMD (4, on,
             CLI_FUN(cli_video_set_default_encrypt, CLI_VIDEO_ON),
             0, "Default session encryption turned ON")

CLI_ARR_CMD (4, off,
             CLI_FUN(cli_video_set_default_encrypt, CLI_VIDEO_OFF),
             0, "Default session encryption turned OFF")

CLI_ARR_CMD (3, owner-id,
             CLI_NULL(),
             0, "Set default owner ID")

CLI_ARR_NUM (4, 0, INT_MAX,
             CLI_FUN(cli_video_set_default_ownerid, 0),
             0, "Default owner ID")

CLI_ARR_CMD (3, cas-system-id,
             CLI_NULL(),
             0, "Set default CAS System ID")

CLI_ARR_NUM (4, 0, 65535,
             CLI_FUN(cli_video_set_default_cas_sys_id, 0),
             0, "Default CAS System ID")


// video set disc-insert-enable {on | off}
//
CLI_ARR_CMD (2, disc-insert-enable,
             CLI_NULL(),
             0, "Configure disc-insert-enable flag")

CLI_ARR_CMD (3, on,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SET_DISC_INSERT_ENABLE),
             0, "Allow discontinuity indicator to be inserted when needed (normal)")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SET_DISC_INSERT_ENABLE_OFF),
             0, "Suppress discontinuity indicator insertion (for testing only!)")


// Clock recovery commands: video cr ...
//
CLI_ARR_CMD (1, cr,
             CLI_NULL(),
             0, "Clock Recovery commands")

CLI_ARR_CMD (2, pll,
             CLI_NULL(),
             0, "Config Clock Recovery PLL parameters")

CLI_ARR_NUM (3, 0, (1 << FRAC_BITS_CLKREC) - 1,
             CLI_NULL(),
             0, "Low pass filter parameter")

CLI_ARR_NUM (4, 0, (1 << FRAC_BITS_CLKREC) - 1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_PLL),
             0, "Loop gain parameter")

CLI_ARR_CMD (2, limit,
             CLI_NULL(),
             0, "Config clock recovery limits")

CLI_ARR_NUM (3, 0, 100,
             CLI_NULL(),
             0, "Frequency offset limit (ppm)")

CLI_ARR_NUM (4, 0, 1000,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_LIMIT),
             0, "Drift rate limit (ppm/hr)")

CLI_ARR_CMD (2, review,
             CLI_NULL(),
             0, "Drift review parameters")

CLI_ARR_NUM (3, 0, 255,
             CLI_NULL(),
             0, "Monitor periods between reviews (0 to turn off)")

CLI_ARR_NUM (4, 2, 1000,
             CLI_NULL(),
             0, "Drift threshold (ms)")

CLI_ARR_NUM (5, 1, 1000,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_REVIEW),
             0, "Adjustment threshold (ms)")

CLI_ARR_CMD (2, restart,
             CLI_NULL(),
             0, "Restart in session")

CLI_ARR_NUM (3, 0, MAX_SESSIONS - 1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_RESTART),
             0, "In session ID")

CLI_ARR_CMD (2, monitor,
             CLI_NULL(),
             0, "Time monitoring")

CLI_ARR_NUM (3, 0, MAX_SESSIONS - 1,
             CLI_NULL(),
             0, "Out session ID")

CLI_ARR_NUM (4, 0, 300,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SET_MONITOR_TIMING),
             0, "Monitor period (seconds)")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_SET_MONITOR_TIMING_OFF),
             0, "Turn off time monitoring")

CLI_ARR_CMD (2, record,
             CLI_NULL(),
             0, "Record timing info")

CLI_ARR_CMD (3, set,
             CLI_NULL(),
             0, "Set record parameters")

CLI_ARR_STR (4,
             CLI_NULL(),
             0, "Filename")

CLI_ARR_NUM (5, 0, 134217728,
             CLI_NULL(),
             0, "Max file size (bytes)")

CLI_ARR_NUM (6, 0, MAX_SESSIONS - 1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_RECORD_SET),
             0, "In session ID")

CLI_ARR_CMD (3, start,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_RECORD_START),
             0, "Start recording")

CLI_ARR_CMD (3, stop,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_RECORD_STOP),
             0, "Stop recording")

CLI_ARR_CMD (3, status,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CR_RECORD_STATUS),
             0, "Recording status")


// PSI recording commands
//
CLI_ARR_CMD (1, psi,
             CLI_NULL(),
             0, "PSI")

CLI_ARR_CMD (2, record,
             CLI_NULL(),
             0, "PSI recording")

CLI_ARR_CMD (3, set,
             CLI_NULL(), 
             0, "Set PSI recording parameters")

CLI_ARR_STR (4,
             CLI_NULL(),
             0, "Filename")

CLI_ARR_NUM (5, 0, 256,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_PSI_RECORD_SET),
             0, "Max number of TPs")

CLI_ARR_CMD (3, start,
             CLI_NULL(),
             0, "Start PSI recording")

CLI_ARR_NUM (4, 0, MAX_SESSIONS - 1,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_PSI_RECORD_START),
             0, "In session ID")

CLI_ARR_CMD (3, status,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_PSI_RECORD_STATUS),
             0, "PSI recording status")


// Video CA commands
//
CLI_ARR_CMD (1, cas,
             CLI_NULL(),
             0, "Video CAS commands")

CLI_ARR_CMD (2, qam,
             CLI_NULL(),
             0, "Qam ID")

CLI_ARR_NUM (3, 0, NUM_QAMS-1,
             CLI_NULL(),
             0, "QAM id")

CLI_ARR_CMD (4, add,
             CLI_NULL(),
             0, "CA add")

CLI_ARR_CMD (5, ca-desc,
             CLI_NULL(),
             0, "Add CA descriptor to PMT")

CLI_ARR_CMD (6, ca-pid,
             CLI_NULL(),
             0, "CA PID")

CLI_ARR_NUM (7, 1, NUM_PIDS - 1,
             CLI_NULL(),
             0, "CA PID value")

CLI_ARR_CMD (8, program,
             CLI_NULL(),
             0, "Add CA-desc to a program")

CLI_ARR_NUM (9, 1, 65535,
             CLI_FUN(cli_video_ca_add_desc, CLI_VIDEO_CA_DESC_PROG),
             0, "Program number")

CLI_ARR_CMD (10, all-es,
             CLI_FUN(cli_video_ca_add_desc, CLI_VIDEO_CA_DESC_ALL_ES),
             0, "All ES PIDs")

CLI_ARR_CMD (8, pid,
             CLI_NULL(),
             0, "Add CA-desc to a PID")

CLI_ARR_NUM (9, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_add_desc, CLI_VIDEO_CA_DESC_PID),
             0, "PID value to add CA-desc")

CLI_ARR_CMD (4, delete,
             CLI_NULL(),
             0, "CA delete")

CLI_ARR_CMD (5, ca-desc,
             CLI_NULL(),
             0, "Delete CA descriptor from PMT")

CLI_ARR_CMD (6, ca-pid,
             CLI_NULL(),
             0, "CA PID")

CLI_ARR_NUM (7, 1, NUM_PIDS - 1,
             CLI_NULL(),
             0, "CA PID value")

CLI_ARR_CMD (8, program,
             CLI_NULL(),
             0, "Delete CA-desc from a program")

CLI_ARR_NUM (9, 1, 65535,
             CLI_FUN(cli_video_ca_delete_desc, CLI_VIDEO_CA_DESC_PROG),
             0, "Program number")

CLI_ARR_CMD (10, all-es,
             CLI_FUN(cli_video_ca_delete_desc, CLI_VIDEO_CA_DESC_ALL_ES),
             0, "All ES PIDs")

CLI_ARR_CMD (8, pid,
             CLI_NULL(),
             0, "Delete CA-desc from a PID")

CLI_ARR_NUM (9, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_delete_desc, CLI_VIDEO_CA_DESC_PID),
             0, "PID value to delete")

CLI_ARR_CMD (4, query,
             CLI_NULL(),
             0, "CA query")

CLI_ARR_CMD (5, pid,
             CLI_NULL(),
             0, "PID query")

CLI_ARR_NUM (6, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_get_pid, 0),
             0, "PID to query")

CLI_ARR_CMD (5, program,
             CLI_NULL(),
             0, "Program query")

CLI_ARR_NUM (6, 1, 65535,
             CLI_FUN(cli_video_ca_get_sid, 0),
             0, "Program number to query")

CLI_ARR_CMD (4, request,
             CLI_NULL(),
             0, "CA request")

CLI_ARR_CMD (5, ecm-pid,
             CLI_NULL(),
             0, "ECM PID request")

CLI_ARR_CMD (6, program,
             CLI_NULL(),
             0, "Program number for ECM PID request")

CLI_ARR_NUM (7, 1, 65535,
             CLI_FUN(cli_video_ca_request_ecm_pid, 0),
             0, "Program number")

CLI_ARR_CMD (5, pid,
             CLI_FUN(cli_video_ca_request_pid, 0),
             0, "PID request or reserve")

CLI_ARR_NUM (6, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_reserve_pid, 0),
             0, "PID value to reserve")

CLI_ARR_CMD (4, release,
             CLI_NULL(),
             0, "CA release")

CLI_ARR_CMD (5, pid,
             CLI_NULL(),
             0, "CA PID release")

CLI_ARR_NUM (6, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_release_pid, 0),
             0, "PID value")

CLI_ARR_CMD (4, set,
             CLI_NULL(),
             0, "CA set")

CLI_ARR_CMD (5, cw-index,
             CLI_NULL(),
             0, "Set codeword index")

CLI_ARR_NUM (6, 0, 100,
             CLI_NULL(),
             0, "Codeword index")

CLI_ARR_CMD (7, pid,
             CLI_NULL(),
             0, "PID to set codeword index")

CLI_ARR_NUM (8, 1, NUM_PIDS - 1,
             CLI_FUN(cli_video_ca_set_cw_index, 0),
             0, "PID value")


// Video capture commands
//
CLI_ARR_CMD (1, capture,
             CLI_NULL(),
             0, "Capture video packet")

CLI_ARR_CMD (2, any,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CAPTURE_ANY),
             0, "Any video packet")

CLI_ARR_CMD (2, drop,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CAPTURE_DROP),
             0, "Dropped video packet")

CLI_ARR_CMD (2, sync-loss,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CAPTURE_SYNCLOSS),
             0, "Video packet with sync loss")

CLI_ARR_CMD (2, cc-err,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CAPTURE_CCERR),
             0, "Video packet with CC error")

CLI_ARR_CMD (2, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_CAPTURE_OFF),
             0, "Turn off video capture")


// Video debug commands
//
CLI_ARR_CMD (1, debug,
             CLI_NULL(),
             0, "Video debug commands")

CLI_ARR_CMD (2, message,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_MSG),
             0, "Debug video message")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_MSG_OFF),
             0, "Debug video message off")

CLI_ARR_CMD (2, event,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_EVENT),
             0, "Debug video event")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_EVENT_OFF),
             0, "Debug video event off")

CLI_ARR_CMD (2, in,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_IN),
             0, "Debug video input")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_IN_OFF),
             0, "Debug video input off")

CLI_ARR_CMD (2, out,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_OUT),
             0, "Debug video output")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_OUT_OFF),
             0, "Debug video output off")

CLI_ARR_CMD (2, psi,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_PSI),
             0, "Debug video PSI")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_PSI_OFF),
             0, "Debug video PSI off")

CLI_ARR_CMD (2, cr,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_CLKREC),
             0, "Debug video clock recovery")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_CLKREC_OFF),
             0, "Debug video clock recovery off")

CLI_ARR_CMD (2, cas,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_CA),
             0, "Debug video CA")

CLI_ARR_CMD (3, off,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_DEBUG_CA_OFF),
             0, "Debug video CA off")


// LCHA commands
//
CLI_ARR_CMD (1, lcred,
             CLI_NULL(),
             0, "LCRED")

CLI_ARR_CMD (2, mode-role,
             CLI_NULL(),
             0, "Simulate LCRED mode-role message")

CLI_ARR_CMD (3, non-redundant,
             CLI_FUN(cli_video_lcred_mode_role,
                     CLI_VIDEO_LCRED_NON_REDUNDANT),
             0, "Non-redundant mode")

CLI_ARR_CMD (3, primary,
             CLI_NULL(),
             0, "Primary mode")

CLI_ARR_CMD (4, active,
             CLI_FUN(cli_video_lcred_mode_role,
                     CLI_VIDEO_LCRED_PRIMARY_ACTIVE),
             0, "Active role")

CLI_ARR_CMD (4, standby,
             CLI_FUN(cli_video_lcred_mode_role,
                     CLI_VIDEO_LCRED_PRIMARY_STANDBY),
             0, "Standby role")

CLI_ARR_CMD (3, secondary,
             CLI_NULL(),
             0, "Secondary mode")

CLI_ARR_CMD (4, active,
             CLI_FUN(cli_video_lcred_mode_role,
                     CLI_VIDEO_LCRED_SECONDARY_ACTIVE),
             0, "Active role")

CLI_ARR_CMD (4, standby,
             CLI_FUN(cli_video_lcred_mode_role,
                     CLI_VIDEO_LCRED_SECONDARY_STANDBY),
             0, "Standby role")

CLI_ARR_CMD (2, go-hot,
             CLI_NULL(),
             0, "Simulate lcred go-hot message")

CLI_ARR_NUM (3, MIN_NG_SLOT_ID, MAX_NG_SLOT_ID,
             CLI_FUN(cli_video_lcred_go_hot, 0),
             0, "Primary ID")


CLI_ARR_CMD (1, reset,
             CLI_SOCK(CLI_VIDEO_SOCK, CLI_VIDEO_RESET),
             0, "Reset all video config and staate (USE WITH CARE)")

CLI_ARR_END

#endif // __SIM_CLI_VIDEO_CMDS_H__
