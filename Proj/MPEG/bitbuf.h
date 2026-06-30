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


static __inline void setBitBuf(BitBuf *bitBuf, uint *curPtr, uint *endPtr,
                             int bitPos)
{
    bitBuf->curPtr = curPtr;
    bitBuf->endPtr = endPtr;
    bitBuf->bitPos = bitPos;
}


static __inline void printBitBuf(BitBuf *bitBuf)
{
    printf("curPtr: %08x  endPtr: %08x  bitPos: %d  buffer: %08x\n",
           bitBuf->curPtr, bitBuf->endPtr, bitBuf->bitPos, bitBuf->buffer);
}


#endif
