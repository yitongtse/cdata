h64914
s 00000/00000/00020
d D 2.1 00/08/21 11:04:19 ytse 2 1
c Before supporting Windows
e
s 00020/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef BLOCK_H
#define BLOCK_H

int readUcharBlock(FILE *fp, uchar *blk, int width, int height, int pitch);

int writeUcharBlock(FILE *fp, uchar *blk, int width, int height, int pitch);

void printUcharBlock(uchar *blk, int width, int height, int pitch);

void printShortBlock(short *blk, int width, int height, int pitch);

void printIntBlock(int *blk, int width, int height, int pitch);

void setConstShortBlock(short *blk, int width, int height, int pitch,
                        int value);

void copyUcharBlock(uchar *srcBlk, int width, int height, int srcPitch,
                    uchar *dstBlk);

#endif
E 1
