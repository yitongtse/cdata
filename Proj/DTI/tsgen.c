/*****************************************************************************
    Copyright (c) 2004 by Cisco Systems, Inc.
    All rights reserved.

    File: tsgen.c

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    02-02-05    ytse    Created.

    This program generate timestamp packet info.
    Each line in the output file represents one timestamp:
    its send time, and its value, both represented as
    32 bit unsigned integer.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "param.h"


#define MAX_TS                  ((double) 4294967296.0)


typedef unsigned int uint;


int num_ts;                     // number of timestamps to simulate
float clk_freq;                 // local (reference) clock frequency
float ts_freq;                  // source (timestamp) clock frequency
float ts_rate;                  // timestamp rate
uint init_ts;                   // initial timestamp value
char out_file[120];
FILE *out_fp;
int ts_idx;

double clk;                     // local clock
double ts;                      // timestamp
double clk_inc;                 // local clock increment between timestamps
double ts_inc;                  // increment between adjacent timestamps
double arvl_time;               // arrival time
uint ts_val;                    // quantized timestamp value
uint clk_val;                   // quantized clock value


void wrap_min_max (double *x)
{
    if (*x < 0.0) {
        *x += MAX_TS;
    } else if (*x >= MAX_TS) {
        *x -= MAX_TS;
    }
}


int main(int argc, char **argv)
{
    Param par;

    if (argc > 1) {
        beginParam(&par, argc, argv);
    } else {
        readParam(&par, "tsgen.par");
    }

    commentParam(&par, "Source parameters:");
    getIntParam(&par, "Number of timestamps to generate", &num_ts, 1000);
    getFloatParam(&par, "Reference clock frequency (Hz)", &clk_freq, 10240000.);
    getFloatParam(&par, "Source clock frequency (Hz)", &ts_freq, 10240000.);
    getFloatParam(&par, "Timestamp rate (Hz)", &ts_rate, 25798.0);
    getIntParam(&par, "Initial timestamp value", &init_ts, 0.);
    getStringParam(&par, "Output filename", out_file, "in.ts");
    endParam(&par);

    out_fp = fopen(out_file, "w");
    if (out_fp == NULL) {
        printf("Error: failed to open output file %s\n", out_file);
        exit(-1);
    }

    // Initialize timestamp generation related variables
    clk_inc = clk_freq / ts_rate;
    ts_inc = ts_freq / ts_rate;
    clk = 0.0;
    ts = init_ts;

    for (ts_idx=0; ts_idx<num_ts; ts_idx++) {
        ts += ts_inc;
        wrap_min_max(&ts);
        ts_val = (uint)ts;

        clk += clk_inc;
        wrap_min_max(&clk);
        clk_val = (uint)clk;

        fprintf(out_fp, "%u %u\n", clk_val, ts_val);
    }

    free(out_fp);
    return 0;
}
