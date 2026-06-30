/*
 * Copyright (c) 2010-2013 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * VDMAN Thread CPU load Benchmark
 */

#ifndef __CPULOAD_STAT_H__
#define __CPULOAD_STAT_H__

#include "common.h"

int clear_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize);
int init_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize);
int cache_cpu_usage(cpu_stat_t *pCpuUsage, int numEntry);
int get_core_usage(cpu_stat_t *pCpuUsage, int coreId, int cached);
int get_proc_usage(cpu_stat_t *pCpuUsage,
                   int coreId,
                   int processId,
                   int threadId,
                   int cached);
int calc_cpu_time(cpu_stat_t *pCpuUsage, int numEntry);
int get_proc_cpuload_stat(time_record_t *rec,
                          int start,
                          int stop);

#endif /* __CPULOAD_STAT_H__ */
