h13595
s 00006/00040/00098
d D 1.2 05/02/03 14:25:22 ytse 2 1
c No longer support noise
e
s 00138/00000/00000
d D 1.1 05/02/02 01:13:09 ytse 1 0
c date and time created 05/02/02 01:13:09 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    Copyright (c) 2004 by Cisco Systems, Inc.
    All rights reserved.

    File: tsgen.c

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    02-02-05    ytse    Created.

    This program generate timestamp packet info.
I 2
    Each line in the output file represents one timestamp:
    its send time, and its value, both represented as
    32 bit unsigned integer.
E 2

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
D 2
float min_delay;                // minumum network delay
float max_delay;                // maximum network delay
int delay_skew;                 // network delay skew factor
E 2
char out_file[120];
FILE *out_fp;
int ts_idx;

double clk;                     // local clock
double ts;                      // timestamp
double clk_inc;                 // local clock increment between timestamps
double ts_inc;                  // increment between adjacent timestamps
D 2
double delay;                   // timestamp arrival delay
E 2
double arvl_time;               // arrival time
uint ts_val;                    // quantized timestamp value
D 2
uint arvl_time_val;             // quantized arrival time value
uint prev_arvl_time_val;        // previous arrival time value

E 2
I 2
uint clk_val;                   // quantized clock value
E 2


void wrap_min_max (double *x)
{
    if (*x < 0.0) {
        *x += MAX_TS;
    } else if (*x >= MAX_TS) {
        *x -= MAX_TS;
    }
}


D 2
// Random number generator (between 0 and 1)
//
float get_rand ()
{
    return ((float) rand()) / 32767.0;
}


E 2
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
D 2

    commentParam(&par, "");
    commentParam(&par, "IP network parameters:");
    getFloatParam(&par, "Min IP network delay (us)", &min_delay, 200.);
    getFloatParam(&par, "Max IP network delay (us)", &max_delay, 1000.);
    getIntParam(&par, "IP delay skew factor", &delay_skew, 2);

    commentParam(&par, "");
E 2
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
D 2
    prev_arvl_time_val = 0;
E 2

    for (ts_idx=0; ts_idx<num_ts; ts_idx++) {
        ts += ts_inc;
        wrap_min_max(&ts);
        ts_val = (uint)ts;

D 2
        // Simulate IP network delay
        delay = pow(get_rand(), delay_skew) * (max_delay - min_delay)
                    + min_delay;
        delay = delay * clk_freq / 1e6;    // convert to ref clk ticks

E 2
        clk += clk_inc;
        wrap_min_max(&clk);
D 2
        arvl_time = clk + delay;
        wrap_min_max(&arvl_time);
        arvl_time_val = (uint)arvl_time;

        // Make sure packets are received in order
        if ((int)(arvl_time_val - prev_arvl_time_val) < 0) {
            arvl_time_val = prev_arvl_time_val;
        } else {
            prev_arvl_time_val = arvl_time_val;
        }
E 2
I 2
        clk_val = (uint)clk;
E 2

D 2
        printf("%d %d\n", arvl_time_val, ts_val);
E 2
I 2
        fprintf(out_fp, "%u %u\n", clk_val, ts_val);
E 2
    }

    free(out_fp);
    return 0;
}
E 1
