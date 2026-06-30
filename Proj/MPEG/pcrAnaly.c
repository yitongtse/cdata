/*****************************************************************************
    File: pcrAnaly.c

    PCR analysis tool

    Note: HP analyzer's 204 packet format is used in the program.
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"


#define  BASE_TO_SEC	        (1. / 45e3)
#define  EXT_TO_SEC             (1. / 27e6)
#define  AT_HIGH_TO_SEC         (4294967296e-8)
#define  AT_LOW_TO_SEC          (1e-8)


int main(int argc, char** argv)
{
    Param par;
    char filename[256];
    FILE *inFp;
    uchar tpBuf[204];
    uint *ptr32 = (uint*)tpBuf;
    int pid;
    int sz;
    int tpCnt = 0;
    int newTimeBase = 1;
    uint arvlTimeHigh, arvlTimeLow;
    uint pcrBase, pcrExt;
    double arvlTime;
    double prevPcrArvlTime;
    double pcrTime;
    double prevPcrTime = 0.;
    double pcrArvlTimeInc;
    double pcrTimeInc;
    double jitter;
    double maxJitter = 0.;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "pcrAnaly.par");
    getStringParam(&par, "Input filename", filename, "test.in");
    getIntParam(&par, "PCR PID to analyze", &pid, 33);
    endParam(&par);

    if ((inFp = fopen(filename, "rb")) == NULL) {
        printf("Error: Failed to open input file %s\n", filename);
        exit (-1);
    }

    while (1) {
        sz = fread(tpBuf, 1, 204, inFp);
        if (sz != 204)  break;

        // Get arrival time
        arvlTimeHigh = (tpBuf[188] << 8) | tpBuf[189];
        arvlTimeLow = (tpBuf[190] << 24) | (tpBuf[191] << 16)
                      | (tpBuf[192] << 8) | tpBuf[193];
        arvlTime = arvlTimeHigh * AT_HIGH_TO_SEC + arvlTimeLow * AT_LOW_TO_SEC;

        if ((((*ptr32 >> 8) & 0x1fff) == pid) &&    // PCR PID
            (*ptr32 & 0x00000020) &&                // has adaptation field
            (tpBuf[4] > 0) &&                       // has flags in adaptation
            (tpBuf[5] & 0x10)) {                    // has PCR

            // Read PCR
            pcrBase = (tpBuf[6]<<24) | (tpBuf[7]<<16)
                      | (tpBuf[8]<<8) | tpBuf[9];
            pcrExt = (((tpBuf[10] & 1)<<8) | tpBuf[11]) + (tpBuf[10]>>7) * 300;
            pcrTime = pcrBase * BASE_TO_SEC + pcrExt * EXT_TO_SEC;

            // Read discontinuity indicator
            if (tpBuf[5] & 0x80) {
                newTimeBase = 1;
            }

            pcrTimeInc = pcrTime - prevPcrTime;
            pcrArvlTimeInc = arvlTime - prevPcrArvlTime;
            jitter = pcrTimeInc - pcrArvlTimeInc;

            printf("\nAT %.9f (%.9f), PCR %.9f (%.9f)",
                   arvlTime, pcrArvlTimeInc, pcrTime, pcrTimeInc);
            printf("\n  jitter %.0f ns", jitter * 1e9);
            if (newTimeBase)
                printf("\n  New timebase!");

            if (!newTimeBase) {
                if (jitter < 0)  jitter = -jitter;
                if (jitter > maxJitter)  maxJitter = jitter;
            }

            // Update
            newTimeBase = 0;
            prevPcrTime = pcrTime;
            prevPcrArvlTime = arvlTime;
        }

        tpCnt++;
    }

    printf("\nMax jitter: %.0f ns\n", maxJitter * 1e9);
    fclose(inFp);
}
