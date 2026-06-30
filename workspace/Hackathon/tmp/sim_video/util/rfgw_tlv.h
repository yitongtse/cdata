/*
 *  Copyright (c) 2007, 2009-2012 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __RFGW_TLV_H__
#define __RFGW_TLV_H__

#include <string.h>

#define RFGW_TLV_HDR_SIZE       4


typedef struct rfgw_tlv_ {
    uint16 type;
    uint16 length;
    uint32 value[0];
} rfgw_tlv_t;


// Move to next TLV
// Returns the total TLV size (including header)
//
static inline
size_t tlv_next (rfgw_tlv_t** tlv)
{
    size_t len = sizeof(rfgw_tlv_t) + (*tlv)->length;
    *tlv = (rfgw_tlv_t*)(((size_t)*tlv) + len);
    return len;
}


// Add a TLV with a specified uint32 as value
//
static inline
void tlv_add_const (rfgw_tlv_t** tlv, int *tot_len,
                    uint16 type, uint16 len, uint32 val)
{
    (*tlv)->type = type;
    (*tlv)->length = len;
    (*tlv)->value[0] = val;
    *tot_len += tlv_next(tlv);
}


// Add a TLV with specified data structure as value (by copying)
//
static inline
void tlv_add_var (rfgw_tlv_t** tlv, int *tot_len,
                  uint16 type, uint16 len, uint32 *val)
{
    (*tlv)->type = type;
    (*tlv)->length = len;
    memcpy((*tlv)->value, val, len);
    *tot_len += tlv_next(tlv);
}


// Check TLV
//   Returns 1 if there is any length problem.  Otherwise returns 0.
//
static inline
int tlv_check (rfgw_tlv_t *tlv, int tot_tlv_len, int min_len)
{
    return ((tlv->length < min_len)
            || (tot_tlv_len < RFGW_TLV_HDR_SIZE + tlv->length));
}


#endif /* __RFGW_TLV_H__ */
