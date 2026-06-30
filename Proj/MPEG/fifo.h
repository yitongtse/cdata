#ifndef FIFO_H
#define FIFO_H


// FIFO based on array

#define FIFO_NORMAL	0
#define	FIFO_OVERFLOW	-1
#define	FIFO_UNDERFLOW	-2


// Note:
//   - The Fifo structure can be used to maintain a circular FIFO buffer
//     using an array.  It contains only the read and write indices which
//     the user can use to reference the corresponding array.  It does not
//     contain the array itself.
//   - The write index points to the available element to be added to the
//     the array in the next addFifo() call.
//   - The read index points to the occupied element to be deleted from the
//     array in the next deleteFifo() call.
//   - The condition of wrIdx == rdIdx can mean either the Fifo is empty or
//     full.  To avoid this ambiguity, we take the former meaning.
//     As a result, the fifo can at most hold size-1 elements.


typedef struct
{
    int size;
    int wrIdx;		// write index
    int rdIdx;		// read index
    int status;
}
Fifo;


static __inline void fifoInit(Fifo* fifo, int size)
{
    fifo->size = size;
    fifo->rdIdx = fifo->wrIdx = 0;
}


static __inline int fifoFullness(Fifo* fifo)
{
    int fullness = fifo->wrIdx - fifo->rdIdx;
    if (fullness<0)  fullness += fifo->size;
    return fullness;
}


static __inline int fifoIdx(Fifo* fifo, int idx, int delta)
{
    idx += delta;
    if (idx < 0)  return idx + fifo->size;
    if (idx >= fifo->size)  return idx - fifo->size;
    return idx;
}


// Put an element into FIFO
//   User needs to check fifo->status for overflow
static __inline int fifoAdd(Fifo* fifo)
{
    int idx = fifo->wrIdx;
    if (++fifo->wrIdx >= fifo->size)  fifo->wrIdx = 0;
    fifo->status = (fifo->wrIdx == fifo->rdIdx)? FIFO_OVERFLOW : FIFO_NORMAL;
    return idx;
}


// Get an element from FIFO
//   User needs to check fifo->statusfor underflow
static __inline int fifoDelete(Fifo* fifo)
{
    int idx = fifo->rdIdx;
    if (++fifo->rdIdx >= fifo->size)  fifo->rdIdx = 0;
    fifo->status = (idx == fifo->wrIdx)? FIFO_UNDERFLOW : 0;
    return idx;
}


#endif
