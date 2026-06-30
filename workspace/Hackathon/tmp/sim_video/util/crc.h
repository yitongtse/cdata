/*
 *  Copyright (c) 2013 by Cisco Systems, Inc.
 *  All rights reserved.
 */

#ifndef __CRC_H__
#define __CRC_H__

#include "common.h"

// CRC-16/32 generation or checking
// - For CRC generation, len should not include the CRC field.
//   The returned value is the CRC-16/32 result.
// - For CRC checking, len should include the CRC field at end of buffer.
//   Return 0 if CRC OK, otherwise CRC is bad.

// The _part version all CRC computation to be chained.

uint16 crc16 (uint8 *buf, uint32 len);
uint16 crc16_part(uint8 *buf, int32 len, uint16 crc);
uint32 crc32 (uint8 *buf, int32 len);
uint32 crc32_part(uint8 *buf, int32 len, uint32 crc);

#endif /* __CRC_H__ */
