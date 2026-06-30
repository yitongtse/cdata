/*****************************************************************************
    Copyright (c) 2010-2012 by Cisco Systems, Inc.
    All rights reserved.

    File: bench_posix.h

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    10-19-10    ytse    Created.

    This file defines the routines for benchmarking for POSIX.
*****************************************************************************/

#ifndef __BENCH_POSIX_H__
#define __BENCH_POSIX_H__


#include <time.h>
#include "common.h"


// Get time in us resolution
static uint64 get_time (void)
{
    struct timespec local_time;
    clock_gettime(CLOCK_REALTIME, &local_time);
    return ((uint64)local_time.tv_sec) * MILLION + local_time.tv_nsec / THOUSAND;
}


// Get time difference (up to 1000 sec)
static int64 get_time_diff_us (uint64 start, uint64 stop)
{
    return (int64)(stop - start);
}


#endif

