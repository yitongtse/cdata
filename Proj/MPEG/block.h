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
