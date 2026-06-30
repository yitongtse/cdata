/*****************************************************************************
    File: 204to188.c

    Convert 204 byte packet to 188 byte packet
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"


int main(int argc, char** argv)
{
    Param par;
    char inFileName[256], outFileName[256];
    FILE *inFp, *outFp;
    char tpBuf[204];
    int sz;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "204to188.par");
    getStringParam(&par, "Input 204 byte packet filename", inFileName,
        "test.in");
    getStringParam(&par, "Output 188 byte packet filename", outFileName,
        "test.out");
    endParam(&par);

    if ((inFp = fopen(inFileName, "rb")) == NULL) {
        printf("Error: Failed to open input file %s\n", inFileName);
        exit (-1);
    }

    if ((outFp = fopen(outFileName, "wb")) == NULL) {
        printf("Error: Failed to open output file %s\n", outFileName);
        exit (-1);
    }

    while (1) {
        sz = fread(tpBuf, 1, 204, inFp);
        if (sz != 204)  break;
        fwrite(tpBuf, 1, 188, outFp);
    }

    fclose(inFp);
    fclose(outFp);
}
