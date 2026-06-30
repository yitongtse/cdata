h52220
s 00013/00005/00144
d D 1.3 04/07/20 14:36:33 ytse 3 2
c Make sure packets are arrived in order
e
s 00014/00013/00135
d D 1.2 04/07/20 14:33:54 ytse 2 1
c Use 10.24MHz clock ticks for the reference and arrival times
e
s 00148/00000/00000
d D 1.1 04/07/20 13:21:25 ytse 1 0
c date and time created 04/07/20 13:21:25 by ytse
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

    File: datagen.c

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    07-20-04    ytse    Created.

    This program generate transport packet info to a text file for
    clock recovery simulation.

    The output file has the following format:
D 3
        ref_time  timestamp_flag  timestamp  arvl_time
E 3
I 3
        ref_time  timestamp_flag  timestamp  in_time
E 3

    Note:
D 3
      - ref_time and arvl_time are in micro seconds (us).
E 3
I 3
      - ref_time and in_time are in micro seconds (us).
E 3
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "param.h"


typedef unsigned int uint;

typedef struct tp_info_ {
  uint ref_time;
  int ts_flag;
  uint ts;
} tp_info;


int num_tp;
D 2
float src_freq;
float pkt_rate;
E 2
I 2
float clk_freq;                 // reference clock frequency
                                //   Note: reference clock should be considered
                                //         as local clock of the edge device
float ts_freq;                  // source clock frequency
float pkt_rate;                 // packet rate
E 2
uint init_ts;
int min_tp_per_ts;
int max_tp_per_ts;
int tp_per_ip;
float min_delay;
float max_delay;
int delay_skew;
char out_file[128];

FILE* out_fp;

double cur_clk;                 // current reference clock (in us)
double cur_ts;                  // current timestamp (in source clock ticks)

double clk_inc;                 // reference clock increment per packet (in us)
double ts_inc;                  // timestamp increment per packet
                                //   (in source clock ticks)
int num_pkt_till_ts;            // number of pkts till next timestamp
double delay;                   // delay introduced by IP network
D 3
double arvl_time;               // arrival time of packet accoridng to
E 3
I 3
double in_time;                 // arrival time of packet accoridng to
E 3
                                //   reference clock
I 3
double prev_in_time;            // arrival time of previous packet
E 3
tp_info *tp;


// Random number generator (between 0 and 1)
float get_rand ()
{
    return ((float) rand()) / 32767.0;
}

int main(int argc, char **argv)
{
    Param par;
    int i, j;

    if (argc > 1) {
        beginParam(&par, argc, argv);
    } else {
        readParam(&par, "datagen.par");
    }

    commentParam(&par, "Source parameters:");
    getIntParam(&par, "Number of transport packets to generate", &num_tp, 1000);
D 2
    getFloatParam(&par, "Source clock frequency (Hz)", &src_freq, 10240000.0);
E 2
I 2
    getFloatParam(&par, "Reference clock frequency (Hz)", &clk_freq, 10240000.);
    getFloatParam(&par, "Source clock frequency (Hz)", &ts_freq, 10240000.);
E 2
    getFloatParam(&par, "Packet rate (Hz)", &pkt_rate, 25798.0);
    getIntParam(&par, "Initial timestamp value", &init_ts, 0.);
    getIntParam(&par, "Min packets per timestamp", &min_tp_per_ts, 1);
    getIntParam(&par, "Max packets per timestamp", &max_tp_per_ts, 100);

    commentParam(&par, "");
    commentParam(&par, "IP network parameters:");
    getIntParam(&par, "Number of TPs per IP", &tp_per_ip, 7);
D 2
    getFloatParam(&par, "Min IP network delay", &min_delay, 4.0);
    getFloatParam(&par, "Max IP network delay", &max_delay, 4.0);
E 2
I 2
    getFloatParam(&par, "Min IP network delay (us)", &min_delay, 200.);
    getFloatParam(&par, "Max IP network delay (us)", &max_delay, 1000.);
E 2
    getIntParam(&par, "IP delay skew factor", &delay_skew, 2);

D 2
#if 0
E 2
    commentParam(&par, "");
    getStringParam(&par, "Output filename", out_file, "tp_info.dat");
D 2
#endif
E 2
    endParam(&par);

D 2
#if 0
E 2
    out_fp = fopen(out_file, "w");
    if (out_fp == NULL) {
        printf("Error: failed to open output file %s\n", out_file);
        exit(-1);
    }
D 2
#endif
E 2

    cur_clk = 0.0;
    cur_ts = init_ts;
    num_pkt_till_ts = 1;

D 2
    clk_inc = 1000000.0 / pkt_rate;
    ts_inc = src_freq / pkt_rate;
E 2
I 2
    clk_inc = clk_freq / pkt_rate;
    ts_inc = ts_freq / pkt_rate;
E 2

    tp = malloc(sizeof(tp_info) * tp_per_ip);

    for (i=0; i<num_tp; i+=tp_per_ip) {
        for (j=0; j<tp_per_ip; j++) {
            tp[j].ref_time = (uint)cur_clk;
            tp[j].ts = (uint)cur_ts;

            if (--num_pkt_till_ts <= 0) {
                tp[j].ts_flag = 1;
                num_pkt_till_ts = get_rand() * (max_tp_per_ts - min_tp_per_ts)
                                      + min_tp_per_ts;
            } else {
                tp[j].ts_flag = 0;
            }

            cur_clk += clk_inc;
            cur_ts += ts_inc;
        }

        delay = pow(get_rand(), delay_skew) * (max_delay - min_delay)
                    + min_delay;
I 2
        delay = delay * clk_freq / 1000000.;    // convert to ref clk ticks
E 2
D 3
        arvl_time = tp[tp_per_ip-1].ref_time + delay;
E 3
I 3
        in_time = tp[tp_per_ip-1].ref_time + delay;

        // Make sure packets are received in order
        if (in_time < prev_in_time) {
            in_time = prev_in_time;
        } else {
            prev_in_time = in_time;
        }
E 3

        for (j=0; j<tp_per_ip; j++) {
D 2
            printf("%d %d %d %d\n", tp[j].ref_time, tp[j].ts_flag, tp[j].ts,
                   (uint)arvl_time);
E 2
I 2
            fprintf(out_fp, "%d %d %d %d\n", tp[j].ref_time, tp[j].ts_flag,
D 3
                    tp[j].ts, (uint)arvl_time);
E 3
I 3
                    tp[j].ts, (uint)in_time);
E 3
E 2
        }
    }

    free(tp);
    return 0;
}
E 1
