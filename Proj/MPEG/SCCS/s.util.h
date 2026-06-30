h53278
s 00018/00000/00096
d D 2.3 03/03/21 14:16:45 ytse 5 4
c backup
e
s 00010/00010/00086
d D 2.2 00/08/21 11:23:54 ytse 4 3
c Added support of Windows
e
s 00000/00000/00096
d D 2.1 00/08/21 11:04:27 ytse 3 2
c Before supporting Windows
e
s 00009/00001/00087
d D 1.2 99/11/05 15:39:16 yitong 2 1
c update
e
s 00088/00000/00000
d D 1.1 99/10/29 15:58:15 yitong 1 0
c date and time created 99/10/29 15:58:15 by yitong
e
u
U
f e 0
t
T
I 1
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
D 4
static inline void setCoor(Coor *dst, int x, int y)
E 4
I 4
static __inline void setCoor(Coor *dst, int x, int y)
E 4
{
    dst->x = x;
    dst->y = y;
}

/* Copy coordinate */
D 4
static inline void copyCoor(Coor *dst, Coor *src)
E 4
I 4
static __inline void copyCoor(Coor *dst, Coor *src)
E 4
{
    dst->x = src->x;
    dst->y = src->y;
}

/* Add coordinate */
D 4
static inline void addCoor(Coor *dst, Coor *src1, Coor *src2)
E 4
I 4
static __inline void addCoor(Coor *dst, Coor *src1, Coor *src2)
E 4
{
    dst->x = src1->x + src2->x;
    dst->y = src1->y + src2->y;
}

/* Subtract coordinate */
D 4
static inline void subCoor(Coor *dst, Coor *src1, Coor *src2)
E 4
I 4
static __inline void subCoor(Coor *dst, Coor *src1, Coor *src2)
E 4
{
    dst->x = src1->x - src2->x;
    dst->y = src1->y - src2->y;
}

/* Extract bit field */
D 2
static inline uint bitField(uint x, int leftPos, int nbits)
E 2
I 2
D 4
static inline uint getBitField(uint x, int leftPos, int nbits)
E 4
I 4
static __inline uint getBitField(uint x, int leftPos, int nbits)
E 4
E 2
{
    return (x << (31-leftPos)) >> (32-nbits);
I 2
}

/* Set bit field */
D 4
static inline void setBitField(uint* x, int leftPos, int nbits, uint value)
E 4
I 4
static __inline void setBitField(uint* x, int leftPos, int nbits, uint value)
E 4
{
    int shfBits = leftPos - nbits + 1;
    uint mask = ((1<<nbits) - 1) << shfBits;
    *x = (*x & ~mask) | (value<<shfBits);
E 2
}

/* Print in binary */
D 4
static inline void printBinary(uint x, int nbits)
E 4
I 4
static __inline void printBinary(uint x, int nbits)
E 4
{
    int i;
    for (i=nbits-1; i>=0; i--)
        printf("%d", (x>>i)&1);
}


/* Clip value */
D 4
static inline void clip(short *x, short min, short max)
E 4
I 4
static __inline void clip(short *x, short min, short max)
E 4
{
    if (*x<min)  *x = min;
    else if (*x>max)  *x = max;
}

/* Chech a value with range.  Returns 1 if off range.  Otherwise returns 0 */
D 4
static inline int checkRange(int x, int min, int max)
E 4
I 4
static __inline int checkRange(int x, int min, int max)
E 4
{
    if (x<min)  return 1;
    if (x>max)  return 1;
    return 0;
}

/* Note: Assumes *x is not too far away from min or max */
D 4
static inline void wrap(int *x, int min, int max)
E 4
I 4
static __inline void wrap(int *x, int min, int max)
E 4
{
    if (*x<min)  *x += max-min+1;
    else if (*x>max)  *x -= max-min+1;
}

I 5

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


E 5
#endif
E 1
