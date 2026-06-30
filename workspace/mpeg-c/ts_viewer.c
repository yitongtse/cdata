/*
 * Copyright (c) 2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * MPEG-2 Transport Stream Viewer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include "common.h"
#include "util.h"
#include "param.h"
#include "video_hdr.h"


static inline
boolean tp_has_pcr (tp_header_t *tp_hdr, af_header_t *af_hdr)
{
    return (tp_hdr->af_ctrl & 2) && (af_hdr->len > 0) && af_hdr->pcr_flag;
}


int find_sync_byte (FILE *fp, int tp_size, int sync_cnt)
{
    while (1) {
        while (fgetc(fp) != SYNC_BYTE) {
            if (feof(fp))  return EOF;
        }
        int i;
        for (i=1; i<sync_cnt; i++) {
            fseek(fp, tp_size - 1, SEEK_CUR);
            if (fgetc(fp) != SYNC_BYTE)  break;
        }
        fseek(fp, -1, SEEK_CUR);
        return EOK;
    }
}


int main (int argc, char *argv[])
{
    param_t par;
    char file[128];
    uint32 num_tp = -1;
    uint32 tp_size;
    uint32 tp_cnt = 0;
    int sync_search;
    int skip_non_pcr;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "ts_viewer.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "Transport Stream Viewer");
    param_get_string(&par, "Bitstream file", file, "test.ts");
    param_get_int(&par, "Bytes per TP", &tp_size, TP_SIZE);
    param_get_int(&par, "Number of TPs to process (-1 for all)", &num_tp, -1);
    param_get_int(&par, "Search for SYNC byte?", &sync_search, 0);
    param_get_int(&par, "Skip non-pcr TP?", &skip_non_pcr, 1);
    param_end(&par);
    printf("\n\n");

    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        printf("Error: Failed to open bitstream file %s\n", file);
        exit(1);
    }

    if (sync_search) {
        int rc = find_sync_byte(fp, tp_size, 3);
        if (rc == EOK) {
            printf("Sync at byte %d\n", ftell(fp));
        } else {
            printf("Failed to find sync\n");
            exit (-1);
        }
    }

    uint8 tp_buf[tp_size];
    tp_header_t* tp_hdr = (tp_header_t*)(tp_buf);
    af_header_t* af = (af_header_t*)(tp_hdr + 1);

    while (1) {
        int pcr_flag = 0;

        int n = fread(tp_buf, tp_size, 1, fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }
        tp_cnt++;

        pcr_flag = tp_has_pcr(tp_hdr, af);

        if (!skip_non_pcr || pcr_flag) {
            printf("TP %d: sync %02x, tei %d, pusi %d, pri %d, pid %d, "
                   "scr %d, afc %d, cc %d", tp_cnt, tp_hdr->sync,
                   tp_hdr->transport_err, tp_hdr->payload_unit_start,
                   tp_hdr->priority, tp_get_pid(tp_hdr),
                   tp_hdr->scrambling_ctrl, tp_hdr->af_ctrl, tp_hdr->cc);

            if (tp_hdr->af_ctrl & 2) {
                printf("  AF: sz %d", af->len);
                if (af->len > 0) {
                    if (af->discontinuity)  printf(", DISC");
                    if (af->pcr_flag) {
                        printf(", PCR %09x:%d",
                               af_get_pcr_base(af), af_get_pcr_ext(af));
                    }
                }
            }
            printf("\n");
        }

        if (tp_cnt >= num_tp)  break;
    }

    printf("Total %d TP found.\n", tp_cnt);
    fclose(fp);
    return 0;
}
