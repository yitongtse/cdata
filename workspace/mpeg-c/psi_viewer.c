/*
 * Copyright (c) 2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * PSI Viewer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "param.h"
#include "video_hdr.h"


void dump_bytes (uint8 *buf, int len)
{
    int i;
    for (i=0; i<len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}


void dump_pat_header (pat_header_t* hdr)
{
    printf("Table ID          : %d\n", hdr->table_id);
    printf("Sector syntax     : %d\n", hdr->sect_syntax);
    printf("Private           : %d\n", hdr->priv);
    printf("Sector length     : %d\n", pat_get_section_size(hdr) - 3);
    printf("TSID              : %d\n", pat_get_tsid(hdr));
    printf("Version           : %d\n", hdr->ver);
    printf("Current/Next      : %d\n", hdr->cur_next);
    printf("Sector number     : %d\n", hdr->sect_num);
    printf("Last sector number: %d\n\n", hdr->last_sect_num);
}


void dump_pat_prog_info (pat_prog_info_t *info)
{
    printf("Program number: %d\n", pat_get_prog_num(info));
    printf("PMT PID       : %d\n\n", pat_get_pmt_pid(info));
}

void dump_pmt_header (pmt_header_t *hdr)
{
    printf("Table ID          : %d\n", hdr->table_id);
    printf("Sector syntax     : %d\n", hdr->sect_syntax);
    printf("Private           : %d\n", hdr->priv);
    printf("Sector length     : %d\n", pmt_get_section_size(hdr) - 3);
    printf("Program number    : %d\n", pmt_get_prog_num(hdr));
    printf("Version           : %d\n", hdr->ver);
    printf("Current/Next      : %d\n", hdr->cur_next);
    printf("Sector number     : %d\n", hdr->sect_num);
    printf("Last sector number: %d\n", hdr->last_sect_num);

    printf("PCR PID           : %d\n", pmt_get_pcr_pid(hdr));
    printf("Program info size : %d\n\n", pmt_get_prog_info_len(hdr));
}


void dump_pmt_es_info (pmt_es_info_t *info)
{
    printf("PID number  : %d\n", pmt_get_es_pid(info));
    printf("ES type     : %d\n", info->es_type);
    printf("ES info size: %d\n\n", pmt_get_es_info_len(info));
}


boolean parse_pat (uint8* tp_buf, int* pmt_pid)
{
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;
    if (!(tp_hdr->af_ctrl & 1)) {
        return FALSE;
    }

    // Skip adaptation field if present
    uint8* ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        ptr += (*ptr) + 1;
    }

    if (!tp_hdr->payload_unit_start)  return FALSE;

    // PUSI = 1: PTR field presents
    ptr += *ptr++;          // skip over first (partial) segment

    pat_header_t* pat_hdr = (pat_header_t*)ptr;
    dump_pat_header(pat_hdr);
    int left_bytes = pat_get_section_size(pat_hdr);
    ptr += sizeof(pat_header_t);

    while (left_bytes > 0) {
        pat_prog_info_t* pat_prog = (pat_prog_info_t*)ptr;
        dump_pat_prog_info(pat_prog);
        left_bytes -= sizeof(pat_prog);
        ptr += sizeof(pat_prog_info_t);

        if (pat_get_prog_num(pat_prog) != 0) {
            *pmt_pid = pat_get_pmt_pid(pat_prog);   // first PMT pid found!
            return TRUE;
        }

    }

    return left_bytes == 0;
}


boolean parse_pmt (uint8* tp_buf)
{
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;
    if (!(tp_hdr->af_ctrl & 1)) {
        return FALSE;
    }

    // Skip adaptation field if present
    uint8* ptr = (uint8*)(tp_hdr + 1);
    if ((tp_hdr->af_ctrl & 2)) {
        ptr += (*ptr) + 1;
    }

    if (!tp_hdr->payload_unit_start)  return FALSE;

    // PUSI = 1: PTR field presents
    ptr += *ptr++;              // skip over first (partial) segment

    pmt_header_t* pmt_hdr = (pmt_header_t*)ptr;

    printf("\n\nPMT:\n");
    dump_pmt_header(pmt_hdr);
    int left_bytes = pmt_get_section_size(pmt_hdr) - 4;

    int skip_sz = sizeof(pmt_header_t) + pmt_get_prog_info_len(pmt_hdr);
    ptr += skip_sz;
    left_bytes -= skip_sz;

    while (left_bytes > 0) {
        pmt_es_info_t* es_info = (pmt_es_info_t*)ptr;
        dump_pmt_es_info(es_info);

        skip_sz = sizeof(pmt_es_info_t) + pmt_get_es_info_len(es_info);
        ptr += skip_sz;
        left_bytes -= skip_sz;
    }

    return FALSE;
}



int main (int argc, char *argv[])
{
    param_t par;
    char file[128];

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "psi_viewer.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "PMT Viewer");
    param_get_string(&par, "Bitstream file", file, "test.ts");
    param_end(&par);
    printf("\n\n");

    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Error: Failed to open bitstream file %s\n", file);
        exit(1);
    }

    uint8 tp_buf[TP_SIZE];

    int pmt_pid = 0xFFFF;

    while (1) {
        int n = fread(tp_buf, TP_SIZE, 1, fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }

        uint16 pid = tp_get_pid((tp_header_t*)tp_buf);
        if (pid == PAT_PID) {
            boolean rc = parse_pat(tp_buf, &pmt_pid);
            printf("PAT parse result: %d\n", rc);
        }

        if (pid == pmt_pid) {
            boolean rc = parse_pmt(tp_buf);
            printf("PMT parse result: %d\n", rc);
            return 0;
        }
    }

    fclose(fp);
    return 0;
}
