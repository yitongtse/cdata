#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "writer.h"

#define BUF_SZ		100


typedef struct
{
    FILE *fp;
    int buf[BUF_SZ];
    Writer writer;
}
FileBuf;
  
FileBuf fileBuf;


int emptyBuf(FileBuf *fileBuf)
{
printf("Empty buffer...\n");
    fwrite(fileBuf->buf, sizeof(int), BUF_SZ, fileBuf->fp);
    setBitBuf(&fileBuf->writer.bitBuf, fileBuf->buf,
              fileBuf->writer.bitBuf.curPtr);
    return 0;
}


main(int argc, char **argv) 
{
    int i;
    fileBuf.fp = fopen("test.out", "w");
    initWriter(&fileBuf.writer, &emptyBuf, &fileBuf);
    setBitBuf(&fileBuf.writer.bitBuf, fileBuf.buf, fileBuf.buf+BUF_SZ);
    setWriterEcho(&fileBuf.writer, 1, stdout);

    for (i=0x61; i<=0x6d; i++) {
        putByte(&fileBuf.writer, i, "byte");
//        putBits(&fileBuf.writer, 8, i, "8 bits");
    }

    flushWriter(&fileBuf.writer);
    closeWriter(&fileBuf.writer);
}
