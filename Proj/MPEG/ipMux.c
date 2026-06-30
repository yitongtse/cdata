/*****************************************************************************
    File: ipMux.c

    Muxing of MPEG/UDP/IP streams

    This program is to mux two IP packet streams containing SPTS.
    The input file is assumed to be in the format output by ipencap.c.
    Mux is based on the arrival time stamps from ipencap.c.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "reader.h"
#include "writer.h"
#include "infile.h"
#include "tp.h"


int main(int argc, char** argv)
{
    Param par;
    char inFilename[2][256];
    char outFilename[256];
    int sz;
    int done;
    int i, j;
    int bitrate[2];
    int arvlTime[2];

    InFile inFile[2];
    unsigned short ipHdrBuf[128];
    unsigned int tpBuf[64];
    unsigned short udpDestPort[2], pyldSz[2];
    FILE *inFp[2], *outFp;
    int max_i, totBitrate;
    int numTp, totTp, outTp;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipMux.par");
    for (i=0; i<2; i++)
        getStringParam(&par, "Filename", inFilename[i], "test.in");
    getIntParam(&par, "Total number of TPs in output", &totTp, 1000);
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);

    outTp = 0;
    max_i = 0;

    for (i=0; i<2; i++) {
        inFp[i] = fopen(inFilename[i], "r");
        if (inFp[i] == NULL) {
            printf("Failed to open file %s\n", inFilename[i]);
            exit (-1);
        }

        // Read first IP packet from stream
        sz = fread(ipHdrBuf, 1, 256, inFp[i]);
        if (sz != 256) {
            printf("Failed to read IP header\n");
            exit (-1);
        }
        udpDestPort[i] = ipHdrBuf[11];
        pyldSz[i] = ipHdrBuf[12];
        arvlTime[i] = (ipHdrBuf[126] << 16) | ipHdrBuf[127];
        printf("Ch %d: arvl %d\n", i, arvlTime[i]);

        if (arvlTime[i] < arvlTime[max_i])  max_i = i;
    }

    outFp = fopen(outFilename, "w");

    while (1) {
        printf("Arvl %d, %d => ch %d\n", arvlTime[0], arvlTime[1], max_i);

	// Output IP/UDP header
        ipHdrBuf[11] = udpDestPort[max_i];
        ipHdrBuf[12] = pyldSz[max_i];
        ipHdrBuf[126] = arvlTime[max_i] >> 16;
        ipHdrBuf[127] = arvlTime[max_i] & 0xFFFF;
        fwrite(ipHdrBuf, 1, 256, outFp);

        // Output all TPs in the selected IP
        numTp = pyldSz[max_i] / TP_SIZE;
        outTp += numTp;
        for (j=0; j<numTp; j++) {
            sz = fread(tpBuf, 1, 256, inFp[max_i]);
            if (sz != 256) {
                printf("Failed to read TP\n");
                exit (-1);
            }
            fwrite(tpBuf, 1, 256, outFp);
        }

        if (outTp >= totTp) {
            printf("%d TPs successfully generated\n", outTp);
            return;
        }

        // Fetch next IP header from previously selected channel
        sz = fread(ipHdrBuf, 1, 256, inFp[max_i]);
        if (sz != 256) {
            printf("Failed to read IP header\n");
            exit (-1);
        }
        udpDestPort[max_i] = ipHdrBuf[11];
        pyldSz[max_i] = ipHdrBuf[12];
        arvlTime[max_i] = (ipHdrBuf[126] << 16) | ipHdrBuf[127];
        printf("Ch %d: arvl %d\n", max_i, arvlTime[max_i]);

        // Determine which stream to output
        max_i = 0;
        for (i=0; i<2; i++)
            if (arvlTime[i] < arvlTime[max_i])  max_i = i;
    }
}
