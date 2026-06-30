#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"

InFile in;


void dumpBuffer(InFile *inFile)
{
    int i;
    printf("\n");
    for (i=0; i<inFile->bufSz>>2; i++)
        printf("%08x ", inFile->buf[i]);
    printf("\n");
}


void main(int argc, char **argv) 
{
    int i;
    openInFile(&in, "test.in", 48);
    setReaderEcho(&in.rdr, 1, stdout);

    getBits(&in.rdr, 4, "get bits");
    for (i=0xab; i<0xab+4; i++) {
        getBits(&in.rdr, 24, "get bits");
        editBits(&in.rdr, 8, i, "edit bits");
        flushBuffer(&in.rdr);
    }
    dumpBuffer(&in);
    closeInFile(&in);
}


/*
void main(int argc, char **argv) 
{
    int i;
    uint x;
    for (i=1; i<16; i++) {
        x = 0xFFFFFFFF;
        setBitField(&x, i-1, i, 0);
        printf("%d: %08x\n", i, x);
    }
}
*/
