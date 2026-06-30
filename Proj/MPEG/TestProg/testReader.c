#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"

#define BUF_SZ		2


typedef struct
{
    FILE *fp;
    int buf[BUF_SZ];
    Reader reader;
}
FileBuf;
  
FileBuf fileBuf;


int refillBuf(FileBuf *fileBuf)
{
    fread(fileBuf->buf, sizeof(int), BUF_SZ, fileBuf->fp);
    if (feof(fileBuf->fp))  return EOF;
    setBitBuf(&fileBuf->reader.bitBuf, fileBuf->buf, fileBuf->buf+BUF_SZ);
    return 0;
}


main(int argc, char **argv) 
{
    int i;
    fileBuf.fp = fopen("test.in", "r");
    initReader(&fileBuf.reader, &refillBuf, &fileBuf);
    setBitBuf(&fileBuf.reader.bitBuf, 0, 0);
    setReaderEcho(&fileBuf.reader, 1, stdout);

    getBits(&fileBuf.reader, 4, "4 bits");
    for (i=0; i<20; i++) {

//        getByte(&fileBuf.reader, "byte");
        getBits(&fileBuf.reader, 8, "8 bits");

        if (fileBuf.reader.errFlag==EOF) {
            printf("\nError: EOF reached!\n");
            exit(-1);
        }
    }
    closeReader(&fileBuf.reader);
}
