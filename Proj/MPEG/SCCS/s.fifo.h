h53894
s 00046/00017/00036
d D 1.3 03/03/24 11:22:46 ytse 3 2
c Backup
e
s 00001/00001/00052
d D 1.2 02/05/23 15:11:11 ytse 2 1
c Fix overflow detection bug
e
s 00053/00000/00000
d D 1.1 02/02/25 17:54:24 ytse 1 0
c date and time created 02/02/25 17:54:24 by ytse
e
u
U
f e 0
t
T
I 1
#ifndef FIFO_H
#define FIFO_H


// FIFO based on array

I 3
#define FIFO_NORMAL	0
E 3
#define	FIFO_OVERFLOW	-1
#define	FIFO_UNDERFLOW	-2


I 3
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


E 3
typedef struct
{
    int size;
D 3
    int rdIdx;		// index of current element
    int wrIdx;		// index of next available space
E 3
I 3
    int wrIdx;		// write index
    int rdIdx;		// read index
    int status;
E 3
}
Fifo;


D 3
static __inline void initFifo(Fifo* fifo, int size)
E 3
I 3
static __inline void fifoInit(Fifo* fifo, int size)
E 3
{
    fifo->size = size;
    fifo->rdIdx = fifo->wrIdx = 0;
}


D 3
// Put an element into FIFO
static __inline int putFifo(Fifo* fifo, int* idx)
E 3
I 3
static __inline int fifoFullness(Fifo* fifo)
E 3
{
D 3
    *idx = fifo->wrIdx;
    if (++fifo->wrIdx >= fifo->size)  fifo->wrIdx = 0;
D 2
    return (*idx == fifo->rdIdx)? FIFO_OVERFLOW : 0;
E 2
I 2
    return (fifo->wrIdx == fifo->rdIdx)? FIFO_OVERFLOW : 0;
E 3
I 3
    int fullness = fifo->wrIdx - fifo->rdIdx;
    if (fullness<0)  fullness += fifo->size;
    return fullness;
E 3
E 2
}


D 3
// Get an element from FIFO
static __inline int getFifo(Fifo* fifo, int* idx)
E 3
I 3
static __inline int fifoIdx(Fifo* fifo, int idx, int delta)
E 3
{
D 3
    if (++fifo->rdIdx >= fifo->size)  fifo->rdIdx = 0;
    *idx = fifo->rdIdx;
    return (*idx == fifo->wrIdx)? FIFO_UNDERFLOW : 0;
E 3
I 3
    idx += delta;
    if (idx < 0)  return idx + fifo->size;
    if (idx >= fifo->size)  return idx - fifo->size;
    return idx;
E 3
}


D 3
static __inline int fifoFullness(Fifo* fifo)
E 3
I 3
// Put an element into FIFO
//   User needs to check fifo->status for overflow
static __inline int fifoAdd(Fifo* fifo)
E 3
{
D 3
    int fullness = fifo->wrIdx - fifo->rdIdx;
    if (fullness<0)  fullness += fifo->size;
    return fullness;
E 3
I 3
    int idx = fifo->wrIdx;
    if (++fifo->wrIdx >= fifo->size)  fifo->wrIdx = 0;
    fifo->status = (fifo->wrIdx == fifo->rdIdx)? FIFO_OVERFLOW : FIFO_NORMAL;
    return idx;
E 3
}

I 3

// Get an element from FIFO
//   User needs to check fifo->statusfor underflow
static __inline int fifoDelete(Fifo* fifo)
{
    int idx = fifo->rdIdx;
    if (++fifo->rdIdx >= fifo->size)  fifo->rdIdx = 0;
    fifo->status = (idx == fifo->wrIdx)? FIFO_UNDERFLOW : 0;
    return idx;
}

E 3

#endif
E 1
