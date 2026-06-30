/*
 * Copyright (c) 2005-2015 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common.h"
#include "phy_mem.h"


void* phy_mem_alloc (uintptr_t size, uint32 *offset)
{
    void* buf = calloc(1, size);
    if (offset) {
        *offset = 0;
    }
    return buf;
}


void phy_mem_free (void *addr)
{
    free(addr);
}
