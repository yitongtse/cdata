/*
 * Copyright (c) 2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * PAT Viewer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include "common.h"
#include "param.h"
#include "video_hdr.h"


uint32 psi_buf[1024/4];


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
    printf("Sector length     : %d\n", hdr->sect_len);
    printf("TSID              : %d\n", (hdr->tsid_1 << 8) | hdr->tsid_2);
    printf("Version           : %d\n", hdr->ver);
    printf("Current/Next      : %d\n", hdr->cur_next);
    printf("Sector number     : %d\n", hdr->sect_num);
    printf("Last sector number: %d\n\n", hdr->last_sect_num);
}


void dump_pat_prog_info (pat_prog_info_t *info)
{
    printf("Program number: %d\n", info->prog_num);
    printf("PMT PID       : %d\n\n", info->pmt_pid);
}


boolean parse_pat(uint8* buf, tp_header_t *tp_hdr)
{
    int skip_sz;
    uint32 hdr[2];

    if (!(tp_hdr->af_ctrl & 1)) {
        return FALSE;
    }

    // Skip adaptation field if present
    uint8* tp_ptr = (uint8*)(buf + 4);
    int left_bytes = TP_SIZE - 4;

    if ((tp_hdr->af_ctrl & 2)) {
        skip_sz = (*tp_ptr) + 1;
        tp_ptr += skip_sz;
        left_bytes -= skip_sz;
    }

    if (!tp_hdr->payload_unit_start)  return FALSE;

    // PUSI = 1: PTR field presents
    skip_sz = *tp_ptr++;

    tp_ptr += skip_sz;          // skip over first (partial) segment
    left_bytes -= skip_sz;

    memcpy(psi_buf, tp_ptr, left_bytes);
    dump_bytes(psi_buf, left_bytes);

    hdr[0] = htonl(psi_buf[0]);
    hdr[1] = htonl(psi_buf[1]);
    pat_header_t* pat_hdr = (pat_header_t*)hdr;
    dump_pat_header(pat_hdr);
    left_bytes = pat_hdr->sect_len + 3;
    int psi_buf_offset = 2;

    while (left_bytes > 0) {
        hdr[0] = htonl(psi_buf[psi_buf_offset++]);
        pat_prog_info_t* pat_prog = (pat_prog_info_t*)hdr;
        dump_pat_prog_info(pat_prog);
        left_bytes -= sizeof(pat_prog);
    }

    return TRUE;
}


int main (int argc, char *argv[])
{
    param_t par;
    char file[128];

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "pat_viewer.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "PAT Viewer");
    param_get_string(&par, "Bitstream file", file, "test.ts");
    param_end(&par);
    printf("\n\n");

    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Error: Failed to open bitstream file %s\n", file);
        exit(1);
    }

    uint32 tp_buf[TP_SIZE/4];
    uint32 hdr[3];
    tp_header_t* tp_hdr = (tp_header_t*)&hdr[0];
    af_header_t* af = (af_header_t*)&hdr[1];

    while (1) {
        int n = fread(tp_buf, TP_SIZE, 1, fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }

        // Little endian specific operation
        hdr[0] = htonl(tp_buf[0]);
        if (tp_hdr->af_ctrl & 2) {
            hdr[1] = htonl(tp_buf[1]);
            hdr[2] = htonl(tp_buf[2]);
        }

        if (tp_hdr->pid == 0) {
            boolean rc = parse_pat((uint8*)tp_buf, tp_hdr);
            printf("Parse result: %d\n", rc);
            return 0;
        }

    }

    fclose(fp);
    return 0;
}
