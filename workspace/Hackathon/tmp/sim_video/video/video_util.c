/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "video.h"
#include "video_util.h"
#include "video_config_db.h"
#include "bench.h"
#include "bit_mask.h"
//#include "pme_public.h"

extern const char* lcred_mode_label[];
extern const char* lcred_role_label[];

#define NUM_VIDEO_STREAM_TYPES          3

#define CRSL_BUFF_NAME_LEN  64

#define CRSL_TP_POOLNAME    "crsl_tp_pool"
#define PSI_TP_POOLNAME     "psi_tp_pool"

uint8 video_stream_types[NUM_VIDEO_STREAM_TYPES] =
{
    STREAM_TYPE_VIDEO_MPEG1,
    STREAM_TYPE_VIDEO_MPEG2,
    128                                 // HBO uses this for video ?!
};

// forward declaration
uint8 in_session_set_type (in_config_t *cfg);

uint32 tp_itvl_to_bitrate (uint32 tp_itvl)
{
    if (tp_itvl != 0) {
        return (uint32)(((((int64)BASE_FREQ) * TP_SIZE_BIT)
                         << FRAC_BITS_TIME) / tp_itvl);
    }
    return 0;
}


uint32 tp_bitrate_to_itvl (uint32 bitrate)
{
    if (bitrate != 0) {
        return (uint32)(((((int64)BASE_FREQ) * TP_SIZE_BIT)
                         << FRAC_BITS_TIME) / bitrate);
    }
    return 0;
}


const char* get_in_session_state_label (uint8 state)
{
    if (!session_is_used(state)) {
        return "NOT USED";
    }
    if (!session_is_enabled(state)) {
        return "NOT ENABLED";
    }
    if (session_is_blocked(state)) {
        return "BLOCKED";
    } 
    if (session_is_no_resources(state)) {
        return "NO RESOURCE";
    }
    if (session_is_active(state)) {
        if (session_is_psi_ready(state)) {
            return "ACTIVE-PSI";
        } else {
            return "ACTIVE";
        }
    }
    if (session_is_idle(state)) {
        return "IDLE";
    }
    if (session_is_off(state)) {
        return "OFF";
    }
    if (session_is_init(state)) {
        return "INIT";
    }
    return "UNKNOWN";
}


const char* get_out_session_state_label (uint8 state)
{
    if (!session_is_used(state)) {
        return "NOT USED";
    }
    if (!session_is_enabled(state)) {
        return "NOT ENABLED";
    }
    if (session_is_blocked(state)) {
        return "BLOCKED";
    }
    if (session_is_no_resources(state)) {
        return "NO RESOURCE";
    }
    return "ENABLED";
}


const char* get_session_psi_state_label (uint8 state)
{
   if (session_is_psi_ready(state)) {
       return "PSI READY";
   }
   return "";
}


const char* get_process_type_label (out_config_t *cfg)
{
    if (cfg->pid_remap_flag) {
        if (cfg->mpts_flag)  return "REMUX";
        else  return "REMAP";
    } else {
        if (cfg->parse_psi_flag)  return "PASSTHRU";
        else  return "DATAPIPE";
    }
    return "";
}


// Initialize video context
//
int video_context_init (int id)
{
    int i, j;
    int rc;
    VIDEO_LOG("Video context %d init", id);

    void* platform_ctx = platform_video_context_init();
    if (platform_ctx == NULL) {
        return ENOMEM;
    }

    video_context_t* ctx = phy_mem_alloc(sizeof(video_context_t), NULL);
    if (ctx == NULL) {
        VIDEO_LOG("Failed to allocate video context");
        platform_video_context_cleanup(platform_ctx);
        return ENOMEM;
    }

    memset(ctx, 0, sizeof(video_context_t));
    ctx->id = id;
    ctx->platform = platform_ctx;

    // Set up in session hash table
    for (i=0; i<NUM_INPUT_PORTS; i++) {
        ctx->ses_hash_tab[i] = hash_table_create(SES_HASH_SLOTS, ses_hash_code,
                                               ses_hash_match, ses_hash_print);
        assert(ctx->ses_hash_tab[i]);
    }

    // Set up carousel insert pool
    rc = pool_create(&ctx->crsl_insert_pool, sizeof(crsl_insert_t),
                     NUM_CRSL_INSERTS, &ctx->crsl_insert);
    if (rc == ENOMEM) {
        return rc;
    }

    // QAM config setup
    for (i=0; i<NUM_QAMS; i++) {
        qam_config_t* cfg = get_qam_config(ctx, i);
        cfg->psi_period = PSI_PERIOD_DEFAULT;

        // Set up qam flow map
        for (j=0; j<MAX_VIDEO_FLOWS_PER_QAM; j++) {
            ctx->qam_flow[i][j] = INVALID_ID;
        }

        // Initialize qam insert list
        que_init(NULL, get_qam_crsl_insert_list(ctx, i));
    }

    char crsl_buff_name[CRSL_BUFF_NAME_LEN];

    memset(crsl_buff_name, 0, CRSL_BUFF_NAME_LEN);
    snprintf(crsl_buff_name, CRSL_BUFF_NAME_LEN, "%s_%d",
             CRSL_TP_POOLNAME, id);
    rc = tp_buf_pool_create_named(&ctx->crsl_tp_buf_pool, NUM_CRSL_TP_BUFS,
                                  crsl_buff_name);
    if (rc == ENOMEM) {
        return rc;
    }

    // Set up input session id pool (for new VSCM support)
    rc = pool_create(&ctx->in_ses_id_pool, sizeof(que_elem_t), MAX_SESSIONS,
                     NULL);

    // Note: this will enable the config thread to touch this context!!
    video_context[id] = ctx;

    return EOK;
}


// Free up a video context
//
int video_context_cleanup (int id)
{
    char crsl_buff_name[CRSL_BUFF_NAME_LEN];
    video_context_t* ctx = video_context[id];
    if (!ctx) {
        return EOK;
    }

    // This should not be the active context
    if (ctx == video_ctx) {
        return EBUSY;
    }

    platform_video_context_cleanup(ctx->platform);

    video_context[id] = NULL;
    pool_destroy(&ctx->crsl_insert_pool);

    memset(crsl_buff_name, 0, CRSL_BUFF_NAME_LEN);
    snprintf(crsl_buff_name, CRSL_BUFF_NAME_LEN, "%s_%d",
             CRSL_TP_POOLNAME, id);
    tp_buf_pool_destroy_named(&ctx->crsl_tp_buf_pool, crsl_buff_name);

    phy_mem_free(ctx);
    video_context[id] = NULL;
    VIDEO_LOG("Video context %d reset", id);
    return EOK;
}


// Check if an elementary stream is of video type
//
boolean is_video_es (pmt_es_info_t *es_info)
{
    int i;
    for (i=0; i<NUM_VIDEO_STREAM_TYPES; i++) {
        if (es_info->es_type == video_stream_types[i]) {
            return TRUE;
        }
    }
    return FALSE;
}


/// Allocate PCR context table
int pcr_context_alloc (in_session_t *ses)
{
    int i;
    que_elem_t *elem;
    pcr_context_t *pcr_ctx;
    ses->pcr_ctx_cnt = 0;

    if ((ses->flags & SESSION_FLAG_MPTS)) {
        ses->max_pcr_ctx = MAX_PCR_CONTEXT_MPTS;
        elem = pool_alloc(&pcr_context_mpts_pool);
    } else {
        ses->max_pcr_ctx = MAX_PCR_CONTEXT_SPTS;
        elem = pool_alloc(&pcr_context_spts_pool);
    }

    if (elem == NULL) {
        return ENOMEM;
    }

    ses->pcr_ctx_tab = (pcr_context_t*)(elem + 1);
    memset(ses->pcr_ctx_tab, 0, sizeof(pcr_context_t) * ses->max_pcr_ctx);

    pcr_ctx = ses->pcr_ctx_tab;
    for (i=0; i<ses->max_pcr_ctx; i++, pcr_ctx++) {
        pcr_ctx->pid = INVALID_PID;
    }

    return EOK;
}


/// Free PCR context table
void pcr_context_free (in_session_t *ses)
{
    if (ses->pcr_ctx_tab) {
        que_elem_t *elem = (que_elem_t*)ses->pcr_ctx_tab;
        pool_free((ses->flags & SESSION_FLAG_MPTS)?
                      &pcr_context_mpts_pool : &pcr_context_spts_pool,
                  --elem);
    }
} 


/// Look up a PCR context for a session's table
pcr_context_t* pcr_context_lookup (in_session_t *ses, uint16 pid)
{
    int i;
    pcr_context_t* pcr_ctx = ses->pcr_ctx_tab;

    for (i=0; i<ses->pcr_ctx_cnt; i++, pcr_ctx++) {
        if (pcr_ctx->pid == pid) {
            return pcr_ctx;
        }
    }
    return NULL;
}


/// Add new PCR context to a session's table
pcr_context_t* pcr_context_add (in_session_t *ses)
{
    pcr_context_t* pcr_ctx = pcr_context_lookup(ses, INVALID_PID);
    if (pcr_ctx == NULL) {
        if (ses->pcr_ctx_cnt >= ses->max_pcr_ctx) {
            return NULL;
        }

        pcr_ctx = &ses->pcr_ctx_tab[ses->pcr_ctx_cnt++];
    }

    memset(pcr_ctx, 0, sizeof(pcr_context_t));
    return pcr_ctx;
}


/// Reset all PCR contexts in a session
void pcr_context_reset (in_session_t *ses)
{
    ses->pcr_ctx_cnt = ses->locked_pcr_cnt = 0;
    ses->anchor_info_idx = ses->first_new_pcr
        = ses->last_new_pcr = INVALID_INFO_IDX;
}


/// Unlock all PCR contexts in a session
void pcr_context_unlock (in_session_t *ses)
{
    int i;
    pcr_context_t* pcr_ctx = ses->pcr_ctx_tab;
    for (i=0; i<ses->pcr_ctx_cnt; i++, pcr_ctx++) {
        pcr_ctx->flags = 0;
    }

    ses->locked_pcr_cnt = 0;
    ses->anchor_info_idx = ses->first_new_pcr
        = ses->last_new_pcr = INVALID_INFO_IDX;
}


/// Adjust TBO of PLL
void pcr_context_adjust_tbo (pcr_context_t* pcr_ctx, mpeg_timediff_t adj)
{
    pcr_ctx->tbo_est += adj;
    pcr_ctx->out_time -= adj;
    pcr_ctx->flags |= PCR_CONTEXT_FLAG_ADJUSTED;
}


/// Compute stay_time_limit (in base ticks) based on jitter size (in ms)
uint32 comp_stay_time_limit (uint16 jitter_size)
{
    return (SYS_LATENCY + jitter_size) * SCALE_MS_BASE;
}


/// Compute the operating stay time (in base ticks) based on jitter size (in ms)
uint32 comp_stay_time_oper (uint16 jitter_size)
{
    return (SYS_LATENCY + jitter_size / 2) * SCALE_MS_BASE;
}


// Allocate a PSI section based on its table_id
void psi_section_alloc (psi_snoop_t *snoop, uint8 table_id)
{
    if (table_id <= PMT_TABLE_ID) {
        // PSI section
        if (snoop->psi_buf) {
            snoop->sect = snoop->psi_buf;
            snoop->psi_buf = NULL;
        } else {
            snoop->sect = (psi_section_t*)pool_alloc(&psi_section_pool);
        }

    } else {
        // Private section
        if (snoop->priv_buf) {
            snoop->sect = snoop->priv_buf;
            snoop->priv_buf = NULL;
        } else {
            snoop->sect = (psi_section_t*)pool_alloc(&private_section_pool);
        }
    }
}


// Free a PSI section based on its table_id
void psi_section_free (psi_snoop_t *snoop)
{
    if (snoop->sect->sect_hdr->table_id <= PMT_TABLE_ID) {
        // PSI section
        //assert(!snoop->psi_buf);
        snoop->psi_buf = snoop->sect;
    } else {
        // Private section
        //assert(!snoop->priv_buf);
        snoop->priv_buf = snoop->sect;
    }
    snoop->sect = NULL;
}


// Clean up snoop context
void psi_snoop_cleanup (psi_snoop_t *snoop)
{
    if (snoop->sect) {
        psi_section_free(snoop);
    }
    if (snoop->psi_buf) {
        pool_free(&psi_section_pool, &snoop->psi_buf->link);
    }
    if (snoop->priv_buf) {
        pool_free(&private_section_pool, &snoop->priv_buf->link);
    }
}


// Find next available PID in a QAM, starting with the given PID
// and look forward upto the provided maximum allowed PID.
// If no pid is available, returns NULL_PID.
//
uint16 qam_find_next_avail_pid (uint16 pid, uint16 max_pid, uint16 qam_id)
{
    assert(max_pid < NULL_PID);
    qam_pid_info_t* qam_pid_tab = get_qam_info(qam_id)->pid_tab;
    for ( ; pid<=max_pid; pid++) {
        if (!qam_pid_tab[pid].used)  return pid;
    }
    return NULL_PID;
}


// Find previous available PID in a QAM, starting with the given PID
// and look backward upto the provided minimum allowed PID.
// If no pid is available, returns NULL_PID.
//
uint16 qam_find_prev_avail_pid (uint16 pid, uint16 min_pid, uint16 qam_id)
{
    qam_pid_info_t* qam_pid_tab = get_qam_info(qam_id)->pid_tab;
    for ( ; pid>=min_pid; pid--) {
        if (!qam_pid_tab[pid].used)  return pid;
    }
    return NULL_PID;
}


uint8 in_session_set_type (in_config_t *cfg)
{
    if (cfg->pid_remap_flag) {
        if (cfg->mpts_flag) {
            assert(cfg->parse_psi_flag && cfg->dejitter_flag);
            return PROCESS_TYPE_REMUX;
        }
        assert(cfg->parse_psi_flag && cfg->dejitter_flag);
        return PROCESS_TYPE_REMAP;
    }

    if (cfg->parse_psi_flag) {
        assert(cfg->dejitter_flag);
        return PROCESS_TYPE_PASSTHRU;
    }
    assert(!cfg->dejitter_flag);
    return PROCESS_TYPE_DATA;
}


// In session initialization
//   Note: in_session should be protected by the flag SESSION_STATE_USED!
//
int in_session_init (video_context_t *ctx, int16 rid)
{
    assert(ctx);
    assert(rid < MAX_SESSIONS);

    in_config_t* cfg = get_in_session_config(ctx, rid);
    in_session_t* ses = get_in_session(rid);

    memset(ses, 0, sizeof(in_session_t));

    ses->id = rid;
    ses->cfg = cfg;
    ses->stat.start_time = get_clock_time();
    ses->state = SESSION_STATE_INIT;
    ses->type = in_session_set_type(cfg);
    ses->prev_tp_arvl = sys_clk.base;

    if (cfg->pid_remap_flag) {
        ses->flags |= SESSION_FLAG_PID_REMAP;
    }
    if (cfg->parse_psi_flag) {
        ses->flags |= SESSION_FLAG_PROCESS_PSI;
    }
    if (cfg->dejitter_flag) {
        ses->flags |= SESSION_FLAG_PROCESS_TIME;
    }
    if (cfg->cbr_flag) {
        ses->flags |= SESSION_FLAG_CBR;
    }
    if (cfg->mpts_flag) {
        ses->flags |= SESSION_FLAG_MPTS;
    }

    if (cfg->enable_flag) {
        if (!cfg->input_type) {
            VIDEO_LOG("in_session_init: Session input type not known");
            return EINVAL;
        }
        ses->state |= SESSION_STATE_ENABLED;
    }

    ses->idle_thres = cfg->idle_thres * SCALE_MS_BASE;
    ses->init_thres = cfg->init_thres * SCALE_MS_BASE;
    ses->off_timer = cfg->off_timer * BASE_FREQ;
    ses->max_jitter = mpeg_time_set(cfg->jitter_size * SCALE_MS_BASE, 0);
    ses->stay_time_limit = comp_stay_time_limit(cfg->jitter_size);

    // Set initial TP interval based on allocated bitrate
    ses->tp_itvl = tp_bitrate_to_itvl(cfg->bitrate_alloc);
    ses->tp_itvl_lower = ses->tp_itvl * 19 / 20;    // relaxed by 5%

    ses->min_bitrate = BIG_INT;

    ses->periods_till_review = clkrec_par.review_period;

    // Set up lists
    que_init(NULL, &ses->prog_list);

    // Set up PCR context table
    int rc = pcr_context_alloc(ses);
    if (rc != EOK)  return rc;

    // Set up PAT snooping
    pat_info_init(&ses->pat);
    ses->pat_snooped = FALSE;
    ses->pat_ver = PSI_VER_UNKNOWN;
    ses->pat_usage_id = PSI_NOT_READY_USAGE_ID;

    // Set up in PID table
    memset(ses->pid_tab, 0, sizeof(ses->pid_tab));
    ses->pid_tab[PAT_PID].referenced = 1;
    ses->pid_tab[PAT_PID].has_psi = 1;

    ses->anchor_info_idx = ses->first_new_pcr = ses->last_new_pcr
        = INVALID_INFO_IDX;
    ses->mon_id = INVALID_ID;

    return EOK;
}


// In session update
//
int in_session_update (video_context_t *ctx, int16 rid)
{
    in_config_t* cfg;
    in_session_t* ses;

    assert(ctx);
    assert(rid < MAX_SESSIONS);

    cfg = get_in_session_config(ctx, rid);
    ses = get_in_session(rid);

    session_set_init(&ses->state);
    ses->prev_tp_arvl = sys_clk.base;

    ses->flags &= ~SESSION_FLAG_CBR;
    if (cfg->cbr_flag) {
        ses->flags |= SESSION_FLAG_CBR;
    }

    ses->idle_thres = cfg->idle_thres * SCALE_MS_BASE;
    ses->init_thres = cfg->init_thres * SCALE_MS_BASE;
    ses->off_timer = cfg->off_timer * BASE_FREQ;
    ses->stay_time_limit = comp_stay_time_limit(cfg->jitter_size);

    return EOK;
}


// Forget previously snooped input PAT
void in_session_forget_pat (in_session_t *ses)
{
    clear_pat(&ses->pat);
    pat_info_init(&ses->pat);
    ses->pat_ver = PSI_VER_UNKNOWN;
    ses->pat_snooped = FALSE;
    ses->state &= ~SESSION_STATE_PSI_READY;
}


// Clean up an in session
//
void in_session_cleanup (in_session_t *ses)
{
    ses->state = 0;

    // Free PSI sections
    clear_pat(&ses->pat);
    psi_snoop_cleanup(&ses->snoop);

    // Free PCR context table
    pcr_context_free(ses);

    in_session_cleanup_progs(ses);

    // Reset TP info buffer
    tp_info_buffer_reset(&ses->tp_info_buf);
}


// Clean up programs for in session
//
void in_session_cleanup_progs (in_session_t *ses)
{
    in_prog_t *prog;
    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        que_elem_t* elem = prog->link.prev;
        que_deque(NULL, &prog->link);

        in_pid_table_clear_pmt(ses->pid_tab, prog->pmt_pid, &prog->pmt);
        in_prog_free(prog);

        prog = (in_prog_t*)elem;
    }
}


// Input program info allocation
//
in_prog_t* in_prog_alloc (void)
{
    in_prog_t* prog = (in_prog_t*)pool_alloc(&in_prog_pool);
    if (prog) {
        memset(prog, 0, sizeof(in_prog_t));
        prog->pmt_ver = PSI_VER_UNKNOWN;
        prog->pmt_usage_id = PSI_NOT_READY_USAGE_ID;
        que_init(NULL, &prog->priv_sect_list);
        que_init(NULL, &prog->priv_crsl.tp_list);
        prog->priv_crsl.used_flag = 1;
    }
    return prog;
}


// Free an input program info
//
void in_prog_free (in_prog_t *prog)
{
    if (prog->pmt.sect) {
        pool_free(&psi_section_pool, &prog->pmt.sect->link);
    }
    psi_snoop_cleanup(&prog->snoop);
    pool_free_list(&private_section_pool, &prog->priv_sect_list);
    pool_free_list(&psi_tp_buf_pool, &prog->priv_crsl.tp_list);
    pool_free(&in_prog_pool, &prog->link);
}


// Find in_prog_t in the list with specified program number
// Returns NULL if not found
//
in_prog_t* in_prog_lookup_by_prog_num (que_elem_t *prog_list, int prog_num)
{
    in_prog_t *prog;

    FOR_ALL_ELEMENTS_IN_QUE(prog_list, prog) {
        if (prog->prog_num == prog_num) {
            return prog;
        }
    }
    return NULL;
}


// Find in_prog_t in the list with specified PMT PID
// Returns NULL if not found
//
in_prog_t* in_prog_lookup_by_pmt_pid (que_elem_t *prog_list, uint16 pmt_pid)
{
    in_prog_t *prog;

    FOR_ALL_ELEMENTS_IN_QUE(prog_list, prog) {
        if (prog->pmt_pid == pmt_pid) {
            return prog;
        }
    }
    return NULL;
}


// Out session initialization
//   Note: out_session should be protected by SESSION_STATE_USED flag!
//
int out_session_init (video_context_t *ctx, int16 rid, int16 in_rid)
{
    assert(ctx);
    assert(rid != INVALID_ID);

    out_config_t* cfg = get_out_session_config(ctx, rid);
    in_config_t* in_cfg = get_in_session_config(ctx, in_rid);
    out_session_t* ses = get_out_session(rid);
    in_session_t* in_ses = get_in_session(in_rid);

    memset(ses, 0, sizeof(out_session_t));

    ses->id = rid;
    ses->cfg = cfg;
    ses->stat.start_time = get_clock_time();
    ses->anchor_info_idx = INVALID_INFO_IDX;
    ses->out_time = sys_clk.base;

    // Set up lists
    que_init(NULL, &ses->prog_list);

    // Copy info from configuration
    ses->qam_id = cfg->qam_id;
    ses->flow_id = cfg->flow_id;
    ses->in_ses = in_ses;

    if (!in_cfg->used_flag) {
        // Error handling if the in session is not used
        // Note: should we use session_is_used(in_ses->state) instead?!
        return EOK;
    }

    // Copy info from in session
    ses->flags = in_ses->flags & SESSION_FLAGS_SHARED;
    if (in_cfg->ms_cr_flag)  ses->flags |= SESSION_FLAG_MASTER_SLAVE_CR;
    ses->stay_time_limit = comp_stay_time_limit(in_cfg->jitter_size);
    ses->min_stay_time = BIG_INT;
    ses->max_stay_time = -BIG_INT;
    ses->pat_usage_id = PSI_NOT_READY_USAGE_ID;

    if (!cfg->enable_flag) {
        return EOK;
    }

    // Update out session count in corresponding in session
    in_ses->out_ses_cnt++;
    
    // The following is for out sessions that are enabled
    uint16 in_id = in_ses->id;
    in_session_lock(in_id);
    sync_readwrite();
    tp_info_ref_init(&ses->tp_info_ref, &in_ses->tp_info_buf,
                     in_ses->ref_tp_info_idx);
    ses->tp_info_usage_id = in_ses->tp_info_usage_id;
    in_session_unlock(in_id);
    
    ses->state |= SESSION_STATE_ENABLED;

    return EOK;
}


// Clean up out session
//
void out_session_cleanup (out_session_t *ses)
{
    ses->state = 0;
    if (ses->in_ses) {
        ses->in_ses->out_ses_cnt--;
    }
    ses->in_ses = NULL;
    ses->tp_info_ref.buf = NULL;    // YTT: should we do this??
    out_session_cleanup_progs(ses);
}


// Clean up programs and usage maps in out session
//
void out_session_cleanup_progs (out_session_t *ses)
{
    que_elem_t *elem, *elem2;

    // Free all out programs
    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, elem) {
        out_prog_t* prog = (out_prog_t*)elem;
        elem2 = elem->prev;
        que_deque(NULL, elem);

        // prog->pmt_usage_id = PSI_NOT_READY_USAGE_ID;
        out_prog_cleanup(prog, ses);
        out_prog_free((out_prog_t*)elem);

        elem = elem2;
    }
}


// Delete out session
//
void out_session_delete (video_context_t *ctx, int16 ses_id)
{
    // Unconfigure the out session
    if (video_ctx) {
        out_session_t* ses = get_out_session(ses_id);

        // Mark the out session not used so output thread will not process it
        ses->state &= ~SESSION_STATE_USED;

        out_session_cleanup(ses);

        // Flag to rebuild qam's PAT
        qam_info_t* qam = get_qam_info(ses->qam_id);
        qam->pat_rebuild = 1;
        qam->recheck_conflict = 1;
    }

    // Mark the session free
    get_out_session_config(ctx, ses_id)->used_flag = 0;
}


// Output program info allocation
//
out_prog_t* out_prog_alloc (void)
{
    out_prog_t* prog = (out_prog_t*)pool_alloc(&out_prog_pool);
    if (prog) {
        memset(prog, 0, sizeof(out_prog_t));
        prog->cfg_idx = INVALID_CFG_IDX;
        prog->pmt_ver = PSI_VER_UNKNOWN;
        prog->pmt_usage_id = PSI_NOT_READY_USAGE_ID;
        ca_desc_set_sys_id (&prog->ca_desc_data, 0);
        prog->ca_desc_loc = 0;
        que_init(NULL, &prog->pmt_crsl.tp_list);
    }
    return prog;
}


// Set up out program
void out_prog_setup (out_prog_t *prog, in_prog_t *in_prog, out_session_t *ses)
{
    uint32 period = get_qam_info(ses->qam_id)->pat_crsl.period;

    prog->in_prog = in_prog;
    prog->in_prog_num = in_prog->prog_num;
    prog->in_pmt_pid = in_prog->pmt_pid;
    prog->out_ses_id = ses->id;

    if ((ses->flags & SESSION_FLAG_PID_REMAP)) {
        // For remapped session 
        // Set up PMT carousel
        prog->pmt_crsl.period = period;
        prog->pmt_crsl.next_time = sys_clk.base;
        prog->pmt_crsl.used_flag = 1;

        // Set up private section carousel
        prog->priv_insert.crsl = &in_prog->priv_crsl;
        prog->priv_insert.period = period;
        prog->priv_insert.target_cnt = CRSL_INSERT_CONTINUOUS;
        prog->priv_insert.next_time = sys_clk.base;

    } else {
        // For passthru session 
        prog->prog_num = prog->in_prog_num;
        prog->pmt_pid = prog->in_pmt_pid;
        prog->pmt_ver = in_prog->pmt_ver;
    }
}


// Forget previously built PMT
void out_prog_forget_pmt (out_prog_t *prog, qam_info_t *qam)
{
    if (prog->pmt_built) {
        pmt_ver_store(qam, prog->prog_num, prog->pmt_ver);

        if (prog->pmt.sect) {
            pool_free(&psi_section_pool, &prog->pmt.sect->link);
        }
        pool_free_list(&psi_tp_buf_pool, &prog->pmt_crsl.tp_list);
        prog->pmt_built = FALSE;
    }
}


// Output program cleanup
void out_prog_cleanup (out_prog_t *prog, out_session_t *ses)
{
    int chg_cnt=0;
    if (prog->pmt_built && qam_is_encrypt(ses->qam_id)) {
        pmt_change_t pmt_chg;
        memset(&pmt_chg, 0, sizeof(pmt_change_t));

        // Record ES delete actions
        video_ca_delete_pmt_es_pids(&pmt_chg, &prog->pmt);
        chg_cnt = video_ca_report_pmt_change(ses->qam_id, &pmt_chg);
    }   

    qam_info_t* qam = get_qam_info(ses->qam_id);
    out_prog_unregister(prog, ses, qam);
    out_prog_forget_pmt(prog, qam);
    out_session_clear_ca_pids(ses, prog);

    // send update after unregister prog
   if (chg_cnt) {
       video_ca_send_update_service(ses->qam_id, prog->prog_num);
   }
}



// Free an output program info
//
void out_prog_free (out_prog_t *prog)
{
    pool_free(&out_prog_pool, &prog->link);
}


// Find out_prog_t in the list with specified program number
// Returns NULL if not found
//
out_prog_t* out_prog_lookup_by_prog_num (que_elem_t *prog_list, int prog_num)
{
    out_prog_t *prog;

    FOR_ALL_ELEMENTS_IN_QUE(prog_list, prog) {
        if (prog->prog_num == prog_num) {
            return prog;
        }
    }
    return NULL;
}


// Find out_prog_t in the list with specified input program number
// Returns NULL if not found
//
out_prog_t* out_prog_lookup_by_in_prog_num (que_elem_t *prog_list,
                                            int in_prog_num)
{
    out_prog_t *prog;
    FOR_ALL_ELEMENTS_IN_QUE(prog_list, prog) {
        if (prog->in_prog_num == in_prog_num) {
            return prog;
        }
    }
    return NULL;
}


// Find out_prog_t in the list that owns the specified ES/CA PID
// Returns NULL if not found
//
out_prog_t* out_prog_lookup_by_pid (que_elem_t *prog_list, uint16 pid)
{
    int i;
    out_prog_t *prog;
    pmt_info_t *pmt;

    FOR_ALL_ELEMENTS_IN_QUE(prog_list, prog) {
        pmt = &prog->pmt;
        for (i=0; i<pmt->es_cnt; i++) {
            if (pmt_get_es_pid(pmt_info_get_es(pmt, i)) == pid) {
                return prog;
            }
        }
        for (i=0; i<pmt->ca_desc_cnt; i++) {
            if (ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i)) == pid) {
                return prog;
            }
        }
    }
    return NULL;
}


// Look up a PSI section from a list by table id
//
psi_section_t* psi_sect_lookup_by_table_id (que_elem_t *sect_list, int table_id)
{
    psi_section_t *s;

    FOR_ALL_ELEMENTS_IN_QUE(sect_list, s) {
        if (s->sect_hdr->table_id == table_id) {
            return s;
        }
    }

    return NULL;
}


// Clear PAT snoop status
//
void clear_pat (pat_info_t *pat)
{
    // Free all PAT sections
    int i;
    for (i=0; i<pat->sect_cnt; i++) {
        if (pat->sect[i]) {
            pool_free(&psi_section_pool, &pat->sect[i]->link);
        }
    }
    pat_info_init(pat);
}


// Check if a program number is already used in a qam configuration
// Return EEXIST if it already exists, and EOK if not.
//
int qam_config_program_check (video_context_t *ctx, uint16 qam_id, int prog_num)
{
    int i;
    int32* flow_map = get_qam_flow_map(ctx, qam_id);
    out_config_t* cfg;

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }
        cfg = get_out_session_config(ctx, get_out_id(flow_map[i]));
        if (video_list_search(prog_num, cfg->out_prog_list, cfg->prog_cnt)
                != -ENOENT) {
            return EEXIST;
        }
    }
    return EOK;
}


void pmt_ver_store (qam_info_t *qam, uint16 prog_num, uint8 ver)
{
    pmt_ver_t *pmt_ver = &qam->pmt_ver_history[qam->pmt_ver_idx];
    pmt_ver->prog_num = prog_num;
    pmt_ver->ver = ver;
    if (++qam->pmt_ver_idx >= MAX_VIDEO_FLOWS_PER_QAM) {
        qam->pmt_ver_idx = 0;
    }
}


uint16 pmt_ver_init (qam_info_t *qam, uint16 prog_num)
{
    int i;
    pmt_ver_t *hist = qam->pmt_ver_history;

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (hist[i].prog_num == prog_num) {
            hist[i].prog_num = 0;            // clear the entry
            return (hist[i].ver + 1) & 0x1F;
        }
    }
    return 0;
}


// Initialize a carousel
void crsl_init (video_context_t *ctx, int16 rid)
{
    crsl_t* crsl = get_crsl(ctx, rid);
    memset(crsl, 0, sizeof(crsl_t));
    que_init(NULL, &crsl->tp_list);
}


void crsl_insert_delete (video_context_t *ctx, crsl_t *crsl, uint16 qam_id)
{
    if (!bit_mask_test(crsl->qam_mask, qam_id)) {
        VIDEO_DEBUG("Error: crsl is not inserted to qam %d\n", qam_id);
        return;
    }

    crsl_insert_t* insert
            = find_crsl_insert(get_qam_crsl_insert_list(ctx, qam_id), crsl);
    assert(insert);

    qam_config_t *qam_cfg = get_qam_config(ctx, qam_id);
    boolean ctx_active = (ctx == video_ctx);
    if (ctx_active)  qam_channel_lock(qam_id);
    que_deque(NULL, &insert->link);
    qam_cfg->crsl_cnt--;
    if (ctx_active)  qam_channel_unlock(qam_id);

    video_db_qam_cnfg_checkin(ctx->id, qam_id, qam_cfg);

    // TBD: clear up the insert
    pool_free(&ctx->crsl_insert_pool, &insert->link);

    bit_mask_clear(crsl->qam_mask, qam_id);
    crsl->num_qam--;
}


#if 0
// Delete a carousel
void crsl_delete (video_context_t *ctx, int16 rid)
{
    int i;
    crsl_insert_t *insert;
    crsl_t* crsl = get_crsl(ctx, rid);
    boolean ctx_active = (ctx == video_ctx);

    // Mark the resource free
    // This stops the output thread from processing the carousel
    crsl->used_flag = 0;

    // remove the corresponding insertions for this carousel
    for (i=0; i<NUM_QAMS; i++) {
        if (bit_mask_test(crsl->qam_mask, i)) {
            insert = find_crsl_insert(get_qam_crsl_insert_list(ctx, i), crsl);
            assert(insert);
            if (ctx_active)  qam_channel_lock(i);
            que_deque(NULL, &insert->link);
            if (ctx_active)  qam_channel_unlock(i);

            // TBD: clean up the insert
            pool_free(&ctx->crsl_insert_pool, &insert->link);
        }
    }

    // Free TP buffers
    pool_free_list(&ctx->crsl_tp_buf_pool, &crsl->tp_list);
}
#endif


uint64 crsl_insert_bitrate (crsl_insert_t *insert)
{
    if (insert->period == 0)  return 0L;

    int num_tp;
    if (insert->ecm_flag) {
        ecm_info_t* ecm_info = get_ecm_info((uintptr_t)insert->crsl);
        if (!ecm_info)  return 0L;
        num_tp = ecm_info->num_tp;
    } else {   
        num_tp = ((crsl_t*)insert->crsl)->num_tp;
    }   
    return ((uint64)num_tp) * TP_SIZE_BIT * BASE_FREQ / insert->period;
}



// Find crsl insert in a list that points to the specified crsl
//
crsl_insert_t* find_crsl_insert (que_elem_t *crsl_insert_list, crsl_t *crsl)
{
    crsl_insert_t *insert;
    FOR_ALL_ELEMENTS_IN_QUE(crsl_insert_list, insert) {
        if (insert->crsl == crsl) {
            return insert;
        }
    }
    return NULL;
}


void tp_info_buffer_print (FILE *fp, tp_info_buffer_t *buf)
{
    int i;
    int chunk_cnt = buf->size >> TP_INFO_CHUNK_SHIFT;
    fprintf(fp, "TP info buffer: size %d\n", buf->size);
    fprintf(fp, "  Chunks:");
    for (i=0; i<chunk_cnt; i++) {
        fprintf(fp, " %x", (uint32)buf->chunk[i]);
    }
    fprintf(fp, "\n");
}


// Update video IP statistics
//
void video_ip_stat_update (video_ip_stat_t *acc_stat, video_ip_stat_t *my_stat)
{
    acc_stat->ipv4_cnt += my_stat->ipv4_cnt;
    acc_stat->ipv6_cnt += my_stat->ipv6_cnt;
    acc_stat->ucast_cnt += my_stat->ucast_cnt;
    acc_stat->mcast_cnt += my_stat->mcast_cnt;
    acc_stat->err_cnt += my_stat->err_cnt;
    acc_stat->unref_cnt += my_stat->unref_cnt;
    acc_stat->overrun_cnt += my_stat->overrun_cnt;
}


// Update video input statistics
void video_in_stat_update (video_in_stat_t *acc_stat, video_in_stat_t *my_stat)
{
    acc_stat->ip_cnt += my_stat->ip_cnt;
    acc_stat->drop_ip_cnt += my_stat->drop_ip_cnt;

    acc_stat->tp_cnt += my_stat->tp_cnt;
    acc_stat->rtp_cnt += my_stat->rtp_cnt;
    acc_stat->null_tp_cnt += my_stat->null_tp_cnt;
    acc_stat->unref_tp_cnt += my_stat->unref_tp_cnt;
    acc_stat->psi_tp_cnt += my_stat->psi_tp_cnt;
    acc_stat->pcr_tp_cnt += my_stat->pcr_tp_cnt;
    acc_stat->non_pcr_tp_cnt += my_stat->non_pcr_tp_cnt;

    acc_stat->sync_loss_cnt += my_stat->sync_loss_cnt;
    acc_stat->disc_cnt += my_stat->disc_cnt;
    acc_stat->tb_disc_cnt += my_stat->tb_disc_cnt;
    acc_stat->cc_err_cnt += my_stat->cc_err_cnt;
    acc_stat->block_tp_cnt += my_stat->block_tp_cnt;

    acc_stat->new_pat_cnt += my_stat->new_pat_cnt;
    acc_stat->new_pmt_cnt += my_stat->new_pmt_cnt;

    acc_stat->pcr_jump_cnt += my_stat->pcr_jump_cnt;
    acc_stat->pcr_xover_cnt += my_stat->pcr_xover_cnt;
    acc_stat->overflow_cnt += my_stat->overflow_cnt;
    acc_stat->underflow_cnt += my_stat->underflow_cnt;
}


// Update video output statistics
void video_out_stat_update (video_out_stat_t *acc_stat,
                            video_out_stat_t *my_stat)
{
    acc_stat->tp_cnt += my_stat->tp_cnt;
    acc_stat->forward_tp_cnt += my_stat->forward_tp_cnt;
    acc_stat->insert_tp_cnt += my_stat->insert_tp_cnt;

    acc_stat->psi_tp_cnt += my_stat->psi_tp_cnt;
    acc_stat->pcr_tp_cnt += my_stat->pcr_tp_cnt;
    acc_stat->non_pcr_tp_cnt += my_stat->non_pcr_tp_cnt;

    acc_stat->new_pat_cnt += my_stat->new_pat_cnt;
    acc_stat->new_pmt_cnt += my_stat->new_pmt_cnt;

    acc_stat->drop_tp_cnt += my_stat->drop_tp_cnt;
    acc_stat->block_tp_cnt += my_stat->block_tp_cnt;
    acc_stat->overdue_tp_cnt += my_stat->overdue_tp_cnt;

    acc_stat->info_overrun_cnt += my_stat->info_overrun_cnt;
    acc_stat->info_err_cnt += my_stat->info_err_cnt;
    acc_stat->invalid_rate_cnt += my_stat->invalid_rate_cnt;
    acc_stat->overflow_cnt += my_stat->overflow_cnt;
    acc_stat->underflow_cnt += my_stat->underflow_cnt;
}


// Update video QAM statistics
void video_qam_stat_update (video_qam_stat_t *acc_stat,
                            video_qam_stat_t *my_stat)
{
    acc_stat->mux_tp_cnt += my_stat->mux_tp_cnt;
    acc_stat->drop_tp_cnt += my_stat->drop_tp_cnt;
    acc_stat->psi_tp_cnt += my_stat->psi_tp_cnt;
    acc_stat->ecm_tp_cnt += my_stat->ecm_tp_cnt;
    acc_stat->crsl_tp_cnt += my_stat->crsl_tp_cnt;
    acc_stat->bitrate += my_stat->bitrate;
}


// Collect overall input statistics
//
void video_in_stat_collect (video_in_stat_t *my_stat)
{
    int i;

    memcpy(my_stat, &video_in_stat_history, sizeof(video_in_stat_t));

    for (i=0; i<MAX_SESSIONS; i++) {
        in_session_t* ses = get_in_session(i);
        if (session_is_used(ses->state)) {
            video_in_stat_update(my_stat, &ses->stat);
        }
    }
}


// Collect overall output statistics
//
void video_out_stat_collect (video_out_stat_t *my_stat)
{
    int i;

    memcpy(my_stat, &video_out_stat_history, sizeof(video_out_stat_t));

    for (i=0; i<MAX_SESSIONS; i++) {
        out_session_t* ses = get_out_session(i);
        if (session_is_used(ses->state)) {
            video_out_stat_update(my_stat, &ses->stat);
        }
    }
}


// Collect output statistics for a specified QAM channel
//
void video_qam_stat_collect (video_qam_stat_t *my_stat)
{
    memset(my_stat, 0, sizeof(*my_stat));

    int i;
    for (i=0; i<NUM_QAMS; i++) {
        video_qam_stat_update(my_stat, &get_qam_info(i)->stat);
    }
}


// Continuity counter check setup
//
void video_cc_check_init (uint8* cc_record)
{
    int i;
    for (i=0; i<NUM_PIDS; i++) {
        cc_record[i] = CC_DISC;
    }
}


// Check and update continuity counter
//   Returns TRUE if continuity counter check pass.
//   Otherwise returns FALSE.
//
boolean video_cc_check (tp_header_t hdr, uint8* cc_record, uint8* exp_cc)
{
    uint16 pid = tp_get_pid(&hdr);
    uint8 prev_cc = cc_record[pid];
    *exp_cc = (prev_cc + (hdr.af_ctrl & 1)) & 0xf;
    cc_record[pid] = hdr.cc;
    return ((hdr.cc == *exp_cc) || (prev_cc == CC_DISC));
}


// Initialize video state data structures
//
int video_state_init (void)
{
    int rc;

#if __QNX__
    in_session = mmap(0, sizeof(in_session_t) * MAX_SESSIONS,
                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_PHYS | MAP_ANON,
                      NOFD, 0);
#else
    in_session = malloc(MAX_SESSIONS * sizeof(in_session_t));
#endif
    assert(in_session);

#if __QNX__
    out_session = mmap(0, sizeof(out_session_t) * MAX_SESSIONS,
                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_PHYS | MAP_ANON,
                      NOFD, 0);
#else
    out_session = malloc(MAX_SESSIONS * sizeof(out_session_t));
#endif
    assert(out_session);

    qam_info = calloc(NUM_QAMS, sizeof(qam_info_t));
    assert(qam_info);

    ip_flow_buf = calloc(MAX_SESSIONS * IP_FLOW_BUF_SIZE,
                         sizeof(ip_pkt_info_t));

    // Create TP info buffer chunk pool
    rc = pool_create(&tp_info_buf_chunk_pool, sizeof(que_elem_t) +
                     sizeof(tp_info_t) * TP_INFO_CHUNK_SIZE,
                     NUM_TP_INFO_BUF_CHUNKS, NULL);

    // Create PCR context pool for SPTS sessions
    rc = pool_create(&pcr_context_spts_pool,
             sizeof(que_elem_t) + sizeof(pcr_context_t) * MAX_PCR_CONTEXT_SPTS,
             MAX_SPTS_SESSIONS, NULL);
    assert(rc == EOK);

    // Create PCR context pool for MPTS sessions
    rc = pool_create(&pcr_context_mpts_pool,
             sizeof(que_elem_t) + sizeof(pcr_context_t) * MAX_PCR_CONTEXT_MPTS,
             MAX_MPTS_SESSIONS, NULL);
    assert(rc == EOK);

    // Create in program info pool
    rc = pool_create(&in_prog_pool, sizeof(in_prog_t), NUM_IN_PROGS, NULL);
    assert(rc == EOK);

    // Create out program info pool
    rc = pool_create(&out_prog_pool, sizeof(out_prog_t), NUM_OUT_PROGS, NULL);
    assert(rc == EOK);

    // Create PSI section pool
    rc = pool_create(&psi_section_pool,
                     sizeof(psi_section_t) + MAX_PSI_SECT_SIZE,
                     NUM_PSI_SECTIONS, NULL);
    assert(rc == EOK);

    // Create private section pool
    rc = pool_create(&private_section_pool,
                     sizeof(psi_section_t) + MAX_PRIVATE_SECT_SIZE,
                     NUM_PRIVATE_SECTIONS, NULL);
    assert(rc == EOK);

    rc = tp_buf_pool_create_named(&psi_tp_buf_pool,
                                  NUM_PSI_TP_BUFS,
                                  PSI_TP_POOLNAME);
    if (rc == ENOMEM) {
        return rc;
    }

    // Initialize clock recovery parameters
    clkrec_config();

    // Initialize stats
    memset(&video_in_stat_history, 0, sizeof(video_in_stat_t));
    memset(&video_out_stat_history, 0, sizeof(video_out_stat_t));

    // Initialize PSI record
    memset(&psi_record, 0, sizeof(psi_record));

    return EOK;
}


void video_prepare_go_hot (int primary_id)
{
    int rc;

    if (lcred_primary_id != INVALID_ID) {
        // Video is already hot
        em_vidman_LCRED_UNEXPECTED_GO_HOT(lcred_mode_label[lcred_mode], 
                                          lcred_role_label[lcred_role]);
        return;
    }

    if (!video_context_inited(primary_id)) {
        rc = video_context_init(primary_id);
        if (rc != EOK) {
            em_vidman_VIDEO_CONTEXT_INIT_FAILED("PROVISION", primary_id);
            return;
        }
    }

    video_context_t* ctx = get_video_context(primary_id);

    // Initialize all QAM info's
    int i;
    for (i=0; i<NUM_QAMS; i++) {
        qam_init(ctx, i);
    }

    // Initialize all in sessions
    uint16 rid;
    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        if (get_in_session_config(ctx, rid)->used_flag) {
            rc = in_session_init(ctx, rid);
            if (rc == EOK) {
                // Mark the in session used so the input thread will process it
                get_in_session(rid)->state |= SESSION_STATE_USED;
            } else {
                em_vidman_VIDEO_IN_SESSION_INIT_FAILED("go_hot", rid, strerror(rc));
            }
        }
    }

    // Initialize all out sessions
    for (rid = 0; rid < MAX_SESSIONS; rid++) {
        out_config_t* out_cfg = get_out_session_config(ctx, rid);
        if (out_cfg->used_flag) {
            rc = out_session_init(ctx, rid, out_cfg->in_ses_id);
            if (rc != EOK) {
                em_vidman_VIDEO_OUT_SESSION_INIT_FAILED("go_hot", rid, strerror(rc));
            }

            // Mark out session used so output thread will process it
            get_out_session(rid)->state |= SESSION_STATE_USED;
        }
    }
}


void video_go_hot (int primary_id) 
{
    lcred_primary_id = primary_id;

    // This enables the video context to be accesses by the processing threads
    video_ctx = get_video_context(lcred_primary_id);
   
    VIDEO_LOG("Going hot for %d", primary_id);
}


// Note: in most cases video_go_active() will immediately follows
//       video_go_hot(), except in the case where LC boots to PRIMARY STANDBY
//       (video_go_hot called), and later moves to PRIMARY ACTIVE after
//       receiving the go-hot message (video_go_active then called).
//
void video_go_active (int primary_id) 
{
    VIDEO_LOG("Going active for %d", primary_id);
    if (lcred_primary_id != primary_id) {
        VIDEO_LOG("Bad primary id %d in go-hot message", primary_id);
    }
    
    // This will send IPC messages to veman
    video_ca_report_add_ts();
}


// QAM info initialization
//   Note: qam_info should be protected by video_ctx
//
void qam_init (video_context_t *ctx, uint16 qam_id)
{
    qam_config_t *cfg;
    qam_info_t *qam;

    assert(qam_id < NUM_QAMS);

    cfg = get_qam_config(ctx, qam_id);
    qam = get_qam_info(qam_id);

    qam->id = qam_id;
    qam->cfg = cfg;

    que_init(NULL, &qam->pat_crsl.tp_list);

    // Set up PAT carousel
    qam->pat_crsl.period = cfg->psi_period * SCALE_MS_BASE;
    qam->pat_crsl.next_time = sys_clk.base;

    qam_build_pat(qam);

    // This will enable qam PAT to be inserted by the output thread
    //qam->pat_crsl.used_flag = 1;

    memset(qam->prog_tab, 0, sizeof(qam->prog_tab));
    memset(qam->pid_tab, 0, sizeof(qam->pid_tab));
}


// Check and capture IP packet if appropriate
//   Return 1 if a new packet is captured.
//   Return 0 otherwise.
//
int check_capture (ip_header_t *ip_hdr, int mode)
{
    if (!capture_flag && mode == capture_mode) {
        ipv6_header_t *ipv6_hdr = (ipv6_header_t*)(ip_hdr);
        int len, offset;
    
        // capture L2 also
        offset = get_l2_header_size();
        memcpy(capture_buf, (((uint8*)ip_hdr) - offset), offset);

        // capture L3
        if (ip_hdr->ver == IPV4_VERSION) {
            len = ip_hdr->len;
        } else {
            len = ipv6_hdr->payload_length + sizeof(ipv6_header_t);
        }

        if (len > IP_PKT_BUF_SIZE) {
            len = IP_PKT_BUF_SIZE;
        }
        memcpy(capture_buf+offset, ip_hdr, len);
        capture_flag = TRUE;
        VIDEO_DEBUG("IP packet captured\n");
        return 1;
    }
    return 0;
}


// Dump TP header
void dump_tp_header (FILE *fp, tp_header_t *tp_hdr)
{
    fprintf(fp, "SYN %02x, TEI %d, PUSI %d, PR %d, PID %04x, SC %d, "
                "AFC %d, CC %d\n",
            tp_hdr->sync, tp_hdr->transport_err, tp_hdr->payload_unit_start,
            tp_hdr->priority, tp_get_pid(tp_hdr), tp_hdr->scrambling_ctrl,
            tp_hdr->af_ctrl, tp_hdr->cc);
}


#if 0
// Dump PSI section
void psi_section_print (FILE *fp, psi_section_header_t *sect)
{
    fprintf(fp, "Table id %d, %slen %d", sect->table_id,
            (sect->priv? "private, " : ""), psi_get_section_size(sect));
    if (sect->sect_syntax) {
        fprintf(fp, ", %s ver %d, sect %d/%d\n",
                (sect->cur_next? "cur" : "next"), sect->ver,
                sect->sect_num, sect->last_sect_num);
    }
    fprintf(fp, "\nSection:\n");
    fprint_hex(fp, sect, psi_get_section_size(sect));
}
#endif


// Dump PAT table
void dump_pat (FILE *fp, pat_info_t *pat)
{
    int i;
    fprintf(fp, "PAT: tsid %d, ver %d, sections %d, progs %d\n",
            pat->tsid, pat->ver, pat->sect_cnt, pat->prog_cnt);
    for (i=0; i<pat->prog_cnt; i++) {
        fprintf(fp, "  Prog %d: PMT %d\n", pat_get_prog_num(pat->prog_info[i]),
                pat_get_pmt_pid(pat->prog_info[i]));
    }
}


static
void dump_info_loop (FILE *fp, uint8 *ptr, int info_len)
{
    while (info_len > 0) {
        descriptor_header_t* desc = (descriptor_header_t*)ptr;
        switch (desc->tag) {

            case DESC_TAG_CA:
            {
                ca_descriptor_header_t *desc2 = (ca_descriptor_header_t*)desc;
                fprintf(fp, ", (ca: sys-id %d, pid %d)",
                        (desc2->sys_id_hi << 8) | desc2->sys_id_lo,
                        (desc2->pid_hi << 8) | desc2->pid_lo);
                break;
            }

            case DESC_TAG_LANG:
            {
                int len = desc->len;
                uint8* ptr2 = (uint8*)(desc + 1);
                fprintf(fp, ", (lang:");

                while (len > 0) {
                    fprintf(fp, " %c%c%c", ptr2[0], ptr2[1], ptr2[2]);
                    len -= 4;
                    ptr2 += 4;
                }
                fprintf(fp, ")");
                break;
            }

            default:
                fprintf(fp, ", (%d, %d)", desc->tag, desc->len);
        }
        ptr += sizeof(descriptor_header_t) + desc->len;
        info_len -= sizeof(descriptor_header_t) + desc->len;
    }
}


// Dump PMT table
void dump_pmt (FILE *fp, pmt_info_t *pmt)
{
    int i;
    pmt_header_t* pmt_hdr = (pmt_header_t*)pmt->sect->sect_hdr;
    int info_len = pmt_get_prog_info_len(pmt_hdr);
    uint8 *ptr = ((uint8*)pmt_hdr) + PMT_SECT_HDR_SIZE;

    fprintf(fp, "PMT: prog %d, ver %d, PCR %d, info_len %d",
            pmt_get_prog_num(pmt_hdr), pmt_hdr->ver, pmt_get_pcr_pid(pmt_hdr),
            info_len);
    dump_info_loop(fp, ptr, info_len);
    ptr += info_len;

    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        info_len = pmt_get_es_info_len(es_info);
        ptr = (uint8*)(es_info + 1);
        fprintf(fp, "\n  PID %d: type %d, info_len %d",
                pmt_get_es_pid(es_info), es_info->es_type, info_len);
        dump_info_loop(fp, ptr, info_len);
        ptr += info_len;
    }
    fprintf(fp, "\n");
}


void print_tp_info (FILE *fp, tp_info_t *tp_info)
{
    fprintf(fp, "Phy addr %08x, TP idx %d, PID %d",
            (uint32)tp_info->addr, tp_info->tp_idx, tp_info->pid);

    if (tp_info->pcr_flag) {
        fprintf(fp, ", PCR");
    }
    if (tp_info->disc_flag) {
        fprintf(fp, ", Disc");
    }
    if (tp_info->pat_flag) {
        fprintf(fp, ", PAT");
    }
    if (tp_info->pmt_flag) {
        fprintf(fp, ", PMT");
    }
    fprintf(fp, "\n");
}


#if 0
void print_pcr_info (FILE *fp, pcr_info_t *pcr_info)
{

    if (pcr_info->info_type == SLAVE_PCR_INFO) {
        fprintf(fp, "Slave PCR %08x:%d\n",
                pcr_info->tbo_base, pcr_info->tbo_ext);
    }
    fprintf(fp, "PCR: TP itvl %d, DT %05x, TBO %08x:%d\n",
            pcr_info->tp_itvl, pcr_info->out_time,
            pcr_info->tbo_base, pcr_info->tbo_ext);
}
#endif


void print_out_prog (FILE *fp, out_prog_t *prog)
{
    if (prog->pmt_built) {
        dump_pmt(fp, &prog->pmt);
    } else {
        fprintf(fp, "PMT not built\n");
    }
}


void print_crsl (FILE *fp, crsl_t *crsl)
{
    fprintf(fp, "resource %d, owner %d, %d TPs, %d qams\n",
            crsl->resource_id, crsl->owner_id, crsl->num_tp, crsl->num_qam);
}


void print_crsl_pkt (FILE *fp, crsl_t *crsl)
{
    int i = 0;
    que_elem_t *elem;
    FOR_ALL_ELEMENTS_IN_QUE(&crsl->tp_list, elem) {
        fprintf(fp, "\nTP %d:\n", i++);
        fprint_hex(fp, elem + 1, TP_SIZE);
    }
}


void print_crsl_insert (FILE *fp, crsl_insert_t *insert)
{
    int remain_cnt = insert->target_cnt == CRSL_INSERT_CONTINUOUS ?
                         -1 : (int32)(insert->target_cnt - insert->cnt);
    fprintf(fp, "remain %d, period %d, cc %d\n",
            remain_cnt, insert->period, insert->next_cc);
}


void print_video_ip_stat (FILE *fp, video_ip_stat_t *my_stat)
{
    fprintf(fp, "  ipv4 %u, ipv6 %u, ucast %u, mcast %u\n",
            my_stat->ipv4_cnt, my_stat->ipv6_cnt, my_stat->ucast_cnt,
            my_stat->mcast_cnt);
    fprintf(fp, "  err %u, unref %u, overrun %d\n",
            my_stat->err_cnt, my_stat->unref_cnt, my_stat->overrun_cnt); 
}


void print_video_in_stat (FILE *fp, video_in_stat_t *my_stat)
{
    fprintf(fp, "  IP: in %u, RTP %d, drop %u\n",
            my_stat->ip_cnt, my_stat->rtp_cnt, my_stat->drop_ip_cnt);
    fprintf(fp, "  TP: in %u, pcr %u, psi %u, null %u, unref %u\n",
            my_stat->tp_cnt, my_stat->pcr_tp_cnt, my_stat->psi_tp_cnt,
            my_stat->null_tp_cnt, my_stat->unref_tp_cnt);
    fprintf(fp, "  new-PAT %u, new-PMT %u, disc %u, tb-disc %d\n",
            my_stat->new_pat_cnt, my_stat->new_pmt_cnt, my_stat->disc_cnt,
            my_stat->tb_disc_cnt);
    fprintf(fp, "  sync-loss %u, cc-err %u, pcr-jump %u, xover %u, "\
                "underflow %u, overflow %u, block %u\n",
            my_stat->sync_loss_cnt, my_stat->cc_err_cnt, my_stat->pcr_jump_cnt,
            my_stat->pcr_xover_cnt, my_stat->underflow_cnt,
            my_stat->overflow_cnt, my_stat->block_tp_cnt);
}


void print_video_out_stat (FILE *fp, video_out_stat_t *my_stat)
{
    fprintf(fp, "  TP: in %u, pcr %u, psi %u\n",
            my_stat->tp_cnt, my_stat->pcr_tp_cnt, my_stat->psi_tp_cnt);
    fprintf(fp, "      drop %u, forward %u, insert %u\n",
            my_stat->drop_tp_cnt, my_stat->forward_tp_cnt,
            my_stat->insert_tp_cnt);
    fprintf(fp, "  new-PAT %u, new-PMT %u\n",
            my_stat->new_pat_cnt, my_stat->new_pmt_cnt);
    fprintf(fp, "  info-overrun %u, info-err %u, block %u, overdue %u\n",
            my_stat->info_overrun_cnt, my_stat->info_err_cnt,
            my_stat->block_tp_cnt, my_stat->overdue_tp_cnt);
    fprintf(fp, "  invalid-rate %u, underflow %u, overflow %u\n",
            my_stat->invalid_rate_cnt, my_stat->underflow_cnt,
            my_stat->overflow_cnt);
}


void show_in_session_cast_type (FILE *fp, in_config_t *cfg)
{
    char src_ip_string[INET6_ADDRSTRLEN];
    char dst_ip_string[INET6_ADDRSTRLEN];

    if (cfg==NULL) {
        return;
    }

    switch (cfg->input_type) {

    case INPUT_TYPE_UNICAST:
    {
        if (!inet_ntop(cfg->ip_ver==IPV4_VERSION ? AF_INET : AF_INET6,
                       &cfg->dst_ip, dst_ip_string, INET6_ADDRSTRLEN)) {
            fprintf(fp, "\nBad dest IP\n");
            break;
        }
        fprintf(fp, "ip %s, udp %d", dst_ip_string, cfg->dst_udp);
        break;
    }

    case INPUT_TYPE_MULTICAST_SSM:
    {
        if (!inet_ntop(cfg->ip_ver==IPV4_VERSION ? AF_INET : AF_INET6,
                       &cfg->src_ip, src_ip_string, INET6_ADDRSTRLEN)) {
            fprintf(fp, "\nBad source IP\n");
            break;
        }
        if (!inet_ntop(cfg->ip_ver==IPV4_VERSION ? AF_INET : AF_INET6,
                       &cfg->dst_ip, dst_ip_string, INET6_ADDRSTRLEN)) {
            fprintf(fp, "\nBad group IP\n");
            break;
        }
        fprintf(fp, "ssm %s->%s", src_ip_string, dst_ip_string);
        break;
    }

    case INPUT_TYPE_MULTICAST_ASM:
    {
        if (!inet_ntop(cfg->ip_ver==IPV4_VERSION ? AF_INET : AF_INET6,
                       &cfg->dst_ip, dst_ip_string, INET6_ADDRSTRLEN)) {
            fprintf(fp, "\nBad group IP\n");
            break;
        }
        fprintf(fp, "asm %s", dst_ip_string);
        break;
    }

    default:
        fprintf(fp, "Bad cast type: %d\n", cfg->input_type);
    }
}


void show_in_session_config (FILE *fp, in_config_t *cfg)
{
    assert(cfg);
    show_in_session_cast_type(fp, cfg);

    fprintf(fp, ", ref_cnt %d\n", cfg->ref_cnt);
    fprintf(fp, "    bitrate %d, jitter %d/%d, timeout %d/%d/%d\n",
            cfg->bitrate_alloc, cfg->delay_target, cfg->jitter_size,
            cfg->init_thres, cfg->idle_thres, cfg->off_timer);
    fprintf(fp, "    CR-mode %s-%s",
            cfg->ms_cr_flag ? "master-slave" : "unified",
            cfg->cbr_flag? "cbr" : "vbr");
    if (cfg->ms_cr_flag && cfg->master_pcr_pid) {
        fprintf(fp, ", preferred-master-PCR-pid %d", cfg->master_pcr_pid);
    }
    fprintf(fp, "\n");
}


void show_in_session_out_list (FILE *fp, in_session_t *ses)
{
    int qam_id, flow_id, map;
    fprintf(fp, "Out list:\n");
    for (qam_id=0; qam_id < NUM_QAMS; qam_id++) {
        for (flow_id=0; flow_id < MAX_VIDEO_FLOWS_PER_QAM; flow_id++) {
            if ((map = video_ctx->qam_flow[qam_id][flow_id]) != -1) {
                if (get_in_id(map) == ses->id) {
                    fprintf(fp, "  Out %d, qam %d, flow %d\n",
                            get_out_id(map), qam_id, flow_id);
                }
            }
        }
    }
    fprintf(fp, "Total %d out sessions\n", ses->out_ses_cnt);
}


void show_ip_flow (FILE *fp, uint16 rid)
{
    ses_hash_item_t *item = &video_ctx->ses_hash_item[rid];
    fprintf(fp, "  IP flow: rd %d, wr %d\n", item->flow_rd, item->flow_wr);
}


void show_in_session_stat (FILE *fp, in_session_t *ses)
{
    print_video_in_stat(fp, &ses->stat);
}


void show_in_session_pat (FILE *fp, in_session_t *ses)
{
    if (ses->pat_snooped) {
        dump_pat(fp, &ses->pat);
        fprintf(fp, "\n");
    } else {
        fprintf(fp, "  PAT not snooped\n");
    }
}


void show_in_session_pmt (FILE *fp, in_session_t *ses)
{
    in_prog_t* prog;
    psi_section_t *sect;
    int n;

    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        // PMT
        if (prog->pmt_snooped) {
            dump_pmt(fp, &prog->pmt);
            fprintf(fp, "\n");
        } else {
            fprintf(fp, "  PMT not snooped\n");
        }

        // Private sections
        n = que_get_size(NULL, &prog->priv_sect_list);
        if (n > 0) {
            fprintf(fp, "Private table:\n");
            FOR_ALL_ELEMENTS_IN_QUE(&prog->priv_sect_list, sect) {
                fprintf(fp, "  Table %d: size %d\n", sect->sect_hdr->table_id,
                        psi_get_section_size(sect->sect_hdr));
            }
            fprintf(fp, "\n");
        }
    }
}


void show_in_session_time (FILE *fp, in_session_t *ses)
{
    fprintf(fp, "  Life: ");
    print_elapsed_time(fp,
                       get_elapsed_sec(ses->stat.start_time, get_clock_time()));
    fprintf(fp, "  Bitrate: measured %d, PCR %d\n", 
            ses->avg_bitrate, tp_itvl_to_bitrate(ses->avg_tp_itvl));
    fprintf(fp, "  Jitter: %d ms\n", mpeg_time_to_ms(ses->jitter));
}


void clear_in_session_time (in_session_t *ses)
{
    ses->min_bitrate = BIG_INT;
    ses->max_bitrate = 0;
    ses->min_tp_itvl = BIG_INT;
    ses->max_tp_itvl = 0;
}


void show_in_session_prog_list (FILE *fp, in_session_t *ses)
{
    in_prog_t* prog;

    fprintf(fp, "Program list:\n");
    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        if (prog->pmt_snooped) {
            fprintf(fp, "  Prog %d: PMT %d, ver %d\n",
                    prog->prog_num, prog->pmt_pid, prog->pmt_ver);
//        } else {
//            fprintf(fp, "  Prog %d: PMT not snooped\n", prog->prog_num);
        }
    }
}


void show_in_session_pcr_context_table (FILE *fp, in_session_t *ses)
{
    int i;
    mpeg_clock_t pcr;
    mpeg_clock_t tbo;

    fprintf(fp, "PCR context: locked %d\n", ses->locked_pcr_cnt);
    for (i=0; i<ses->pcr_ctx_cnt; i++) {
        pcr_context_t* pcr_ctx = &ses->pcr_ctx_tab[i];
        mpeg_time_to_clock(pcr_ctx->pcr_time, &pcr);
        mpeg_time_to_clock(pcr_ctx->tbo_est, &tbo);
        fprintf(fp, "  Pid %d: flags %02x, pcr %d:%d, tbo %d:%d, clk %d ppb\n",
                pcr_ctx->pid, pcr_ctx->flags, pcr.base, pcr.ext, tbo.base,
                tbo.ext, (int32)(pcr_ctx->freq_adj/(SCALE_FREQ_ADJ/1000)));
    }
}


void show_out_session_process_type (FILE *fp, out_config_t *cfg)
{
    const char* proc_lbl = get_process_type_label(cfg);
    fprintf(fp, "%s", proc_lbl);
    if (!strcmp(proc_lbl, "REMAP")) {
        fprintf(fp, ": prog %d, pid %d-%d", cfg->out_prog_list[0],
                cfg->pid_range[0].first, cfg->pid_range[0].last);
    } else if (strcmp(proc_lbl, "DATAPIPE")) {
        fprintf(fp, ": %d progs, %d pids", cfg->prog_cnt, cfg->pid_cnt);
    }
}


void show_out_session_config (FILE *fp, out_config_t *cfg)
{
    fprintf(fp, "In %d, qam %d, flow %d",
            cfg->in_ses_id, cfg->qam_id, cfg->flow_id);
    if (!cfg->enable_flag)  fprintf(fp, ", DISABLE");

    fprintf(fp, ", ");
    show_out_session_process_type(fp, cfg);

    if (cfg->encrypt_flag)  fprintf(fp, ", ENCRYPT");
    fprintf(fp, "\n");
}


void show_out_session_pid_list (FILE *fp, out_config_t *cfg)
{
    if (cfg->pid_cnt == 0)  return;

    fprintf(fp, "  Pid list:");
    if (cfg->pid_remap_flag) {
        int i;
        for (i=0; i<cfg->pid_cnt; i++) {
            fprintf(fp, " %d->%d", cfg->in_pid_list[i], cfg->out_pid_list[i]);
        }
        fprintf(fp, "\n");
    } else {
        video_list_print(fp, cfg->in_pid_list, cfg->pid_cnt);
    }
}


void show_out_session_program_list (FILE *fp, out_config_t *cfg)
{
    if (cfg->prog_cnt == 0)  return;

    fprintf(fp, "  Program list:");
    if (cfg->pid_remap_flag) {
        int i;
        for (i=0; i<cfg->prog_cnt; i++) {
            fprintf(fp, "\n    Prog %d->%d: pid %d-%d",
                    cfg->in_prog_list[i], cfg->out_prog_list[i],
                    cfg->pid_range[i].first, cfg->pid_range[i].last);
        }
        fprintf(fp, "\n");
    } else {
        video_list_print(fp, cfg->in_prog_list, cfg->prog_cnt);
    }
}


void show_out_session_stat (FILE *fp, out_session_t *ses)
{
    print_video_out_stat(fp, &ses->stat);
}


void show_out_session_psi (FILE *fp, out_session_t *ses)
{
    out_prog_t *prog;

    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        // PMT
        if (prog->pmt_built) {
            dump_pmt(fp, &prog->pmt);
            fprintf(fp, "\n");
//        } else {
//            fprintf(fp, "  PMT not built\n");
        }
    }
}


void show_out_session_time (FILE *fp, out_session_t *ses)
{
    fprintf(fp, "  Life: ");
    print_elapsed_time(fp,
                       get_elapsed_sec(ses->stat.start_time, get_clock_time()));
    fprintf(fp, "  Bitrate: %d\n", ses->avg_bitrate);
}


void show_out_session_prog_list (FILE *fp, out_session_t *ses)
{
    out_prog_t* prog;

    fprintf(fp, "Program list:\n");
    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        if (prog->pmt_built) {
            fprintf(fp, "  Prog %d->%d: PMT %d, ver %d, id %d",
                    prog->in_prog_num, prog->prog_num, prog->pmt_pid,
                    prog->pmt_ver, get_out_prog_id(prog));
            if (ses->cfg->encrypt_flag || prog->pmt.ca_desc_cnt) {
                fprintf(fp, ", ca_desc_cnt %d", prog->pmt.ca_desc_cnt);
            }
        } else {
            fprintf(fp, "  Prog %d: PMT not built", prog->in_prog_num);
        }

        if (prog->filtered) {
            fprintf(fp, ", filtered");
        }
        if (prog->blocked) {
            fprintf(fp, ", blocked");
        }
        fprintf(fp, "\n");
    }
}


void show_video_pid_range (FILE *fp, video_pid_range_t *pid_range,
                           int list_size)
{
    int i;
    for (i=0; i<list_size; i++) {
        if (pid_range[i].first != pid_range[i].last) {
            fprintf(fp, "pid %d - %d", pid_range[i].first, pid_range[i].last);
        }
        fprintf(fp, "\n");
    }
}


void show_qam_config (FILE *fp, qam_config_t *cfg)
{
    fprintf(fp, "owner %d, TSID %d, ONID %d, encrypt %d, %d flows, %d crsl, "\
                "PSI period %d\n",
            cfg->owner_id, cfg->tsid, cfg->onid, cfg->encrypt_flag,
            cfg->flow_cnt, cfg->crsl_cnt, cfg->psi_period);
}


void show_qam_stat (FILE *fp, qam_info_t *qam)
{
    fprintf(fp, "    ready %d, %d progs, bitrate %lld bps\n",
            qam_channel_ready(qam->id), qam->prog_cnt, qam->stat.bitrate);
}


void show_qam_flows (FILE *fp, qam_info_t *qam)
{
    int i, j;
    uint32 total_rate = 0;
    int flow_cnt = 0;
    int prog_cnt = 0;
    int es_cnt = 0;

    // List all flow info
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }

        out_session_t* ses = get_out_session(get_out_id(flow_map[i]));
        in_session_t* in_ses = ses->in_ses;

        fprintf(fp, "Flow %d: ses %d, %d (%s) -> %d (%s), %d progs",
                i, ses->cfg->client_id, in_ses->id,
                get_in_session_state_label(in_ses->state), ses->id,
                get_out_session_state_label(ses->state), in_ses->pat.prog_cnt);

        if (session_is_active(in_ses->state)
                && session_is_enabled(in_ses->state & ses->state)
                && !session_is_blocked(in_ses->state | ses->state)) {
            total_rate += in_ses->avg_bitrate;
            fprintf(fp, ", %d bps", in_ses->avg_bitrate);
        }
        fprintf(fp, "\n");

        int prog_es_cnt = 0;
        out_prog_t* prog;
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
            if (prog->filtered || prog->blocked)  continue;

            fprintf(fp, "  Prog %d, PMT %d", prog->prog_num, prog->pmt_pid);
            if (prog->pmt_built) {
                pmt_info_t *pmt = &prog->pmt;
                fprintf(fp, ", ES:");
                for (j=0; j<pmt->es_cnt; j++) {
                    uint16 pid = pmt_get_es_pid(pmt_info_get_es(pmt, j));
                    fprintf(fp, " %d", pid);
                    if (pid == pmt->pcr_pid) {
                        fprintf(fp, "*");
                    }
                }
                prog_es_cnt += prog->pmt.es_cnt;
            }
            if (prog->filtered) {
                fprintf(fp, ": filtered");
            }
            fprintf(fp, "\n");
        }

        flow_cnt++;
        if (!session_is_blocked(ses->state)) {
            prog_cnt += in_ses->pat.prog_cnt;
            es_cnt += prog_es_cnt;
        }
    }
    fprintf(fp, "Total: %d flows, %d progs, %d pids, %d bps\n",
            flow_cnt, prog_cnt, es_cnt, total_rate);
}


void show_qam_map (FILE *fp, video_context_t *ctx, uint16 qam_id)
{
    int i;
    fprintf(fp, "QAM %d flow map:\n", qam_id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        int32 map = ctx->qam_flow[qam_id][i];
        if (map != INVALID_ID) {
            fprintf(fp, "  Flow %d: in %d, out %d\n",
                    i, get_in_id(map), get_out_id(map));
        }
    }
    fprintf(fp, "Total %d flows\n", get_qam_config(ctx, qam_id)->flow_cnt);
}


#if 0
void pid_histogram_print (FILE *fp, uint32 *histogram)
{
    int i;
    uint32 tot = 0;

    fprintf(fp, "PID histogram:\n");
    for (i=0; i<NUM_PIDS; i++) {
        if (histogram[i] > 0) {
            fprintf(fp, "  Pid %d: %d\n", i, histogram[i]);
            tot += histogram[i];
        }
    }
    fprintf(fp, "Total: %d\n", tot);
}
#endif


// Set up input PID info table for all PIDs referenced in PMT
void in_pid_table_setup_pmt (in_pid_info_t* pid_tab, pmt_info_t *pmt)
{
    // Set up for PCR pid
    uint16 pid = pmt->pcr_pid;
    if (pid != NULL_PID) { 
        pid_tab[pid].has_pcr = 1;
        pid_tab[pid].referenced = 1;
    }           

    // Set up for all referenced PIDs
    int i;      
    for (i=0; i<pmt->es_cnt; i++) {
        pid = pmt_get_es_pid(pmt_info_get_es(pmt, i));
        pid_tab[pid].referenced = 1;
    }           

    // TODO: should we also include CA pids?
}


// Clear up input PID info table for PMT pid and all referenced PID in PMT
void in_pid_table_clear_pmt (in_pid_info_t* pid_tab, uint16 pmt_pid,
                             pmt_info_t *pmt)
{
    // Clear PMT pid
    in_pid_info_t* info = &pid_tab[pmt_pid];
    info->seen = 0;
    info->has_psi = 0;
    info->has_pcr = 0;
    info->referenced = 0;

    // Set up for PCR pid
    uint16 pid = pmt->pcr_pid;
    if (pid != NULL_PID) {
        info = &pid_tab[pid];
        info->seen = 0;
        info->has_psi = 0;
        info->has_pcr = 0;
        info->referenced = 0;
    }

    // Set up for all referenced PIDs
    int i;
    for (i=0; i<pmt->es_cnt; i++) {
        pid = pmt_get_es_pid(pmt_info_get_es(pmt, i));
        info = &pid_tab[pid];
        info->seen = 0;
        info->has_psi = 0;
        info->has_pcr = 0;
        info->referenced = 0;
    }

    // TODO: should we also include CA pids?
}


void in_pid_table_print (FILE *fp, in_pid_info_t *tab)
{
    int i;
    int cnt = 0;
    fprintf(fp, "Input PID table:");
    for (i=0; i<NUM_PIDS; i++, tab++) {
        if (tab->seen) {
            fprintf(fp, "\n  Pid %d: CC %d", i, tab->prev_cc);
            if (tab->has_psi)  fprintf(fp, ", PSI");
            if (tab->has_pcr)  fprintf(fp, ", PCR");
            if (!tab->referenced)  fprintf(fp, ", Unref");
            cnt++;
        }
    }
    fprintf(fp, "\nTotal %d PIDs found\n", cnt);
}


void out_pid_table_print (FILE *fp, out_pid_info_t *tab)
{
    int i;
    int cnt = 0;
    fprintf(fp, "Output PID table:");
    for (i=PAT_PID+1, tab++; i<NULL_PID; i++, tab++) {
        if (tab->reversed) {
            fprintf(fp, "\n  Pid %d -> %d, reversed %d / %d",
                    i, tab->pid, tab->pid_reversed, tab->prog_reversed);
            cnt++;
        }
    }
    fprintf(fp, "\nTotal %d PIDs reversed\n", cnt);
}


#define MAX_TP_PER_IP           7
#define MAX_PAYLOAD_LEN         (MAX_TP_PER_IP * TP_SIZE + 32)
                                // 32 bytes to account for IP/UDP header and FCS

// display captured L3 header 
void video_show_l3_hdr_payload (FILE *fp)
{
    int payload_len;
    udp_header_t *udp_hdr;
    int offset = get_l2_header_size();

    ip_header_t* ip_hdr = (ip_header_t *)(capture_buf + offset);
    ipv6_header_t* ipv6_hdr = (ipv6_header_t *)(capture_buf + offset);

    boolean v4_hdr = TRUE;
    if (ip_hdr->ver == IPV4_VERSION) {
        udp_hdr = (udp_header_t*)(ip_hdr + 1);
        v4_hdr = TRUE;
        payload_len = ip_hdr->len -sizeof(ip_header_t) - sizeof(udp_header_t);
    } else {
        udp_hdr = (udp_header_t*)(ipv6_hdr + 1);
        v4_hdr = FALSE;
        payload_len = ipv6_hdr->payload_length - sizeof(udp_header_t);
    }

    if (payload_len > MAX_PAYLOAD_LEN) {
        fprintf(fp, "  Payload is too long.  Reduced to %d bytes.\n",
               MAX_PAYLOAD_LEN);
        payload_len = MAX_PAYLOAD_LEN;
    }

    if (capture_flag) {
        if (v4_hdr) {
            fprintf(fp, "IP hdr:\n");
            fprint_hex(fp, ip_hdr, sizeof(ip_header_t));
        } else {
             fprintf(fp, "IP V6 hdr:\n");
             fprint_hex(fp, ipv6_hdr, sizeof(ipv6_header_t));
        }
        fprintf(fp, "UDP hdr:\n");
        fprint_hex(fp, udp_hdr, sizeof(udp_header_t));  
        fprintf(fp, "Payload:\n");
        fprint_hex(fp, udp_hdr+1, payload_len);
    } else {
        fprintf(fp, "Packet not captured yet\n");
    }
}


int video_list_search (uint16 value, uint16* list, uint8 num_item)
{
    int i;
    for (i=0; i<num_item; i++) {
        if (list[i] == value)  return i;
    }
    return -ENOENT;
}


int video_list_add (uint16 value, uint16* list, uint8* num_item, uint8 max)
{
    if (video_list_search(value, list, *num_item) != -ENOENT)  return EEXIST;
    if (*num_item >= max)  return ENOMEM;
    video_list_set(value, list, (*num_item)++);
    return EOK;
}


int video_list_delete (uint16 value, uint16* list, uint8* num_item)
{
    int idx = video_list_search(value, list, *num_item);
    if (idx == -ENOENT)  return ENOENT;
    video_list_set(list[(*num_item)-1], list, idx);
    video_list_set(VIDEO_LIST_INVALID_ITEM, list, (*num_item)--);
    return EOK;
}


// Print video list
void video_list_print (FILE *fp, uint16 *list, uint8 num_item)
{
    int i;
    for (i=0; i<num_item; i++) {
        fprintf(fp, " %d", list[i]);
    }
}


/// Check if program number is having conflict in QAM
boolean prog_num_check_conflict (uint32 prog_num,
                                 qam_prog_info_t *qam_prog_tab)
{
    qam_prog_info_t* info = &qam_prog_tab[prog_num];
    return (info->used);
}


/// Check if PID is having conflict in QAM
/// (PID already registered by other session in QAM)
static inline
boolean pid_check_conflict (uint32 pid, qam_pid_info_t *qam_pid_tab,
                            uint16 ses_id)
{
    qam_pid_info_t* info = &qam_pid_tab[pid];
    return (info->used && info->out_ses_id != ses_id);
}


// Check if a program causes any conflict in QAM on program number and PMT PID
//   pid contains the conflicting PMT pid if appropriate, otherwise NULL_PID
boolean out_prog_check_conflict (out_prog_t *prog, out_session_t *ses,
                                 uint16 *pid)
{
    *pid = NULL_PID;
    qam_info_t* qam = get_qam_info(ses->qam_id);
    if (prog_num_check_conflict(prog->prog_num, qam->prog_tab)) {
        //VIDEO_DEBUG("\nProgram number %d conflict detected", prog->prog_num);
        return TRUE;
    }
    if (prog->pmt_pid == INVALID_PID ||
            pid_check_conflict(prog->pmt_pid, qam->pid_tab, ses->id)) {
        //VIDEO_DEBUG("\nPMT pid %d conflict detected", prog->pmt_pid);
        *pid = prog->pmt_pid;
        return TRUE;
    }
    return FALSE;
}


// Check if a PMT causes any conflict in QAM
//   pid will contain the conflicting PID
boolean pmt_check_conflict (pmt_info_t *pmt, out_session_t *ses, uint16 *pid)
{
    int i;
    qam_pid_info_t* qam_pid_tab = get_qam_info(ses->qam_id)->pid_tab;

    for (i=0; i<pmt->es_cnt; i++) {
        *pid = pmt_get_es_pid(pmt_info_get_es(pmt, i));
        if (pid_check_conflict(*pid, qam_pid_tab, ses->id))  return TRUE;
    }
    for (i=0; i<pmt->ca_desc_cnt; i++) {
        *pid = ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i));
        if (pid_check_conflict(*pid, qam_pid_tab, ses->id))  return TRUE;
    }

    *pid = NULL_PID;
    return FALSE;
}


// Recheck the conflict status of all blocked program in an out session
boolean out_session_recheck_conflict (out_session_t *ses, qam_info_t *qam)
{
    //VIDEO_DEBUG("\nSes %d: recheck conflict", ses->cfg->client_id);

    boolean changed = FALSE;
    boolean ses_blocked = FALSE;
    out_prog_t *prog;
    FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
        if (prog->filtered)  continue;

        if (prog->blocked) {
            uint16 conflict_pid;
            prog->blocked = out_prog_check_conflict(prog, ses, &conflict_pid);
            prog->blocked |= prog->pmt_built &&
                                 pmt_check_conflict(&prog->pmt, ses,
                                                    &conflict_pid);

            //VIDEO_DEBUG("\nProg %d-%d: recheck conflict: pmt_built %d, res %d",
            //            ses->cfg->client_id, prog->prog_num, prog->pmt_built,
            //            prog->blocked);

            // If a ses is local encrypted, block till at least the first CA desc is already added to PMT.
            prog->blocked |= (ses->cfg->encrypt_flag && 
                             (prog->pmt.ca_desc_cnt > prog->in_prog->pmt.ca_desc_cnt) && 
                              prog->in_prog->pmt.ca_desc_cnt);           

            //VIDEO_DEBUG("\nProg %d-%d: ca conflict: encrypt_flag %d,ca_des_cnt(out %d, in %d) res %d",
            //            ses->cfg->client_id, prog->prog_num, ses->cfg->encrypt_flag, prog->pmt.ca_desc_cnt,
            //            prog->in_prog->pmt.ca_desc_cnt, prog->blocked);

            if (prog->blocked) {
                ses_blocked = TRUE;

            } else {
                VIDEO_DEBUG("\nProg %d-%d: conflict recovered",
                            ses->cfg->client_id, prog->prog_num);
                out_prog_set_register(prog, 1, 1);
                out_prog_register(prog, ses, qam);
                prog->cfg_changed = 1;
                changed = TRUE;
            }
        }
    }

    ses->state &= ~SESSION_STATE_BLOCKED;
    if (ses_blocked)  ses->state |= SESSION_STATE_BLOCKED;

    return changed;
}


/// Set out_pid_table for an out program
///   Will be applied to all referenced PIDs in PMT.
///   pid_rev and prog_rev will be used to replace the corresponding
///   field in the out_pid_table entry for the referenced PID unless
///   they are set to NO_CHANGE.
///
void out_prog_set_out_pid_table (out_prog_t *prog, out_pid_info_t *out_pid_tab,
                                 uint16 pid_rev, uint16 prog_rev)
{
    //VIDEO_DEBUG("\nSet Out Pid: pid-rev %d, prog-rev %d", pid_rev, prog_rev);

    uint16 pid = prog->in_pmt_pid;
    out_pid_info_set(&out_pid_tab[pid], pid_rev, prog_rev, prog->pmt_pid);
    //VIDEO_DEBUG("\n  PMT %d", pid);

    if (!prog->pmt_built)  return;

    pmt_info_t* pmt = &prog->pmt;
    pid = pmt->orig_pcr_pid;
    out_pid_info_set(&out_pid_tab[pid], pid_rev, prog_rev, pmt->pcr_pid);
    //VIDEO_DEBUG(", PCR %d", pid);

    int i;
    //VIDEO_DEBUG(", ES: ");
    for (i=0; i<pmt->es_cnt; i++) {
        pid = pmt->es[i].orig_pid;
        out_pid_info_set(&out_pid_tab[pid], pid_rev, prog_rev,
                         pmt_get_es_pid(pmt_info_get_es(pmt, i)));
        //VIDEO_DEBUG("%d ", pmt_get_es_pid(pmt_info_get_es(pmt, i)));
    }

    //VIDEO_DEBUG(", CA: ");
    for (i=0; i<pmt->ca_desc_cnt; i++) {
        pid = pmt->ca_desc[i].orig_pid;
        out_pid_info_set(&out_pid_tab[pid], pid_rev, prog_rev,
                         ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i)));
        //VIDEO_DEBUG("%d ", ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i)));
    }
    //VIDEO_DEBUG("\n");
}


/// Set out_pid_table for an out program
///   Will be applied to all referenced PIDs in PMT.
///   used, forwarded, and out_ses will be used to replace the corresponding
///   field in the out_pid_table entry for the referenced PID unless they are
///   set to NO_CHANGE.
///
void out_prog_set_qam_pid_table (out_prog_t *prog, qam_pid_info_t *qam_pid_tab,
                                 uint16 used, uint16 forwarded,
                                 uint16 out_ses_id)
{
    //VIDEO_DEBUG("\nSet Qam Pid: used %d, fwd %d, sesId %d",
    //            used, forwarded, out_ses_id);

    qam_pid_info_t* info = &qam_pid_tab[prog->pmt_pid];
    qam_pid_info_set(info, used, forwarded, out_ses_id, NO_CHANGE);
    //VIDEO_DEBUG("\n  PMT %d->%d", prog->in_pmt_pid, prog->pmt_pid);

    if (!prog->pmt_built)  return;

    pmt_info_t* pmt = &prog->pmt;
    info = &qam_pid_tab[pmt->pcr_pid];
    qam_pid_info_set(info, used, forwarded, out_ses_id, NO_CHANGE);
    //VIDEO_DEBUG(", PCR %d->%d", pmt->orig_pcr_pid, pmt->pcr_pid);

    int i;
    //VIDEO_DEBUG(", ES: ");
    for (i=0; i<pmt->es_cnt; i++) {
        info = &qam_pid_tab[pmt_get_es_pid(pmt_info_get_es(pmt, i))];
        qam_pid_info_set(info, used, forwarded, out_ses_id, NO_CHANGE);
        //VIDEO_DEBUG("%d->%d ", pmt->es[i].orig_pid,
        //            pmt_get_es_pid(pmt_info_get_es(pmt, i)));
    }

    //VIDEO_DEBUG(", CA: ");
    for (i=0; i<pmt->ca_desc_cnt; i++) {
        info = &qam_pid_tab[ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i))];
        qam_pid_info_set(info, used, forwarded, out_ses_id, NO_CHANGE);
        //VIDEO_DEBUG("%d->%d ", pmt->ca_desc[i].orig_pid,
        //            ca_desc_get_pid(pmt_info_get_ca_desc(pmt, i)));
    }
    //VIDEO_DEBUG("\n");
}


// Need to address the qam table corruption issue!!
void out_prog_register (out_prog_t *prog, out_session_t *ses, qam_info_t *qam)
{
    //VIDEO_DEBUG(" %d/%d/%d/%d ", prog->ses_reg_prog, prog->ses_reg_pid,
    //            prog->qam_reg_prog, prog->qam_reg_pid);

    boolean remapped = ses->flags & SESSION_FLAG_PID_REMAP;
    boolean included = !(prog->filtered | prog->blocked);

    if (prog->ses_reg_prog) {
        out_pid_info_set(&ses->pid_tab[prog->in_pmt_pid], NO_CHANGE,
                    remapped ^ !included, prog->pmt_pid);
        prog->ses_reg_prog = 0;
    }

    if (prog->qam_reg_prog) {
        uint16 prog_id = get_out_prog_id(prog);
        qam_prog_info_t* info = &qam->prog_tab[prog->prog_num];
        if (included || info->out_prog_id == prog_id) {
            qam_prog_info_set(info, included, prog_id);
        }
        qam_pid_info_set(&qam->pid_tab[prog->pmt_pid], included, included,
                         ses->id, NO_CHANGE);
        prog->qam_reg_prog = 0;
    }

    if (!prog->pmt_built) {
        if (!(prog->ses_reg_pid | prog->ses_reg_pid))  prog->cfg_changed = 0;
        return;
    }

    if (prog->ses_reg_pid) {
        out_prog_set_out_pid_table(prog, ses->pid_tab, NO_CHANGE,
                                   remapped ^ !included);
        prog->ses_reg_pid = 0;
    }

    if (prog->qam_reg_pid) {
        out_prog_set_qam_pid_table(prog, qam->pid_tab, included, included,
                                   ses->id);
        prog->qam_reg_pid = 0;
    }

    prog->cfg_changed = 0;
}


// Unregister a program
void out_prog_unregister (out_prog_t *prog, out_session_t *ses, qam_info_t *qam)
{
    boolean remapped = ses->flags & SESSION_FLAG_PID_REMAP;
    boolean excluded = prog->filtered || prog->blocked;

    if (remapped ^ excluded) {
        out_pid_info_set(&ses->pid_tab[prog->in_pmt_pid], 0, 0, INVALID_PID);
        if (prog->pmt_built) {
            out_prog_set_out_pid_table(prog, ses->pid_tab, NO_CHANGE, 0);
        }
    }

    if (!excluded) {
        qam_prog_info_set(&qam->prog_tab[prog->prog_num], 0, 0);
        qam_pid_info_set(&qam->pid_tab[prog->pmt_pid], 0, 0, ses->id,
                         NO_CHANGE);
        if (prog->pmt_built) {
            out_prog_set_qam_pid_table(prog, qam->pid_tab, 0, 0, ses->id);
        }
    }
}

boolean is_PME_session(out_prog_t *prog)
{
#if 0
   pmt_info_t* pmt = &prog->pmt;
   int i;

   for (i=0; i<pmt->ca_desc_cnt; i++) 
   {
       ca_descriptor_header_t* ca_desc = pmt_info_get_ca_desc(pmt, i);
       uint16_t cas_id = ca_desc_get_sys_id(ca_desc);
       if (cas_id == PME_ECMG_SUPER_CAS_ID)
           return true;
   }
#endif
   return FALSE;
}

// Remap program number and PMT pid of a remapped program
void out_prog_remap_prog (out_prog_t *prog, out_session_t *ses)
{
    out_config_t* cfg = ses->cfg;

    // Set out program number
    prog->prog_num = cfg->out_prog_list[prog->cfg_idx];

    // Allocate out PMT pid
    video_pid_range_t* pid_range = &cfg->pid_range[prog->cfg_idx];
    prog->pmt_pid = qam_find_next_avail_pid(pid_range->first, pid_range->last,
                                            ses->qam_id);
    if (prog->pmt_pid == NULL_PID)  prog->blocked = 1;

    // TODO: How about prog->pmt_ver?
}
 

void video_perf_wake_update (video_perf_t *perf, uint32 now)
{
    if (perf->now) {
        int32 wake_itvl = (int32)(now - perf->now);
        if (wake_itvl > perf->max_wake_itvl)  perf->max_wake_itvl = wake_itvl;
        perf->avg_wake_itvl += (wake_itvl - perf->avg_wake_itvl) >> 4;
    }
    perf->now = now;
}

void out_session_clear_ca_pids(out_session_t *ses, out_prog_t *prog)
{
    if (!ses || !prog || !ca_desc_get_sys_id (&prog->ca_desc_data)) {
        //VIDEO_DEBUG("\n%s %d:NULL input", __FUNCTION__,__LINE__);
        return;
    }
    qam_info_t* qam = get_qam_info(ses->qam_id);
    uint16 ecm_pid = ca_desc_get_pid(&prog->ca_desc_data);
    qam_pid_info_t *ecm_pid_info = &qam->pid_tab[ecm_pid];
    //clear used flag
    if (ecm_pid_info) {
        ecm_pid_info->used=0;         
        ecm_pid_info->out_ses_id=MAX_SESSIONS;
        //VIDEO_DEBUG("\n%s %d:ecm_pid[%d] cleared", __FUNCTION__,__LINE__, ecm_pid);
    }
}
