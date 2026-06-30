h05525
s 00069/00000/00000
d D 1.1 03/03/22 01:39:42 ytse 1 0
c date and time created 03/03/22 01:39:42 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: ipfilter.c

    Extract MPEG TS stream from an IP/UDP/MPEG stream.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"

#define	TP_SIZE		188
#define	MAX_IP_SIZE	1600


int main(int argc, char** argv)
{
    Param par;
    char inFile[256], outFile[256];
    FILE *inFp, *outFp;
    int nbytes, len, numTp;
    unsigned char ipBuf[MAX_IP_SIZE];
    int ipCnt = 0;
int dstPort;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipfilter.par");
    getStringParam(&par, "Input IP/UDP/MPEG stream", inFile, "test.in");
    getStringParam(&par, "Output filename", outFile, "test.out");
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input file %s\n", inFile);
        exit (-1);
    }

    outFp = fopen(outFile, "w");
    if (outFp == NULL) {
        printf("Error: Failed to open output file %s\n", outFile);
        exit (-1);
    }

    while (1) {
        // Process next IP
        nbytes = fread(ipBuf, 1, 28, inFp);	// read IP and UDP header
        if (nbytes != 28)  return 0;
        dstPort = *(short*)(ipBuf+22);
        len = *(short*)(ipBuf+24);
        len -= 8;
        printf("IP %d: dstPort %d, len %d\n", ipCnt, dstPort, len);
        if (len % TP_SIZE)
            printf("Error: %d is not integer multiple of TPs\n", len);

        nbytes = fread(ipBuf+28, 1, len, inFp);
        if (nbytes != len) {
            printf("EOF reached\n");
            return 0;
        }

        nbytes = fwrite(ipBuf+28, 1, len, outFp);
        if (nbytes != len) {
            printf("Write error\n");
            return 0;
        }

        ipCnt++;
    }
}
E 1
