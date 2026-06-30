/*****************************************************************************
    Copyright (c) 2010-2013 by Cisco Systems, Inc.
    All rights reserved.

    File: bench_cdm.h

    Date        Who     Modification
    --------    ------  ------------------------------------------------------
    02-05-13    ytse    Created.

    This file defines the routines for benchmarking for CDMAN INFRA.
*****************************************************************************/

#ifndef __BENCH_CDM_H__
#define __BENCH_CDM_H__

#include "common.h"

// Note: Modify this to match with the actual system core clock frequency
#define SYS_CLK_PER_US          1000

extern uint64_t pclc_cvmx_get_clock_count(int type);


static int64 cycle_to_us (int64 cycle)
{
    return cycle / SYS_CLK_PER_US;
}


// Get time in us resolution
static uint64 get_time (void)
{
    return pclc_cvmx_get_clock_count(2);                // CDM_CORE_CLK = 2
}


// Get time difference (up to 1000 sec)
static int64 get_time_diff_us (uint64 start, uint64 stop)
{
    return cycle_to_us((int64)(stop - start));
}

#endif

