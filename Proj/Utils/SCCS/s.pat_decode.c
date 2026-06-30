h36084
s 00171/00000/00000
d D 1.1 07/08/15 16:16:07 ytse 1 0
c date and time created 07/08/15 16:16:07 by ytse
e
u
U
f e 0
t
T
I 1
/*
   Copyright (c) 2007 by Cisco Systems, Inc.
   All rights reserved.

   File: pat_decode.c
   Decode a PAT, assuming it fits within one TP

   Usage: pat_decode <in_file>
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;


#define TP_SIZE                 188
#define SYNC_BYTE               0x47
#define PAT_PID                 0


// TP header
//
typedef struct tp_header_ {
    uint32 sync : 8;
    uint32 transport_err : 1;
    uint32 payload_unit_start : 1;
    uint32 priority : 1;
    uint32 pid : 13;
    uint32 scrambling_ctrl : 2;
    uint32 af_ctrl : 2;
    uint32 cc : 4;
} tp_header_t;


// PAT section header
//
typedef struct pat_header_ {
    uint64 table_id : 8;
    uint64 sect_syntax : 1;
    uint64 priv : 1;
    uint64 resv1 : 2;
    uint64 sect_len : 12;
    uint64 tsid : 16;
    uint64 resv2 : 2;
    uint64 ver : 5;
    uint64 cur_next : 1;
    uint64 sect_num : 8;
    uint64 last_sect_num : 8;
} pat_header_t;


// PAT program info
//
typedef struct pat_prog_info_ {
    uint16 prog_num;
    uint16 resv : 3;
    uint16 pmt_pid : 13;
} pat_prog_info_t;


// Global variables
//
uint8 buf[TP_SIZE];


// Dump TP header
//
void dump_tp_header (tp_header_t *tp_hdr)
{
    printf("SYN %02x, TEI %d, PUSI %d, PR %d, PID %04x, SC %d, AFC %d, CC %d\n",
           tp_hdr->sync, tp_hdr->transport_err, tp_hdr->payload_unit_start,
           tp_hdr->priority, tp_hdr->pid, tp_hdr->scrambling_ctrl,
           tp_hdr->af_ctrl, tp_hdr->cc);
}



int main (int argc, char** argv)
{
    FILE *fp;
    tp_header_t *tp_hdr;
    pat_header_t *pat_hdr;
    pat_prog_info_t *pat_prog;
    int offset;
    int prog_cnt;
    int i;

    if (argc != 3) {
        printf("Usage: %s <in_file> <offset>\n", argv[0]);
        printf("       offset is location in bytes in in_file where PAT TP starts\n");
        exit (-1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open input file %s\n", argv[1]);
        exit (-1);
    }

    offset = atoi(argv[2]);

    if (fread(buf, 1, offset, fp) != offset) {
        printf("Failed to skip to start of PAT TP\n");
        exit (-1);
    }

    if (fread(buf, sizeof(tp_header_t) + 1, 1, fp) != 1) {
        printf("Failed to read TP from file\n");
        exit (-1);
    }

    tp_hdr = (tp_header_t*)buf;
    dump_tp_header(tp_hdr);

    if (tp_hdr->sync != SYNC_BYTE) {
        printf("Error: Sync byte is wrong %02x\n", tp_hdr->sync);
        exit (-1);
    }

    if (tp_hdr->pid != PAT_PID) {
        printf("Error: This is not a PAT TP\n");
        exit (-1);
    }

    if (!tp_hdr->payload_unit_start) {
        printf("Error: This program expects PUSI to be set\n");
        exit (-1);
    }

    if (tp_hdr->af_ctrl != 1) {
        printf("Error: This program expects af = '01'\n");
        exit (-1);
    }

    // Skip to the first segment
    offset = *(uint8*)(tp_hdr + 1);
    printf("Pointer offset: %d\n", offset);
    fseek(fp, offset, SEEK_CUR);

    fread(buf, 1, TP_SIZE - 5 - offset, fp);
    pat_hdr = (pat_header_t*)buf;

    printf("Table id: %d\n", pat_hdr->table_id);
    printf("Length: %d\n", pat_hdr->sect_len);
    printf("TSID: %04x\n", pat_hdr->tsid);
    printf("Version: %d\n", pat_hdr->ver);
    printf("sect_num: %d\n", pat_hdr->sect_num);
    printf("last_sect_num: %d\n", pat_hdr->last_sect_num);

    prog_cnt = (pat_hdr->sect_len - 9) / 4;

    pat_prog = (pat_prog_info_t*)(pat_hdr + 1);

    for (i=0; i<prog_cnt; i++, pat_prog++) {
        printf("  Program %d: PMT PID %04x\n",
               pat_prog->prog_num, pat_prog->pmt_pid);
    }
}

E 1
