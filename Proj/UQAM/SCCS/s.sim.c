h12395
s 00005/00003/00411
d D 1.10 05/02/01 17:03:49 ytse 10 9
c Backup before fixing NaN issue.
e
s 00046/00020/00368
d D 1.9 04/11/12 18:12:11 ytse 9 8
c Backup before changes for further study
e
s 00085/00110/00303
d D 1.8 04/07/29 11:55:35 ytse 8 7
c Reorganize code to make it modular
e
s 00065/00004/00348
d D 1.7 04/07/29 11:12:00 ytse 7 6
c Backup before major reorganizatino
e
s 00022/00016/00330
d D 1.6 04/07/27 12:36:26 ytse 6 5
c update
e
s 00051/00046/00295
d D 1.5 04/07/26 16:29:58 ytse 5 4
c Update
e
s 00008/00017/00333
d D 1.4 04/07/23 17:24:24 ytse 4 3
c Allow turning off dejittering
e
s 00018/00015/00332
d D 1.3 04/07/23 17:02:37 ytse 3 2
c Debug
e
s 00042/00015/00305
d D 1.2 04/07/23 14:18:54 ytse 2 1
c Update
e
s 00320/00000/00000
d D 1.1 04/07/22 01:36:59 ytse 1 0
c date and time created 04/07/22 01:36:59 by ytse
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

    File: sim.c

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    07-20-04    ytse    Created.

    This program generate transport packet info and perform clock recovery
    simulation.

    The output file has the following format:
D 3
        ref_time  timestamp_flag  timestamp  in_time
        arvl_time  dj_arvl_time  out_time  out_timestamp
E 3
I 3
        tp_idx  ref_time  in_ts  out_ts  arvl_time  out_time
D 5
        est_tbo  err_tbo  flt_err_tbo  freq_adj  phase_err
E 5
I 5
        est_tbo  err_tbo  flt_err_tbo  freq_adj  phase_err  ts_jitter
E 5
E 3

    Note:
      - All time variables are in reference clock ticks
I 7

    Changes:
      - An automatic phase adjustment technique is now used
        in the dejitter filter.
I 8
      - Reorganize code to make it modular.
E 8

E 7
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "param.h"


I 9
#define ERR_TBO_STAT            0


E 9
typedef unsigned int uint;

I 8

// MPEG transport packet (TP) info
//
E 8
typedef struct tp_info_ {
D 6
  uint ref_time;                // reference time when TP is generated
E 6
I 6
  double ref_time;              // reference time when TP is generated
E 6
  int ts_flag;                  // whether TP has timestamp
  uint ts;                      // timestamp value
} tp_info;


I 8
// Clock recovery state data structure
//
typedef struct {
  char tp_found;                // whether the first TP has been found
  char ts_found;                // whether the first timestamp has been found
  char rate_found;              // whether the input packet rate is found
  double in_tpi;                // input TP interval
  double dj_tpi;                // dejittered TP interval
  double dj_arvl_time;          // dejittered arrival time
  double prev_arvl_time;        // arrival time of previous packet

  double dj_tbo;                // time base offset between timestamp
  double freq_adj;              // clock frequency adjustment (in ratio)
  double est_drift;             // estimated drift in phase from previous
                                //   timestamps
  double est_tbo;               // estimated time base ofset
  double err_tbo;               // estimation error in time base offset
                                //   and dejittered arrival time
  double norm_err_tbo;          // normalized time base offset error
  double flt_err_tbo;           // filtered time base offset error

  uint prev_ts_tp_idx;          // TP index of the previous timestamp
  uint prev_ts;                 // previous timestamp value
  uint ts_inc;                  // increment between current and previous
                                //   timestamps
} clkrec_info;


E 8
// Input parameters
int num_tp;                     // number of TP to simulate
float clk_freq;                 // reference clock frequency
                                //   Note: reference clock should be considered
                                //         as local clock of the edge device
float ts_freq;                  // source clock frequency
float pkt_rate;                 // TP packet rate
uint init_ts;                   // initial timestamp value
int min_tp_per_ts;              // minimum number of TPs between timestamps
int max_tp_per_ts;              // maximum number of TPs between timestamps
int tp_per_ip;                  // number of TPs per IP packet
float min_ip_delay;             // minumum IP network delay
float max_ip_delay;             // maximum IP network delay
int ip_delay_skew;              // IP network delay skew factor
float samp_itvl;                // input sampling interval (in us)
float djf_par;                  // dejitter filter parameter
float lpf_par;                  // low pass filter parameter
float loop_gain;                // loop gain
I 2
float max_freq_adj;             // maximum frequency adjustment allowed
E 2
float sys_delay;                // system delay
I 7
float dj_phase_thres;           // dejitter filter phase reset threshold
E 7
char out_file[128];             // output filename
I 10
int seed;                       // random number seed
E 10

D 8
FILE* out_fp;

E 8
I 8
// Global variables for input generation
E 8
double clk;                     // current reference clock (in us)
double ts;                      // current timestamp (in source clock ticks)
D 8

E 8
double clk_incr;                // reference clock increment per packet (in us)
double ts_incr;                 // timestamp increment per packet
                                //   (in source clock ticks)
int num_pkt_till_ts;            // number of pkts till next timestamp
double delay;                   // delay introduced by IP network
double in_time;                 // arrival time of packet accoridng to
                                //   reference clock
double prev_in_time;            // arrival time of previous packet
D 8
tp_info *tp;


// Global variables for clock recovery
D 5
uint arvl_time;         // arrival time
uint period_idx;        // input period index
E 5
I 5
D 6
uint arvl_time;                 // arrival time
E 6
I 6
float arvl_time;                // arrival time
E 6
uint period_idx;                // input period index
E 5

D 5
double dj_arvl_time;    // dejittered arrival time
uint prev_arvl_time;    // arrival time of previous packet
double in_tpi;          // input TP interval
double dj_tpi;          // dejittered TP interval
E 5
I 5
double dj_arvl_time;            // dejittered arrival time
D 6
uint prev_arvl_time;            // arrival time of previous packet
E 6
I 6
float prev_arvl_time;           // arrival time of previous packet
E 6
double in_tpi;                  // input TP interval
double dj_tpi;                  // dejittered TP interval
E 8
E 5

// Clock recovery related variables
D 5
int tp_found;           // whether the 1st TP has been found
int ts_found;           // whether the 1st timestamp has been found
E 5
I 5
D 8
int tp_found;                   // whether the 1st TP has been found
int ts_found;                   // whether the 1st timestamp has been found
E 5

E 8
D 5
uint tp_idx;            // TP index
uint ts_idx;            // timestamp index
uint prev_ts_tp_idx;    // TP index of the previous timestamp
uint prev_ts;           // previous timestamp value
D 2
uint ts_tp_idx_inc;     // increment between TP index containing
                        //   current and previous timestamps
E 2
uint ts_inc;            // increment between current and previous timestamps
double dj_tbo;          // time base offset between timestamp
double freq_adj;        // clock frequency adjustment (in ratio)
D 2
double tbo_drift;       // drift in phase btw previous and current timestamps
E 2
I 2
double est_drift;       // estimated drift in phase from previous timestamps
E 2
double est_tbo;         // estimated time base ofset
double err_tbo;         // estimation error in time base offset
                        //   and dejittered arrival time
double norm_err_tbo;    // normalized time base offset error
double flt_err_tbo;     // filtered time base offset error
E 5
I 5
uint tp_idx;                    // TP index
uint ts_idx;                    // timestamp index
D 8
uint prev_ts_tp_idx;            // TP index of the previous timestamp
uint prev_ts;                   // previous timestamp value
uint ts_inc;                    // increment between current and previous
                                //   timestamps
double dj_tbo;                  // time base offset between timestamp
double freq_adj;                // clock frequency adjustment (in ratio)
double est_drift;               // estimated drift in phase from previous
                                //   timestamps
double est_tbo;                 // estimated time base ofset
double err_tbo;                 // estimation error in time base offset
                                //   and dejittered arrival time
double norm_err_tbo;            // normalized time base offset error
double flt_err_tbo;             // filtered time base offset error
E 5

E 8
I 8
uint period_idx;                // input period index
double arvl_time;               // arrival time
E 8
D 5
uint out_ts;            // output timestamp
uint out_time;          // delivery time
E 5
I 5
uint out_ts;                    // output timestamp
D 6
uint out_time;                  // delivery time
E 6
I 6
double out_time;                // delivery time
E 6
E 5
D 8

E 8
I 2
D 5
int ts_adj;      // adjustment in output timestamp
int latency;     // overall latency, including IP, input and system delay
int phase_err;   // recovered clock phase error
E 5
I 5
int ts_adj;                     // adjustment in output timestamp
D 6
int latency;                    // overall latency, including IP,
E 6
I 6
double latency;                 // overall latency, including IP,
E 6
                                //   input and system delay
int phase_err;                  // recovered clock phase error
E 5
D 8

E 8
I 5
uint prev_out_ts;               // previous output timestamp
D 6
uint prev_ts_out_time;          // output time of previous timestamp
E 6
I 6
double prev_ts_out_time;        // output time of previous timestamp
E 6
D 9
double ts_jitter;               // timestamp jitter
E 9
I 9
double ts_jitter = 0;           // timestamp jitter
E 9
E 5
E 2

I 8
FILE* out_fp;
clkrec_info cr;
tp_info *tp;

E 8

I 9
#if ERR_TBO_STAT
#define STAT_TS_PERIOD          1000
int stat_ts_cnt = 0;
double min_err_tbo = 1e30;
double max_err_tbo = -1e30;
#endif


E 9
// Random number generator (between 0 and 1)
//
float get_rand ()
{
    return ((float) rand()) / 32767.0;
}


// Dejitter filter
//
D 6
void dejitter_filter ()
E 6
I 6
D 8
void dejitter_filter (uint arvl_time)
E 8
I 8
void dejitter_filter (clkrec_info *cr, uint arvl_time)
E 8
E 6
{
I 3
D 4
#if 0
E 4
E 3
    double diff;

D 8
    if (!tp_found) {
        dj_arvl_time = arvl_time;
I 5
        dj_tpi = 0.;
E 5
        tp_found = 1;
E 8
I 8
    if (!cr->tp_found) {
        cr->dj_arvl_time = arvl_time;
        cr->dj_tpi = 0.;
        cr->tp_found = 1;
I 9
D 10
//cr->dj_tpi = 397.;
E 10
E 9
E 8

    } else {
D 8
        in_tpi = arvl_time - prev_arvl_time;
E 8
I 8
        cr->in_tpi = arvl_time - cr->prev_arvl_time;
E 8

D 8
        diff = in_tpi - dj_tpi;
        dj_tpi += diff * djf_par;
E 8
I 8
        diff = cr->in_tpi - cr->dj_tpi;
        cr->dj_tpi += diff * djf_par;
E 8

D 8
        dj_arvl_time += dj_tpi;
E 8
I 8
        cr->dj_arvl_time += cr->dj_tpi;
E 8
D 4
        prev_arvl_time = arvl_time;
E 4
    }
I 3
D 4
#else
    // Turn off dejitter filter
    dj_arvl_time = arvl_time;
#endif
E 4
I 4

D 8
    prev_arvl_time = arvl_time;
E 8
I 8
    cr->prev_arvl_time = arvl_time;
E 8
I 7

D 8
    diff = dj_arvl_time - arvl_time;
E 8
I 8
    diff = cr->dj_arvl_time - arvl_time;
E 8
    if (diff > dj_phase_thres || diff < -dj_phase_thres) {
D 10
printf("    ** DJ phase reset: TP %d\n", tp_idx);
E 10
I 10
        printf("    ** DJ phase reset: TP %d\n", tp_idx);
E 10
D 8
        dj_arvl_time = arvl_time;
E 8
I 8
        cr->dj_arvl_time = arvl_time;
E 8

        // Reset PLL parameter also
D 8
        est_tbo = 0.;
        flt_err_tbo = 0.;
        freq_adj = 0.;
E 8
I 8
        cr->est_tbo = 0.;
        cr->flt_err_tbo = 0.;
        cr->freq_adj = 0.;
E 8
    }
E 7
E 4
E 3
}


// Clock recovery for TP containing timestamps
//
D 8
void clkrec_ts_tp (uint cur_ts)
E 8
I 8
void clkrec_ts_tp (clkrec_info *cr, uint cur_ts)
E 8
{
    double diff;

D 4
    dejitter_filter();

E 4
D 8
    if (!ts_found) {
E 8
I 8
    if (!cr->ts_found) {
E 8
        // For 1st timestamp
D 8
        dj_tbo = tp->ts - dj_arvl_time;
        est_tbo = dj_tbo;
        err_tbo = 0.;
I 5
        flt_err_tbo = 0.;
        freq_adj = 0.;
E 5
        ts_found = 1;
E 8
I 8
        cr->dj_tbo = tp->ts - cr->dj_arvl_time;
        cr->est_tbo = cr->dj_tbo;
        cr->err_tbo = 0.;
        cr->flt_err_tbo = 0.;
        cr->freq_adj = 0.;
        cr->ts_found = 1;
E 8

    } else {
D 8
        ts_inc = cur_ts - prev_ts;
D 2
        ts_tp_idx_inc = tp_idx = prev_ts_tp_idx;
E 2
        dj_tbo = cur_ts - dj_arvl_time;
E 8
I 8
        cr->ts_inc = cur_ts - cr->prev_ts;
        cr->dj_tbo = cur_ts - cr->dj_arvl_time;
E 8

        // Drift prediction
D 2
        tbo_drift = ts_inc * freq_adj;
        est_tbo += tbo_drift;
E 2
I 2
D 8
        est_drift = ts_inc * freq_adj;
        est_tbo += est_drift;
E 2

I 7
//printf("    Pred: freqAdj %g, TSinc %d, estDft %.2f, estTBO %.2f\n",
//       freq_adj, ts_inc, est_drift, est_tbo);
E 8
I 8
        cr->est_drift = cr->ts_inc * cr->freq_adj;
        cr->est_tbo += cr->est_drift;
E 8

E 7
        // Differencing
D 8
        err_tbo = dj_tbo - est_tbo;
E 8
I 8
        cr->err_tbo = cr->dj_tbo - cr->est_tbo;
E 8

I 9
#if ERR_TBO_STAT
        if (cr->err_tbo > max_err_tbo) {
            max_err_tbo = cr->err_tbo;
        }
        if (cr->err_tbo < min_err_tbo) {
            min_err_tbo = cr->err_tbo;
        }
        if (++stat_ts_cnt > STAT_TS_PERIOD) {
//            printf("ErrTBO stat: %g - %g\n",
//                   min_err_tbo*1e3/clk_freq, max_err_tbo*1e3/clk_freq);
            min_err_tbo = 1e30;
            max_err_tbo = -1e30;
            stat_ts_cnt = 0;
        }
#endif

E 9
        // Normalization
D 8
        norm_err_tbo = err_tbo / ts_inc;
E 8
I 8
        cr->norm_err_tbo = cr->err_tbo / cr->ts_inc;
E 8
D 5

E 5
I 5
    
E 5
        // Loop filter
D 8
        diff = norm_err_tbo - flt_err_tbo;
        flt_err_tbo += diff * lpf_par;
E 8
I 8
        diff = cr->norm_err_tbo - cr->flt_err_tbo;
        cr->flt_err_tbo += diff * lpf_par;
E 8

        // Convert to error frequency
D 8
        freq_adj = loop_gain * flt_err_tbo;

I 7
//printf("    Update: errTBO %.2f, fltErrTBO %g, freqAdj %g\n",
//       err_tbo, flt_err_tbo, freq_adj);
E 8
I 8
        cr->freq_adj = loop_gain * cr->flt_err_tbo;
E 8

E 7
D 2
        // Clamping (TBD)
E 2
I 2
        // Clamping
D 8
        if (freq_adj > max_freq_adj) {
I 6
D 7
printf("Freq adj: %g -> %g\n", freq_adj, max_freq_adj);
E 7
I 7
//printf("Freq adj: %g -> %g\n", freq_adj, max_freq_adj);
E 7
E 6
            freq_adj = max_freq_adj;
        } else if (freq_adj < -max_freq_adj) {
            freq_adj = -max_freq_adj;
I 6
D 7
printf("Freq adj: %g -> %g\n", freq_adj, -max_freq_adj);
E 7
I 7
//printf("Freq adj: %g -> %g\n", freq_adj, -max_freq_adj);
E 8
I 8
        if (cr->freq_adj > max_freq_adj) {
            cr->freq_adj = max_freq_adj;
        } else if (cr->freq_adj < -max_freq_adj) {
            cr->freq_adj = -max_freq_adj;
E 8
E 7
E 6
        } 
E 2
    }

D 5
    // Compute delivery time
    out_time = arvl_time + sys_delay;

E 5
    // Compute output timestamp
D 2
    out_ts = cur_ts + est_tbo;
E 2
I 2
D 8
    out_ts = out_time + est_tbo;
E 8
I 8
    out_ts = out_time + cr->est_tbo;
E 8
I 3

    // Update
D 8
    prev_ts = cur_ts;
E 8
I 8
    cr->prev_ts = cur_ts;
E 8
E 3
E 2
}


D 5
// Clock recovery for TP without timestamps
//
void clkrec_non_ts_tp ()
{
D 4
    dejitter_filter();
E 4
    out_time = arvl_time + sys_delay;
}


E 5
int main(int argc, char **argv)
{
    Param par;
    int i, j;
D 4
    uint in_time2;
    uint dj_arvl_time2;
E 4

    if (argc > 1) {
        beginParam(&par, argc, argv);
    } else {
        readParam(&par, "sim.par");
    }

    commentParam(&par, "Source parameters:");
    getIntParam(&par, "Number of transport packets to generate", &num_tp, 1000);
    getFloatParam(&par, "Reference clock frequency (Hz)", &clk_freq, 10240000.);
    getFloatParam(&par, "Source clock frequency (Hz)", &ts_freq, 10240000.);
    getFloatParam(&par, "Packet rate (Hz)", &pkt_rate, 25798.0);
    getIntParam(&par, "Initial timestamp value", &init_ts, 0.);
    getIntParam(&par, "Min packets per timestamp", &min_tp_per_ts, 1);
    getIntParam(&par, "Max packets per timestamp", &max_tp_per_ts, 100);

    commentParam(&par, "");
    commentParam(&par, "IP network parameters:");
    getIntParam(&par, "Number of TPs per IP", &tp_per_ip, 7);
    getFloatParam(&par, "Min IP network delay (us)", &min_ip_delay, 200.);
    getFloatParam(&par, "Max IP network delay (us)", &max_ip_delay, 1000.);
    getIntParam(&par, "IP delay skew factor", &ip_delay_skew, 2);

    commentParam(&par, "");
    getFloatParam(&par, "Input sampling interval (us)", &samp_itvl, 100);
D 4
    getFloatParam(&par, "Dejittering filter parameter", &djf_par, 0.0004);
E 4
I 4
    getFloatParam(&par, "Dejittering filter parameter (1 to turn off)",
                  &djf_par, 0.0004);
E 4
    getFloatParam(&par, "Loop filter parameter", &lpf_par, 0.0001);
    getFloatParam(&par, "PLL loop gain", &loop_gain, 0.0001);
I 2
    getFloatParam(&par, "Max frequency adjustment (Hz)", &max_freq_adj, 100);
E 2
    getFloatParam(&par, "System latency (us)", &sys_delay, 500.);
I 7
    getFloatParam(&par, "Dejitter filter phase reset threshold (us)",
                  &dj_phase_thres, 1000);
E 7

    getStringParam(&par, "Output filename", out_file, "tp_info.dat");
I 10
//    getIntParam(&par, "Random number seed", &seed, 0);
E 10
    endParam(&par);

I 9
D 10
//srand(320);
E 10
E 9
    out_fp = fopen(out_file, "w");
    if (out_fp == NULL) {
        printf("Error: failed to open output file %s\n", out_file);
        exit(-1);
    }

I 10
//    srand((unsigned int)seed);

E 10
    // Initialize timestamp generation related variables
    clk_incr = clk_freq / pkt_rate;
    ts_incr = ts_freq / pkt_rate;
    clk = 0.0;
    ts = init_ts;
    num_pkt_till_ts = 1;
    tp = malloc(sizeof(tp_info) * tp_per_ip);
I 7
D 8
#if 0
    printf("Packet rate: %g\n", pkt_rate);
    printf("Reference clock: frequency %f, increment %f\n", clk_freq, clk_incr);
    printf("Time stamp: frequency %f, increment %f\n", ts_freq, ts_incr);
#endif
E 8
E 7

    // Initialize clock recovery related variables
I 8
    tp_idx = ts_idx = 0;
E 8
D 2
    sys_delay = sys_delay * clk_freq / 1000000.;
    samp_itvl = samp_itvl * clk_freq / 1000000.;
E 2
I 2
    sys_delay = sys_delay * clk_freq / 1e6;
    samp_itvl = samp_itvl * clk_freq / 1e6;
    max_freq_adj = max_freq_adj / clk_freq;    // convert to ratio
I 6
D 7
printf("max_freq_adj = %f\n", max_freq_adj);
E 7
I 7
D 8
//printf("max_freq_adj = %g\n", max_freq_adj);
E 8
E 7
E 6
E 2
    period_idx = 0;
    arvl_time = samp_itvl;
I 9
    out_time = 0.;
E 9
D 8
    tp_idx = ts_idx = 0;
    tp_found = ts_found = 0;
E 8
D 3
    dj_tpi = 0;
E 3
I 3
D 5
    dj_tpi = 0.;
    flt_err_tbo = 0.;
E 5
I 5
    prev_in_time = 0;
I 7
    dj_phase_thres = dj_phase_thres / 1e6 * clk_freq;
E 7
E 5
E 3

I 8
    cr.tp_found = cr.ts_found = 0;

E 8
D 2

E 2
    for (i=0; i<num_tp; i+=tp_per_ip) {
D 2

E 2
        // Generate data for one IP packet
I 7
D 8
//printf("\n----------------------------------------------------------------\n");
//printf("IP %d:\n", i);
E 8
E 7
        for (j=0; j<tp_per_ip; j++) {
D 6
            tp[j].ref_time = (uint)clk;
E 6
I 6
            tp[j].ref_time = clk;
E 6
            tp[j].ts = (uint)ts;

            if (--num_pkt_till_ts <= 0) {
                tp[j].ts_flag = 1;
                num_pkt_till_ts = get_rand() * (max_tp_per_ts - min_tp_per_ts)
                                      + min_tp_per_ts;
I 7
D 8
//printf("    Next TS interval: %d\n", num_pkt_till_ts);
E 8
E 7
            } else {
                tp[j].ts_flag = 0;
            }
I 7
D 8
//printf("  TP %d: time %.2f, TS %d, flag %d\n",
//       j, tp[j].ref_time, tp[j].ts, tp[j].ts_flag);
E 8
E 7

            clk += clk_incr;
            ts += ts_incr;
        }

        // Simulate IP network delay
        delay = pow(get_rand(), ip_delay_skew) * (max_ip_delay - min_ip_delay)
                    + min_ip_delay;
D 2
        delay = delay * clk_freq / 1000000.;    // convert to ref clk ticks
E 2
I 2
        delay = delay * clk_freq / 1e6;    // convert to ref clk ticks
E 2
        in_time = tp[tp_per_ip-1].ref_time + delay;
I 7
D 8
//printf("  Delay %f, In_time %.2f\n", delay, in_time);
E 8
E 7

        // Make sure packets are received in order
        if (in_time < prev_in_time) {
            in_time = prev_in_time;
I 7
D 8
//printf("    ** reversed order\n");
E 8
E 7
        } else {
            prev_in_time = in_time;
        }

        // Simulate input sampling
D 2
        if (in_time > arvl_time) {
E 2
I 2
        while (in_time > arvl_time) {
E 2
            period_idx++;
            arvl_time += samp_itvl;
I 7
D 8
//printf("Period %d: Arvl Time %.2f\n", period_idx, arvl_time);
E 8
E 7
D 2
            printf("Period %d: freq_adj %g\n", period_idx, freq_adj);
E 2
I 2
D 3
#if 0
            printf("Per %d: %d TS, freq_adj %g ppm, TBO %g ms\n", period_idx,
                   ts_idx, freq_adj * 1e6, phase_err * 1e3 / clk_freq);
#endif
E 3
E 2
D 9
        }
E 9

I 6
D 9
        // Compute delivery time of first TP
        out_time = arvl_time + sys_delay;
E 9
I 9
            // Compute delivery time of first TP in IP
            if (out_time < arvl_time + sys_delay) {
                out_time = arvl_time + sys_delay;
            }
        }
E 9
I 7
D 8
//printf("\nClkrec: in time %.2f, arvl time %.2f, base out time %.2f\n",
//        in_time, arvl_time, out_time);
E 8
E 7

E 6
        // Perform clock recovery
        for (j=0; j<tp_per_ip; j++) {
I 4

            // Dejittering
D 6
            dejitter_filter();

I 5
            // Compute delivery time
            out_time = arvl_time + sys_delay;
E 6
I 6
D 8
            dejitter_filter((uint)arvl_time);
I 7
//printf("  TP %d: djaT %.2f, outTime %.2f\n", j, dj_arvl_time, out_time);
E 8
I 8
            dejitter_filter(&cr, (uint)arvl_time);
E 8
E 7
E 6

            // Special processing for timestamp
E 5
E 4
            if (tp[j].ts_flag) {
D 8
                clkrec_ts_tp(tp[j].ts);
E 8
I 8
                clkrec_ts_tp(&cr, tp[j].ts);
E 8
I 2

                // Timestamp adjustment
                ts_adj = out_ts - tp[j].ts;

                // Overall latency
                latency = out_time - tp[j].ref_time;

                // Phase error
                phase_err = tp[j].ts + latency - out_ts; 

E 2
D 3
                ts_idx++;
E 3
I 2
D 4
#if 1
E 4
D 3
                fprintf(out_fp, "%g %g\n",
                        freq_adj * 1e6, phase_err * 1e3 / clk_freq);
E 3
I 3
D 5
                fprintf(out_fp, "%7d %7d %7d %7d %7d %7d %9g %9g %9g %9g %9g\n",
E 5
I 5
                // Check timestamp jitter
D 9
                ts_jitter = ((out_ts - prev_out_ts)
                            - (out_time - prev_ts_out_time)) / clk_freq;
E 9
I 9
                if (ts_idx > 0) {
                    ts_jitter = ((out_ts - prev_out_ts)
                                - (out_time - prev_ts_out_time)) / clk_freq;
                }
E 9
I 7
                if (ts_jitter < 0) {
                    ts_jitter = -ts_jitter;
                }
E 7

I 7
D 8
//printf("    TS %d->%d (%d)\n", tp[j].ts, out_ts, out_ts - tp[j].ts);

E 8
D 9
#if 0
E 7
D 6
                fprintf(out_fp, "%7d %7d %7d %7d %7d %7d %9g %9g %9g %9g %9g %9g\n",
E 5
                        tp_idx, tp[j].ref_time, tp[j].ts, out_ts, arvl_time,
                        out_time, est_tbo, err_tbo, flt_err_tbo, freq_adj*1e6,
D 5
                        phase_err*1e3/clk_freq);
E 5
I 5
                        phase_err*1e3/clk_freq, ts_jitter*1e9);
E 6
I 6
                fprintf(out_fp,
                        "%7d %7d %7d %7d %7d %9g %9g %9g %9g %9g %9g %9g\n",
                        tp_idx, (uint)tp[j].ref_time, tp[j].ts, out_ts,
                        (uint)arvl_time, out_time, est_tbo, err_tbo,
                        flt_err_tbo, freq_adj*1e6, phase_err*1e3/clk_freq,
                        ts_jitter*1e9);
I 7
#endif

#if 1
D 8
                fprintf(out_fp, "%9g %9g %9g %9g %9g\n", flt_err_tbo,
                        freq_adj*1e6, phase_err*1e3/clk_freq, ts_jitter*1e9,
                        err_tbo);
E 8
I 8
                fprintf(out_fp, "%9g %9g %9g %9g %9g\n", cr.flt_err_tbo,
E 9
I 9
                // err_tbo (ms), flt_err_tbo (ms), freq_adj (ppm),
                // phase_err (ms), ts_jitter (ns), dj_phase_err (ms)
                //
                fprintf(out_fp, "%9g %9g %9g %9g %9g %9g\n",
                        cr.err_tbo*1e3/clk_freq, cr.flt_err_tbo,
E 9
                        cr.freq_adj*1e6, phase_err*1e3/clk_freq, ts_jitter*1e9,
D 9
                        cr.err_tbo);
E 8
#endif
E 9
I 9
                        (cr.dj_arvl_time-tp[j].ref_time)*1e3/clk_freq);
E 9
E 7
E 6
E 5
E 3
D 4
#endif
E 4

I 5
D 7
                // Update
E 7
I 7
                // Update for timestmp-carrying TPs
E 7
                prev_out_ts = out_ts;
                prev_ts_out_time = out_time;
E 5
I 3
                ts_idx++;
D 5

E 3
E 2
            } else {
                clkrec_non_ts_tp();
E 5
            }
D 4

I 2
D 3
#if 0
E 3
E 2
            in_time2 = in_time;
            dj_arvl_time2 = dj_arvl_time;
E 4
D 8

I 7
#if 0
                fprintf(out_fp, "%f %f %f\n",
                        in_tpi, dj_tpi, dj_arvl_time - arvl_time);
#endif
E 8

            // Update for all TPs
E 7
I 6
            out_time += clk_incr;    // output time for next TP in IP
E 6
D 3
            fprintf(out_fp, "%7d %1d %7d %7d %7d %7d %7d %7d\n",
                   tp[j].ref_time, tp[j].ts_flag, tp[j].ts, in_time2, arvl_time,
                   dj_arvl_time2, out_time, (tp[j].ts_flag? out_ts : 0));
I 2
#endif
E 3
E 2
            tp_idx++;
        }
    }

    free(tp);
    return 0;
}
D 9

E 9
E 1
