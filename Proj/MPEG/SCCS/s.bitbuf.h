h58549
s 00002/00002/00043
d D 2.2 00/08/21 11:23:54 ytse 3 2
c Added support of Windows
e
s 00000/00000/00045
d D 2.1 00/08/21 11:04:18 ytse 2 1
c Before supporting Windows
e
s 00045/00000/00000
d D 1.1 99/10/29 15:58:12 yitong 1 0
c date and time created 99/10/29 15:58:12 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef BITBUF_H
#define BITBUF_H

#include "util.h"


/*
    Conventions for BitBuf usage:
      The current word being processed is stored in bitBuf.buffer.
      curPtr points to the word being processed.
      endPtr points to the next word right after the buffer.
      bitPos = 32 means the MSB is the next available bit.
      bitPos = 0 means all the bits in buffer is consumed.
*/


typedef struct
{
                                // Note: assumes bit buffer is word aligned!
    uint* curPtr;		// points to current word being processed
    uint* endPtr;		// points right after end of buffer
    uint  buffer;		// 32-bit buffer
    int   bitPos;		// current bit position: 32-MSB, 1-LSB
                                // 0 means buffer has been used up
}
BitBuf;


D 3
static inline void setBitBuf(BitBuf *bitBuf, uint *curPtr, uint *endPtr,
E 3
I 3
static __inline void setBitBuf(BitBuf *bitBuf, uint *curPtr, uint *endPtr,
E 3
                             int bitPos)
{
    bitBuf->curPtr = curPtr;
    bitBuf->endPtr = endPtr;
    bitBuf->bitPos = bitPos;
}


D 3
static inline void printBitBuf(BitBuf *bitBuf)
E 3
I 3
static __inline void printBitBuf(BitBuf *bitBuf)
E 3
{
    printf("curPtr: %08x  endPtr: %08x  bitPos: %d  buffer: %08x\n",
           bitBuf->curPtr, bitBuf->endPtr, bitBuf->bitPos, bitBuf->buffer);
}


#endif
E 1
