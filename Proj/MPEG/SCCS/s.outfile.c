h62573
s 00002/00002/00052
d D 2.2 00/08/21 11:23:55 ytse 3 2
c Added support of Windows
e
s 00000/00000/00054
d D 2.1 00/08/21 11:04:23 ytse 2 1
c Before supporting Windows
e
s 00054/00000/00000
d D 1.1 99/10/29 15:58:10 yitong 1 0
c date and time created 99/10/29 15:58:10 by yitong
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
#include "bitbuf.h"
#include "writer.h"
#include "outfile.h"


void openOutFile(OutFile *outFile, char *filename, int bufSz)
{
    if ((outFile->fp = fopen(filename, "w")) == NULL) {
        printf("Error: Failed to open file %s for output\n", filename);
D 3
        exit(-1);
E 3
I 3
        exit(-4076);
E 3
    }

    outFile->bufSz = bufSz & 0xFFFFFFFc; /* make sure it's integer # of words */
    if ((outFile->buf = (uint*)malloc(outFile->bufSz)) == NULL) {
        printf("Error: Failed to allocate buffer for file output\n");
D 3
        exit(-1);
E 3
I 3
        exit(-546);
E 3
    }

    initWriter(&outFile->wtr, writeFile, outFile);
    setBitBuf(&outFile->wtr.bitBuf, outFile->buf,
              outFile->buf+(outFile->bufSz>>2), 32);
}


/* Note: assumes writer is byte aligned.
*/
int writeFile(OutFile *outFile)
{
    fwrite(outFile->buf, 4, outFile->wtr.bitBuf.curPtr - outFile->buf,
           outFile->fp);
    setBitBuf(&outFile->wtr.bitBuf, outFile->buf, outFile->wtr.bitBuf.endPtr,
              32);
    return 0;
}


void flushOutFile(OutFile *outFile)
{
    flushWriter(&outFile->wtr);
}


void closeOutFile(OutFile *outFile)
{
    byteAlignWriter(&outFile->wtr);	/* make sure no partially filled byte */
    flushWriter(&outFile->wtr);
    fclose(outFile->fp);
    free(outFile->buf);
}

E 1
