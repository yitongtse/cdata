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



// Global variables
//
FILE *in_fp;
int32 skip_bytes;
int32 tp_size;

tp_info_t *tp_info_array;
pid_info_t pid_info_array[MAX_PID + 1];
prog_info_t prog_info_array[MAX_PROGS];

uint8 tp_buf[TP_SIZE];
pat_info_t pat_info;
pmt_info_t pmt_info[MAX_PROGS];
pes_info_t pes_info;

uint8 video_buf[TP_SIZE];


void index_tp_by_pid (int32 tp_cnt)
{
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

        fread(tp_buf, TP_SIZE, 1, in_fp);
        tp_hdr = (tp_header_t*)tp_buf;

        if (tp_hdr->sync != SYNC_BYTE) {
            printf("Lose sync at TP %d: sync %02x\n", idx, tp_hdr->sync);
            exit (-1);
        }

        tp_info->pid_next_idx = UNKNOWN_IDX;
        tp_info->pid = tp_hdr->pid;
        tp_info->cc = tp_hdr->cc;
        tp_info->pusi = tp_hdr->payload_unit_start;

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
        tp_info->disc_flag = af_flag && af_hdr->discontinuity;
        tp_info->ra_flag = af_flag && af_hdr->random_access;

        // Parse PCR
        if (tp_info->pcr_flag) {
            tp_info->pcr = parse_pcr(af_hdr);
        }

        if (extra_bytes) {
            fseek(in_fp, extra_bytes, 1);
        }
    }
}


void index_tp_by_prog (int32 tp_cnt)
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

        if (tp_info->pcr_flag && prog_info->first_pcr_idx == UNKNOWN_IDX) {
            prog_info->first_pcr_idx = idx;
        }
    }
}


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
    mpeg_time_t min_tp_itvl = 10000000;
    mpeg_time_t max_tp_itvl = 0;

    pcr1_idx = find_next_pcr_in_program(prog->first_idx);
    if (pcr1_idx == UNKNOWN_IDX) {
        printf("Program has no PCR\n");
        return;
    }
    pcr1 = tp_info_array[pcr1_idx].pcr;

    while (pcr1_idx != UNKNOWN_IDX) {
        idx = tp_info_array[pcr1_idx].prog_next_idx;
        pcr2_idx = find_next_pcr_in_program(idx);

        if (pcr2_idx != UNKNOWN_IDX) {
            pcr2 = tp_info_array[pcr2_idx].pcr;

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

    printf("  Bitrate %d - %d bps\n",
           tp_itvl_to_bitrate(max_tp_itvl), tp_itvl_to_bitrate(max_tp_itvl));
}


int snoop_vid_hdr (uint8 *ptr)
{
    int start_code;

    memcpy(video_buf, ptr, 64);

    start_code = *(uint32*)video_buf;
    if ((start_code >> 8) == START_CODE_PREFIX) {
        return (start_code & 0xFF);
    }
    return -1;
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
            printf("PES header:\n");
            print_hex(pes_info.pes_hdr, 64);
#endif

            if ((pes_info.pes_hdr->stream_id & 0xF0) == 0xE0) {
                // This is a mpeg-2 video PES
                uint8* pes_pyld = pes_info.buf + sizeof(pes_header_t) +
                                       pes_info.pes_hdr->pes_header_data_len;
                int sc = snoop_vid_hdr(pes_pyld);

                switch (sc) {

                case SEQ_START_CODE:
                    dump_seq_hdr((video_seq_header_t*)video_buf);
                    break;

                case GOP_START_CODE:
                    printf("GOP hdr\n");
                    break;

                case PIC_START_CODE:
                    dump_pic_hdr((video_pic_header_t*)video_buf);
                    break;

                default:
                }
            }
        }
        idx = tp_info_array[idx].pid_next_idx;
    } while (idx != UNKNOWN_IDX);

    printf("Total %d AUs\n", au_cnt);
}


int main(int argc, char** argv)
{
    char in_file[128];
    Param par;
    int32 in_size;
    int32 tp_cnt;
    pid_info_t *pid_info;
    prog_info_t *prog_info;
    int i, j;
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
        prog_info->tp_cnt = 0;
    }

    // Find the input file info
    fseek(in_fp, 0, 2);
    in_size = ftell(in_fp);
    tp_cnt = (in_size - skip_bytes) / tp_size;
    printf("Input file %s, %d bytes, %d TPs\n", in_file, in_size, tp_cnt);


    printf("Indexing TP by PID...\n");
    fseek(in_fp, skip_bytes, 0);
    index_tp_by_pid(tp_cnt);

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


    printf("Indexing TP by program...");
    fseek(in_fp, skip_bytes, 0);
    index_tp_by_prog(tp_cnt);
    printf("\n");
    dump_prog_info();


    // Time processing
    printf("\nTime processing...\n");
    for (i=0; i<pat_info.prog_cnt; i++) {
        prog_info = &prog_info_array[i];
        printf("\nProgram %d:\n", prog_info->prog_num);
        time_process(prog_info);
    }


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
}

