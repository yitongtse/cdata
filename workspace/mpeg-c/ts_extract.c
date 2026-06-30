/*
 * Copyright (c) 2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * MPEG-2 Transport Stream Extraction
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


#if 0
static
void reset_discontinuity (uint32 *tp_buf)
{
    tp_header_t* tp_hdr = (tp_header_t*)tp_buf;
    af_header_t* af_hdr = (af_header_t*)(tp_hdr + 1);
    af_hdr->discontinuity = 0;
}
#endif


int main (int argc, char *argv[])
{
    param_t par;
    char in_file[128];
    char out_file[128];
    uint32 num_tp = -1;
    uint32 tp_size;
    uint32 skip_size;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "ts_extract.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "Transport Stream Extraction");
    param_get_string(&par, "Input bitstream file", in_file, "in.ts");
    param_get_string(&par, "Output bitstream file", out_file, "out.ts");
    param_get_int(&par, "Beginning bytes to skip", &skip_size, 0);
    param_get_int(&par, "Input TP size", &tp_size, TP_SIZE);
    param_get_int(&par, "Number of TPs to extract (-1 for all)", &num_tp, -1);
    param_end(&par);
    printf("\n\n");

    if (tp_size < TP_SIZE) {
        printf("Error: input TP size must be > %d\n", TP_SIZE);
        exit (-1);
    }

    FILE* in_fp = fopen(in_file, "r");
    if (in_fp == NULL) {
        printf("Error: Failed to open input bitstream file %s\n", in_file);
        exit (-1);
    }

    FILE* out_fp = fopen(out_file, "w");
    if (out_fp == NULL) {
        printf("Error: Failed to open output bitstream file %s\n", out_file);
        exit (-1);
    }

    fseek(in_fp, skip_size, SEEK_SET);

    uint32 tp_buf[(tp_size + 3) / 4];
    uint32 tp_cnt = 0;

    while (1) {
        int n = fread(tp_buf, tp_size, 1, in_fp);
        if (n != 1) {
            printf("End of file reached\n");
            break;
        }

#if 0
        // Customized operation
        if (tp_cnt == 2) {
            reset_discontinuity(tp_buf);
        }
#endif

        n = fwrite(tp_buf, TP_SIZE, 1, out_fp);
        if (n != 1) {
            printf("Error: output write failed\n");
            exit (-1);
        }
        tp_cnt++;

        if (tp_cnt >= num_tp)  break;
    }

    printf("Total %d TPs extracted.\n", tp_cnt);
    fclose(in_fp);
    fclose(out_fp);
    return 0;
}
