/*
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __VIDEO_UTIL_H__
#define __VIDEO_UTIL_H__

#include <arpa/inet.h>
#include "bit_mask.h"
#include "common.h"
#include "video.h"
#include "video_query.h"


static inline
boolean video_context_inited (int id)
{
    return (video_context[id] != NULL);
}

static inline
video_context_t* get_video_context (int id)
{
    return video_context[id];
}

static inline
que_elem_t* get_in_session_id (video_context_t *ctx, uint32 rid)
{
    assert(ctx);
    return &((que_elem_t*)ctx->in_ses_id_pool.buf)[rid];
}

static inline
in_session_t* get_in_session (uint32 rid)
{
    assert(in_session);
    assert(rid < MAX_SESSIONS);
    return &in_session[rid];
}

static inline
out_session_t* get_out_session (uint32 rid)
{
    assert(out_session);
    assert(rid < MAX_SESSIONS);
    return &out_session[rid];
}

static inline
qam_info_t* get_qam_info (uint32 qam_id)
{
    assert(qam_info);
    assert(qam_id < NUM_QAMS);
    return &qam_info[qam_id];
}

static inline
in_config_t* get_in_session_config (video_context_t *ctx, uint32 rid)
{
    assert(ctx);
    assert(ctx->in_session_config);
    assert(rid < MAX_SESSIONS);
    return &ctx->in_session_config[rid];
}

static inline
out_config_t* get_out_session_config (video_context_t *ctx, uint32 rid)
{
    assert(ctx);
    assert(ctx->out_session_config);
    assert(rid < MAX_SESSIONS);
    return &ctx->out_session_config[rid];
}

static inline
out_prog_t* get_out_prog (uint16 out_prog_id)
{
    return ((out_prog_t*)out_prog_pool.buf) + out_prog_id;
}

static inline
uint16 get_out_prog_id (out_prog_t *prog)
{
    return prog - (out_prog_t*)out_prog_pool.buf;
}

static inline
qam_config_t* get_qam_config (video_context_t *ctx, uint32 qam_id)
{
    assert(ctx);
    assert(ctx->qam_config);
    return &ctx->qam_config[qam_id];
}

static inline
int32* get_qam_flow_map (video_context_t *ctx, uint32 qam_id)
{
    assert(ctx);
    assert(ctx->qam_flow);
    return ctx->qam_flow[qam_id];
}

static inline
uint16 get_in_id (int32 id_map)
{
    return id_map >> 16;
}

static inline
uint16 get_out_id (int32 id_map)
{
    return id_map & 0xFFFF;
}

static inline
que_elem_t* get_qam_crsl_insert_list (video_context_t *ctx, uint32 qam_id)
{
    assert(ctx);
    return &ctx->crsl_insert_list[qam_id];
}

static inline
crsl_t* get_crsl (video_context_t *ctx, uint32 rid)
{
    assert(ctx);
    assert(ctx->crsl);
    return &ctx->crsl[rid];
}

static inline
int crsl_insert_due (crsl_insert_t *insert)
{
    return ((insert->target_cnt > 0) && (insert->cnt < insert->target_cnt)
            && ((int32)(insert->next_time - sys_clk.base) <= 0)); 
}


static inline
int psi_crsl_insert_due (psi_crsl_t *crsl)
{
    return ((int32)(crsl->next_time - sys_clk.base)) <= 0; 
}


// Check if an IPv4 address is multicast
#ifdef __BIG_ENDIAN__
static inline
boolean is_multicast_v4 (uint32 ip_addr)
{
    return (ip_addr >> 28) == 0xE;
}

// Check if an IPv6 address is multicast
static inline
boolean is_multicast_v6 (uint32 ip_addr[])
{
    return (ip_addr[0] >> 24) == 0xFF;
}
#endif  // __BIG_ENDINA__


#ifdef __LITTLE_ENDIAN__
static inline
boolean is_multicast_v4 (uint32 ip_addr)
{
    return (ntohl(ip_addr) >> 28) == 0xE;
}

// Check if an IPv6 address is multicast
static inline
boolean is_multicast_v6 (uint32 ip_addr[])
{
    return (ntohl(ip_addr[0]) >> 24) == 0xFF;
}
#endif  // __LITTLE_ENDIAN__


static inline
boolean session_is_used (uint8 state)
{
    return ((state & SESSION_STATE_USED));
}

static inline
boolean session_is_enabled (uint8 state)
{
    return ((state & SESSION_STATE_ENABLED));
}

static inline
boolean session_is_psi_ready (uint8 state)
{
    return ((state & SESSION_STATE_PSI_READY));
}

static inline
boolean session_is_blocked (uint8 state)
{
    return ((state & SESSION_STATE_BLOCKED));
}

static inline
boolean session_is_no_resources (uint8 state)
{
	return ((state & SESSION_STATE_NO_RESOURCES));
}

static inline
boolean session_is_init (uint8 state)
{
    return ((state & SESSION_STATE_TRAFFIC_MASK) == SESSION_STATE_INIT);
}

static inline
boolean session_is_active (uint8 state)
{
    return ((state & SESSION_STATE_TRAFFIC_MASK) == SESSION_STATE_ACTIVE);
}

static inline
boolean session_is_idle (uint8 state)
{
    return ((state & SESSION_STATE_TRAFFIC_MASK) == SESSION_STATE_IDLE);
}

static inline
boolean session_is_off (uint8 state)
{
    return ((state & SESSION_STATE_TRAFFIC_MASK) == SESSION_STATE_OFF);
}

static inline
boolean session_is_udp (in_config_t *cfg)
{
    return (cfg->input_type == INPUT_TYPE_UNICAST);    
}

static inline
boolean session_is_asm (in_config_t *cfg)
{
    return (cfg->input_type == INPUT_TYPE_MULTICAST_ASM); 
}

static inline
boolean session_is_ssm (in_config_t *cfg)
{
    return (cfg->input_type == INPUT_TYPE_MULTICAST_SSM);   
}

static inline
boolean session_is_mux (in_config_t *cfg)
{
    return (cfg->pid_remap_flag && cfg->parse_psi_flag &&
                cfg->dejitter_flag && !cfg->mpts_flag);
}

static inline
boolean session_is_passthru (in_config_t *cfg)
{
    return (!cfg->pid_remap_flag && cfg->parse_psi_flag &&
                cfg->dejitter_flag);
}

static inline
boolean session_is_data (in_config_t *cfg)
{
    return (!cfg->pid_remap_flag && !cfg->parse_psi_flag &&
                !cfg->dejitter_flag);
}

static inline
void session_set_init (uint8 *state)
{
    *state = (*state & ~SESSION_STATE_TRAFFIC_MASK) | SESSION_STATE_INIT;
}

static inline
void session_set_active (uint8 *state)
{
    *state = (*state & ~SESSION_STATE_TRAFFIC_MASK) | SESSION_STATE_ACTIVE;
}

static inline
void session_set_idle (uint8 *state)
{
    *state = (*state & ~SESSION_STATE_TRAFFIC_MASK) | SESSION_STATE_IDLE;
}

static inline
void session_set_off (uint8 *state)
{
    *state = (*state & ~SESSION_STATE_TRAFFIC_MASK) | SESSION_STATE_OFF;
}


// Initialize PAT info
//
static inline
void pat_info_init (pat_info_t* pat)
{
    memset(pat, 0, sizeof(pat_info_t));
    pat->ver = PSI_VER_UNKNOWN;
}


// Initialize PMT info
//
static inline
void pmt_info_init (pmt_info_t *pmt)
{
    memset(pmt, 0, sizeof(pmt_info_t));
}


static inline
pmt_es_info_t* pmt_info_get_es (pmt_info_t *pmt, int i)
{
    return (pmt_es_info_t*)(((uintptr_t)pmt->sect->sect_hdr) + pmt->es[i].offset);
}


static inline
void pmt_info_set_es (pmt_info_t *pmt, int i, pmt_es_info_t *es_info)
{
    pmt->es[i].offset = ((uintptr_t)es_info) - ((uintptr_t)pmt->sect->sect_hdr);
}


static inline
ca_descriptor_header_t* pmt_info_get_ca_desc (pmt_info_t *pmt, int i)
{
    return (ca_descriptor_header_t*)(((uintptr_t)pmt->sect->sect_hdr)
                                     + pmt->ca_desc[i].offset);
}


static inline
void pmt_info_set_ca_desc (pmt_info_t *pmt, int i,
                           ca_descriptor_header_t *ca_desc)
{
    pmt->ca_desc[i].processed_flag = 0;
    pmt->ca_desc[i].offset = ((uintptr_t)ca_desc) - ((uintptr_t)pmt->sect->sect_hdr);
}



// TP info buffer utilities
//

static inline
tp_info_t* tp_info_buffer_chunk_alloc (void)
{
    que_elem_t* elem = pool_alloc(&tp_info_buf_chunk_pool);
    return elem ? (tp_info_t*)(elem + 1) : NULL;
}


static inline
void tp_info_buffer_chunk_free (tp_info_t* chunk)
{
    pool_free(&tp_info_buf_chunk_pool, ((que_elem_t*)chunk) - 1);
}


// Get TP info from a specified index
static inline
tp_info_t* tp_info_get (const tp_info_buffer_t *buf, uint32 idx)
{
    return buf->chunk[idx >> TP_INFO_CHUNK_SHIFT] + (idx & TP_INFO_CHUNK_MASK);
}


// Get previous TP info from a specified index
static inline
tp_info_t* tp_info_get_prev (const tp_info_buffer_t *buf, uint32 idx)
{
    return tp_info_get(buf, (idx == 0 ? buf->size - 1 : idx - 1));
}


// Get next TP info from a specified index
static inline
tp_info_t* tp_info_get_next (const tp_info_buffer_t *buf, uint32 idx)
{
    return tp_info_get(buf, idx == buf->size - 1 ? 0 : idx + 1);
}


static inline
void tp_info_buffer_grow (tp_info_buffer_t *buf)
{
    tp_info_t* chunk = tp_info_buffer_chunk_alloc();
    if (chunk) {
        buf->chunk[buf->size >> TP_INFO_CHUNK_SHIFT] = chunk;
        buf->size += TP_INFO_CHUNK_SIZE;
    }
}


static inline
void tp_info_buffer_reset (tp_info_buffer_t *buf)
{
    int chunk_cnt = buf->size >> TP_INFO_CHUNK_SHIFT;
    buf->size = 0;
    while (--chunk_cnt >= 0) {
        tp_info_t *chunk = buf->chunk[chunk_cnt];
        buf->chunk[chunk_cnt] = NULL;
        tp_info_buffer_chunk_free(chunk);
    }
}


// TP info buffer reference utilities
//

static inline
void tp_info_ref_wraparound (tp_info_ref_t *ref)
{
    ref->idx = 0;
    ref->chunk = ref->buf->chunk[0];
    assert(ref->chunk != NULL);
}


static inline
void tp_info_ref_set_chunk (tp_info_ref_t *ref)
{
    ref->chunk = ref->buf->chunk[ref->idx >> TP_INFO_CHUNK_SHIFT];
}


static inline
void tp_info_ref_init (tp_info_ref_t *ref, tp_info_buffer_t *buf, uint32 idx)
{
    ref->buf = buf;
    ref->idx = idx;
    tp_info_ref_set_chunk(ref);
}


// Advance TP info buffer index
//   Returns TRUE if wrap around happens so produce can grow the buffer.
//   Otherwise returns FALSE.
static inline
boolean tp_info_ref_advance (tp_info_ref_t *ref)
{
    if ((++ref->idx & TP_INFO_CHUNK_MASK) == 0) {
        if (ref->idx == ref->buf->size) {
            return TRUE;
        } else {
            tp_info_ref_set_chunk(ref);
        }
    }
    return FALSE;
}


static inline
tp_info_t* tp_info_ref_get (const tp_info_ref_t *ref)
{
    return &ref->chunk[ref->idx & TP_INFO_CHUNK_MASK];
}



// Check whether the value of x is within the specified number of bits
// Returns 0 if x lies within the specified number of bits.
//
#if DEBUG_FP_MATH
#define check_range(x, nbits)                                             \
    if ( ((x) >= 0? (x) : -(x)) & (~((((uint64)1) << (nbits)) - 1)) ) {   \
        VIDEO_LOG("Range violation in %s:%d (%016llx)\n",                 \
                  __FILE__, __LINE__, (int64)(x));                        \
    }

#else
#define check_range(x, nbit)            ((void)0)
#endif


// Set mpeg_clock_t
//
static inline
void mpeg_clock_set (mpeg_clock_t *c, uint32 base, uint16 ext)
{
    c->base = base;
    c->ext = ext;
}


// Set mpeg_time_t based on base and ext values
//
static inline
mpeg_time_t mpeg_time_set (uint32 base, uint16 ext)
{
    return (((int64)base) * SCALE_BASE_EXT + ext) << FRAC_BITS_TIME;
}


// Print mpeg_clock_t
//
static inline
void print_mpeg_clock (FILE *fp, mpeg_clock_t *c)
{
    fprintf(fp, "%08x:%d", c->base, c->ext);
}


// Convert mpeg_clock_t to mpeg_time_t
//
static inline
mpeg_time_t mpeg_clock_to_time (mpeg_clock_t *c)
{
    return mpeg_time_set(c->base, c->ext);
}


// Normalize mpeg_time_t
//
static inline
void mpeg_time_normalize (mpeg_time_t *t)
{
// int64 orig = *t;
    if (*t >= MPEG_TIME_RANGE) {
        *t -= MPEG_TIME_RANGE;

    } else if (*t < 0) {
        *t += MPEG_TIME_RANGE;
    }
// if (orig != *t) {
//   VIDEO_DEBUG("\nTime_norm: %lld -> %lld\n", orig, *t); 
// }
}


// Normalize mpeg_timediff_t
//
static inline
void mpeg_timediff_normalize (mpeg_timediff_t *td)
{
// int64 orig = *td;
    if (*td >= MPEG_TIME_RANGE / 2) {
        *td -= MPEG_TIME_RANGE;

    } else if (*td < -MPEG_TIME_RANGE / 2) {
        *td += MPEG_TIME_RANGE;
    }
}


// Convert mpeg_time_t to mpeg_clock_t
//
static inline
void mpeg_time_to_clock (mpeg_time_t t, mpeg_clock_t *c)
{
    t >>= FRAC_BITS_TIME;
    c->base = (uint32)(t / SCALE_BASE_EXT);
    c->ext = (int16)(t - c->base * SCALE_BASE_EXT);
    if (c->ext < 0) {
        c->base--;
        c->ext += SCALE_BASE_EXT;
    }
}


// Convert mpeg_time_t to base part (in 45 kHz ticks)
//
static inline
uint32 mpeg_time_to_base (mpeg_time_t t)
{
    return (uint32)((t >> FRAC_BITS_TIME) / SCALE_BASE_EXT);
}


// Convert mpeg_time_t to millisecond
//
static inline
uint32 mpeg_time_to_ms (mpeg_time_t t)
{
    uint32 ms = mpeg_time_to_base(t);
    return ((ms + 23) / 45);
}


// Convert millisecond to mpeg_time_t
//
static inline
mpeg_time_t mpeg_ms_to_time (uint32 ms)
{
    return (((int64)ms) * SCALE_MS_BASE * SCALE_BASE_EXT) << FRAC_BITS_TIME;
}


static inline
void print_ca_desc (ca_descriptor_header_t *desc)
{
    VIDEO_MSG_DEBUG("CA desc: CA_sys_ID %04x, CA_PID %04x\n",
                    ca_desc_get_sys_id(desc), ca_desc_get_pid(desc));
}


// For ring buffer index manipulation
//
static inline
int16 ring_mod (int16 idx, int16 size)
{
   return (idx + size) % size;
}


static inline
int32 ring_mod32 (int32 idx, int32 size)
{
   return (idx + size) % size;
}


// Copy CRC from a PSI section to the psi_seciton_t struct
//
static inline
void psi_section_get_crc (psi_section_t *sect)
{
    psi_section_header_t* hdr = sect->sect_hdr;
    memcpy(&sect->crc, ((uint8*)hdr) + psi_get_section_size(hdr) - CRC_SIZE,
           CRC_SIZE);
}


// Copy CRC from the psi_section_t struct to the PSI section
//
static inline
void psi_section_set_crc (psi_section_t *sect, uint32 crc)
{
    psi_section_header_t* hdr = sect->sect_hdr;
    memcpy(((uint8*)hdr) + psi_get_section_size(hdr) - CRC_SIZE,
           &crc, CRC_SIZE);
}



// Set up out_pid_info_t
// TODO: check all calls to make sure all fields are set correctly!!
#define NO_CHANGE               0xFFFF

static inline
void out_pid_info_set (out_pid_info_t *info, uint16 pid_reversed,
                       uint16 prog_reversed, uint16 pid)
{
    if (pid_reversed != NO_CHANGE)   info->pid_reversed = pid_reversed;
    if (prog_reversed != NO_CHANGE)  info->prog_reversed = prog_reversed;
    if (pid != INVALID_PID)          info->pid = pid;
    info->reversed = info->pid_reversed | info->prog_reversed;
}


// Set up qam_pid_info_t
//
// Note: for turning off (used == 0), change is only made IF out_ses_id match!
//       Caller is expected to pass it its out_ses_id even when it is trying
//       to unregister the pid from the qam!
//
static inline
void qam_pid_info_set (qam_pid_info_t *info, uint16 used, uint16 forwarded,
                       uint16 out_ses_id, uint16 cw_idx)
{
    if (used || info->out_ses_id == out_ses_id) {
        if (used != NO_CHANGE)       info->used = used;
        if (forwarded != NO_CHANGE)  info->forwarded = forwarded;
        if (out_ses_id != NO_CHANGE) info->out_ses_id = out_ses_id;
        if (cw_idx != NO_CHANGE)     info->cw_idx = cw_idx;
    }
}


// Set up qam_prog_info_t
static inline
void qam_prog_info_set (qam_prog_info_t *info, uint16 used, uint16 out_prog_id)
{
    info->used = used;
    info->out_prog_id = out_prog_id;
}


static inline
void video_list_set (uint16 value, uint16 *list, uint8 idx)
{
    list[idx] = value;
}


// Set up output program for inclusion
static inline
void out_prog_set_include (out_prog_t *prog, out_session_t *ses, uint8 cfg_idx)
{
    prog->filtered = 0;
    prog->cfg_idx = cfg_idx;
    prog->cfg_changed = 1;
    ses->include_prog_flag = 1;
}


// Set up output program for exclusion
static inline
void out_prog_set_exclude (out_prog_t *prog, out_session_t *ses, uint8 cfg_idx)
{
    prog->cfg_idx = cfg_idx;
    prog->cfg_changed = 1;
    ses->exclude_prog_flag = 1;
}


// Set up out program for session/qam registration
static inline
void out_prog_set_register (out_prog_t *prog, int ses_reg, int qam_reg)
{
    prog->ses_reg_prog = prog->ses_reg_pid = ses_reg;
    prog->qam_reg_prog = prog->qam_reg_pid = qam_reg;
}

extern int get_out_sess_id(uint32 rid, uint16 *out_sess_id);

#endif /* __VIDEO_UTIL_H__ */

