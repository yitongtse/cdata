///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"
#include "video.h"
#include "video_util.h"
#include "video_messages.h"
#include "video_event.h"
#include "sim_video.h"


uint8 video_msg_buf[VIDEO_MSG_BUF_SIZE];
uint8 video_rsp_buf[VIDEO_MSG_BUF_SIZE];

uint32 video_msg_cnt = 0;


static
int sim_in_session_dealloc (video_context_t *ctx, uint16 ses_id)
{
    int rc;
    boolean ctx_active = (video_ctx == ctx);
    in_config_t *cfg = get_in_session_config(ctx, ses_id);
    
    if (cfg->ref_cnt != 0) {
        VIDEO_LOG("IN %d: DEALLOC: Bad ref cnt %d", ses_id,  cfg->ref_cnt);
    }
    
    if (ctx_active) {
        in_session_t* ses = get_in_session(ses_id);

        in_session_lock(ses_id);

        // Mark the session not used so input thread will not process it
        ses->state &= ~SESSION_STATE_USED;

        // Clean up in session
        in_session_cleanup(ses);

        in_session_unlock(ses_id);
    }

    rc = unprovision_in_session(ctx, &ses_id, cfg);

    // Mark the in session free
    cfg->used_flag = 0;
    
    ctx->in_session_cnt--;

    // Free the corresponding in session id
    pool_free(&ctx->in_ses_id_pool, get_in_session_id(ctx, ses_id));

    if (ctx_active) {
        // Accumulate the statistics
        video_in_stat_update(&video_in_stat_history,
                             &get_in_session(ses_id)->stat);
    }
    return rc;
}   


static
out_config_t* sim_out_session_alloc (video_context_t *ctx, uint16 ses_id,
                                     const char *label, int *rc)
{
    out_config_t *cfg;

    if (ses_id >= MAX_SESSIONS) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_BAD_OUT_SESSION_ID, label, ses_id);
        *rc = EINVAL;
        return NULL;
    }

    cfg = get_out_session_config(ctx, ses_id);
    if (!cfg) {
        *rc = EINVAL;
        return NULL;
    }
    if (cfg->used_flag) {
        // Note: we accept this condition as a handled error case
        if (cfg->in_ses_id != (uint16)INVALID_ID) {
            // The out session already is in use - active output session.
            // do not reuse if in_seesion is associated.
            VIDEO_LOG("ERR_HANDLE: %s: Out %d associated with in_session %d",
                      label, ses_id, cfg->in_ses_id);
            *rc = EEXIST;
            return NULL;
        } else {
            // Error handling: Just create the out session
            VIDEO_LOG("ERR_HANDLE: %s: Out %d reused", label, ses_id);
            out_session_dealloc(ctx, ses_id);
        }
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->in_ses_id = (uint16)INVALID_ID;
    cfg->used_flag = 1;

    ctx->out_session_cnt++;

    return cfg;
}



// Handler for video add session message
void sim_process_video_add_session (ipc_message *ipc_msg)
{
    int rc = EOK;
    uint16 out_id = INVALID_SES_ID;
    out_config_t* out_cfg = NULL;
    in_config_t* in_cfg = NULL;

    video_msg_cnt++;
    video_add_session_msg_t *cmd = IPC_DATA(ipc_msg);

    VIDEO_MSG_DEBUG("ADD_SES: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    video_context_t* ctx = get_config_context(cmd->primary_id, "ADD_IN", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    // Make sure sesion_id is not currently used
    out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id != INVALID_SES_ID) {
        printf("Error: session ID %d already used\n", cmd->resource_id);
        out_id = INVALID_SES_ID;
        rc = EEXIST;
        goto done;
    }

    // Allocate out ID
    out_id = session_id_alloc(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        printf("Error: Failed to allocate out session ID\n");
        rc = ENOMEM;
        goto done;
    }

    // Setup output config
    out_cfg = sim_out_session_alloc(ctx, out_id, "ADD_SES", &rc);
    if (!out_cfg)  goto done;
    out_cfg->client_id = cmd->resource_id;

    // Setup input config
    uint16 in_id = INVALID_SES_ID;
    in_cfg = in_session_alloc(ctx, &in_id, "ADD_SES", &rc);
    if (!in_cfg)  goto done;

    in_cfg->ref_cnt = 1;
    out_cfg->in_ses_id = in_id;

    // Process message
    rc = process_video_add_session_msg(cmd, in_cfg, out_cfg);
    if (rc != EOK) {
        rc = -1;
        goto done;
    }

    // Sanity check
    // TODO: need to improve this!!
    if (out_cfg->prog_cnt) {
        uint16 prog_num = out_cfg->out_prog_list[0];

        if (!in_cfg->pid_remap_flag) {
            errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_UNEXPECTED_PROG_NUM,
                   "ADD_SES_PASSTHRU", out_id, prog_num);
            rc = EINVAL;
            goto done;
        }

        // Check if the program number is already used in the QAM
        rc = qam_config_program_check(ctx, out_cfg->qam_id, prog_num);
        if (rc != EOK) {
            errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_PROG_NUM_IN_USE,
                   "ADD_SES", out_id, out_cfg->qam_id, prog_num);
            goto done;
        }

    } else {
        if (in_cfg->pid_remap_flag && !in_cfg->mpts_flag) {
            errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_MISSING_PROG_NUM,
                   "ADD_SES_REMAP", out_id);
            rc = EINVAL;
            goto done;
        }
    }

    // Provision input session
    rc = provision_in_session(ctx, &in_id, in_cfg);
    if (rc == EEXIST) {
        // In session already exists, free the new in_cfg just allocated
        in_cfg->used_flag = 0;
        pool_free(&ctx->in_ses_id_pool,
                  get_in_session_id(ctx, in_cfg - ctx->in_session_config));
        ctx->in_session_cnt--;
        out_cfg->in_ses_id = (uint16)INVALID_ID;

        in_cfg = get_in_session_config(ctx, in_id);
        if (in_cfg->input_type == INPUT_TYPE_MULTICAST_ASM ||
                in_cfg->input_type == INPUT_TYPE_MULTICAST_SSM) {
            rc = EOK;
            VIDEO_DEBUG(": reuse in %d", in_id);

            in_cfg->ref_cnt++;
            out_cfg->in_ses_id = in_id;
        }
    }

    if (rc != EOK) {
        goto done;
    }

    // Provision the QAM flow
    rc = provision_qam_flow(ctx, out_id, out_cfg);
    if (rc != EOK) {
        goto done;
    }

    if (ctx != video_ctx) {
        goto done;
    }

    // Input/output session state setup
    //   TODO: need to check error path!!
    in_session_t* in_ses = get_in_session(in_id);
    if (in_cfg->ref_cnt == 1) {
        rc = in_session_init(ctx, (uint16)in_id);
        if (rc != EOK) {
            errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_IN_SESSION_INIT_FAILED,
                   "ADD_SES", in_id, strerror(rc));
            in_ses->state |= SESSION_STATE_NO_RESOURCES;
        }
    }

    out_session_t* out_ses = get_out_session(out_id);
    int rc2 = out_session_init(ctx, out_id, in_id);
    if (rc2 != EOK) {
        errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_OUT_SESSION_INIT_FAILED,
               "ADD_SES", out_id, strerror(rc));
        out_ses->state |= SESSION_STATE_NO_RESOURCES;
    }

    if (rc != EOK && rc2 != EOK) {
        // Accept the transaction, but do not mark the in/out sessions as used.
        // TODO: need to report it as config error!
        rc = EOK;
        goto done;
    }

    // Mark the in/out session used
    in_ses->state |= SESSION_STATE_USED;
    out_ses->state |= SESSION_STATE_USED;

done:
    VIDEO_DEBUG(": status %d\n", rc);
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);

    if (rc == EOK)  return;

    // Send video event
    video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, 0, 0,
                                cmd->resource_id, rc, 0, NULL);

    // Clean up for error cases
    if (out_id != INVALID_SES_ID && out_cfg) {
        out_cfg->used_flag = 0;
        ctx->out_session_cnt--;

        if (in_cfg && (out_cfg->in_ses_id != INVALID_SES_ID)
                && (--in_cfg->ref_cnt == 0)) {
            pool_free(&ctx->in_ses_id_pool, get_in_session_id(ctx, in_id));
            in_cfg->used_flag = 0;
            ctx->in_session_cnt--;
        }
        session_id_free(ctx, cmd->resource_id);
    }
}


// Handler for video delete session message
static
void sim_process_video_delete_session (ipc_message *ipc_msg)
{
    int rc;
    video_msg_cnt++;
    video_delete_session_msg_t *cmd = IPC_DATA(ipc_msg);

    VIDEO_MSG_DEBUG("DELETE_SES: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    video_context_t* ctx = get_config_context(cmd->primary_id, "DELETE_IN",
                                              &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint16 out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        printf("Error: session ID %d not used\n", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    // Check resource state
    out_config_t* out_cfg = get_out_session_config(ctx, out_id);
    if (!out_cfg->used_flag) {
        // Note: we accept this condition as a handled error case
        // Error handling: simply skip the delete operation
        VIDEO_LOG("ERR_HANDLE: DELETE_SES: Out %d removed", out_id);
        goto done;
    }

    uint16 in_id = out_cfg->in_ses_id;
    in_config_t* in_cfg = get_in_session_config(ctx, in_id);

    // Delete out session
    rc = out_session_dealloc(ctx, out_id);
    if (rc != EOK) {
        goto done;
    }

    // clean up hash table and deallocate in session
    if (in_cfg->ref_cnt == 0) {
        sim_in_session_dealloc(ctx, in_id);
    }

    // Free sesion ID
    session_id_free(ctx, cmd->resource_id);

done:
    VIDEO_DEBUG(": status %d\n", rc);
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);

    if (rc != EOK) {
        // Send video event
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, 0, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


static
void sim_process_video_add_crsl (ipc_message *msg)
{
    int rc = EOK;
    uint16 crsl_id = INVALID_CRSL_ID;

    video_add_crsl_msg_t *cmd = IPC_DATA(msg);
    VIDEO_MSG_DEBUG("\n\nADD_CRSL: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    video_context_t* ctx = get_config_context(cmd->primary_id, "ADD_CRSL", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    // Make sure resource_id is not currently used
    crsl_id = resource_id_to_crsl_id(ctx, cmd->resource_id);
    if (crsl_id != INVALID_CRSL_ID) {
        VIDEO_DEBUG("\nError: resource ID %d already used", cmd->resource_id);
        crsl_id = INVALID_CRSL_ID;
        rc = EEXIST;
        goto done;
    }

    // Allocate crsl ID
    crsl_id = crsl_id_alloc(ctx, cmd->resource_id);
    if (crsl_id == INVALID_CRSL_ID) {
        printf("Error: Failed to allocate crsl ID\n");
        rc = ENOMEM;
        goto done;
    }

    rc = process_video_add_crsl_msg(cmd, ctx, crsl_id);
    if (rc != EOK) {
        rc = -1;
        goto done;
    }

done:
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);
    if (rc == EOK)  return;

    // Error cleanup
    if (crsl_id != INVALID_CRSL_ID) {
        crsl_t* crsl = get_crsl(ctx, crsl_id);
        if (crsl->num_qam == 0) {
            pool_free_list(&ctx->crsl_tp_buf_pool, &crsl->tp_list);
            crsl->used_flag = 0;
        }
        crsl_id_free(ctx, cmd->resource_id);

        video_event_fire_config_err(IPC_HEADER(msg)->type, 0, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


static
void sim_process_video_delete_crsl (ipc_message *msg)
{
    int rc = EOK;

    video_delete_crsl_msg_t *cmd = IPC_DATA(msg);
    VIDEO_MSG_DEBUG("\n\nDELETE_CRSL: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    video_context_t* ctx = get_config_context(cmd->primary_id, "DELETE_CRSL",
                                              &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    // Make sure resource_id is currently used
    uint16 crsl_id = resource_id_to_crsl_id(ctx, cmd->resource_id);
    if (crsl_id == INVALID_CRSL_ID) {
        VIDEO_DEBUG("\nError: resource ID %d not used", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    int i;
    crsl_t* crsl = get_crsl(ctx, crsl_id);

    if (cmd->num_qam) {
        for (i=0; i<cmd->num_qam; i++) {
            crsl_insert_delete(ctx, crsl, cmd->qam_id[i]);
        }
    } else {
        // Delete insertions in all QAMs
        for (i=0; i<NUM_QAMS; i++) {
            if (bit_mask_test(crsl->qam_mask, i)) {
                crsl_insert_delete(ctx, crsl, i);
            }
        }
    }

    if (crsl->num_qam == 0) {
        // Free the carousel
        pool_free_list(&ctx->crsl_tp_buf_pool, &crsl->tp_list);
        crsl->used_flag = 0;
        crsl_id_free(ctx, cmd->resource_id);
    }

done:
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);
    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(msg)->type, 0, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


// Handler for video delete owner message
static
void sim_process_video_delete_owner (ipc_message *ipc_msg)
{
    int rc = EOK;

    video_delete_owner_msg_t *cmd = IPC_DATA(ipc_msg);
    VIDEO_MSG_DEBUG("DELETE_OWNER: primary %d, transaction %d, owner %d",
                    cmd->primary_id, cmd->transaction_id, cmd->owner_id);

    video_context_t* ctx = get_config_context(cmd->primary_id, "DELETE_IN",
                                              &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint32 owner_id = cmd->owner_id;

    // Delete all out sessions from the owner
    uint16 out_id;
    for (out_id = 0; out_id < MAX_SESSIONS; out_id++) {

        out_config_t* out_cfg = get_out_session_config(ctx, out_id);
        if (!out_cfg->used_flag)  continue;
        if (out_cfg->owner_id != owner_id)  continue;

        uint16 in_id = out_cfg->in_ses_id;
        in_config_t* in_cfg = get_in_session_config(ctx, in_id);

        session_id_free(ctx, out_cfg->client_id);
        rc = out_session_dealloc(ctx, out_id);
        assert(rc == EOK);

        if (in_cfg->ref_cnt == 0) {
            sim_in_session_dealloc(ctx, in_id);
        }
    }

    // Delete all carousels from the owner
    uint16 crsl_id;
    for (crsl_id=0; crsl_id < NUM_CRSLS; crsl_id++) {

        crsl_t* crsl = get_crsl(ctx, crsl_id);
        if (!crsl->used_flag)  continue;
        if (crsl->owner_id != owner_id)  continue;

        // Remove all carousel inserts
        uint16 qam_id;
        for (qam_id = 0; qam_id < NUM_QAMS; qam_id++) {
            if (bit_mask_test(crsl->qam_mask, qam_id)) {
                crsl_insert_delete(ctx, crsl, qam_id);
                if (crsl->num_qam == 0)  break;
            }
        }

        // Remove carousel
        crsl_id_free(ctx, crsl->resource_id);
        pool_free_list(&ctx->crsl_tp_buf_pool, &crsl->tp_list);
        crsl->used_flag = 0;
    }

done:
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);
    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, 0, 0,
                                    cmd->owner_id, rc, 0, NULL);
    }
}


void analyzer_stat_update (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    // Collect program statistics
    in_session_t* ses = get_in_session(0);
    pat_info_t* pat = &ses->pat;

    int i, j;

    //printf("Time id: %d\n", time_id);
    for (i=0; i<pat->prog_cnt; i++) {
        int tp_cnt = 0;
        in_prog_t* prog = in_prog_lookup_by_prog_num(&ses->prog_list,
                                  pat_get_prog_num(pat->prog_info[i]));
        if (prog) {
            pmt_info_t* pmt = &prog->pmt;
            pmt_header_t* pmt_hdr = (pmt_header_t*)pmt->sect->sect_hdr;
            for (j=0; j<pmt->es_cnt; j++) {
                pmt_es_info_t* es_info = pmt_info_get_es(pmt, j);
                int pid = pmt_get_es_pid(es_info);
                tp_cnt += pid_stat_tab[cur_stat_id][pid].tp_cnt;
            }
        }
        prog_tp_cnt[i][time_id] = tp_cnt;
        //printf("Prog %d: br %d\n", i, tp_cnt * 1504);
    }

    if (++time_id == MAX_TIME_ID)  time_id = 0;
    cur_stat_id = 1 - cur_stat_id;
    prev_stat_id = 1 - prev_stat_id;
    //printf("Stat update: cur %d\n", cur_stat_id);
    memset(pid_stat_tab[cur_stat_id], 0, sizeof(pid_stat_t) * NUM_PIDS);


#define STAT_DIR	"/home/pi/Hackathon/Site/cgi-bin/analyzer_stat/"
#define STAT_DIR2	"/var/www/Hackathon/"

    static char stat_buf[10000];
    char stat_file[100];
    FILE *fp;
    int rc;

    // Generate pid_stat file
    pid_stat_convert_json(pid_stat_tab[prev_stat_id], stat_buf);
    sprintf(stat_file, "%s.%d", STAT_DIR "pid_stat", prev_stat_id);
    fp = fopen(stat_file, "w+");
    if (fp == NULL) {
        printf("Failed to generate pid_stat file: %s\n",
               strerror(errno));
    } else {
        fprintf(fp, "%s", stat_buf);
        fclose(fp);
    }

    // Link pid_stat file
    unlink(STAT_DIR2 "pid_stat.txt");
    rc = symlink(stat_file, STAT_DIR2 "pid_stat.txt");
    if (rc == -1) {
        printf("File to link pid_stat file: %s\n", strerror(errno));
    }

   // Generate prog_stat file
    prog_stat_convert_json(stat_buf);
    sprintf(stat_file, "%s.%d", STAT_DIR "prog_stat", prev_stat_id);
    fp = fopen(stat_file, "w+");
    if (fp == NULL) {
        printf("Failed to generate prog_stat file: %s\n", strerror(errno));
    } else {
        fprintf(fp, "%s", stat_buf);
        fclose(fp);
    }

    // Link prog_stat file
    unlink(STAT_DIR2 "prog_stat.txt");
    rc = symlink(stat_file, STAT_DIR2 "prog_stat.txt");
    if (rc == -1) {
        printf("File to link prog_stat file: %s\n", strerror(errno));
    }
}


static
void analyzer_get_pid_stat (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    char* rsp = IPC_DATA(rsp_msg);
    memset(rsp_msg, 0, VIDEO_MSG_BUF_SIZE);
    pid_stat_convert_json(pid_stat_tab[prev_stat_id], rsp);

    ipc_message_header* rsp_hdr = IPC_HEADER(rsp_msg);
    rsp_hdr->size = strlen(rsp);
}


static
void analyzer_get_psi_table (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    char* rsp = IPC_DATA(rsp_msg);
    memset(rsp_msg, 0, VIDEO_MSG_BUF_SIZE);
    psi_table_convert_html(rsp);

    ipc_message_header* rsp_hdr = IPC_HEADER(rsp_msg);
    rsp_hdr->size = strlen(rsp);
}


static
void analyzer_get_prog_stat (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    char* rsp = IPC_DATA(rsp_msg);
    memset(rsp_msg, 0, VIDEO_MSG_BUF_SIZE);
    prog_stat_convert_json(rsp);

    ipc_message_header* rsp_hdr = IPC_HEADER(rsp_msg);
    rsp_hdr->size = strlen(rsp);
}


static
boolean platform_video_msg_handler (ipc_message *ipc_msg, ipc_message *rsp_msg,
                                    boolean *rsp_flag)
{
    *rsp_flag = FALSE;

    switch (IPC_HEADER(ipc_msg)->type) {

    case MSG_TYPE_VIDEO_STAT_UPDATE:	// currently not used
        analyzer_stat_update(ipc_msg, rsp_msg);
        break;

    case MSG_TYPE_VIDEO_GET_PID_STAT:
        analyzer_get_pid_stat(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_GET_PSI_TABLE:
        analyzer_get_psi_table(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_GET_PROG_STAT:
        analyzer_get_prog_stat(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_ADD_SESSION:
        sim_process_video_add_session(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_DELETE_SESSION:
        sim_process_video_delete_session(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_ADD_CAROUSEL:
        sim_process_video_add_crsl(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_DELETE_CAROUSEL:
        sim_process_video_delete_crsl(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_DELETE_OWNER:
        sim_process_video_delete_owner(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_LCRED_MODE_ROLE:
        sim_video_lcred_mode_role_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_LCRED_GO_HOT:
        printf("received go hot\n");
        sim_video_lcred_go_hot_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_LCRED_LC_JOINED_GROUP:
        sim_video_lcred_lc_joined_group_handler(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_LCRED_LC_LEFT_GROUP:
        sim_video_lcred_lc_left_group_handler(ipc_msg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


vdman_ipc_rc_t video_ca_ipc_send_to_scs (uint32_t msg_type, void *app_data, vipc_rc_t *rc)
{
    switch (msg_type) {

#if 0
    case MSG_TYPE_PID_EXISTS: {
        msg_pid_exists_t *rpy = app_data;
        printf("PID_EXISTS: pid %d\n", rpy->pid);
        break;
    }

    case MSG_TYPE_NO_PID: {
        msg_no_pid_t *rpy = app_data;
        printf("NO_PID: pid %d\n", rpy->pid);
        break;
    }

    case MSG_TYPE_SID_EXISTS: {
        msg_sid_exists_t *rpy = app_data;
        printf("SID_EXISTS: sid %d, %d pids\n", rpy->sid, rpy->num_pids);
        if (rpy->num_pids > MAX_PIDS_PER_PROGRAM) {
            printf("Error: Too many pids!\n");
            break;
        }
        int i;
        for (i=0; i<rpy->num_pids; i++) {
            printf("  pid %d: type %d\n", rpy->pid_list[i].pid,
                   rpy->pid_list[i].m_en_stream_type);
        }
        break;
    }

    case MSG_TYPE_NO_SID: {
        msg_no_sid_t *rpy = app_data;
        printf("NO_SID: pid %d\n", rpy->sid);
        break;
    }

    case MSG_TYPE_PID_PROVISIONED: {
        msg_pid_provisioned_t *rpy = app_data;
        printf("PID_PROVISIONED: qam %d, pid %d\n", rpy->qam_id, rpy->pid);
        break;
    }

    case MSG_TYPE_NEW_PID: {
        msg_new_pid_t* msg = app_data;
        printf("NEW_PID: qam %d, pid %d\n", msg->qam_id, msg->pid);
        break;
    }

    case MSG_TYPE_PID_GONE: {
        msg_pid_gone_t* msg = app_data;
        printf("PID_GONE: qam %d, pid %d\n", msg->qam_id, msg->pid);
        break;
    }

    case MSG_TYPE_UPDATE_SERVICE: {
        msg_update_service_t* msg = app_data;
        printf("UPDATE_SERVICE: qam %d, sid %d\n", msg->qam_id, msg->sid);
        break;
    }

    case MSG_TYPE_NEW_TS: {
        msg_new_onid_tsid_t* msg = app_data;
        printf("NEW_TS: qam %d, onid %d, tsid %d\n",
               msg->qam_id, msg->onid, msg->tsid);
        break;
    }

    case MSG_TYPE_REMOVE_TS: {
        msg_remove_onid_tsid_t* msg = app_data;
        printf("REMOVE_TS: qam %d, onid %d, tsid %d\n",
               msg->qam_id, msg->onid, msg->tsid);
        break;
    }

    case MSG_TYPE_ONID_TSID_CHANGE: {
        msg_change_on_onid_tsid_t* msg = app_data;
        printf("ONID_TSID_CHANGE: qam %d, onid %d -> %d, tsid %d -> %d\n",
               msg->qam_id, msg->old_onid, msg->new_onid, msg->old_tsid,
               msg->new_tsid);
        break;
    }
#endif

    default:
        printf("Unknown CA message type %d\n", msg_type);
    }

    return EOK;
}


// Message handler argument
//
typedef struct {
    ipc_message *ipc_msg;       // command or query message
    ipc_message *rsp_msg;       // response for query message
    boolean rsp_flag;           // set by handler:
                                //   TRUE if response message is provided
                                //   FALSE otherwise
} msg_handler_arg_t;


// Message handler
//   Returns TRUE if response message is provided.  Otherwise FALSE.
//
void sim_video_msg_handler (void* arg)
{
    msg_handler_arg_t* hdl_arg = (msg_handler_arg_t*)arg;

    // Check for platform message handler first
    if (!platform_video_msg_handler(hdl_arg->ipc_msg, hdl_arg->rsp_msg,
                                    &hdl_arg->rsp_flag)
        && !video_msg_handler(hdl_arg->ipc_msg, hdl_arg->rsp_msg,
                              &hdl_arg->rsp_flag)
        && !video_ca_msg_handler(hdl_arg->ipc_msg, hdl_arg->rsp_msg,
                                 &hdl_arg->rsp_flag)
       ) {

        VIDEO_DEBUG("\nUnknown message type %d ??",
                    IPC_HEADER(hdl_arg->ipc_msg)->type);
        errmsg(ERRMSG_ERR, LC_ERRMSG_VIDEO_UNKNOWN_MSG,
               "NG_VIDEO", IPC_HEADER(hdl_arg->ipc_msg)->type);
    }
    VIDEO_DEBUG("\n");          // flush log buffer
}


void sim_send_video_response (int fd, uint8 *msg_buf) 
{
    ipc_message_header *msg_hdr = IPC_HEADER(msg_buf);
    uint32 len = sizeof(ipc_message_header) + msg_hdr->size;
    uint32 n = write(fd, msg_buf, len);
    if (n != len) {
        printf("%s Error: write size mismatch\n", __FUNCTION__);
    }
}


void* msg_server_loop (void *arg)
{
printf("Message thread %lu started\n", syscall(__NR_gettid));
    signal(SIGPIPE, SIG_IGN);           // ignore SIGPIPE signal
    server_config_t* cfg = (server_config_t*)arg;
    
    msg_handler_arg_t hdl_arg = {NULL, (ipc_message*)video_rsp_buf};
    uint32 nbytes = 0;                     // number of date bytes in buffer
    uint32 offset = 0;

    while (1) {
        int n;
        if (nbytes >= sizeof(ipc_message_header)) {
            hdl_arg.ipc_msg = (ipc_message*)(video_msg_buf + offset);
            uint32 msg_size = sizeof(ipc_message_header)
                                      + IPC_HEADER(hdl_arg.ipc_msg)->size;

            if (nbytes >= msg_size) {
ipc_message_header *ipc_hdr = IPC_HEADER(video_msg_buf);
printf("Thread %lu: msg %d, len %d\n", syscall(__NR_gettid), ipc_hdr->type, ipc_hdr->size);
                cfg->handler(&hdl_arg);
                if (hdl_arg.rsp_flag) {
                    sim_send_video_response(cfg->sock_fd, video_rsp_buf);
                }

                nbytes -= msg_size;
                offset += msg_size;
                continue;
            }
        }

        // Refill buffer
        if (nbytes > 0) {
            memcpy(video_msg_buf, video_msg_buf + offset, nbytes);
        }
        n = read(cfg->sock_fd, video_msg_buf + nbytes,
                 sizeof(video_msg_buf) - nbytes);
        if (n == -1) {
            printf("PID %d: Read error: %d\n", getpid(), errno);
            break;
        }
        if (n == 0) {
            break;
        }

        nbytes += n;
        offset = 0;
    }

    close(cfg->sock_fd);

printf("Thread %lu terminated\n", syscall(__NR_gettid));
    pthread_exit(NULL);
    return NULL;
}

void video_print_info_log (const char *format, ...) 
{
}
