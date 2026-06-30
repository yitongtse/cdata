/*
 * Copyright (c) 2005-2014 by Cisco Systems, Inc.
 * All rights reserved.
 */

#ifndef __PHY_MEM_H__
#define __PHY_MEM_H__


// Allocate continguous physical memory region
void* phy_mem_alloc(uintptr_t len, uint32 *offset);

// Free physical memory region
void phy_mem_free(void *addr);

#endif /* __PHY_MEM_H__ */
