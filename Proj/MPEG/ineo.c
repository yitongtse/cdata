/*****************************************************************************
    File: ineo.c

    Convert IneoQuest capture file to MPEG transport stream.
*****************************************************************************/
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "param.h"


// Constants used for converting Ineoquest's nanosecond timestamps
// to the 45 kHz timestamps (1e9/45e3 = 200000/9)
//
#define	A	9
#define	B	200000


int main(int argc, char** argv)
{
    Param par;
    char inFile[256], ipFile[256], atsFile[256], infoFile[256];
    FILE *inFp, *ipFp, *atsFp, *infoFp;
    int i, j;
    char line[3200];
    int id, dth, dtl, len, status, ipg, jitter;
    unsigned int tsh, tsl, ts;
    unsigned char *ipData, *tpPtr;
    unsigned char tpBuf[188];
    unsigned int byte;
    int pktCnt = 0;
    unsigned char frmHdr[14];
    int badFrm;
    double temp;
    int numTp;
    int pcrFlag;
    unsigned int pcrBase, pcrExt;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ineo.par");
    getStringParam(&par, "Input IneoQuest Capture filename", inFile, "test.in");
    getStringParam(&par, "Output IP/UDP/MPEG filename", ipFile, "test.ip");
    getStringParam(&par, "Output ATS filename", atsFile, "test.ats");
    getStringParam(&par, "Output TP info filename", infoFile, "test.info");
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input file %s\n", inFile);
        exit (-1);
    }

    ipFp = fopen(ipFile, "w");
    if (ipFp == NULL) {
        printf("Error: Failed to open output IP/UDP/MPEG file %s\n", ipFile);
        exit (-1);
    }

    atsFp = fopen(atsFile, "w");
    if (atsFp == NULL) {
        printf("Error: Failed to open output ATS file %s\n", atsFile);
        exit (-1);
    }

    infoFp = fopen(infoFile, "w");
    if (infoFp == NULL) {
        printf("Error: Failed to open output TP info file %s\n", infoFile);
        exit (-1);
    }

    fgets(line, 3200, inFp);	// Skip header line
    fgets(line, 3200, inFp);	// Skip 1st TP

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

            // Time stamp conversion
            temp = (((double)tsh) * 4294967296. + tsl) * A / B;
            temp = fmod(temp, 4294967296.);
            ts = (unsigned int)temp;
            fprintf(atsFp, "%08x\n", ts);

            // Copy IP/UDP header (20 + 8 bytes)
            for (i=0; i<28; i++, ipData+=2) {
                sscanf(ipData, "%2x", &byte);
                fputc(byte, ipFp);
            }

            numTp = (len - 28) / 188;
            for (i=0; i<numTp; i++) {
                // Copy TP data into buffer
                for (j=0, tpPtr=tpBuf; j<188; j++, ipData+=2) {
                    sscanf(ipData, "%2x", &byte);
                    *tpPtr++ = byte;
                }
                assert(tpBuf[0] == 0x47);
                fwrite(tpBuf, 1, 188, ipFp);

                pcrFlag = 0;
                pcrBase = 0;
                pcrExt = 0;

                if ((tpBuf[3] & 0x20)		// adaptation field exists
                        && (tpBuf[4]>0)		// AF size > 0
                        && (tpBuf[5] & 0x10))	// PCR flag set
                {
                    pcrFlag = 1;
                    pcrBase = (tpBuf[6]<<24) | (tpBuf[7]<<16)
                                  | (tpBuf[8]<<8) | tpBuf[9];
                    pcrExt = (((tpBuf[10] & 1)<<8) | tpBuf[11])
                                 + (tpBuf[10]>>7) * 300;
                }

                fprintf(infoFp, "%08x %d %08x %03x\n",
                        ts, pcrFlag, pcrBase, pcrExt);
            }
        }
        else
            printf("Pkt %d skipped\n", pktCnt);

        pktCnt++;
    }

    fclose(inFp);
    fclose(ipFp);
    fclose(infoFp);
    fclose(atsFp);
}
