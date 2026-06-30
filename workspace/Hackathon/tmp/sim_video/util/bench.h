/*****************************************************************************
    Copyright (c) 2010-2013 by Cisco Systems, Inc.
    All rights reserved.

    File: bench.h

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    10-19-10    ytse    Created.

    This file defines the routines for benchmarking.
*****************************************************************************/

#ifndef __BENCH_H__
#define __BENCH_H__

#ifdef BENCH_POSIX
#include "bench_posix.h"
#endif

#ifdef BENCH_QNX
#include "bench_qnx.h"
#endif

#ifdef BENCH_PPC
#include "bench_ppc.h"
#endif

#ifdef BENCH_CDM
#include "bench_cdm.h"
#endif

#ifdef BENCH_DUMMY
#include "bench_dummy.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "common.h"


static inline
void bench_test (void)
{
    uint64 start, stop;
    start = get_time();
    sleep(1);
    stop = get_time();
    printf("Benchmark ticks for 1 sec: %lld\n",
           get_time_diff_us(start, stop));
}

#endif  // __BENCH_H__

