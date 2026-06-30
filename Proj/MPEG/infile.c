#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"

#include "def.h"


void openInFile(InFile *inFile, char *filename, int bufSz)
{
    if ((inFile->fp = fopen(filename, "rb")) == NULL) {
        printf("Error: Failed to open file %s for input\n", filename);
        exit(-4563);
    }

    inFile->bufSz = bufSz & 0xFFFFFFFc;  /* make sure it's integer # of words */
    if ((inFile->buf = (uint*)malloc(inFile->bufSz)) == NULL) {
        printf("Error: Failed to allocate buffer to file input\n");
        exit(-6783);
    }

    initReader(&inFile->rdr, readFile, inFile);
    setBitBuf(&inFile->rdr.bitBuf, 0, 0, 0);
}


int readFile(InFile *inFile)
{
    int sz = fread(inFile->buf, 1, inFile->bufSz, inFile->fp);

#ifndef UNIX
    int i;
    char tmp;
    union {
        char ch[4];
        uint ui;
    } data;
#endif

    setBitBuf(&inFile->rdr.bitBuf, inFile->buf, inFile->buf+(sz>>2), 32);

#ifndef UNIX
    /* for the big indian / small indian problem */
    for(i=0;i<sz>>2;i++) {
        data.ui = inFile->buf[i];
        tmp = data.ch[0];
        data.ch[0] = data.ch[3];
        data.ch[3] = tmp;
        tmp = data.ch[1];
        data.ch[1] = data.ch[2];
        data.ch[2] = tmp;
        inFile->buf[i] = data.ui;
    }
#endif

    return feof(inFile->fp)? EOF : 0;
}


void closeInFile(InFile *inFile)
{
    fclose(inFile->fp);
    free(inFile->buf);
}

