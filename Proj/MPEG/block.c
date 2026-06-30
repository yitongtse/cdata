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
