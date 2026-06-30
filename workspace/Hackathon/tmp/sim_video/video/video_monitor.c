/*
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "video.h"
#include "video_util.h"
#include "cli_common.h"


int mon_period;
int ses_mon_wake_cnt;
uint32 mon_time;
out_session_t *mon_out_ses = NULL;
video_in_stat_t mon_in_stat;
video_out_stat_t mon_out_stat;
FILE *mon_fp = NULL;


void video_monitor_init (void)
{
    memset(&mon_in_stat, 0, sizeof(video_in_stat_t));
    memset(&mon_out_stat, 0, sizeof(video_out_stat_t));
}


void video_monitor_error_report (void)
{
    static uint8 info_overrun_alarm_flag = FALSE;
    static uint8 overdue_alarm_flag = FALSE;
    video_in_stat_t in_stat;
    video_out_stat_t out_stat;
    int32 diff;

    // Collect statistics
    video_in_stat_collect(&in_stat);
    video_out_stat_collect(&out_stat);

    // Dejitter buffer underflow
    diff = (int32)(in_stat.underflow_cnt - mon_in_stat.underflow_cnt);
    if (diff > 0) {
//        VIDEO_LOG("In U/F %d", diff);
        mon_in_stat.underflow_cnt += diff;
    }

    // Dejitter buffer overflow
    diff = (int32)(in_stat.overflow_cnt - mon_in_stat.overflow_cnt);
    if (diff > 0) {
        VIDEO_LOG("In O/F %d", diff);
        mon_in_stat.overflow_cnt += diff;
    }

    // Output underflow
    diff = (int32)(out_stat.underflow_cnt - mon_out_stat.underflow_cnt);
    if (diff > 0) {
//        VIDEO_LOG("Out U/F %d", diff);
        mon_out_stat.underflow_cnt += diff;
    }

    // Output overflow
    diff = (int32)(out_stat.overflow_cnt - mon_out_stat.overflow_cnt);
    if (diff > 0) {
        VIDEO_LOG("Out O/F %d", diff);
        mon_out_stat.overflow_cnt += diff;
    }

    // TP info overrun
    diff = (int32)(out_stat.info_overrun_cnt - mon_out_stat.info_overrun_cnt);
    if (diff > 0) {
        VIDEO_LOG("Info overrun %d", diff);
        mon_out_stat.info_overrun_cnt += diff;

        if (!info_overrun_alarm_flag) {
            printf("ALARM_VIDEO_IN_BUF_OVERRUN: set\n");
            info_overrun_alarm_flag = TRUE;
        }

    } else if (info_overrun_alarm_flag) {
        printf("ALARM_VIDEO_IN_BUF_OVERRUN: cleared\n");
        info_overrun_alarm_flag = FALSE;
    }

    // TP info error
    diff = (int32)(out_stat.info_err_cnt - mon_out_stat.info_err_cnt);
    if (diff > 0) {
        VIDEO_LOG("Info err %d", diff);
        mon_out_stat.info_err_cnt += diff;
    }

    // Output TP overdue
    diff = (int32)(out_stat.overdue_tp_cnt - mon_out_stat.overdue_tp_cnt);
    if (diff > 0) {
        VIDEO_LOG("Overdue TP %d", diff);
        mon_out_stat.overdue_tp_cnt += diff;

        if (!overdue_alarm_flag) {
            printf("ALARM_VIDEO_OUT_DROP: set\n");
            overdue_alarm_flag = TRUE;
        }

    } else if (overdue_alarm_flag) {
        printf("ALARM_VIDEO_OUT_DROP: cleared\n");
        overdue_alarm_flag = FALSE;
    }
}


void video_monitor_timing (out_session_t *ses)
{
    int i;
    in_session_t* in_ses = ses->in_ses;
    video_in_stat_t* in_ses_stat = &in_ses->stat;
    video_out_stat_t* out_ses_stat = &ses->stat;

    mon_time += mon_period;

    fprintf(mon_fp, "\nOut %d: time %d:%02d, TPs %d, PCRs %d, BR %d, "
                    "stay %d - %d\n",
            ses->id, mon_time/60, mon_time%60,
            out_ses_stat->pcr_tp_cnt + out_ses_stat->non_pcr_tp_cnt
            + out_ses_stat->psi_tp_cnt + out_ses_stat->drop_tp_cnt,
            out_ses_stat->pcr_tp_cnt, in_ses->avg_bitrate,
            (ses->min_stay_time + 23) / SCALE_MS_BASE,
            (ses->max_stay_time + 23) / SCALE_MS_BASE);
    fprintf(mon_fp, "  in: cc err %d, disc %d, jump %d, "\
            "U/F %d, O/F %d, xover %d\n",
            in_ses_stat->cc_err_cnt, in_ses_stat->disc_cnt,
            in_ses_stat->pcr_jump_cnt, in_ses_stat->underflow_cnt,
            in_ses_stat->overflow_cnt, in_ses_stat->pcr_xover_cnt);
    fprintf(mon_fp, "  out: U/F %d, O/F %d, info O/R %d, inv BR %d\n",
            out_ses_stat->underflow_cnt, out_ses_stat->overflow_cnt,
            out_ses_stat->info_overrun_cnt, out_ses_stat->invalid_rate_cnt);

    for (i=0; i<in_ses->pcr_ctx_cnt; i++) {
        pcr_context_t* pcr_ctx = &in_ses->pcr_ctx_tab[i];
        fprintf(mon_fp, "  Pid %d: PCR %08x, TBO %08x, PCR itvl %d, jt %d, "
                        "clk %d ppb\n",
                pcr_ctx->pid, mpeg_time_to_base(pcr_ctx->pcr_time),
                mpeg_time_to_base(pcr_ctx->tbo_est),
                mpeg_time_to_ms(pcr_ctx->max_pcr_inc),
                mpeg_time_to_ms(in_ses->jitter),
                (int32)(pcr_ctx->freq_adj/(SCALE_FREQ_ADJ/1000)));

        pcr_ctx->max_pcr_inc = 0;
    }

    ses->min_stay_time = BIG_INT;
    ses->max_stay_time = -BIG_INT;
}


void video_monitor_timing_set (int out_rid, int period, char* tty)
{
    mon_time = 0;
    ses_mon_wake_cnt = -1;
    mon_period = period;
    mon_out_ses = get_out_session(out_rid);
    mon_fp = fopen(tty, "w");
}


void video_monitor_timing_clear (void)
{
    mon_out_ses = NULL;
    fclose(mon_fp);
    mon_fp = NULL;
}

