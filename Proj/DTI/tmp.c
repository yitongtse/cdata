/*****************************************************************************
    Copyright (c) 2004 by Cisco Systems, Inc.
    All rights reserved.

    File: sim.c

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    07-20-04    ytse    Created.

    This program generate transport packet info and perform clock recovery
    simulation.

    The output file has the following format:
        tp_idx  ref_time  in_ts  out_ts  arvl_time  out_time
        est_tbo  err_tbo  flt_err_tbo  freq_adj  phase_err  ts_jitter

    Note:
      - All time variables are in reference clock ticks

    Changes:
      - An automatic phase adjustment technique is now used
        in the dejitter filter.
      - Reorganize code to make it modular.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "param.h"


typedef unsigned int uint;


// Random number generator (between 0 and 1)
//
float get_rand ()
{
    return ((float) rand()) / 32767.0;
}


int main(int argc, char **argv)
{
    int i;
    double delay;
    FILE *out_fp;

    out_fp = fopen("my.jit", "w");
    if (out_fp == NULL) {
        printf("Error: failed to open output file\n");
        exit(-1);
    }

    for (i=0; i<200000; i++) {
        delay = pow(get_rand(), 2.);
        fprintf(out_fp, "%9g\n", delay);
    }

    free(out_fp);
    return 0;
}
