/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *   This file implement the VBR version of clock recovery
 *   that can handle both SPTS and MPTS
 *
 */

/**************************************************************************

The simplified clock recovery algorithm is as follows:

  For 1st PCR:
    // PCR scheduling
    pcr_out_time = arvl_time + delay

    // non-PCR TP scheduling
    tp_itvl set using allocate bitrate

    // Output TBO calculation
    tbo_est = in_pcr - arvl_time
    out_tbo_adj = delay
    out_tbo = tbo_est - out_tbo_adj

    // Output PCR verification
    out_pcr = pcr_out_time + out_tbo
            = (arvl_time + delay) + (in_pcr - arvl_time - delay)
            = in_pcr


  For subsequent PCRs (unified mode), or master PCRs (master-slave mode):
    // PCR scheduling
    drift = pcr_inc * freq_adj
    pcr_out_time += pcr_inc - drift

    // non-PCR TP scheduling
    pcr_out_time_inc = pcr_out_time - prev_inter_pcr_out_time
    tp_idx_inc = tp_idx - prev_inter_pcr_tp_idx
    tp_itvl = pcr_out_time_inc / tp_idx_inc

    // Clock adjustment
    tbo_est += drift
    tbo_in = pcr_in - arvl_time
    tbo_err_norm = (tbo_in - tbo_est) / pcr_inc
    tbo_err_lpf = LPF(tbo_err_norm)
    freq_adj = clamp(tbo_err_lpf * gain)

    // Output TBO calculation
    out_tbo = tbo_est - out_tbo_adj

    // Output PCR verification
    out_pcr = pcr_out_time + out_tbo
             = pcr_out_time + tbo_est - out_tbo_adj
    out_pcr_inc = pcr_out_time_inc + tbo_est_inc
                 = pcr_inc - drift + drift
                 = pcr_inc


  For slave PCRs (master-slave mode):
    Slave PCR correction:
      out_tbo = out_pcr - out_time
              = in_pcr - out_time
              = (in_pcr - anchor_out_time) - (out_time - anchor_out_time)
              = (in_pcr - anchor_out_time) - tp_itvl * (tp_idx - anchor_tp_idx)
      anchor_out_time: delivery time of previous master PCR
      Input processing will save (in_pcr - anchor_out_time)
          in tbo_base & tbo_ext in pcr_info.
      Output processing will do the rest to compute out_tbo.


----------------------------------------------------------------------
Practical implementation:
    // PCR, times, and TBOs are in ext F42.20
    //
    // In input:
    //   - drift is accumulated in ext F.44
    //   - out_time's are in ext F.20
    //   - tp_itvl is in base F10.20
    //
    // In output:
    //   - out_time is in base F.20
    //


    // PCR scheduling
    //
    pcr_inc: in ext F24.20
             24 integer bits => about 1 sec max
    freq_adj: in ratio (F-12.44), max ~240 ppm!

    For drift up to 1 sec, we need ~24 integer bits
    drift: accumulated in ext F24.40
    drift_quant: quantized drift, in ext F24.20

        ----------------
        new_drift = (pcr_inc * freq_adj) >> 24                  // ext F12.40
                     F24.20    F-12.44
        drift += new_drift                                      // ext F24.40
        drift_quant = drift >> 20                               // ext F24.20
        ----------------


    // non-PCR TP scheduling
    //
    tp_idx_inc: for worst case of 52 Mbps, 100 ms PCR interval,
                we have ~3500 TPs between.  3500 * 600 => 2^21.
    tp_itvl: in base F10.20
             this can support session as low rate as 60 kbps.

        ----------------
        pcr_out_time += pcr_inc - drift_quant                   // ext F.20
        anchor_out_time_inc = pcr_out_time - anchor_out_time    // ext F.20
        tp_idx_inc = tp_idx - anchor_tp_idx
        tp_itvl = anchor_out_time_inc / (tp_idx_inc * 600)      // base F.20
        ----------------


    // Clock adjustment
    tbo_in = pcr_in - arvl_time
    tbo_est += drift_quant
    tbo_err = tbo_in - tbo_est

        ----------------
        tbo_in = pcr_in - arvl_time                             // ext F.20
        tbo_est += drift_quant                                  // ext F.20
        tbo_err_norm = (tbo_in - tbo_est) / (pcr_inc >> 20)     // F.20
        ----------------


    // Update freq adj
    tbo_err_lpf = LPF(tbo_err_norm)
    freq_adj = clamp(tbo_err_lpf * gain)

        ----------------
        tbo_err_diff = tbo_err_norm - tbo_err_lpf               // ext F.20
        tbo_err_lpf += (tbo_err_diff * lpf_par) >> 24           // ext F.20
                        F.20           F0.24
        freq_adj = tbo_err_lpf * loop_gain                      // F.44
                    F.20         F.24
        ----------------

**************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "video.h"
#include "video_util.h"


#define MAX_PCR_INC                     ((int64)(EXT_FREQ / 2) << 20)
                                        // 0.5 sec in 27 MHz ticks

#define LPF_PAR_DEFAULT                 168
                                        // 0.00001 in F.24

#define LOOP_GAIN_DEFAULT               34
                                        // 0.000002 in F.24

#define REVIEW_PERIOD_DEFAULT           10
#define DRIFT_THRES_DEFAULT             (15 * SCALE_MS_BASE)
#define ADJUST_THRES_DEFAULT            (3 * SCALE_MS_BASE)

#define MAX_FREQ_ADJ_PPM                30

#define MAX_SLEW_PPM_PER_HR             10

#define SCALE_BITS_AVG                  8

#define JITTER_SHRINK                   (1 << FRAC_BITS_TIME)

#define SIGN_64                         0x8000000000000000LL
#define MASK_LO                         0x00000000FFFFFFFFLL


// Global variables
clkrec_par_t clkrec_par;               // clock recovery parameters
mpeg_clock_t sys_clk;                  // system clock

static mpeg_time_t PCR_RANGE
            = ((uint64)SCALE_BASE_EXT) << (FRAC_BITS_TIME + 32);


// Add two 64-bit integers with carry-in
// Return the sum, and update carry with carry-out
//
static inline
int64 add64_w_carry (int64 x, int64 y, int* carry)
{
    int64 z = x + y + *carry;
    *carry = (((x & y) | ((x ^ y) & (z ^ SIGN_64))) >> 63) & 1;
    return z;
}


// Multiply two 64-bit integers to form a 128-bit result 
//
static
void mul64 (int64 x, int64 y, int64* hi, uint64* lo)
{
    int64 xh, xl, yh, yl, mid1, mid2;
    int c1, c2;

    xh = x >> 32;
    xl = x & MASK_LO;
    yh = y >> 32;
    yl = y & MASK_LO;

    *lo = xl * yl;
    mid1 = xl * yh;
    mid2 = xh * yl;
    *hi = xh * yh;

    c1 = c2 = 0;
    *lo = add64_w_carry(*lo, mid1 << 32, &c1);
    *lo = add64_w_carry(*lo, mid2 << 32, &c2);
    *hi = add64_w_carry(*hi, mid1 >> 32, &c1);
    *hi = add64_w_carry(*hi, mid2 >> 32, &c2);
}


// Right shift a 128-bit integer to get a 64-bit result
//   Note: pos must be > 0!
//
static
int64 rshift64 (int64 hi, uint64 lo, int pos)
{
    assert(pos > 0);

    if (pos < 64) {
        check_range(hi, pos);
        return (hi << (64 - pos)) | (lo >> pos);

    } else {
        check_range(hi, 128 - pos);
        return (hi >> (pos - 64));
    }
}


// Returns (x * y) >> pos
//   Note: pos must be > 0!
//
static inline
int64 mul_rshift64 (int64 x, int64 y, int pos)
{
    int64 hi;
    uint64 lo;
    mul64(x, y, &hi, &lo);
    return rshift64(hi, lo, pos);
}


// Check and handle buffer violation
//
static
boolean clkrec_check_buffer (in_session_t *ses, pcr_context_t *pcr_ctx)
{
    int adjust = 0;
    if (ses->stay_time < SYS_LATENCY * SCALE_MS_BASE) {
        adjust = -1;             // underflow
        ses->stat.underflow_cnt++;

    } else if (ses->stay_time > ses->stay_time_limit) {
        adjust = 1;            // overflow
        ses->stat.overflow_cnt++;
    }

    if (!adjust)  return FALSE;

    if (mpeg_time_to_ms(ses->jitter) > ses->cfg->jitter_size) {
        // Ignore suspicious jitter value
        ses->jitter = 0;
    }

    uint32 stay_time_oper = comp_stay_time_oper(ses->cfg->jitter_size);
    mpeg_timediff_t tb_adjust;

    if (adjust < 0) {
        tb_adjust = mpeg_time_set(stay_time_oper - ses->stay_time, 0)
                        - ses->jitter / 2;
        if (tb_adjust > 0) {
            pcr_ctx->out_time += tb_adjust;
        }
    } else {
        tb_adjust = mpeg_time_set(ses->stay_time - stay_time_oper, 0)
                        - ses->jitter / 2;
        if (tb_adjust > 0) {
            pcr_ctx->out_time -= tb_adjust;
        }
    }

    ses->jitter = 0;            // reset jitter
    return TRUE;
}


// Process the first PCR in a session
//   tbo_est = tbo_in
//   out_time = arvl_time + init_delay
//   out_tbo_adj = init_delay + hw_time_adj
//
static
void process_first_pcr (in_session_t *ses, pcr_context_t *pcr_ctx,
                        pcr_info_t *pcr_info, mpeg_time_t arvl_time,
                        mpeg_timediff_t tbo, mpeg_time_t out_time)
{
    pcr_info->info_type = PCR_INFO;
    pcr_info->valid_flag = 0;
    pcr_info->tp_itvl = ses->tp_itvl;    // default to allocated bitrate

    pcr_ctx->tbo_est = tbo;

    mpeg_time_normalize(&out_time);
    ses->stay_time = mpeg_time_to_base(out_time - arvl_time);
    ses->anchor_out_time = pcr_ctx->out_time = out_time;
    ses->out_tbo_adj = mpeg_time_set(ses->stay_time, 0) + hw_time_adj;

    // Save results to PCR info
    pcr_info->out_time = mpeg_time_to_base(pcr_ctx->out_time) & OUT_TIME_MASK;

    mpeg_clock_t tmp;
    mpeg_time_to_clock(pcr_ctx->tbo_est - ses->out_tbo_adj, &tmp);
    pcr_info->tbo_base = tmp.base;
    pcr_info->tbo_ext = tmp.ext;
    pcr_info->valid_flag = 1;

    pcr_ctx->flags |= PCR_CONTEXT_FLAG_LOCKED;
    ses->locked_pcr_cnt++;

    ses->anchor_tp_idx = ses->tp_idx;
    ses->anchor_info_idx = ses->tp_info_ref.idx;

    VIDEO_CLKREC_DEBUG("\n<FST>, DT %d", mpeg_time_to_base(pcr_ctx->out_time));
}


// Process a new PCR pid in a session
//
static
void process_new_pcr (in_session_t *ses, new_pcr_info_t *new_pcr_info,
                      mpeg_time_t arvl_time)
{
    mpeg_time_t out_time;
    new_pcr_info->info_type = NEW_PCR_INFO;
    new_pcr_info->link = INVALID_INFO_IDX;
    new_pcr_info->pcr_base = ses->pcr.base;
    new_pcr_info->pcr_ext = ses->pcr.ext;

    // Default out time based on CBR model
    out_time = ses->anchor_out_time
            + ((int64)SCALE_BASE_EXT) * ses->tp_itvl
              * (ses->tp_idx - ses->anchor_tp_idx);
    new_pcr_info->out_time = mpeg_time_to_base(out_time) & OUT_TIME_MASK;

    ses->stay_time = mpeg_time_to_base(out_time - arvl_time);
}


static
void link_new_pcr (in_session_t *ses, pcr_context_t *pcr_ctx,
                   new_pcr_info_t *new_pcr_info)
{
    new_pcr_info->pcr_ctx_idx = pcr_ctx - ses->pcr_ctx_tab;

    // NOTE: critical section
    // Add to new PCR chain
    if (ses->first_new_pcr == INVALID_INFO_IDX) {
        ses->first_new_pcr = ses->tp_info_ref.idx;

    } else {
        ((new_pcr_info_t*)tp_info_get(&ses->tp_info_buf, ses->last_new_pcr))
            ->link = ses->tp_info_ref.idx;
    }
    ses->last_new_pcr = ses->tp_info_ref.idx;
    // NOTE: End of critical section

    VIDEO_CLKREC_DEBUG("\n<NEW>, ctx %d", new_pcr_info->pcr_ctx_idx);
}


// Update new PCR info once the next anchor PCR is found
//
//   tbo_out = out_pcr - out_time
//           = in_pcr - out_time
//   tbo_est = tbo_out + out_tbo_adj
//   tbo_in = tbo_est
//
static
void update_new_pcr (in_session_t *ses)
{
    pcr_info_t *pcr_info;
    new_pcr_info_t *new_pcr_info;
    mpeg_timediff_t out_time_inc;

    uint16 info_idx = ses->first_new_pcr;

    if (info_idx != INVALID_INFO_IDX) {
        VIDEO_CLKREC_DEBUG("\nIn %d: update new PCR: anc %d, 1st %d, last %d",
                    ses->id, ses->anchor_info_idx, info_idx, ses->last_new_pcr);
    }

    // Update all new PCRs found after the anchor
    while (info_idx != INVALID_INFO_IDX) {
        mpeg_clock_t in_pcr;
        mpeg_time_t tbo_out;
        mpeg_clock_t tbo;
        pcr_context_t *pcr_ctx;

        tp_info_t* tp_info = tp_info_get_prev(&ses->tp_info_buf, info_idx);
        int16 tp_idx_inc = (int16)(tp_info->tp_idx - ses->anchor_tp_idx);

        new_pcr_info = (new_pcr_info_t*)tp_info_get(&ses->tp_info_buf,
                                                    info_idx);
        pcr_info = (pcr_info_t*)new_pcr_info;

        if (new_pcr_info->info_type != NEW_PCR_INFO) {
            uint32* ptr = (uint32*)new_pcr_info;
            VIDEO_DEBUG("\nIn %d: Bad newPCR at %d: %08x %08x %08x %08x",
                        ses->id, info_idx, *ptr, *(ptr+1), *(ptr+2), *(ptr+3));

            break;    // terminate the operation
        }

        pcr_ctx = &ses->pcr_ctx_tab[new_pcr_info->pcr_ctx_idx];
        info_idx = new_pcr_info->link;

        in_pcr.base = new_pcr_info->pcr_base;
        in_pcr.ext = new_pcr_info->pcr_ext;

        // Compute output time
        out_time_inc = ((int64)SCALE_BASE_EXT) * ses->tp_itvl * tp_idx_inc;
        pcr_ctx->out_time = ses->anchor_out_time + out_time_inc;
        pcr_info->out_time = mpeg_time_to_base(pcr_ctx->out_time)
                                 & OUT_TIME_MASK;

        // Compute TBO
        tbo_out = mpeg_time_set(new_pcr_info->pcr_base, new_pcr_info->pcr_ext)
                      - pcr_ctx->out_time - hw_time_adj;
        mpeg_time_to_clock(tbo_out, &tbo);

        // Convert new PCR info to PCR info
        pcr_info->info_type = PCR_INFO;
        pcr_info->tp_itvl = ses->tp_itvl;
        pcr_info->tbo_base = tbo.base;
        pcr_info->tbo_ext = tbo.ext;
        pcr_info->valid_flag = 1;

        // Update PCR context
        if (!(pcr_ctx->flags & PCR_CONTEXT_FLAG_LOCKED)) {
            pcr_ctx->flags |= PCR_CONTEXT_FLAG_LOCKED;
            ses->locked_pcr_cnt++;
        }
        pcr_ctx->tbo_est = tbo_out + ses->out_tbo_adj;
        mpeg_timediff_normalize(&pcr_ctx->tbo_est);

        VIDEO_CLKREC_DEBUG("\n<Update %d-%d: pid %d, TBO %d, DT %d>",
                ses->id, tp_info->tp_idx, pcr_ctx->pid, pcr_info->tbo_base,
                mpeg_time_to_base(pcr_ctx->out_time));
    }

    ses->first_new_pcr = ses->last_new_pcr = INVALID_INFO_IDX;
}


// Process a known PCR pid in a session
//   pcr_out_time = prev_intra_pcr_out_time + pcr_inc (+ drift)
//   tp_itvl = (pcr_out_time - anchor_out_time) / (pcr_tp_idx - anchor_tp_idx)
//
static
void process_locked_pcr (in_session_t *ses, pcr_context_t *pcr_ctx,
                         pcr_info_t *pcr_info, mpeg_time_t arvl_time,
                         mpeg_timediff_t tbo_in, mpeg_timediff_t pcr_inc)
{ 
    pcr_info_t *anchor_pcr_info;
    int32 anchor_tp_idx_inc;
    int64 anchor_out_time_inc;
    mpeg_clock_t tbo;
    int64 max_freq_adj_diff;
    int64 drift_quant;                  // ext F24.20
    int64 tbo_err_norm;                 // ext F.20
    int64 freq_adj;                     // F-12.44
    int64 tmp;
    uint64 tmp2;

    pcr_info->info_type = PCR_INFO;
    pcr_info->valid_flag = 0;

    // Update statistics
    if (pcr_inc > pcr_ctx->max_pcr_inc) {
        pcr_ctx->max_pcr_inc = pcr_inc;
    }

    // Update jitter estimate
    mpeg_timediff_t arvl_inc = arvl_time - pcr_ctx->prev_arvl_time;
    mpeg_timediff_normalize(&arvl_inc);
    mpeg_timediff_t jitter = arvl_inc - pcr_inc;
    if (jitter < 0)  jitter = -jitter; 
    if (jitter > ses->jitter)  ses->jitter = jitter;
    else ses->jitter -= JITTER_SHRINK;

    // Estimate clock drift
    //   drift += (pcr_inc * freq_adj) >> 24                    // ext F24.40
    //             F24.20    F-12.44
    //   drift_quant = drift >> 20                              // ext F24.20
    //
    pcr_ctx->drift += mul_rshift64(pcr_inc, pcr_ctx->freq_adj, 24);
    drift_quant = pcr_ctx->drift >> 20;
    pcr_ctx->drift &= 0xFFFFF;                                  // ext F-20.40

    // Compute PCR out time (using intra PCR)
    pcr_ctx->out_time += pcr_inc - drift_quant;
    mpeg_time_normalize(&pcr_ctx->out_time);

    // Check TP stay time
    ses->stay_time = mpeg_time_to_base(pcr_ctx->out_time - arvl_time);

    if (clkrec_check_buffer(ses, pcr_ctx)) {

        // Buffer violation handling
        VIDEO_CLKREC_DEBUG(", AT %d", mpeg_time_to_base(arvl_time));

        if (ses->anchor_info_idx != INVALID_INFO_IDX) {
            // Update previous anchor's valid flag
            anchor_pcr_info = (pcr_info_t*)tp_info_get(&ses->tp_info_buf,
                                                       ses->anchor_info_idx);
            anchor_pcr_info->valid_flag = 1;

            // Update all new PCRs from previous anchor
            update_new_pcr(ses);
        }

        // Mark all PCR pids as unlocked
        pcr_context_unlock(ses);

        // Handle current PCR as 1st PCR
        process_first_pcr(ses, pcr_ctx, pcr_info, arvl_time, pcr_ctx->tbo_est,
                          pcr_ctx->out_time);
        ses->flags |= SESSION_FLAG_SET_DISC;    // insert discountinuity point

        return;
    }        

    // Set out time in pcr_info
    pcr_info->out_time = mpeg_time_to_base(pcr_ctx->out_time) & OUT_TIME_MASK;

    // Compute intra PCR TP interval
    int32 intra_tp_idx_inc = (int32)(ses->tp_idx - pcr_ctx->tp_idx);
    pcr_ctx->tp_itvl = (pcr_inc - drift_quant) / (intra_tp_idx_inc * 600);
    pcr_ctx->flags |= PCR_CONTEXT_FLAG_BITRATE_KNOWN;

    // Compute inter PCR TP interval
    //   anchor_out_time_inc = pcr_out_time - anchor_out_time
    //   anchor_tp_idx_inc = pcr_tp_idx - anchor_tp_idx;
    //   tp_itvl = anchor_out_time_inc / anchor_tp_idx_inc  (F.20)
    //
    anchor_tp_idx_inc = (int32)(ses->tp_idx - ses->anchor_tp_idx);
    anchor_out_time_inc = pcr_ctx->out_time - ses->anchor_out_time; // F.20
    mpeg_timediff_normalize(&anchor_out_time_inc);

    if (anchor_out_time_inc > 0) {
        ses->tp_itvl = anchor_out_time_inc
                       / (anchor_tp_idx_inc * SCALE_BASE_EXT);
                                                        // base F.20
    } else {
        ses->tp_itvl = 0;
        ses->stat.pcr_xover_cnt++;
    }

    pcr_info->tp_itvl = ses->tp_itvl;    // Initial guess

    // Update previous anchor with correct bitrate
    assert(ses->anchor_info_idx < ses->tp_info_buf.size);
    anchor_pcr_info = (pcr_info_t*)tp_info_get(&ses->tp_info_buf,
                                               ses->anchor_info_idx);
    if (anchor_pcr_info->info_type != PCR_INFO) {
        VIDEO_CLKREC_DEBUG("\nIn %d: ** anc overwritten: cur %d, anc %d",
                ses->id, ses->tp_info_ref.idx-1, ses->anchor_info_idx);
        VIDEO_LOG("In %d: anchor overwritten", ses->id);

    } else {
        anchor_pcr_info->tp_itvl = ses->tp_itvl;
        anchor_pcr_info->valid_flag = 1;

        // Update all new PCRs from previous anchor
        update_new_pcr(ses);
    }

    // Collect bitrate statistics
    ses->avg_tp_itvl += (pcr_ctx->tp_itvl - ses->avg_tp_itvl) >> SCALE_BITS_AVG;
    if (pcr_ctx->tp_itvl < ses->min_tp_itvl) {
        ses->min_tp_itvl = pcr_ctx->tp_itvl;
    }
    if (pcr_ctx->tp_itvl > ses->max_tp_itvl) {
        ses->max_tp_itvl = pcr_ctx->tp_itvl;
    }

    // TBO calculation
    //   tbo_est += drift_quant
    //
    pcr_ctx->tbo_est += drift_quant;                                // F.20
    mpeg_timediff_normalize(&pcr_ctx->tbo_est);

    // Compute output TBO
    mpeg_time_to_clock(pcr_ctx->tbo_est - ses->out_tbo_adj, &tbo);

    pcr_info->tbo_base = tbo.base;
    pcr_info->tbo_ext = tbo.ext;

    // Update anchor
    ses->anchor_tp_idx = ses->tp_idx;
    if (anchor_out_time_inc > 0) {
        ses->anchor_out_time = pcr_ctx->out_time;
    }
    ses->anchor_info_idx = ses->tp_info_ref.idx;
    ses->anchor_arvl = ses->perf.now;

    if ((pcr_inc >> 20) == 0) {
        goto skip;
    }

    // Clock adjustment
    //   tbo_err_norm = (tbo_in - tbo_est) / pcr_inc
    //   tbo_err_lpf = LPF(tbo_err_norm)
    //   freq_adj = clamp(tbo_err_lpf * loop_gain)
    //
    tbo_err_norm = (tbo_in - pcr_ctx->tbo_est) / (pcr_inc >> 20);   // F.20

    // LPF
    //   tbo_err_diff = tbo_err_norm - tbo_err_lpf
    //   tbo_err_lpf += tbo_err_diff * lpf_par
    //                  F.20           F0.24
    //
    tmp = tbo_err_norm - pcr_ctx->tbo_err_lpf;                      // F.20
    pcr_ctx->tbo_err_lpf += mul_rshift64(tmp, clkrec_par.lpf_par, 24);
                                                                    // F.20

    //   freq_adj = tbo_err_lpf * loop_gain                         // F.44
    //              F.20          F.24
    //tmp2 = (uint64)freq_adj;
    mul64(pcr_ctx->tbo_err_lpf, clkrec_par.loop_gain, &tmp, &tmp2);
    freq_adj = tmp2;


    // Compute slew rate
    //     max_freq_adj_diff = pcr_inc * max_slew                   // F.20
    //                         F.20      F.44
#if COMP_DRIFT_RATE
#define DRIFT_CONST                             3599792519831
    int64 prev_freq_adj = pcr_ctx->freq_adj;
#endif

    max_freq_adj_diff = (pcr_inc * clkrec_par.max_slew) >> 44;

    tmp = freq_adj - pcr_ctx->freq_adj;
    if (tmp > max_freq_adj_diff) {
        tmp = max_freq_adj_diff;
        VIDEO_CLKREC_DEBUG(", S-CLP");
    } else if (tmp < -max_freq_adj_diff) {
        tmp = -max_freq_adj_diff;
        VIDEO_CLKREC_DEBUG(", S-CLP");
    }
    pcr_ctx->freq_adj += tmp;

    // Clamp for clock accuracy
    if (pcr_ctx->freq_adj > clkrec_par.max_freq_adj) {
        pcr_ctx->freq_adj = clkrec_par.max_freq_adj;
        VIDEO_CLKREC_DEBUG(", F-CLP");
    } else if (pcr_ctx->freq_adj < -clkrec_par.max_freq_adj) {
        pcr_ctx->freq_adj = -clkrec_par.max_freq_adj;
        VIDEO_CLKREC_DEBUG(", F-CLP");
    }

#if COMP_DRIFT_RATE
    // For debugging only
    ses->drift_rate = (pcr_ctx->freq_adj - prev_freq_adj)
                          * DRIFT_CONST / pcr_inc;
#endif

skip:
    VIDEO_CLKREC_DEBUG(", clk %lld", (pcr_ctx->freq_adj * 1953125) >> 35);
}


static
void process_slave_pcr (in_session_t *ses, pcr_info_t *pcr_info)
{
    mpeg_clock_t tbo;
    pcr_info->info_type = SLAVE_PCR_INFO;
    mpeg_time_t pcr_time = mpeg_clock_to_time(&ses->pcr);
    mpeg_timediff_t tbo_out = pcr_time - ses->anchor_out_time;
    mpeg_time_to_clock(tbo_out, &tbo);
    pcr_info->tbo_base = tbo.base;
    pcr_info->tbo_ext = tbo.ext;
}


void process_pcr_tp (in_session_t *ses, uint16 pid, int disc_flag)
{
    mpeg_time_t arvl_time;                 // arrival time (ext F42.0)
    mpeg_time_t pcr_time;                  // input PCR (ext F42.0)
    mpeg_timediff_t tbo_in;                // input TBO (ext F.20)
    mpeg_timediff_t pcr_inc = 0LL;

    pcr_time = mpeg_clock_to_time(&ses->pcr);
    arvl_time = mpeg_time_set(ses->arvl_time, 0);

    // tbo_in = pcr_time - arvl_time
    tbo_in = pcr_time - arvl_time;
    mpeg_timediff_normalize(&tbo_in);

    VIDEO_CLKREC_DEBUG("\nIn %d-%d: info %d-%d, pid %d, AT %d, PCR %u",
            ses->id, ses->tp_idx, ses->tp_info_usage_id, ses->tp_info_ref.idx,
            pid, mpeg_time_to_base(arvl_time), ses->pcr.base);

    advance_input_tp_info_wr(ses);

    tp_info_t* info = tp_info_ref_get(&ses->tp_info_ref);
    pcr_context_t* pcr_ctx;

    if (ses->cfg->ms_cr_flag) {
        // Master-slave mode
        pcr_ctx = &ses->pcr_ctx_tab[0];

        if (ses->pcr_ctx_cnt == 0) {    // master PCR not found yet
            // Use this pid as master PCR pid
            memset(pcr_ctx, 0, sizeof(pcr_context_t));
            pcr_ctx->pid = pid;
            ses->pcr_ctx_cnt = 1;
        }

        if (pid != pcr_ctx->pid) {
            if (pid == ses->cfg->master_pcr_pid) {
                // Switch to use preferred master PCR pid
                // TODO: maybe a smoother transition!?
                memset(pcr_ctx, 0, sizeof(pcr_context_t));
                pcr_ctx->pid = pid;
                ses->locked_pcr_cnt = 0;

            } else {
                // Slave PCR processing
                process_slave_pcr(ses, (pcr_info_t*)info);
                return;
            }
        }

    } else {
        // Unified mode
        pcr_ctx = pcr_context_lookup(ses, pid);
        if (!pcr_ctx) {
            // New PCR pid
            pcr_ctx = pcr_context_add(ses);
            if (!pcr_ctx) {
                // PCR context table full.  Treat TP as non-PCR TP
                process_new_pcr(ses, (new_pcr_info_t*)info, arvl_time);
                return;
            }
            pcr_ctx->pid = pid;
        }
    }

    if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_LOCKED)) {
        if (disc_flag) {
            VIDEO_CLKREC_DEBUG(", <disc>");
            pcr_ctx->flags &= ~PCR_CONTEXT_FLAG_LOCKED;
            ses->locked_pcr_cnt--;

        } else {
            // Compute PCR increment
            //   pcr_inc = pcr - prev_pcr
            //
            pcr_inc = pcr_time - pcr_ctx->pcr_time;

            // Handle for PCR wraparound
            if (pcr_inc < -PCR_RANGE/2) {
                pcr_inc += PCR_RANGE;
            }

            if (pcr_inc < 0 || pcr_inc > MAX_PCR_INC) {
                // Illegal PCR jump
                pcr_ctx->flags &= ~PCR_CONTEXT_FLAG_LOCKED;
                ses->locked_pcr_cnt--;
                ses->stat.pcr_jump_cnt++;
                VIDEO_CLKREC_DEBUG(", <jump, lck_cnt %d>", ses->locked_pcr_cnt);

            } else if ((ses->flags & SESSION_FLAG_CBR)
                       && !ses->cfg->ms_cr_flag) {
                // Check bitrate based on CBR model
                int32 tp_idx_inc = (int32)(ses->tp_idx - pcr_ctx->tp_idx);
                int32 tp_itvl = (int32)(pcr_inc / (tp_idx_inc*SCALE_BASE_EXT));

                if ((tp_itvl < ses->tp_itvl_lower) ||
                        (tp_itvl > ses->tp_itvl_upper)) {
                    // Bad PCR bitrate
                    pcr_ctx->flags &= ~PCR_CONTEXT_FLAG_LOCKED;
                    ses->locked_pcr_cnt--;
                    ses->stat.pcr_jump_cnt++;
                    VIDEO_CLKREC_DEBUG(", <bad pcr, lck_cnt %d>",
                                       ses->locked_pcr_cnt);
                }
            }
        }
    }

    if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_LOCKED)) {
        process_locked_pcr(ses, pcr_ctx, (pcr_info_t*)info, arvl_time, tbo_in,
                           pcr_inc);

    } else {
        if (ses->locked_pcr_cnt == 0) {
            // First PCR
            ses->stay_time = comp_stay_time_oper(ses->cfg->jitter_size);

            process_first_pcr(ses, pcr_ctx, (pcr_info_t*)info, arvl_time,
                       tbo_in, arvl_time + mpeg_time_set(ses->stay_time, 0));

        } else {
            // New PCR
            process_new_pcr(ses, (new_pcr_info_t*)info, arvl_time);
            link_new_pcr(ses, pcr_ctx, (new_pcr_info_t*)info);
        }

        ses->flags |= SESSION_FLAG_SET_DISC;    // insert disc point
    }

    // Update PCR context
    pcr_ctx->last_seen = (ses->perf.now >> 8) & 0xFF;
    pcr_ctx->tp_idx = ses->tp_idx;
    pcr_ctx->pcr_time = pcr_time;
    pcr_ctx->prev_arvl_time = arvl_time;

    if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_ADJUSTED)) {
        ses->flags |= SESSION_FLAG_SET_DISC;    // insert disc point
        pcr_ctx->flags &= ~PCR_CONTEXT_FLAG_ADJUSTED;
    }

    VIDEO_CLKREC_DEBUG(", TBO %d, DT %d, stay %d, BR %d",
            mpeg_time_to_base(pcr_ctx->tbo_est),
            mpeg_time_to_base(pcr_ctx->out_time), ses->stay_time/SCALE_MS_BASE,
            tp_itvl_to_bitrate(ses->tp_itvl));
}


void clkrec_config (void)
{
    clkrec_par.lpf_par = LPF_PAR_DEFAULT;
    clkrec_par.loop_gain = LOOP_GAIN_DEFAULT;
    clkrec_par.max_freq_adj = MAX_FREQ_ADJ_PPM * SCALE_FREQ_ADJ;
    clkrec_par.max_slew = MAX_SLEW_PPM_PER_HR * SLEW_CONST;

    clkrec_par.review_period = REVIEW_PERIOD_DEFAULT;
    clkrec_par.drift_thres = DRIFT_THRES_DEFAULT;
    clkrec_par.adjust_thres = ADJUST_THRES_DEFAULT;
}


// PLL review to control clock drift
//
void pll_review (in_session_t *ses)
{
    int i;
    int32 min_lead = 0x7FFFFFFF;
    int32 max_lead = 0x80000000;
    int32 avg_lead = 0;
    int pcr_ctx_cnt = 0;
    int32 lead[MAX_PCR_CONTEXT_MPTS];
    uint32 now = sys_clk.base;
    uint32 adj_pcr_cnt = 0;
    pcr_context_t* pcr_ctx = ses->pcr_ctx_tab;

    VIDEO_CLKREC_DEBUG("\n\nPLL review: IN-%d", ses->id);
    for (i=0; i<ses->pcr_ctx_cnt; i++, pcr_ctx++) {
        if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_BITRATE_KNOWN)) {
            int32 tp_idx_inc = (int32)(ses->tp_idx - pcr_ctx->tp_idx);
            int64 pcr_inc = ((int64)pcr_ctx->tp_itvl) * tp_idx_inc * 600;
            uint32 out_time = mpeg_time_to_base(pcr_ctx->out_time + pcr_inc);
                                                            // ignore drift
            lead[i] = (int32)(out_time - now);
            avg_lead += lead[i];
            if (lead[i] > max_lead)  max_lead = lead[i];
            if (lead[i] < min_lead)  min_lead = lead[i];
            pcr_ctx_cnt++;
            VIDEO_CLKREC_DEBUG("\n Pid %d: lead %d",
                               pcr_ctx->pid, lead[i]);
        }
    }

    if (pcr_ctx_cnt == 0) {
        return;       // no valid PCR contexts
    }

    avg_lead /= pcr_ctx_cnt;

    VIDEO_CLKREC_DEBUG("\nSummary: TP %d, lead %d - %d, avg %d, drift %d\n",
           ses->tp_idx, min_lead, max_lead, avg_lead, max_lead - min_lead);

    if (max_lead - min_lead <= clkrec_par.drift_thres) {
        return;
    }

    // Drift adjustment needed
    min_lead = avg_lead - clkrec_par.adjust_thres;
    max_lead = avg_lead + clkrec_par.adjust_thres;

    VIDEO_CLKREC_DEBUG("\nPIDs adjusted:");
    //ses->stat.tot_drift_cnt++;
    pcr_ctx = ses->pcr_ctx_tab;
    for (i=0; i<ses->pcr_ctx_cnt; i++, pcr_ctx++) {
        if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_BITRATE_KNOWN) &&
                (lead[i] < min_lead || lead[i] > max_lead)) {
            mpeg_timediff_t adj = lead[i] - avg_lead;
            adj = (adj * SCALE_BASE_EXT) << FRAC_BITS_TIME;
            pcr_context_adjust_tbo(pcr_ctx, adj);
            adj_pcr_cnt++;
            VIDEO_CLKREC_DEBUG(" %d", pcr_ctx->pid);
        }
    }
    //ses->stat.adj_pcr_cnt += adj_pcr_cnt;
    VIDEO_CLKREC_DEBUG(": Total %d pids\n", adj_pcr_cnt);
}

