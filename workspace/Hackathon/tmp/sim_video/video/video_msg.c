/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "ipc.h"
#include "bit_mask.h"
#include "video.h"
#include "video_util.h"
#include "video_query.h"
#include "video_config_db.h"


#define STATUS_NOT_SET              0xFFFF
#define QAM_CONFIG_ALL_FIELDS       0
#define QAM_CONFIG_SINGLE_FIELD     1
#define ITEM_BUF_SIZE 256


// Message counts
uint32 msg_cnt_config_qam = 0;
uint32 msg_cnt_add_session = 0;
uint32 msg_cnt_delete_session = 0;
uint32 msg_cnt_delete_session_list = 0;
uint32 msg_cnt_delete_owner = 0;
uint32 msg_cnt_change_source = 0;
uint32 msg_cnt_update_pid_filter = 0;
uint32 msg_cnt_update_prog_filter = 0;
uint32 msg_cnt_update_pid_remap = 0;
uint32 msg_cnt_update_prog_remap = 0;
uint32 msg_cnt_add_crsl = 0;
uint32 msg_cnt_delete_crsl = 0;

static uint8_t item_val_buf[ITEM_BUF_SIZE];


video_context_t* get_config_context (int context_id, const char *label, int *rc)
{
    if (!lcred_is_valid_primary(context_id)) {
        em_vidman_VIDEO_BAD_PRIMARY_ID(label, context_id);
        *rc = EINVAL;
        return NULL;
    }

    // Get the video context
    if (!video_context_inited(context_id)) {
        *rc = video_context_init(context_id);
        if (*rc != EOK) {
            em_vidman_VIDEO_CONTEXT_INIT_FAILED(label, context_id);
            *rc = ENOMEM;
            return NULL;
        }
    }
    return get_video_context(context_id);
}


in_config_t* in_session_alloc (video_context_t *ctx, uint16 *ses_id,
                               const char* label, int *rc)
{
    in_config_t *cfg;

    if (*ses_id == INVALID_SES_ID) {
        que_elem_t* elem = pool_alloc(&ctx->in_ses_id_pool);
        if (!elem) {
            em_vidman_VIDEO_IN_SESSION_ALLOC_FAILED(label, ctx->id);
            *rc = ENOMEM;
            return NULL;
        }
        *ses_id = pool_get_elem_id(&ctx->in_ses_id_pool, elem,
                                   sizeof(que_elem_t));
        cfg = get_in_session_config(ctx, *ses_id);
        if (!cfg) {
            *rc = EINVAL;
            return NULL;
        }

    } else {
        if (*ses_id >= MAX_SESSIONS) {
            em_vidman_VIDEO_BAD_IN_SESSION_ID(label, *ses_id);
            *rc = EINVAL;
            return NULL;
        }

        cfg = get_in_session_config(ctx, *ses_id);
        if (!cfg) {
            *rc = EINVAL;
            return NULL;
        }
        if (cfg->used_flag) {
#if 0
            // Note: we accept this condition as a handled error case
            // Error handling: delete the old in session before moving forward
            VIDEO_LOG("ERR_HANDLE: %s: In %d reused", label, *ses_id);
            ng_in_session_dealloc(ctx, *ses_id);
#else
            // TODO: Temporarily turn off error handling!!
            *rc = EEXIST;
            return NULL;
#endif
        }

        pool_get(&ctx->in_ses_id_pool, get_in_session_id(ctx, *ses_id));
    }

    // Initialize in config
    memset(cfg, 0, sizeof(*cfg));
    cfg->used_flag = 1;

    ctx->in_session_cnt++;

    return cfg;
}


static que_elem_t *
in_ses_id_pool_get (video_context_t *ctx_p, uint32 in_id)
{
    pool_t      *pool_p;
    que_elem_t  *elem_array;
    que_elem_t  *elem_p;

    pool_p = &ctx_p->in_ses_id_pool;

    elem_array = (que_elem_t *) pool_p->buf;
    elem_p = &elem_array[in_id];

    pool_get(pool_p, elem_p);

    return elem_p;
}


in_config_t *in_session_assign (video_context_t *ctx_p, uint16 in_id)
{
    in_config_t *cfg_p;

    assert(in_id != INVALID_SES_ID);
    cfg_p = get_in_session_config(ctx_p, in_id);

    if (cfg_p->used_flag) {
        return cfg_p;
    }

    in_ses_id_pool_get(ctx_p, in_id);
    // Initialize in config
    memset(cfg_p, 0, sizeof(in_config_t));

    ctx_p->in_session_cnt++;

    return cfg_p;
}


int out_session_dealloc (video_context_t *ctx, uint16 ses_id)
{
    boolean ctx_active = (video_ctx == ctx);
    out_config_t* cfg = get_out_session_config(ctx, ses_id);

    if (ctx_active)  qam_channel_lock(cfg->qam_id);
    int rc = unprovision_qam_flow(ctx, cfg);
    if (ctx_active)  qam_channel_unlock(cfg->qam_id);
    assert(rc == EOK);

    if (ctx_active) {
        out_session_lock(ses_id);
    }

    out_session_delete(ctx, ses_id);

    if (ctx_active) {
        out_session_unlock(ses_id);

        // Accumulate the statistics
        video_out_stat_update(&video_out_stat_history,
                              &get_out_session(ses_id)->stat);
    }

    // Update reference count for corresponding in session config
    get_in_session_config(ctx, cfg->in_ses_id)->ref_cnt--;

    cfg->used_flag = 0;
    ctx->out_session_cnt--;
    cfg->in_ses_id = INVALID_SES_ID;

    video_db_out_cnfg_delete(ctx->id, ses_id);

    return rc;
}


// Process ADD_SESSION message
int process_video_add_session_msg (video_add_session_msg_t *msg,
                                   in_config_t *in_cfg,
                                   out_config_t *out_cfg)
{
    // following are input parameters

    in_cfg->owner_id = msg->owner_id;

    // input type
    VIDEO_MSG_DEBUG("  INPUT_TYPE %d", msg->input_type);
    if ((msg->input_type < INPUT_TYPE_UNICAST) || 
        (msg->input_type > INPUT_TYPE_MULTICAST_ASM)) {
        em_vidman_VIDEO_BAD_INPUT_TYPE("ADD_IN", msg->input_type);
        return EINVAL;
    }
    in_cfg->input_type = msg->input_type;

    if (msg->ip_addr_len == IPV4_ADDR_LEN) {
        // source ipv4 address
        VIDEO_MSG_DEBUG("  SRC_IPV4_ADDR %08x", msg->src_ip_addr[0]);
        in_cfg->src_ip[0] = msg->src_ip_addr[0];

        // destination ipv4 address
        VIDEO_MSG_DEBUG("  DST_IPV4_ADDR %08x", msg->dst_ip_addr[0]);
        in_cfg->dst_ip[0] = msg->dst_ip_addr[0];
        in_cfg->ip_ver = IPV4_VERSION;      // TODO: Need more checking

    } else if (msg->ip_addr_len == IPV6_ADDR_LEN) {
        // source ipv6 address
        VIDEO_MSG_DEBUG("  SRC_IPV6_ADDR %08x-%08x-%08x-%08x",
                        msg->src_ip_addr[0], msg->src_ip_addr[1],
                        msg->src_ip_addr[2], msg->src_ip_addr[3]);
        in_cfg->src_ip[0] = msg->src_ip_addr[0];
        in_cfg->src_ip[1] = msg->src_ip_addr[1];
        in_cfg->src_ip[2] = msg->src_ip_addr[2];
        in_cfg->src_ip[3] = msg->src_ip_addr[3];

        // destination ipv6 address
        VIDEO_MSG_DEBUG("  DST_IPV6_ADDR %08x-%08x-%08x-%08x",
                        msg->dst_ip_addr[0], msg->dst_ip_addr[1],
                        msg->dst_ip_addr[2], msg->dst_ip_addr[3]);
        in_cfg->dst_ip[0] = msg->dst_ip_addr[0];
        in_cfg->dst_ip[1] = msg->dst_ip_addr[1];
        in_cfg->dst_ip[2] = msg->dst_ip_addr[2];
        in_cfg->dst_ip[3] = msg->dst_ip_addr[3];
        in_cfg->ip_ver = IPV6_VERSION;      // TODO: Need more checking

    } else {
        em_vidman_VIDEO_BAD_IP_ADDR_LEN("ADD_SES", msg->ip_addr_len);
        return EINVAL;
    }

    // source udp port
    VIDEO_MSG_DEBUG("  SRC_UDP_PORT %d", msg->src_udp_port);
    in_cfg->src_udp = msg->src_udp_port;

    // destination udp port
    VIDEO_MSG_DEBUG("  DST_UDP_PORT %d", msg->dst_udp_port);
    if (msg->dst_udp_port == 0 && msg->input_type == INPUT_TYPE_UNICAST) {
        VIDEO_MSG_DEBUG("Error: dst_udp_port out of range %d",
                        msg->dst_udp_port);
        em_vidman_VIDEO_BAD_DST_UDP_PORT("ADD_SES", msg->dst_udp_port);
        return EINVAL;
    }
    in_cfg->dst_udp = msg->dst_udp_port;

    // stream type
    VIDEO_MSG_DEBUG("  STREAM_TYPE %d", msg->stream_type);
    in_cfg->mpts_flag = out_cfg->mpts_flag
                      = (msg->stream_type == STREAM_TYPE_MPTS);

    // pid remap
    VIDEO_MSG_DEBUG("  PID_REMAP %d", msg->pid_remap);
    in_cfg->pid_remap_flag = out_cfg->pid_remap_flag = (msg->pid_remap != 0);

    // parse psi
    VIDEO_MSG_DEBUG("  PARSE_PSI %d", msg->parse_psi);
    in_cfg->parse_psi_flag = out_cfg->parse_psi_flag = (msg->parse_psi != 0);

    // dejitter
    VIDEO_MSG_DEBUG("  DEJITTER %d", msg->dejitter);
    in_cfg->dejitter_flag = out_cfg->dejitter_flag = (msg->dejitter != 0);

    // jitter size
    VIDEO_MSG_DEBUG("  JITTER_SIZE %d", msg->jitter_size);
    in_cfg->jitter_size = msg->jitter_size;    // in ms
    if (in_cfg->jitter_size > MAX_JITTER) {
        VIDEO_MSG_DEBUG("Error:  out of range jitter %d", msg->jitter_size);
        em_vidman_VIDEO_BAD_JITTER("ADD_SES", in_cfg->jitter_size);
        return EINVAL;
    }

    // target delay
    VIDEO_MSG_DEBUG("  DELAY_TARGET %d", msg->delay_target);
    if (msg->delay_target > msg->jitter_size) {
        VIDEO_MSG_DEBUG("Error: Bad target delay value %d", msg->delay_target);
        em_vidman_VIDEO_BAD_DELAY("ADD_SES", msg->delay_target);
        return EINVAL;
    }
    in_cfg->delay_target = msg->delay_target;    // in ms

    // Clock recovery mode
    VIDEO_MSG_DEBUG("  CBR %d", msg->cbr);
    if (msg->cbr > VIDEO_CR_MODE_MAX) {
        VIDEO_MSG_DEBUG("Error: illegal CR mode %d", msg->cbr);
        em_vidman_VIDEO_BAD_CR_MODE("ADD_SES", msg->cbr);
        return EINVAL;
    }
    in_cfg->cbr_flag = (msg->cbr != VIDEO_CR_MODE_UNIFIED_VBR);
    in_cfg->ms_cr_flag = (msg->cbr == VIDEO_CR_MODE_MASTER_SLAVE);

    // bitrate allocation
    VIDEO_MSG_DEBUG("  BITRATE_ALLOC %d", msg->bitrate_alloc);
    if (msg->bitrate_alloc > MAX_BITRATE_ALLOC) {
         VIDEO_MSG_DEBUG("Error:  bad bitrate alloc %d", msg->bitrate_alloc);
         em_vidman_VIDEO_BAD_BITRATE_ALLOC("ADD_SES", msg->bitrate_alloc);
         return EINVAL;
    }
    in_cfg->bitrate_alloc = msg->bitrate_alloc;

    // idle threshold
    VIDEO_MSG_DEBUG("  IDLE_THRESHOLD %d", msg->idle_threshold);
    in_cfg->idle_thres = msg->idle_threshold;    // in ms

    // init threshold
    VIDEO_MSG_DEBUG("  INIT_THRESHOLD %d", msg->init_threshold);
    in_cfg->init_thres = msg->init_threshold;    // in ms

    // off timer
    VIDEO_MSG_DEBUG("  OFF_TIMER %d", msg->off_timer);
    in_cfg->off_timer = msg->off_timer;    // in sec

    // following are output parameters

    out_cfg->owner_id = msg->owner_id;

    // QAM id
    VIDEO_MSG_DEBUG("  QAM_ID %d", msg->qam_id);
    if (msg->qam_id >= NUM_QAMS) {
        em_vidman_VIDEO_BAD_QAM_ID("ADD_SES", msg->qam_id);
        return EINVAL;
    }
    out_cfg->qam_id = msg->qam_id;

    // program number (> 0 for remapped sessions, 0 for passthru/remux sesions)
    VIDEO_MSG_DEBUG("  PROGRAM_NUM %d", msg->prog_num);
    if (msg->prog_num >= NUM_PROGS) {
        em_vidman_VIDEO_BAD_PROG_NUM("ADD_SES", msg->prog_num);
        return EINVAL;
    }
    if (msg->prog_num) {
        out_cfg->out_prog_list[0] = msg->prog_num;
        out_cfg->prog_cnt = 1;
    }

    out_cfg->owner_id = msg->owner_id;
    in_cfg->owner_id = msg->owner_id;

    // pid range
    VIDEO_MSG_DEBUG("  PID %d - %d", msg->res_pid.first, msg->res_pid.last);
    if (msg->res_pid.first > msg->res_pid.last
            || msg->res_pid.last >= NULL_PID) {
        em_vidman_VIDEO_BAD_PID_RANGE("ADD_SES", 
                                      msg->res_pid.first, 
                                      msg->res_pid.last);
        return EINVAL;
    }
    out_cfg->pid_range[0] = msg->res_pid;

    // encryption mode
    VIDEO_MSG_DEBUG("  ENCRYPT_MODE %d", msg->encrypt_flag);
    out_cfg->encrypt_flag = msg->encrypt_flag;

    // Enale flags
    // always enable the input session
    in_cfg->enable_flag = 1;

    // output enable
    VIDEO_MSG_DEBUG("  ENABLE %d", msg->enable);
    out_cfg->enable_flag = (msg->enable != 0);

    return EOK;
}


// Process CHANGE_SOURCE message
int process_video_change_source_msg (video_change_source_msg_t *msg,
                                     in_config_t *old_cfg,
                                     uint32 *new_src_ip,
                                     uint32 *new_grp_ip,
                                     uint32 *init_thres)
{
    // input type
    VIDEO_MSG_DEBUG("  INPUT_TYPE %d", msg->input_type);
    if ((msg->input_type < INPUT_TYPE_UNICAST) ||
        (msg->input_type > INPUT_TYPE_MULTICAST_ASM)) {
        em_vidman_VIDEO_BAD_INPUT_TYPE("CHANGE_SOURCE", msg->input_type);
        return EINVAL;
    }
    old_cfg->input_type = msg->input_type;

    // input port
    VIDEO_MSG_DEBUG("  INPUT_PORT %d", msg->input_port);
    old_cfg->input_port = msg->input_port;
    if (old_cfg->input_port >= NUM_INPUT_PORTS) {
        em_vidman_VIDEO_BAD_INPUT_PORT("CHANGE_SOURCE", old_cfg->input_port);
        return EINVAL;
    }

    // source ipv4 address
    VIDEO_MSG_DEBUG("  SRC_IPV4_ADDR %08x", msg->src_ip_addr[0]);
    old_cfg->src_ip[0] = msg->src_ip_addr[0];

    // destination ipv4 address
    VIDEO_MSG_DEBUG("  DST_IPV4_ADDR %08x", msg->dst_ip_addr[0]);
    old_cfg->dst_ip[0] = msg->dst_ip_addr[0];

    // new source ipv4 address
    VIDEO_MSG_DEBUG("  SRC_IPV4_ADDR_NEW %08x", msg->src_ip_addr_new[0]);
    *new_src_ip = msg->src_ip_addr_new[0];

    // new destination ipv4 address
    VIDEO_MSG_DEBUG("  DST_IPV4_ADDR_NEW %08x", msg->dst_ip_addr_new[0]);
    *new_grp_ip = msg->dst_ip_addr_new[0];

    // init threshold
    VIDEO_MSG_DEBUG("  DST_IPV4_ADDR %08x", msg->init_threshold);
    *init_thres = msg->init_threshold;

    return EOK;
}


// TODO: clean up crsl on error paths
//
int process_video_add_crsl_msg (video_add_crsl_msg_t *msg,
                                video_context_t *ctx, uint16 crsl_id)
{
    int rc = EOK;

    VIDEO_MSG_DEBUG("  NUM_QAM %d", msg->num_qam);
    VIDEO_MSG_DEBUG("  NUM_TP %d", msg->num_tp);
    VIDEO_MSG_DEBUG("  INSERT_CNT %d", msg->insert_count);
    VIDEO_MSG_DEBUG("  INSERT_PERIOD %d", msg->insert_period);

    crsl_t* crsl = get_crsl(ctx, crsl_id);
    if (crsl->used_flag) {
        em_vidman_VIDEO_CAROUSEL_IN_USE("ADD_CRSL", crsl_id);
        return EBUSY;
    }

    crsl_init(ctx, crsl_id);

    crsl->owner_id = msg->owner_id;

    int i;
    for (i=0; i<msg->num_tp; i++) {
        que_elem_t* elem = pool_alloc(&ctx->crsl_tp_buf_pool);
        if (!elem) {
            em_vidman_VIDEO_PACKET_INSERT_ALLOC_FAILED("ADD_CRSL", crsl_id);

            // Free TP buffers already allocated
            pool_free_list(&ctx->crsl_tp_buf_pool, &crsl->tp_list);
            return ENOMEM;
        }

        memcpy(elem + 1, msg->tp_data + i * TP_SIZE, TP_SIZE);
        que_put(NULL, &crsl->tp_list, elem);
    }
    crsl->num_tp = msg->num_tp;

    uint32 now = sys_clk.base;

    for (i=0; i<msg->num_qam; i++) {
        int qam_id = msg->qam_id[i];
        VIDEO_MSG_DEBUG("  QAM %d", qam_id);
        if (qam_id >= NUM_QAMS) {
            em_vidman_VIDEO_BAD_QAM_ID("ADD_CRSL", qam_id);
            return EINVAL;
        }
        bit_mask_set(crsl->qam_mask, qam_id);
        crsl->num_qam++;

        // Allocate crsl insert
        crsl_insert_t* insert
                = (crsl_insert_t*)pool_alloc(&ctx->crsl_insert_pool);
        if (!insert) {
            em_vidman_VIDEO_CAROUSEL_ALLOC_FAILED("CRSL_ADD", crsl_id);
            rc = ENOMEM;
            goto done;
        }

        insert->ecm_flag = 0;
        insert->crsl = crsl;
        insert->period = msg->insert_period * SCALE_MS_BASE;
        insert->target_cnt = msg->insert_count;
        insert->next_time = now;

        // Put crsl to qam's queue
        que_elem_t   *insert_list = get_qam_crsl_insert_list(ctx, qam_id);
        qam_config_t *qam_cfg = get_qam_config(ctx, qam_id);

        qam_channel_lock(qam_id);
        que_put(NULL, insert_list, &insert->link);
        qam_cfg->crsl_cnt++;
        qam_channel_unlock(qam_id);

        video_db_qam_cnfg_checkin(ctx->id, qam_id, qam_cfg);
    }

    crsl->insert_count = msg->insert_count;
    crsl->insert_period = msg->insert_period;
    crsl->resource_id = msg->resource_id;
    crsl->used_flag = 1;        // turn on carousel

done:
    return rc;
}


// Process CONFIG_QAM message
int process_video_config_qam_msg (video_config_qam_msg_t* msg,
                                  qam_config_t* cfg)
{
    cfg->owner_id = msg->owner_id;

    if ((msg->field_mask >> qam_cfg_tsid) & 1) {
        VIDEO_MSG_DEBUG("  TSID %d\n", msg->tsid);
        cfg->tsid = msg->tsid;
    }

    if ((msg->field_mask >> qam_cfg_onid) & 1) {
        VIDEO_MSG_DEBUG("  ONID %d\n", msg->onid);
        cfg->onid = msg->onid;
    }

    if ((msg->field_mask >> qam_cfg_psi_period) & 1) {
        VIDEO_MSG_DEBUG("  PSI_PERIOD %d\n", msg->psi_period);
        cfg->psi_period = msg->psi_period;
    }

    if ((msg->field_mask >> qam_cfg_enable) & 1) {
        VIDEO_MSG_DEBUG("  ENABLE %d\n", msg->enable);
        cfg->enable_flag = msg->enable;
    }

    if ((msg->field_mask >> qam_cfg_encrypt) & 1) {
        VIDEO_MSG_DEBUG("  ENCRYPT_MODE %d\n", msg->encrypt_flag);
        cfg->encrypt_flag = msg->encrypt_flag;
    }

    return EOK;
}


#if 0
// Handler for video change in session message
//   TODO: Need to support init_thres change also!
//
static
void process_video_change_source (ipc_message *ipc_msg)
{
    int rc;
    uint16 in_rid;
    video_context_t* ctx;
    in_config_t old_cfg;
    in_config_t *cfg;
    uint32 new_src_ip = 0;
    uint32 new_dst_ip = 0;
    uint32 init_thres = 0;
    boolean ctx_active;
    video_change_source_msg_t *cmd = IPC_DATA(ipc_msg);

    msg_cnt_change_source++;
    VIDEO_MSG_DEBUG("CHANGE_SOURCE: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    ctx = get_config_context(cmd->primary_id, "CHANGE_SOURCE", &rc);
    if (!ctx) {
        goto done;
    }

    // Process message
    rc = process_video_change_source_msg(cmd, &old_cfg, &new_src_ip,
                                         &new_dst_ip, &init_thres);
    if (rc != EOK) {
        em_vidman_VIDEO_INCOMPLETE_MSG("CHANGE_SOURCE");
        goto done;
    }
    ctx_active = (video_ctx == ctx);

    if (old_cfg.input_type != INPUT_TYPE_MULTICAST_SSM)  {
        rc = EINVAL;
        goto done;
    }

    if (new_src_ip == old_cfg.src_ip[0] && new_dst_ip == old_cfg.dst_ip[0]) {
        // No change
        goto done;
    }

    // Unprovision the old session
    rc = unprovision_in_session(ctx, &in_rid, &old_cfg);
    if (rc != EOK) {
        goto done;
    }

    cfg = get_in_session_config(ctx, in_rid);

    if (ctx_active)  in_session_lock(in_rid);

    // Change session configuration
    cfg->src_ip[0] = new_src_ip;
    cfg->dst_ip[0] = new_dst_ip;
    if (init_thres != 0) {
        cfg->init_thres = init_thres;
    }

    rc = provision_in_session(ctx, &in_rid, cfg);
    if (rc == EOK) {
        rc = in_session_update(ctx, in_rid);
    }
    if (ctx_active)  in_session_unlock(in_rid);

done:
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);
    return;
}
#endif


// Handler for qam config message
//
void process_video_config_qam (ipc_message *ipc_msg)
{
    int rc;
    uint16 old_onid;
    uint16 old_tsid;
    boolean old_encrypt_mode;
    video_context_t *ctx = NULL;
    qam_config_t *cfg = NULL;

    video_config_qam_msg_t *cmd = IPC_DATA(ipc_msg);
    uint16 rid = cmd->resource_id;
    msg_cnt_config_qam++;

    VIDEO_MSG_DEBUG("\nQAM_CONFIG: primary %d, transaction %d, resource %d",
                    cmd->primary_id, cmd->transaction_id, cmd->resource_id);

    ctx = get_config_context(cmd->primary_id, "QAM_CONFIG", &rc);
    if (!ctx) {
        goto done;
    }

    // Sanity checks
    if (rid >= NUM_QAMS) {
        em_vidman_VIDEO_BAD_QAM_ID("QAM_CONFIG", rid);
        rc = EINVAL;
        goto done;
    }

    cfg = get_qam_config(ctx, rid);

    old_encrypt_mode = (cfg->enable_flag && cfg->encrypt_flag);
    old_onid = cfg->onid;
    old_tsid = cfg->tsid;

    // Process message
    rc = process_video_config_qam_msg(cmd, cfg);
    if (rc != EOK) {
        goto done;
    }

    video_db_qam_cnfg_checkin(cmd->primary_id, rid, cfg);

    if (video_ctx) {
        if (cfg->tsid != old_tsid || cfg->onid != old_onid) {
            get_qam_info(rid)->pat_rebuild = 1;
        }

        video_ca_report_qam_change(rid, old_encrypt_mode, old_onid, old_tsid);
    }

done:
    VIDEO_MSG_DEBUG("Status %d, transaction %d", rc, cmd->transaction_id);
}


void process_video_update_pid_filter (ipc_message *ipc_msg)
{
    int i;
    int rc = EOK;
    out_session_t* ses = NULL;
    video_update_output_msg_t *cmd = IPC_DATA(ipc_msg);
    msg_cnt_update_pid_filter++;

    if (is_trace_enabled(1)) {
        VIDEO_DEBUG("\n\nUPDATE_PID_FILTER: primary %d, transaction %d, "\
                    "resource %d, oper %d", cmd->primary_id,
                    cmd->transaction_id, cmd->resource_id, cmd->oper);
        for (i=0; i<cmd->item_count; i++) {
            VIDEO_DEBUG("\n  Pid %d", cmd->list[i]);
        }
        VIDEO_DEBUG("\n");
    }

    video_context_t* ctx = get_config_context(cmd->primary_id,
                                              "UPDATE_PID_FILTER", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint16 out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        em_vidman_VIDEO_SESSION_NOT_USED("UPDATE_PID_FILTER", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    out_config_t* cfg = get_out_session_config(ctx, out_id);
    if (!cfg->used_flag) {
        em_vidman_VIDEO_OUT_SESSION_NOT_USED("UPDATE_PID_FILTER", out_id);
        rc = ENOENT;
        goto done;
    }

    // Wait for previous pending transaction
    out_cfg_wait_for_pending(cfg);

    // Check session type
    in_config_t* in_cfg = get_in_session_config(ctx, cfg->in_ses_id);
    assert(in_cfg->used_flag);
    if (in_cfg->pid_remap_flag) {
        em_vidman_VIDEO_FEATURE_NOT_AVAIL("PID_FILTER", out_id);
        rc = EPERM;
        goto done;
    }

    // Update out session
    if (video_ctx) {
        out_session_lock(out_id);
        ses = get_out_session(out_id);
    }

    switch (cmd->oper) {

    case VIDEO_LIST_OPER_ADD:
        for (i=0; i<cmd->item_count; i++) {
            uint16 pid = cmd->list[i];
            if (pid <= PAT_PID || pid >= NULL_PID) {
                em_vidman_VIDEO_BAD_FILTERED_PID("Add", pid, out_id);
                continue;
            }
            if (video_ctx) {
                out_pid_info_set(&ses->pid_tab[pid], 1, NO_CHANGE, 0);
            }
            video_list_add(pid, cfg->in_pid_list, &cfg->pid_cnt,
                           VIDEO_PID_LIST_SIZE);
        }
        break;

    case VIDEO_LIST_OPER_DELETE:
        for (i=0; i<cmd->item_count; i++) {
            uint16 pid = cmd->list[i];
            if (pid <= PAT_PID || pid >= NULL_PID) {
                em_vidman_VIDEO_BAD_FILTERED_PID("Delete", pid, out_id);
                continue;
            }
            if (video_ctx) {
                out_pid_info_set(&ses->pid_tab[pid], 0, NO_CHANGE, 0);
            }
            video_list_delete(pid, cfg->in_pid_list, &cfg->pid_cnt);
        }
        break;

    case VIDEO_LIST_OPER_CLEAR:
        for (i=0; i<cfg->pid_cnt; i++) {
            if (video_ctx) {
                uint16 pid = cfg->in_pid_list[i];
                out_pid_info_set(&ses->pid_tab[pid], 0, NO_CHANGE, 0);
            }
        }

        cfg->pid_cnt = 0;
        break;

    default:
        VIDEO_DEBUG("PID FILTER: Unknown list operation %d\n", cmd->oper);
    }

    video_db_out_cnfg_checkin(ctx->id, out_id, cfg);

    if (video_ctx) {
        out_session_unlock(out_id);
    }

done:
    VIDEO_MSG_DEBUG("\nStatus %d, transaction %d", rc, cmd->transaction_id);

    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, cmd->oper, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


void process_video_update_prog_filter (ipc_message *ipc_msg)
{
    int i;
    int rc = EOK;
    out_session_t* ses = NULL;
    out_prog_t* prog = NULL;

    video_update_output_msg_t *cmd = IPC_DATA(ipc_msg);
    msg_cnt_update_prog_filter++;

    if (is_trace_enabled(1)) {
        VIDEO_DEBUG("\n\nUPDATE_PROG_FILTER: primary %d, transaction %d, "\
                    "resource %d, oper %d", cmd->primary_id,
                    cmd->transaction_id, cmd->resource_id, cmd->oper);
        for (i=0; i<cmd->item_count; i++) {
            VIDEO_DEBUG("\n  Pid %d", cmd->list[i]);
        }
        VIDEO_DEBUG("\n");
    }

    video_context_t* ctx = get_config_context(cmd->primary_id,
                                              "UPDATE_PROG_FILTER", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint16 out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        em_vidman_VIDEO_SESSION_NOT_USED("UPDATE_PROG_FILTER", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    out_config_t* cfg = get_out_session_config(ctx, out_id);
    if (!cfg->used_flag) {
        em_vidman_VIDEO_OUT_SESSION_NOT_USED("UPDATE_PROG_FILTER", out_id);
        rc = ENOENT;
        goto done;
    }

    // Wait for previous pending transaction
    out_cfg_wait_for_pending(cfg);

    // Check session type
    in_config_t* in_cfg = get_in_session_config(ctx, cfg->in_ses_id);
    assert(in_cfg->used_flag);
    if (in_cfg->pid_remap_flag) {
        em_vidman_VIDEO_FEATURE_NOT_AVAIL("PROG_FILTER", out_id);
        rc = EPERM;
        goto done;
    }

    // Update out session
    if (video_ctx) {
        out_session_lock(out_id);
        ses = get_out_session(out_id);
    }

    switch (cmd->oper) {

    case VIDEO_LIST_OPER_ADD:
        for (i=0; i<cmd->item_count; i++) {
            uint16 prog_num = cmd->list[i];
            if (prog_num == 0) {
                em_vidman_VIDEO_BAD_FILTERED_PROG("Add", prog_num, out_id);
                continue;
            }
            rc = video_list_add(prog_num, cfg->in_prog_list, &cfg->prog_cnt,
                                VIDEO_PROG_LIST_SIZE);
            if (rc != EOK || !video_ctx)  continue;

            prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
            if (prog && !prog->filtered) {
                prog->filtered = 1;
                out_prog_set_exclude(prog, ses, cfg->prog_cnt - 1);
                out_prog_set_register(prog, 1, 1);
            }
        }
        break;

    case VIDEO_LIST_OPER_DELETE:
        for (i=0; i<cmd->item_count; i++) {
            uint16 prog_num = cmd->list[i];
            if (prog_num == 0) {
                em_vidman_VIDEO_BAD_FILTERED_PROG("Delete", prog_num, out_id);
                continue;
            }
            rc = video_list_delete(prog_num, cfg->in_prog_list, &cfg->prog_cnt);
            if (rc != EOK || !video_ctx)  continue;

            prog = out_prog_lookup_by_prog_num(&ses->prog_list, prog_num);
            if (prog && prog->filtered) {
                out_prog_set_include(prog, ses, INVALID_CFG_IDX);
                out_prog_set_register(prog, 1, 1);
            }
        }
        break;

    case VIDEO_LIST_OPER_CLEAR:
        cfg->prog_cnt = 0;
        if (!video_ctx)  break;

        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
            if (prog->filtered) {
                out_prog_set_include(prog, ses, INVALID_CFG_IDX);
                out_prog_set_register(prog, 1, 1);
            }
        }
        break;

    default:
        VIDEO_DEBUG("PROG FILTER: Unknown list operation %d\n", cmd->oper);
    }

    video_db_out_cnfg_checkin(ctx->id, out_id, cfg);

    if (video_ctx) {
        out_session_unlock(out_id);
    }

done:
    VIDEO_MSG_DEBUG("\nStatus %d, transaction %d", rc, cmd->transaction_id);
    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, cmd->oper, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


void process_video_update_pid_remap (ipc_message *ipc_msg)
{
    int i;
    int rc = EOK;
    out_session_t* ses = NULL;
    video_update_remap_pid_msg_t* cmd = IPC_DATA(ipc_msg);
    msg_cnt_update_pid_remap++;

    if (is_trace_enabled(1)) {
        VIDEO_DEBUG("\n\nUPDATE_PID_REMAP: primary %d, transaction %d, "\
                    "resource %d, oper %d", cmd->primary_id,
                    cmd->transaction_id, cmd->resource_id, cmd->oper);
        for (i=0; i<cmd->item_count; i++) {
            VIDEO_DEBUG("\n  Pid %d->%d",
                        cmd->in_pid_list[i], cmd->out_pid_list[i]);
        }
        VIDEO_DEBUG("\n");
    }

    video_context_t* ctx = get_config_context(cmd->primary_id,
                                              "UPDATE_PID_REMAP", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint16 out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        em_vidman_VIDEO_SESSION_NOT_USED("UPDATE_PID_REMAP", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    out_config_t* cfg = get_out_session_config(ctx, out_id);
    if (!cfg->used_flag) {
        em_vidman_VIDEO_OUT_SESSION_NOT_USED("UPDATE_PID_REMAP", out_id);
        rc = ENOENT;
        goto done;
    }

    // Wait for previous pending transaction
    out_cfg_wait_for_pending(cfg);

    // Check session type
    in_config_t* in_cfg = get_in_session_config(ctx, cfg->in_ses_id);
    assert(in_cfg->used_flag);
    if (!(in_cfg->pid_remap_flag & in_cfg->mpts_flag)) {
        em_vidman_VIDEO_FEATURE_NOT_AVAIL("PID_REMAP", out_id);
        rc = EPERM;
        goto done;
    }

    // Update out session
    if (video_ctx) {
        out_session_lock(out_id);
        ses = get_out_session(out_id);
    }

    switch (cmd->oper) {
    
    case VIDEO_LIST_OPER_ADD:
        for (i=0; i<cmd->item_count; i++) {
            // Sanity checks
            if (cfg->pid_cnt >= VIDEO_PID_LIST_SIZE) {
                VIDEO_DEBUG("Too many remap pids\n");
                rc = ENOMEM;
                break;
            }

            uint16 in_pid = cmd->in_pid_list[i];
            uint16 out_pid = cmd->out_pid_list[i];

            int rc1 = video_list_search(in_pid, cfg->in_pid_list, cfg->pid_cnt);
            int rc2 = video_list_search(out_pid, cfg->out_pid_list,
                                        cfg->pid_cnt);
            if (rc1 != -ENOENT || rc2 != -ENOENT)  {
                VIDEO_DEBUG("Bad remap pid: %d -> %d\n", in_pid, out_pid);
                continue;
            }

            if (out_pid <= PAT_PID || out_pid >= NULL_PID) {
                em_vidman_VIDEO_BAD_REMAPPED_PID("Add", out_pid, out_id);
                continue;
            }

            video_list_set(in_pid, cfg->in_pid_list, cfg->pid_cnt);
            video_list_set(out_pid, cfg->out_pid_list, cfg->pid_cnt++);

            if (video_ctx) {
                out_pid_info_set(&ses->pid_tab[in_pid], 1, NO_CHANGE, out_pid);
            }
        }
        break;

    case VIDEO_LIST_OPER_DELETE:
        for (i=0; i<cmd->item_count; i++) {
            uint16 in_pid = cmd->in_pid_list[i];

            int idx = video_list_search(in_pid, cfg->in_pid_list, cfg->pid_cnt);
            if (idx == -ENOENT) {
                em_vidman_VIDEO_BAD_REMAPPED_PID("Delete", in_pid, 0);
                continue;
            }

            // Update config
            cfg->pid_cnt--;
            video_list_set(cfg->in_pid_list[cfg->pid_cnt],
                           cfg->in_pid_list, idx);
            video_list_set(cfg->out_pid_list[cfg->pid_cnt],
                           cfg->out_pid_list, idx);

            if (video_ctx) {
                out_pid_info_set(&ses->pid_tab[in_pid], 0, NO_CHANGE, 0);
            }
        }
        break;

    case VIDEO_LIST_OPER_CLEAR:
        for (i=0; i<cfg->pid_cnt; i++) {
            if (video_ctx) {
                uint16 in_pid = cfg->in_pid_list[i];
                out_pid_info_set(&ses->pid_tab[in_pid], 0, NO_CHANGE, 0);
            }
        }
        cfg->pid_cnt = 0;
        break;

    default:
        VIDEO_DEBUG("PID REMAP: Unknown list operation %d\n", cmd->oper);
    }

    if (video_ctx) {
        out_session_unlock(out_id);
    }

    video_db_out_cnfg_checkin(ctx->id, out_id, cfg);

done:
    VIDEO_MSG_DEBUG("\nStatus %d, transaction %d", rc, cmd->transaction_id);
    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, cmd->oper, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


void process_video_update_prog_remap (ipc_message *ipc_msg)
{
    int i;
    int rc = EOK;
    out_session_t* ses = NULL;
    out_prog_t* prog = NULL;
    video_update_remap_prog_msg_t* cmd = IPC_DATA(ipc_msg);
    msg_cnt_update_prog_remap++;

    if (is_trace_enabled(1)) {
        VIDEO_DEBUG("\n\nUPDATE_PROG_REMAP: primary %d, transaction %d, "\
                    "resource %d, oper %d", cmd->primary_id,
                    cmd->transaction_id, cmd->resource_id, cmd->oper);
        for (i=0; i<cmd->item_count; i++) {
            VIDEO_DEBUG("\n  Prog %d->%d, pids %d-%d",
                        cmd->in_prog_list[i], cmd->out_prog_list[i],
                        cmd->pid_range[i].first, cmd->pid_range[i].last);
        }
        VIDEO_DEBUG("\n");
    }

    video_context_t* ctx = get_config_context(cmd->primary_id,
                                              "UPDATE_PROG_REMAP", &rc);
    if (!ctx) {
        rc = EINVAL;
        goto done;
    }

    uint16 out_id = session_id_to_out_id(ctx, cmd->resource_id);
    if (out_id == INVALID_SES_ID) {
        em_vidman_VIDEO_SESSION_NOT_USED("UPDATE_PROG_REMAP", cmd->resource_id);
        rc = ENOENT;
        goto done;
    }

    out_config_t* cfg = get_out_session_config(ctx, out_id);
    if (!cfg->used_flag) {
        em_vidman_VIDEO_OUT_SESSION_NOT_USED("UPDATE_PROG_REMAP", out_id);
        rc = ENOENT;
        goto done;
    }

    // Wait for previous pending transaction
    out_cfg_wait_for_pending(cfg);

    // Check session type
    in_config_t* in_cfg = get_in_session_config(ctx, cfg->in_ses_id);
    assert(in_cfg->used_flag);
    if (!(in_cfg->pid_remap_flag & in_cfg->mpts_flag)) {
        em_vidman_VIDEO_FEATURE_NOT_AVAIL("PROG_REMAP", out_id);
        rc = EPERM;
        goto done;
    }

    // Update out session
    if (video_ctx) {
        out_session_lock(out_id);
        ses = get_out_session(out_id);
    }

    switch (cmd->oper) {

    case VIDEO_LIST_OPER_ADD:
        for (i=0; i<cmd->item_count; i++) {
            // Sanity checks
            if (cfg->prog_cnt >= VIDEO_PROG_LIST_SIZE) {
                VIDEO_DEBUG("Too many remap programs\n");
                rc = ENOMEM;
                break;
            }

            uint16 in_prog_num = cmd->in_prog_list[i];
            uint16 out_prog_num = cmd->out_prog_list[i];

            int rc1 = video_list_search(in_prog_num, cfg->in_prog_list,
                                        cfg->prog_cnt);
            int rc2 = video_list_search(out_prog_num, cfg->out_prog_list,
                                        cfg->prog_cnt);
            if (rc1 != -ENOENT || rc2 != -ENOENT) {
                VIDEO_DEBUG("Bad remap program: %d -> %d\n",
                            in_prog_num, out_prog_num);
                continue;
            }

            if (cmd->pid_range[i].last >= NULL_PID) {
                VIDEO_DEBUG("Bad pid range: %d - %d\n",
                            cmd->pid_range[i].first, cmd->pid_range[i].last);
                continue;
            }

            // Add to config
            video_list_set(cmd->in_prog_list[i], cfg->in_prog_list,
                           cfg->prog_cnt);
            video_list_set(cmd->out_prog_list[i], cfg->out_prog_list,
                           cfg->prog_cnt);
            cfg->pid_range[cfg->prog_cnt] = cmd->pid_range[i];

            if (video_ctx) {
                prog = out_prog_lookup_by_in_prog_num(&ses->prog_list,
                                                      in_prog_num);
                if (prog && prog->filtered) {
                    out_prog_set_include(prog, ses, cfg->prog_cnt);
                    out_prog_set_register(prog, 1, 1);
#if 1  // Experimental code!!
                    out_prog_remap_prog(prog, ses);
#endif
                }
            }

            cfg->prog_cnt++;
        }
        break;

    case VIDEO_LIST_OPER_DELETE:
        for (i=0; i<cmd->item_count; i++) {
            uint16 in_prog_num = cmd->in_prog_list[i];

            int idx = video_list_search(in_prog_num, cfg->in_prog_list,
                                        cfg->prog_cnt);
            if (idx == -ENOENT)  continue;

            // Update config
            cfg->prog_cnt--;
            video_list_set(cfg->in_prog_list[cfg->prog_cnt],
                           cfg->in_prog_list, idx);
            video_list_set(cfg->out_prog_list[cfg->prog_cnt],
                           cfg->out_prog_list, idx);
            cfg->pid_range[idx] = cfg->pid_range[cfg->prog_cnt];

            if (video_ctx) {
                prog = out_prog_lookup_by_in_prog_num(&ses->prog_list,
                                                      in_prog_num);
                if (prog && !prog->filtered) {
                    prog->filtered = 1;
                    out_prog_set_exclude(prog, ses, INVALID_CFG_IDX);
                    out_prog_set_register(prog, 1, 1);
                }
            }
        }
        break;

    case VIDEO_LIST_OPER_CLEAR:
        cfg->prog_cnt = 0;

        if (video_ctx) {
            FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
                if (!prog->filtered) {
                    prog->filtered = 1;
                    out_prog_set_exclude(prog, ses, INVALID_CFG_IDX);
                    out_prog_set_register(prog, 1, 1);
                }
            }
        }
        break;

    default:
        VIDEO_DEBUG("PROG REMAP: Unknown list operation %d\n", cmd->oper);
    }

    if (video_ctx) {
        out_session_unlock(out_id);
    }

    video_db_out_cnfg_checkin(ctx->id, out_id, cfg);

done:
    VIDEO_MSG_DEBUG("\nStatus %d, transaction %d", rc, cmd->transaction_id);

    if (rc != EOK) {
        video_event_fire_config_err(IPC_HEADER(ipc_msg)->type, cmd->oper, 0,
                                    cmd->resource_id, rc, 0, NULL);
    }
}


void process_video_query_crsl_insert (ipc_message *qry_msg,
                                      ipc_message *rsp_msg)
{
    video_query_crsl_insert_t *qry = IPC_DATA(qry_msg);
    video_reply_crsl_insert_t *rsp = IPC_DATA(rsp_msg);
    memset(rsp_msg, 0, MSG_BUF_SIZE);

    VIDEO_MSG_DEBUG("\n\nQUERY_CRSL: primary %d, transaction %d",
                    qry->primary_id, qry->transaction_id);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(qry_msg)->type;
    rsp->primary_id = qry->primary_id;
    rsp->owner_id = qry->owner_id;
    rsp->transaction_id = qry->transaction_id;
    rsp->num_item = qry->num_item;

    int rc = EOK;
    video_context_t* ctx
            = get_config_context(qry->primary_id, "QUERY_CRSL", &rc);
    VIDEO_MSG_DEBUG("Items %d\n", qry->num_item);

    uint32 i;
    for (i=0; i<rsp->num_item; i++) {
        uint32 rid = qry->item[i].resource_id;
        uint16 qam_id = qry->item[i].qam_id;
        VIDEO_MSG_DEBUG("Resource %d: QAM %d\n", rid, qam_id);

        rsp->item[i].resource_id = rid;
        rsp->item[i].qam_id = qam_id;

        if (!ctx) {
            rc = CRSL_NOT_FOUND;
        } else {
            uint16 crsl_id = resource_id_to_crsl_id(ctx, rid);
            if (crsl_id == INVALID_CRSL_ID) {
                rc = CRSL_NOT_FOUND;
            } else {
                crsl_t* crsl = get_crsl(ctx, crsl_id);
                if (qam_id >= NUM_QAMS ||
                        !bit_mask_test(crsl->qam_mask, qam_id)) {
                    rc = CRSL_INSERT_NOT_FOUND;
                } else {
                    crsl_insert_t* insert
                        = find_crsl_insert(
                             get_qam_crsl_insert_list(ctx, qam_id), crsl);
                    assert(insert);
                    rc = (insert->target_cnt == CRSL_INSERT_CONTINUOUS ||
                          insert->cnt < insert->target_cnt) ?
                             CRSL_INSERT_ACTIVE : CRSL_INSERT_INACTIVE;
                    rsp->item[i].insert_cnt = insert->cnt;
                }
            }
        }
        rsp->item[i].state = rc;
    }

    ipc_message_header* rsp_hdr = IPC_HEADER(rsp_msg);
    rsp_hdr->type = MSG_TYPE_VIDEO_QUERY_CAROUSEL_INSERT;
    rsp_hdr->size = sizeof(*rsp) + rsp->num_item * sizeof(crsl_insert_resp_t);
}


#if 0
static
void process_video_session_summary (video_query_cmd* cmd, int msg_size,
                                    ipc_message **rpy_msg)
{
    uint32 i;
    video_list_summary_resp *resp;
    rfgw_tlv_t *tlv;
    uint32 tot_tlv_len;
    in_session_t * in_sess;
    out_session_t* out_sess;
    int qam_id = INVALID_ID;
    uint32 num_sess_found = 0;
    
    *rpy_msg = ipc_get_pak_message(sizeof(*resp), NULL, 0);
    if (!*rpy_msg) {
        em_vidman_IPC_MALLOC_BUFFER_FAILED("LIST_SESSION_SUMMARY", "reply");
        return;
    }

    resp = IPC_DATA(*rpy_msg);
    resp->status = STATUS_NOT_SET;
    memset(&resp->info, 0, sizeof (video_stat_summary_t));

    VIDEO_MSG_DEBUG("LIST_SESSION_SUMMARY: primary %d, transaction %d",
                     cmd->primary_id, cmd->transaction_id);

    if (!lcred_is_valid_primary(cmd->primary_id)) {
        em_vidman_VIDEO_BAD_PRIMARY_ID("LIST_SESSION_SUMMARY", cmd->primary_id);
        resp->status = EINVAL;
        goto send_resp;
    }

    //TODO: check to make sure the linecard has gone hot.

     // Process config TLVs
    //
    tlv = cmd->tlv;
    tot_tlv_len = msg_size - sizeof(video_query_cmd);

    while (1) {
        if (tot_tlv_len == 0) {
            break;        // end of message reached
        }
        if (tot_tlv_len < RFGW_TLV_HDR_SIZE) {
            goto incomplete_tlv;
        }

        switch (tlv->type) {
        case TYPE_QAM_ID:
            VIDEO_MSG_DEBUG("  QAM_ID %d", tlv->value[0]);

            if (tlv_check(tlv, tot_tlv_len, sizeof(uint32))) {
                goto incomplete_tlv;
            }
            // make sure that we've only received this filter once
            assert(qam_id == INVALID_ID);
            
            qam_id = tlv->value[0];
            if (qam_id < 0 || qam_id >= NUM_QAMS) {
                em_vidman_VIDEO_BAD_QAM_ID("LIST_SESSION_SUMMARY", qam_id);

                resp->status = EINVAL;
                goto send_resp;
            }
            break;

        default:
            em_vidman_VIDEO_UNKNOWN_MSG("LIST_SESSION_SUMMARY", tlv->type);
            resp->status = ENOTSUP;
            goto send_resp;
        }

        // Go to next TLV
        tot_tlv_len -= tlv_next(&tlv);
    }

    for (i = 0; i < MAX_SESSIONS; i++) {
        out_sess = get_out_session(i);

        // if this session is not in use then we skip
        if(!session_is_used(out_sess->state) || 
           !session_is_used(out_sess->in_ses->state)) {
            continue;
        }
        in_sess = out_sess->in_ses;

        // Apply filters
        if(qam_id != INVALID_ID) {
            // this means we are filtering based off of a qam id
            if(out_sess->cfg->qam_id != qam_id) {
                continue;
            }
        }

        // fill out the response
        
        // get session state
        if (session_is_off(in_sess->state)) {
            resp->info.off_cnt++;
        } else if (session_is_idle(in_sess->state)) {
            resp->info.idle_cnt++;
        } else if (session_is_active(in_sess->state)) {
            resp->info.active_cnt++;
        } else if (session_is_init(in_sess->state)) {
            resp->info.init_cnt++;
        }


        if (session_is_enabled(in_sess->state & out_sess->state)) {
            resp->info.enable_cnt++;
        }

        if (session_is_blocked(in_sess->state | out_sess->state) || 
            session_is_no_resources(in_sess->state | out_sess->state)) {
            resp->info.block_cnt++;
        }
        
        if (session_is_psi_ready(in_sess->state & out_sess->state)) {
            resp->info.psi_rdy_cnt++;
        }

        if (session_is_udp(in_sess->cfg)) {
            resp->info.udp_cnt++;
        } else if (session_is_asm(in_sess->cfg)) {
            resp->info.asm_cnt++;
        } else if (session_is_ssm(in_sess->cfg)) {
            resp->info.ssm_cnt++;
        }

        if (session_is_mux(in_sess->cfg)) {
            resp->info.mux_cnt++;
        } else if (session_is_passthru(in_sess->cfg)) {
            resp->info.passthru_cnt++;
        } else if (session_is_data(in_sess->cfg)) {
            resp->info.data_cnt++;
        }

        if (session_is_active(in_sess->state)) {
            resp->info.measured_bitrate += in_sess->avg_bitrate;
        }
        
        num_sess_found++;
    }

    if (qam_id != INVALID_ID) {
        resp->info.measured_bitrate += qam_crsl_insert_bitrate(qam_id);
    } else {
        for (qam_id = 0; qam_id < NUM_QAMS; qam_id++) {
            resp->info.measured_bitrate += qam_crsl_insert_bitrate(qam_id);
        }
    }

    resp->status = EOK;

send_resp:
    VIDEO_MSG_DEBUG("LIST_SESSION_SUMMARY: %d sessions", num_sess_found);
    resp->primary_id = cmd->primary_id;
    resp->transaction_id = cmd->transaction_id;
    resp->header.type = cmd->header.type;
    resp->header.len = sizeof(*resp) - sizeof(rfgw_msg_hdr_t);
    assert(resp->status != STATUS_NOT_SET);

    VIDEO_MSG_DEBUG("Status %d, transaction %d",
                    resp->status, resp->transaction_id);
    return;

incomplete_tlv:
    em_vidman_VIDEO_INCOMPLETE_MSG("LIST_SESSION_SUMMARY");
    resp->status = EINVAL;
    goto send_resp;
}
#endif  // 0


#if 0
// Handler for MSG_TYPE_VIDEO_LIST_OUT_SESSION
//
static
void process_video_list_out_session (video_list_cmd* cmd, int msg_size,
                                     ipc_message **rpy_msg)
{
    video_list_resp *resp;
    rfgw_tlv_t *tlv;
    uint32 tot_tlv_len;
    uint32 i;
    in_session_t * in_sess;
    out_session_t* out_sess;
    uint16 *session_state;
    int32 qam_id = INVALID_ID;
    uint32 max_num_sess_info = (VIDEO_MSG_BUF_SIZE - sizeof(video_list_resp)) 
                                        / sizeof(sess_info_t);
    uint32 num_sess_found = 0;
    
    *rpy_msg = ipc_get_pak_message(sizeof(*resp), NULL, 0);
    if (!*rpy_msg) {
        em_vidman_IPC_MALLOC_BUFFER_FAILED("LIST_OUT_SESSION", "reply");
        return;
    }

    resp = IPC_DATA(*rpy_msg);
    resp->status = STATUS_NOT_SET;

    VIDEO_MSG_DEBUG("LIST_OUT_SESSION: primary %d, transaction %d, "
                    "start resource id: %d",
                     cmd->primary_id, cmd->transaction_id, 
                     cmd->start_resource_id);

    if (!lcred_is_valid_primary(cmd->primary_id)) {
        em_vidman_VIDEO_BAD_PRIMARY_ID("LIST_OUT_SESSION", cmd->primary_id);
        resp->status = EINVAL;
        goto send_resp;
    }

    //TODO: check to make sure the linecard has gone hot.

    // Process config TLVs
    //
    tlv = cmd->filter_tlv;
    tot_tlv_len = msg_size - sizeof(video_list_cmd);

    while (1) {
        if (tot_tlv_len == 0) {
            break;        // end of message reached
        }
        if (tot_tlv_len < RFGW_TLV_HDR_SIZE) {
            goto incomplete_tlv;
        }

        switch (tlv->type) {
        case TYPE_QAM_ID:
            VIDEO_MSG_DEBUG("  QAM_ID %d", tlv->value[0]);

            if (tlv_check(tlv, tot_tlv_len, sizeof(uint32))) {
                goto incomplete_tlv;
            }
            // make sure that we've only received this filter once
            assert(qam_id == INVALID_ID);
            
            qam_id = tlv->value[0];
            if (qam_id >= NUM_QAMS) {
                em_vidman_VIDEO_BAD_QAM_ID("LIST_OUT_SESSION", qam_id);
                resp->status = EINVAL;
                goto send_resp;
            }
            break;

        default:
            em_vidman_VIDEO_UNKNOWN_MSG("LIST_OUT_SESSION", tlv->type);
            resp->status = ENOTSUP;
            goto send_resp;
        }

        // Go to next TLV
        tot_tlv_len -= tlv_next(&tlv);
    }

    for(i = cmd->start_resource_id; i < MAX_SESSIONS; i++) {
        out_sess = get_out_session(i);

        // if this session is not in use then we skip
        if(!session_is_used(out_sess->state) || 
           !session_is_used(out_sess->in_ses->state)) {
            continue;
        }
        in_sess = out_sess->in_ses;

        // Apply filters
        if (qam_id != INVALID_ID) {
            //this means we are filtering based off of a qam id
            if(out_sess->cfg->qam_id != qam_id) {
                continue;
            }
        }

        // we have found a session, but can not fit it
        // we break and set value to too large. 
        if (num_sess_found == max_num_sess_info + 1) {
            VIDEO_MSG_DEBUG(LC_ERRMSG_VIDEO_RESP_MSG_TRUNCATED, 
                "LIST_OUT_SESSION");
            resp->status = E2BIG;
            goto send_resp;
        }
        session_state = &resp->info[num_sess_found].session_state;
       
        //fill out the response
        resp->info[num_sess_found].ses_id = out_sess->cfg->vscm_id;
        *session_state = 0 ;
        *session_state |= session_is_off(in_sess->state) ?
                              SESSION_STATE_OFF : 0;
        *session_state |= session_is_idle(in_sess->state) ?
                              SESSION_STATE_IDLE : 0;
        *session_state |= session_is_init(in_sess->state) ?
                              SESSION_STATE_INIT : 0;
        *session_state |= session_is_active(in_sess->state) ?
                              SESSION_STATE_ACTIVE : 0;
        *session_state |= session_is_used(in_sess->state & out_sess->state) ?
                              SESSION_STATE_USED : 0;
        *session_state |= session_is_enabled(in_sess->state & out_sess->state) ?
                              SESSION_STATE_ENABLED : 0;
        *session_state |= session_is_psi_ready(in_sess->state & out_sess->state) ?
                              SESSION_STATE_PSI_READY : 0;
        *session_state |= session_is_blocked(in_sess->state | out_sess->state) || 
                          session_is_no_resources(in_sess->state | 
                                                               out_sess->state) ?
                              SESSION_STATE_BLOCKED : 0;
        resp->info[num_sess_found].program_cnt = in_sess->pat.prog_cnt;
        resp->info[num_sess_found].measured_bitrate = in_sess->avg_bitrate;

        num_sess_found++;
    }

    resp->status = EOK;

send_resp:
    VIDEO_MSG_DEBUG("LIST_OUT_SESSION: num sessions found %d",
                        num_sess_found);
    resp->primary_id = cmd->primary_id;
    resp->transaction_id = cmd->transaction_id;
    resp->header.type = cmd->header.type;
    assert(resp->status != STATUS_NOT_SET);
    
    // adjust the payload size
    IPC_HEADER(*rpy_msg)->size += num_sess_found * sizeof(sess_info_t);
    resp->header.len = IPC_HEADER(*rpy_msg)->size - sizeof(rfgw_msg_hdr_t);

    VIDEO_MSG_DEBUG("Status %d, transaction %d",
                    resp->status, resp->transaction_id);
    return;

incomplete_tlv:
    em_vidman_VIDEO_INCOMPLETE_MSG("LIST_OUT_SESSION");
    resp->status = EINVAL;
    goto send_resp;
}
#endif  // 0


#if 0
// Handler of the VIDEO_MULTICAST_SESSION_RESTART message
//   - re-enable session updates
//   - reset timers to IDLE state
//   - put all sessions to INIT state unless it is ACTIVE
//
static
void process_video_multicast_session_restart (void)
{
    int i;
    mpeg_clock_t now;

    get_mpeg_time(&now);

    for (i=0; i<MAX_SESSIONS; i++) {
        in_session_t* ses = get_in_session(i);
        if (!session_is_used(ses->state)) {
            continue;
        }

        in_session_lock(i);
        ses->prev_tp_arvl = now.base;
        if (session_is_active(ses->state)) {
            session_set_init(&ses->state);
        }
        in_session_unlock(i);
    }

    session_state_update_flag = TRUE;
    VIDEO_LOG("Session state updates re-enabled");
}
#endif

static void sort_program(uint16 *p_prg, uint16 no_of_prog)
{
    uint16 cnt_i,cnt_j,temp_prg;

    // simple logic to have the indexes sorted.
    for (cnt_i = 0; cnt_i < no_of_prog; ++cnt_i)
    {
        for (cnt_j = cnt_i+1; cnt_j < no_of_prog; ++cnt_j)
        {
            if (p_prg[cnt_j] < p_prg[cnt_i])
            {
                temp_prg = p_prg[cnt_j];
                p_prg[cnt_j] = p_prg[cnt_i];
                p_prg[cnt_i] = temp_prg;
            }
        }
    }
}

void process_video_snmp_query_input_ts_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{

    video_query_snmp_input_ts_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_input_ts_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint64_t item_val = 0;

    VIDEO_MSG_DEBUG("InputTsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_input_ts_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;


    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP input ts info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;

        switch(req->arg) {
            case 1:
                // 1- SPTS, 2 - MPTS;
                item_val = (uint64_t)((in_ses->flags & SESSION_FLAG_MPTS)?2:1);
            break;
            case 2:
                // 2 - UDP, 1 - Other
                item_val = 2;
            break;
            case 3:
            case 4:
                // TS Connection or Active TS Connection
                item_val = 0;
            break;
            case 5:
                // true or false
                item_val = (uint64_t)((in_ses->state & SESSION_STATE_PSI_READY)?1:2);
            break;
            case 6:
                item_val = in_ses->stat.start_time;
            break;
            case 7:
                // true or false
                item_val = (uint64_t)((in_ses->state & SESSION_STATE_NO_RESOURCES)?2:1);
            break;
            case 8:
                item_val = (uint64_t)(in_ses->pat.prog_cnt);
            break;
            case 9:
                item_val = (uint64_t)(in_ses->avg_bitrate);
            break;
            case 10:
                item_val = (uint64_t)(in_ses->max_bitrate);
            break;
            case 11:
                item_val = (uint64_t)(in_ses->pat_ver);
            break;
            case 12:
                item_val = 0;
            break;
            case 13:
                item_val = (uint64_t)(in_ses->pat.nit_pid);
            break;
            case 14:
                item_val = 0;
            break;
            case 15:
                item_val = (uint64_t)(in_ses->pat.tsid);
            break;
            case 16:
                // 4 - Not Monitored
                item_val = 4;
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
             break;
        }

        resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_input_prog_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_input_prog_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_input_prog_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint8_t *p_item_val_buf = item_val_buf;
    uint16_t buf_len = 0, item_len = 0;
    uint16 *p_prg_lst = NULL;

    VIDEO_MSG_DEBUG("InputProgInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_input_prog_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP input prog info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0; (i < no_of_sessions) && (resp->no_of_sessions < 50);++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;
        in_prog_t *in_prog = NULL;
        uint16 no_of_prog, cnt_prg;

        psi_lock(in_ses->id);
        no_of_prog = in_ses->pat.prog_cnt;
        psi_unlock(in_ses->id);

        if(no_of_prog > 0)
        {
            if(p_prg_lst) free(p_prg_lst);
        
            p_prg_lst = malloc( no_of_prog * sizeof(uint16) );
            if(!p_prg_lst) {
                VIDEO_LOG("InputProgInfo: unable to allocate memeory\n");
                break;
            }
        }
        else
        {
            continue;
        }

        psi_lock(in_ses->id);
        for(cnt_prg=0;cnt_prg<no_of_prog;++cnt_prg)
        {
            p_prg_lst[cnt_prg] = pat_get_prog_num(in_ses->pat.prog_info[cnt_prg]);
        }
        psi_unlock(in_ses->id);

        sort_program(p_prg_lst, no_of_prog);

        for(cnt_prg=0; (cnt_prg < no_of_prog) && (resp->no_of_sessions < 50); ++cnt_prg)
        {
            if (p_prg_lst[cnt_prg] < req->input_prog_index[i]) {
                continue;
            }

            psi_lock(in_ses->id);
            in_prog = in_prog_lookup_by_prog_num(&in_ses->prog_list, p_prg_lst[cnt_prg]);
            psi_unlock(in_ses->id);
            if (!in_prog) {
                VIDEO_LOG("Program Info for Program index %d is not found\n",
                             req->input_prog_index[i]);
                continue;
            }

            switch(req->arg) {
                case 1: //  I_mpegInputProgNo
                    *((uint32_t *)p_item_val_buf) = in_prog->prog_num;
                    item_len = sizeof(uint32_t);
                break;
                case 2: //  I_mpegInputProgPmtVersion
                    *((uint32_t *)p_item_val_buf) = in_prog->pmt_ver;
                    item_len = sizeof(uint32_t);
                break;
                case 3: //  I_mpegInputProgPmtPid
                    *((uint32_t *)p_item_val_buf) = in_prog->pmt_pid;
                    item_len = sizeof(uint32_t);
                break;
                case 4: //  I_mpegInputProgPcrPid
                    *((uint32_t *)p_item_val_buf) = in_prog->pmt.pcr_pid;
                    item_len = sizeof(uint32_t);
                break;
                case 5: //  I_mpegInputProgEcmPid
                    if(in_prog->pmt.ca_desc_cnt > 0){
                        // report only the first ecm pid
                        ca_descriptor_header_t* ca_desc = 
                                              pmt_info_get_ca_desc(&in_prog->pmt,0);
                        
                        *((uint32_t *)p_item_val_buf)  = 
                                                 (uint32_t)ca_desc_get_pid(ca_desc);
                    }
                    else {
                        *((uint32_t *)p_item_val_buf)  = 0;
                    }
                    item_len = sizeof(uint32_t);
                break;
                case 6: //  I_mpegInputProgNumElems
                    *((uint32_t *)p_item_val_buf) = in_prog->pmt.es_cnt;
                    item_len = sizeof(uint32_t);
                break;
                case 7: //  I_mpegInputProgNumEcms
                    *((uint32_t *)p_item_val_buf) = in_prog->pmt.ca_desc_cnt;
                    item_len = sizeof(uint32_t);
                break;
                case 8: //  I_mpegInputProgCaDescr
                {
                    ca_descriptor_header_t* ca_desc = NULL;
                    item_len = 0;

                    if(in_prog->pmt.ca_desc_cnt > 0){
                        ca_desc = pmt_info_get_ca_desc(&in_prog->pmt,0);
                        item_len = ca_desc->len + 2;
                    }

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);

                    if(ca_desc && item_len) {
                        memcpy(p_item_val_buf,ca_desc,item_len);
                    }
                }
                break;
                case 9: //  I_mpegInputProgScte35Descr
                    //item_len = strlen("SCTE35");
                    item_len = 0;

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);
                    
                    //strcpy(p_item_val_buf,"SCTE35");

                break;
                case 10: //  I_mpegInputProgScte18Descr
                    //item_len = strlen("SCTE18");
                    item_len = 0;

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);
                    
                    //strcpy(p_item_val_buf,"SCTE18");
                break;
                default:
                    rc = 1;
                    VIDEO_LOG("Invalid arg type %d\n", req->arg);
                 break;
            }

            if(rc) {
                break;
            }

            VIDEO_LOG("input ts index %d, prog id %d\n"
                ,req->input_ts_index[i], in_prog->prog_num);

            resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
            resp->prog_index[resp->no_of_sessions] = in_prog->prog_num;
            buf_len = buf_len + item_len;
            p_item_val_buf = p_item_val_buf + item_len;

            resp->no_of_sessions++;
        }
        
        if(rc) {
            break;
        }

    }

    resp->item_value_len = buf_len;
    if(buf_len) resp->item_value = item_val_buf;
    else resp->item_value = NULL;

    if(p_prg_lst){
        free(p_prg_lst);
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_prog_es_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_prog_es_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_prog_es_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,j,rc,no_of_sessions;
    uint8_t *p_item_val_buf = item_val_buf;
    uint16_t buf_len = 0, item_len = 0;
    uint16 *p_prg_lst = NULL;

    VIDEO_MSG_DEBUG("ProgEsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_prog_es_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP input prog es info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0; (i < no_of_sessions) && (resp->no_of_sessions < 50); ++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;
        in_prog_t *in_prog = NULL;
        uint16 no_of_prog, cnt_prg;

        psi_lock(in_ses->id);
        no_of_prog = in_ses->pat.prog_cnt;
        psi_unlock(in_ses->id);

        if(no_of_prog > 0)
        {
            if(p_prg_lst) free(p_prg_lst);
        
            p_prg_lst = malloc( no_of_prog * sizeof(uint16) );
            if(!p_prg_lst) {
                VIDEO_LOG("ProgEsInfo: unable to allocate memeory\n");
                break;
            }
        }
        else
        {
            continue;
        }
        
        psi_lock(in_ses->id);
        for(cnt_prg=0; cnt_prg < no_of_prog; ++cnt_prg)
        {
            p_prg_lst[cnt_prg] = pat_get_prog_num(in_ses->pat.prog_info[cnt_prg]);
        }
        psi_unlock(in_ses->id);

        sort_program(p_prg_lst, no_of_prog);

        for(cnt_prg=0; (cnt_prg < no_of_prog) && (resp->no_of_sessions < 50); ++cnt_prg)
        {
            if (p_prg_lst[cnt_prg] < req->input_prog_index[i]) {
                continue;
            }

            psi_lock(in_ses->id);
            in_prog = in_prog_lookup_by_prog_num(&in_ses->prog_list, p_prg_lst[cnt_prg]);
            psi_unlock(in_ses->id);
            
            if (!in_prog) {
                VIDEO_LOG("Program Info for Program index %d is not found\n",
                             req->input_prog_index[i]);
                continue;
            }

            if( (i == 0) && 
                (p_prg_lst[cnt_prg] == req->input_prog_index[i]) &&
                (req->input_es_index > in_prog->pmt.es_cnt) ) {
                continue;
            }

            if( (resp->no_of_sessions + in_prog->pmt.es_cnt) > 50) {
                VIDEO_LOG("Max no of elem in resp reached.\n");
                rc = 2;
                break;
            }

            ca_descriptor_header_t* ca_desc = NULL;
            
            if(in_prog->pmt.ca_desc_cnt > 0){
                ca_desc = pmt_info_get_ca_desc(&in_prog->pmt,0);
            }
            
            for(j=0; j<in_prog->pmt.es_cnt; ++j) {

                pmt_es_info_t* es_info = pmt_info_get_es(&(in_prog->pmt), j);

                switch(req->arg) {
                    case 1: //  I_mpegProgESPID
                        *((uint32_t *)p_item_val_buf) = pmt_get_es_pid(es_info);
                        item_len = sizeof(int32_t);
                    break;
                    case 2: //  I_mpegProgESType
                        *((uint32_t *)p_item_val_buf) = es_info->es_type;
                        item_len = sizeof(int32_t);
                    break;
                    case 3: //  I_mpegProgESCaDescr
                        item_len = 0;
                        
                        if(ca_desc){
                            item_len = ca_desc->len + 2;
                        }
                        
                        if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                            rc = 2;
                            break;
                        }
                        
                        *((uint16_t *)p_item_val_buf) = item_len;
                        p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                        buf_len = buf_len + sizeof(uint16_t);
                        
                        if(ca_desc && item_len) {
                            memcpy(p_item_val_buf,ca_desc,item_len);
                        }
                    break;
                    case 4: //  I_mpegProgESScte35Descr
                        //item_len = strlen("SCTE35");
                        item_len = 0;
                        
                        if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                            rc = 2;
                            break;
                        }
                        
                        *((uint16_t *)p_item_val_buf) = item_len;
                        p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                        buf_len = buf_len + sizeof(uint16_t);
                        
                        //strcpy(p_item_val_buf,"SCTE35");
                    break;
                    case 5: //  I_mpegProgESScte18Descr
                        //item_len = strlen("SCTE18");
                        item_len = 0;
                        
                        if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                            rc = 2;
                            break;
                        }
                        
                        *((uint16_t *)p_item_val_buf) = item_len;
                        p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                        buf_len = buf_len + sizeof(uint16_t);
                        
                        //strcpy(p_item_val_buf,"SCTE18");
                    break;
                    default:
                        rc = 1;
                        VIDEO_LOG("Invalid arg type %d\n", req->arg);
                     break;
                }

                if(rc) {
                    break;
                }

                VIDEO_LOG("input ts index %d, prog id %d, es id  %d\n"
                    ,req->input_ts_index[i],in_prog->prog_num, j+1);

                resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
                resp->prog_index[resp->no_of_sessions] = in_prog->prog_num;
                resp->es_index[resp->no_of_sessions] = j + 1;
                buf_len = buf_len + item_len;
                p_item_val_buf = p_item_val_buf + item_len;

                resp->no_of_sessions++;
            }

            if(rc) {
                break;
            }

        }

        if(rc) {
            break;
        }

    }

    resp->item_value_len = buf_len;
    if(buf_len) resp->item_value = item_val_buf;
    else resp->item_value = NULL;

    if(p_prg_lst){
        free(p_prg_lst);
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}


void process_video_snmp_query_input_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_input_stats_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_input_stats_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint32_t item_val = 0;

    VIDEO_MSG_DEBUG("InputStatsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_input_stats_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP input stats info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;

        switch(req->arg) {
            case 0: //  I_mpegInputStatsPcrJitter
                item_val = -1;
            break;
            case 1: //  I_mpegInputStatsMaxPacketJitter
                //item_val = -1;
                if (in_ses->pcr_ctx_cnt > 0) {
                    item_val = mpeg_time_to_ms(in_ses->jitter);
                }
                else {
                    item_val = 0;
                }
            break;
            case 2: //  I_mpegInputStatsPcrPackets
                item_val = in_ses->stat.pcr_tp_cnt;
            break;
            case 3: //  I_mpegInputStatsNonPcrPackets
                item_val = in_ses->stat.non_pcr_tp_cnt;
            break;
            case 4: //  I_mpegInputStatsUnexpectedPackets
                item_val = in_ses->stat.unref_tp_cnt;
            break;
            case 5: //  I_mpegInputStatsContinuityErrors
                item_val = in_ses->stat.cc_err_cnt;
            break;
            case 6: //  I_mpegInputStatsSyncLossPackets
                item_val = in_ses->stat.sync_loss_cnt;
            break;
            case 7: //  I_mpegInputStatsPcrIntervalExceeds
                item_val = in_ses->stat.pcr_xover_cnt;
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        VIDEO_LOG("input ts index = %d, value = %d\n"
            ,req->input_ts_index[resp->no_of_sessions],item_val);

        resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_input_udp_org_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_input_udp_org_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_input_udp_org_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint32_t item_val = 0;

    VIDEO_MSG_DEBUG("InputUdpOrgInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_input_udp_org_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP input udp org info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;

        switch(req->arg) {
            case 2: // I_mpegInputUdpOriginationIfIndex
                item_val = -1;
            break;
            case 3: //  I_mpegInputUdpOriginationInetAddrType
                item_val = 1;   // IPv4
            break;
            case 4: //  I_mpegInputUdpOriginationSrcInetAddr
                item_val = in_ses->cfg->src_ip[0];
            break;
            case 5: //  I_mpegInputUdpOriginationDestInetAddr
                item_val = in_ses->cfg->dst_ip[0];
            break;
            case 6: //  I_mpegInputUdpOriginationDestPort
                item_val = in_ses->cfg->dst_udp;
            break;
            case 7: //  I_mpegInputUdpOriginationActive (true or false)
                item_val = session_is_active(in_ses->state)?1:2;
            break;
            case 8: //  I_mpegInputUdpOriginationPacketsDetected (true or false)
                item_val = session_is_active(in_ses->state)?1:2;
            break;
            case 9: //  I_mpegInputUdpOriginationRank
                item_val = 0;
            break;
            case 10: //  I_mpegInputUdpOriginationInputTSIndex
                item_val = 0;
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        VIDEO_LOG("input ts index = %d, value = %d\n"
            ,req->input_ts_index[resp->no_of_sessions],item_val);

        resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_output_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_output_stats_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_output_stats_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint64_t item_val = 0;

    VIDEO_MSG_DEBUG("OutputStatsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_output_stats_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
 //   resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP output stats info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        switch(req->arg) {

            case 0: //  I_mpegOutputStatsDroppedPackets
                item_val = (uint64_t)out_ses->stat.drop_tp_cnt;
            break;
            case 1: //  I_mpegOutputStatsFifoOverflow
                item_val = (uint64_t)out_ses->stat.overflow_cnt;
            break;
            case 2: //  I_mpegOutputStatsFifoUnderflow
                item_val = (uint64_t)out_ses->stat.underflow_cnt;
            break;
            case 3: //  I_mpegOutputStatsDataRate
                item_val = out_ses->avg_bitrate;
            break;
            case 4: //  I_mpegOutputStatsAvailableBandwidth
                item_val = (uint64_t)(out_ses->in_ses->cfg->bitrate_alloc);
            break;
            case 5: //  I_mpegOutputStatsChannelUtilization
                item_val = (uint64_t)(out_ses->avg_bitrate);
            break;
            case 6: //  I_mpegOutputStatsTotalPackets
                item_val = (uint64_t)(out_ses->stat.tp_cnt);
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        VIDEO_LOG("output ts index = %d, value = %d\n"
            ,req->session_id[resp->no_of_sessions],(int)item_val);

        resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_output_ts_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_output_ts_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_output_ts_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint64_t item_val = 0;

    VIDEO_MSG_DEBUG("OutputTsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_output_ts_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
 //   resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP output ts info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }

        uint8_t no_of_prog = 0;
        out_prog_t *prog;
        
        FOR_ALL_ELEMENTS_IN_QUE(&out_ses->prog_list, prog) {
            no_of_prog++;
        }
        
        qam_info_t* qam = get_qam_info((out_ses->cfg)->qam_id);
        if(!qam) {
                VIDEO_LOG("Unable to get qam info for qam id %d out_ses_id %d\n",
                             (out_ses->cfg)->qam_id, out_ses_id);
                continue;
        }

        switch(req->arg) {

            case 1: //  I_mpegOutputTSType
                item_val = (out_ses->flags & SESSION_FLAG_MPTS)?2:1;
            break;
            case 2: //  I_mpegOutputTSConnectionType
                item_val = 2; // Qam
            break;
            case 3: //  I_mpegOutputTSConnection
                item_val = 0;   // led to fill in
            break;
            case 4: //  I_mpegOutputTSNumPrograms
                item_val = no_of_prog;
            break;
            case 5: //  I_mpegOutputTSTSID
                item_val = qam->cfg->tsid;
            break;
            case 6: //  I_mpegOutputTSNitPid
                item_val = 65535;
            break;
            case 7: //  I_mpegOutputTSCaPid
                item_val = 65535;
            break;
            case 8: //  I_mpegOutputTSCatInsertRate
                item_val = 0;
            break;
            case 9: //  I_mpegOutputTSPatInsertRate
                item_val = qam->cfg->psi_period;
            break;
            case 10: //  I_mpegOutputTSPmtInsertRate
                item_val = qam->cfg->psi_period;
            break;
            case 11: //  I_mpegOutputTSStartTime
                item_val = out_ses->stat.start_time;
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        VIDEO_LOG("output ts index = %d, value = %d\n"
            ,req->session_id[resp->no_of_sessions],(int)item_val);

        resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("No of sess in resp %d\n", resp->no_of_sessions);

    return;
}


void process_video_snmp_query_output_prog_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_output_prog_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_output_prog_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint8_t *p_item_val_buf = item_val_buf;
    uint16_t buf_len = 0, item_len = 0;
    uint16 *p_prg_lst = NULL;
    uint16_t ecm_pid[MAX_CA_DESC_PER_PROG];

    VIDEO_MSG_DEBUG("OutputProgInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_output_prog_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP output prog info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;(i < no_of_sessions) && (resp->no_of_sessions < 50);++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }

        out_prog_t *out_prog = NULL;
        uint16 no_of_prog, cnt_prg;

        no_of_prog = que_get_size(NULL,&(out_ses->prog_list));

        if(no_of_prog > 0)
        {
            if(p_prg_lst) free(p_prg_lst);
        
            p_prg_lst = malloc( no_of_prog * sizeof(uint16) );
            if(!p_prg_lst) {
                VIDEO_LOG("OutputProgInfo: unable to allocate memeory\n");
                break;
            }
        }
        else
        {
            continue;
        }

        cnt_prg = 0;
        FOR_ALL_ELEMENTS_IN_QUE(&out_ses->prog_list, out_prog) {
            if(cnt_prg >= no_of_prog) {
                break;
            }
            p_prg_lst[cnt_prg] = out_prog->prog_num;
            ++cnt_prg;
        }

        sort_program(p_prg_lst, no_of_prog);

        for(cnt_prg=0; (cnt_prg < no_of_prog) && (resp->no_of_sessions < 50); ++cnt_prg)
        {
            if (p_prg_lst[cnt_prg] < req->output_prog_index[i]) {
                continue;
            }

            out_prog = out_prog_lookup_by_prog_num(&out_ses->prog_list,p_prg_lst[cnt_prg]);
            
            if (!out_prog) {
                VIDEO_LOG(" Program Info for Program index %d is not found\n",
                             req->output_prog_index[i]);
                continue;
            }

            switch(req->arg) {
                case 1: //  I_mpegOutputProgNo
                    *((uint32_t *)p_item_val_buf) = out_prog->prog_num;
                    item_len = sizeof(uint32_t);
                break;
                case 2: //  I_mpegOutputProgPmtVersion
                    *((uint32_t *)p_item_val_buf) = out_prog->pmt_ver;
                    item_len = sizeof(uint32_t);
                break;
                case 3: //  I_mpegOutputProgPmtPid
                    *((uint32_t *)p_item_val_buf) = out_prog->pmt_pid;
                    item_len = sizeof(uint32_t);
                break;
                case 4: //  I_mpegOutputProgPcrPid
                    *((uint32_t *)p_item_val_buf) = out_prog->pmt.pcr_pid;
                    item_len = sizeof(uint32_t);
                break;
                case 5: //  I_mpegOutputProgEcmPid
                if(out_prog->pmt.ca_desc_cnt > 0){
                    // report only the first ecm pid
                    ca_descriptor_header_t* ca_desc = 
                                          pmt_info_get_ca_desc(&out_prog->pmt,0);
                    
                    *((uint32_t *)p_item_val_buf)  = 
                                             (uint32_t)ca_desc_get_pid(ca_desc);
                }
                else {
                    *((uint32_t *)p_item_val_buf)  = 0;
                }

                item_len = sizeof(uint32_t);

                break;
                case 6: //  I_mpegOutputProgNumElems
                    *((uint32_t *)p_item_val_buf) = out_prog->pmt.es_cnt;
                    item_len = sizeof(uint32_t);
                break;
                case 7: //  I_mpegOutputProgNumEcms
                {
                    ca_descriptor_header_t* ca_desc;
                    uint32_t num_of_ecms = 0,j,k;
                    for(j=0;j<out_prog->pmt.ca_desc_cnt;++j) {

                        ca_desc = pmt_info_get_ca_desc(&out_prog->pmt,j);
                        ecm_pid[j] = ca_desc_get_pid(ca_desc);

                        for(k=0;k<j;++k) {
                            if(ecm_pid[j] == ecm_pid[k]){
                                break;
                            }
                        }

                        if( j==0 || k>=j) ++num_of_ecms;
                    }

                    *((uint32_t *)p_item_val_buf) = (uint32_t)num_of_ecms;
                    item_len = sizeof(uint32_t);
                }
                break;
                case 8: //  I_mpegOutputProgCaDescr
                {
                    ca_descriptor_header_t* ca_desc = NULL;

                    item_len = 0;

                    if(out_prog->pmt.ca_desc_cnt > 0){
                        ca_desc = pmt_info_get_ca_desc(&out_prog->pmt,0);
                        item_len = ca_desc->len + 2;
                    }

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);


                    if(ca_desc && item_len) {
                        memcpy(p_item_val_buf,ca_desc,item_len);
                    }
                }

                break;
                case 9: //  I_mpegOutputProgScte35Descr
                    //item_len = strlen("SCTE35");
                    item_len = 0;

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);
                    
                    //strcpy(p_item_val_buf,"SCTE35");

                break;
                case 10: //  I_mpegOutputProgScte18Descr
                    //item_len = strlen("SCTE18");
                    item_len = 0;

                    if( (buf_len + item_len + sizeof(uint16_t)) > ITEM_BUF_SIZE) {
                        rc = 2;
                        break;
                    }

                    *((uint16_t *)p_item_val_buf) = item_len;
                    p_item_val_buf = p_item_val_buf + sizeof(uint16_t);
                    buf_len = buf_len + sizeof(uint16_t);
                    
                    //strcpy(p_item_val_buf,"SCTE18");
                break;
                default:
                    rc = 1;
                    VIDEO_LOG("Invalid arg type %d\n", req->arg);
                 break;
            }

            if(rc) {
                break;
            }

            VIDEO_LOG("output ts index %d, prog id %d\n"
                ,req->session_id[i], out_prog->prog_num);

            resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
            resp->output_prog_index[resp->no_of_sessions] = out_prog->prog_num;
            buf_len = buf_len + item_len;
            p_item_val_buf = p_item_val_buf + item_len;

            resp->no_of_sessions++;

        }

        if(rc) {
            break;
        }

    }

    resp->item_value_len = buf_len;
    if(buf_len) resp->item_value = item_val_buf;
    else resp->item_value = NULL;

    if(p_prg_lst){
        free(p_prg_lst);
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_output_prog_stats_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_output_prog_stats_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_output_prog_stats_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,j,rc,no_of_sessions;
    uint32_t item_val = 0;
    uint16 *p_prg_lst = NULL;

    VIDEO_MSG_DEBUG("OutputProgStatsInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_output_prog_stats_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP output prog ststs info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0; (i < no_of_sessions) && (resp->no_of_sessions < 50); ++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }

        out_prog_t *out_prog = NULL;
        uint16 no_of_prog, cnt_prg;

        no_of_prog = que_get_size(NULL,&(out_ses->prog_list));

        if(no_of_prog > 0)
        {
            if(p_prg_lst) free(p_prg_lst);
        
            p_prg_lst = malloc( no_of_prog * sizeof(uint16) );
            if(!p_prg_lst) {
                VIDEO_LOG("OutputProgStatsInfo: unable to allocate memeory\n");
                break;
            }
        }
        else
        {
            continue;
        }
        
        cnt_prg = 0;
        FOR_ALL_ELEMENTS_IN_QUE(&out_ses->prog_list, out_prog) {
            if(cnt_prg >= no_of_prog) {
                break;
            }
            p_prg_lst[cnt_prg] = out_prog->prog_num;
            ++cnt_prg;
        }

        sort_program(p_prg_lst, no_of_prog);

        for(cnt_prg=0; (cnt_prg < no_of_prog) && (resp->no_of_sessions < 50); ++cnt_prg)
        {
            if (p_prg_lst[cnt_prg] < req->output_prog_index[i]) {
                continue;
            }

            out_prog = out_prog_lookup_by_prog_num(&out_ses->prog_list,p_prg_lst[cnt_prg]);
            
            if (!out_prog) {
                VIDEO_LOG("Program Info for Program index %d is not found\n",
                             req->output_prog_index[i]);
                continue;
            }

            if( (i == 0) && 
                (p_prg_lst[cnt_prg] == req->output_prog_index[i]) &&
                (req->output_es_index > out_prog->pmt.es_cnt) ) {
                continue;
            }

            if( (resp->no_of_sessions + out_prog->pmt.es_cnt) > 50) {
                VIDEO_LOG("Max no of elem in resp reached.\n");
                rc = 2;
                break;
            }

            for(j=0; j<out_prog->pmt.es_cnt; ++j) {

                pmt_es_info_t* es_info = pmt_info_get_es(&(out_prog->pmt), j);

                switch(req->arg) {
                    case 1: //  I_mpegOutputProgElemStatsPid
                        item_val = (uint32_t) pmt_get_es_pid(es_info);
                    break;
                    case 2: //  I_mpegOutputProgElemStatsElemType
                        item_val = (int32_t) es_info->es_type;
                    break;
                    case 3: //  I_mpegOutputProgElemStatsDataRate
                        item_val = (int32_t) -1;
                    break;
                    default:
                        rc = 1;
                        VIDEO_LOG("Invalid arg type %d\n", req->arg);
                     break;
                }

                if(rc) {
                    break;
                }

                VIDEO_LOG("output ts index %d, prog id %d, es id  %d\n"
                    ,req->output_prog_index[i],out_prog->prog_num, j+1);

                resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
                resp->output_prog_index[resp->no_of_sessions] = out_prog->prog_num;
                resp->output_es_index[resp->no_of_sessions] = j + 1;
                resp->item_value[resp->no_of_sessions] = item_val;

                resp->no_of_sessions++;
            }

            if(rc) {
                break;
            }

        }

        if(rc) {
            break;
        }

    }

    if(p_prg_lst){
        free(p_prg_lst);
    }
    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_output_udp_dest_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_output_udp_dest_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_output_udp_dest_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint32_t item_val = 0;

    VIDEO_MSG_DEBUG("OutputUdpDestInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_output_udp_dest_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
 //   resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP udp dest info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;

        switch(req->arg) {
            case 2: //  I_mpegOutputUdpDestinationIfIndex
                item_val = -1;  // sup will calculate
            break;
            case 3: //  I_mpegOutputUdpDestinationInetAddrType
                item_val = 1;   // IPv4
            break;
            case 4: //  I_mpegOutputUdpDestinationSrcInetAddr
                item_val = in_ses->cfg->src_ip[0];
            break;
            case 5: //  I_mpegOutputUdpDestinationDestInetAddr
                item_val = in_ses->cfg->dst_ip[0];
            break;
            case 6: //  I_mpegOutputUdpDestinationDestPort
                item_val = in_ses->cfg->dst_udp;
            break;
            case 7: //  I_mpegOutputUdpDestinationOutputTSIndex
                item_val = req->session_id[i];
            break;
            default:
                rc = 1;
            break;
        }

        if(rc) {
            VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        VIDEO_LOG("output ts index = %d, value = %d\n"
            ,req->session_id[resp->no_of_sessions],(int)item_val);

        resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

void process_video_snmp_query_video_sess_info_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_video_sess_info_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_video_sess_info_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    uint8_t *p_item_val_buf = item_val_buf;
    uint16_t buf_len = 0, item_len = 0;

    VIDEO_MSG_DEBUG("VideoSessInfo: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_video_sess_info_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
 //   resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP video sess info", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    rc = 0;

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }
        
        in_session_t *in_ses = out_ses->in_ses;

        switch(req->arg) {

            case 1: //  I_mpegVideoSessionPhyMappingIndex
                *((uint32_t *)p_item_val_buf) = req->session_id[i];
                item_len = sizeof(uint32_t);
            break;
            case 2: //  I_mpegVideoSessionPIDRemap
                *((int32_t *)p_item_val_buf) = 
                                  (out_ses->flags & SESSION_FLAG_PID_REMAP)?1:0;
                item_len = sizeof(int32_t);
            break;
            case 3: //  I_mpegVideoSessionMode
                switch(in_ses->type)
                {
                    case PROCESS_TYPE_REMUX:
                        *((int32_t *)p_item_val_buf) = 3;    // Multiplex
                    break;
                    case PROCESS_TYPE_PASSTHRU:
                        *((int32_t *)p_item_val_buf) = 2;    // Passthrough
                    break;
                    default :
                        *((int32_t *)p_item_val_buf) = 1;    // Other
                    break;
                }
                item_len = sizeof(int32_t);
            break;
            case 4: //  I_mpegVideoSessionState
                *((int32_t *)p_item_val_buf) = session_is_used(out_ses->state)?1:2;
                item_len = sizeof(int32_t);
            break;
            case 5: //  I_mpegVideoSessionProvMethod
                *((int32_t *)p_item_val_buf) = 0;
                item_len = sizeof(int32_t);
            break;
            case 6: //  I_mpegVideoSessionEncryptionType
                *((int32_t *)p_item_val_buf) = 0;
                item_len = sizeof(int32_t);
            break;
            case 7: //  I_mpegVideoSessionEncryptionInfo
                *((uint32_t *)p_item_val_buf) = 0;
                item_len = sizeof(uint32_t);
            break;
            case 8: //  I_mpegVideoSessionBitRate
                *((int32_t *)p_item_val_buf) = out_ses->avg_bitrate;
                item_len = sizeof(int32_t);
            break;
            case 9: //  I_mpegVideoSessionID

                item_len = SNMP_GQI_SESSION_ID_LEN + 1;

                if( (buf_len + item_len) > ITEM_BUF_SIZE) {
                    rc = 2;
                    break;
                }

                memset(p_item_val_buf,0,item_len);
            break;
            case 10: //  I_mpegVideoSessionSelectedInput
                *((uint32_t *)p_item_val_buf) = 0;
                item_len = sizeof(uint32_t);
            break;
            case 11: //  I_mpegVideoSessionSelectedOutput
                *((uint32_t *)p_item_val_buf) = 0;
                item_len = sizeof(uint32_t);
            break;
            default:
                rc = 1;
                VIDEO_LOG("Invalid arg type %d\n", req->arg);
            break;
        }

        if(rc) {
            break;
        }

        VIDEO_LOG("output ts index = %d\n"
            ,req->session_id[resp->no_of_sessions]);

        resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
        buf_len = buf_len + item_len;
        p_item_val_buf = p_item_val_buf + item_len;
        resp->no_of_sessions++;
    }

    resp->item_value_len = buf_len;
    if(buf_len) resp->item_value = item_val_buf;
    else resp->item_value = NULL;
    

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}


void process_video_snmp_query_video_sess_ptr_req(ipc_message *ipc_msg,
                                                          ipc_message *rsp_msg)
{
    video_query_snmp_video_sess_ptr_msg_t *req = IPC_DATA(ipc_msg);
    video_query_snmp_video_sess_ptr_resp_t *resp = IPC_DATA(rsp_msg);
    int i,rc,no_of_sessions;
    int32_t item_val = 0;

    VIDEO_MSG_DEBUG("VideoSessPtr: resource %d arg %d\n", req->resource_id, req->arg);
    memset(resp, 0, sizeof(video_query_snmp_video_sess_ptr_resp_t));

    no_of_sessions = req->no_of_sessions;

    resp->primary_id      = req->primary_id;
    resp->transaction_id  = req->transaction_id;
//    resp->resource_id     = req->resource_id;
    resp->owner_id        = req->owner_id;
    resp->no_of_sessions  = 0;

    if(req->arg != 8) { 
        VIDEO_LOG("Invalid arg type %d\n", req->arg);
        return;
    }

    video_context_t *ctx = get_config_context(req->primary_id,
                                             "SNMP video sess ptr", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        return;
    }

    for(i=0;i<no_of_sessions;++i) {

        uint16 out_ses_id = session_id_to_out_id(ctx, req->session_id[i]);
        if (out_ses_id == INVALID_SES_ID) {
            VIDEO_LOG("Session %d not found\n", req->session_id[i]);
            continue;
        }

        out_session_t *out_ses = get_out_session(out_ses_id);
        if (!session_is_used(out_ses->state)) {
            VIDEO_LOG("Out session %d not used\n", out_ses_id);
            continue;
        }

        in_session_t *in_ses = out_ses->in_ses;

        item_val = 2;   // assume session is not active

        if( session_is_active(in_ses->state) &&
            session_is_enabled(out_ses->state) &&
            !session_is_blocked(out_ses->state) ) {

            if(session_is_data(in_ses->cfg)) {
                item_val = 1;
            }
            else if(session_is_psi_ready(out_ses->state)) {
                item_val = 1;
            }
        }

        VIDEO_LOG("output ts index = %d, value = %d\n"
            ,req->session_id[resp->no_of_sessions],(int)item_val);

        resp->input_ts_index[resp->no_of_sessions] = req->input_ts_index[i];
        resp->output_ts_index[resp->no_of_sessions] = req->session_id[i];
        resp->input_prog_index[resp->no_of_sessions] = req->input_prog[i];
        resp->output_prog_index[resp->no_of_sessions] = req->output_prog[i];
        resp->item_value[resp->no_of_sessions] = item_val;
        resp->no_of_sessions++;
    }

    VIDEO_MSG_DEBUG("  No of sess in resp %d\n", resp->no_of_sessions);

    return;
}

// Handler for video query stat req message
// Fill in video in and out stats of a given session id
//
void process_video_query_stat_req (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    video_query_stat_msg_t *req = IPC_DATA(ipc_msg);
    video_query_stat_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_stat_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_STAT: resource %d", req->resource_id);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_STAT", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d", req->primary_id);
        resp->status = ENOENT;
        goto done;
    }

    uint16 out_ses_id = session_id_to_out_id(ctx, req->resource_id);
    if (out_ses_id == INVALID_SES_ID) {
        VIDEO_LOG("Session %d not found", req->resource_id);
        resp->status = ENOENT;
        goto done;
    }

    out_session_t *out_ses = get_out_session(out_ses_id);
    if (!session_is_used(out_ses->state)) {
        VIDEO_LOG("Out session %d not used", out_ses_id);
        resp->status = ENOENT;
        goto done;
    }

    in_session_t *in_ses = out_ses->in_ses;

    // Populate input video statistics
    video_in_stat_t* in_stat = &in_ses->stat;
    resp->in_stat.session_state = in_ses->state;
    resp->in_stat.ip_cnt = in_stat->ip_cnt;
    resp->in_stat.drop_ip_cnt = in_stat->drop_ip_cnt;
    resp->in_stat.tp_cnt = in_stat->tp_cnt;
    resp->in_stat.rtp_cnt = in_stat->rtp_cnt;
    resp->in_stat.null_tp_cnt = in_stat->null_tp_cnt;
    resp->in_stat.unref_tp_cnt = in_stat->unref_tp_cnt;
    resp->in_stat.psi_tp_cnt = in_stat->psi_tp_cnt;
    resp->in_stat.pcr_tp_cnt = in_stat->pcr_tp_cnt;
    resp->in_stat.non_pcr_tp_cnt = in_stat->non_pcr_tp_cnt;
    resp->in_stat.sync_loss_cnt = in_stat->sync_loss_cnt;
    resp->in_stat.disc_cnt = in_stat->disc_cnt;
    resp->in_stat.tb_disc_cnt = in_stat->tb_disc_cnt;
    resp->in_stat.cc_err_cnt = in_stat->cc_err_cnt;
    resp->in_stat.block_tp_cnt = in_stat->block_tp_cnt;
    resp->in_stat.new_pat_cnt = in_stat->new_pat_cnt;
    resp->in_stat.new_pmt_cnt = in_stat->new_pmt_cnt;
    resp->in_stat.pcr_jump_cnt = in_stat->pcr_jump_cnt;
    resp->in_stat.pcr_xover_cnt = in_stat->pcr_xover_cnt;
    resp->in_stat.overflow_cnt = in_stat->overflow_cnt;
    resp->in_stat.underflow_cnt = in_stat->underflow_cnt;
    resp->in_stat.idle_cnt = in_stat->idle_cnt;
    resp->in_stat.idle_time = in_stat->idle_time;
    resp->in_stat.measured_bitrate = in_ses->avg_bitrate;
    resp->in_stat.measured_min_bitrate = in_ses->min_bitrate;
    resp->in_stat.measured_max_bitrate = in_ses->max_bitrate;
    resp->in_stat.pcr_bitrate
            = tp_itvl_to_bitrate((uint32)in_ses->avg_tp_itvl);
    resp->in_stat.pcr_min_bitrate
            = tp_itvl_to_bitrate((uint32)in_ses->max_tp_itvl);
    resp->in_stat.pcr_max_bitrate
            = tp_itvl_to_bitrate((uint32)in_ses->min_tp_itvl);
    resp->in_stat.stay_time = in_ses->stay_time / SCALE_MS_BASE;
    if (in_ses->pcr_ctx_cnt > 0) {
        resp->in_stat.jitter = mpeg_time_to_ms(in_ses->jitter);
    }
    resp->in_stat.start_time = difftime(time(NULL), in_ses->stat.start_time);

    // Populate output video statistics
    video_out_stat_t* out_stat = &out_ses->stat;
    resp->out_stat.session_state = out_ses->state;
    resp->out_stat.tp_cnt = out_stat->tp_cnt;
    resp->out_stat.forward_tp_cnt = out_stat->forward_tp_cnt;
    resp->out_stat.insert_tp_cnt = out_stat->insert_tp_cnt;
    resp->out_stat.psi_tp_cnt = out_stat->psi_tp_cnt;
    resp->out_stat.pcr_tp_cnt = out_stat->pcr_tp_cnt;
    resp->out_stat.non_pcr_tp_cnt = out_stat->non_pcr_tp_cnt;
    resp->out_stat.new_pat_cnt = out_stat->new_pat_cnt;
    resp->out_stat.new_pmt_cnt = out_stat->new_pmt_cnt;
    resp->out_stat.drop_tp_cnt = out_stat->drop_tp_cnt;
    resp->out_stat.block_tp_cnt = out_stat->block_tp_cnt;
    resp->out_stat.overdue_tp_cnt = out_stat->overdue_tp_cnt;
    resp->out_stat.info_overrun_cnt = out_stat->info_overrun_cnt;
    resp->out_stat.info_err_cnt = out_stat->info_err_cnt;
    resp->out_stat.invalid_rate_cnt = out_stat->invalid_rate_cnt;
    resp->out_stat.overflow_cnt = out_stat->overflow_cnt;
    resp->out_stat.underflow_cnt = out_stat->underflow_cnt;
    resp->out_stat.measured_bitrate = out_ses->avg_bitrate;
    resp->out_stat.start_time = difftime(time(NULL), out_ses->stat.start_time);
    resp->out_stat.cmdbuf_maxhit_cnt = out_stat->cmdbuf_maxhit_cnt;

    resp->out_stat.ca_desc_present = 0;
    if( (in_ses->type == PROCESS_TYPE_REMAP) && (out_ses->cfg->encrypt_flag) ) {

        out_prog_t *out_prog = NULL;

        FOR_ALL_ELEMENTS_IN_QUE(&out_ses->prog_list, out_prog) {
            if(out_prog->pmt.ca_desc_cnt > 0) {
                resp->out_stat.ca_desc_present = 1;
                break;
            }
        }
    }

    resp->status = EOK;

done:
    IPC_HEADER(rsp_msg)->size = sizeof(video_query_stat_resp_t);
    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


// Handler for in session PAT query
//
void process_video_query_in_pat_req (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    video_query_pat_msg_t *req = IPC_DATA(ipc_msg);
    video_query_pat_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_pat_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_IN_PAT: resource %d\n", req->resource_id);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id  = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_STAT", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d\n", req->primary_id);
        resp->status = ENOENT;
        goto done;
    }

    uint16 out_ses_id = session_id_to_out_id(ctx, req->resource_id);
    if (out_ses_id == INVALID_SES_ID) {
        VIDEO_LOG("Session %d not found", req->resource_id);
        resp->status = ENOENT;
        goto done;
    }

    out_session_t *out_ses = get_out_session(out_ses_id);
    if (!session_is_used(out_ses->state)) {
        VIDEO_LOG("Out session %d not used\n", out_ses_id);
        resp->status = ENOENT;
        goto done;
    }


    int size = 0;
    video_psi_info_t* pat = (video_psi_info_t*)resp->section;
    in_session_t *in_ses = out_ses->in_ses;

    in_session_lock(in_ses->id);
    pat->flags = in_ses->pat_snooped;
    pat->section_cnt = in_ses->pat.sect_cnt;
    uint8* sect_ptr = (uint8*)pat->psi_section;

    resp->status = EOK;
    int i;
    for (i=0; i<pat->section_cnt; i++) {
        psi_section_t *sect = in_ses->pat.sect[i];
        int sect_len = psi_get_section_size(sect->sect_hdr);    // TODO
        size += sect_len;
        if (size > VIDEO_MSG_BUF_SIZE) {
            resp->status = E2BIG;
            break;
        }
        memcpy(sect_ptr, sect->sect_hdr, sect_len);
        resp->size += sect_len;
        sect_ptr += sect_len;
    }

    resp->total_section = pat->section_cnt;
    resp->num_section = i;
    resp->remaining_section = resp->total_section - i;
    in_session_unlock(in_ses->id);

done:
    IPC_HEADER(rsp_msg)->size = sizeof(video_query_pat_resp_t) + resp->size;
    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


// Handler for qam session PAT query
//
void process_video_query_qam_pat_req (ipc_message *ipc_msg,
                                      ipc_message *rsp_msg)
{
    video_query_pat_msg_t *req = IPC_DATA(ipc_msg);
    video_query_pat_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_pat_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_QAM_PAT: resource %d", req->resource_id);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_QAM_PAT", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context");
        resp->status = ENOENT;
        goto done;
    }

    uint16 qam_id = req->resource_id;
    if (qam_id >= NUM_QAMS) {
        VIDEO_LOG("Bad qam ID %d\n", qam_id);
        resp->status = ENOENT;
        goto done;
    }

    int size = 0;
    video_psi_info_t *pat = (video_psi_info_t*)resp->section;
    qam_info_t* qam = get_qam_info(qam_id);

    qam_channel_lock(qam_id);
    pat->flags = qam->pat_built;
    pat->section_cnt = qam->pat.sect_cnt;
    uint8* sect_ptr = (uint8*)pat->psi_section;

    resp->status = EOK;
    int i;
    for (i=0; i<qam->pat.sect_cnt; i++) {
        psi_section_t* sect = qam->pat.sect[i];
        int sect_len = psi_get_section_size(sect->sect_hdr);    // TODO

        size += sect_len;
        if (size > VIDEO_MSG_BUF_SIZE) {
            resp->status = E2BIG;
            break;
        }        

        memcpy(sect_ptr, sect->sect_hdr, sect_len);
        resp->size += sect_len;
        sect_ptr += sect_len;
    }

    resp->total_section = qam->pat.sect_cnt;
    resp->num_section = i;
    resp->remaining_section = resp->total_section - i;
    qam_channel_unlock(qam_id);

done:
    IPC_HEADER(rsp_msg)->size = sizeof(video_query_pat_resp_t) + resp->size;
    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


// Handler for in session PMT query
//
void process_video_query_in_pmt_req (ipc_message *ipc_msg, ipc_message *rsp_msg)
{
    video_query_pmt_msg_t *req = IPC_DATA(ipc_msg);
    video_query_pmt_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_pmt_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_IN_PMT: resource %d, program index %d",
                    req->resource_id, req->prog_idx);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    int tot_len = 0;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_STAT", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d", req->primary_id);
        resp->status = ENOENT;
        goto done;
    }

    uint16 out_ses_id = session_id_to_out_id(ctx, req->resource_id);
    if (out_ses_id == INVALID_SES_ID) {
        VIDEO_LOG("Session %d not found", req->resource_id);
        resp->status = ENOENT;
        goto done;
    }

    out_session_t *out_ses = get_out_session(out_ses_id);
    if (!session_is_used(out_ses->state)) {
        VIDEO_LOG("Out session %d not used", out_ses_id);
        resp->status = ENOENT;
        goto done;
    }

    in_session_t *in_ses = out_ses->in_ses;

    boolean buf_full = FALSE;
    uint32_t idx = 0;
    video_psi_info_t *pmt = (video_psi_info_t *)resp->section;
    in_prog_t *prog;
    resp->status = EOK;

    psi_lock(in_ses->id);
    FOR_ALL_ELEMENTS_IN_QUE(&in_ses->prog_list, prog) {
        if (prog->pmt_snooped)  resp->total_section++;
        if (idx++ < req->prog_idx)  continue;

        if (prog->pmt_snooped) {
            if (!buf_full) {
                pmt_header_t *pmt_hdr
                        = (pmt_header_t*)prog->pmt.sect->sect_hdr;
                int sect_len = pmt_get_section_size(pmt_hdr);

                int len = sizeof(video_psi_info_t) + sect_len;
                int pad_len = len % 4;
                if (pad_len)  len += 4 - pad_len;

                if (tot_len + len < VIDEO_MSG_BUF_SIZE) {
                    resp->num_section++;
                    pmt->flags = prog->pmt_snooped;
                    pmt->section_cnt = 1;
                    memcpy(pmt->psi_section, pmt_hdr, sect_len);
                    tot_len += len;
                    pmt = (video_psi_info_t*)(((uintptr_t)pmt) + len);

                } else {
                    buf_full = TRUE;
                    resp->remaining_section = 1;
                    resp->status = E2BIG;
                }

            } else {
                resp->remaining_section++;
            }
        }
    }
    psi_unlock(in_ses->id);

done:
    resp->size = tot_len;
    IPC_HEADER(rsp_msg)->size = sizeof(video_query_pmt_resp_t) + tot_len;
    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


// Handler for out session PMT query
//
void process_video_query_out_pmt_req (ipc_message *ipc_msg,
                                      ipc_message *rsp_msg)
{
    video_query_pmt_msg_t *req = IPC_DATA(ipc_msg);
    video_query_pmt_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_pmt_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_OUT_PMT: resource %d, program index %d",
                    req->resource_id, req->prog_idx);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    int tot_len = 0;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_STAT", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d", req->primary_id);
        resp->status = ENOENT;
        goto done;
    }

    uint16 out_ses_id = session_id_to_out_id(ctx, req->resource_id);
    if (out_ses_id == INVALID_SES_ID) {
        VIDEO_LOG("Session %d not found", req->resource_id);
        resp->status = ENOENT;
        goto done;
    }

    out_session_t *out_ses = get_out_session(out_ses_id);
    if (!session_is_used(out_ses->state)) {
        VIDEO_LOG("Out session %d not used", out_ses_id);
        resp->status = ENOENT;
        goto done;
    }

    boolean buf_full = FALSE;
    uint32_t idx = 0;
    video_psi_info_t *pmt = (video_psi_info_t *)resp->section;
    out_prog_t *prog;
    resp->status = EOK;

    out_session_lock(out_ses_id);
    FOR_ALL_ELEMENTS_IN_QUE(&out_ses->prog_list, prog) {
        if (prog->pmt_built)  resp->total_section++;
        if (prog->filtered)  continue;
        if (idx++ < req->prog_idx)  continue;

        if (prog->pmt_built && !prog->blocked) {
            if (!buf_full) {
                pmt_header_t* pmt_hdr = (pmt_header_t*)prog->pmt.sect->sect_hdr;
                int sect_len = pmt_get_section_size(pmt_hdr);

                int len = sizeof(video_psi_info_t) + sect_len;
                int pad_len = len % 4;
                if (pad_len)  len += 4 - pad_len;

                if (tot_len + len < VIDEO_MSG_BUF_SIZE) {
                    resp->num_section++;
                    pmt->flags = prog->pmt_built;
                    pmt->section_cnt = 1;
                    memcpy(pmt->psi_section, pmt_hdr, sect_len);
                    tot_len += len;
                    pmt = (video_psi_info_t*)(((uintptr_t)pmt) + len);

                } else {
                    buf_full = TRUE;
                    resp->remaining_section = 1;
                    resp->status = E2BIG;
                }

            } else {
                resp->remaining_section++;
            }
        }
    }
    out_session_unlock(out_ses_id);

done:
    resp->size = tot_len;
    IPC_HEADER(rsp_msg)->size = sizeof(video_query_pmt_resp_t) + tot_len;
    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


// Handler for PID Map query
//
void process_video_query_pid_map_req (ipc_message *ipc_msg,
                                      ipc_message *rsp_msg)
{
    video_query_pid_map_msg_t *req = IPC_DATA(ipc_msg);
    video_query_pid_map_resp_t *resp = IPC_DATA(rsp_msg);
    memset(resp, 0, sizeof(video_query_pid_map_resp_t));

    VIDEO_MSG_DEBUG("  QUERY_PID_MAP: resource %d", req->resource_id);

    // Populate response header
    IPC_HEADER(rsp_msg)->type = IPC_HEADER(ipc_msg)->type;
    resp->primary_id = req->primary_id;
    resp->owner_id = req->owner_id;
    resp->transaction_id = req->transaction_id;
    resp->resource_id = req->resource_id;

    int rc;
    video_context_t *ctx = get_config_context(req->primary_id,
                                              "QUERY_PID_MAP", &rc);
    if (!ctx) {
        VIDEO_LOG("Invalid video context %d", req->primary_id);
        resp->status = ENOENT;
        return;
    }

    uint16 out_ses_id = session_id_to_out_id(ctx, req->resource_id);
    if (out_ses_id == INVALID_SES_ID) {
        VIDEO_LOG("Session %d not found", req->resource_id);
        resp->status = ENOENT;
        return;
    }

    out_session_t *out_ses = get_out_session(out_ses_id);
    if (!session_is_used(out_ses->state)) {
        VIDEO_LOG("Out session %d not used", out_ses_id);
        resp->status = ENOENT;
        return;
    }

    int i;
    int cnt = 0;
    out_pid_info_t *tab = out_ses->pid_tab;

    for (i = PAT_PID + 1, tab++; i < NULL_PID; i++, tab++) {
        if (tab->reversed) {
            resp->pid_map.map[cnt].in_pid = i;
            resp->pid_map.map[cnt].out_pid = tab->pid;
            cnt++;
        }
    }
    resp->pid_map.num_pid = cnt;

    VIDEO_MSG_DEBUG("  Status %d\n", resp->status);
}


void process_video_get_sess_list (uint8_t  owner_id,
                                  uint32_t *sess_array,
                                  uint16_t *total_sessions)
{
    int i;
    uint16_t num_sessions = 0;

    for (i = 0; i < MAX_SESSIONS; i++) {
        out_session_t* ses = get_out_session(i);
        assert(ses);

        if (session_is_used(ses->state)) {
            assert(ses->cfg);

            if (ses->cfg->owner_id == owner_id) {
                in_session_t* in_ses = ses->in_ses;
                assert(in_ses);

                *sess_array++ = ses->cfg->client_id;
                num_sessions++;
            }
        }
    }
    *total_sessions = num_sessions;
    VIDEO_LOG("\nTOTAL NUMBER OF SESSIONS %d\n", *total_sessions);
}


// Video message handler
//   Returns TRUE if the message is handled.  Otherwise returns FALSE.
//   Note: caller is recsponsible for freeing ipc message buffer.
//   Note: currently the rfgw_msg_hdr_t is part of the video message header!
//         For future improvement we should remove this platform-dependent
//         header from the video message definition!
//
boolean video_msg_handler (ipc_message *ipc_msg,
                           ipc_message *rsp_msg, boolean *rsp_flag)
{
    *rsp_flag = FALSE;

    switch (IPC_HEADER(ipc_msg)->type) {

    case MSG_TYPE_VIDEO_UPDATE_PID_FILTER:
        process_video_update_pid_filter(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_UPDATE_PROG_FILTER:
        process_video_update_prog_filter(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_UPDATE_PID_REMAP:
        process_video_update_pid_remap(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_UPDATE_PROG_REMAP:
        process_video_update_prog_remap(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_CONFIG_QAM:
        process_video_config_qam(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_QUERY_STAT:
        process_video_query_stat_req(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_QUERY_IN_PAT:
        process_video_query_in_pat_req(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_QUERY_QAM_PAT:
        process_video_query_qam_pat_req(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_QUERY_IN_PMT:
        process_video_query_in_pmt_req(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_QUERY_OUT_PMT:
        process_video_query_out_pmt_req(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

    case MSG_TYPE_VIDEO_QUERY_SESS_LIST:
//      process_video_query_sess_list_req(ipc_msg, rsp_msg);
        break;

    case MSG_TYPE_VIDEO_QUERY_CAROUSEL_INSERT:
        process_video_query_crsl_insert(ipc_msg, rsp_msg);
        *rsp_flag = TRUE;
        break;

#if 0
    case MSG_TYPE_VIDEO_CHANGE_SOURCE:
        process_video_change_source(ipc_msg);
        break;

    case MSG_TYPE_VIDEO_MULTICAST_SESSION_RESTART:
        process_video_multicast_session_restart();
        break;
#endif

    default:
        return FALSE;
    }

    return TRUE;
}      

