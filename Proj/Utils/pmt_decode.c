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
#define PMT_SECT_HDR_SIZE       12
#define PMT_ES_HDR_SIZE         5
#define PMT_TABLE_ID            2


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


// PMT section header
//
typedef struct pmt_header_ {
    uint64 table_id : 8;
    uint64 sect_syntax : 1;
    uint64 priv : 1;
    uint64 resv1 : 2;
    uint64 sect_len : 12;
    uint64 prog_num : 16;
    uint64 resv2 : 2;
    uint64 ver : 5;
    uint64 cur_next : 1;
    uint64 sect_num : 8;
    uint64 last_sect_num : 8;

    uint16 resv3 : 3;
    uint16 pcr_pid : 13;
    uint16 resv4 : 4;
    uint16 prog_info_len : 12;
} pmt_header_t;


// PMT elementary stream info
//
typedef struct pmt_es_info_ {
    uint8 es_type;
    uint8 resv1 : 3;
    uint8 pid_hi : 5;
    uint8 pid_lo;
    uint8 resv2 : 4;
    uint8 es_info_len_hi : 4;
    uint8 es_info_len_lo;
} pmt_es_info_t;


// Generic descriptor header
//
typedef struct descriptor_header_ {
    uint8 tag;
    uint8 len;
} descriptor_header_t;



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


// Scan descriptors
void scan_descriptors (int info_len, descriptor_header_t *desc)
{
    while (info_len > 0) {
        printf("Desc: tag %d, len %d\n", desc->tag, desc->len);
        desc++;
        info_len -= desc->len + 2;
    }

    if (info_len != 0) {
        printf("Ill formatted descriptor loop\n");
    }
}


int main (int argc, char** argv)
{
    FILE *fp;
    tp_header_t *tp_hdr;
    pmt_header_t *pmt_hdr;
    pmt_es_info_t *pmt_es;
    uint8 *ptr;
    uint8 *esinfo_end_addr;
    int offset;
    int info_len;
    int pid;
    int i;

    if (argc != 3) {
        printf("Usage: %s <in_file> <offset>\n", argv[0]);
        printf("       offset is location in bytes in in_file where PMT TP starts\n");
        exit (-1);
    }

    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open input file %s\n", argv[1]);
        exit (-1);
    }

    offset = atoi(argv[2]);

    if (fread(buf, 1, offset, fp) != offset) {
        printf("Failed to skip to start of PMT TP\n");
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
    pmt_hdr = (pmt_header_t*)buf;
    ptr = buf;

    printf("Table id: %d\n", pmt_hdr->table_id);
    printf("Length: %d\n", pmt_hdr->sect_len);
    printf("Program no.: %04x\n", pmt_hdr->prog_num);
    printf("Version: %d\n", pmt_hdr->ver);
    printf("sect_num: %d\n", pmt_hdr->sect_num);
    printf("last_sect_num: %d\n", pmt_hdr->last_sect_num);

    if (pmt_hdr->table_id != PMT_TABLE_ID) {
        printf("This is not a PMT TP\n");
        exit (-1);
    }

    esinfo_end_addr = ptr + pmt_hdr->sect_len + 3 - 4;
    ptr += PMT_SECT_HDR_SIZE;

    // Scan prog_info for CA descriptor
    scan_descriptors(pmt_hdr->prog_info_len, (descriptor_header_t*)ptr);

    ptr += pmt_hdr->prog_info_len;

    // ES loop
    do {
        pmt_es_info_t* es_info = (pmt_es_info_t*)ptr;

        pid = (es_info->pid_hi << 8) | es_info->pid_lo;
        info_len = (es_info->es_info_len_hi << 8) | es_info->es_info_len_lo;
        ptr += PMT_ES_HDR_SIZE;
        printf("  ES pid %04x, type %d, info_len %d\n",
               pid, es_info->es_type, info_len);

        // Scan prog_info for CA descriptor
        scan_descriptors(info_len, (descriptor_header_t*)ptr);
        ptr += info_len;

    } while (ptr < esinfo_end_addr);
}

