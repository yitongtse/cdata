/*
 * Copyright (c) 2010-2013, 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include "bench.h"
#include "thread_stat.h"


#define SCALE_BITS_AVG          4


int64 thread_stat_start (thread_stat_t *my_stat, uint64 start_time)
{
    int64 itvl_time = 0;

    if (my_stat->run_cnt++) {
        itvl_time = get_time_diff_us(my_stat->start_time, start_time);
        my_stat->avg_itvl += (itvl_time - my_stat->avg_itvl) >> SCALE_BITS_AVG;
        if (itvl_time > my_stat->max_itvl) {
            my_stat->max_itvl = itvl_time;
        }
    } else {
        // Initialization for the first time
        my_stat->avg_itvl = my_stat->avg_run = 0;
        my_stat->max_itvl = my_stat->max_run = 0;
    }

    my_stat->start_time = my_stat->restart_time = start_time;
    my_stat->run_time = 0;
    return itvl_time;
}


int64 thread_stat_stop (thread_stat_t *my_stat, uint64 stop_time)
{
    my_stat->run_time += get_time_diff_us(my_stat->restart_time, stop_time);
    my_stat->avg_run += (my_stat->run_time - my_stat->avg_run)
                            >> SCALE_BITS_AVG;
    if (my_stat->run_time > my_stat->max_run) {
        my_stat->max_run = my_stat->run_time;
    }
    return my_stat->run_time;
}


void thread_stat_reset (thread_stat_t *my_stat)
{
    my_stat->run_cnt = 0;
}


void thread_stat_print (FILE *fp, thread_stat_t *my_stat)
{
    fprintf(fp, "Count %d, Run time: %lld / %lld, Interval: %lld / %lld\n",
            my_stat->run_cnt, my_stat->avg_run, my_stat->max_run,
            my_stat->avg_itvl, my_stat->max_itvl);
}

