///  Copyright (c) 2014-2015 by Cisco Systems, Inc.
///  All rights reserved.
///

#define _GNU_SOURCE
#include <stdlib.h>
#include "common.h"
#include "crc.h"
#include "video.h"
#include "video_util.h"
#include "sim_video.h"

mpeg_timediff_t hw_time_adj = 0;

// Simulated mpeg clock
mpeg_time_t sys_time = 0;
mpeg_time_t sys_time_hi = 0;

void in_session_lock (int rid) {}
void in_session_unlock (int rid) {}
void out_session_lock (int rid) {}
void out_session_unlock (int rid) {}
void qam_channel_lock (int rid) {}
void qam_channel_unlock (int rid) {}
void psi_lock (int rid) {}
void psi_unlock (int rid) {}
void crsl_lock (int rid) {}
void crsl_unlock (int rid) {}
void ses_hash_table_lock (int port_id) {}
void ses_hash_table_unlock (int port_id) {}
void prog_hash_table_lock (void) {}
void prog_hash_table_unlock (void) {}
void pid_hash_table_lock (void) {}
void pid_hash_table_unlock (void) {}

int out_cfg_wait_for_pending (out_config_t *cfg) { return 0; }

boolean qam_channel_ready (uint16 qam_id)
{ return TRUE; }

boolean qam_is_encrypt (uint16 qam_id)
{
    assert(qam_id < NUM_QAMS);
    return get_qam_config(video_ctx, qam_id)->encrypt_flag;
}

boolean is_video_qam (uint16 qam_id)
{ return TRUE; }

int32 video_get_flow_avail_space (uint16 qam_id, uint16 flow_id)
{ return OUT_CMD_PER_FLOW; }

int get_l2_header_size (void)
{ return 0; }

// Note: this is the quick-and-dirty way to fix the PI/PD violation in
//       the current code base.  We should fix this in the future.
void *pclc_cvmx_bootmem_alloc_named (uint64_t size, uint64_t alignment,
                                     const char *name)
{ return malloc(size); }

void *pclc_cvmx_bootmem_find_named (uint64_t *size, const char *name)
{ return NULL; }

void pclc_cvmx_bootmem_free_named (void *ptr, const char *name)
{}


out_command_t* get_cmd_buf (uint16 qam_id, uint16 flow_id)
{
    return out_cmd_buf[qam_id][flow_id];
}


out_command_t* get_cmd (uint16 qam_id, uint16 flow_id, uint16 idx)
{
    return &get_cmd_buf(qam_id, flow_id)[idx];
}


ring_buf_t* get_cmd_ring (uint16 qam_id, uint16 flow_id)
{
    return &out_cmd_ring[qam_id][flow_id];
}


// Get and prepare the next output command for a flow
out_command_t* setup_outcmd (uint16 qam_id, uint16 flow_id,
                             uint32 out_time, uintptr_t addr)
{
    out_command_t* buf = get_cmd_buf(qam_id, flow_id);
    ring_buf_t* flow = get_cmd_ring(qam_id, flow_id);
    out_command_t* cmd = &buf[flow->wr++];

    OUTCMD_FLAGS(cmd) = 0;
    cmd->qam_id = qam_id;
    cmd->flow_id = flow_id;
    cmd->out_time = out_time;
    cmd->addr = addr;

    flow->wr = ring_mod(flow->wr, OUTCMDS_PER_FLOW);

    // Prefetch the next out command (can save up to 3% CPU load)
    data_hint(cmd + 1);

    return cmd;
}


void outcmd_copy (out_command_t *dst_outcmd, out_command_t *src_outcmd)
{
    uint64 *dst = (uint64*)dst_outcmd;
    uint64 *src = (uint64*)src_outcmd;
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}


uint32 outcmd_get_outtime (out_command_t *outcmd)
{
    return outcmd->out_time;
}


// Setup output command for PID remapping
void outcmd_set_pid_restamp (out_command_t *cmd, uint16 out_pid)
{
    cmd->pid_restamp_flag = 1;
    cmd->pid = out_pid;
}
   

// Setup output command for PCR restamping
void outcmd_set_pcr_restamp (out_command_t *cmd, mpeg_clock_t out_tbo)
{
    cmd->pcr_restamp_flag = 1;
    cmd->tbo_base = out_tbo.base;
    cmd->tbo_ext = out_tbo.ext;
}
   

// Setup output command for CC restampoing
void outcmd_set_cc_restamp (out_command_t *cmd, uint8 out_cc)
{
    cmd->cc_restamp_flag = 1;
    cmd->cc = out_cc;
}


// Setup output command for scrambling
void outcmd_set_cw_index (out_command_t *cmd, uint16 cw_idx)
{
//    cmd->cwi_hi = cw_idx >> 12;
//    cmd->cwi_lo = cw_idx & 0xFFF;
}


out_command_t* get_muxcmd (uint16 qam_id)
{
    out_command_t* cmd;
    ring_buf_t* flow = &mux_cmd_ring[qam_id];
    int16 wr = ring_mod(flow->wr + 1, OUTCMDS_PER_FLOW);
    if (wr == flow->rd) {
        return NULL;
    }
    cmd = &mux_cmd_buf[qam_id][flow->wr];
    flow->wr = wr;
    return cmd;
}


#define SCALE_NSEC_EXT  (0.027)
struct timespec local_clk;

void init_mpeg_time (void)
{
    clock_gettime(CLOCK_REALTIME, &local_clk);    
    sys_time = 0L;
}

void get_mpeg_time (mpeg_clock_t *clk)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);    

    int64 nsec_inc = (int64)(now.tv_nsec - local_clk.tv_nsec);
    int64 sec_inc = (int64)(now.tv_sec - local_clk.tv_sec);

    local_clk = now;

    int64 ext_inc = (int64)(SCALE_NSEC_EXT * nsec_inc);
    ext_inc += EXT_FREQ * sec_inc;

    sys_time += ext_inc << FRAC_BITS_TIME;
    mpeg_time_normalize(&sys_time);
    mpeg_time_to_clock(sys_time, clk);
}


// Simulated time in sec
uint64 get_clock_time (void)
{
    static uint64 WRAP_SEC = (((uint64)SCALE_BASE_EXT) << 32) / EXT_FREQ;
    return sys_time_hi * WRAP_SEC + (sys_time >> FRAC_BITS_TIME) / EXT_FREQ;
}


uint64 get_elapsed_sec (uint64 start_time, uint64 end_time)
{
    return end_time - start_time;
}


void process_outcmd_for_flow (uint16 qam_id, uint16 flow_id)
{ }

void video_hw_stat_update (void)
{ }

void set_load_credit (int value)
{ }

boolean acquire_load_credit (int value)
{ return FALSE; }


// Hash code for input session
uint32 ses_hash_code (void *key)
{
    ses_hash_key_t* key2 = key;
    uint32 hash = key2->dst_ip[0];
    if (key2->ip_ver == IPV6_VERSION) {
        hash ^= key2->dst_ip[1] ^ key2->dst_ip[2] ^ key2->dst_ip[3];
    }
    if (!key2->mcast_flag) {
        hash *= key2->extra[0];
    }
    return hash;
}


// Hash code for session ID / carousel resource ID
uint32 resource_id_hash (void *key)
{
    return *(uint32*)key;
}

// Seesion ID / carousel resource ID matching
boolean resource_id_match (void *key, hash_item_t *item)
{
    return *(uint32*)key == item->hash_code;
}

// Session ID printing
void ses_id_print (FILE *fp, hash_item_t *item)
{
    resource_id_hash_item_t* item2 = (resource_id_hash_item_t*)item;
    printf("  Ses %d: out %d\n", item2->hash_code, item2->id);
}

// Carousel resource ID printing
void crsl_id_print (FILE *fp, hash_item_t *item)
{
    resource_id_hash_item_t* item2 = (resource_id_hash_item_t*)item;
    printf("  Resource %d: crsl %d\n", item2->hash_code, item2->id);
}


// Mapping from session ID to out sesion ID
uint16 session_id_to_out_id (video_context_t *ctx, uint32 ses_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item = (resource_id_hash_item_t*)
            hash_table_lookup(sim_ctx->ses_id_hash_tab, &ses_id);
    return item ? item->id : INVALID_SES_ID;
}

// Allocate a session ID
uint16 session_id_alloc (video_context_t *ctx, uint32 ses_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item =
            (resource_id_hash_item_t*)pool_alloc(&sim_ctx->ses_id_hash_pool);
    if (!item)  return INVALID_SES_ID;
    item->used_flag = TRUE;
    hash_table_add(sim_ctx->ses_id_hash_tab, &ses_id, (hash_item_t*)item);
    return item->id;
}

// Free a session ID
void session_id_free (video_context_t *ctx, uint32 ses_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item = (resource_id_hash_item_t*)
            hash_table_delete(sim_ctx->ses_id_hash_tab, &ses_id);
    if (item) {
        item->used_flag = FALSE;
        pool_free(&sim_ctx->ses_id_hash_pool, &item->link);
    }
}

// Mapping from carousel resource ID to crsl ID
uint16 resource_id_to_crsl_id (video_context_t *ctx, uint32 resource_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item = (resource_id_hash_item_t*)
            hash_table_lookup(sim_ctx->crsl_id_hash_tab, &resource_id);
    return item ? item->id : INVALID_CRSL_ID;
}

// Allocate a crsl ID
uint16 crsl_id_alloc (video_context_t *ctx, uint32 resource_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item =
            (resource_id_hash_item_t*)pool_alloc(&sim_ctx->crsl_id_hash_pool);
    if (!item)  return INVALID_CRSL_ID;
    item->used_flag = TRUE;
    hash_table_add(sim_ctx->crsl_id_hash_tab, &resource_id, (hash_item_t*)item);
    return item->id;
}

// Free a crsl ID
void crsl_id_free (video_context_t *ctx, uint32 resource_id)
{
    sim_video_context_t* sim_ctx = (sim_video_context_t*)ctx->platform;
    resource_id_hash_item_t* item = (resource_id_hash_item_t*)
            hash_table_delete(sim_ctx->crsl_id_hash_tab, &resource_id);
    if (item) {
        item->used_flag = FALSE;
        pool_free(&sim_ctx->crsl_id_hash_pool, &item->link);
    }
}


int tp_buf_pool_create_named (pool_t *pool, int num_buf, const char *name)
{
    return pool_create(pool, TP_SIZE + sizeof(que_elem_t), num_buf, NULL);
}


int tp_buf_pool_destroy_named (pool_t* pool, const char *name)
{
    pool_destroy(pool);
    return EOK;
}


void platform_video_context_cleanup (void* platform_ctx)
{
    if (!platform_ctx)  return;

    sim_video_context_t* sim_ctx = (sim_video_context_t*)platform_ctx;
    if (sim_ctx->ses_id_hash_tab) {
        hash_table_free(sim_ctx->ses_id_hash_tab);
    }
    pool_destroy(&sim_ctx->ses_id_hash_pool);

    if (sim_ctx->crsl_id_hash_tab) {
        hash_table_free(sim_ctx->crsl_id_hash_tab);
    }
    pool_destroy(&sim_ctx->crsl_id_hash_pool);

    free(sim_ctx);
}


void* platform_video_context_init (void)
{
    sim_video_context_t* sim_ctx = calloc(1, sizeof(sim_video_context_t));
    if (sim_ctx == NULL)  goto fail;

    // Set up hash table for session ID lookup
    sim_ctx->ses_id_hash_tab
            = hash_table_create(SES_ID_HASH_SLOTS, resource_id_hash,
                                resource_id_match, ses_id_print);
    if (sim_ctx->ses_id_hash_tab == NULL)  goto fail;

    // Set up session ID hash item pool
    int rc = pool_create(&sim_ctx->ses_id_hash_pool,
                         sizeof(resource_id_hash_item_t), MAX_SESSIONS, NULL);
    if (!rc == EOK)  goto fail;

    resource_id_hash_item_t* item = sim_ctx->ses_id_hash_pool.buf;
    int i;
    for (i=0; i<MAX_SESSIONS; i++) {
        (item++)->id = i;
    }

    // Set up hash table for carousel resource ID lookup
    sim_ctx->crsl_id_hash_tab
            = hash_table_create(CRSL_ID_HASH_SLOTS, resource_id_hash, 
                                resource_id_match, crsl_id_print);
    if (sim_ctx->crsl_id_hash_tab == NULL)  goto fail;

    // Set up carousel resource ID hash item pool
    rc = pool_create(&sim_ctx->crsl_id_hash_pool,
                     sizeof(resource_id_hash_item_t), NUM_CRSLS, NULL);
    if (!rc == EOK)  goto fail;

    item = sim_ctx->crsl_id_hash_pool.buf;
    for (i=0; i<NUM_CRSLS; i++) {
        (item++)->id = i;
    }

    return sim_ctx;

fail:
    platform_video_context_cleanup(sim_ctx);
    return NULL;
}



