/*
 * Copyright (c) 2010-2013, 2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __THREAD_STAT_H__
#define __THREAD_STAT_H__


#include "common.h"
#include "bench.h"


typedef struct {
    int run_cnt;                // how many time this rouine is run
    uint64 start_time;          // start time
    uint64 restart_time;        // restart time
    int64 run_time;             // run time
    int64 max_itvl;
    int64 avg_itvl;
    int64 max_run;
    int64 avg_run;
} thread_stat_t;


/// Restart thread's iteration
/// @note
///   - the thread's iteration should have been paused
///
static inline
void thread_stat_restart (thread_stat_t *my_stat, uint64 restart_time)
{
    my_stat->restart_time = restart_time;
}


/// Pause thread's iteration
/// @note
///   - the thread's iteration should have been started or restarted
///
static inline
void thread_stat_pause (thread_stat_t *my_stat, uint64 pause_time)
{
    my_stat->run_time += get_time_diff_us(my_stat->restart_time, pause_time);
}


/// Start a thread's iteration
int64 thread_stat_start(thread_stat_t *my_stat, uint64 start_time);

/// Stop a thread's iteration
int64 thread_stat_stop(thread_stat_t *my_stat, uint64 stop_time);

void thread_stat_reset(thread_stat_t *my_stat);
void thread_stat_print(FILE *fp, thread_stat_t *my_stat);


#endif
