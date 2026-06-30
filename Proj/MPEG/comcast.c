/*****************************************************************************
    File: comcast.c

    Analyze timing capture info from Comcast trip

*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"

#define MAX_LINE_WIDTH  80


int main(int argc, char** argv)
{
    Param par;
    char inFile[256];
    char outFile[256];
    FILE *inFp;
    FILE *outFp;
    char line[MAX_LINE_WIDTH];
    char *res;
    int seqNo;
    char timeStampStr[13];
    long long timeStamp, prevTimeStamp, startTimeStamp;
    long timeStampDiff, relTimeStamp;
    int pktSize;
    int offset;
    float bitrate;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "comcast.par");
    getStringParam(&par, "Input timing info filename", inFile, "data.in");
    getStringParam(&par, "Output filename", outFile, "data.out");
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        fprintf(stderr, "Failed to open input file %s\n", inFile);
        exit (-1);
    }

    outFp = fopen(outFile, "w");
    if (outFp == NULL) {
        fprintf(stderr, "Failed to open output file %s\n", outFile);
        exit (-1);
    }

    // Skip the header line
    fgets(line, MAX_LINE_WIDTH, inFp);

    while (1) {
        res = fgets(line, MAX_LINE_WIDTH, inFp);
        if (res == NULL) {
            fclose(inFp);
            fclose(outFp);
            return 0;
        }

        sscanf(line, "%d, %12s, %d, %d",
               &seqNo, timeStampStr, &pktSize, &offset);
        timeStamp = strtoll(timeStampStr, NULL, 16);
        timeStampDiff = timeStamp - prevTimeStamp;

        bitrate = (pktSize * 8.) * 1e2 / timeStampDiff;

        prevTimeStamp = timeStamp;

        if (seqNo > 0) {
            relTimeStamp = (timeStamp - startTimeStamp) / 100;
            fprintf(outFp, "%d %d %d %f\n",
                    seqNo, relTimeStamp, timeStampDiff, bitrate);
        } else {
            startTimeStamp = timeStamp;
        }
    }
}
