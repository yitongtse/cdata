/**
*    Copyright (c) 2005-2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file common.h
*    @brief Common definitions used in RFGW-10
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <inttypes.h>


typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;
                        
typedef enum {
    FALSE = 0,
    TRUE = 1
} boolean;


/* define the bit pattern */
#define BIT0        0x00000001
#define BIT1        0x00000002
#define BIT2        0x00000004
#define BIT3        0x00000008
#define BIT4        0x00000010
#define BIT5        0x00000020
#define BIT6        0x00000040
#define BIT7        0x00000080
#define BIT8        0x00000100
#define BIT9        0x00000200
#define BIT10       0x00000400
#define BIT11       0x00000800
#define BIT12       0x00001000
#define BIT13       0x00002000
#define BIT14       0x00004000
#define BIT15       0x00008000
#define BIT16       0x00010000
#define BIT17       0x00020000
#define BIT18       0x00040000
#define BIT19       0x00080000
#define BIT20       0x00100000
#define BIT21       0x00200000
#define BIT22       0x00400000
#define BIT23       0x00800000
#define BIT24       0x01000000
#define BIT25       0x02000000
#define BIT26       0x04000000
#define BIT27       0x08000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

#define THOUSAND    1000
#define MILLION     1000000
#define BILLION     1000000000L

#define WORD(addr)              (*((uint32*)(addr)))


#ifndef __QNX__
/* Make QNX defined error code available for other OSes */
#define EOK         0
#endif


#endif /* __COMMON_H__ */
