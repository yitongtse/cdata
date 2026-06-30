h59579
s 00002/00001/00118
d D 1.5 03/02/28 15:19:19 ytse 5 4
c Update
e
s 00002/00002/00117
d D 1.4 02/12/05 17:28:09 ytse 4 3
c Update
e
s 00005/00001/00114
d D 1.3 02/12/05 16:31:35 ytse 3 2
c Backup
e
s 00041/00050/00074
d D 1.2 02/12/03 12:03:28 ytse 2 1
c Support new input data format
e
s 00124/00000/00000
d D 1.1 02/11/19 16:15:06 ytse 1 0
c date and time created 02/11/19 16:15:06 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: ipMux.c

    Muxing of MPEG/UDP/IP streams

    This program is mux several IP packet streams containing SPTS to generate
D 2
    an MPTS IP stream.  The input file is in the format output by ipEncap.c.
    Mux is based on the arrival time stamps from ipEncap.c.
E 2
I 2
    an MPTS IP stream.  The input file is in the format output by ipEncap2.c.
    Mux is based on the arrival time stamps from ipEncap2.c.
E 2
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


D 2
#define	MAX_PROG		18
E 2
I 2
#define	MAX_PROG		30
E 2
				// max number of programs to mux


int main(int argc, char** argv)
{
    Param par;
    char inFilename[MAX_PROG][256];
    char outFilename[256];
    int sz;
    int done;
    int numProg;
D 2
    int i, j;
    int bitrate[MAX_PROG];
E 2
I 2
    int i, k;
    int ipPktSz[MAX_PROG];
E 2
    int arvlTime[MAX_PROG];

    InFile inFile[MAX_PROG];
D 2
    unsigned short ipHdrBuf[128];
    unsigned int tpBuf[64];
E 2
I 2
    unsigned int ipHdrBuf[5];
    char ipPktBuf[2048];
E 2
    unsigned short udpDestPort[MAX_PROG], pyldSz[MAX_PROG];
    FILE *inFp[MAX_PROG], *outFp;
D 2
    int max_i, totBitrate;
    int numTp, totTp, outTp;
E 2
I 2
    int min_i, totBitrate;
    int totIpPkt;
E 2

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipMux.par");
    getIntParam(&par, "Number of programs to mux", &numProg, 2);
I 3
    if (numProg > MAX_PROG) {
        printf("Error: Can mux up to %d streams only!\n", MAX_PROG);
        exit (-1);
    }
E 3
    for (i=0; i<numProg; i++)
        getStringParam(&par, "Filename", inFilename[i], "test.in");
D 2
    getIntParam(&par, "Total number of TPs in output", &totTp, 1000);
E 2
I 2
    getIntParam(&par, "Total number of IP packets to generate", &totIpPkt, 100);
E 2
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);

D 2
    outTp = 0;
    max_i = 0;
E 2

D 2
    for (i=0; i<numProg; i++) {
E 2
I 2
    for (i=0, min_i=0; i<numProg; i++) {
E 2
        inFp[i] = fopen(inFilename[i], "r");
        if (inFp[i] == NULL) {
D 3
            printf("Failed to open file %s\n", inFilename[i]);
E 3
I 3
            printf("Error: Failed to open file %s\n", inFilename[i]);
E 3
            exit (-1);
        }

        // Read first IP packet from stream
D 2
        sz = fread(ipHdrBuf, 1, 256, inFp[i]);
        if (sz != 256) {
E 2
I 2
        sz = fread(ipHdrBuf, 1, 20, inFp[i]);
        if (sz != 20) {
E 2
D 5
            printf("Failed to read IP header\n");
E 5
I 5
            printf("Error: Failed to read 1st IP header\n");
E 5
            exit (-1);
        }
D 2
        udpDestPort[i] = ipHdrBuf[11];
        pyldSz[i] = ipHdrBuf[12];
        arvlTime[i] = (ipHdrBuf[126] << 16) | ipHdrBuf[127];
E 2
I 2
        fseek(inFp[i], -20, SEEK_CUR);		// rewind

        ipPktSz[i] = ipHdrBuf[0];
        arvlTime[i] = ipHdrBuf[4];
E 2
        printf("Ch %d: arvl %d\n", i, arvlTime[i]);

D 2
        if (arvlTime[i] < arvlTime[max_i])  max_i = i;
E 2
I 2
        if (arvlTime[i] < arvlTime[min_i])  min_i = i;
E 2
    }

    outFp = fopen(outFilename, "w");

D 2
    while (1) {
        printf("Arvl %d, %d => ch %d\n", arvlTime[0], arvlTime[1], max_i);
E 2
I 2
    for (k=0; k<totIpPkt; k++) {
D 4
        printf("Arvl %08x, %08x => ch %d\n", arvlTime[0], arvlTime[1], min_i);
E 4
I 4
//        printf("Arvl %08x, %08x => ch %d\n", arvlTime[0], arvlTime[1], min_i);
E 4
E 2

D 2
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
E 2
I 2
	// Output earliest IP packet
        sz = fread(ipPktBuf, 1, ipPktSz[min_i], inFp[min_i]);
        if (sz != ipPktSz[min_i]) {
            printf("Failed to read IP packet\n");
            exit (-1);
E 2
        }
D 2

        if (outTp >= totTp) {
            printf("%d TPs successfully generated\n", outTp);
            return;
E 2
I 2
        
        sz = fwrite(ipPktBuf, 1, ipPktSz[min_i], outFp);
        if (sz != ipPktSz[min_i]) {
            printf("Failed to write IP packet\n");
            exit (-1);
E 2
        }

        // Fetch next IP header from previously selected channel
D 2
        sz = fread(ipHdrBuf, 1, 256, inFp[max_i]);
        if (sz != 256) {
E 2
I 2
        sz = fread(ipHdrBuf, 1, 20, inFp[min_i]);
        if (sz != 20) {
E 2
            printf("Failed to read IP header\n");
I 5
            printf("%d IP generated.\n", k);
E 5
            exit (-1);
        }
D 2
        udpDestPort[max_i] = ipHdrBuf[11];
        pyldSz[max_i] = ipHdrBuf[12];
        arvlTime[max_i] = (ipHdrBuf[126] << 16) | ipHdrBuf[127];
        printf("Ch %d: arvl %d\n", max_i, arvlTime[max_i]);
E 2
I 2
        fseek(inFp[min_i], -20, SEEK_CUR);		// rewind
E 2

I 2
        ipPktSz[min_i] = ipHdrBuf[0];
        arvlTime[min_i] = ipHdrBuf[4];
D 4
        printf("Ch %d: arvl %08x\n", min_i, arvlTime[min_i]);
E 4
I 4
//        printf("Ch %d: arvl %08x\n", min_i, arvlTime[min_i]);
E 4

E 2
        // Determine which stream to output
D 2
        max_i = 0;
E 2
I 2
        min_i = 0;
E 2
        for (i=0; i<numProg; i++)
D 2
            if (arvlTime[i] < arvlTime[max_i])  max_i = i;
E 2
I 2
            if (arvlTime[i] < arvlTime[min_i])  min_i = i;
E 2
    }
I 2

    printf("%d IP packets successfully generated\n", k);
E 2
}
E 1
