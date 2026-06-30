#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "writer.h"
#include "outfile.h"


#define BUF_SZ		16


main(int argc, char **argv) 
{
    int i;
    OutFile outFile;

    openOutFile(&outFile, "test.out", BUF_SZ);
    setWriterEcho(&outFile.wtr, 1, stdout);

    for (i=0; i<20; i++) {
        printBitBuf(&outFile.wtr.bitBuf);
        printf("Writer: bitPos %d\n", outFile.wtr.bitPos);
        printf("Writing byte %d\n\n", i);
        putByte(&outFile.wtr, i, NULL);
    }
}
