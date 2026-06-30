#ifndef UTIL_H
#define UTIL_H


/* Common types
*/
typedef unsigned char uchar;
typedef unsigned int uint;


/* 2-D coordinate class
*/
typedef struct Coor {
    int x;
    int y;
} Coor;



/* Inline functions
*/

/* Set coordinate */
static __inline void setCoor(Coor *dst, int x, int y)
{
    dst->x = x;
    dst->y = y;
}

/* Copy coordinate */
static __inline void copyCoor(Coor *dst, Coor *src)
{
    dst->x = src->x;
    dst->y = src->y;
}

/* Add coordinate */
static __inline void addCoor(Coor *dst, Coor *src1, Coor *src2)
{
    dst->x = src1->x + src2->x;
    dst->y = src1->y + src2->y;
}

/* Subtract coordinate */
static __inline void subCoor(Coor *dst, Coor *src1, Coor *src2)
{
    dst->x = src1->x - src2->x;
    dst->y = src1->y - src2->y;
}

/* Extract bit field */
static __inline uint getBitField(uint x, int leftPos, int nbits)
{
    return (x << (31-leftPos)) >> (32-nbits);
}

/* Set bit field */
static __inline void setBitField(uint* x, int leftPos, int nbits, uint value)
{
    int shfBits = leftPos - nbits + 1;
    uint mask = ((1<<nbits) - 1) << shfBits;
    *x = (*x & ~mask) | (value<<shfBits);
}

/* Print in binary */
static __inline void printBinary(uint x, int nbits)
{
    int i;
    for (i=nbits-1; i>=0; i--)
        printf("%d", (x>>i)&1);
}


/* Clip value */
static __inline void clip(short *x, short min, short max)
{
    if (*x<min)  *x = min;
    else if (*x>max)  *x = max;
}

/* Chech a value with range.  Returns 1 if off range.  Otherwise returns 0 */
static __inline int checkRange(int x, int min, int max)
{
    if (x<min)  return 1;
    if (x>max)  return 1;
    return 0;
}

/* Note: Assumes *x is not too far away from min or max */
static __inline void wrap(int *x, int min, int max)
{
    if (*x<min)  *x += max-min+1;
    else if (*x>max)  *x -= max-min+1;
}


#ifdef UNIX
// min() and max() already defined in VC++

static __inline int min(int a, int b)
{
    return a < b? a : b;
}


static __inline int max(int a, int b)
{
    return a > b? a : b;
}

#endif


#endif
