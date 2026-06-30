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
#include "crc.h"
#include "video.h"
#include "video_util.h"

static uint8 C0_table[] = {0xC0, 0x00, 0x15, 0x00, 0xaa, 0xbb, 0x00, 0x01, 
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D};

// Copy PMT info
static
void pmt_info_copy (pmt_info_t *to_pmt, pmt_info_t *from_pmt)
{
    psi_section_t* from_sect = from_pmt->sect;
    psi_section_t* to_sect = to_pmt->sect;

    *to_pmt = *from_pmt;

    to_pmt->sect = to_sect;
    to_sect->crc = from_sect->crc;
    memcpy(to_sect->sect_hdr, from_sect->sect_hdr,
           psi_get_section_size(from_sect->sect_hdr));
}


// Schedule to insert a carousel
//   Returns number of TP inserted
//   If out_pid == INVALID_PID, no pid remap will be done.
//
int crsl_insert_schedule (crsl_insert_t *insert, uint16 qam_id,
                          uint16 flow_id, uint32 offset, uint16 out_pid)
{
    que_elem_t *elem;
    boolean pid_remap = (out_pid != INVALID_PID);
    crsl_t *crsl;
    uint16* cmdbuf_avail = &get_qam_info(qam_id)->cmdbuf_avail[flow_id];

#if 1
    assert(insert);
    assert(insert->crsl);
#else
    if (!insert || !insert->crsl) {
        VIDEO_DEBUG("\nLogger locked inside crsl_insert_schedule()\n");
        logger_lock(video_log);
        return 0;
    }
#endif

    crsl = insert->crsl;
    if (!crsl->used_flag) {
        return 0;
    }

    if (*cmdbuf_avail <= crsl->num_tp) {
        *cmdbuf_avail = video_get_flow_avail_space(qam_id, flow_id);

        if (*cmdbuf_avail <= crsl->num_tp) {
            VIDEO_OUT_DEBUG(", NO-CMD %d", *cmdbuf_avail);
//            VIDEO_LOG("Skipped carousel insertion due to output block");
            return 0;
        }
    }

    int tp_cnt = 0;
    FOR_ALL_ELEMENTS_IN_QUE(&crsl->tp_list, elem) {
        // Set up output command
        uintptr_t tp_addr = (uintptr_t)(elem + 1);
        out_command_t* cmd
                = setup_outcmd(qam_id, flow_id, sys_clk.base & OUT_TIME_MASK,
                               tp_addr + offset);
        if (cmd == NULL) {
            VIDEO_LOG("DEBUG: cmd ptr is NULL qam/flow %d/%d",
                      qam_id, flow_id);
             return 0;
        }
        if (pid_remap) {
            outcmd_set_pid_restamp(cmd, out_pid);
        }
        outcmd_set_cc_restamp(cmd, insert->next_cc++);
        insert->next_cc &= 0xF;
        tp_cnt++;

#if VIDEO_DIAG
        if (qam_id == diag_qam_id) {
            video_diag_prepare(flow_id, NULL, NULL, (void *) cmd, tp_addr);
        }
#endif
    }

    (*cmdbuf_avail) -= crsl->num_tp;

    insert->cnt++;
    insert->next_time += insert->period;

    // Make sure that we are scheduling this in the future
    if (insert->next_time < sys_clk.base) {
        insert->next_time = sys_clk.base + insert->period;
    }

    return tp_cnt;
}


// Set original PIDs for out_prog
//
static
void out_prog_set_orig (out_prog_t *prog)
{
    pmt_info_t* pmt = &prog->pmt;
    pmt_header_t* pmt_hdr = (pmt_header_t*)(pmt->sect + 1);
    pmt->orig_pcr_pid = pmt_get_pcr_pid(pmt_hdr);

    // ES pids
    int i;
    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);
        pmt->es[i].orig_pid = pmt_get_es_pid(es_info);
    }

    // CA pids
    for (i=0; i<pmt->ca_desc_cnt; i++) {
        ca_descriptor_header_t* ca_desc = pmt_info_get_ca_desc(pmt, i);
        pmt->ca_desc[i].orig_pid = ca_desc_get_pid(ca_desc);
    }
}


// Remap PIDs of a program
//   If it fails to find available pid, the pid will be set as NULL PID.
//   Returns:
//     - ENOENT if matching program is not found.
//     - ENOMEM if PID range runs out.
//     - EOK otherwise.
//
static
int pmt_modify (out_prog_t *prog, out_session_t *ses)
{
    pmt_info_t* pmt = &prog->pmt;
    pmt_header_t* pmt_hdr = (pmt_header_t*)pmt->sect->sect_hdr;

    pmt->orig_pcr_pid = pmt_get_pcr_pid(pmt_hdr);

    pmt_set_prog_num(pmt_hdr, prog->prog_num);
    pmt_hdr->ver = prog->pmt_ver;

    // Find pid range from config
    in_config_t* in_cfg = ses->in_ses->cfg;

    int idx = 0;
    if (in_cfg->mpts_flag) {
        // TODO: should use the prog->cfg_idx instead!!

        idx = video_list_search(prog->in_prog_num, ses->cfg->in_prog_list,
                                ses->cfg->prog_cnt);
        if (idx == -ENOENT) {
            VIDEO_DEBUG("\npmt_modify: can't find in prog num %d",
                        prog->in_prog_num);
            return ENOENT;
        }
    }

    video_pid_range_t* pid_range = &ses->cfg->pid_range[idx];
    uint16 next_pid = pid_range->first;
    uint16 pid;

    // Remap ES pids
    int i;
    uint16 orig_pid;
    out_pid_info_t *pid_info;

    for (i=0; i<pmt->es_cnt; i++, pid++) {
        pmt_es_info_t* es_info = pmt_info_get_es(pmt, i);

        orig_pid = pmt_get_es_pid(es_info);
        pmt->es[i].orig_pid = orig_pid;

        pid_info = &ses->pid_tab[orig_pid];
        if (pid_info->reversed) {       // input pid is already remapped
            pid = pid_info->pid;

        } else {
            // allocate next available PID
            pid = qam_find_next_avail_pid(next_pid, pid_range->last,
                                          ses->qam_id);
            if (pid == NULL_PID) {
                // rc = ENOMEM;
                return ENOMEM;
            } else {
                next_pid = pid + 1;
            }
        }

        pmt_set_es_pid(es_info, pid);

        if (orig_pid == pmt->orig_pcr_pid) {
            pmt->pcr_pid = pid;
            pmt_set_pcr_pid(pmt_hdr, pid);
        }
    }

    for (i=0; i<pmt->ca_desc_cnt; i++) {
        ca_descriptor_header_t* ca_desc = pmt_info_get_ca_desc(pmt, i);

        if (pmt->ca_desc[i].processed_flag) {
            continue;    // ca_desc has been processed
        }

        // Get the input CA PID
        orig_pid = ca_desc_get_pid(ca_desc);
        pmt->ca_desc[i].orig_pid = orig_pid;

        pid_info = &ses->pid_tab[orig_pid];
        if (pid_info->reversed) {       // input pid is already remapped
            pid = pid_info->pid;

        } else {
            // allocate CA PID
            pid = qam_find_next_avail_pid(next_pid, pid_range->last,
                                          ses->qam_id);
            assert(pid != INVALID_PID);
            next_pid = pid + 1;
        }
        ca_desc_set_pid(ca_desc, pid);

        // Search for the same CA PID below
        pmt->ca_desc[i].processed_flag = 1;
        int j;
        for (j=i+1; j<pmt->ca_desc_cnt; j++) {
            ca_descriptor_header_t* ca_desc2 = pmt_info_get_ca_desc(pmt, j);
            if (ca_desc_get_pid(ca_desc2) == orig_pid) {
                ca_desc_set_pid(ca_desc2, pid);
                pmt->ca_desc[j].processed_flag = 1;
            }
        }
    }

    return EOK;
}


// Packetize a PSI section into MPEG transport packets
//   Return number of TPs used.
//
int psi_section_packetize (psi_section_t *sect, uint16 pid, que_elem_t *tp_list)
{
    int pusi = 1;                           // payload_unit_start_indicator
    que_elem_t* elem;
    tp_header_t* tp_hdr;
    uint8* tp_ptr;
    int remain_sect_size, remain_tp_size, copy_size;
    int num_tp = 0;

    uint8* sect_ptr = (uint8*)sect->sect_hdr;
    remain_sect_size = psi_get_section_size(sect->sect_hdr);

    while (1) {
        elem = pool_alloc(&psi_tp_buf_pool);
        if (!elem) {
            em_vidman_VIDEO_PACKET_ALLOC_FAILED("PSI packetize");
            assert ( 0 );
        }

        que_put(NULL, tp_list, elem);
        num_tp++;

        tp_hdr = (tp_header_t*)(elem + 1);
        tp_ptr = (uint8*)(tp_hdr + 1);
        remain_tp_size = TP_SIZE - CRC_SIZE;

        tp_hdr->sync = SYNC_BYTE;
        tp_hdr->transport_err = 0;
        tp_hdr->payload_unit_start = pusi;
        tp_hdr->priority = 0;
        tp_set_pid(tp_hdr, pid);
        tp_hdr->scrambling_ctrl = 0;
        tp_hdr->af_ctrl = 1;

        if (pusi) {
            *tp_ptr++ = 0;                  // pointer field
            remain_tp_size--;
        }

        copy_size = remain_sect_size;
        if (copy_size > remain_tp_size) {
            copy_size = remain_tp_size;
        }

        // Copy section data into TP buffer
        memcpy(tp_ptr, sect_ptr, copy_size);

        sect_ptr += copy_size;
        remain_tp_size -= copy_size;
        if (remain_tp_size > 0) {
            memset(tp_ptr + copy_size, 0xFF, remain_tp_size);
        }

        remain_sect_size -= copy_size;
        if (remain_sect_size == 0) {
            break;
        }

        pusi = 0;
    }

    return num_tp;
}

// Packetize a PSI section into MPEG transport packets
//   Return number of TPs used.
//
int C0_section_packetize (uint16 pid, que_elem_t *tp_list)
{
    int pusi = 1;                           // payload_unit_start_indicator
    que_elem_t* elem;
    tp_header_t* tp_hdr;
    uint8* tp_ptr;
    int remain_sect_size, remain_tp_size, copy_size;
    int num_tp = 0;

    uint8* sect_ptr = (uint8*)C0_table;
    remain_sect_size = sizeof(C0_table);

    while (1) {
        elem = pool_alloc(&psi_tp_buf_pool);
        if (!elem) {
            em_vidman_VIDEO_PACKET_ALLOC_FAILED("PSI packetize");
            assert ( 0 );
        }

        que_put(NULL, tp_list, elem);
        num_tp++;

        tp_hdr = (tp_header_t*)(elem + 1);
        tp_ptr = (uint8*)(tp_hdr + 1);
        remain_tp_size = TP_SIZE - CRC_SIZE;

        tp_hdr->sync = SYNC_BYTE;
        tp_hdr->transport_err = 0;
        tp_hdr->payload_unit_start = pusi;
        tp_hdr->priority = 0;
        tp_set_pid(tp_hdr, pid);
        tp_hdr->scrambling_ctrl = 0;
        tp_hdr->af_ctrl = 1;

        if (pusi) {
            *tp_ptr++ = 0;                  // pointer field
            remain_tp_size--;
        }

        copy_size = remain_sect_size;
        if (copy_size > remain_tp_size) {
            copy_size = remain_tp_size;
        }

        // Copy section data into TP buffer
        memcpy(tp_ptr, sect_ptr, copy_size);

        sect_ptr += copy_size;
        remain_tp_size -= copy_size;
        if (remain_tp_size > 0) {
            memset(tp_ptr + copy_size, 0xFF, remain_tp_size);
        }

        remain_sect_size -= copy_size;
        if (remain_sect_size == 0) {
            break;
        }

        pusi = 0;
    }

    return num_tp;
}

void add_C0_table_to_pmt_carousel(out_prog_t *prog, uint16 pmt_pid)
{
    uint32 C0_crc = 0;
    
    //Update the prog num in C0 table
    memcpy((char *)&C0_table[4], (char *) &prog->prog_num, sizeof(prog->prog_num));
    C0_crc = crc32(C0_table, sizeof(C0_table)-CRC_SIZE);
    memcpy((char *)&C0_table[sizeof(C0_table)-CRC_SIZE],
            (char *) &C0_crc, CRC_SIZE);

    // Generate PMT carousel with C0 table
    prog->pmt_crsl.num_tp += C0_section_packetize(pmt_pid,
                                              &prog->pmt_crsl.tp_list);
    VIDEO_DEBUG("inside (%s) Crsl PMT tp (post C0) = %u CRC %u\n", 
                __FUNCTION__, prog->pmt_crsl.num_tp, C0_crc);

}

// Collect info of all output programs in a QAM
//
static
void qam_collect_prog_info (qam_info_t *qam)
{
    int i;
    int k = 0;
    uint16 out_id;
    boolean changed = FALSE;
    out_session_t *ses;
    out_prog_t *prog;
    pat_prog_info_t* prog_info = qam->prog_info;
    int32* flow_map = get_qam_flow_map(video_ctx, qam->id);

    for (i=0; i<MAX_VIDEO_FLOWS_PER_QAM; i++) {
        if (flow_map[i] == INVALID_ID) {
            continue;
        }

        out_id = get_out_id(flow_map[i]);
        out_session_lock(out_id);
        ses = get_out_session(out_id);

        if (session_is_used(ses->state)) {
            FOR_ALL_ELEMENTS_IN_QUE(&ses->prog_list, prog) {

                if (prog->filtered || prog->blocked)  continue;  // skip program

                if (prog->prog_num != pat_get_prog_num(prog_info) ||
                    prog->pmt_pid != pat_get_pmt_pid(prog_info)) {
                    changed = TRUE;
                    pat_set_prog_num(prog_info, prog->prog_num);
                    prog_info->resv = 0x7;
                    pat_set_pmt_pid(prog_info, prog->pmt_pid);
                }
                if (++k > MAX_PROGS_PER_MPTS) {
                    em_vidman_VIDEO_TOO_MANY_PROG_IN_QAM(qam->id);
                    return;          // TBD: Need better error handling
                }
                prog_info++;
            }
        }

        out_session_unlock(out_id);
    }

    if (k != qam->prog_cnt) {
        qam->prog_cnt = k;
        changed = TRUE;
    }

    if (changed) {
        qam->pat.ver = (qam->pat.ver + 1) & 0x1F;
    }
}


// Modify this to change how many programs per output PAT section
#define NUM_PROGS_PER_PAT_SECT          MAX_PROGS_PER_TS

// Build PAT carousel
//
void qam_build_pat (qam_info_t *qam)
{
    pat_prog_info_t* prog_info = qam->prog_info;

    // Calculate number of PAT sections needed (typically it will be 1)
    int sect_cnt = qam->prog_cnt / NUM_PROGS_PER_PAT_SECT;
    int last_sect_prog_cnt = qam->prog_cnt - sect_cnt * NUM_PROGS_PER_PAT_SECT;
    if (last_sect_prog_cnt > 0) {
        sect_cnt++;
    }
    if (sect_cnt == 0) {
        sect_cnt = 1;    // at least one section
    }

    if (qam->pat_built) {
        // Free PAT sections in old PAT
        int ver = qam->pat.ver;         // backup PAT version
        clear_pat(&qam->pat);
        qam->pat.ver = ver;             // restore PAT version

        // Free TP buffers in old PAT
        pool_free_list(&psi_tp_buf_pool, &qam->pat_crsl.tp_list);
        qam->pat_crsl.num_tp = 0;
    }

    int i;
    for (i=0; i<sect_cnt; i++) {
        int sect_prog_cnt = (i < sect_cnt - 1) ?
                                MAX_PROGS_PER_PAT_SECT : last_sect_prog_cnt;

        psi_section_t* sect = (psi_section_t*)pool_alloc(&psi_section_pool);
        if (!sect) {
            em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED("QAM PAT", qam->id);
            assert ( 0 );
        }

        // Fill PAT section header
        pat_header_t* pat_hdr = (pat_header_t*)sect->sect_hdr;
        pat_hdr->table_id = PAT_TABLE_ID;
        pat_hdr->sect_syntax = 1;
        pat_hdr->priv = 0;
        pat_hdr->resv1 = 0x3;
        pat_set_section_size(pat_hdr,
                (sect_prog_cnt << 2) + PAT_SECT_HDR_SIZE + CRC_SIZE);
        pat_set_tsid(pat_hdr, qam->cfg->tsid);
        pat_hdr->resv2 = 0x3;
        pat_hdr->ver = qam->pat.ver;    // also update PAT version number
        pat_hdr->cur_next = PSI_VER_CURRENT;
        pat_hdr->sect_num = i;
        pat_hdr->last_sect_num = sect_cnt - 1;

        // Copy program loop
        memcpy(pat_hdr + 1, prog_info, sizeof(pat_prog_info_t) * sect_prog_cnt);
        prog_info += sect_prog_cnt;

        // Calculate CRC32
        sect->crc = crc32((uint8*)pat_hdr,
                          pat_get_section_size(pat_hdr) - CRC_SIZE);
        psi_section_set_crc(sect, sect->crc);

        // Insert the section to PAT's session list
        qam->pat.sect[qam->pat.sect_cnt++] = sect;

        // Generate PAT carousel
        qam->pat_crsl.num_tp +=
                psi_section_packetize(sect, PAT_PID, &qam->pat_crsl.tp_list);
    }

    int rc = parse_pat(&qam->pat);
    if (rc != EOK) {
        em_vidman_VIDEO_PAT_BUILD_FAILED(qam->id);
    }

    qam->pat_built = TRUE;
}


void regenerate_pat (qam_info_t *qam)
{
    qam_collect_prog_info(qam);
    qam_build_pat(qam);
}


// Regenerate output PMT
//
int regenerate_pmt (out_session_t *ses, out_prog_t *prog, uint16 pmt_pid)
{
    assert(prog);

    prog->pmt_ver = (prog->pmt_ver + 1) % 32;

    // Initialize PMT change table
    pmt_change_t pmt_chg;
    memset(&pmt_chg, 0, sizeof(pmt_change_t));

    // Record ES delete actions
    video_ca_delete_pmt_es_pids(&pmt_chg, &prog->pmt);

    // Allocate PSI section for output PMT (PMT can only have 1 section)
    prog->pmt.sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!prog->pmt.sect) {
        em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED("Regen PMT", ses->id);
        return ENOMEM;
    }

    // Copy input PMT to output
    int rc = EOK;
    psi_lock(ses->in_ses->id);
    if (prog->in_prog->pmt_snooped) {
        pmt_info_copy(&prog->pmt, &prog->in_prog->pmt);
    } else {
        rc = EAGAIN;
    }
    psi_unlock(ses->in_ses->id);
    if (rc != EOK)  goto error;

    // Modify output PMT
    rc = pmt_modify(prog, ses);
    if (rc != EOK)  goto error;

    // Apply ca desc 
    if (qam_is_encrypt(ses->qam_id)) {
        video_ca_apply_desc(prog, ses->qam_id);
    }

    prog->pmt.sect->crc = crc32((uint8*)prog->pmt.sect->sect_hdr,
                                psi_get_section_size(prog->pmt.sect->sect_hdr)
                                    - CRC_SIZE);
    psi_section_set_crc(prog->pmt.sect, prog->pmt.sect->crc);

    // Generate PMT carousel
    prog->pmt_crsl.num_tp = psi_section_packetize(prog->pmt.sect, pmt_pid,
                                                  &prog->pmt_crsl.tp_list);

    if (is_PME_session(prog)) {
        add_C0_table_to_pmt_carousel(prog, pmt_pid);
    }

    if (qam_is_encrypt(ses->qam_id)) {
        // Record ES add actions
        video_ca_add_pmt_es_pids(&pmt_chg, &prog->pmt);
        int chg_cnt = video_ca_report_pmt_change(ses->qam_id, &pmt_chg);
        if (chg_cnt) {
            video_ca_send_update_service(ses->qam_id, prog->prog_num);
        }
    }

    // Schedule PMT carousel right away
    prog->pmt_crsl.next_time = sys_clk.base;
    prog->pmt_built = TRUE;
    if (!ses->cfg->encrypt_flag || prog->pmt.ca_desc_cnt > 0) {
        ses->stat.insert_tp_cnt +=
            psi_crsl_insert_schedule(&prog->pmt_crsl, ses->qam_id, PSI_FLOW_ID,
                                 psi_tp_buf_offset, INVALID_PID);
    }

    ses->pmt_generated_cnt++;
    if ( ses->pmt_generated_cnt == ses->in_ses->pat.prog_cnt ) {
        ses->state |= SESSION_STATE_PSI_READY;
    }

    return EOK;

error:
    pool_free(&psi_section_pool, &prog->pmt.sect->link);
    return rc;
}


// Pass through new input PMT to output
//
int passthru_pmt (out_session_t *ses, out_prog_t *prog)
{
    // Allocate PSI section for output PMT (PMT can only have 1 section)
    prog->pmt.sect = (psi_section_t*)pool_alloc(&psi_section_pool);
    if (!prog->pmt.sect) {
        em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED("Passthru PMT", ses->id);
        assert ( 0 );
    }

    // Copy input PMT section to output
    int rc = EOK;
    psi_lock(ses->in_ses->id);
    if (prog->in_prog->pmt_snooped) {
        pmt_info_copy(&prog->pmt, &prog->in_prog->pmt);
    } else {
        rc = EAGAIN;
    }
    psi_unlock(ses->in_ses->id);
    if (rc != EOK) {
        pool_free(&psi_section_pool, &prog->pmt.sect->link);
        return rc;
    }

    out_prog_set_orig(prog);
    prog->pmt_ver = prog->pmt.ver;
    prog->pmt_built = TRUE;

    VIDEO_DEBUG("\nPassthru PMT: Ses %d, prog %d: filtered %d, blocked %d",
                ses->id, prog->prog_num, prog->filtered, prog->blocked);

    ses->pmt_generated_cnt++;
    if ( ses->pmt_generated_cnt == ses->in_ses->pat.prog_cnt ) {
        ses->state |= SESSION_STATE_PSI_READY;
    }

    return EOK;
}


int psi_crsl_insert_schedule (psi_crsl_t *crsl, uint16 qam_id, uint16 flow_id,
                              uint32 offset, uint16 out_pid)
{
    uint16* cmdbuf_avail = &get_qam_info(qam_id)->cmdbuf_avail[flow_id];
    if (*cmdbuf_avail <= crsl->num_tp) {
        *cmdbuf_avail = video_get_flow_avail_space(qam_id, flow_id);
        if (*cmdbuf_avail <= crsl->num_tp) {
            VIDEO_OUT_DEBUG(", NO-CMD %d", *cmdbuf_avail);
            return 0;
        }
    }

    int tp_cnt = 0;
    boolean pid_remap = (out_pid != INVALID_PID);

    que_elem_t *elem;
    FOR_ALL_ELEMENTS_IN_QUE(&crsl->tp_list, elem) {
        // Set up output command
        uintptr_t tp_addr = (uintptr_t)(elem + 1);
        out_command_t* cmd
                = setup_outcmd(qam_id, flow_id, sys_clk.base & OUT_TIME_MASK,
                               tp_addr + offset);
        if (cmd == NULL) {
            VIDEO_LOG("DEBUG: cmd ptr is NULL qam/flow %d/%d",
                      qam_id, flow_id);
             return 0;
        }
        if (pid_remap) {
            outcmd_set_pid_restamp(cmd, out_pid);
        }
        outcmd_set_cc_restamp(cmd, crsl->next_cc++);
        crsl->next_cc &= 0xF;
        tp_cnt++;

#if VIDEO_DIAG
        if (qam_id == diag_qam_id) {
            video_diag_prepare(flow_id, NULL, NULL, (void *) cmd, tp_addr);
        }
#endif
    }

    (*cmdbuf_avail) -= crsl->num_tp;

    crsl->next_time += crsl->period;
    // Make sure that we are scheduling this in the future
    if (crsl->next_time < sys_clk.base) {
        crsl->next_time = sys_clk.base + crsl->period;
    }

    return tp_cnt;
}

