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

    Changes in sim2.c:
      - Use a phased approach.  Get the first two timestamps.
        Determine packet rate, and then initialize the dejitter filter
        output with correct packet rate.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "param.h"

typedef unsigned int uint;


#define ERR_TBO_STAT            1

#define SIMPLE_DJF              0
#define CENTER_DJF              1

#define SIMPLE_LPF              1
#define ADAPTIVE_LPF            0
#define SUM_LPF                 0
#define NTAP_LPF                0



// MPEG transport packet (TP) info
//
typedef struct tp_info_ {
  double ref_time;              // reference time when TP is generated
  int ts_flag;                  // whether TP has timestamp
  uint ts;                      // timestamp value
} tp_info;


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
float max_freq_adj;             // maximum frequency adjustment allowed
float sys_delay;                // system delay
float dj_phase_thres;           // dejitter filter phase reset threshold
char out_file[128];             // output filename

// Global variables for input generation
double clk;                     // current reference clock (in us)
double ts;                      // current timestamp (in source clock ticks)
double clk_incr;                // reference clock increment per packet (in us)
double ts_incr;                 // timestamp increment per packet
                                //   (in source clock ticks)
int num_pkt_till_ts;            // number of pkts till next timestamp
double delay;                   // delay introduced by IP network
double in_time;                 // arrival time of packet accoridng to
                                //   reference clock
double prev_in_time;            // arrival time of previous packet

// Clock recovery related variables
uint tp_idx;                    // TP index
uint ts_idx;                    // timestamp index
uint period_idx;                // input period index
double arvl_time;               // arrival time
uint out_ts;                    // output timestamp
double out_time;                // delivery time
int ts_adj;                     // adjustment in output timestamp
double latency;                 // overall latency, including IP,
                                //   input and system delay
int phase_err;                  // recovered clock phase error
uint prev_out_ts;               // previous output timestamp
double prev_ts_out_time;        // output time of previous timestamp
double ts_jitter;               // timestamp jitter

FILE* out_fp;
clkrec_info cr;
tp_info *tp;


///////////////////////////////////////////////////////////////////////////
#if ERR_TBO_STAT
#define STAT_TS_PERIOD          1000
int stat_ts_cnt = 0;
double min_err_tbo = 1e30;
double max_err_tbo = -1e30;
#endif

#if CENTER_DJF
int djf_period = 10000;
int djf_cnt = 0;
double min_djf_phase_err = 10000000;
double max_djf_phase_err = -10000000;
double djf_phase_err;
double avg_djf_phase_err;
#endif

#if ADAPTIVE_LPF
int lpf_taps;
#endif

#if SUM_LPF
double lpf_sum;
#endif

#if NTAP_LPF
int lpf_ntap;
int lpf_cur_tap;
double lpf_sum;
double lpf_taps[10000];
#endif
///////////////////////////////////////////////////////////////////////////

// Random number generator (between 0 and 1)
//
float get_rand ()
{
    return ((float) rand()) / 32767.0;
}


// Dejitter filter
//
void dejitter_filter (clkrec_info *cr, uint arvl_time)
{
    double diff;

    if (!cr->tp_found) {
        cr->dj_arvl_time = arvl_time;
        cr->dj_tpi = 0.;
        cr->tp_found = 1;

    } else {
        cr->in_tpi = arvl_time - cr->prev_arvl_time;

        diff = cr->in_tpi - cr->dj_tpi;
        cr->dj_tpi += diff * djf_par;

        cr->dj_arvl_time += cr->dj_tpi;
    }

    cr->prev_arvl_time = arvl_time;

#if SIMPLE_DJF
    diff = cr->dj_arvl_time - arvl_time;
    if (diff > dj_phase_thres || diff < -dj_phase_thres) {
printf("    ** DJ phase reset: TP %d\n", tp_idx);
        cr->dj_arvl_time = arvl_time;

        // Reset PLL parameter also
        cr->est_tbo = 0.;
        cr->flt_err_tbo = 0.;
        cr->freq_adj = 0.;
    }
#endif

#if CENTER_DJF
    djf_phase_err = arvl_time - cr->dj_arvl_time;
    if (djf_phase_err < min_djf_phase_err) {
        min_djf_phase_err = djf_phase_err;
    }
    if (djf_phase_err > max_djf_phase_err) {
        max_djf_phase_err = djf_phase_err;
    }

    if (++djf_cnt >= djf_period) {
        djf_cnt = 0;
        avg_djf_phase_err = (min_djf_phase_err + max_djf_phase_err) / 2.;

#if 1
printf("TP %d: DJPE: %g - %g, avg %g, thres %g\n",
       tp_idx, min_djf_phase_err, max_djf_phase_err, avg_djf_phase_err,
       dj_phase_thres);
#endif

        min_djf_phase_err = 1000000;
        max_djf_phase_err = -1000000;

        if (avg_djf_phase_err < -dj_phase_thres ||
            avg_djf_phase_err > dj_phase_thres) {

printf(" * DJ phase adj: TP %d, dj_phase %g -> %g\n",
       tp_idx, cr->dj_arvl_time, cr->dj_arvl_time+avg_djf_phase_err);

            cr->dj_arvl_time += avg_djf_phase_err;

            // Reset PLL parameter also
            cr->est_tbo = 0.;
            cr->flt_err_tbo = 0.;
            cr->freq_adj = 0.;
        }
    }
#endif
}


// Clock recovery for TP containing timestamps
//
void clkrec_ts_tp (clkrec_info *cr, uint cur_ts)
{
    double diff;

    if (!cr->ts_found) {
        // For 1st timestamp
        cr->dj_tbo = tp->ts - cr->dj_arvl_time;
        cr->est_tbo = cr->dj_tbo;
        cr->err_tbo = 0.;
        cr->flt_err_tbo = 0.;
        cr->freq_adj = 0.;
        cr->ts_found = 1;

    } else {
        cr->ts_inc = cur_ts - cr->prev_ts;
        cr->dj_tbo = cur_ts - cr->dj_arvl_time;

        // Drift prediction
        cr->est_drift = cr->ts_inc * cr->freq_adj;
        cr->est_tbo += cr->est_drift;

        // Differencing
        cr->err_tbo = cr->dj_tbo - cr->est_tbo;

#if ERR_TBO_STAT
        if (cr->err_tbo > max_err_tbo) {
            max_err_tbo = cr->err_tbo;
        }
        if (cr->err_tbo < min_err_tbo) {
            min_err_tbo = cr->err_tbo;
        }
        if (++stat_ts_cnt > STAT_TS_PERIOD) {
            printf("ErrTBO stat: %g - %g\n",
                   min_err_tbo*1e3/clk_freq, max_err_tbo*1e3/clk_freq);
            min_err_tbo = 1e30;
            max_err_tbo = -1e30;
            stat_ts_cnt = 0;
        }
#endif

        // Normalization
        cr->norm_err_tbo = cr->err_tbo / cr->ts_inc;
    
        // Loop filter
#if SIMPLE_LPF
        diff = cr->norm_err_tbo - cr->flt_err_tbo;
        cr->flt_err_tbo += diff * lpf_par;
#endif

#if ADAPTIVE_LPF
        if (ts_idx >= lpf_taps) {
            diff = cr->norm_err_tbo - cr->flt_err_tbo;
            cr->flt_err_tbo += diff * lpf_par;
        } else {
            diff = cr->norm_err_tbo - cr->flt_err_tbo;
            cr->flt_err_tbo += diff / (ts_idx + 1);
        }
#endif

#if SUM_LPF
        lpf_sum += cr->norm_err_tbo;
        cr->flt_err_tbo = lpf_sum / (ts_idx + 1);
#endif

#if NTAP_LPF
        lpf_sum += cr->norm_err_tbo - lpf_taps[lpf_cur_tap];
        lpf_taps[lpf_cur_tap++] = cr->norm_err_tbo;
        if (lpf_cur_tap >= lpf_ntap) {
            lpf_cur_tap = 0;
        }
        cr->flt_err_tbo =  lpf_sum / lpf_ntap;
#endif

        // Convert to error frequency
        cr->freq_adj = loop_gain * cr->flt_err_tbo;

        // Clamping
        if (cr->freq_adj > max_freq_adj) {
            cr->freq_adj = max_freq_adj;
        } else if (cr->freq_adj < -max_freq_adj) {
            cr->freq_adj = -max_freq_adj;
        } 
    }

    // Compute output timestamp
    out_ts = out_time + cr->est_tbo;

    // Update
    cr->prev_ts = cur_ts;
}


int main(int argc, char **argv)
{
    Param par;
    int i, j;

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
    getFloatParam(&par, "Dejittering filter parameter (1 to turn off)",
                  &djf_par, 0.0004);
    getFloatParam(&par, "Loop filter parameter", &lpf_par, 0.0001);
    getFloatParam(&par, "PLL loop gain", &loop_gain, 0.0001);
    getFloatParam(&par, "Max frequency adjustment (Hz)", &max_freq_adj, 100);
    getFloatParam(&par, "System latency (us)", &sys_delay, 500.);
    getFloatParam(&par, "Dejitter filter phase reset threshold (us)",
                  &dj_phase_thres, 1000);

    getStringParam(&par, "Output filename", out_file, "tp_info.dat");
    endParam(&par);

srand(200);

    out_fp = fopen(out_file, "w");
    if (out_fp == NULL) {
        printf("Error: failed to open output file %s\n", out_file);
        exit(-1);
    }

    // Initialize timestamp generation related variables
    clk_incr = clk_freq / pkt_rate;
    ts_incr = ts_freq / pkt_rate;
    clk = 0.0;
    ts = init_ts;
    num_pkt_till_ts = 1;
    tp = malloc(sizeof(tp_info) * tp_per_ip);

    // Initialize clock recovery related variables
    tp_idx = ts_idx = 0;
    sys_delay = sys_delay * clk_freq / 1e6;
    samp_itvl = samp_itvl * clk_freq / 1e6;
    max_freq_adj = max_freq_adj / clk_freq;    // convert to ratio
    period_idx = 0;
    arvl_time = samp_itvl;
    out_time = 0.;
    prev_in_time = 0;
    dj_phase_thres = dj_phase_thres / 1e6 * clk_freq;

    cr.tp_found = cr.ts_found = 0;

#if ADAPTIVE_LPF
    lpf_taps = (int)(1./lpf_par);
    printf("lpf_taps = %d\n", lpf_taps);
#endif

#if SUM_LPF
    lpf_sum = 0;
#endif

#if NTAP_LPF
    lpf_ntap = 1000;
    lpf_cur_tap = 0;
    lpf_sum = 0.;
    for (i=0; i<lpf_ntap; i++) {
        lpf_taps[i] = 0.;
    }
#endif

    for (i=0; i<num_tp; i+=tp_per_ip) {
        // Generate data for one IP packet
        for (j=0; j<tp_per_ip; j++) {
            tp[j].ref_time = clk;
            tp[j].ts = (uint)ts;

            if (--num_pkt_till_ts <= 0) {
                tp[j].ts_flag = 1;
                num_pkt_till_ts = get_rand() * (max_tp_per_ts - min_tp_per_ts)
                                      + min_tp_per_ts;
            } else {
                tp[j].ts_flag = 0;
            }

            clk += clk_incr;
            ts += ts_incr;
        }

        // Simulate IP network delay
        delay = pow(get_rand(), ip_delay_skew) * (max_ip_delay - min_ip_delay)
                    + min_ip_delay;
        delay = delay * clk_freq / 1e6;    // convert to ref clk ticks
        in_time = tp[tp_per_ip-1].ref_time + delay;

        // Make sure packets are received in order
        if (in_time < prev_in_time) {
            in_time = prev_in_time;
        } else {
            prev_in_time = in_time;
        }

        // Simulate input sampling
        while (in_time > arvl_time) {
            period_idx++;
            arvl_time += samp_itvl;

            // Compute delivery time of first TP in IP
            if (out_time < arvl_time + sys_delay) {
                out_time = arvl_time + sys_delay;
            }
        }

        // Perform clock recovery
        for (j=0; j<tp_per_ip; j++) {

            // Dejittering
            dejitter_filter(&cr, (uint)arvl_time);

            // Special processing for timestamp
            if (tp[j].ts_flag) {
                clkrec_ts_tp(&cr, tp[j].ts);

                // Timestamp adjustment
                ts_adj = out_ts - tp[j].ts;

                // Overall latency
                latency = out_time - tp[j].ref_time;

                // Phase error
                phase_err = tp[j].ts + latency - out_ts; 

                // Check timestamp jitter
                ts_jitter = ((out_ts - prev_out_ts)
                            - (out_time - prev_ts_out_time)) / clk_freq;
                if (ts_jitter < 0) {
                    ts_jitter = -ts_jitter;
                }

                // err_tbo (ms), flt_err_tbo (ms), freq_adj (ppm),
                // phase_err (ms), ts_jitter (ns), dj_phase_err (ms)
                //
                fprintf(out_fp, "%9g %9g %9g %9g %9g %9g\n",
                        cr.err_tbo*1e3/clk_freq, cr.flt_err_tbo*1e3/clk_freq,
                        cr.freq_adj*1e6, phase_err*1e3/clk_freq, ts_jitter*1e9,
                        (cr.dj_arvl_time-tp[j].ref_time)*1e3/clk_freq);

                // Update for timestmp-carrying TPs
                prev_out_ts = out_ts;
                prev_ts_out_time = out_time;
                ts_idx++;
            }

            // Update for all TPs
            out_time += clk_incr;    // output time for next TP in IP
            tp_idx++;
        }
    }

    free(tp);
    return 0;
}

