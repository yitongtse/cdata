h54534
s 00015/00008/00095
d D 1.2 03/03/24 11:20:53 ytse 2 1
c Working version
e
s 00103/00000/00000
d D 1.1 03/03/21 23:30:42 ytse 1 0
c date and time created 03/03/21 23:30:42 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: ineo.c

    Convert IneoQuest capture file to MPEG transport stream.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "param.h"


int main(int argc, char** argv)
{
    Param par;
D 2
    char inFile[256], outFile[256];
    FILE *inFp, *outFp;
E 2
I 2
    char inFile[256], ipFile[256], atsFile[256];
    FILE *inFp, *ipFp, *atsFp;
E 2
    int i;
    char line[3200];
    int id, tsh, tsl, dth, dtl, len, status, ipg, jitter;
    char* ipData;
    unsigned int byte;
    int pktCnt = 0;
    unsigned char frmHdr[14];
    int badFrm;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ineo.par");
    getStringParam(&par, "Input IneoQuest Capture filename", inFile, "test.in");
D 2
    getStringParam(&par, "Output TS filename", outFile, "test.out");
E 2
I 2
    getStringParam(&par, "Output IP/UDP/MPEG filename", ipFile, "test.ip");
    getStringParam(&par, "Output ATS filename", atsFile, "test.ats");
E 2
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input file %s\n", inFile);
        exit (-1);
    }

D 2
    outFp = fopen(outFile, "w");
    if (outFp == NULL) {
        printf("Error: Failed to open output file %s\n", outFile);
E 2
I 2
    ipFp = fopen(ipFile, "w");
    if (ipFp == NULL) {
        printf("Error: Failed to open output IP/UDP/MPEG file %s\n", ipFile);
E 2
        exit (-1);
    }

I 2
    atsFp = fopen(atsFile, "w");
    if (atsFp == NULL) {
        printf("Error: Failed to open output ATS file %s\n", atsFile);
        exit (-1);
    }

E 2
    fgets(line, 3200, inFp);	// Skip header line

    while (1) {
        if (fgets(line, 3200, inFp) == NULL) {
            printf("\nEnd of file\n");
            break;
        }

        // Parse the line
        id = atoi(strtok(line, ","));
        tsh = atoi(strtok(NULL, ","));
        tsl = atoi(strtok(NULL, ","));
        dth = atoi(strtok(NULL, ","));
        dtl = atoi(strtok(NULL, ","));
        len = atoi(strtok(NULL, ","));
        ipData = strtok(NULL, ",");
        status = atoi(strtok(NULL, ","));
        ipg = atoi(strtok(NULL, ","));
        jitter = atoi(strtok(NULL, ","));

        badFrm = 0;
        if (pktCnt==0) {
            // Backup the first Ethernet frame header
            for (i=0; i<14; i++) {
                sscanf(ipData, "%2x", &byte);
                frmHdr[i] = byte;
                ipData += 2;
            }
        }
        else {
            // Compare Ethernet frame header with the backup one
            for (i=0; i<14; i++) {
                sscanf(ipData, "%2x", &byte);
                if (byte != frmHdr[i]) {
                    badFrm = 1;
                    break;
                }
                ipData += 2;
            }
        }

        // Note: the first 14 bytes are Ethernet frame header
        //       the last 4 bytes are Ethernet frame checksum
        if (!badFrm) {
            len -= 14 + 4;
            if ((len-28) % 188)
                printf("Warning: IP packet %d length %d\n", pktCnt, len);

            for (i=0; i<len; i++, ipData+=2) {
                sscanf(ipData, "%2x", &byte);
D 2
                fputc(byte, outFp);
E 2
I 2
                fputc(byte, ipFp);
E 2
            }
        }
        else
            printf("Pkt %d skipped\n", pktCnt);

        pktCnt++;
    }
D 2
    fclose(outFp);
E 2
I 2
    fclose(ipFp);
E 2
}
E 1
