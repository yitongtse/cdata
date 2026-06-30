/* 
 *------------------------------------------------------------------
 * trace.h -- Trace Management toolkit
 *
 * Copyright (c) 2007-2015 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

#ifndef __TRACE_H__
#define __TRACE_H__


#ifdef TRACE

#include <assert.h>
#include "common.h"
#include "bit_mask.h"

#define TRACE_LEVEL_MAX 32

extern uint32 _trace_mask[1];

static inline
boolean is_trace_enabled (int level)
{
    assert(level >= 0 && level < TRACE_LEVEL_MAX);
    return bit_mask_test(_trace_mask, level);
}

static inline
void trace_enable (int level)
{
    assert(level >= 0 && level < TRACE_LEVEL_MAX);
    bit_mask_set(_trace_mask, level);
}

static inline
void trace_disable (int level)
{
    assert(level >= 0 && level < TRACE_LEVEL_MAX);
    bit_mask_clear(_trace_mask, level);
}


#else

#define is_trace_enabled(...)           FALSE
#define trace_enable(...)
#define trace_disable(...)

#endif  // TRACE

#endif // __TRACE_H__
