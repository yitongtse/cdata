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
#include "video.h"
#include "video_util.h"
#include "video_cli.h"
#include "video_event.h"


#define MAX_TP_PER_IP                   7


static inline
boolean is_rtp_header (rtp_header_t *hdr)
{
    return ((hdr->version == RTP_VERSION)
            && (hdr->payload_type == RTP_PAYLOAD_TYPE_MPEG2));
}


// Return the size of RTP header if present
// Otherwise return 0
//
int get_rtp_header_size (uintptr_t addr)
{
    int hdr_len;
    rtp_header_t *hdr = (rtp_header_t*)addr;

    if (!is_rtp_header(hdr)) {
        return 0;
    }

    hdr_len = sizeof(rtp_header_t) + hdr->csrc_cnt * sizeof(uint32);

    // Check if there is extension for the RTP header
    if (hdr->ext) {
        rtp_ext_header_t* ext_hdr = (rtp_ext_header_t*)(addr + hdr_len);
        hdr_len += sizeof(rtp_ext_header_t) + ext_hdr->length * sizeof(uint32);
    }
    return hdr_len;
}


// Update session's average bitrate
//
void update_avg_bitrate (in_session_t *in_ses)
{
    int32 time_elapsed
            = (int32)(in_ses->perf.now - in_ses->prev_bitrate_update_time);
    if (time_elapsed > 0) {
        int32 tp_proc = (int32)(in_ses->stat.tp_cnt - in_ses->prev_tp_cnt);
        uint32 bitrate = ((int64)tp_proc * TP_SIZE_BIT * BASE_FREQ)
                                 / ((int64) time_elapsed);
        in_ses->avg_bitrate += ((int32)bitrate - (int32)in_ses->avg_bitrate)
                                       >> 6;

        if (bitrate < in_ses->min_bitrate) {
            in_ses->min_bitrate = bitrate;
        }
        if (bitrate > in_ses->max_bitrate) {
            in_ses->max_bitrate = bitrate;
        }

        // Adjust tp_itvl upper bound for bad PCR detection
        in_ses->tp_itvl_upper = tp_bitrate_to_itvl(in_ses->avg_bitrate)
                                        * 21 / 20;      // relaxed by 5%
    }

    // update stats for next time
    in_ses->prev_bitrate_update_time = in_ses->perf.now;
    in_ses->prev_tp_cnt = in_ses->stat.tp_cnt; 
}


// Check if the session's PCR disappears
//
boolean check_session_pcr_missing (in_session_t *ses)
{
    if (session_is_active(ses->state) &&
            (ses->anchor_info_idx != INVALID_INFO_IDX)) {
        int32 time_diff = (int32)(sys_clk.base - ses->anchor_arvl);

        if (time_diff >= MAX_PCR_ITVL) {
            VIDEO_EVENT_DEBUG("\nIn %d: PCR missing for %d ms",
                              ses->id, time_diff / SCALE_MS_BASE);
            return TRUE;
        }
    }
    return FALSE;
}


// Check if the session becomes idle
//
static inline
boolean check_session_idle (in_session_t *ses)
{
    int32 idle_time = (int32)(sys_clk.base - ses->prev_tp_arvl);
    return (session_is_init(ses->state)
               && (idle_time >= (int32)ses->init_thres))
           || (session_is_active(ses->state)
               && (idle_time >= (int32)ses->idle_thres));
}


// Check if the session becomes off
//
static inline
boolean check_session_off (in_session_t *ses)
{
    int32 idle_time = (int32)(sys_clk.base - ses->prev_tp_arvl);
    return (session_is_idle(ses->state) && (idle_time >= ses->off_timer));
}


// Clean up PCR context table if PCR disappears
//
void pcr_context_cleanup (in_session_t *ses)
{
    int i;
    uint8 cur_clk = (sys_clk.base >> 8) & 0xFF;

    for (i=0; i<ses->pcr_ctx_cnt; i++) {
        pcr_context_t* pcr_ctx = &ses->pcr_ctx_tab[i];
        if (pcr_ctx->pid == INVALID_PID) {
            continue;
        }

        if ((int8)(cur_clk - pcr_ctx->last_seen) < 0) {
            VIDEO_CLKREC_DEBUG("\nIn %d: Clean up PCR pid %d\n",
                               ses->id, pcr_ctx->pid);
            pcr_ctx->pid = INVALID_PID;
            if ((pcr_ctx->flags & PCR_CONTEXT_FLAG_LOCKED)) {
                pcr_ctx->flags = 0;
                ses->locked_pcr_cnt--;
            }
        }
    }

    for (i=ses->pcr_ctx_cnt-1; i>=0; i--) {
        pcr_context_t* pcr_ctx = &ses->pcr_ctx_tab[i];
        if (pcr_ctx->pid != INVALID_PID) {
            break;
        }
    }

    ses->pcr_ctx_cnt = i + 1;
}


// Advance TP info write pointer for an input session
//
void advance_input_tp_info_wr (in_session_t *ses)
{
      if (tp_info_ref_advance(&ses->tp_info_ref)) {
        if ((ses->tp_info_buf.size < TP_INFO_CHUNK_SIZE * TP_INFO_MAX_CHUNKS)
            && ((int32)(ses->perf.now - ses->tp_info_buf_wrap_time)
                <= MAX_LATENCY * SCALE_MS_BASE)) {
            tp_info_buffer_grow(&ses->tp_info_buf);

            if (ses->tp_info_ref.idx < ses->tp_info_buf.size) {
                tp_info_ref_set_chunk(&ses->tp_info_ref);
                return;
            }
        }

        tp_info_ref_wraparound(&ses->tp_info_ref);
        ses->tp_info_buf_wrap_time = ses->perf.now;
        ses->tp_info_usage_id++;
    }
}


// Process an input TP
//   Returns 1 if a TP info is generated for this TP.
//   Otherwise returns 0.
//
int process_input_tp (tp_header_t tp_hdr, uintptr_t tp_addr, in_session_t *ses)
{
    // Update TP index for the session
    ses->tp_idx++;

    // Parse TP header
    uint16 pid = tp_get_pid(&tp_hdr);

    // Check for SYNC
    if (tp_hdr.sync != SYNC_BYTE) {
        check_capture(ses->ip_hdr, CLI_VIDEO_CAPTURE_SYNCLOSS);
        ses->stat.tp_cnt++;
        ses->stat.sync_loss_cnt++;

        VIDEO_IN_DEBUG("\nIn %d, TP %d: Sync loss (%02x)",
                       ses->id, ses->tp_idx, tp_hdr.sync);
        return 0;
    }

    // Drop NULL TP
    if (pid == NULL_PID) {
        ses->stat.null_tp_cnt++;
        return 0;
    }

    ses->stat.tp_cnt++;

    // Prepare TP Info
    tp_info_t *tp_info = tp_info_ref_get(&ses->tp_info_ref);
    INFO_FLAGS(tp_info) = 0;        // reset all flags in TP info
    tp_info->info_type = TP_INFO;
    tp_info->usage_id = ses->tp_info_usage_id;
    tp_info->tp_idx = ses->tp_idx;
    tp_info->pid = pid;
    tp_info->addr = tp_addr + input_buf_offset[ses->cfg->input_port];
    tp_info->arvl_time = ses->arvl_time & OUT_TIME_MASK;

    // Check for adaptation field
    int af_flag = 0;
    af_header_t* af_hdr = NULL;
    if ((tp_hdr.af_ctrl & 2)) {
        af_hdr = (af_header_t*)(tp_addr + sizeof(tp_header_t));
        af_flag = (af_hdr->len > 0);
    }

    // Check whether TP carries PCR 
    // Note: when a session is PSI ready, we will only honor those PCRs
    //       whose pids are referenced by PMT as PCR pids
    if (af_flag && af_hdr->pcr_flag) {
        tp_info->pcr_flag = session_is_psi_ready(ses->state) ?
                                ses->pid_tab[pid].has_pcr : 1;
    }

    // Check for discontinuity point
    if (af_flag && af_hdr->discontinuity) {
        tp_info->disc_flag = 1;
        ses->stat.disc_cnt++;
        VIDEO_EVENT_DEBUG("\nIn %d: TP %d, pid %d, Disc",
                          ses->id, ses->tp_idx, pid);
        if (tp_info->pcr_flag) {
            ses->stat.tb_disc_cnt++;
            VIDEO_EVENT_DEBUG(", TB-disc");
        }
    }

    in_pid_info_t* pid_info = &ses->pid_tab[pid];

    // Check for unreferenced PID
    if (!pid_info->referenced && (ses->state & SESSION_STATE_PSI_READY)) {
        ses->stat.unref_tp_cnt++;
    }

    // Check input CC
    // TODO: need to handle discontinuous point (2 types)
    uint8 exp_cc = (pid_info->prev_cc + (tp_hdr.af_ctrl & 1)) & 0xf;
    if (tp_hdr.cc != exp_cc && !tp_info->disc_flag && pid_info->seen) {
        check_capture(ses->ip_hdr, CLI_VIDEO_CAPTURE_CCERR);
        ses->stat.cc_err_cnt++;

        VIDEO_EVENT_DEBUG("\nIn %d: CC-ERR, pid %d, TP %d, CC %d, " \
                          "expected %d, idle %d",
                          ses->id, pid, ses->tp_idx, tp_hdr.cc, exp_cc,
                          ses->flags & SESSION_FLAG_GO_ACTIVE);
    }
    pid_info->prev_cc = tp_hdr.cc;
    pid_info->seen = 1;

    // PSI Processing
    if (ses->cfg->parse_psi_flag) {
        if (pid == PAT_PID) {
            tp_info->pat_flag = 1;
            ses->stat.psi_tp_cnt++;
            parse_pat_tp(ses, tp_info);
            goto update;
        }

        if (pid_info->has_psi) {
            tp_info->pmt_flag = 1;
            ses->stat.psi_tp_cnt++;
            parse_pmt_tp(ses, tp_info);
            goto update;
        }
    }

    // TODO: why is this checked here??
    // Drop IP if the session is blocked
    if (session_is_blocked(ses->state)) {
        ses->stat.drop_ip_cnt++;
        return 0;
    }

    // Time processing
    boolean orig_disc_flag = tp_info->disc_flag;    // save for timing record
    if (tp_info->pcr_flag) {
        // PCR TP
        af_get_pcr_mod(af_hdr, &ses->pcr.base, &ses->pcr.ext);
        process_pcr_tp(ses, pid, tp_info->disc_flag);

        ses->stat.pcr_tp_cnt++;

        // Insert discontinuity point if needed
        if ((ses->flags & SESSION_FLAG_SET_DISC)) {
#if 0
            VIDEO_EVENT_DEBUG("\nIn %d: TP %d: insert disc",
                              ses->id, ses->tp_idx);
#endif
            // NOTE: disc_insert_enable shoud only be set to FALSE
            //       for testing only!!
            if (disc_insert_enable) {
                af_hdr->discontinuity = 1;
                tp_info->disc_flag = 1;
            }
            ses->flags &= ~SESSION_FLAG_SET_DISC;   // reset the flag
        }

    } else {
        // Non-PCR TP
        ses->stat.non_pcr_tp_cnt++;
    }

    if ((ses->flags & SESSION_FLAG_GO_ACTIVE)) {
        tp_info->idle_flag = 1;
        ses->flags &= ~SESSION_FLAG_GO_ACTIVE;    // reset the flag
    }

    if (rec_on && (ses->id == rec_in_sid)) {
        // Record timing info
        clkrec_record_tp(ses->tp_idx, pid, ses->arvl_time, tp_info->pcr_flag,
                         &ses->pcr, orig_disc_flag);
    }

update:
    // Update
    if (tp_info->addr) {
        advance_input_tp_info_wr(ses);
        return 1;
    }
    return 0;
}


#if 0
// Note: process_input_ip_pkt() is now PD code in order to handle
//       platform-specific optimizations such as pre-touch and
//       HW-assisted header.
//
void process_input_ip_pkt_std (ip_pkt_info_t *ip_pkt_info, in_session_t *ses)
{
    ip_header_t* ip_hdr = (ip_header_t*)ip_pkt_info->addr;
    assert(ip_hdr != 0);
    ipv6_header_t* ipv6_hdr  = (ipv6_header_t*)ip_pkt_info->addr;

    // perfrom ip version check to set up 
    // address pointer for udp header 
    udp_header_t* udp_hdr;
    if (ip_hdr->ver == IPV4_VERSION) {
        udp_hdr = (udp_header_t*)(ip_hdr + 1);
    } else {
       udp_hdr = (udp_header_t*)(ipv6_hdr + 1); 
    }

    uintptr_t addr = (uintptr_t)(udp_hdr + 1);

    // RTP is not yet supported!
    int rtp_hdr_size = get_rtp_header_size(addr);
    if (rtp_hdr_size > 0) {
        // drop IP
        ses->stat.rtp_cnt++;
        return;
    }
    addr += rtp_hdr_size;

    // Jumbo frame not yet supported!
    int num_tp = (udp_hdr->length - rtp_hdr_size) / TP_SIZE;
    if (num_tp > MAX_TP_PER_IP) {
        // drop IP
        ses->stat.drop_ip_cnt++;
        return;
    }

    // Check for source IP change
    if (ip_hdr->ver == IPV4_VERSION) {
        if (ip_hdr->src_ip != ses->src_ip[0]) {
            ses->src_ip_change_cnt++;
            ses->src_ip[0] = ip_hdr->src_ip;
            VIDEO_IN_DEBUG("\nIn %d: new src IP %08x", ses->id, ses->src_ip[0]);
        }
    } else {
        if (!memcmp(ipv6_hdr->src_ip, ses->src_ip, sizeof(ipv6_hdr->src_ip))) {
            ses->src_ip_change_cnt++;
            memcpy(ses->src_ip,ipv6_hdr->src_ip, sizeof(ipv6_hdr->src_ip));
            VIDEO_IN_DEBUG("\nIn %d: new src IP %08x", ses->id, ses->src_ip[3]);
        }
    }

    ses->ip_hdr = ip_hdr;
    ses->arvl_time = ip_pkt_info->arvl_time;

    // Process all TPs in IP
    int i;
    for (i=0; i<num_tp; i++) {
        process_input_tp(*(tp_header_t*)addr, addr, ses, ip_hdr);
        addr += TP_SIZE;
    }
}
#endif


// Process an input session going from IDLE/OFF into ACTIVE
void process_input_session_going_active (in_session_t *ses)
{
    if (ses->stat.idle_cnt) {
        uint32 idle_time = (int32)(ses->perf.now - ses->prev_tp_arvl);
        idle_time /= SCALE_MS_BASE;
        ses->stat.idle_time += idle_time;

        VIDEO_EVENT_DEBUG("\nIn %d: active after idle for %d ms",
                          ses->id, idle_time);
    }

    if (ses->tp_info_buf.size == 0) {
        // Assign one chunk in TP buffer initially
        tp_info_buffer_grow(&ses->tp_info_buf);
        tp_info_ref_init(&ses->tp_info_ref, &ses->tp_info_buf, 0);
    }
    ses->tp_info_buf_wrap_time = ses->perf.now;

    session_set_active(&ses->state);
    ses->flags |= SESSION_FLAG_GO_ACTIVE;
        // This flag is to help set the idle_flag in the first TP
        // after the idle/off period.  Output thread needs this to
        // set the delivery time correctly.

    ses->flags |= SESSION_FLAG_STATE_CHANGED;
        // This will cause the state changed to be reported to the SUP.
}


static inline
void process_input_session_going_idle (in_session_t *ses)
{
    VIDEO_EVENT_DEBUG("\nIn %d: goes IDLE", ses->id);
    ses->flags |= SESSION_FLAG_STATE_CHANGED;
    session_set_idle(&ses->state);
    ses->stat.idle_cnt++;
}


static inline
void process_input_session_going_off (in_session_t *ses)
{
    VIDEO_EVENT_DEBUG("\nIn %d: goes OFF", ses->id);
    ses->flags |= SESSION_FLAG_STATE_CHANGED;
    session_set_off(&ses->state);
    ses->state &= ~SESSION_STATE_BLOCKED;

    // Clean up the session
    psi_lock(ses->id);
    in_session_cleanup_progs(ses);
    psi_unlock(ses->id);

    // Set initial TP interval based on allocated bitrate
    ses->tp_itvl = tp_bitrate_to_itvl(ses->cfg->bitrate_alloc);

    // Forget snooped PAT
    in_session_forget_pat(ses);

    // Reset PCR table
    pcr_context_reset(ses);

    // Reset TP info buffer
    tp_info_buffer_reset(&ses->tp_info_buf);
}


// Process an input session
//
void process_input_session (in_session_t *ses, uint16 *flow_rd, uint16 flow_wr)
{
    uint16 flow_rd2 = *flow_rd;
    int32 num_ip = ring_mod(flow_wr - flow_rd2, IP_FLOW_BUF_SIZE);
    ip_pkt_info_t* info = ip_flow_buf + (ses->id * IP_FLOW_BUF_SIZE + flow_rd2);

    // Get the session
    // Skip the packet if the session is not used or enabled
    if (!(session_is_used(ses->state) && session_is_enabled(ses->state)) ||
        session_is_no_resources(ses->state)) {
        ses->stat.drop_ip_cnt += num_ip;
        return;
    }

    // Prefetch the first TP info in the session
    data_hint(tp_info_ref_get(&ses->tp_info_ref));

    // Check if session is becoming ACTIVE
    //
    if (!session_is_active(ses->state)) {
#if 0
        if (!acquire_load_credit(1)) {
            ses->stat.drop_ip_cnt += num_ip;
            goto DONE;
        }
#endif
        process_input_session_going_active(ses);
    }

    // Update session's last TP arrival time
    ses->prev_tp_arvl = ses->perf.now;

    // Process all IP packets for the in session
    int i;
    for (i=0; i<num_ip; i++) {
        process_input_ip_pkt(info++, ses);
        if (++flow_rd2 == IP_FLOW_BUF_SIZE) {
            flow_rd2 = 0;
            info -= IP_FLOW_BUF_SIZE;
        }
    }

    sync_readwrite();
    ses->ref_tp_info_idx = ses->tp_info_ref.idx;
    ses->stat.ip_cnt += num_ip;

//DONE:
    *flow_rd = flow_wr;
}


// Monitor input session
//
void video_monitor_in_session (in_session_t *ses)
{
    if (!session_is_used(ses->state) || session_is_no_resources(ses->state)) {
        return;
    }

    // Update session measured bitrate
    update_avg_bitrate(ses);

    if (ses->cfg->bitrate_alloc) {
        if (ses->bitrate_exceeded) {
            if (ses->avg_bitrate < (ses->cfg->bitrate_alloc * 31 >> 5)) {
                ses->bitrate_exceeded = 0;
            }
        } else {
            if (ses->avg_bitrate > ses->cfg->bitrate_alloc) {
                video_event_fire_source_err(ses, VIDEO_EVENT_BITRATE_EXCEEDED);
                ses->bitrate_exceeded = 1;
            }
        }
    }

    // Check if PCR is missing
    if (check_session_pcr_missing(ses)) {
        pcr_context_reset(ses);
        VIDEO_CLKREC_DEBUG("\nIn %d: no PCR", ses->id);
    }

    // Clean up PCR context table for disappearing PCR 
    pcr_context_cleanup(ses);

    // Check if the session becomes idle
    if (check_session_idle(ses)) {
        uint8 old_state = ses->state;
        process_input_session_going_idle(ses);
        video_event_fire_source_state_change(ses->cfg,
                old_state & SESSION_STATE_TRAFFIC_MASK,
                ses->state & SESSION_STATE_TRAFFIC_MASK);
    }

    // Check if the session becomes off 
    if (check_session_off(ses)) {
        process_input_session_going_off(ses);
    }

    if (clkrec_par.review_period > 0 && --ses->periods_till_review == 0) {
        // Review drifts among all PLLs in the session
        if (ses->pcr_ctx_cnt > 1 && session_is_active(ses->state)) {
            pll_review(ses);
        }

        ses->periods_till_review = clkrec_par.review_period;
    }
}

