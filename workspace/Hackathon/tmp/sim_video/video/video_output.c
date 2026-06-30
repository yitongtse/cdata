/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "video.h"
#include "video_util.h"
#include "video_event.h"


static inline
void copy_tp_info_buffer_ptr (out_session_t *out_ses, in_session_t *in_ses)
{
    uint16 in_id = in_ses->id;
    in_session_lock(in_id);
    tp_info_ref_init(&out_ses->tp_info_ref, &in_ses->tp_info_buf,
                     in_ses->ref_tp_info_idx);
    out_ses->tp_info_usage_id = in_ses->tp_info_usage_id;
    in_session_unlock(in_id);
}


static inline
void check_stay_time (out_session_t *ses, const tp_info_t *tp_info)
{
    int32 stay_time;

    stay_time = (int32)((ses->out_time - tp_info->arvl_time) << OUT_TIME_SHIFT);
    stay_time >>= OUT_TIME_SHIFT;

    VIDEO_OUT_DEBUG(", OT %d, stay %d", ses->out_time, stay_time);
    
    if (stay_time < ses->min_stay_time) {
        ses->min_stay_time = stay_time;
    }
    if (stay_time > ses->max_stay_time) {
        ses->max_stay_time = stay_time;
    }

    if (ses->flags & SESSION_FLAG_PROCESS_TIME) {
        // Check for buffer violation
        if (stay_time < BUF_HEADROOM) {
            ses->stat.underflow_cnt++;
            VIDEO_OUT_DEBUG(", U/F");

            // Buffer underflow handling
            ses->out_time = tp_info->arvl_time + BUF_HEADROOM;
        }

        if (stay_time > ses->stay_time_limit) {
            ses->stat.overflow_cnt++;
            VIDEO_OUT_DEBUG(", O/F");

            // TBD: need to handle buffer overflow!!
        }
    }
}


static inline
boolean check_overdue (in_session_t *in_ses, out_session_t *ses,
                       uint32 past_out_time_limit)
{
    int32 drop_count;

    if ((int32)((ses->out_time - past_out_time_limit) << OUT_TIME_SHIFT) >= 0) {
        return FALSE;
    }

    drop_count = in_ses->ref_tp_info_idx - ses->tp_info_ref.idx;
    if (drop_count <= 0) {
        drop_count += in_ses->tp_info_buf.size;
    }

    VIDEO_OUT_DEBUG("\nOut %d: OVERDUE, info %d-%d, now %d, OT %d, lmt %d, "\
                    "%d TPs",
                    ses->id, ses->tp_info_usage_id, ses->tp_info_ref.idx,
                    sys_clk.base & 0xFFFFF, ses->out_time & 0xFFFFF,
                    past_out_time_limit & 0xFFFFF, drop_count);

    copy_tp_info_buffer_ptr(ses, in_ses);

    ses->flags &= ~SESSION_FLAG_TP_FOUND;
    ses->stat.overdue_tp_cnt += drop_count;

    return TRUE;
}


// Recheck conflict status of all blocked programs in QAM
static
void qam_recheck_conflict (qam_info_t *qam)
{
    int i;
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);
    boolean changed = FALSE;

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++, flow_map++) {
        if (*flow_map != INVALID_ID) {
            out_session_t* ses = get_out_session(get_out_id(*flow_map));
            if (session_is_used(ses->state)) {
                changed = out_session_recheck_conflict(ses, qam);
            }
        }
    }

    if (changed)  regenerate_pat(qam);
}


// Output PCR TP processing
//
static
void process_out_pcr (out_session_t *ses, const tp_info_t *tp_info,
                      const pcr_info_t *pcr_info, mpeg_clock_t *tbo)
{
    uint32 prev_out_time = ses->out_time;
    ses->out_time = pcr_info->out_time;
    mpeg_clock_set(tbo, pcr_info->tbo_base, pcr_info->tbo_ext);

    // Update session's TP interval
    ses->tp_itvl = pcr_info->tp_itvl;

    ses->stat.pcr_tp_cnt++;

    VIDEO_OUT_DEBUG("\nOut %d-%d: info %d-%d, itvl %d, prevOT %d, gap %d",
                        ses->id, tp_info->tp_idx, ses->tp_info_usage_id,
                        ses->tp_info_ref.idx, ses->tp_itvl, prev_out_time,
                        ses->out_time - prev_out_time);

    // Check stay time
    check_stay_time(ses, tp_info);
}


static inline
void compute_inter_out_time (out_session_t *ses, const tp_info_t *tp_info)
{
    int tp_idx_inc = (int16)(tp_info->tp_idx - (uint16)ses->tp_idx);
    int64 out_time_inc = (((int64)ses->tp_itvl) * tp_idx_inc) >> FRAC_BITS_TIME;
    ses->out_time += (int32)out_time_inc;
}


// Output new PCR TP processing
//
static
void process_out_new_pcr (out_session_t *ses, const tp_info_t *tp_info,
                          const new_pcr_info_t *pcr_info, mpeg_clock_t *tbo)
{
    mpeg_timediff_t tbo_out;

    // Compute out_time as if this is a non-PCR TP
    compute_inter_out_time(ses, tp_info);

    // Compute TBO
    tbo_out = mpeg_time_set(pcr_info->pcr_base - ses->out_time,
                            pcr_info->pcr_ext) - hw_time_adj;
    mpeg_time_to_clock(tbo_out, tbo);

    ses->stat.pcr_tp_cnt++;

    VIDEO_OUT_DEBUG("\nOut %d-%d: NEW, info %d-%d", ses->id, tp_info->tp_idx,
                    ses->tp_info_usage_id, ses->tp_info_ref.idx);
}


// Slave PCR TP processing
//
static
void process_out_slave_pcr (out_session_t *ses, tp_info_t *tp_info,
                            pcr_info_t * pcr_info, mpeg_clock_t *tbo)
{
    // Compute out_time as if this is a non-PCR TP
    compute_inter_out_time(ses, tp_info);

    // Compute TBO
    uint16 tp_idx_inc = (int16)(tp_info->tp_idx - ses->anchor_tp_idx);
    mpeg_timediff_t out_time_inc
        = ((mpeg_timediff_t)tp_idx_inc) * ses->tp_itvl * SCALE_BASE_EXT;
    mpeg_timediff_t out_tbo
        = mpeg_time_set(pcr_info->tbo_base, pcr_info->tbo_ext)
              - out_time_inc - hw_time_adj;
    mpeg_time_to_clock(out_tbo, tbo);
}


// Non-PCR TP processing
//
static inline
void process_out_non_pcr (out_session_t *ses, const tp_info_t *tp_info)
{
    if ((ses->flags & SESSION_FLAG_PROCESS_TIME)) {
        if ((ses->flags & SESSION_FLAG_TP_FOUND) && (!tp_info->idle_flag)) {
            // Most common case
            compute_inter_out_time(ses, tp_info);

        } else {
            // First output TP at startup, or after an idle period
            ses->out_time = tp_info->arvl_time +
                            ses->in_ses->cfg->jitter_size * SCALE_MS_BASE / 2;
        }

    } else {
        // Schedule TP for output immediately (for data piping sessions)
        ses->out_time = sys_clk.base;
    }
}


static
int out_prog_process_new_pat (out_session_t *ses, in_prog_t *in_prog)
{
    out_prog_t* prog = out_prog_lookup_by_in_prog_num(&ses->prog_list,
                                                      in_prog->prog_num);

    if (prog) {
        // Updated input program
        VIDEO_DEBUG("\nSes %d: updated prog %d, PMT %d",
                    ses->cfg->client_id, in_prog->prog_num, in_prog->pmt_pid);
        prog->in_prog = in_prog;
        prog->in_pmt_pid = in_prog->pmt_pid;
        prog->pmt_usage_id = PSI_NOT_READY_USAGE_ID;
        // TODO: need to set updated flage?
        // TODO: Need to clean up old pmt_pid
        // TODO: How and when old PMT will be cleaned up?

        qam_info_t* qam = get_qam_info(ses->qam_id);
        int ses_reg=1, qam_reg=1;
        out_prog_set_register(prog, ses_reg, qam_reg);
        out_prog_register(prog, ses, qam);
    } else {
        // New input program
        //VIDEO_DEBUG("\nSes %d: new prog %d, PMT %d",
        //            ses->cfg->client_id, in_prog->prog_num, in_prog->pmt_pid);
        prog = out_prog_alloc();
        if (!prog) {
            em_vidman_VIDEO_OUT_PROG_ALLOC_FAILED("Update Passthru PAT", 
                                                  in_prog->prog_num);
            return ENOMEM;
        }

        out_prog_setup(prog, in_prog, ses);

        // Look up input program in configured program list
        out_config_t* cfg = ses->cfg;
        int idx = video_list_search(in_prog->prog_num, cfg->in_prog_list,
                                    cfg->prog_cnt);

        boolean remap = ses->flags & SESSION_FLAG_PID_REMAP;
        boolean included = remap ^ (idx == -ENOENT);

        if (included) {
            out_prog_set_include(prog, ses, idx);
        } else {
            prog->filtered = 1;
            out_prog_set_exclude(prog, ses, idx);
        }
        out_prog_set_register(prog, included == remap, included);

        //VIDEO_DEBUG(", filtered %d, cfg_idx %d, %d/%d/%d/%d",
        //            prog->filtered, prog->cfg_idx,
        //            prog->ses_reg_prog, prog->ses_reg_pid,
        //            prog->qam_reg_prog, prog->qam_reg_pid);

        // Add out program to out session
        que_put(NULL, &ses->prog_list, &prog->link);
    }

    prog->present = 1;
    return EOK;
}


static
void process_out_pat (out_session_t *ses, const psi_info_t * psi_info)
{
    out_prog_t *prog;
    qam_info_t* qam = get_qam_info(ses->qam_id);
    boolean changed = FALSE;

    ses->stat.psi_tp_cnt++;

    if (psi_info->psi_usage_id != ses->pat_usage_id
            && ses->in_ses->pat_snooped) {

        // Process new PAT
        VIDEO_PSI_DEBUG("\n\nOut %d: new PAT TP", ses->id);
        ses->stat.new_pat_cnt++;

        //VIDEO_DEBUG("\nSes %d: new out PAT", ses->cfg->client_id);

        in_session_t* in_ses = ses->in_ses;
        in_prog_t *in_prog;
        boolean first_prog = TRUE;

        psi_lock(in_ses->id);
        FOR_ALL_ELEMENTS_IN_QUE(&in_ses->prog_list, in_prog) {
            if (first_prog && in_ses->type == PROCESS_TYPE_REMAP) {
                ses->cfg->in_prog_list[0] = in_prog->prog_num;
            }
            out_prog_process_new_pat(ses, in_prog);
            first_prog = FALSE;
        }
        psi_unlock(in_ses->id);

        // Process disappearing programs
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
            if (prog->present) {
                prog->present = 0;

            } else {
                // Disappearing program
                VIDEO_DEBUG("\nSes %d: prog %d disappearing",
                            ses->cfg->client_id, prog->in_prog_num);
                changed = TRUE;

                // Clean up out program
                que_elem_t* elem = prog->link.prev;
                que_deque(NULL, &prog->link);
                out_prog_cleanup(prog, ses);
                out_prog_free(prog);
                prog = (out_prog_t*)elem;

                qam->recheck_conflict = TRUE;
            }
        }

        ses->flags |= SESSION_FLAG_PAT_BUILT;
        ses->pat_usage_id = psi_info->psi_usage_id;
    }

    if (changed || ses->include_prog_flag || ses->exclude_prog_flag) {
        ses->state &= ~SESSION_STATE_BLOCKED;

        //VIDEO_DEBUG("\nSes %d: PAT scan-2", ses->cfg->client_id);

        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {

            // Check conflict for new included session
            if (prog->qam_reg_prog && !prog->filtered) {
                // Determine out program number and PMT pid for remapped program
                if ((ses->flags & SESSION_FLAG_PID_REMAP)) {
                    out_prog_remap_prog(prog, ses);
                }

                // Check for conflict
                uint16 pid;             // pid in conflict
                prog->blocked |= out_prog_check_conflict(prog, ses, &pid) ||
                                     (prog->pmt_built &&
                                         pmt_check_conflict(&prog->pmt, ses,
                                                            &pid));
                if (prog->blocked) {
                    out_prog_set_register(prog, 1, 1);
                    prog->cfg_changed = 1;

                    if (pid == NULL_PID) {
                        video_event_fire_conflict_err(
                                VIDEO_EVENT_PROGRAM_CONFLICT, ses->qam_id,
                                ses->cfg->owner_id, ses->cfg->client_id,
                                prog->prog_num);
                    } else {
                        video_event_fire_conflict_err(
                                VIDEO_EVENT_PID_CONFLICT, ses->qam_id,
                                ses->cfg->owner_id, ses->cfg->client_id, pid);
                    }
                }
            }

            // Process excluded programs
            if (prog->filtered || prog->blocked) {
                //VIDEO_DEBUG("\nSes %d: register excluded prog %d",
                //            ses->cfg->client_id, prog->in_prog_num);
                out_prog_register(prog, ses, qam);

                if ((ses->flags & SESSION_FLAG_PID_REMAP)) {
                    out_prog_forget_pmt(prog, qam);
                }

                prog->cfg_changed = 0;
                qam->recheck_conflict = TRUE;
                ses->state |= SESSION_STATE_BLOCKED;
            }
        }
        changed = TRUE;
        ses->exclude_prog_flag = 0;
        qam->pat_rebuild = 1;
    }

    // Process old included programs
    if (changed) {
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
            if (!prog->cfg_changed && !(prog->filtered || prog->blocked)) {
                //VIDEO_DEBUG("\nSes %d: register included prog %d",
                //            ses->cfg->client_id, prog->in_prog_num);
                out_prog_set_register(prog, 1, 1);
                out_prog_register(prog, ses, qam);
            }
        }
    }

    // Process new included programs
    // TODO: maybe this should be combined with the previous loop!??
    if (ses->include_prog_flag) {
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {
            if (prog->cfg_changed && !(prog->filtered || prog->blocked)) {
                //VIDEO_DEBUG("\nSes %d: register new included prog %d, pmt %d",
                //       ses->cfg->client_id, prog->in_prog_num, prog->pmt_pid);

                // TODO: new mode to support PID allocation!!!
                out_prog_set_register(prog, 1, 1);
                out_prog_register(prog, ses, qam);    // new mode

                prog->pmt_crsl.next_time = sys_clk.base;
                prog->priv_insert.next_time = sys_clk.base;

                if (prog->pmt_built)  prog->cfg_changed = 0;
            }
        }
        ses->include_prog_flag = 0;
        qam->pat_rebuild = 1;
    }

    // Regenerate PAT if needed
    if (qam->pat_rebuild) {
        regenerate_pat(qam);
        qam->stat.psi_tp_cnt +=
            psi_crsl_insert_schedule(&qam->pat_crsl, qam->id, PSI_FLOW_ID,
                                     psi_tp_buf_offset, INVALID_PID);
        qam->pat_rebuild = 0;
    }
}


static
void process_out_pmt (out_session_t *ses, const psi_info_t *psi_info,
                      uint16 pid)
{
    ses->stat.psi_tp_cnt++;

    if (pid==0) {
        // prevent PMT being erroneously sent out on PID0
        ses->pat_usage_id = 0; // force PAT resnoop.
        return;
    }

    // Check whether output PAT is valid
    if (!(ses->flags & SESSION_FLAG_PAT_BUILT))  return;

    out_prog_t* prog
        = out_prog_lookup_by_in_prog_num(&ses->prog_list, psi_info->prog_num);
    if (!prog) {
        VIDEO_DEBUG("\nOut %d: prog for PMT %d, in prog %d not found",
                  ses->id, pid, psi_info->prog_num);
        return;
    }

    if (!prog->in_prog) {
        VIDEO_DEBUG("\nOut %d: in prog for PMT %d, in prog %d not available",
                  ses->id, pid, psi_info->prog_num);
        return;
    }

    if (!prog->in_prog->pmt_snooped)  return;

    qam_info_t* qam = get_qam_info(ses->qam_id);
    boolean pmt_changed = (prog->pmt_usage_id != psi_info->psi_usage_id);
    boolean excluded = FALSE;

    if (pmt_changed) {
        ses->stat.new_pmt_cnt++;

        if (prog->pmt_built) {
            // Unregister old PMT
            //VIDEO_DEBUG("\nProg %d-%d: unregister old PMT, ver %d/%d",
            //            ses->cfg->client_id, prog->in_prog_num,
            //            prog->pmt_usage_id, psi_info->psi_usage_id);
            out_prog_unregister(prog, ses, get_qam_info(ses->qam_id));
            excluded = TRUE;

            if ((ses->flags & SESSION_FLAG_PID_REMAP)) {
                // Free the PMT section
                pool_free(&psi_section_pool, &prog->pmt.sect->link);
            }

            // Free TP buffers used by PMT
            pool_free_list(&psi_tp_buf_pool, &prog->pmt_crsl.tp_list);

            prog->pmt_built = FALSE;
            ses->pmt_generated_cnt--;
            ses->state &= ~SESSION_STATE_PSI_READY;

            // Reregister the program
            out_prog_set_register(prog, 1, 1);
            out_prog_register(prog, ses, qam);
        }
    }

    if (!(pmt_changed || excluded || prog->cfg_changed)) {
        return;
    }

    //VIDEO_DEBUG("\nProg %d-%d: updating (%d/%d/%d)", ses->cfg->client_id,
    //            prog->in_prog_num, pmt_changed, excluded, prog->cfg_changed);

    boolean remapped = ses->flags & SESSION_FLAG_PID_REMAP;
    boolean prog_included = !(prog->filtered || prog->blocked);
    int rc = EOK;

    if (remapped) {
        // ca block status update
        if (ses->cfg->encrypt_flag) {
            video_ca_block_update(ses, prog);
            excluded = prog->blocked;
            prog_included = !prog->blocked;
            //VIDEO_DEBUG("\n%s %d: ca block: excluded %d, prog_included %d rc %d", 
            //            __FUNCTION__, __LINE__, excluded, prog_included, rc);
        }
        if (prog_included) {
            rc = regenerate_pmt(ses, prog, pid);
        }
    } else if (pmt_changed || !prog->pmt_built) {
        rc = passthru_pmt(ses, prog);
    }

    if (rc == EAGAIN) {
        // Input PMT being update.  Ignore this psi_info.
        return;
    }

    if (prog_included) {
        if (rc == EOK) {
            uint16 bad_pid;         // conflicting pid
            prog->blocked |= pmt_check_conflict(&prog->pmt, ses, &bad_pid);
            if (prog->blocked) {
                video_event_fire_conflict_err(VIDEO_EVENT_PID_CONFLICT,
                                              ses->qam_id, ses->cfg->owner_id,
                                              ses->cfg->client_id, bad_pid);
            }
        } else {
            prog->blocked = 1;
        }

        if (prog->blocked) {
            out_prog_set_exclude(prog, ses, prog->cfg_idx);
            out_prog_set_register(prog, 1, 1);
            ses->state |= SESSION_STATE_BLOCKED;
        }
    }

    if (prog->filtered || prog->blocked) {
        excluded = TRUE;
        prog->on_air = 0;
    }

    //VIDEO_DEBUG("\nProg %d-%d: register, filtered %d, blocked %d, cfg_idx %d",
    //            ses->cfg->client_id, prog->in_prog_num, prog->filtered,
    //            prog->blocked, prog->cfg_idx);
    out_prog_register(prog, ses, qam);

    prog->pmt_usage_id = psi_info->psi_usage_id;
    prog->cfg_changed = 0;

    if (excluded) {
        // Re-register all included programs
        out_prog_t *prog2;
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog2) {
            if (!(prog2->filtered || prog2->blocked)) {
                //VIDEO_DEBUG("\nProg %d-%d: reregister",
                //            ses->cfg->client_id, prog2->in_prog_num);
                out_prog_set_register(prog2, 1, 1);
                out_prog_register(prog2, ses, qam);
            }
        }
    }

    if (!prog->on_air && !excluded) {
        // Program being turned on
        prog->on_air = 1;
        video_event_fire_session_state_change(ses->cfg->client_id,
                prog->prog_num, ses->cfg->owner_id, 0, 1);
    }
}


// Process an output TP
//   Returns TRUE if there are more TP in the session to process.
//   Otherwise returns FALSE.
//
//   Note: the following conditions will return FALSE
//     - there are no more tp_info to process
//     - the last master PCR is met and it is not due
//
static
boolean process_out_tp (out_session_t *ses, in_session_t *in_ses,
                        boolean *rd_ptr_update, uint32 past_out_time_limit)
{
    // Check if there is any more TP info to process
    if (ses->tp_info_ref.idx == in_ses->ref_tp_info_idx) {
        VIDEO_OUT_DEBUG(", END %d", ses->tp_info_ref.idx);
        return FALSE;
    }

    // Check if any output command buffers available
    if (ses->cmdbuf_avail <= 0) {
        if (!*rd_ptr_update) {
            // Update available cmd buffer count
            ses->cmdbuf_avail = video_get_flow_avail_space(ses->qam_id,
                                                           ses->flow_id);
            *rd_ptr_update = TRUE;
        }

        if (ses->cmdbuf_avail <= 0) {
            VIDEO_OUT_DEBUG(", NO-CMD %d", ses->cmdbuf_avail);
            ring_buf_t* ring = get_cmd_ring(ses->qam_id,ses->flow_id);
            VIDEO_OUT_DEBUG("\nring buf qam/fl/wr/rd %d/%d/%d/%d\n",
                      ses->qam_id, ses->flow_id,ring->wr, ring->rd);
            //ses->stat.cmdbuf_maxhit_cnt++;
            return FALSE;
        }
    }

    tp_info_t* tp_info = tp_info_ref_get(&ses->tp_info_ref);

    // Prefetch next tp_info
    data_hint(tp_info + 1);

    // Check for info type error
    if (tp_info->info_type == PCR_INFO || tp_info->info_type == NEW_PCR_INFO) {
        VIDEO_OUT_DEBUG("\nOut %d: unexpected PCR info %d",
                        ses->id, ses->tp_info_ref.idx);
        ses->stat.info_err_cnt++;
        goto cont;
    }

    // Get out PID info
    // Note: only TP_INFO and PSI_USAGE can reach this point,
    //       both of them have pid in the same location!
    out_pid_info_t *pid_info = &ses->pid_tab[tp_info->pid];

    boolean remapped = (ses->flags & SESSION_FLAG_PID_REMAP);

    // New PSI handling
    if (tp_info->info_type == PSI_USAGE) {
        psi_info_t* psi_info = (psi_info_t*)tp_info;
        if (psi_info->pat_flag) {
            process_out_pat(ses, psi_info);

        } else if (psi_info->pmt_flag) {
            process_out_pmt(ses, psi_info,
                            remapped ? pid_info->pid : tp_info->pid);
        }
        goto cont;
    }

    // Note: only TP_INFO Can reach this point!
    ses->stat.tp_cnt++;

    // Check for TP info array overrun
    if ((ses->tp_info_usage_id & 0xF) != tp_info->usage_id) {
        VIDEO_DEBUG("\n** Out %d: %d -> %d: info overrun %d -> %d\n",
                    ses->id, ses->tp_info_ref.idx, in_ses->ref_tp_info_idx,
                    ses->tp_info_usage_id & 0xF, tp_info->usage_id);

        copy_tp_info_buffer_ptr(ses, in_ses);

        ses->flags &= ~SESSION_FLAG_TP_FOUND;
        ses->stat.info_overrun_cnt++;
        return TRUE;
    }

    // Get PCR info for PCR TP
    pcr_info_t* pcr_info = NULL;
    boolean pcr_flag = tp_info->pcr_flag;
    if (pcr_flag) {
        // Get PCR info
        if (tp_info_ref_advance(&ses->tp_info_ref)) {
            tp_info_ref_wraparound(&ses->tp_info_ref);
            ses->tp_info_usage_id++;
        }
        pcr_info = (pcr_info_t*)tp_info_ref_get(&ses->tp_info_ref);

        // Expecting a PCR_info here
        if (pcr_info->info_type == TP_INFO
                || pcr_info->info_type == PSI_USAGE) {
            VIDEO_LOG("Out %d: unexpected TP info %d",
                      ses->id, ses->tp_info_ref.idx);
            ses->stat.info_err_cnt++;
            goto cont;
        }
    }

    if (remapped ^ pid_info->reversed) {
        ses->stat.drop_tp_cnt++;
        goto cont;      // drop TP
    }

    // PAT processing
    if (tp_info->pat_flag) {
        goto cont;    // drop PAT TP (QAM PAT are inserted asynchronously)
    }

    // PMT processing
    if (tp_info->pmt_flag && (ses->flags & SESSION_FLAG_PID_REMAP)) {
        goto cont;    // drop PMT TP for muxed session
                      // (out PMT inserted asynchronously)
    }

    // TP will be forwarded to output at this point
    ses->stat.forward_tp_cnt++;

    // Regular TP scheduling
    // Note: both NEW_PCR_IFO and SLAVE_PCR_INFO have the same value
    //
    mpeg_clock_t tbo = {0, 0};
    if (pcr_flag) {
        // PCR TP
        if (pcr_info->info_type == PCR_INFO) {
            // Update anchor
            ses->anchor_info_idx = ses->tp_info_ref.idx;
            ses->anchor_tp_idx = tp_info->tp_idx;

            // PCR (unified CR) or master PCR (master-slave CR)
            process_out_pcr(ses, tp_info, pcr_info, &tbo);

        } else if ((ses->flags & SESSION_FLAG_MASTER_SLAVE_CR)) {
            // Slave PCR (master slave CR)
            process_out_slave_pcr(ses, tp_info, pcr_info, &tbo);

        } else {
            // New PCR (unified CR)
            process_out_new_pcr(ses, tp_info, (new_pcr_info_t*)pcr_info, &tbo);
        }

    } else {
        // Non-PCR TP
        process_out_non_pcr(ses, tp_info);
        ses->stat.non_pcr_tp_cnt++;
    }

    // TODO: should check overdue on per session level, instead of per TP!
    if (check_overdue(in_ses, ses, past_out_time_limit)) {
        VIDEO_OUT_DEBUG(", OVERDUE");
        return FALSE;
    }

    ses->tp_idx = tp_info->tp_idx;
    ses->flags |= SESSION_FLAG_TP_FOUND;


    // Set up output command
    ses->cmdbuf_avail--;    // ??

    out_command_t* cmd
        = setup_outcmd(ses->qam_id, ses->flow_id,
                       ses->out_time & OUT_TIME_MASK, tp_info->addr);

    if (cmd == NULL) {
        VIDEO_OUT_DEBUG("CMDBUF MAXHIT");
        //ses->stat.cmdbuf_maxhit_cnt++;
        return FALSE;
    }

    // PID remap
    if (remapped) {
        outcmd_set_pid_restamp(cmd, pid_info->pid);
    }

    // PCR TP
    if (pcr_flag) {
        outcmd_set_pcr_restamp(cmd, tbo);
    }

    if (pid_info) {
        outcmd_set_cw_index(cmd,
                 get_qam_info(ses->qam_id)->pid_tab[pid_info->pid].cw_idx);
    }

#if VIDEO_DIAG
    // Video diagnostics
    if (ses->qam_id == diag_qam_id) {
        uintptr_t vir_addr = tp_info->addr -
                              input_buf_offset[in_ses->cfg->input_port];
        video_diag_prepare(ses->flow_id, tp_info, pcr_info, cmd, vir_addr);
    }
#endif

cont:
    if (tp_info_ref_advance(&ses->tp_info_ref)) {
        tp_info_ref_wraparound(&ses->tp_info_ref);
        ses->tp_info_usage_id++;
    }
    return TRUE;
}


// Process an output session
//
static
void process_out_session (out_session_t *ses, in_session_t *in_ses,
                          boolean update_pmt_flag, uint32 future_out_time_limit,
                          uint32 past_out_time_limit)
{
    qam_info_t* qam = get_qam_info(ses->qam_id);
    boolean rd_ptr_update = FALSE;
    uint16 in_anchor_info_idx = in_ses->anchor_info_idx;        // local copy
    uint8 traffic_state = in_ses->state & SESSION_STATE_TRAFFIC_MASK;

    if (traffic_state != ses->prev_traffic_state) {

        if (traffic_state == SESSION_STATE_IDLE) {
            if (ses->prev_traffic_state == SESSION_STATE_ACTIVE) {
                video_event_fire_session_state_change(ses->cfg->client_id,
                                0, ses->cfg->owner_id, 1, 0);
            }

        } else if (traffic_state == SESSION_STATE_OFF) {
            ses->state &= ~SESSION_STATE_BLOCKED;

            if (!que_is_empty(NULL, &ses->prog_list)) {
                out_session_cleanup_progs(ses);
                qam->pat_rebuild = 1;
                qam->recheck_conflict = 1;
                ses->flags &= ~SESSION_FLAG_PAT_BUILT;
                ses->pmt_generated_cnt = 0;
                ses->state &= ~SESSION_STATE_PSI_READY;
                // reset tp_info_buf
                pcr_context_reset(in_ses);
                tp_info_buffer_reset(&in_ses->tp_info_buf);
                copy_tp_info_buffer_ptr(ses, in_ses);
            }
            ses->prev_traffic_state = traffic_state;
            return;
        } else if (traffic_state == SESSION_STATE_ACTIVE) {
            video_event_fire_session_state_change(ses->cfg->client_id,
                                  0, ses->cfg->owner_id, 0, 1);
            // Update anchor
            ses->anchor_info_idx = ses->tp_info_ref.idx;
        }
        ses->prev_traffic_state = traffic_state;
    }

    if (ses->tp_info_ref.chunk == NULL) {
        tp_info_ref_set_chunk(&ses->tp_info_ref);
        if (ses->tp_info_ref.chunk == NULL) {
            return;
        }
    }

    // Check if the input session is missing PCR
    if (in_anchor_info_idx == INVALID_INFO_IDX) {
        // Treat the first processed out TP as a discontinuity point
        ses->flags &= ~SESSION_FLAG_TP_FOUND;
    }

    // Check to see if we are due to send out PMT information
    if (update_pmt_flag && (ses->flags & SESSION_FLAG_PID_REMAP)) {
        out_prog_t *out_prog;
        FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, out_prog) {
            if (out_prog->filtered)  continue;

            if (!ses->cfg->encrypt_flag || out_prog->pmt.ca_desc_cnt > 0) {
                // Only playout PMT if this is a clear session,
                // or at least one CA descriptor has been inserted
                if (out_prog->pmt_built) {
                    // Send out PMT
                    ses->stat.insert_tp_cnt +=
                        psi_crsl_insert_schedule(&out_prog->pmt_crsl,
                                ses->qam_id, PSI_FLOW_ID, psi_tp_buf_offset,
                                INVALID_PID);
                }

                // Send out private sections
                out_prog->priv_insert.next_cc = out_prog->pmt_crsl.next_cc;
                ses->stat.insert_tp_cnt +=
                    crsl_insert_schedule(&out_prog->priv_insert, ses->qam_id,
                                         PSI_FLOW_ID, psi_tp_buf_offset,
                                         out_prog->pmt_pid);
                out_prog->pmt_crsl.next_cc = out_prog->priv_insert.next_cc;
            }
        }
    }

    // Prefetch the first tp_info
    data_hint(tp_info_ref_get(&ses->tp_info_ref));

    if (in_anchor_info_idx != INVALID_INFO_IDX) {
        if (ses->anchor_info_idx != INVALID_INFO_IDX) {
            // Reread tp_itvl from anchor again
            pcr_info_t* pcr_info = (pcr_info_t*)
                    tp_info_get(ses->tp_info_ref.buf, ses->anchor_info_idx);
            ses->tp_itvl = pcr_info->tp_itvl;
        }
    }

    // Process up to future_out_time_limit
    while (1) {
        int32 time_diff = (int32)((ses->out_time - future_out_time_limit)
                                  << OUT_TIME_SHIFT);
        if (time_diff > 0 ||
            !process_out_tp(ses, in_ses, &rd_ptr_update,
                            past_out_time_limit)) {
            break;
        }
    }

    VIDEO_OUT_DEBUG("\nOUT %d: Done, idx %d, OT %d",
                    ses->id, ses->tp_info_ref.idx, ses->out_time & 0xFFFFF);

    // Send output command to output FPGA
    process_outcmd_for_flow(ses->qam_id, ses->flow_id);
}


// Enable a QAM channel
//
static
void qam_channel_enable (qam_info_t *qam)
{
    int i;
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++, flow_map++) {
        if (*flow_map == INVALID_ID) {
            continue;
        }   

        out_session_t* ses = get_out_session(get_out_id(*flow_map));
        if (!(session_is_used(ses->state))) {
            continue;
        }   

        VIDEO_OUT_DEBUG("\nQam %d restarted Out %d: %d -> %d: %d -> %d",
                qam->id, ses->id, ses->tp_info_ref.idx,
                ses->in_ses->ref_tp_info_idx, ses->tp_info_usage_id & 0xF,
                ses->in_ses->tp_info_usage_id & 0xF);

        copy_tp_info_buffer_ptr(ses, ses->in_ses);

        ses->flags &= ~SESSION_FLAG_TP_FOUND;
        ses->pat_usage_id = 0;              // force PAT resnoop
    }   

    qam->pmt_next_insert_time = sys_clk.base;
}


// Process a QAM channel
//
void process_qam_channel (qam_info_t *qam)
{
    if (!qam_channel_ready(qam->id)) {
        qam->flags &= ~QAM_FLAG_READY;
        return;                // Skip the rest
    }

    if (!(qam->flags & QAM_FLAG_READY)) {
        qam_channel_enable(qam);
        qam->flags |= QAM_FLAG_READY;
    }

    // Recheck conflict
    if (qam->recheck_conflict) {
        qam_recheck_conflict(qam);
        qam->recheck_conflict = FALSE;
    }

    // Insert PAT if needed
    if (qam->pat_built && psi_crsl_insert_due(&qam->pat_crsl)) {

        // Rebuild PAT due to QAM config changes
        if (qam->pat_rebuild) {
            regenerate_pat(qam);
            qam->pat_rebuild = 0;
        }

        qam->pat_crsl.period = qam->cfg->psi_period * SCALE_MS_BASE;
        qam->stat.psi_tp_cnt +=
            psi_crsl_insert_schedule(&qam->pat_crsl, qam->id, PSI_FLOW_ID,
                                     psi_tp_buf_offset, INVALID_PID);
    }

    // Carousel insertion
    crsl_insert_t *insert;
    FOR_ALL_ELEMENTS_IN_QUE(get_qam_crsl_insert_list(video_ctx, qam->id),
                            insert) {
        data_hint((void*)(((que_elem_t*)(insert))->next));
        if (crsl_insert_due(insert)) {
            if (insert->ecm_flag) {
                qam->stat.ecm_tp_cnt +=
                    ecm_insert_schedule(insert, qam->id, CRSL_FLOW_ID,
                                        0, INVALID_PID);
            } else {
                qam->stat.crsl_tp_cnt +=
                    crsl_insert_schedule(insert, qam->id, CRSL_FLOW_ID,
                                         video_ctx->crsl_tp_buf_offset,
                                         INVALID_PID);
            }
        }
    }

    // Send output commands to output FPGA
    process_outcmd_for_flow(qam->id, CRSL_FLOW_ID);

    // check to see if it is time to send out PMT information
    boolean update_pmt_flag = FALSE;
    if ((int32)(qam->pmt_next_insert_time - sys_clk.base) <= 0) {
        update_pmt_flag = TRUE;
        qam->pmt_next_insert_time
            = sys_clk.base + qam->cfg->psi_period * SCALE_MS_BASE - SCH_WIN;
    }

    // Process all out sessions in qam channel
    int i;
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);
    uint32 future_out_time_limit = (sys_clk.base + SCH_WIN) & 0xFFFFF;
    uint32 past_out_time_limit = (sys_clk.base - PAST_SCH_WIN) & 0xFFFFF;

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++, flow_map++) {
        data_hint(flow_map + 1);
        if (*flow_map == INVALID_ID)  continue;

        int16 out_id = get_out_id(*flow_map);
        out_session_t* ses = get_out_session(out_id);
        in_session_t* in_ses = get_in_session(get_in_id(*flow_map));
        data_hint(in_ses);
        data_hint(&ses->stat);    // Note: not sure this is useful

        out_session_lock(out_id);
        if (session_is_used(ses->state) &&
            session_is_enabled(ses->state) &&
            !session_is_no_resources(ses->state) && 
            !session_is_no_resources(in_ses->state)) {

            // Process the current output session
            process_out_session(ses, in_ses, update_pmt_flag,
                                future_out_time_limit, past_out_time_limit);
        }
        out_session_unlock(out_id);
    }

    // Send output commands to output FPGA
    process_outcmd_for_flow(qam->id, PSI_FLOW_ID);
}


// Update average output bitrate
//
void update_avg_out_bitrate (out_session_t *ses, int32 time_elapsed)
{
    uint32 tp_cnt = ses->stat.forward_tp_cnt + ses->stat.insert_tp_cnt;
    if (time_elapsed > 0) {
        int32 tps_processed = (int32)(tp_cnt - ses->prev_tp_cnt);
        uint32 bitrate = ((int64)tps_processed * TP_SIZE_BIT * BASE_FREQ)
                                 / ((int64) time_elapsed);
        ses->avg_bitrate += ((int32)bitrate - (int32) ses->avg_bitrate) >> 6;

        if (bitrate < ses->min_bitrate) {
            ses->min_bitrate = bitrate;
        }
        if (bitrate > ses->max_bitrate) {
            ses->max_bitrate = bitrate;
        }
    }

    // update stats for next time
    ses->prev_tp_cnt = tp_cnt;
}


// Monitor qam channel
//
void video_monitor_qam (qam_info_t *qam)
{
    uint32 now = sys_clk.base;
    int32 time_elapsed = (int32)(now - qam->prev_bitrate_update_time);

    int i;
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);
    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID)  continue;

        int16 out_id = get_out_id(flow_map[i]);
        out_session_t* ses = get_out_session(out_id);
        // TODO: do we need to lock out session here?

        update_avg_out_bitrate(ses, time_elapsed);
    }

    if (time_elapsed > 0) {
        int32 tps_processed = (int32)(qam->stat.mux_tp_cnt - qam->prev_tp_cnt);
        uint32 bitrate = ((int64)tps_processed * TP_SIZE_BIT * BASE_FREQ)
                                 / ((int64) time_elapsed);
        qam->stat.bitrate += ((int32)bitrate - (int32)qam->stat.bitrate) >> 6;
    }

    // update stats for next time
    qam->prev_bitrate_update_time = now;
    qam->prev_tp_cnt = qam->stat.mux_tp_cnt;
}

