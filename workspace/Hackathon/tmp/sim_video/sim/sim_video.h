///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#ifndef __SIM_VIDEO_H__
#define __SIM_VIDEO_H__

#include "common.h"
#include "thread_stat.h"
#include "video.h"
#include "cli_common.h"

/*
 * TODO: Need to revisit the errmsg and provide the proper printf call for debugging
 */ 
#define errmsg(...) 

#define MY_SLOT_ID              0       // physical slot ID

#define MAX_TP_PER_IP           7
#define BUF_SZ                  (TP_SIZE * MAX_TP_PER_IP)
#define HDR_SZ                  (sizeof(ip_header_t) + sizeof(udp_header_t))

#define NUM_IP_BUFS             1000

#define VIDEO_DATA_PORT         50000
#define VIDEO_FLOWCTRL_PORT     50001
#define VIDEO_MSG_PORT          50002
#define VIDEO_CLI_PORT          50003

// cbr8 slot id
#define NUM_SLOTS                       10
#ifndef MIN_NG_SLOT_ID
#define MIN_NG_SLOT_ID                  0
#endif  
#ifndef MAX_NG_SLOT_ID
#define MAX_NG_SLOT_ID                  9
#endif

#define OUT_CMD_PER_FLOW        2048

#define VIDEO_LOG_NAME          "/video.log"
#define VIDEO_LOG_SIZE          (1024 * 1024)

#define MAX_TIME_ID		61


// Generic message/CLI server
//
typedef struct {
    int port;
    int sock_fd;
    void* (*server_loop)(void* arg);
    void (*handler)(void* arg);
} server_config_t;

void* msg_server_loop(void *arg);
void* cli_server_loop(void *arg);
void sim_video_msg_handler(void* arg);
void cli_handler(void* arg);


typedef struct {
    uint32 sys_time;            // in base ticks
    uint32 time_win_size;       // in base ticks
} sim_ctrl_msg_t;


typedef struct {
    uint32 arvl_time;           // arrival time (in base ticss)
    ip_header_t ip_hdr;
    udp_header_t udp_hdr;
    rtp_header_t rtp_hdr;
} sim_video_header_t;


extern out_command_t* out_cmd_buf[NUM_QAMS][FLOWS_PER_QAM];
extern ring_buf_t out_cmd_ring[NUM_QAMS][FLOWS_PER_QAM];

typedef uint32* resource_id_hash_key_t;

typedef struct {
    que_elem_t link;            // required
    uint32 hash_code;           // required (same as resource_id)
    uint16 id;                  // internal ID (E.g.: out_id or crsl_id)
    boolean used_flag;
} resource_id_hash_item_t;


typedef struct {
    hash_table_t *ses_id_hash_tab;
    pool_t ses_id_hash_pool;    // of resource_id_hash_item_t

    hash_table_t *crsl_id_hash_tab;
    pool_t crsl_id_hash_pool;   // of resource_id_hash_item_t
} sim_video_context_t;


uint32 resource_id_hash(void *key);
boolean resource_id_match(void *key, hash_item_t *item);
void ses_id_print(FILE *fp, hash_item_t *item);
void crsl_id_print(FILE *fp, hash_item_t *item);

uint16 session_id_alloc(video_context_t *ctx, uint32 ses_id);
void session_id_free(video_context_t *ctx, uint32 ses_id);

uint16 crsl_id_alloc(video_context_t *ctx, uint32 resource_id);
void crsl_id_free(video_context_t *ctx, uint32 resource_id);

extern int cur_stat_id, prev_stat_id;
extern pid_stat_t pid_stat_tab[2][NUM_PIDS];

extern int time_id;
extern int prog_tp_cnt[MAX_PROGS_PER_TS][MAX_TIME_ID];

extern mpeg_time_t sys_time;
extern mpeg_time_t sys_time_hi;

extern uint32 slot_id;
extern uint32 wake_cnt;
extern int srv_cnt;
extern int pending_srv_cnt;

extern thread_stat_t rx_thread_stat;
extern thread_stat_t demux_thread_stat;
extern thread_stat_t input_thread_stat;
extern thread_stat_t output_thread_stat;

extern int ip_buf_rd;
extern int ip_buf_wr;

extern video_ip_stat_t ip_stat;

extern uint64 mux_tp_cnt;
extern out_command_t* mux_cmd_buf[NUM_QAMS];
extern ring_buf_t mux_cmd_ring[NUM_QAMS];

extern const char* lcred_mode_label[];
extern const char* lcred_role_label[];
extern lcred_mode_t lcred_mode;
extern lcred_role_t lcred_role;
extern int lcred_primary_id;
extern int sim_video_lcred_change_mode_role(lcred_mode_t* mode,
                                            lcred_role_t* role);
extern void sim_video_lcred_mode_role_handler(ipc_message *ipc_msg);
extern void sim_video_lcred_go_hot_handler(ipc_message *ipc_msg);
extern void sim_video_lcred_lc_joined_group_handler(ipc_message *ipc_msg);
extern void sim_video_lcred_lc_left_group_handler(ipc_message *ipc_msg);
extern int exec_video_show_lcred(cli_control_block *ccb);

void sim_process_video_add_session(ipc_message *ipc_msg);

void sim_video_cli_init(void);

void pid_stat_convert_json(pid_stat_t *stat, char *buf);
void prog_stat_convert_json(char *buf);
void psi_table_convert_html(char *buf);

#endif // __SIM_VIDEO_H__
