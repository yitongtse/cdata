/*
 * Copyright (c) 2010-2014 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * VDMAN Thread CPU load Benchmark
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "bench.h"
#include "cpuload_data.h"
#include "cpuload_stat.h"

// Following code section will be compiled when the
// comiplation option is set.
// run make with "make PERFMON=y"


#ifdef VDM_PERFMON
/**
 * int clear_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize)
 *
 * Clear cpu_stat_t struct
 * compared in next usage retrieval.
 *
 * @param[in] pCpuUsage : Pointer to cpu_stat_t data struct
 * @param[in] byteSize  : total size of entry in pCpuUsage
 *
 * @return int
 * @retval  0 : success
 * @retval -1 : failure
 */

int clear_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize)
{
    int i, startPos;
    int entrySize, lastBytes;
    uint64_t *tmp64Ptr = NULL;
    uint8_t *tmp8Ptr = NULL;

    if (pCpuUsage == NULL)
    {
        return -1;
    }

    entrySize = byteSize/sizeof(uint64_t);
    lastBytes = byteSize % sizeof(uint64_t);

    tmp64Ptr = (uint64_t *)pCpuUsage;

    // clear uint64 aligned data struct
    for (i = 0; i < entrySize; i++)
    {
        tmp64Ptr[i] = 0;
    }

    // clear rest of bytes remained
    if (lastBytes > 0)
    {
        tmp8Ptr = (uint8_t *)pCpuUsage;
        startPos = byteSize - lastBytes;

        for (i = 0; i < lastBytes; i++)
        {
            tmp8Ptr[startPos + i] = 0;
        }
    }

    return 0;
}

/**
 * int init_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize)
 *
 * Initialize cpu_stat_t struct
 *
 * @param[in] pCpuUsage : Pointer to cpu_stat_t data struct
 * @param[in] byteSize  : total size of entry in pCpuUsage
 *
 * @return int
 * @retval  0 : success
 * @retval -1 : failure
 */

int init_cpu_usage(cpu_stat_t *pCpuUsage, int byteSize)
{
    int i, count, ret;

    // pCpuUsage data struct size should be aligned in unit64.
    if ((pCpuUsage == NULL) || ((byteSize % sizeof(cpu_single_t)) > 0))
    {
        return -1;
    }

    // clear data first
    ret = clear_cpu_usage(pCpuUsage, byteSize);

    // then preset the min cpu load value to 100%
    if (ret == 0)
    {
        count = byteSize/sizeof(cpu_single_t);
        for (i = 0; i < count; i++)
        {
            pCpuUsage->t_usage[i].min_load = 100;
        }

    }

    return ret;
}


/**
 * int get_core_usage(cpu_stat_t *pCpuUsage, int coreId, int cached)
 *
 * Retrieve the core cpu uage from /proc/stat
 *
 * @param[in] pCpuUsage : Pointer to cpu_stat_t data struct
 * @param[in] coreId    : coreId you are interested in
 * @param[in] cached    : 0 - no data cached, 1 - data cached
 *
 * @return int
 * @retval 0 on success
 * @retval -1 on failure
 */

int get_core_usage(cpu_stat_t *pCpuUsage, int coreId, int cached)
{

    FILE *inFd = NULL;
    uint64_t user_val = 0, nice_val = 0, system_val = 0;
    uint64_t idle_val, iowait_val, irq_val;
    uint64_t softirq_val, steal_val, quest_val;
    uint64_t dummy10 = 0;
    char myBuf[256], cpuStr[64];
    int coreNum;

    if (pCpuUsage == NULL)
    {
        return -1;
    }

    // open /proc/stat
    inFd = fopen("/proc/stat", "r");

    if (inFd == NULL)
    {
        return -1;
    }

    myBuf[0] = '\0';
    cpuStr[0] = '\0';

    /*
     * /proc/pid/stat
     * cpu entry, USER_HZ (usually 1/100 sec, jiffy)
     * user, nice, system, idle, iowait, irq, softirq, steal, quest, ??
     */

    while (fgets(myBuf, 255, inFd) != NULL)
    {
        myBuf[255] = '\0';
        // interested in cpu entry only
        if (strstr(myBuf, "cpu") != NULL)
        {
            // have 10 fields : 2.6 kernel
            if (sscanf(myBuf, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                       cpuStr, &user_val, &nice_val, &system_val,
                       &idle_val, &iowait_val, &irq_val, &softirq_val,
                       &steal_val, &quest_val, &dummy10) > 7)
            {
                // save the cpu load to matched core stat entry
                if (strlen(cpuStr) > 3)
                {
                    // multi core
                    sscanf(cpuStr, "cpu%d", &coreNum);
                    pCpuUsage->c_usage[coreNum].user = user_val;
                    pCpuUsage->c_usage[coreNum].nice = nice_val;
                    pCpuUsage->c_usage[coreNum].system = system_val;
                    pCpuUsage->c_usage[coreNum].idle = idle_val;
                    pCpuUsage->c_usage[coreNum].iowait = iowait_val;
                    pCpuUsage->c_usage[coreNum].irq = irq_val;
                    pCpuUsage->c_usage[coreNum].softirq = softirq_val;
                    pCpuUsage->c_usage[coreNum].inuse = 1;
                    pCpuUsage->c_usage[coreNum].total_time =
                                (user_val + nice_val + system_val +
                                         idle_val + irq_val + softirq_val);

                   // save first time value retrieved.
                   if (cached == 0)
                   {
                        pCpuUsage->c_usage[coreNum].user_start = user_val;
                        pCpuUsage->c_usage[coreNum].nice_start = nice_val;
                        pCpuUsage->c_usage[coreNum].system_start = system_val;
                        pCpuUsage->c_usage[coreNum].idle_start = idle_val;
                        pCpuUsage->c_usage[coreNum].iowait_start = iowait_val;
                        pCpuUsage->c_usage[coreNum].irq_start = irq_val;
                        pCpuUsage->c_usage[coreNum].softirq_start = softirq_val;                        pCpuUsage->c_usage[coreNum].total_time_start =
                                   pCpuUsage->c_usage[coreNum].total_time;
                        pCpuUsage->t_usage[coreNum].core_time_start =
                                pCpuUsage->c_usage[coreId].total_time_start;
                   }
                }
            }
        }

        myBuf[0] = '\0';
        cpuStr[0] = '\0';
    }

    fclose(inFd);

    return 0;
}

/**
 * int get_proc_usage(cpu_stat_t *pCpuUsage,
 *                    int coreId,
 *                    int processId,
 *                    int threadId,
 *                    int cached)
 *
 * Retrieve the process cpu uage from /proc/<pid>/stat
 *
 * @param[in] pCpuUsage : Pointer to cpu_stat_t data struct
 * @param[in] coreId    : coreId used for this process
 * @param[in] procId    : process/thread Id
 * @param[in] cached    : 0 - no data cached, 1 - data cached
 *
 * @return void
 * @retval 0 on success
 * @retval -1 on failure
 */

int get_proc_usage(cpu_stat_t *pCpuUsage,
                   int coreId,
                   int processId,
                   int threadId,
                   int cached)
{
    FILE *inFd = NULL;
    uint64_t dummy1,dummy4, dummy5;
    char dummy2[32], dummy3[32];
    uint64_t dummy6, dummy7, dummy9, dummy10;
    int64_t dummy8;
    uint64_t dummy11, dummy12, dummy13;
    uint64_t utime_val, stime_val;
    char myBuf[256], cpuStr[64];
    char procFile[128];

    if (pCpuUsage == NULL)
    {
        return -1;
    }

    // open /proc/<pid>/task/<tid>/stat
    sprintf(procFile, "/proc/%d/task/%d/stat", processId, threadId);
    //printf("coreId %d, procFile: %s\n", coreId, procFile);
    inFd = fopen(procFile, "r");

    if (inFd == NULL)
    {
        printf("cannot open %s\n", procFile);
        return -1;
    }

    myBuf[0] = '\0';
    cpuStr[0] = '\0';

    if (fgets(myBuf, 255, inFd) != NULL)
    {
        myBuf[255] = '\0';
        // interested in 14, 15th entries. utime & stime
        if (sscanf(myBuf,
                   "%lu %s %s %lu %lu %lu %lu %ld %lu %lu %lu %lu %lu %lu %lu",
                       &dummy1, dummy2, dummy3, &dummy4, &dummy5, &dummy6,
                       &dummy7, &dummy8, &dummy9, &dummy10, &dummy11, &dummy12,
                       &dummy13, &utime_val, &stime_val) == 15)
       {
            /*
            printf("coreId %d, pid: %d, tid %d, utime: %lu, stime: %lu\n",
                        coreId, processId, threadId, utime_val, stime_val);
            */
            pCpuUsage->t_usage[coreId].utime = utime_val;
            pCpuUsage->t_usage[coreId].stime = stime_val;
            pCpuUsage->t_usage[coreId].inuse = 1;

            if (cached == 0)
            {
                // save first time value retrieved.
                pCpuUsage->t_usage[coreId].utime_start = utime_val;
                pCpuUsage->t_usage[coreId].stime_start = stime_val;
                pCpuUsage->t_usage[coreId].core_time_start =
                                pCpuUsage->c_usage[coreId].total_time_start;
            }
        }
        pCpuUsage->t_usage[coreId].core_tick_cache =
                             pCpuUsage->t_usage[coreId].core_tick;
        pCpuUsage->t_usage[coreId].core_tick = 0;
        pCpuUsage->t_usage[coreId].core_tick_pkt_cache =
                             pCpuUsage->t_usage[coreId].core_tick_pkt;
        pCpuUsage->t_usage[coreId].core_tick_pkt = 0;
    }
    else
    {
        fclose(inFd);
        return -1;
    }

    fclose(inFd);

    return 0;
}

/**
 * int vdm_calc_cpu_time(cpu_stat_t *pCpuUsage)
 *
 * Calculate core/process cpu load usage min/avg/max
 *
 * @param[in] pCpuUsage : Pointer to cpu_stat_t data struct
 * @param[in] numEntry  : Number of process entry in pCpuUsage
 *
 * @return void
 * @retval 0 on success
 * @retval -1 on failure
 */

int calc_cpu_time(cpu_stat_t *pCpuUsage, int numEntry)
{
    int i, count;
    uint64_t utime_diff = 0, stime_diff = 0, core_time_diff = 0;
    uint64_t core_time_total;
    uint64_t utime_total;
    uint64_t stime_total;
    uint64_t load_diff;

    if (pCpuUsage == NULL)
    {
        return -1;
    }

    count = numEntry;
    // limit max num process to MAXNUM_CORES (32 cores)
    if (numEntry > MAXNUM_CORES)
    {
        count = MAXNUM_CORES;
    }

    // calculate core/thread cpu load when core/thread is in use
    // % based cpu load is calculated
    for (i = 0; i < count; i++)
    {
        if ((pCpuUsage->c_usage[i].inuse == 1) &&
            (pCpuUsage->t_usage[i].inuse == 1) &&
            (pCpuUsage->c_usageOld[i].inuse == 1) &&
            (pCpuUsage->t_usageOld[i].inuse == 1))
        {
            /*
            printf("core %d old utime %lu, stime %lu, total_time %lu\n",
                     i,
                     pCpuUsage->t_usageOld[i].utime,
                     pCpuUsage->t_usageOld[i].stime,
                     pCpuUsage->c_usageOld[i].total_time);
            */
            core_time_diff = pCpuUsage->c_usage[i].total_time -
                                     pCpuUsage->c_usageOld[i].total_time;
            pCpuUsage->t_usage[i].core_time =
                                     pCpuUsage->c_usage[i].total_time;

            /*
            printf("udiff %lu, sdiff %lu, ctimediff %lu\n",
                  utime_diff, stime_diff, core_time_diff);
            */
            if (core_time_diff > 0)
            {
                // user time and system time 
                utime_diff = pCpuUsage->t_usage[i].utime -
                                         pCpuUsage->t_usageOld[i].utime;

                stime_diff = pCpuUsage->t_usage[i].stime -
                                         pCpuUsage->t_usageOld[i].stime;

                // get min, max, current, total_avg cpu load

                pCpuUsage->t_usage[i].current_load =
                          ((utime_diff + stime_diff)*100)/core_time_diff;
                pCpuUsage->t_usage[i].current_utime_load =
                          (utime_diff*100)/core_time_diff;
                pCpuUsage->t_usage[i].current_stime_load =
                          (stime_diff*100)/core_time_diff;
                // min cpu load

                if (pCpuUsage->t_usage[i].min_load >
                                    pCpuUsage->t_usage[i].current_load)
                {
                    pCpuUsage->t_usage[i].min_load =
                                    pCpuUsage->t_usage[i].current_load;
                }
                // max cpu load

                if (pCpuUsage->t_usage[i].max_load <
                                    pCpuUsage->t_usage[i].current_load)
                {
                    pCpuUsage->t_usage[i].max_load =
                                    pCpuUsage->t_usage[i].current_load;
                }

                // core clock run calculation
                // jiffy = 100 per 1 sec
                // octeon core tick 1200000000 per 1 sec
                // clock_run = ((core_tick*100) / (1200000000/(jiffy*100))

                /*
                printf("core_tick_cache %lu, core_time_diff %lu\n",
                         pCpuUsage->t_usage[i].core_tick_cache,
                         core_time_diff);
                */
                pCpuUsage->t_usage[i].core_tick_load =
                       pCpuUsage->t_usage[i].core_tick_cache /
                                            (core_time_diff * 120000);
                pCpuUsage->t_usage[i].core_tick_pkt_load =
                       pCpuUsage->t_usage[i].core_tick_pkt_cache /
                                            (core_time_diff * 120000);
                // core cpu load usage calculation
                load_diff = pCpuUsage->c_usage[i].user -
                                 pCpuUsage->c_usageOld[i].user;
                pCpuUsage->c_usage[i].user_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].nice -
                                 pCpuUsage->c_usageOld[i].nice;
                pCpuUsage->c_usage[i].nice_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].system -
                                 pCpuUsage->c_usageOld[i].system;
                pCpuUsage->c_usage[i].system_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].idle -
                                 pCpuUsage->c_usageOld[i].idle;
                pCpuUsage->c_usage[i].idle_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].iowait -
                                 pCpuUsage->c_usageOld[i].iowait;
                pCpuUsage->c_usage[i].iowait_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].irq -
                                 pCpuUsage->c_usageOld[i].irq;
                pCpuUsage->c_usage[i].irq_load =
                                 (load_diff * 100)/core_time_diff;
                load_diff = pCpuUsage->c_usage[i].softirq -
                                 pCpuUsage->c_usageOld[i].softirq;
                pCpuUsage->c_usage[i].softirq_load =
                                 (load_diff * 100)/core_time_diff;
            }

            // total_avg load
            core_time_total = pCpuUsage->t_usage[i].core_time -
                                 pCpuUsage->t_usage[i].core_time_start;
            utime_total = pCpuUsage->t_usage[i].utime -
                                 pCpuUsage->t_usage[i].utime_start;
            stime_total = pCpuUsage->t_usage[i].stime -
                                 pCpuUsage->t_usage[i].stime_start;
            if (core_time_total > 0)
            {
                pCpuUsage->t_usage[i].total_avg_load =
                     (((utime_total + stime_total)*100)/core_time_total);
            }


            
            
        }
    }

    return 0;
}

#endif /* VDM_PERFMON */

/**
 * int get_proc_cpuload_stat(time_record_t *rec,
 *                           int start,
 *                           int stop)
 *
 * retrieve cpu load from /proc/stat for core ID from 'start' to 'stop'
 *
 * @param[in/out] ret : pointer to the time_record_t data struct
 * @param[in] start : start coreID
 * @param[in] stop : stop coreID
 *
 * @return int
 * @retval  0  : success
 * @retval  -1 : failure
 */

int get_proc_cpuload_stat(time_record_t *rec,
                          int start,
                          int stop)
{
    FILE *inFd = NULL;
    uint64_t user_val = 0, nice_val = 0, system_val = 0;
    uint64_t idle_val, iowait_val, irq_val;
    uint64_t softirq_val, steal_val, quest_val;
    uint64_t dummy10 = 0;
    uint64_t clock_time;
    char myBuf[256], cpuStr[64];
    int coreNum;

    if ((rec == NULL) ||
        (start < 0) ||
        (stop >= MAXNUM_CORES))
    {
        return -1;
    }

    // get CPU time
    // open /proc/stat
    inFd = fopen("/proc/stat", "r");

    if (inFd == NULL)
    {
        return -1;
    }

    while (fgets(myBuf, 255, inFd) != NULL)
    {
        myBuf[255] = '\0';
        // interested in cpu entry only
        // we have 32 cores
        if (strstr(myBuf, "cpu") != NULL)
        {
            // have 10 fields : 2.6 kernel
            if (sscanf(myBuf, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                           cpuStr, &user_val, &nice_val, &system_val,
                           &idle_val, &iowait_val, &irq_val, &softirq_val,
                           &steal_val, &quest_val, &dummy10) > 7)
            {
                if (strlen(cpuStr) > 3)
                {
                    clock_time = (user_val + nice_val + system_val +
                                         idle_val + irq_val + softirq_val);

                    sscanf(cpuStr, "cpu%d", &coreNum);
                    if ((coreNum >= start) && (coreNum <= stop))
                    {
                        rec->idle_timer_time[coreNum] = idle_val;
                        rec->clock_time[coreNum] = clock_time;
                        /*
                        printf("COREID: %d, idel_time: %lu, clock_time %lu\n",
                                    coreNum, rec->idle_timer_time[coreNum],
                                    rec->clock_time[coreNum]);
                        */
                    }
                }
            }
            myBuf[0] = '\0';
            cpuStr[0] = '\0';
        }
    }

    fclose(inFd);
    inFd = NULL;

    return 0;
}

