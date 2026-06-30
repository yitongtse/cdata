/*
 * Copyright (c) 2013 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * VDMAN CPU load Benchmark public data structure
 */

#ifndef __CPULOAD_DATA_H__
#define __CPULOAD_DATA_H__

// Octeon has 32 cores 
#define MAXNUM_CORES      32

typedef struct {
    uint64_t clock_time[MAXNUM_CORES];
    uint64_t idle_timer_time[MAXNUM_CORES];
} time_record_t;

// Octeon core CPU load stat
// obtained from /proc/stat
typedef struct core_stat_s
{
    int coreId;
    int inuse;
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t total_time;
    // core load snapshot
    uint64_t user_load;
    uint64_t nice_load;
    uint64_t system_load;
    uint64_t idle_load;
    uint64_t iowait_load;
    uint64_t irq_load;
    uint64_t softirq_load;
    // value from first time stat retrieval
    uint64_t user_start;
    uint64_t nice_start;
    uint64_t system_start;
    uint64_t idle_start;
    uint64_t iowait_start;
    uint64_t irq_start;
    uint64_t softirq_start;
    uint64_t total_time_start;

} core_stat_t;

// process CPU load stat
// obtained from /proc/<pid>/stat
typedef struct process_stat_s
{
    int processId;
    int threadId;
    int coreId;
    int type;
    int inuse;
    int workId;
    // cpu load usage
    uint64_t utime;
    uint64_t stime;
    uint64_t core_time;
    uint64_t min_load;
    uint64_t max_load;
    uint64_t current_utime_load;
    uint64_t current_stime_load;
    uint64_t current_load;
    uint64_t total_avg_load;
    // value from first time stat retrieval
    uint64_t utime_start;
    uint64_t stime_start;
    uint64_t core_time_start;
    uint64_t core_tick;
    uint64_t core_tick_load;
    uint64_t core_tick_cache;
    uint64_t core_tick_total;
    uint64_t core_tick_pkt;
    uint64_t core_tick_pkt_load;
    uint64_t core_tick_pkt_cache;
    uint64_t core_tick_pkt_total;
} process_stat_t;

typedef struct cpu_single_s
{
    core_stat_t c_usageOld;
    process_stat_t t_usageOld;
    core_stat_t c_usage;
    process_stat_t t_usage;
} cpu_single_t;

// predefined core/process cpu load usage struct for 32 Octeon cores
typedef struct cpu_stat_s
{
    core_stat_t c_usageOld[MAXNUM_CORES];
    process_stat_t t_usageOld[MAXNUM_CORES];
    core_stat_t c_usage[MAXNUM_CORES];
    process_stat_t t_usage[MAXNUM_CORES];
} cpu_stat_t;

#endif /* __CPULOAD_DATA_H__ */
