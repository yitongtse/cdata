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


boolean parse_pat (uint8* tp_buf)
{
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;
    int skip_sz;

    if (!(tp_hdr->af_ctrl & 1)) {
        return FALSE;
    }

    // Skip adaptation field if present
    uint8* ptr = (uint8*)(tp_hdr + 1);
    int left_bytes = TP_SIZE - 4;

    if ((tp_hdr->af_ctrl & 2)) {
        skip_sz = (*ptr) + 1;
        ptr += skip_sz;
        left_bytes -= skip_sz;
    }

    if (!tp_hdr->payload_unit_start)  return FALSE;

    // PUSI = 1: PTR field presents
    skip_sz = *ptr++;

    ptr += skip_sz;          // skip over first (partial) segment
    left_bytes -= skip_sz;

    dump_bytes(ptr, left_bytes);

    pat_header_t* pat_hdr = (pat_header_t*)ptr;
    dump_pat_header(pat_hdr);
    left_bytes = pat_get_section_size(pat_hdr);
    ptr += sizeof(pat_header_t);

    while (left_bytes > 0) {
        pat_prog_info_t* pat_prog = (pat_prog_info_t*)ptr;
        dump_pat_prog_info(pat_prog);
        left_bytes -= sizeof(pat_prog);
        ptr += sizeof(pat_prog_info_t);
    }

    return left_bytes == 0;
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

    uint8 tp_buf[TP_SIZE];

    while (1) {
        int n = fread(tp_buf, TP_SIZE, 1, fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }

        if (tp_get_pid((tp_header_t*)tp_buf) == 0) {
            boolean rc = parse_pat(tp_buf);
            printf("Parse result: %d\n", rc);
            return 0;
        }

    }

    fclose(fp);
    return 0;
}
