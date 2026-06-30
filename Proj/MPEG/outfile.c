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
        exit(-4076);
    }

    outFile->bufSz = bufSz & 0xFFFFFFFc; /* make sure it's integer # of words */
    if ((outFile->buf = (uint*)malloc(outFile->bufSz)) == NULL) {
        printf("Error: Failed to allocate buffer for file output\n");
        exit(-546);
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

