/*****************************************************************************
    File: ipMux.c

    Muxing of MPEG/UDP/IP streams

    This program is mux several IP packet streams containing SPTS to generate
    an MPTS IP stream.  The input file is in the format output by ipEncap2.c.
    Mux is based on the arrival time stamps from ipEncap2.c.
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


#define	MAX_PROG		30
				// max number of programs to mux


int main(int argc, char** argv)
{
    Param par;
    char inFilename[MAX_PROG][256];
    char outFilename[256];
    int sz;
    int done;
    int numProg;
    int i, k;
    int ipPktSz[MAX_PROG];
    int arvlTime[MAX_PROG];

    InFile inFile[MAX_PROG];
    unsigned int ipHdrBuf[5];
    char ipPktBuf[2048];
    unsigned short udpDestPort[MAX_PROG], pyldSz[MAX_PROG];
    FILE *inFp[MAX_PROG], *outFp;
    int min_i, totBitrate;
    int totIpPkt;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipMux.par");
    getIntParam(&par, "Number of programs to mux", &numProg, 2);
    if (numProg > MAX_PROG) {
        printf("Error: Can mux up to %d streams only!\n", MAX_PROG);
        exit (-1);
    }
    for (i=0; i<numProg; i++)
        getStringParam(&par, "Filename", inFilename[i], "test.in");
    getIntParam(&par, "Total number of IP packets to generate", &totIpPkt, 100);
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);


    for (i=0, min_i=0; i<numProg; i++) {
        inFp[i] = fopen(inFilename[i], "r");
        if (inFp[i] == NULL) {
            printf("Error: Failed to open file %s\n", inFilename[i]);
            exit (-1);
        }

        // Read first IP packet from stream
        sz = fread(ipHdrBuf, 1, 20, inFp[i]);
        if (sz != 20) {
            printf("Error: Failed to read 1st IP header\n");
            exit (-1);
        }
        fseek(inFp[i], -20, SEEK_CUR);		// rewind

        ipPktSz[i] = ipHdrBuf[0];
        arvlTime[i] = ipHdrBuf[4];
        printf("Ch %d: arvl %d\n", i, arvlTime[i]);

        if (arvlTime[i] < arvlTime[min_i])  min_i = i;
    }

    outFp = fopen(outFilename, "w");

    for (k=0; k<totIpPkt; k++) {
//        printf("Arvl %08x, %08x => ch %d\n", arvlTime[0], arvlTime[1], min_i);

	// Output earliest IP packet
        sz = fread(ipPktBuf, 1, ipPktSz[min_i], inFp[min_i]);
        if (sz != ipPktSz[min_i]) {
            printf("Failed to read IP packet\n");
            exit (-1);
        }
        
        sz = fwrite(ipPktBuf, 1, ipPktSz[min_i], outFp);
        if (sz != ipPktSz[min_i]) {
            printf("Failed to write IP packet\n");
            exit (-1);
        }

        // Fetch next IP header from previously selected channel
        sz = fread(ipHdrBuf, 1, 20, inFp[min_i]);
        if (sz != 20) {
            printf("Failed to read IP header\n");
            printf("%d IP generated.\n", k);
            exit (-1);
        }
        fseek(inFp[min_i], -20, SEEK_CUR);		// rewind

        ipPktSz[min_i] = ipHdrBuf[0];
        arvlTime[min_i] = ipHdrBuf[4];
//        printf("Ch %d: arvl %08x\n", min_i, arvlTime[min_i]);

        // Determine which stream to output
        min_i = 0;
        for (i=0; i<numProg; i++)
            if (arvlTime[i] < arvlTime[min_i])  min_i = i;
    }

    printf("%d IP packets successfully generated\n", k);
}
