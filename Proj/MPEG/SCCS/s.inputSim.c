h08020
s 00109/00000/00000
d D 1.1 03/03/24 16:28:12 ytse 1 0
c date and time created 03/03/24 16:28:12 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: inputSim.c

    Simulate PB input task with clock recovery
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"

#define	TP_SIZE		188
#define	MAX_IP_SIZE	1600


int ipCnt = 0;
int tpCnt = 0;


void clkrec_PCR_TP()
{
}


void clkrec_non_PCR_TP()
{
}


int main(int argc, char** argv)
{
    Param par;
    char ipFile[256], atsFile[256];
    int udpPort;
    FILE *ipFp, *atsFp;
    int nbytes, port, len, numTp;
    unsigned char ipBuf[MAX_IP_SIZE];
    int i;
    int ipSz, pcrExist;
    unsigned int ats;
    unsigned int pcrBase, pcrExt;
    unsigned int* tpPtr;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "inputSim.par");
    getStringParam(&par, "Input IP/UDP/MPEG stream", ipFile, "test.ip");
    getStringParam(&par, "Arrival timestamp file", atsFile, "test.ats");
    getIntParam(&par, "UDP port", &udpPort, 49152);
    endParam(&par);

    ipFp = fopen(ipFile, "r");
    if (ipFp == NULL) {
        printf("Error: Failed to open input IP/UDP/MPEG file %s\n", ipFile);
        exit (-1);
    }

    atsFp = fopen(atsFile, "r");
    if (atsFp == NULL) {
        printf("Error: Failed to open ATS file %s\n", atsFile);
        exit (-1);
    }

    // Process each IP
    while (1) {
        // Read ATS
        fscanf(atsFp, "%x", &ats);

        // Read the entire IP
        nbytes = fread(ipBuf, 1, 28, ipFp);	// read IP and UDP header
        if (nbytes != 28)  return 0;
        ipSz = *(short*)(ipBuf+2);
        nbytes = fread(ipBuf+28, 1, ipSz-28, ipFp);
        if (nbytes != ipSz-28)  return 0;
        port = *(short*)(ipBuf+22);
        printf("\nIP %d: sz %d, UDP %d", ipCnt, ipSz, port);
        if (port != udpPort)
            continue;		// skip this IP

        len = *(short*)(ipBuf+24) - 8;
        if (len % TP_SIZE)
            printf("Error: %d is not integer multiple of TPs\n", len);
        numTp = len / TP_SIZE;

        // Process each TP within the IP
        tpPtr = (unsigned int*)(ipBuf+28);
        for (i=0; i<numTp; i++, tpPtr+=TP_SIZE/4) {
            printf("\n  TP %d: ATS %08x", tpCnt, ats);
            printf(", HDR %08x", tpPtr[0]);
            pcrExist = (tpPtr[0] & 0x20)		// AF field presents
                           && ((tpPtr[1]>>24) > 0)	// AF size > 0
                           && (tpPtr[1] & 0x00100000);	// PCR flag set

            if (pcrExist) {
                // Read PCR
                pcrBase = ((tpPtr[1] & 0x3f) << 17) | (tpPtr[2] >> 15);
                pcrExt = tpPtr[2] & 0x1ff;
                printf(", PCR %08x:%d", pcrBase, pcrExt);
                clkrec_PCR_TP();
            }
            else {
                clkrec_non_PCR_TP();
            }

            tpCnt++;
        }

        ipCnt++;
    }
}
E 1
