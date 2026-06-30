h22120
s 00073/00349/00290
d D 1.5 08/09/18 14:07:09 ytse 5 4
c Update
e
s 00028/00012/00611
d D 1.4 08/09/08 23:20:24 ytse 4 3
c update
e
s 00073/00039/00550
d D 1.3 08/09/08 22:49:08 ytse 3 2
c update
e
s 00043/00001/00546
d D 1.2 08/09/06 00:57:01 ytse 2 1
c 
e
s 00547/00000/00000
d D 1.1 08/09/06 00:25:44 ytse 1 0
c date and time created 08/09/06 00:25:44 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: mpeg2test.c
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "param.h"
#include "mpeg2.h"
#include "mpeg2test.h"


D 5
#define MAX_PID                 0x1FFF
#define UNKNOWN_IDX             -1
E 5

D 5

E 5
// Global variables
//
I 5
FILE *in_fp;
int32 skip_bytes;
int32 tp_size;

E 5
tp_info_t *tp_info_array;
pid_info_t pid_info_array[MAX_PID + 1];
prog_info_t prog_info_array[MAX_PROGS];

uint8 tp_buf[TP_SIZE];
pat_info_t pat_info;
pmt_info_t pmt_info[MAX_PROGS];
I 5
pes_info_t pes_info;
E 5

I 5
uint8 video_buf[TP_SIZE];
E 5

D 5
void print_hex (void* ptr, int nbytes)
{
    int i;
    uint8 *temp = ptr;
E 5

D 5
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
E 5
I 5
void index_tp_by_pid (int32 tp_cnt)
E 5
{
D 5
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
I 2
    mpeg_time_t prev_pcr;
E 2

    printf("Prog idx %d: prog num %d\n", prog_idx, prog_info->prog_num);

    while (idx != UNKNOWN_IDX) {
        tp_info_t* tp_info = &tp_info_array[idx];
        printf("  TP %d: PID %d", idx, tp_info->pid);
        if (tp_info->pcr_flag) {
            printf(", PCR %lld", tp_info->pcr);
I 2
        } else {
            printf(", iPCR %lld", tp_info->pcr);
E 2
        }
D 2
        printf("\n");
E 2
I 2
        printf(", +%lld\n", tp_info->pcr - prev_pcr);

E 2
        cnt++;
        idx = tp_info->prog_next_idx;
I 2
        prev_pcr = tp_info->pcr;
E 2
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


void index_tp_by_pid (FILE *fp, int32 tp_cnt, int32 tp_size)
{
E 5
    int idx;
    tp_header_t *tp_hdr;
    tp_info_t *tp_info;
    pid_info_t *pid_info;
    af_header_t* af_hdr = NULL;
    int af_flag = 0;
    int extra_bytes = tp_size - TP_SIZE;

    tp_info_array = calloc(tp_cnt, sizeof(tp_info_t));
    if (tp_info_array == NULL) {
        printf("Failed to allocate TP info array\n");
        exit (-1);
    }

    for (idx=0, tp_info=tp_info_array; idx<tp_cnt; idx++, tp_info++) {

D 5
        fread(tp_buf, TP_SIZE, 1, fp);
E 5
I 5
        fread(tp_buf, TP_SIZE, 1, in_fp);
E 5
        tp_hdr = (tp_header_t*)tp_buf;

        if (tp_hdr->sync != SYNC_BYTE) {
            printf("Lose sync at TP %d: sync %02x\n", idx, tp_hdr->sync);
            exit (-1);
        }

        tp_info->pid_next_idx = UNKNOWN_IDX;
        tp_info->pid = tp_hdr->pid;
        tp_info->cc = tp_hdr->cc;
I 5
        tp_info->pusi = tp_hdr->payload_unit_start;
E 5

        pid_info = &pid_info_array[tp_info->pid];
        pid_info->tp_cnt++;

        if (pid_info->first_idx == UNKNOWN_IDX) {
            pid_info->first_idx = idx;
        } else {
            tp_info_array[pid_info->prev_idx].pid_next_idx = idx;
        }
        pid_info->prev_idx = idx;

        // Check whether TP carries PCR
        af_flag = 0;
        if ((tp_hdr->af_ctrl & 2)) {
            af_hdr = (af_header_t*)(tp_hdr + 1);
            af_flag = (af_hdr->len > 0);
        }
        tp_info->pcr_flag = af_flag && af_hdr->pcr_flag;
I 5
        tp_info->disc_flag = af_flag && af_hdr->discontinuity;
        tp_info->ra_flag = af_flag && af_hdr->random_access;
E 5

D 5
        // Check for discontinuity point
        if (af_flag && af_hdr->discontinuity) {
            tp_info->disc_flag = 1;
        }

E 5
        // Parse PCR
        if (tp_info->pcr_flag) {
            tp_info->pcr = parse_pcr(af_hdr);
        }

        if (extra_bytes) {
D 5
            fseek(fp, extra_bytes, 1);
E 5
I 5
            fseek(in_fp, extra_bytes, 1);
E 5
        }
    }
}


D 5
void index_tp_by_prog (FILE *fp, int32 tp_cnt, int32 tp_size)
E 5
I 5
void index_tp_by_prog (int32 tp_cnt)
E 5
{
    int idx;
    tp_info_t *tp_info;
    prog_info_t *prog_info;
    int prog_idx;

    for (idx=0, tp_info=tp_info_array; idx<tp_cnt; idx++, tp_info++) {

        // Find which program this TP belongs to
        prog_idx = pid_info_array[tp_info->pid].prog_idx;

        if (prog_idx == UNKNOWN_IDX) {
            tp_info->prog_next_idx = UNKNOWN_IDX;
            continue;
        }

        tp_info->prog_next_idx = UNKNOWN_IDX;

        prog_info = &prog_info_array[prog_idx];
        prog_info->tp_cnt++;

        if (prog_info->first_idx == UNKNOWN_IDX) {
            prog_info->first_idx = idx;
        } else {
            tp_info_array[prog_info->prev_idx].prog_next_idx = idx;
        }
        prog_info->prev_idx = idx;
I 2

D 3
        // Time processing
        if (tp_info->pcr_flag) {
            mpeg_time_t pcr_diff;
            mpeg_time_t tp_itvl;
            int32 tp_idx_diff;
            mpeg_time_t cur_pcr;
            int32 idx1, idx2;

            if (prog_info->prev_pcr_idx == UNKNOWN_IDX) {
                // 1st PCR

            } else {
                // Compute TP interval
                pcr_diff = tp_info->pcr -
                               tp_info_array[prog_info->prev_pcr_idx].pcr;
                tp_idx_diff = idx - prog_info->prev_pcr_idx;
                tp_itvl = (pcr_diff + (tp_idx_diff >> 1)) / tp_idx_diff;

                // Scan through all TP belonging to this program
                idx1 = prog_info->prev_pcr_idx;
                cur_pcr = tp_info_array[idx1].pcr;

                while (1) {
                    idx2 = tp_info_array[idx1].prog_next_idx;
                    if (idx2 == idx) {
                        break;
                    }
                    cur_pcr += tp_itvl * (idx2 - idx1);
                    tp_info_array[idx2].pcr = cur_pcr;
                    idx1 = idx2;
                }
            }

            prog_info->prev_pcr_idx = idx;
E 3
I 3
        if (tp_info->pcr_flag && prog_info->first_pcr_idx == UNKNOWN_IDX) {
            prog_info->first_pcr_idx = idx;
E 3
        }
E 2
    }
}


D 5
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


I 4
uint32 tp_itvl_to_bitrate (mpeg_time_t tp_itvl)
{
    return (uint32)((((int64)27e6) * 1504 << 16) / tp_itvl);
}


E 5
E 4
I 3
int32 find_next_pcr_in_program (int32 idx)
{
    while (idx != UNKNOWN_IDX) {
        if (tp_info_array[idx].pcr_flag) {
            return idx;
        }
        idx = tp_info_array[idx].prog_next_idx;
    }
    return UNKNOWN_IDX;
}


void time_process (prog_info_t *prog)
{
    int32 pcr1_idx = UNKNOWN_IDX;
    int32 pcr2_idx = UNKNOWN_IDX;
    int32 idx, idx2, tp_idx_diff;
    mpeg_time_t pcr1, pcr2, cur_pcr, pcr_diff;
    mpeg_time_t tp_itvl;
I 4
    mpeg_time_t min_tp_itvl = 10000000;
    mpeg_time_t max_tp_itvl = 0;
E 4

    pcr1_idx = find_next_pcr_in_program(prog->first_idx);
    if (pcr1_idx == UNKNOWN_IDX) {
        printf("Program has no PCR\n");
        return;
    }
    pcr1 = tp_info_array[pcr1_idx].pcr;

D 4
    while (1) {
E 4
I 4
    while (pcr1_idx != UNKNOWN_IDX) {
E 4
        idx = tp_info_array[pcr1_idx].prog_next_idx;
        pcr2_idx = find_next_pcr_in_program(idx);
D 4
        if (pcr2_idx == UNKNOWN_IDX) {
            return;
        }
        pcr2 = tp_info_array[pcr2_idx].pcr;
E 4

D 4
        // Compute TP interval
        pcr_diff = pcr2 - pcr1;
        tp_idx_diff = pcr2_idx - pcr1_idx;
        tp_itvl = ((pcr_diff << 16) + (tp_idx_diff >> 1)) / tp_idx_diff;
E 4
I 4
        if (pcr2_idx != UNKNOWN_IDX) {
            pcr2 = tp_info_array[pcr2_idx].pcr;
E 4

I 4
            // Compute TP interval
            pcr_diff = pcr2 - pcr1;
            tp_idx_diff = pcr2_idx - pcr1_idx;
            tp_itvl = ((pcr_diff << 16) + (tp_idx_diff >> 1)) / tp_idx_diff;

            if (tp_itvl > max_tp_itvl) {
                max_tp_itvl = tp_itvl;
            }
            if (tp_itvl < min_tp_itvl) {
                min_tp_itvl = tp_itvl;
            }
        }

E 4
        // Scan through all TP belonging to this program
        idx = pcr1_idx;
        cur_pcr = pcr1;

        while (1) {
            idx2 = tp_info_array[idx].prog_next_idx;
            if (idx2 == pcr2_idx) {
                break;
            }
            cur_pcr += (tp_itvl * (idx2 - idx) + 32768) >> 16;
            tp_info_array[idx2].pcr = cur_pcr;
            idx = idx2;
        }

        // Update
        pcr1_idx = pcr2_idx;
        pcr1 = pcr2;
    }
I 4

    printf("  Bitrate %d - %d bps\n",
           tp_itvl_to_bitrate(max_tp_itvl), tp_itvl_to_bitrate(max_tp_itvl));
E 4
}


I 5
void snoop_vid_hdr (uint8 *ptr)
{
    memcpy(video_buf, ptr, 64);

    vid_hdr = (video_seq_header_t*)video_buf;
    if (vid_hdr->start_code]
}


void conformance_test (pmt_es_info_t *es_info)
{
    int32 au_cnt = 0;        // access unit count
    uint16 pid = (es_info->pid_hi << 8) | es_info->pid_lo;
    int32 idx = pid_info_array[pid].first_idx;

    printf("Pid %d: type %d\n", pid, es_info->es_type);

    do {
        if (tp_info_array[idx].pusi) {
            au_cnt++;
            fseek(in_fp, skip_bytes + idx * tp_size, 0);
            fread(tp_buf, TP_SIZE, 1, in_fp);
            snoop_pes(&pes_info, tp_buf);

#if 0
            printf("\nRA: %d\n", tp_info_array[idx].ra_flag);
            printf("PES header:\n");
            print_hex(pes_info.pes_hdr, 64);
#endif

            if ((pes_info.pes_hdr->stream_id & 0xF0) == 0xE0) {
                // This is a mpeg-2 video PES
                snoop_vid_hdr(pes_info.buf + sizeof(pes_header_t) +
                              pes_info.pes_hdr->pes_header_data_len);
            }
        }
        idx = tp_info_array[idx].pid_next_idx;
    } while (idx != UNKNOWN_IDX);

    printf("Total %d AUs\n", au_cnt);
}


E 5
E 3
int main(int argc, char** argv)
{
    char in_file[128];
    Param par;
D 5
    FILE *in_fp;
E 5
    int32 in_size;
D 5
    int32 skip_bytes;
    int32 tp_size;
E 5
    int32 tp_cnt;
    pid_info_t *pid_info;
    prog_info_t *prog_info;
D 5
    int i;
E 5
I 5
    int i, j;
E 5
    int idx;
    int pid;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2test.par");
    getStringParam(&par, "Input filename", in_file, "test.ts");
    getIntParam(&par, "No. of bytes to skip in the beginning", &skip_bytes, 0);
    getIntParam(&par, "Transport packet size", &tp_size, 188);
    endParam(&par);

    if ((in_fp = fopen(in_file, "r")) == NULL) {
        printf("Failed to open input file %s\n", in_file);
        exit (-1);
    }

    // Set up PID info array
    for (i=0, pid_info=pid_info_array; i<=MAX_PID; i++, pid_info++) {
        pid_info->first_idx = UNKNOWN_IDX;
        pid_info->prev_idx = UNKNOWN_IDX;
        pid_info->prog_idx = UNKNOWN_IDX;
        pid_info->tp_cnt = 0;
    }

    // Set up prog info array
    for (i=0, prog_info=prog_info_array; i<=MAX_PROGS; i++, prog_info++) {
        prog_info->prog_num = -1;
        prog_info->first_idx = UNKNOWN_IDX;
        prog_info->prev_idx = UNKNOWN_IDX;
I 2
D 4
        prog_info->prev_pcr_idx = UNKNOWN_IDX;
E 4
E 2
        prog_info->tp_cnt = 0;
    }

    // Find the input file info
    fseek(in_fp, 0, 2);
    in_size = ftell(in_fp);
    tp_cnt = (in_size - skip_bytes) / tp_size;
    printf("Input file %s, %d bytes, %d TPs\n", in_file, in_size, tp_cnt);


D 3
    // First pass
    //
E 3
    printf("Indexing TP by PID...\n");
    fseek(in_fp, skip_bytes, 0);
D 5
    index_tp_by_pid(in_fp, tp_cnt, tp_size);
E 5
I 5
    index_tp_by_pid(tp_cnt);
E 5

    // Snoop PAT
    printf("\nSnooping PAT...\n");
    idx = pid_info_array[PAT_PID].first_idx;
    if (idx == UNKNOWN_IDX) {
        printf("PAT not found\n");
        exit (-1);
    }
    fseek(in_fp, skip_bytes + idx * tp_size, 0);
    fread(tp_buf, TP_SIZE, 1, in_fp);
    snoop_pat(&pat_info, tp_buf);
    dump_pat(&pat_info);

    // Snoop PMT
    printf("\nSnooping PMT...\n");
    for (i=0; i<pat_info.prog_cnt; i++) {

        // TBD: need special handling for NIT

        pid = pat_info.prog_info[i]->pmt_pid;
        idx = pid_info_array[pid].first_idx;
        if (idx == UNKNOWN_IDX) {
            printf("PMT not found\n");
            exit (-1);
        }
        fseek(in_fp, skip_bytes + idx * tp_size, 0);
        fread(tp_buf, TP_SIZE, 1, in_fp);
        snoop_pmt(&pmt_info[i], tp_buf, i);
        dump_pmt(&pmt_info[i]);
        printf("\n");
    }

    dump_pid_info();


D 3
    // Second pass
    //
E 3
    printf("Indexing TP by program...");
    fseek(in_fp, skip_bytes, 0);
D 5
    index_tp_by_prog(in_fp, tp_cnt, tp_size);
E 5
I 5
    index_tp_by_prog(tp_cnt);
E 5
    printf("\n");
    dump_prog_info();

D 3
    trace_prog(1);
E 3
I 3

    // Time processing
    printf("\nTime processing...\n");
    for (i=0; i<pat_info.prog_cnt; i++) {
        prog_info = &prog_info_array[i];
D 4
        printf("Program %d\n", prog_info->prog_num);
E 4
I 4
        printf("\nProgram %d:\n", prog_info->prog_num);
E 4
        time_process(prog_info);
    }

I 5

    // ES conformance test
    printf("\nConformance test...\n");
    for (i=0; i<pat_info.prog_cnt; i++) {
        prog_info = &prog_info_array[i];
        printf("\nProgram %d\n", prog_info->prog_num);

        for (j=0; j<pmt_info[i].es_cnt; j++) {
            // For now, only check video ES!
            if (pmt_info[i].es_info[j]->es_type == 2) {
                conformance_test(pmt_info[i].es_info[j]);
            }
        }
    }
E 5
D 4
//    trace_prog(1);
E 4
E 3
}

E 1
