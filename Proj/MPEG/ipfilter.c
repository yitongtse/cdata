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


#if 1
void print_hex (void* ptr, int nbytes)
{
    int i;
    unsigned char *temp = ptr;

    for (i=0; nbytes>0; nbytes--) {
        printf("%02x ", *temp++);
        if (++i==16) {
            i = 0;
            printf("\n");
        }
    }
    if (i) {
        printf("\n");
    }
}
#endif


int main(int argc, char** argv)
{
    Param par;
    char inFile[256], outFile[256];
    FILE *inFp, *outFp;
    int fileHdrSz, frmHdrSz;
    int nbytes, len, numTp;
    unsigned char ipBuf[MAX_IP_SIZE];
    int ipCnt = 0;
    unsigned short dstPort;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipfilter.par");
    getStringParam(&par, "Input IP/UDP/MPEG stream", inFile, "test.in");
    getStringParam(&par, "Output filename", outFile, "test.out");
    getIntParam(&par, "File header size", &fileHdrSz, 0);
    getIntParam(&par, "Frame header size", &frmHdrSz, 0);
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

    // Skip file header
    if (fseek(inFp, fileHdrSz, SEEK_CUR) == -1) {
        printf("Error: Failed to skip file header\n");
        exit (-1);
    }

    while (1) {
        // Skip frame header
        if (fseek(inFp, frmHdrSz, SEEK_CUR) == -1) {
            printf("Error: Failed to skip frame header\n");
            exit (-1);
        }

        // Process next IP
        nbytes = fread(ipBuf, 1, 28, inFp);	// read IP and UDP header
        if (nbytes != 28)  return 0;
printf("IP / UDP header:\n");
print_hex(ipBuf, 28);
        dstPort = *(short*)(ipBuf+22);
        len = *(short*)(ipBuf+24);
        len -= 8;
        printf("Pkt %d: dstPort %04x, len %d\n", ipCnt, dstPort, len);
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
