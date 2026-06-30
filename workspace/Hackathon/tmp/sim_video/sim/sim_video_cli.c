///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include "common.h"
#include "cli_common.h"
#include "generic_cli.h"
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "sim_video.h"


void sim_video_cli_init (void)
{
    cli_peer_init();
}


static void exec_video_show_stat (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Run time: ");
    print_elapsed_time(ccb->fp, get_elapsed_sec(0LL, get_clock_time()));
    fprintf(ccb->fp, "Num servers %d, pending %d, wake %d\n",
           srv_cnt, pending_srv_cnt, wake_cnt);

    if (!video_ctx) {
        fprintf(ccb->fp, "Video is not hot yet\n");
        return;
    }

    fprintf(ccb->fp, "\nIP stat:\n");
    print_video_ip_stat(ccb->fp, &ip_stat);

    video_in_stat_t in_stat;
    fprintf(ccb->fp, "\nIn sessions: %u\n", video_ctx->in_session_cnt);
    video_in_stat_collect(&in_stat);
    print_video_in_stat(ccb->fp, &in_stat);

    video_out_stat_t out_stat;
    fprintf(ccb->fp, "\nOut sessions: %u\n", video_ctx->out_session_cnt);
    video_out_stat_collect(&out_stat);
    print_video_out_stat(ccb->fp, &out_stat);

    video_qam_stat_t qam_stat;
    fprintf(ccb->fp, "\nQAMs:\n");
    video_qam_stat_collect(&qam_stat);
    fprintf(ccb->fp, "  Bitrate %lld\n", qam_stat.bitrate);
    fprintf(ccb->fp, "    mux %lld, drop %lld\n",
           qam_stat.mux_tp_cnt, qam_stat.drop_tp_cnt);
}


static void exec_video_show_stat_thread (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Packet receive: ");
    thread_stat_print(ccb->fp, &rx_thread_stat);
    fprintf(ccb->fp, "Demux: ");
    thread_stat_print(ccb->fp, &demux_thread_stat);
    fprintf(ccb->fp, "Input: ");
    thread_stat_print(ccb->fp, &input_thread_stat);
    fprintf(ccb->fp, "Output: ");
    thread_stat_print(ccb->fp, &output_thread_stat);
}


static void show_map_session_id (FILE *fp, video_context_t *ctx)
{
    fprintf(fp, "Session ID map:\n");
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    hash_table_print(fp, sim_ctx->ses_id_hash_tab);
}


static void exec_video_show_map_session_id (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    show_map_session_id(ccb->fp, video_ctx);
}


static void show_map_crsl_id (FILE *fp, video_context_t *ctx)
{
    fprintf(fp, "Carousel ID map:\n");
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    hash_table_print(fp, sim_ctx->crsl_id_hash_tab);
}


static void exec_video_show_map_crsl_id (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    show_map_crsl_id(ccb->fp, video_ctx);
}


static void exec_video_show_config_map_session_id (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;
    show_map_session_id(ccb->fp, ctx);
}


static void exec_video_show_config_map_crsl_id (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;
    show_map_crsl_id(ccb->fp, ctx);
}


static void exec_video_show_config_sessions (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;

    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item
            = (resource_id_hash_item_t*)sim_ctx->ses_id_hash_pool.buf;
    int i;
    int ses_cnt = 0;
    for (i=0; i<MAX_SESSIONS; i++, item++) {
        if (item->used_flag) {
            ses_cnt++;
            out_config_t* cfg = get_out_session_config(ctx, item->id);
            fprintf(ccb->fp, "Ses %d: IN %d, OUT %d\n",
                    item->hash_code, cfg->in_ses_id, item->id);
        }
    }
    fprintf(ccb->fp, "Total %d sessions configured\n", ses_cnt);
}


int exec_video_show_lcred (cli_control_block *ccb)
{
    fprintf(ccb->fp, "LCRED: mode %s, role %s\n",
            lcred_mode_label[lcred_mode], lcred_role_label[lcred_role]);
    if (lcred_role == LCRED_ACTIVE_ROLE) {
        fprintf(ccb->fp, "Slot: physical %d, primary %d\n",
                MY_SLOT_ID, lcred_primary_id);
    }
    return EOK;
}


static
int exec_video_show_contexts (cli_control_block *ccb)
{
    int i;
    for (i=MIN_NG_SLOT_ID; i<=MAX_NG_SLOT_ID; i++) {
        fprintf(ccb->fp, "Slot %d: ", i);
        if (!video_context_inited(i)) {
            fprintf(ccb->fp, "not initialized\n");
        } else {
            fprintf(ccb->fp, "%x\n", (uintptr_t)video_context[i]);
        }
    }
    return EOK;
}


void exec_video_reset (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Resetting video data...\n");
    video_ctx = NULL;
    sleep(1);

    int i;
    for (i=MIN_NG_SLOT_ID; i<=MAX_NG_SLOT_ID; i++) {
        video_context_cleanup(i);
    }

    lcred_primary_id = INVALID_ID;
    lcred_mode = LCRED_MAX_MODE;
    lcred_role = LCRED_MAX_ROLE;
}


static
void exec_video_get_pid_stat (cli_control_block *ccb)
{
    char json_buf[2000];
    //pid_stat_convert_json(pid_stat_tab[prev_stat_id], json_buf);
pid_stat_convert_json(pid_stat_tab[cur_stat_id], json_buf);
    fprintf(ccb->fp, "%s\n", json_buf);
}


static
boolean sim_video_cli_handler (cli_control_block *ccb)
{
    
    switch (ccb->fooid) {

    case CLI_VIDEO_GET_PID_STAT:
        exec_video_get_pid_stat(ccb);
        break;

    case CLI_VIDEO_SHOW_STAT:
        exec_video_show_stat(ccb);
        break;

    case CLI_VIDEO_SHOW_STAT_THREAD:
        exec_video_show_stat_thread(ccb);
        break;

    case CLI_VIDEO_SHOW_MAP_SESSION_ID:
        exec_video_show_map_session_id(ccb);
        break;

    case CLI_VIDEO_SHOW_MAP_CRSL_ID:
        exec_video_show_map_crsl_id(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_MAP_SESSION_ID:
        exec_video_show_config_map_session_id(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_MAP_CRSL_ID:
        exec_video_show_config_map_crsl_id(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_SESSIONS:
        exec_video_show_config_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_LCRED:
        exec_video_show_lcred(ccb);
        break;

    case CLI_VIDEO_SHOW_CONTEXTS:
        exec_video_show_contexts(ccb);
        break;

    case CLI_VIDEO_RESET:
        exec_video_reset(ccb);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


void cli_handler (void* arg)
{
    cli_control_block* ccb = (cli_control_block*)arg;
    cli_prepare_cc(ccb);
    cli_peer_open_console(ccb);

    if (!sim_video_cli_handler(ccb)
         && !video_cli_handler(ccb))
    {
        fprintf(ccb->fp, "Unknown CLI function ID: %d\n", ccb->fooid);
    }
    cli_peer_close_console(ccb);
}


void* cli_server_loop (void *arg)
{
    signal(SIGPIPE, SIG_IGN);           // ignore SIGPIPE signal
    server_config_t* cfg = (server_config_t*)arg;
    int sock_fd = cfg->sock_fd;

    while (1) {
        cli_control_block ccb;
        int n = read(sock_fd, &ccb, sizeof(ccb));
        if (n == 0) {
            break;
        }
        if (n == -1) {
            printf("Error: %s\n", strerror(errno));
            break;
        }
        cfg->handler(&ccb);
        write(sock_fd, &ccb, 1);        // prompt control
    }

    close(sock_fd);
    pthread_exit(NULL);
    return NULL;
}
