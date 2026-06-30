/*
 * Copyright (c) 2004-2014 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "param.h"
#include "video.h"
#include "video_util.h"
#include "timerec.h"


char in_file[128];
int show_pcr, show_non_pcr;
tp_time_info_t info;


int main (int argc, char *argv[])
{
    param_t par;
    uint32 num_tp = -1;
    FILE*  fp;

    if (argc > 1) {
        param_begin(&par, argc, argv);
    } else {
        param_read(&par, "timerec_view.par");
    }
    par.echo_flag = 1;

    param_comment(&par, "View Time Record");
    param_get_string(&par, "Input file", in_file, "test.in");
    param_get_int(&par, "Number of TPs to process (-1 for all)", &num_tp, -1);
    param_get_int(&par, "show PCR TPs?", &show_pcr, 1);
    param_get_int(&par, "show non-PCR TPs?", &show_non_pcr, 0);
    param_end(&par);


    fp = fopen(in_file, "r");
    if (fp == NULL) {
        printf("Error: Failed to open file %s\n", in_file);
        exit(1);
    }

    while (1) {
        if (tp_time_info_read(fp, &info)) {
            printf("EOF reached.\n");
            break;
        }

        if ((info.pcr_flag && show_pcr) || (!info.pcr_flag && show_non_pcr)) {
            tp_time_info_print(&info);
            printf("\n");
        }

        if (num_tp != -1 && info.tp_idx >= num_tp) {
            break;
        }
    }

    return 0;
}
