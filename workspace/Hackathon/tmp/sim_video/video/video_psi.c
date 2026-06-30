/*
 * Copyright (c) 2006-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "crc.h"
#include "video.h"
#include "video_util.h"


// Snoop a PSI section
//   Returns TRUE if a complete section is snooped.
//   In this case this routine should be call again with the same TP.
//   Otherwise return FALSE.
//
boolean snoop_section (psi_snoop_t *snoop, tp_header_t *tp_hdr)
{
    uint8* tp_ptr = snoop->tp_ptr;
    int tp_byte_left;                   // remaining # bytes in TP
    boolean seg_end = FALSE;            // completed segment?
    int seg_size;                       // current segment size
    psi_section_header_t *sect_hdr;
    boolean priv_flag;
    int max_size;

    if (!tp_ptr) {
        // This is a new TP for snooping

        // Skip TP if it has no payload
        if (!(tp_hdr->af_ctrl & 1)) {
            return FALSE;
        }

        // Skip adaptation field if present
        tp_ptr = (uint8*)(tp_hdr + 1);
        if ((tp_hdr->af_ctrl & 2)) {
            tp_ptr += (*tp_ptr) + 1;
        }

        tp_byte_left = TP_SIZE - (tp_ptr - (uint8*)tp_hdr);
        if (tp_byte_left <= 0) {
            goto done_tp;
        }

        seg_size = tp_byte_left;

        if (tp_hdr->payload_unit_start) {
            // PUSI = 1: PTR field presents
            seg_size = *tp_ptr++;
            tp_byte_left--;

            if (seg_size >= tp_byte_left) {
                // PSI session syntax error.  Drop TP
                //printf("Bad PSI section: bad pointer value.\n");
                goto done_tp;
            }

            if (!snoop->sect) {
                // skip the first segment
                tp_ptr += seg_size;
                tp_byte_left -= seg_size;
                seg_size = tp_byte_left;

            } else {
                seg_end = TRUE;
            }

        } else {
            // PUSI = 0
            if (!snoop->sect) {
                // No section starts in this TP.  Wait for next chance.
                goto done_tp;
            }
        }

    } else {
        tp_byte_left = snoop->tp_byte_left;
        seg_size = tp_byte_left;
    }

continue_tp:
    if (((seg_size <= 0) && !seg_end) ||                // end of TP reached
        ((!snoop->sect) && (*tp_ptr == STUFF_BYTE))     // no more segments
       ) {
        goto done_tp;
    }

    // At this point, tp_ptr, seg_size, and seg_end are all set for
    // the current segment to process

    if (!snoop->sect) {
        // Start snooping a new section
        // Allocate PSI section buffer
        psi_section_alloc(snoop, *tp_ptr);
        if (!snoop->sect) {
            em_vidman_VIDEO_PSI_SECTION_ALLOC_FAILED("Snoop", *tp_ptr);
            goto done_tp;
        }

        // Copy the first segment into the section buffer
        memcpy(((uint8*)snoop->sect->sect_hdr), tp_ptr, seg_size);
        snoop->snooped_len = seg_size;

    } else {
        // Check for oversize
        int seg_size2 = seg_size;

        sect_hdr = snoop->sect->sect_hdr;
        priv_flag = (sect_hdr->table_id > PMT_TABLE_ID);
        max_size = priv_flag? MAX_PRIVATE_SECT_SIZE : MAX_PSI_SECT_SIZE;

        if (snoop->snooped_len + seg_size > max_size) {
            // Truncate the segment!
            seg_size2 = max_size - snoop->snooped_len;
        }

        // Append the segment into the section buffer
        memcpy(((uint8*)sect_hdr) + snoop->snooped_len, tp_ptr, seg_size2);
        snoop->snooped_len += seg_size2;
    }

    // Update for remaining segment in TP
    tp_ptr += seg_size;
    tp_byte_left -= seg_size;
    seg_size = tp_byte_left;


    // Examining current snooped section
    //
    sect_hdr = snoop->sect->sect_hdr;

    // Check if the section length is known
    if (snoop->snooped_len >= PSI_SECT_LEN_ADJ) {
        int sect_len = psi_get_section_size(sect_hdr);
        int sect_byte_missing = sect_len - snoop->snooped_len;

        priv_flag = (sect_hdr->table_id > PMT_TABLE_ID);
        max_size = priv_flag? MAX_PRIVATE_SECT_SIZE : MAX_PSI_SECT_SIZE;

        // Check if the session size is valid
        if (sect_len > max_size) {
            // Drop oversized PSI section
            // printf("Bad PSI section: too big\n");
            psi_section_free(snoop);
            snoop->snooped_len = 0;

            if (seg_end) {
                seg_end = FALSE;
                goto continue_tp;
            }
            goto done_tp;
        }

        // Check if the section is complete
        if (sect_byte_missing > 0) {
            if (seg_end) {
                // PSI section syntax error.  Drop the section.
                // printf("Bad PSI section: mismatched length\n");
                psi_section_free(snoop);
                snoop->snooped_len = 0;
    
                // Continue with other segments in the TP
                seg_end = FALSE;
                goto continue_tp;
            }

            // Wait for more data
            goto done_tp;
        }

        // Section snoop completed

        // Read CRC from the section
        if (snoop->sect->sect_hdr->sect_syntax) {
            psi_section_get_crc(snoop->sect);
        }

        // Backtracking tp_ptr and tp_byte_left for remainnig segments in TP
        tp_ptr += sect_byte_missing;
        tp_byte_left -= sect_byte_missing;

        snoop->tp_ptr = tp_ptr;
        snoop->tp_byte_left = tp_byte_left;
        snoop->snooped_len = 0;
        return TRUE;

    } else {
        // PSI section length not known yet
        if (seg_end) {
            // PSI section syntax error.  Drop the section.
            // printf("Bad PSI section: mismatch length\n");
            psi_section_free(snoop);
            snoop->snooped_len = 0;

            // Continue with other segments in the TP
            goto continue_tp;
        }
    }

done_tp:
    // End of TP reached
    snoop->tp_ptr = NULL;
    return FALSE;
}


// Snoop a PAT section
//   Return TRUE if a new PAT section is completely snooped.
//   Otherwise returns FALSE.
//
boolean snoop_pat_section (in_session_t *in_ses)
{
    psi_section_t* sect = in_ses->snoop.sect;
    psi_section_header_t* sect_hdr = sect->sect_hdr;
    if (sect_hdr->table_id != PAT_TABLE_ID) {
        // Drop any non-PAT section in pid 0
        em_vidman_VIDEO_BAD_TABLE_IN_PAT(sect_hdr->table_id, in_ses->id);
        goto drop_section;
    }

    if (sect_hdr->cur_next == PSI_VER_NEXT) {
        // Note: To simplify snoop code, we currently ignore future PSI version.
        //       We will process it when the next version becomes current!
        goto drop_section;
    }

    // Check number of PAT sections
    if (sect_hdr->last_sect_num >= MAX_PAT_SECT) {
        em_vidman_VIDEO_TOO_MANY_SECTION_IN_PAT(in_ses->id, sect_hdr->last_sect_num);
        goto drop_section;
    }

    // Check current section number
    if (sect_hdr->sect_num > sect_hdr->last_sect_num) {
        em_vidman_VIDEO_BAD_PSI_SECTION_NUM(sect_hdr->sect_num, 
                                            in_ses->id, 
                                            sect_hdr->last_sect_num);
        goto drop_section;
    }

    // Get the corresponding section in current PAT
    psi_section_t* s = in_ses->pat.sect[sect_hdr->sect_num];

    if (in_ses->pat_snooped                     // PAT already snooped
        && sect_hdr->ver == in_ses->pat.ver     // PAT version matches
        && s && sect->crc == s->crc             // CRC matches
       ) {
        // Ignore repeated PAT section
        goto drop_section;
    }

    VIDEO_PSI_DEBUG("\nIn %d: New PAT: sect %d/%d, ver %d",
                    in_ses->id, sect_hdr->sect_num,
                    sect_hdr->last_sect_num, sect_hdr->ver);

    // CRC check
    if (crc32((uint8*)sect_hdr, psi_get_section_size(sect_hdr))) {
        // CRC check fails
        em_vidman_VIDEO_PSI_CRC_ERROR("PAT", in_ses->id);
        VIDEO_PSI_DEBUG("\nIn %d: CRC error in PAT", in_ses->id);
        goto drop_section;
    }

    if ((sect_hdr->ver != in_ses->pat.ver)      // PAT version changes
            || (s && sect->crc != s->crc))      // CRC changes
    {
        in_ses->pat_snooped = FALSE;
        in_ses->state &= ~SESSION_STATE_PSI_READY;
        in_ses->pat_ver = sect_hdr->ver;

        pat_info_t* pat = &in_ses->pat;
        if (pat->ver != PSI_VER_UNKNOWN) {
            VIDEO_PSI_DEBUG("\nIn %d PAT sect %d: ver %d->%d, CRC %08x->%08x",
                            in_ses->id, sect_hdr->sect_num, pat->ver,
                            sect_hdr->ver, s->crc, sect->crc);
            clear_pat(pat);
        }

        pat->sect_cnt = sect_hdr->last_sect_num + 1;

        // Save the PAT section
        pat->sect[sect_hdr->sect_num] = sect;
        pat->sect_snooped++;

        return (pat->sect_snooped == pat->sect_cnt);
    }

drop_section:
    // Drop the section
    psi_section_free(&in_ses->snoop);
    return FALSE;
}


// Parse PAT
//
int parse_pat (pat_info_t *pat)
{
    int i, j;

    pat->prog_cnt = 0;
    pat->nit_pid = INVALID_PID;

    for (i=0; i<pat->sect_cnt; i++) {
        psi_section_t* sect = pat->sect[i];
        pat_header_t* pat_hdr = (pat_header_t*)sect->sect_hdr;
        pat_prog_info_t* prog_info = (pat_prog_info_t*)(pat_hdr + 1);
        int sect_prog_cnt = (pat_get_section_size(pat_hdr) - 12) / 4;

        // Check for oversubscription of number of programs (for pat_info_t)
        if (sect_prog_cnt > MAX_PROGS_PER_TS) {
            em_vidman_VIDEO_TOO_MANY_PROG_IN_PAT("Snoop", sect_prog_cnt);
            return EINVAL;
        }

        // Set TSID and PAT version
        if (pat_hdr->sect_num == 0) {
            pat->tsid = pat_get_tsid(pat_hdr);
            pat->ver = pat_hdr->ver;
        }

        for (j=0; j<sect_prog_cnt; j++) {
            if (pat_get_prog_num(prog_info) == 0) {
                if (pat->nit_pid != INVALID_PID) {
                    em_vidman_VIDEO_TOO_MANY_NIT_IN_PAT();
                    return EINVAL;
                }

                pat->nit_pid = pat_get_pmt_pid(prog_info);
                prog_info++;
                continue;        // skip NIT
            }

            pat->prog_info[pat->prog_cnt++] = (pat_prog_info_t*)prog_info++;
        }
    }

    return EOK;
}


// Update PAT of an in section
//
int update_pat (in_session_t *in_ses)
{
    int i;
    for (i=0; i<in_ses->pat.prog_cnt; i++) {
        pat_prog_info_t* prog_info = in_ses->pat.prog_info[i];

        // Allocate input program
        in_prog_t* prog = in_prog_alloc();
        assert(prog);
        prog->pmt_pid = pat_get_pmt_pid(prog_info);
        prog->prog_num = pat_get_prog_num(prog_info);
        que_put(NULL, &in_ses->prog_list, &prog->link);

        // Set up PMT pid in input pid table
        in_pid_info_t* pid_info = &in_ses->pid_tab[prog->pmt_pid];
        pid_info->has_psi = 1;
        pid_info->referenced = 1;
    }
    return EOK;
}


// Snoop a PMT section
//   Return TRUE if a new PMT section is snooped.
//   Otherwise returns FALSE.
//
boolean snoop_pmt_section (in_session_t *in_ses, in_prog_t **prog)
{
    in_prog_t* prog2 = *prog;
    psi_section_t* sect = prog2->snoop.sect;
    psi_section_header_t* sect_hdr = sect->sect_hdr;

    if (sect_hdr->cur_next == PSI_VER_NEXT) {
        // Note: To simplify snoop code, we currently ignore future PSI version.
        //       We will process it till the next version becomes current!
        goto drop_section;
    }

    // Make sure we have the matching program
    uint16 prog_num = pmt_get_prog_num((pmt_header_t*)sect_hdr);
    if (prog_num != prog2->prog_num) {
        prog2 = in_prog_lookup_by_prog_num(&in_ses->prog_list, prog_num);
        if (!prog2) {
            em_vidman_VIDEO_UNKNOWN_PROG_NUM("Snoop", prog_num, in_ses->id);
#if 0
            // Turn on PSI recording for this session (debugging only)
            if (psi_record.in_id == INVALID_SES_ID) {
                if (strlen(psi_record.filename) == 0) {
                    psi_record_init(&psi_record, "/harddisk/tmp/mips/psi.cap",
                                    1000);
                }
                psi_record_start(&psi_record, in_ses->id);
            }
#endif
            goto drop_section;
        }
        *prog = prog2;
    }

    if (prog2->pmt_ver != PSI_VER_UNKNOWN) {
        if ((prog2->pmt_ver == sect_hdr->ver) &&
            (prog2->pmt.sect->crc == sect->crc)) {
            // PMT not changed.  No need to resnoop.
            goto drop_section;
        }
    }

    VIDEO_PSI_DEBUG("\nIn %d: New PMT: ver %d, CRC %08x",
                    in_ses->id, sect_hdr->ver, sect->crc);

    // Sanity check new PMT table
    if (sect_hdr->last_sect_num != 0) {
        em_vidman_VIDEO_TOO_MANY_PMT_SECTIONS("Snoop", 
                                              in_ses->id, 
                                              sect_hdr->last_sect_num+1);
        goto drop_section;
    }

    // Input PMT CRC check
    if (crc32((uint8*)sect_hdr, psi_get_section_size(sect_hdr))) {
        em_vidman_VIDEO_PSI_CRC_ERROR("PMT", in_ses->id);
        goto drop_section;
    }

    return TRUE;

drop_section:
   // Drop the section
    psi_section_free(&(*prog)->snoop);
   return FALSE;
}


// Parse PMT table
//
int parse_pmt (pmt_info_t *pmt, psi_section_t *sect)
{
    uint8* ptr = (uint8*)sect->sect_hdr;
    pmt_header_t* pmt_hdr = (pmt_header_t*)ptr;
    uint8* esinfo_end_addr = ptr + pmt_get_section_size(pmt_hdr) - CRC_SIZE;

    pmt->sect = sect;
    pmt->ver = pmt_hdr->ver;
    pmt->pcr_pid = pmt_get_pcr_pid(pmt_hdr);
    pmt->es_cnt = 0;
    pmt->ca_desc_cnt = 0;

    ptr += PMT_SECT_HDR_SIZE;
    int info_len = pmt_get_prog_info_len(pmt_hdr);

    // Scan prog_info for CA descriptor
    while (info_len > 0) {
        descriptor_header_t* desc = (descriptor_header_t*)ptr;
        if (desc->tag == DESC_TAG_CA) {
            if (pmt->ca_desc_cnt >= MAX_CA_DESC_PER_PROG) {
                em_vidman_VIDEO_TOO_MANY_CA_DESC("Snoop",
                                         pmt->ca_desc_cnt,
                                         MAX_CA_DESC_PER_PROG);
                return EINVAL;
            }
            pmt_info_set_ca_desc(pmt, pmt->ca_desc_cnt++,
                                 (ca_descriptor_header_t*)desc);
        }
        ptr += sizeof(descriptor_header_t) + desc->len;
        info_len -= sizeof(descriptor_header_t) + desc->len;
    }

    if (info_len != 0) {
        em_vidman_VIDEO_BAD_PROG_DESC("Snoop");
        return EINVAL;
    }

    // ES loop
    while (ptr < esinfo_end_addr) {
        pmt_es_info_t* es_info = (pmt_es_info_t*)ptr;
        if (pmt->es_cnt >= MAX_ES_PER_PROG) {
            em_vidman_VIDEO_TOO_MANY_ES("ES",
                                        pmt->es_cnt,
                                        MAX_ES_PER_PROG);
            return EINVAL;
        }
        pmt_info_set_es(pmt, pmt->es_cnt++, es_info);

        info_len = pmt_get_es_info_len(es_info);
        ptr += PMT_ES_HDR_SIZE;

        // Scan es_info for CA descriptor
        while (info_len > 0) {
            descriptor_header_t* desc = (descriptor_header_t*)ptr;
            if (desc->tag == DESC_TAG_CA) {
                if (pmt->ca_desc_cnt >= MAX_CA_DESC_PER_PROG) {
                    em_vidman_VIDEO_TOO_MANY_CA_DESC("Snoop",
                                         pmt->ca_desc_cnt,
                                         MAX_CA_DESC_PER_PROG);
                    return EINVAL;

                }
                pmt_info_set_ca_desc(pmt, pmt->ca_desc_cnt++, 
                                     (ca_descriptor_header_t*)desc);
            }
            ptr += sizeof(descriptor_header_t) + desc->len;
            info_len -= sizeof(descriptor_header_t) + desc->len;
        }

        if (info_len != 0) {
            em_vidman_VIDEO_BAD_ES_DESC("Snoop");
            return EINVAL;
        }
    }

    // Make sure total PID count does not exceed the limit
    if (pmt->es_cnt + pmt->ca_desc_cnt > MAX_ES_PER_PROG) {
        em_vidman_VIDEO_TOO_MANY_ES("ES+CAT",
                                    pmt->es_cnt + pmt->ca_desc_cnt,
                                    MAX_ES_PER_PROG);
        return EINVAL;
    }

    return EOK;
}


// Snoop a private section
//   Return TRUE if a new private section is snooped.
//   Otherwise returns FALSE.
//
static
boolean snoop_private_section (in_prog_t *prog)
{
    psi_section_t* sect = prog->snoop.sect;
    psi_section_header_t* sect_hdr = sect->sect_hdr;
    psi_section_t *s = psi_sect_lookup_by_table_id(&prog->priv_sect_list,
                                                   sect_hdr->table_id);
    if (!s) {
        // New private section
        return TRUE;
    }

    if (sect_hdr->sect_syntax != s->sect_hdr->sect_syntax) {
        // Treat this as new private section
        goto replace_private_section;
    }

    if (sect_hdr->sect_syntax &&
        (sect_hdr->ver != s->sect_hdr->ver || sect->crc != s->crc)) {
        // Private sections (with generic section syntax) that changes
        goto replace_private_section;
    }

    // Drop the session for all other cases
    // - Private section using generic section syntax, but with no change
    //        in version and CRC
    // - Private section using proprietary syntax (to save CPU cycles,
    //       we only resnoop these sections when PMT change is detected)
    //
    psi_section_free(&prog->snoop);
    return FALSE;

replace_private_section:
    // Remove old private table
    que_deque(NULL, &s->link);
    pool_free(&private_section_pool, &s->link);
    return TRUE;
}


// Parse a PAT TP
//
void parse_pat_tp (in_session_t *in_ses, tp_info_t *tp_info)
{
    tp_header_t* tp_hdr = (tp_header_t*)(tp_info->addr -
                                  input_buf_offset[in_ses->cfg->input_port]);

    psi_record_tp(&psi_record, in_ses->id, tp_hdr);

    // PAT snooping
    while (snoop_section(&in_ses->snoop, tp_hdr)) {

        if (snoop_pat_section(in_ses)) {
            in_ses->stat.new_pat_cnt++;

            // if we've hit a reserved number, skip it
            if ( ++in_ses->pat_usage_id == PSI_NOT_READY_USAGE_ID ) {
                in_ses->pat_usage_id++;
            }

            in_ses->state &= ~SESSION_STATE_BLOCKED;

            int rc = parse_pat(&in_ses->pat);
            if (rc == EOK) {
                psi_lock(in_ses->id);
                in_session_cleanup_progs(in_ses);
                rc = update_pat(in_ses);
                psi_unlock(in_ses->id);
            }
            in_ses->pat_ver = in_ses->pat.ver;
            in_ses->pat_snooped = TRUE;
            in_ses->pmt_snooped_cnt = 0;

            if (rc != EOK) {
                in_ses->state |= SESSION_STATE_BLOCKED;
                em_vidman_VIDEO_PSI_BLOCKED("In PAT", in_ses->id, strerror(rc));
                in_ses->snoop.sect = NULL;
                break;
            }
        }

        // Clean up snoop context
        in_ses->snoop.sect = NULL;            
    }

    // if we've snooped the pat then it is safe to inject psi_info
    if (in_ses->pat_snooped) {
        advance_input_tp_info_wr(in_ses);
        psi_info_t* psi_info
                = (psi_info_t*)tp_info_ref_get(&in_ses->tp_info_ref);

        INFO_FLAGS(psi_info) = 0;
        psi_info->info_type = PSI_USAGE;
        psi_info->pat_flag = 1;
        psi_info->psi_usage_id = in_ses->pat_usage_id;
    }
}


// Parse a PMT TP
//
void parse_pmt_tp (in_session_t *in_ses, tp_info_t *tp_info)
{
    int err;
    tp_header_t* tp_hdr = (tp_header_t*)(tp_info->addr -
                                  input_buf_offset[in_ses->cfg->input_port]);

    psi_record_tp(&psi_record, in_ses->id, tp_hdr);

    // PMT snooping
    // Note: here we simply use the snoop context from the first program
    //       with a matching PMT PID.  However, this may not be the matching
    //       program since MPEG-2 allows multiple programs to share the
    //       same PMT PIDs.
    uint16 pid = tp_get_pid(tp_hdr);
    in_prog_t* prog = in_prog_lookup_by_pmt_pid(&in_ses->prog_list, pid);
    if (!prog) {
        em_vidman_VIDEO_PMT_NOT_FOUND("Snoop", pid, in_ses->id);
        return;
    }

    while (snoop_section(&prog->snoop, tp_hdr)) {
        psi_section_t* sect = prog->snoop.sect;

        if (sect->sect_hdr->table_id == PMT_TABLE_ID) {
            // This is a PMT table
            if (snoop_pmt_section(in_ses, &prog)) {
                in_ses->stat.new_pmt_cnt++;

                // if we've hit a reserved number, skip it
                if (++prog->pmt_usage_id == PSI_NOT_READY_USAGE_ID) {
                    prog->pmt_usage_id++;
                }

                psi_lock(in_ses->id);

                // Clean up for old PMT
                if (prog->pmt_snooped) {
                    pool_free(&psi_section_pool, &prog->pmt.sect->link);
                    in_ses->pmt_snooped_cnt--;
                    in_ses->state &= ~SESSION_STATE_PSI_READY;
                }

                prog->pmt_snooped = FALSE;

                // Parse PMT
                err = parse_pmt(&prog->pmt, sect);

                psi_unlock(in_ses->id);

                if (err != EOK) {
                    in_ses->state |= SESSION_STATE_BLOCKED;
                    em_vidman_VIDEO_PSI_BLOCKED("In PMT", in_ses->id, strerror(err));
                    VIDEO_DEBUG("\n\nIn %d: Bad PMT\n", in_ses->id);
                    psi_section_free(&prog->snoop);
                    continue;
                }

                // Set up reference PIDs in input pid table
                in_pid_table_setup_pmt(in_ses->pid_tab, &prog->pmt);

                prog->pmt_ver = prog->pmt.ver;
                prog->pmt_snooped = TRUE;
                in_ses->pmt_snooped_cnt++;

                // if we have snooped all PMTs
                if (in_ses->pmt_snooped_cnt == in_ses->pat.prog_cnt) {
                    in_ses->state |= SESSION_STATE_PSI_READY; 
                }
            }

            if (prog->pmt_snooped) {
                // Inject psi_info
                advance_input_tp_info_wr(in_ses);
                psi_info_t* psi_info
                    = (psi_info_t*)tp_info_ref_get(&in_ses->tp_info_ref);
                INFO_FLAGS(psi_info) = 0;
                psi_info->info_type = PSI_USAGE;
                psi_info->pmt_flag = 1;
                psi_info->pid = pid;
                psi_info->psi_usage_id = prog->pmt_usage_id;
                psi_info->prog_num = prog->prog_num;
            }

        } else if (sect->sect_hdr->table_id < PMT_TABLE_ID) {
            // Unexpected table in PMT pid
            VIDEO_DEBUG("Bad PSI table %d in PMT pid\n",
                        sect->sect_hdr->table_id);
            psi_section_free(&prog->snoop);

        } else if (snoop_private_section(prog)) {
            // Private tables in PMT PID
            que_put(NULL, &prog->priv_sect_list, &sect->link);

            // Remove current private carousel
            pool_free_list(&psi_tp_buf_pool, &prog->priv_crsl.tp_list);
            prog->priv_crsl.num_tp = 0;

            // Generate carousel for all private sections
            psi_section_t *s;
            FOR_ALL_ELEMENTS_IN_QUE(&prog->priv_sect_list, s) {
                prog->priv_crsl.num_tp +=
                        psi_section_packetize(s, pid, &prog->priv_crsl.tp_list);
            }
        }

        // Clean up snoop context
        prog->snoop.sect = NULL;            
    }
}

