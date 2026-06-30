/*****************************************************************************
    File: mpeg2util.c
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "param.h"
#include "mpeg2.h"
#include "mpeg2test.h"


float frame_rate[] = {0., 23.976, 24., 25., 29.97, 30., 50., 59.94, 60.};

char pic_type[] = {"U", "I", "P", "B"};


void print_hex (void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        printf("%02x ", *temp++);
        if (++i==16) {
            i = 0;
            printf("\n");
        }
    }
    if (i) {
        printf("\n");
    }
}


void dump_pid_info (void)
{
    int i;
    pid_info_t *pid_info;

    printf("\nPid table:\n");
    for (i=0, pid_info=pid_info_array; i<=MAX_PID; i++, pid_info++) {
        if (pid_info->tp_cnt > 0) {
            printf("  PID %4d: prog %d, %d TPs\n",
                   i, prog_info_array[pid_info->prog_idx].prog_num,
                   pid_info->tp_cnt);
        }
    }
}


void dump_prog_info (void)
{
    int i;
    prog_info_t *prog_info;

    printf("\nPid table:\n");
    for (i=0, prog_info=prog_info_array; i<MAX_PROGS; i++, prog_info++) {
        if (prog_info->tp_cnt > 0) {
            printf("  Prog idx %d: prog num %d, %d TPs\n",
                   i, prog_info->prog_num, prog_info->tp_cnt);
        }
    }
}


// Dump TP header
//
void dump_tp_header (tp_header_t *tp_hdr)
{
    printf("SYN %02x, TEI %d, PUSI %d, PR %d, PID %04x, SC %d, AFC %d, CC %d\n",
           tp_hdr->sync, tp_hdr->transport_err, tp_hdr->payload_unit_start,
           tp_hdr->priority, tp_hdr->pid, tp_hdr->scrambling_ctrl,
           tp_hdr->af_ctrl, tp_hdr->cc);
}


// Trace all TP for a PID
//
void trace_pid (int16 pid)
{
    int cnt = 0;
    int idx = pid_info_array[pid].first_idx;

    printf("PID %d:\n", pid);

    while (idx != UNKNOWN_IDX) {
        printf("  TP %d\n", idx);
        cnt++;
        idx = tp_info_array[idx].pid_next_idx;
    }
    printf("Total %d TPs\n", cnt);
}


// Trace all TPs for a program
//
void trace_prog (int prog_idx)
{
    prog_info_t* prog_info = &prog_info_array[prog_idx];
    int idx = prog_info->first_idx;
    int cnt = 0;
    mpeg_time_t prev_pcr;

    printf("Prog idx %d: prog num %d\n", prog_idx, prog_info->prog_num);

    while (idx != UNKNOWN_IDX) {
        tp_info_t* tp_info = &tp_info_array[idx];
        printf("  TP %d: PID %d", idx, tp_info->pid);
        if (tp_info->pcr_flag) {
            printf(", PCR %lld", tp_info->pcr);
        } else {
            printf(", iPCR %lld", tp_info->pcr);
        }
        printf(", +%lld\n", tp_info->pcr - prev_pcr);

        cnt++;
        idx = tp_info->prog_next_idx;
        prev_pcr = tp_info->pcr;
    }
    printf("Total %d TPs\n", cnt);
}


// Parse PCR
//
mpeg_time_t parse_pcr (af_header_t *af_hdr)
{
    uint32 base = (af_hdr->pcr_base_1 << 16) | af_hdr->pcr_base_2;
    int16 ext = af_hdr->pcr_base_3 * 300 + af_hdr->pcr_ext;
    return ((int64)base) * SCALE_BASE_EXT + ext;
}


// Snoop for PAT
//
void snoop_pat (pat_info_t *pat, uint8 *tp_buf)
{
    tp_header_t *tp_hdr;
    tp_info_t *tp_info;
    uint8* tp_ptr;
    boolean pusi = FALSE;
    int skip_size;       
    int remain_size;
    pat_prog_info_t *prog_info;
    int i;

    tp_hdr = (tp_header_t*)tp_buf;

    // Check if PAT TP has payload
    if (!(tp_hdr->af_ctrl & 1)) {
        printf("PAT TP has no payload\n");
        exit (-1);
    }

    // Skip adaptation field if present
    tp_ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        tp_ptr += (*tp_ptr) + 1;
    }

    pusi = tp_hdr->payload_unit_start;
    if (!pusi) {
        printf("No PAT section starts in PAT TP\n");
        exit (-1);
    }

    skip_size = *tp_ptr++;
    tp_ptr += skip_size;

    // Allocate PAT buffer
    pat->buf = calloc(1, TP_SIZE);
    if (pat->buf == NULL) {
        printf("Failed to allocate PAT buffer\n");
        exit (-1);
    }

    // Copy PAT section
    remain_size = TP_SIZE - (tp_ptr - (uint8*)tp_hdr);
    memcpy(pat->buf, tp_ptr, remain_size);

    pat->pat_hdr = (pat_header_t*)pat->buf;
    prog_info = (pat_prog_info_t*)(pat->pat_hdr + 1);
    pat->prog_cnt = (pat->pat_hdr->sect_len - 9) / 4;

    for (i=0; i<pat->prog_cnt; i++) {
        int pmt_pid;
        int prog_num;
        int prog_idx;

        pat->prog_info[i] = (pat_prog_info_t*)prog_info++;

        // Set the program number for all PMT PID
        pmt_pid = pat->prog_info[i]->pmt_pid;
        prog_num = pat->prog_info[i]->prog_num;

        if (prog_num == 0) {
            // Ignore NIT
            continue;
        }

        prog_idx = pid_info_array[pmt_pid].prog_idx;
        if (prog_idx != UNKNOWN_IDX) {
            printf("PID %d already used for program %d\n",
                   pmt_pid, prog_info_array[prog_idx].prog_num);
            exit(-1);
        }

        pid_info_array[pmt_pid].prog_idx = i;
        prog_info_array[i].prog_num = prog_num;
    }
}


// Dump PAT table
//
void dump_pat (pat_info_t *pat)
{
    int i;
    pat_header_t *pat_hdr = pat->pat_hdr;

    printf("PAT: tsid %d, ver %d, %d sects, %d progs\n",
           pat_hdr->tsid, pat_hdr->ver, pat_hdr->last_sect_num + 1,
           pat->prog_cnt);
    for (i=0; i<pat->prog_cnt; i++) {
        printf("  Prog %d: PMT %d\n",
               pat->prog_info[i]->prog_num, pat->prog_info[i]->pmt_pid);
    }
}


// Snoop for PMT
//
void snoop_pmt (pmt_info_t *pmt, uint8 *tp_buf, int32 prog_idx)
{
    tp_header_t *tp_hdr;
    tp_info_t *tp_info;
    int32 idx;
    uint8* tp_ptr;
    boolean pusi = FALSE;
    int skip_size;
    int remain_size;
    int i;
    pat_prog_info_t *prog_info;
    uint8* ptr;
    uint8* esinfo_end_addr;
    int info_len;

    tp_hdr = (tp_header_t*)tp_buf;

    // Check if PMT TP has payload
    if (!(tp_hdr->af_ctrl & 1)) {
        printf("PMT TP has no payload\n");
        exit (-1);
    }

    // Skip adaptation field if present
    tp_ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        tp_ptr += (*tp_ptr) + 1;
    }

    pusi = tp_hdr->payload_unit_start;
    if (!pusi) {
        printf("No PMT section starts in PMT TP\n");
        exit (-1);
    }

    skip_size = *tp_ptr++;
    tp_ptr += skip_size;

    // Allocate PMT buffer
    pmt->buf = calloc(1, TP_SIZE);
    if (pmt->buf == NULL) {
        printf("Failed to allocate PMT buffer\n");
        exit (-1);
    }

    // Copy PMT section
    remain_size = TP_SIZE - (tp_ptr - (uint8*)tp_hdr);
    memcpy(pmt->buf, tp_ptr, remain_size);

    pmt->pmt_hdr = (pmt_header_t*)pmt->buf;
    pmt->es_cnt = 0;
    ptr = pmt->buf + PMT_SECT_HDR_SIZE + pmt->pmt_hdr->prog_info_len;
    esinfo_end_addr = pmt->buf + pmt->pmt_hdr->sect_len
                          + SECT_LEN_ADJ - CRC_SIZE;

    // ES loop
    while (ptr < esinfo_end_addr) {
        pmt_es_info_t* es_info = (pmt_es_info_t*)ptr;
        int pid = (es_info->pid_hi << 8) | es_info->pid_lo;

        if (pmt->es_cnt >= MAX_ES_PER_PROG) {
            printf("Too many ES in program");
            exit (-1);
        }
        pmt->es_info[pmt->es_cnt++] = es_info;

        // Set program index for the PID
        if (pid_info_array[pid].prog_idx != UNKNOWN_IDX) {
            printf("PID %d already used in program %d\n",
                   pid, prog_info_array[pid_info_array[pid].prog_idx].prog_num);
            exit(-1);
        }
        pid_info_array[pid].prog_idx = prog_idx;

        info_len = (es_info->es_info_len_hi << 8) | es_info->es_info_len_lo;
        ptr += PMT_ES_HDR_SIZE + info_len;
    }
}


// Dump PMT table
//
void dump_pmt (pmt_info_t *pmt)
{
    int i;
    pmt_header_t* pmt_hdr = (pmt_header_t*)pmt->pmt_hdr;
    printf("PMT: prog %d, ver %d, PCR %d",
           (int)pmt_hdr->prog_num, pmt_hdr->ver, pmt_hdr->pcr_pid);

    for (i=0; i<pmt->es_cnt; i++) {
        pmt_es_info_t* es_info = pmt->es_info[i];
        int pid = (es_info->pid_hi << 8) | es_info->pid_lo;
        int es_info_len = (es_info->es_info_len_hi << 8)
                              | es_info->es_info_len_lo;
        printf("\n  PID %d: type %d, len %d",
               pid, es_info->es_type, es_info_len);

        // Scan descriptors
        while (es_info_len > 0) {
            uint8* ptr = (uint8*)(es_info + 1);
            descriptor_header_t* desc = (descriptor_header_t*)ptr;
            printf(", (%d, %d)", desc->tag, desc->len);
            ptr += desc->len + 2;
            es_info_len -= desc->len + 2;
        }
    }
    printf("\n");
}


mpeg_time_t get_time_stamp (time_stamp_t *ts)
{
    mpeg_time_t time = ts->ts_1;
    time <<= 30;
    time |= (ts->ts_2 << 22) | (ts->ts_3 << 15) | (ts->ts_4 << 7) | ts->ts_5;
    return time;
}


void snoop_pes (pes_info_t *pes_info, uint8 *tp_buf)
{
    uint8* tp_ptr;
    int remain_size;
    time_stamp_t *ts;
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;

    // Check if TP has payload
    if (!(tp_hdr->af_ctrl & 1)) {
        printf("TP has no payload\n");
        exit (-1);
    }

    // Skip adaptation field if present
    tp_ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        tp_ptr += (*tp_ptr) + 1;
    }

    pes_info->buf = calloc(1, TP_SIZE);

    remain_size = TP_SIZE - (tp_ptr - (uint8*)tp_hdr);
    memcpy(pes_info->buf, tp_ptr, remain_size);

    pes_info->pes_hdr = (pes_header_t*)pes_info->buf;

    ts = (time_stamp_t*)(pes_info->pes_hdr + 1);

    if ((pes_info->pes_hdr->pts_dts_flag & 0x2)) {
        // Get PTS
        pes_info->pts = get_time_stamp(ts);
        ts++;
    } else {
        pes_info->pts = -1;
    }

    if ((pes_info->pes_hdr->pts_dts_flag & 0x1)) {
        // Get DTS
        pes_info->dts = get_time_stamp(ts);
    } else {
        pes_info->dts = -1;
    }

}


// Dump PES header
//
void dump_pes (pes_info_t *pes_info)
{
    pes_header_t* pes_hdr = pes_info->pes_hdr;

    printf("PES header: id %08x, len %d, align %x, DTS_PTS %x, hdr_len %d\n",
           pes_hdr->stream_id, ((pes_hdr->pes_len1 << 8) | pes_hdr->pes_len2),
           pes_hdr->data_align, pes_hdr->pts_dts_flag,
           pes_hdr->pes_header_data_len);
    if (pes_info->dts != -1) {
        printf("  DTS %09llx\n", pes_info->dts);
    }
    if (pes_info->pts != -1) {
        printf("  PTS %09llx\n", pes_info->pts);
    }
}


uint32 tp_itvl_to_bitrate (mpeg_time_t tp_itvl)
{
    return (uint32)((((int64)27e6) * 1504 << 16) / tp_itvl);
}


// Dump video sequence header
//
void dump_seq_hdr (video_seq_header_t *seq_hdr)
{
    printf("Seq: %d x %d, %f fps, BR %d bps, VBV size %d\n",
           seq_hdr->hori_size_value, seq_hdr->vert_size_value,
           frame_rate[seq_hdr->frame_rate_code], seq_hdr->bit_rate_value * 400,
           seq_hdr->vbv_size_value * 16 * 1024);
}


// Dump video picture header
//
void dump_pic_hdr (video_pic_header_t *pic_hdr)
{
    printf("Pic: TR %d, type %s, VBV delay %d\n",
           pic_hdr->tr, pic_type[pic_hdr->pic_type], pic_hdr->vbv_delay);
}

