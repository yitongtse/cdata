#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"
#include "vld.h"


char vldTable[] =
  "100\n" \
  "00\n" \
  "01\n" \
  "101\n" \
  "110\n" \
  "1110\n" \
  "1111 0\n" \
  "1111 10\n" \
  "1111 110\n" \
  "1111 1110\n" \
  "1111 1111 0\n" \
  "1111 1111 1\n";
 
 
void main()
{
    Reader in;
    InFile inFile;
    Vld vld;
    int i;
 
    buildVld(&vld, vldTable, 4);
    printVld(&vld);
 
    openInFile(&inFile, &in, "test.in", 16);
    setReaderEcho(&in, 1, stdout);
 
    for (i=0; i<7; i++)
        getVld(&in, &vld, "");
}

