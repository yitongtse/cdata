/*
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "video_config_db.h"
#include "bit_mask.h"
#include "generic_cli.h"
#include "trace.h"

#if !defined(MB_LC) && !defined(MV_LC) && !defined(PB_LC)
#define msecdelay(...)
#endif


// disable CLI
boolean disable_input_thread = FALSE;


video_context_t* check_get_video_context (FILE* fp, int context_id)
{
    video_context_t* ctx = get_video_context(context_id);
    if (ctx) {
        fprintf(fp, "Context %d:\n", context_id);
    } else {
        fprintf(fp, "Error: Context %d not initialized\n", context_id);
    }
    return ctx;
}

static void exec_video_show_session_map (cli_control_block *ccb)
{
    int i;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    fprintf(ccb->fp, "Session map:\n");
    for (i=0; i<NUM_INPUT_PORTS; i++) {
        fprintf(ccb->fp, "\nInput port %d:\n", i);
        hash_table_print(ccb->fp, video_ctx->ses_hash_tab[i]);
    }
}

static void exec_video_show_session_map_summary (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    fprintf(ccb->fp, "Session summary map:\n");
    int i;
    for (i=0; i<NUM_INPUT_PORTS; i++) {
        fprintf(ccb->fp, "\nInput port %d:\n", i);
        hash_table_print_summary(ccb->fp, video_ctx->ses_hash_tab[i]);
    }
}


static void exec_video_show_map_qam (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    uint32 qam_id = ccb->data[4].i32;
    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Bad qam ID: %d\n", qam_id);
        return;
    }
    show_qam_map(ccb->fp, video_ctx, qam_id);
}


static void exec_video_show_in_sessions (cli_control_block *ccb)
{
    int rid;
    int off_cnt = 0;
    int active_cnt = 0;
    int idle_cnt = 0;
    int init_cnt = 0;
    int all_flag = 0;
    int off_flag = 0;
    int active_flag = 0;
    int idle_flag = 0;
    int total_cnt = 0;
    int64 total_rate = 0;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    switch (ccb->fooid) {
    case CLI_VIDEO_SHOW_IN_SESSIONS:
        all_flag = 1;
        break;

    case CLI_VIDEO_SHOW_IN_SESSIONS_OFF:
        off_flag = 1;
        break;

    case CLI_VIDEO_SHOW_IN_SESSIONS_ACTIVE:
        active_flag = 1;
        break;

    case CLI_VIDEO_SHOW_IN_SESSIONS_IDLE:
        idle_flag = 1;
        break;

    default:
        break;
    }

    fprintf(ccb->fp, "In sessions:\n");

    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        in_session_t* ses;
        in_config_t* cfg = get_in_session_config(video_ctx, rid);

        if (!cfg->used_flag) {
            continue;
        }

        ses = get_in_session(rid);
        boolean show_flag = FALSE;

        switch (ses->state & SESSION_STATE_TRAFFIC_MASK) {

        case SESSION_STATE_OFF:
            show_flag = all_flag || off_flag;
            off_cnt++;
            break;

        case SESSION_STATE_ACTIVE:
            show_flag = all_flag || active_flag;
            active_cnt++;
            total_rate += ses->avg_bitrate;
            break;

        case SESSION_STATE_IDLE:
            idle_cnt++;
            show_flag = all_flag || idle_flag;
            break;

        case SESSION_STATE_INIT:
            init_cnt++;
            show_flag = all_flag;
            break;

        default:
            fprintf(ccb->fp, "In %d: Illegal session state %d\n",
                    ses->id, ses->state);
        }

        if (show_flag) {
            fprintf(ccb->fp, "  In %d: %s, ",
                    rid, get_in_session_state_label(ses->state));
            show_in_session_config(ccb->fp, cfg);
        }

        total_cnt++;
    }

    fprintf(ccb->fp, "Summary: %d active, %d idle, %d off, %d init, "
                     "total: %d, %lld bps\n",
            active_cnt, idle_cnt, off_cnt, init_cnt, total_cnt, total_rate);
}


static void exec_video_show_in_session (cli_control_block *ccb)
{
    uint32 rid = ccb->data[3].i32;
    in_session_t *ses;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (rid >= MAX_SESSIONS) {
        fprintf(ccb->fp, "Error: Session id %d out of range\n", rid);
        return;
    }

    ses = get_in_session(rid);
    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Input session %d not used\n", rid);
        return;
    }

    fprintf(ccb->fp, "Config:\n");
    fprintf(ccb->fp, "  In %d: owner %d, %s, ",
            rid, ses->cfg->owner_id, get_in_session_state_label(ses->state));
    show_in_session_config(ccb->fp, ses->cfg);

    fprintf(ccb->fp, "\n");
    show_in_session_out_list(ccb->fp, ses);

    fprintf(ccb->fp, "\nStatistics:\n");
    show_in_session_stat(ccb->fp, ses);
    show_in_session_time(ccb->fp, ses);
    clear_in_session_time(ses);
    fprintf(ccb->fp, "  Src IP change: %d\n", ses->src_ip_change_cnt);
    show_ip_flow(ccb->fp, rid);
    fprintf(ccb->fp, "\nTP info buffer: size %d\n", ses->tp_info_buf.size);
    fprintf(ccb->fp, "Ses group: %d\n\n", ses->group_id);

    in_pid_table_print(ccb->fp, ses->pid_tab);

    fprintf(ccb->fp, "\n");
    if (ses->pat_snooped) {
        fprintf(ccb->fp, "PAT: ver %d, %d sects, %d progs\n",
                ses->pat_ver, ses->pat.sect_cnt, ses->pat.prog_cnt);
    } else {
        fprintf(ccb->fp, "PAT: not snooped\n");
    }

    fprintf(ccb->fp, "\n");
    show_in_session_prog_list(ccb->fp, ses);

    fprintf(ccb->fp, "\n");
    show_in_session_pcr_context_table(ccb->fp, ses);

    fprintf(ccb->fp, "\n");
}


static void exec_video_show_out_sessions (cli_control_block *ccb)
{
    int rid;
    int total_cnt = 0;
    int off_cnt = 0;
    int active_cnt = 0;
    int idle_cnt = 0;
    int init_cnt = 0;
    int show_flag = 0;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (ccb->fooid == CLI_VIDEO_SHOW_OUT_SESSIONS) {
         show_flag = 1;
    }

    fprintf(ccb->fp, "Out sessions:\n");

    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        out_config_t* cfg = get_out_session_config(video_ctx, rid);
        out_session_t* ses = get_out_session(rid);

        if ((cfg == NULL) || (ses == NULL)) {
            fprintf(ccb->fp, "VIDEO: NULL cfg/sess ptr");
            return;
        }

        if (!cfg->used_flag) {
            continue;
        }

        if (show_flag) {
            fprintf(ccb->fp, "  Out %d: owner %d, ", rid, cfg->owner_id);
            show_out_session_config(ccb->fp, cfg);
        }

        if (ses->in_ses == NULL) {
           fprintf(ccb->fp, "VIDEO: in_ses NULL ptr");
           return;
        }

        switch (ses->in_ses->state & SESSION_STATE_TRAFFIC_MASK) {

        case SESSION_STATE_OFF:
            off_cnt++;
            break;

        case SESSION_STATE_ACTIVE:
            active_cnt++;
            break;

        case SESSION_STATE_IDLE:
            idle_cnt++;
            break;

        case SESSION_STATE_INIT:
            init_cnt++;
            break;

        default:
            break;
        }

        total_cnt++;
    }

    fprintf(ccb->fp, "Summary: %d active, %d idle, %d off, %d init, total %d\n",
            active_cnt, idle_cnt, off_cnt, init_cnt, total_cnt);
}


static void exec_video_show_out_session (cli_control_block *ccb)
{
    uint32 rid = ccb->data[3].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (rid >= MAX_SESSIONS) {
        fprintf(ccb->fp, "Error: Session id %d out of range\n", rid);
        return;
    }

    out_session_t* ses = get_out_session(rid);
    if (ses == NULL) {
        fprintf(ccb->fp, "Error: ses is NULL\n");
        return;
    }

    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Output session %d not used\n", rid);
        return;
    }

    out_config_t* cfg = ses->cfg;

    fprintf(ccb->fp, "Config:\n");
    fprintf(ccb->fp, "  Out %d: ", rid);
    show_out_session_config(ccb->fp, cfg);
    show_out_session_pid_list(ccb->fp, cfg);
    show_out_session_program_list(ccb->fp, cfg);

    fprintf(ccb->fp, "\nStatistics:\n");
    show_out_session_stat(ccb->fp, ses);
    show_out_session_time(ccb->fp, ses);
    clear_in_session_time(ses->in_ses);

    fprintf(ccb->fp, "\n");
    out_pid_table_print(ccb->fp, ses->pid_tab);

    fprintf(ccb->fp, "\n");
    show_out_session_prog_list(ccb->fp, ses);

    fprintf(ccb->fp, "\n");
    show_out_session_psi(ccb->fp, ses);
    fprintf(ccb->fp, "\n");
}


static void video_show_session (FILE *fp, out_session_t *ses)
{
    fprintf(fp, "Ses %d: %d -> %d, %s, ",
            ses->cfg->client_id, ses->cfg->in_ses_id, ses->id,
            get_in_session_state_label(ses->in_ses->state));
    show_in_session_cast_type(fp, ses->in_ses->cfg);
    fprintf(fp, ", qam %d, ", ses->qam_id);
    show_out_session_process_type(fp, ses->cfg);
    fprintf(fp, "\n");
}


static void exec_video_show_session_range (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int start_id, stop_id, inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &start_id, &stop_id, &inc);
    if (rc == -1 || inc != 1 || start_id >= stop_id) {
        fprintf(ccb->fp, "Bad session id range: %s\n", ccb->tokens[3]);
    }

    int tot_ses_cnt = 0;
    int ses_cnt[SESSION_STATE_MAX];
    int i;
    for (i=0; i<SESSION_STATE_MAX; i++) {
        ses_cnt[i] = 0;
    }
    int64 tot_in_bitrate = 0;
    int64 tot_out_bitrate = 0;

    int ses_id;
    for (ses_id=start_id; ses_id<=stop_id; ses_id++) {
        uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
        if (out_id == INVALID_SES_ID) {
            continue;
        }

        out_session_t* ses = get_out_session(out_id);
        video_show_session(ccb->fp, ses);

        int traffic_state = ses->in_ses->state & SESSION_STATE_TRAFFIC_MASK;
        ses_cnt[traffic_state]++;
        tot_ses_cnt++;
        if (traffic_state == SESSION_STATE_ACTIVE) {
            tot_in_bitrate += ses->in_ses->avg_bitrate;
            tot_out_bitrate += ses->avg_bitrate;
        }
    }

    fprintf(ccb->fp, "Summary: %d active, %d idle, %d off, %d init, total %d\n",
            ses_cnt[SESSION_STATE_ACTIVE], ses_cnt[SESSION_STATE_IDLE],
            ses_cnt[SESSION_STATE_OFF], ses_cnt[SESSION_STATE_INIT],
            tot_ses_cnt);
    fprintf(ccb->fp, "Bitrate: in %lld, out %lld\n",
            tot_in_bitrate, tot_out_bitrate);
}


static void video_show_sessions (FILE *fp, boolean show_flag)
{
    int i;
    int tot_ses_cnt = 0;
    int ses_cnt[SESSION_STATE_MAX];
    for (i=0; i<SESSION_STATE_MAX; i++) {
        ses_cnt[i] = 0;
    }
    int64 tot_in_bitrate = 0;
    int64 tot_out_bitrate = 0;

    for (i=0; i<MAX_SESSIONS; i++) {
        out_session_t* ses = get_out_session(i);
        assert(ses);
        if (session_is_used(ses->state)) {
            if (show_flag) {
                video_show_session(fp, ses);
            }

            in_session_t* in_ses = ses->in_ses;
            assert(in_ses);
            int traffic_state = in_ses->state & SESSION_STATE_TRAFFIC_MASK;
            ses_cnt[traffic_state]++;
            tot_ses_cnt++;
            if (traffic_state == SESSION_STATE_ACTIVE) {
                tot_in_bitrate += in_ses->avg_bitrate;
                tot_out_bitrate += ses->avg_bitrate;
            }
        }
    }

    fprintf(fp, "Summary: %d active, %d idle, %d off, %d init, total %d\n",
            ses_cnt[SESSION_STATE_ACTIVE], ses_cnt[SESSION_STATE_IDLE],
            ses_cnt[SESSION_STATE_OFF], ses_cnt[SESSION_STATE_INIT],
            tot_ses_cnt);
    fprintf(fp, "Bitrate: in %lld, out %lld\n",
            tot_in_bitrate, tot_out_bitrate);
}


static void exec_video_show_sessions (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    video_show_sessions(ccb->fp, TRUE);
}


static void exec_video_show_session_summary (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    video_show_sessions(ccb->fp, FALSE);
}


static void exec_video_show_session (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    uint32 ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "Session %d not exists\n", ses_id);
        return;
    }
    out_session_t* ses = get_out_session(out_id);
    assert(ses);
    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Session %d not used\n", ses_id);
        return;
    }

    video_show_session(ccb->fp, ses);

    in_session_t* in_ses = ses->in_ses;
    if (in_ses) {
        fprintf(ccb->fp, "\nIn config:\n");
        fprintf(ccb->fp, "  In %d: owner %d, %s, ",
                in_ses->id, in_ses->cfg->owner_id,
                get_in_session_state_label(in_ses->state));
        show_in_session_config(ccb->fp, in_ses->cfg);

        fprintf(ccb->fp, "\nIn statistics:\n");
        show_in_session_stat(ccb->fp, in_ses);
        show_in_session_time(ccb->fp, in_ses);
        clear_in_session_time(in_ses);
        fprintf(ccb->fp, "  Src IP change: %d\n", in_ses->src_ip_change_cnt);
        show_ip_flow(ccb->fp, in_ses->id);
    }

    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "\nOut session %d not used\n", ses->id);
        return;
    }

    out_config_t* cfg = ses->cfg;
    fprintf(ccb->fp, "\nOut config:\n");
    fprintf(ccb->fp, "  Out %d: owner %d, ", ses->id, cfg->owner_id);
    show_out_session_config(ccb->fp, cfg);
    show_out_session_pid_list(ccb->fp, cfg);
    show_out_session_program_list(ccb->fp, cfg);
    if ((ses->state & SESSION_STATE_BLOCKED)) {
        fprintf(ccb->fp, "  CONFLICT detected\n");
    }

    fprintf(ccb->fp, "\nOut statistics:\n");
    show_out_session_stat(ccb->fp, ses);
    show_out_session_time(ccb->fp, ses);
    if (in_ses) {
        fprintf(ccb->fp, "  TP info buffer: size %d, rd %d, wr %d\n\n",
                in_ses->tp_info_buf.size, in_ses->tp_info_ref.idx,
                ses->tp_info_ref.idx);
    }
}


static void exec_video_show_session_in_pcr (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "\nSession %d not used\n", ses_id);
        return;
    }

    out_session_t* ses = get_out_session(out_id);
    fprintf(ccb->fp, "Ses %d: in %d, out %d\n\n",
            ses_id, ses->cfg->in_ses_id, out_id);

    in_session_t* in_ses = ses->in_ses;
    if (!session_is_used(in_ses->state)) {
        fprintf(ccb->fp, "Input session %d not used\n", in_ses->id);
        return;
    }

    show_in_session_pcr_context_table(ccb->fp, in_ses);
}


static void exec_video_show_session_in_psi (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "\nSession %d not used\n", ses_id);
        return;
    }

    out_session_t* ses = get_out_session(out_id);
    fprintf(ccb->fp, "Ses %d: in %d, out %d\n\n",
            ses_id, ses->cfg->in_ses_id, out_id);

    in_session_t* in_ses = ses->in_ses;
    if (!session_is_used(in_ses->state)) {
        fprintf(ccb->fp, "Input session %d not used\n", in_ses->id);
        return;
    }

    show_in_session_prog_list(ccb->fp, in_ses);
    fprintf(ccb->fp, "\n");
    show_in_session_pat(ccb->fp, in_ses);
    show_in_session_pmt(ccb->fp, in_ses);
}


static void exec_video_show_session_in_pid (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "\nSession %d not used\n", ses_id);
        return;
    }

    out_session_t* ses = get_out_session(out_id);
    fprintf(ccb->fp, "Ses %d: in %d, out %d\n\n",
            ses_id, ses->cfg->in_ses_id, out_id);

    in_session_t* in_ses = ses->in_ses;
    if (!session_is_used(in_ses->state)) {
        fprintf(ccb->fp, "Input session %d not used\n", in_ses->id);
        return;
    }

    in_pid_table_print(ccb->fp, in_ses->pid_tab);
}


static void exec_video_show_session_out_psi (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "\nSession %d not used\n", ses_id);
        return;
    }

    out_session_t* ses = get_out_session(out_id);
    fprintf(ccb->fp, "Ses %d: in %d, out %d\n\n",
            ses_id, ses->cfg->in_ses_id, out_id);

    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Output session %d not used\n", out_id);
        return;
    }

    show_out_session_prog_list(ccb->fp, ses);

    fprintf(ccb->fp, "\n");
    show_out_session_psi(ccb->fp, ses);
}


static void exec_video_show_session_out_pid (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int ses_id = ccb->data[3].i32;
    uint16 out_id = session_id_to_out_id(video_ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "\nSession %d not used\n", ses_id);
        return;
    }

    out_session_t* ses = get_out_session(out_id);
    fprintf(ccb->fp, "Ses %d: in %d, out %d\n\n",
            ses_id, ses->cfg->in_ses_id, out_id);

    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Output session %d not used\n", out_id);
        return;
    }

    out_pid_table_print(ccb->fp, ses->pid_tab);
}


static void show_crsls (FILE *fp, video_context_t *ctx)
{
    int crsl_cnt = 0;
    int i;
    for (i=0; i<NUM_CRSLS; i++) {
        crsl_t* crsl = get_crsl(ctx, i);
        if (crsl->used_flag) {
            fprintf(fp, "Carousel %d: ", i);
            print_crsl(fp, crsl);
            crsl_cnt++;
        }
    }
    fprintf(fp, "Total %d carousels\n", crsl_cnt);
}


static void exec_video_show_crsls (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    show_crsls(ccb->fp, video_ctx);
}


static void show_crsl (FILE *fp, video_context_t *ctx, uint32 rid)
{
    uint16 crsl_id = resource_id_to_crsl_id(ctx, rid);
    if (crsl_id == INVALID_CRSL_ID) {
        fprintf(fp, "Carousel resouce %d: not found\n", rid);
        return;
    }

    crsl_t* crsl = get_crsl(ctx, crsl_id);
    if (!crsl->used_flag) {
        fprintf(fp, "Carousel %d: resouce %d: not used\n", crsl_id, rid);
        return;
    }

    fprintf(fp, "Carousel %d: ", crsl_id);
    print_crsl(fp, crsl);

    int i;
    fprintf(fp, "Inserts:\n");
    for (i=0; i<NUM_QAMS; i++) {
        if (bit_mask_test(crsl->qam_mask, i)) {
            crsl_insert_t* insert
                    = find_crsl_insert(get_qam_crsl_insert_list(ctx, i), crsl);
            assert(insert);
            fprintf(fp, "  Qam %d: ", i);
            print_crsl_insert(fp, insert);
        }
    }

    print_crsl_pkt(fp, crsl);
}


static void exec_video_show_crsl (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    show_crsl(ccb->fp, video_ctx, ccb->data[3].i32);
}


static void exec_video_show_qam (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    uint32 qam_id = ccb->data[3].i32;
    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    qam_info_t* qam = get_qam_info(qam_id);
    fprintf(ccb->fp, "QAM %d: ", qam_id);
    show_qam_config(ccb->fp, qam->cfg);
    fprintf(ccb->fp, "Qam group: %d\n", qam->group_id);
    show_qam_stat(ccb->fp, qam);
    fprintf(ccb->fp, "    mux %lld, drop %lld\n",
            qam->stat.mux_tp_cnt, qam->stat.drop_tp_cnt);
}


static void exec_video_show_qam_summary (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    int ready_cnt = 0;
    int encrypt_cnt = 0;
    int64 tot_bitrate = 0;

    int i;
    for (i=0; i<NUM_QAMS; i++) {
        qam_info_t* qam = get_qam_info(i);
        if (qam_channel_ready(qam->id)) {
            ready_cnt++;
            tot_bitrate += qam->stat.bitrate;
        }
        if (qam->cfg->encrypt_flag)  encrypt_cnt++;
    }

    fprintf(ccb->fp, "QAM summary: %d ready, %d encrypted, total %lld bps\n",
            ready_cnt, encrypt_cnt, tot_bitrate);
}


static void exec_video_show_qam_crsl (cli_control_block *ccb)
{
    int cnt = 0;
    crsl_insert_t *insert;

    FOR_ALL_ELEMENTS_IN_QUE(
            get_qam_crsl_insert_list(video_ctx, ccb->data[3].i32), insert) {
        if (insert->ecm_flag) {
            ecm_info_t* ecm_info = get_ecm_info((uintptr_t)insert->crsl);
            fprintf(ccb->fp, "ECM addr %016x, %u TPs, period %u",
                   ((uintptr_t)ecm_pkt_buf + ecm_info->ecm_pkt_offset),
                   ecm_info->num_tp, ecm_info->insert_period);
        } else {
            crsl_t* crsl = (crsl_t*)insert->crsl;
            fprintf(ccb->fp, "Carousel %d: ", (int)(crsl - video_ctx->crsl));
            print_crsl(ccb->fp, crsl);
        }
        cnt++;
    }

    fprintf(ccb->fp, "Total %d carousels\n", cnt);
}


static void exec_video_show_qam_pat_pkt (cli_control_block *ccb)
{
    uint32 qam_id = ccb->data[3].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    //print_crsl_insert(ccb->fp, &get_qam_info(qam_id)->pat_insert);
}


static void exec_video_show_owner (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    uint16 i;
    uint32 owner_id = ccb->data[3].i32;

    fprintf(ccb->fp, "Sessions owned by owner %d:\n", owner_id);
    int cnt = 0;
    for (i = 0; i<MAX_SESSIONS; i++) {
        out_config_t* out_cfg = get_out_session_config(video_ctx, i);
        if (out_cfg->used_flag && out_cfg->owner_id == owner_id) {
            fprintf(ccb->fp, "  ");
            video_show_session(ccb->fp, get_out_session(i));
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d sessions\n", cnt);

    fprintf(ccb->fp, "\nCarousels owned by owner %d:\n", owner_id);
    cnt = 0;
    for (i=0; i<NUM_CRSLS; i++) {
        crsl_t* crsl = get_crsl(video_ctx, i);
        if (crsl->used_flag && crsl->owner_id == owner_id) {
            fprintf(ccb->fp, "  Carousel %d: ", i);
            print_crsl(ccb->fp, crsl);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d carousels\n", cnt);

    fprintf(ccb->fp, "\nCarousels owned by owner %d:\n", owner_id);
    cnt = 0;
    for (i=0; i<NUM_QAMS; i++) {
        qam_config_t* qam_cfg = get_qam_config(video_ctx, i);
        if (qam_cfg->enable_flag && qam_cfg->owner_id == owner_id) {
            fprintf(ccb->fp, "  QAM %d: ", i);
            show_qam_config(ccb->fp, qam_cfg);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d qams\n", cnt);
}


static void exec_video_show_qam_flows (cli_control_block *ccb)
{
    uint32 qam_id = ccb->data[3].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    show_qam_flows(ccb->fp, get_qam_info(qam_id));
}


static void exec_video_show_qam_pid_map (cli_control_block *ccb)
{
    int i;
    int cnt = 0;
    uint32 qam_id = ccb->data[3].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    qam_pid_info_t* info = get_qam_info(qam_id)->pid_tab;

    fprintf(ccb->fp, "PIDs used in QAM %d:\n", qam_id);
    for (i=0; i<NUM_PIDS; i++, info++) {
        if (info->used) {
            fprintf(ccb->fp, "  PID %d: out %d, cw idx %d\n",
                    i, info->out_ses_id, info->cw_idx);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d PIDs used\n", cnt);
}


static void exec_video_show_qam_pid (cli_control_block *ccb)
{
    int i;
    uint32 qam_id = ccb->data[3].i32;
    uint32 pid = ccb->data[5].i32;
    out_session_t* ses = NULL;
    out_prog_t *prog = NULL;
    int32 *flow_map;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (get_qam_info(qam_id)->pid_tab[pid].used) {
        fprintf(ccb->fp, "PID %d not found in QAM %d\n", pid, qam_id);
        return;
    }

    flow_map = get_qam_flow_map(video_ctx, qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        ses = get_out_session(get_out_id(flow_map[i]));
        prog = out_prog_lookup_by_pid(&ses->prog_list, pid);
        if (prog) {
            break;
        }
    }

    if (!prog) {
        fprintf(ccb->fp, "PID %d not found in QAM %d (internal err)\n",
                pid, qam_id);
        return;
    }

    assert(ses);
    fprintf(ccb->fp, "Qam %d: PID %d used in session %d, program %d\n",
            qam_id, pid, ses->id, prog->prog_num);
}


static void exec_video_show_qam_prog_map (cli_control_block *ccb)
{
    int i;
    int cnt = 0;
    uint32 qam_id = ccb->data[3].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    qam_prog_info_t* info = get_qam_info(qam_id)->prog_tab;

    fprintf(ccb->fp, "Program numbers used in QAM %d:\n", qam_id);
    for (i=0; i<NUM_PROGS; i++, info++) {
        if (info->used) {
            fprintf(ccb->fp, "  Program %d: prog id %d\n",
                    i, info->out_prog_id);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d program used\n", cnt);
}


static void exec_video_show_qam_prog (cli_control_block *ccb)
{
    int i;
    out_session_t* ses = NULL;
    out_prog_t *prog = NULL;
    uint32 qam_id = ccb->data[3].i32;
    uint32 prog_num = ccb->data[5].i32;
    int32 *flow_map;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    // Search all sessions in qam for program with specified program number
    flow_map = get_qam_flow_map(video_ctx, qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        ses = get_out_session(get_out_id(flow_map[i]));
        prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
        if (prog) {
            break;
        }
    }
    if (!prog) {
        fprintf(ccb->fp, "Program %d does not exist in QAM %d\n",
                prog_num, qam_id);
        return;
    }

    fprintf(ccb->fp, "In %d, prog %d -> out %d, prog %d, qam %d\n",
            ses->in_ses->id, prog->in_prog->prog_num, ses->id, prog->prog_num,
            qam_id);
}


static void exec_video_show_qam_prog_pmt (cli_control_block *ccb)
{
    uint32 qam_id = ccb->data[3].i32;
    uint32 prog_num = ccb->data[5].i32;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    qam_prog_info_t* info = &get_qam_info(qam_id)->prog_tab[prog_num];
    if (!info->used) {
        fprintf(ccb->fp, "Program %d does not exist in QAM %d\n",
                prog_num, qam_id);
        return;
    }

    out_prog_t* prog = get_out_prog(info->out_prog_id);
    fprintf(ccb->fp, "Qam %d, PMT %d\n", qam_id, prog->pmt_pid);
    print_out_prog(ccb->fp, prog);
}


static void exec_video_show_qam_prog_pmt_pkt (cli_control_block *ccb)
{
    int i;
    out_session_t *ses;
    out_prog_t *prog = NULL;
    uint32 qam_id = ccb->data[3].i32;
    uint32 prog_num = ccb->data[5].i32;
    int32 *flow_map;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    // Search all sessions in qam for program with specified program number
    flow_map = get_qam_flow_map(video_ctx, qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        ses = get_out_session(get_out_id(flow_map[i]));
        prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
        if (prog) {
            break;
        }
    }
    if (!prog) {
        fprintf(ccb->fp, "Program %d does not exist in QAM %d\n",
                prog_num, qam_id);
        return;
    }

    //fprint_crsl_insert(ccb->fp, &prog->pmt_insert);
}


static void exec_video_show_qam_prog_priv (cli_control_block *ccb)
{
    int i;
    out_session_t* ses = NULL;
    out_prog_t* prog = NULL;
    uint32 qam_id = ccb->data[3].i32;
    uint32 prog_num = ccb->data[5].i32;
    psi_section_t *sect;
    in_prog_t *in_prog;
    int32 *flow_map;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    // Search all sessions in qam for program with specified program number
    flow_map = get_qam_flow_map(video_ctx, qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        ses = get_out_session(get_out_id(flow_map[i]));
        prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
        if (prog) {
            break;
        }
    }
    if (!prog) {
        fprintf(ccb->fp, "Program %d does not exist in QAM %d\n",
                prog_num, qam_id);
        return;
    }

    in_prog = prog->in_prog;

    fprintf(ccb->fp, "Qam %d, prog %d -> in %d, prog %d: private tables %d\n",
            qam_id, prog_num, ses->in_ses->id, in_prog->prog_num, 
            que_get_size(NULL, &in_prog->priv_sect_list));

    FOR_ALL_ELEMENTS_IN_QUE(&in_prog->priv_sect_list, sect) {
         psi_section_header_t* hdr = sect->sect_hdr;
         fprintf(ccb->fp, "  Table %d: sect_syntax %d, len %d\n",
                 hdr->table_id, hdr->sect_syntax, psi_get_section_size(hdr));
    }
}


static void exec_video_show_qam_prog_priv_pkt (cli_control_block *ccb)
{
    int i;
    out_session_t *ses;
    out_prog_t *prog = NULL;
    uint32 qam_id = ccb->data[3].i32;
    uint32 prog_num = ccb->data[5].i32;
    int32 *flow_map;

    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    // Search all sessions in qam for program with specified program number
    flow_map = get_qam_flow_map(video_ctx, qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        ses = get_out_session(get_out_id(flow_map[i]));
        prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
        if (prog) {
            break;
        }
    }
    if (!prog) {
        fprintf(ccb->fp, "Program %d does not exist in QAM %d\n",
                prog_num, qam_id);
        return;
    }

    print_crsl_insert(ccb->fp, &prog->priv_insert);
}


static void exec_video_show_qam_pat (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp)) {
        return;
    }

    uint32 qam_id = ccb->data[3].i32;
    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Error: QAM id %d out of range\n", qam_id);
        return;
    }

    qam_info_t* qam = get_qam_info(qam_id);
    if (qam->pat_built) {
        dump_pat(ccb->fp, &qam->pat);
    } else {
        fprintf(ccb->fp, "  PAT not built\n");
    }
}


static void video_show_qams (FILE *fp, int start_qam_id, int stop_qam_id)
{
    int i;
    for (i=start_qam_id; i<=stop_qam_id; i++) {
        fprintf(fp, "QAM %d: ", i);
        qam_info_t* qam = get_qam_info(i);
        show_qam_config(fp, qam->cfg);
        show_qam_stat(fp, qam);
    }
}


static void exec_video_show_qams (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;
    video_show_qams(ccb->fp, 0, NUM_QAMS-1);
}


static void exec_video_show_qam_range (cli_control_block *ccb)
{
    if (!check_lcred_state(ccb->fp))  return;

    int start_id, stop_id, inc;
    int rc = cli_parse_range_int(ccb->tokens[3], &start_id, &stop_id, &inc);
    if (rc == -1 || start_id >= stop_id || inc != 1
            || start_id < 0 || stop_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Bad session id range: %s\n", ccb->tokens[3]);
        return;
    }

    video_show_qams(ccb->fp, start_id, stop_id);
}


static void exec_video_show_config_map_session (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;

    int i;
    for (i=0; i<NUM_INPUT_PORTS; i++) {
        fprintf(ccb->fp, "\nInput port %d:\n", i);
        hash_table_print(ccb->fp, ctx->ses_hash_tab[i]);
    }
}


static void exec_video_show_config_map_qam (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;
    uint32 qam_id = ccb->data[6].i32;
    if (qam_id >= NUM_QAMS) {
        fprintf(ccb->fp, "Bad qam ID: %d\n", qam_id);
        return;
    }
    show_qam_map(ccb->fp, ctx, qam_id);
}


static void exec_video_show_config_in_sessions (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;

    int rid;
    fprintf(ccb->fp, "Input sessions:\n");
    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        in_config_t* cfg = get_in_session_config(ctx, rid);
        if (cfg->used_flag) {
            fprintf(ccb->fp, "  In %d: ", rid);
            show_in_session_config(ccb->fp, cfg);
            fprintf(ccb->fp, "\n");
        }
    }
    fprintf(ccb->fp, "%d input sessions\n", ctx->in_session_cnt);
}


static void exec_video_show_config_out_sessions (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;

    int rid;
    fprintf(ccb->fp, "Output sessions:\n");
    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        out_config_t* cfg = get_out_session_config(ctx, rid);
        if (cfg->used_flag) {
            fprintf(ccb->fp, "  Out %d: ", rid);
            show_out_session_config(ccb->fp, cfg);
        }
    }
    fprintf(ccb->fp, "%d output sessions\n", ctx->out_session_cnt);
}


static void exec_video_show_config_session (cli_control_block *ccb)
{
    int ctx_id = ccb->data[3].i32;
    video_context_t* ctx = check_get_video_context(ccb->fp, ctx_id);
    if (!ctx)  return;

    uint32 ses_id = ccb->data[5].i32;
    uint16 out_id = session_id_to_out_id(ctx, ses_id);
    if (out_id == INVALID_SES_ID) {
        fprintf(ccb->fp, "Context %d Session %d not used\n", ctx_id, ses_id);
        return;
    }

    out_config_t* out_cfg = get_out_session_config(ctx, out_id);

    uint16 in_id = out_cfg->in_ses_id;
    in_config_t* in_cfg = get_in_session_config(ctx, in_id);
    fprintf(ccb->fp, "Context %d Session %d: IN %d -> OUT %d\n",
            ctx_id, ses_id, in_id, out_id);

    fprintf(ccb->fp, "\nIn config:\n");
    fprintf(ccb->fp, "  In %d: ", in_id);
    show_in_session_config(ccb->fp, in_cfg);

    fprintf(ccb->fp, "\nOut config:\n");
    fprintf(ccb->fp, "  Out %d: ", out_id);
    show_out_session_config(ccb->fp, out_cfg);
    show_out_session_pid_list(ccb->fp, out_cfg);
    show_out_session_program_list(ccb->fp, out_cfg);
}


static void exec_video_show_config_qams (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;

    int i;
    for (i=0; i<NUM_QAMS; i++) {
        fprintf(ccb->fp, "QAM %d: ", i);
        show_qam_config(ccb->fp, get_qam_config(ctx, i));
    }
}


static void exec_video_show_config_crsls (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;
    show_crsls(ccb->fp, ctx);
}


static void exec_video_show_config_crsl (cli_control_block *ccb)
{
    video_context_t* ctx = check_get_video_context(ccb->fp, ccb->data[3].i32);
    if (!ctx)  return;
    show_crsl(ccb->fp, ctx, ccb->data[5].i32);
}


static void exec_video_show_config_owner (cli_control_block *ccb)
{
    uint32 ctx_id = ccb->data[3].i32;
    video_context_t* ctx = check_get_video_context(ccb->fp, ctx_id);
    if (!ctx)  return;

    uint16 i;
    uint32 owner_id = ccb->data[5].i32;

    fprintf(ccb->fp, "Sessions owned by owner %d:\n", owner_id);
    int cnt = 0;
    for (i = 0; i<MAX_SESSIONS; i++) {
        out_config_t* out_cfg = get_out_session_config(ctx, i);
        if (out_cfg->used_flag && out_cfg->owner_id == owner_id) {
            fprintf(ccb->fp, "  Session %d: IN %d -> OUT %d\n",
                    out_cfg->client_id, out_cfg->in_ses_id, i);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d sessions\n", cnt);

    fprintf(ccb->fp, "\nCarousels owned by owner %d:\n", owner_id);
    cnt = 0;
    for (i=0; i<NUM_CRSLS; i++) {
        crsl_t* crsl = get_crsl(ctx, i);
        if (crsl->used_flag && crsl->owner_id == owner_id) {
            fprintf(ccb->fp, "  Carousel %d: ", i);
            print_crsl(ccb->fp, crsl);
            cnt++;
        }
    }
    fprintf(ccb->fp, "Total %d carousels\n", cnt);
}


static void exec_video_show_cr (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Clock recovery parameters:\n");
    fprintf(ccb->fp, "  PLL: LPF %d, loop gain %d\n",
            clkrec_par.lpf_par, clkrec_par.loop_gain);
    fprintf(ccb->fp, "  Limit: freq offset %d ppm, drift rate %d ppm/hr\n",
            (int32)(clkrec_par.max_freq_adj / SCALE_FREQ_ADJ),
            clkrec_par.max_slew / SLEW_CONST);
    fprintf(ccb->fp, "  Review: periods %d, drift thres %d ms, "\
                    "adjust thres %lld ms\n",
            clkrec_par.review_period, clkrec_par.drift_thres/SCALE_MS_BASE,
            clkrec_par.adjust_thres/SCALE_MS_BASE);
    return;
}


static void exec_video_show_error (cli_control_block *ccb)
{
    int qam_id, flow_id;
    for (qam_id=0; qam_id < NUM_QAMS; qam_id++) {
        int32* flow_map = get_qam_flow_map(video_ctx, qam_id);
        for (flow_id=0; flow_id < MAX_VIDEO_FLOWS_PER_QAM; flow_id++) {
            if (flow_map[flow_id] == INVALID_ID) {
                continue;
            }

            in_session_t* ses = get_in_session(get_in_id(flow_map[flow_id]));
            if (!ses->cfg->used_flag) {
                continue;
            }

            video_in_stat_t* in_stat = &ses->stat;
            if (in_stat->cc_err_cnt || in_stat->sync_loss_cnt) {
                fprintf(ccb->fp, "Flow %d-%d: in %d, out %d, cc err %d, "\
                                 "sync loss %d\n",
                        qam_id, flow_id, get_in_id(flow_map[flow_id]),
                        get_out_id(flow_map[flow_id]), in_stat->cc_err_cnt,
                        in_stat->sync_loss_cnt);
            }
        }
    }
}


#define xstr(s)         str(s)
#define str(s)          #s

static void exec_video_show_version (cli_control_block *ccb)
{
    fprintf(ccb->fp, "SW built by %s on %s %s\n\n",
            xstr(USER), __DATE__, __TIME__);
}


static void exec_video_capture (cli_control_block *ccb)
{
// Note: synchronization issue?
    capture_flag = FALSE;
    capture_mode = ccb->fooid;
}


static void exec_video_show_capture (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Capture mode: ");

    switch (capture_mode) {
    case CLI_VIDEO_CAPTURE_OFF:
        fprintf(ccb->fp, "not configured\n");
        return;

    case CLI_VIDEO_CAPTURE_ANY:
        fprintf(ccb->fp, "any\n");
        break;

    case CLI_VIDEO_CAPTURE_DROP:
        fprintf(ccb->fp, "drop\n");
        break;

    case CLI_VIDEO_CAPTURE_SYNCLOSS:
        fprintf(ccb->fp, "sync-loss\n");
        break;

    case CLI_VIDEO_CAPTURE_CCERR:
        fprintf(ccb->fp, "cc-err\n");
        break;

    default:
        fprintf(ccb->fp, "unknown (%d)\n", capture_mode);
        return;
    }

    if (capture_flag) {
        ip_header_t *ip_hdr = (ip_header_t*)capture_buf;
        udp_header_t *udp_hdr = (udp_header_t*)(ip_hdr + 1);

        fprintf(ccb->fp, "IP hdr: ");
        fprint_hex(ccb->fp, ip_hdr, sizeof(ip_header_t));
        fprintf(ccb->fp, "UDP hdr: ");
        fprint_hex(ccb->fp, udp_hdr, sizeof(udp_header_t));
        fprintf(ccb->fp, "Payload:\n");
        fprint_hex(ccb->fp, udp_hdr+1, udp_hdr->length);

    } else {
        fprintf(ccb->fp, "Packet not captured yet\n");
    }
}


static void exec_video_show_buffer (cli_control_block *ccb)
{
    fprintf(ccb->fp, "Major data structure size:\n");
    fprintf(ccb->fp, "  video_context_t: %" PRIdPTR "\n",
            sizeof(video_context_t));
    fprintf(ccb->fp, "  in_config_t: %" PRIdPTR "\n", sizeof(in_config_t));
    fprintf(ccb->fp, "  out_config_t: %" PRIdPTR "\n", sizeof(out_config_t));
    fprintf(ccb->fp, "  qam_config_t: %" PRIdPTR "\n", sizeof(qam_config_t));
    fprintf(ccb->fp, "  in_session_t: %" PRIdPTR "\n", sizeof(in_session_t));
    fprintf(ccb->fp, "  out_session_t: %" PRIdPTR "\n", sizeof(out_session_t));
    fprintf(ccb->fp, "  in_prog_t: %" PRIdPTR "\n", sizeof(in_prog_t));
    fprintf(ccb->fp, "  out_prog_t: %" PRIdPTR "\n", sizeof(out_prog_t));
    fprintf(ccb->fp, "  qam_info_t: %" PRIdPTR "\n", sizeof(qam_info_t));
    fprintf(ccb->fp, "  pcr_context_t: %" PRIdPTR "\n", sizeof(pcr_context_t));
    fprintf(ccb->fp, "  tp_info_t: %" PRIdPTR "\n", sizeof(tp_info_t));
    fprintf(ccb->fp, "  pcr_info_t: %" PRIdPTR "\n", sizeof(pcr_info_t));
    fprintf(ccb->fp, "  crsl_t: %" PRIdPTR "\n", sizeof(crsl_t));
    fprintf(ccb->fp, "  crsl_insert_t: %" PRIdPTR "\n", sizeof(crsl_insert_t));
    fprintf(ccb->fp, "  in_pid_info_t: %" PRIdPTR "\n", sizeof(in_pid_info_t));
    fprintf(ccb->fp, "  out_pid_info_t: %" PRIdPTR "\n",
            sizeof(out_pid_info_t));
    fprintf(ccb->fp, "  qam_prog_info_t: %" PRIdPTR "\n",
            sizeof(qam_prog_info_t));
    fprintf(ccb->fp, "  qam_pid_info_t: %" PRIdPTR "\n",
            sizeof(qam_pid_info_t));
    fprintf(ccb->fp, "  video_ip_stat_t: %" PRIdPTR "\n",
            sizeof(video_ip_stat_t));
    fprintf(ccb->fp, "  video_in_stat_t: %" PRIdPTR "\n",
            sizeof(video_in_stat_t));
    fprintf(ccb->fp, "  video_out_stat_t: %" PRIdPTR "\n",
            sizeof(video_out_stat_t));

    if (lcred_role != LCRED_STANDBY_ROLE) {
        // Show pool utilization
        fprintf(ccb->fp, "\nGlobal pools:\n");
        fprintf(ccb->fp, "  In program: %d / %d\n",
               in_prog_pool.count, NUM_IN_PROGS);
        fprintf(ccb->fp, "  Out program: %d / %d\n",
               out_prog_pool.count, NUM_OUT_PROGS);
        fprintf(ccb->fp, "  TP info buffer chunks: %d / %d\n",
               tp_info_buf_chunk_pool.count, NUM_TP_INFO_BUF_CHUNKS);
        fprintf(ccb->fp, "  SPTS pcr context table: %d / %d\n",
               pcr_context_spts_pool.count, MAX_SPTS_SESSIONS);
        fprintf(ccb->fp, "  MPTS pcr context table: %d / %d\n",
               pcr_context_mpts_pool.count, MAX_MPTS_SESSIONS);
        fprintf(ccb->fp, "  PSI section: %d / %d\n",
               psi_section_pool.count, NUM_PSI_SECTIONS);
        fprintf(ccb->fp, "  Private section: %d / %d\n",
               private_section_pool.count, NUM_PRIVATE_SECTIONS);
        fprintf(ccb->fp, "  PSI TP buffer: %d / %d\n",
               psi_tp_buf_pool.count, NUM_PSI_TP_BUFS);
        if (video_ctx) {
            fprintf(ccb->fp, "  In session ID: %d / %d\n",
                   video_ctx->in_ses_id_pool.count, MAX_SESSIONS);
            fprintf(ccb->fp, "  Carousel insert: %d / %d\n",
                   video_ctx->crsl_insert_pool.count, NUM_CRSL_INSERTS);
            fprintf(ccb->fp, "  Carousel TP buffer: %d / %d\n",
                   video_ctx->crsl_tp_buf_pool.count, NUM_CRSL_TP_BUFS);
        }
    }

    fprintf(ccb->fp, "\n");
    if (video_ctx) {
        fprintf(ccb->fp, "Carousel TP buffers: vir %" PRIxPTR ", "\
                          "phy %" PRIxPTR "\n",
               (size_t)video_ctx->crsl_tp_buf_pool.buf,
               (size_t)video_ctx->crsl_tp_buf_pool.buf
                       + video_ctx->crsl_tp_buf_offset);
    }
    fprintf(ccb->fp, "PSI TP buffers: vir %" PRIxPTR ", phy %" PRIxPTR "\n",
               (size_t)psi_tp_buf_pool.buf,
               (size_t)psi_tp_buf_pool.buf + psi_tp_buf_offset);

    fprintf(ccb->fp, "In session: addr %" PRIxPTR ", count %d\n",
           (size_t)in_session, MAX_SESSIONS);
    fprintf(ccb->fp, "Out session: addr %" PRIxPTR ", count %d\n",
           (size_t)out_session, MAX_SESSIONS);
}


static void exec_video_set_monitor_timing (cli_control_block *ccb)
{
    video_monitor_timing_set(ccb->data[3].i32, ccb->data[4].i32, ccb->cli_tty);
}


static void exec_video_set_monitor_timing_off (cli_control_block *ccb __UNUSED)
{
    video_monitor_timing_clear();
}


#if VIDEO_DIAG
static void exec_video_set_qam_diag_on (cli_control_block *ccb)
{
    diag_qam_id = ccb->data[5].i32;
    diag_tp_idx = 0;
    VIDEO_DEBUG(" Diagnostic mode turned on for QAM %d\n", diag_qam_id);
}

static void exec_video_set_qam_diag_off (cli_control_block *ccb)
{
    diag_qam_id = INVALID_QAM_ID;
    fprintf(ccb->fp, "Diagnostic mode turned off\n");
    VIDEO_DEBUG(" Diagnostic mode turned off\n");
}
#endif


static void exec_video_cr_pll (cli_control_block *ccb)
{
    clkrec_par.lpf_par = ccb->data[3].i32;
    clkrec_par.loop_gain = ccb->data[4].i32;
}


static void exec_video_cr_limit (cli_control_block *ccb)
{
    clkrec_par.max_freq_adj = ccb->data[3].i32 * SCALE_FREQ_ADJ;
    clkrec_par.max_slew = ccb->data[4].i32 * SLEW_CONST;
}


static void exec_video_cr_review (cli_control_block *ccb)
{
    clkrec_par.review_period = ccb->data[3].i32;
    clkrec_par.drift_thres = ccb->data[4].i32 * SCALE_MS_BASE;
    clkrec_par.adjust_thres = ccb->data[5].i32 * SCALE_MS_BASE;
}


static void exec_video_cr_restart (cli_control_block *ccb)
{
    int rid = ccb->data[3].i32;
    in_session_lock(rid);
    pcr_context_reset(get_in_session(rid));
    in_session_unlock(rid);
}


static void exec_video_set_in_idle (cli_control_block *ccb)
{
    uint16 rid = ccb->data[3].i32;
    in_session_t* ses = get_in_session(rid);

    if (!session_is_used(ses->state)) {
        fprintf(ccb->fp, "Input session %d not used\n", rid);
        return;
    }

    VIDEO_LOG("Simulate In %d idle", rid);

    in_session_lock(rid);
    session_set_idle(&ses->state);
    in_session_unlock(rid);
}


static void exec_video_set_in_jitter (cli_control_block *ccb)
{
    in_session_t* ses = get_in_session(ccb->data[3].i32);
    ses->cfg->jitter_size = ccb->data[5].i32;
    ses->stay_time_limit = comp_stay_time_limit(ses->cfg->jitter_size);
}


static void exec_video_set_in_master_pcr_pid (cli_control_block *ccb)
{
    in_session_t* ses = get_in_session(ccb->data[3].i32);
    ses->cfg->master_pcr_pid = ccb->data[5].i32;
}


static void exec_video_set_disc_insert_enable (cli_control_block *ccb __UNUSED)
{
    disc_insert_enable = TRUE;
}


static void exec_video_set_disc_insert_enable_off (cli_control_block *ccb __UNUSED)
{
    disc_insert_enable = FALSE;
}


static void exec_video_debug (int level, int sense)
{
    if (sense) {
        trace_enable(level);
        VIDEO_DEBUG("\nDebug %d turned on\n", level);
    } else {
        trace_disable(level);
        VIDEO_DEBUG("\nDebug %d turned off\n", level);
    }
}


static void exec_video_disable_input (cli_control_block *ccb)
{
#if 0
    struct timespec start_time, end_time;
    uint32 d_time;
    
    fprintf(ccb->fp, "\nDisabling video for %dms\n", ccb->data[3].i32);
    if (clock_gettime(CLOCK_REALTIME, &start_time) == -1) {
        fprintf(ccb->fp, "WARNING: Could not get current system time\n");
        return;
    }

    disable_input_thread = TRUE;
    
    // sleep zzzzzz....
    msecdelay(ccb->data[3].i32);

    // reenable the input thread
    disable_input_thread = FALSE;
    
    if (clock_gettime(CLOCK_REALTIME, &end_time) == -1) {
        fprintf(ccb->fp, "WARNING: Could not get current system time\n");
        return;
    }
    d_time = ((end_time.tv_sec - start_time.tv_sec) * 1000 +
              (int) (end_time.tv_nsec - start_time.tv_nsec) / 1000000);

    fprintf(ccb->fp, "Enabling video after %dms\n", d_time);
#else
    fprintf(ccb->fp, "command not supported\n");
#endif
}


static void exec_video_export_config (cli_control_block *ccb)
{
    fprintf(ccb->fp, "VIDEO: export context %d configuration to file %s\n",
            ccb->data[3].i32, ccb->tokens[4]);

    video_context_t* ctx = get_video_context(ccb->data[3].i32);
    if (!ctx) {
        fprintf(ccb->fp, "Error: Context %d not initialized\n",
                ccb->data[3].i32);
        return;
    }

    int fd = open(ccb->tokens[4], O_CREAT | O_RDWR);
    if (fd == -1) {
        fprintf(ccb->fp, "Error: Failed to open output file %s\n",
                ccb->tokens[4]);
        return;
    }

    int num_ses = 0;
    int byte_cnt = write(fd, &ccb->data[3].i32, sizeof(int));
    byte_cnt += write(fd, &num_ses, sizeof(uint32));
    byte_cnt += write(fd, ctx->in_session_config,
                      sizeof(in_config_t) * MAX_SESSIONS);
    byte_cnt += write(fd, &num_ses, sizeof(uint32));
    byte_cnt += write(fd, ctx->out_session_config,
                      sizeof(out_config_t) * MAX_SESSIONS);
    close(fd);

    fprintf(ccb->fp, "%d bytes exported.\n", byte_cnt);
}


// Video CLI handler
//
boolean video_cli_handler (cli_control_block *ccb)
{

    if (ccb == NULL) {
        return FALSE;
    }

    switch (ccb->fooid) {

    case CLI_VIDEO_SHOW_BUFFER:
        exec_video_show_buffer(ccb);
        break;

    case CLI_VIDEO_SHOW_CAPTURE:
        exec_video_show_capture(ccb);
        break;

    case CLI_VIDEO_SHOW_CR:
        exec_video_show_cr(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_MAP_SESSION:
        exec_video_show_config_map_session(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_MAP_QAM:
        exec_video_show_config_map_qam(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_IN_SESSIONS:
        exec_video_show_config_in_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_OUT_SESSIONS:
        exec_video_show_config_out_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_SESSION:
        exec_video_show_config_session(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_QAMS:
        exec_video_show_config_qams(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_CRSLS:
        exec_video_show_config_crsls(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_CRSL:
        exec_video_show_config_crsl(ccb);
        break;

    case CLI_VIDEO_SHOW_CONFIG_OWNER:
        exec_video_show_config_owner(ccb);
        break;

    case CLI_VIDEO_SHOW_MAP_SESSION:
        exec_video_show_session_map(ccb);
        break;
        
    case CLI_VIDEO_SHOW_MAP_SESSION_SUMMARY:
        exec_video_show_session_map_summary(ccb);
        break;

    case CLI_VIDEO_SHOW_MAP_QAM:
        exec_video_show_map_qam(ccb);
        break;
        
    case CLI_VIDEO_SHOW_IN_SESSIONS:
    case CLI_VIDEO_SHOW_IN_SESSIONS_ACTIVE:
    case CLI_VIDEO_SHOW_IN_SESSIONS_IDLE:
    case CLI_VIDEO_SHOW_IN_SESSIONS_OFF:
    case CLI_VIDEO_SHOW_IN_SESSIONS_SUMMARY:
        exec_video_show_in_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_IN_SESSION:
        exec_video_show_in_session(ccb);
        break;

    case CLI_VIDEO_SHOW_OUT_SESSIONS:
    case CLI_VIDEO_SHOW_OUT_SESSIONS_SUMMARY:
        exec_video_show_out_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_OUT_SESSION:
        exec_video_show_out_session(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSIONS:
        exec_video_show_sessions(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_RANGE:
        exec_video_show_session_range(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_SUMMARY:
        exec_video_show_session_summary(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION:
        exec_video_show_session(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_IN_PCR:
        exec_video_show_session_in_pcr(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_IN_PSI:
        exec_video_show_session_in_psi(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_IN_PID:
        exec_video_show_session_in_pid(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_OUT_PSI:
        exec_video_show_session_out_psi(ccb);
        break;

    case CLI_VIDEO_SHOW_SESSION_OUT_PID:
        exec_video_show_session_out_pid(ccb);
        break;

    case CLI_VIDEO_SHOW_CRSLS:
        exec_video_show_crsls(ccb);
        break;

    case CLI_VIDEO_SHOW_CRSL:
        exec_video_show_crsl(ccb);
        break;

    case CLI_VIDEO_SHOW_QAMS:
        exec_video_show_qams(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_RANGE:
        exec_video_show_qam_range(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM:
        exec_video_show_qam(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_SUMMARY:
        exec_video_show_qam_summary(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_CRSL:
        exec_video_show_qam_crsl(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_FLOWS:
        exec_video_show_qam_flows(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PID_MAP:
        exec_video_show_qam_pid_map(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PID:
        exec_video_show_qam_pid(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG:
        exec_video_show_qam_prog(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG_MAP:
        exec_video_show_qam_prog_map(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG_PMT:
        exec_video_show_qam_prog_pmt(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG_PMT_PKT:
        exec_video_show_qam_prog_pmt_pkt(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG_PRIV:
        exec_video_show_qam_prog_priv(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PROG_PRIV_PKT:
        exec_video_show_qam_prog_priv_pkt(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PAT:
        exec_video_show_qam_pat(ccb);
        break;

    case CLI_VIDEO_SHOW_QAM_PAT_PKT:
        exec_video_show_qam_pat_pkt(ccb);
        break;

    case CLI_VIDEO_SHOW_OWNER:
        exec_video_show_owner(ccb);
        break;

    case CLI_VIDEO_SHOW_ERROR:
        exec_video_show_error(ccb);
        break;

    case CLI_VIDEO_SHOW_VER:
        exec_video_show_version(ccb);
        break;

    case CLI_VIDEO_CAPTURE_OFF:
    case CLI_VIDEO_CAPTURE_ANY:
    case CLI_VIDEO_CAPTURE_DROP:
    case CLI_VIDEO_CAPTURE_SYNCLOSS:
    case CLI_VIDEO_CAPTURE_CCERR:
        exec_video_capture(ccb);
        break;

    case CLI_VIDEO_CR_PLL:
        exec_video_cr_pll(ccb);
        break;

    case CLI_VIDEO_CR_LIMIT:
        exec_video_cr_limit(ccb);
        break;

    case CLI_VIDEO_CR_REVIEW:
        exec_video_cr_review(ccb);
        break;

    case CLI_VIDEO_CR_RECORD_SET:
        clkrec_record_init(ccb->fp, ccb->tokens[4], ccb->data[5].i32,
                           ccb->data[6].i32);
        fprintf(ccb->fp, "Record input session %d timing info to %s "
                         "for up to %d bytes\n",
                ccb->data[6].i32, ccb->tokens[4], ccb->data[5].i32);
        break;

    case CLI_VIDEO_CR_RECORD_START:
        clkrec_record_start();
        fprintf(ccb->fp, "Record started\n");
        break;

    case CLI_VIDEO_CR_RECORD_STOP:
        clkrec_record_stop();
        fprintf(ccb->fp, "Record stopped\n");
        break;

    case CLI_VIDEO_CR_RECORD_STATUS:
        clkrec_record_status(ccb->fp);
        break;

    case CLI_VIDEO_CR_RESTART:
        exec_video_cr_restart(ccb);
        break;

    case CLI_VIDEO_PSI_RECORD_SET:
        psi_record_init(&psi_record, ccb->tokens[4], ccb->data[5].i32);
        fprintf(ccb->fp, "Record PSI to %s for up to %d TPs\n",
                psi_record.filename, psi_record.max_tp);
        if (psi_record.fd == -1) {
            fprintf(ccb->fp,"Error: Failed to open file %s for PSI recording\n",
                    ccb->tokens[4]);
        }
        break;

    case CLI_VIDEO_PSI_RECORD_START:
        psi_record_start(&psi_record, ccb->data[4].i32);
        fprintf(ccb->fp, "Start PSI recording for IN %d\n", ccb->data[4].i32);
        break;

    case CLI_VIDEO_PSI_RECORD_STATUS:
        psi_record_status(&psi_record, ccb->fp);
        break;

    case CLI_VIDEO_SET_IN_IDLE:
        exec_video_set_in_idle(ccb);
        break;

    case CLI_VIDEO_SET_IN_JITTER:
        exec_video_set_in_jitter(ccb);
        break;

    case CLI_VIDEO_SET_IN_MASTER_PCR_PID:
        exec_video_set_in_master_pcr_pid(ccb);
        break;

    case CLI_VIDEO_SET_DISC_INSERT_ENABLE:
        exec_video_set_disc_insert_enable(ccb);
        break;

    case CLI_VIDEO_SET_DISC_INSERT_ENABLE_OFF:
        exec_video_set_disc_insert_enable_off(ccb);
        break;

    case CLI_VIDEO_SET_MONITOR_TIMING:
        exec_video_set_monitor_timing(ccb);
        break;

    case CLI_VIDEO_SET_MONITOR_TIMING_OFF:
        exec_video_set_monitor_timing_off(ccb);
        break;

#if VIDEO_DIAG
    case CLI_VIDEO_SET_QAM_DIAG_ON:
        exec_video_set_qam_diag_on(ccb);
        break;

    case CLI_VIDEO_SET_QAM_DIAG_OFF:
        exec_video_set_qam_diag_off(ccb);
        break;
#endif

    case CLI_VIDEO_DEBUG_MSG:
        exec_video_debug(1, 1);
        break;

    case CLI_VIDEO_DEBUG_MSG_OFF:
        exec_video_debug(1, 0);
        break;

    case CLI_VIDEO_DEBUG_EVENT:
        exec_video_debug(2, 1);
        break;

    case CLI_VIDEO_DEBUG_EVENT_OFF:
        exec_video_debug(2, 0);
        break;

    case CLI_VIDEO_DEBUG_IN:
        exec_video_debug(3, 1);
        break;

    case CLI_VIDEO_DEBUG_IN_OFF:
        exec_video_debug(3, 0);
        break;

    case CLI_VIDEO_DEBUG_OUT:
        exec_video_debug(4, 1);
        break;

    case CLI_VIDEO_DEBUG_OUT_OFF:
        exec_video_debug(4, 0);
        break;

    case CLI_VIDEO_DEBUG_PSI:
        exec_video_debug(5, 1);
        break;

    case CLI_VIDEO_DEBUG_PSI_OFF:
        exec_video_debug(5, 0);
        break;

    case CLI_VIDEO_DEBUG_CLKREC:
        exec_video_debug(6, 1);
        break;

    case CLI_VIDEO_DEBUG_CLKREC_OFF:
        exec_video_debug(6, 0);
        break;

    case CLI_VIDEO_DEBUG_THREAD:
        exec_video_debug(7, 1);
        break;

    case CLI_VIDEO_DEBUG_THREAD_OFF:
        exec_video_debug(7, 0);
        break;

    case CLI_VIDEO_DEBUG_CA:
        exec_video_debug(8, 1);
        break;

    case CLI_VIDEO_DEBUG_CA_OFF:
        exec_video_debug(8, 0);
        break;

    case CLI_VIDEO_DEBUG_ELCARO_IN_CHECK:
        video_db_in_cnfg_diag(ccb->fp);
        break;

    case CLI_VIDEO_DEBUG_ELCARO_OUT_CHECK:
        video_db_out_cnfg_diag(ccb->fp);
        break;

    case CLI_VIDEO_DEBUG_ELCARO_QAM_CHECK:
        video_db_qam_cnfg_diag(ccb->fp);
        break;

    case CLI_VIDEO_DEBUG_ELCARO_CRSL_CHECK:
        video_db_crsl_cnfg_diag(ccb->fp);
        break;

    case CLI_VIDEO_DISABLE_INPUT:
        exec_video_disable_input(ccb);
        break;

    case CLI_VIDEO_EXPORT_CONFIG:
        exec_video_export_config(ccb);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

