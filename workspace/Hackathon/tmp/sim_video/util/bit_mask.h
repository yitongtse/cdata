///  Copyright (c) 2011-2012 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      bit_mask.h
///  @brief     Bit Mask utilities
///  @author    Yi Tong Tse

#ifndef __BIT_MASK_H__
#define __BIT_MASK_H__

#include <string.h>


/// Initialize a bit mask
///   Note: Caller is to allocate memory for the mask before the call
static inline 
void bit_mask_init (uint32 *mask, int size)
{
    memset(mask, 0, (size+31) >> 3);
}


/// Set bit mask at specified position
static inline 
void bit_mask_set (uint32 *mask, int pos)
{
    mask[pos >> 5] |= 1UL << (pos & 0x1F);
}


/// Clear bit mask at specified position
static inline 
void bit_mask_clear (uint32 *mask, int pos)
{
    mask[pos >> 5] &= ~(1UL << (pos & 0x1F));
}


/// Return bit value of the bit mask at specified position
static inline 
int bit_mask_test (uint32 *mask, int pos)
{
    return (mask[pos >> 5] >> (pos & 0x1F)) & 1;
}

static inline
boolean bit_mask_is_empty (uint32 *mask, int size)
{
    int i = (size+31) >> 5;

    for (; i > 0; i--) {
        if (*mask++) {
            return FALSE;
        }
    }

    return TRUE;
}
    
#endif

