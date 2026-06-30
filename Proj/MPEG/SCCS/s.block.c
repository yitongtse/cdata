h07930
s 00000/00000/00080
d D 2.1 00/08/21 11:04:18 ytse 2 1
c Before supporting Windows
e
s 00080/00000/00000
d D 1.1 99/10/29 15:58:07 yitong 1 0
c date and time created 99/10/29 15:58:07 by yitong
e
u
U
f e 0
t
T
I 1
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


int readUcharBlock(FILE *fp, uchar *blk, int width, int height, int pitch)
{
    for ( ; height>0; height--, blk+=pitch)
        if (!fread(blk, 1, width, fp))  return -1;
    return 0;
}


int writeUcharBlock(FILE *fp, uchar *blk, int width, int height, int pitch)
{
    for ( ; height>0; height--, blk+=pitch)
        if (!fwrite(blk, 1, width, fp))  return -1;
    return 0;
}


void printUcharBlock(uchar *blk, int width, int height, int pitch)
{
    int x;
    pitch -= width;
    for ( ; height>0; height--) {
        for (x=width; x>0; x--)
            printf("%4d ", *blk++);
        printf("\n");
        blk += pitch;
    }
}


void printShortBlock(short* blk, int width, int height, int pitch)
{
    int x;
    pitch -= width;
    for ( ; height>0; height--) {
        for (x=width; x>0; x--)
            printf("%4d ", *blk++);
        printf("\n");
        blk += pitch;
    }
}
 
 
void printIntBlock(int *blk, int width, int height, int pitch)
{
    int x;
    pitch -= width;
    for ( ; height>0; height--) {
        for (x=width; x>0; x--)
            printf("%4d ", *blk++);
        printf("\n");
        blk += pitch;
    }
}
 

void setConstShortBlock(short* blk, int width, int height, int pitch, int value)
{
    int x;
    pitch -= width;
    for ( ; height>0; height--, blk+=pitch)
        for (x=width; x>0; x--)
            *blk++ = value;
}


void copyUcharBlock(uchar *srcBlk, int width, int height, int srcPitch,
                    uchar *dstBlk)
{
    int x;
    srcPitch -= width;
    for ( ; height>0; height--, srcBlk+=srcPitch)
        for (x=width; x>0; x--)
            *dstBlk++ = *srcBlk++;
}
E 1
