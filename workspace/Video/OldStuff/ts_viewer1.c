/*
 * Copyright (c) 2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * Input Bitrate Analysis
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include "common.h"
#include "param.h"
#include "video_hdr.h"


void print_raw_bytes (uint8 *buf, int len)
{
    int i;
    for (i=0; i<len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}


int main (int argc, char *argv[])
{
    param_t par;
    char file[128];
    uint32 num_tp = -1;
    uint32 tp_cnt = 0;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "ts_viewer.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "Transport Stream Viewer");
    param_get_string(&par, "Bitstream file", file, "test.ts");
    param_get_int(&par, "Number of TPs to process (-1 for all)", &num_tp, -1);
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
        int pcr_flag = 0;

        int n = fread(tp_buf, TP_SIZE, 1, fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }
        tp_cnt++;

        hdr[0] = htonl(tp_buf[0]);

        if (tp_hdr->af_ctrl & 2) {
            hdr[1] = htonl(tp_buf[1]);
            hdr[2] = htonl(tp_buf[2]);
            pcr_flag = af->pcr_flag;
        }

//        if (pcr_flag) {
            printf("\nTP %d: sync %02x, tei %d, pusi %d, pri %d, pid %d, "
                   "scr %d, afc %d, cc %d\n", tp_cnt, tp_hdr->sync,
                   tp_hdr->transport_err, tp_hdr->payload_unit_start,
                   tp_hdr->priority, tp_hdr->pid, tp_hdr->scrambling_ctrl,
                   tp_hdr->af_ctrl, tp_hdr->cc);

            if (tp_hdr->af_ctrl & 2) {
                printf("  AF: sz %d", af->len);
                if (af->discontinuity)  printf(", DISC");
                if (af->pcr_flag) {
                    uint64 pcr_base = (af->pcr_base_1 << 17)
                                      | (af->pcr_base_2 << 1) | af->pcr_base_3;
                    printf(", PCR %09x:%d", pcr_base, af->pcr_ext);
                }
                printf("\n");
            }
//        }

        if (tp_cnt >= num_tp)  break;
    }

    printf("Total %d TP found.\n", tp_cnt);
    fclose(fp);
    return 0;
}
