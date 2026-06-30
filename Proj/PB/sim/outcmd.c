/*****************************************************************************
    Palm Beach Output Command Generation

    This program accepts an MPEG-2 transport stream and generate the
    corresponding output command file for output FPGA debugging.

    Output command format:
      Word	Bits	Description
	0	9:0	PCR bits [41:32]
		13:10	CC correction value
		15:14	Used by HW.  Initialize with binary 10
		28:16	PID
		29	PCR correction enable
		30	PID remap enable
		31	CC replace enable
	1	31:0	PCR[31:0]
	2	23:0	control word [23:0]
		31:24	reserved
	3	23:0	control word [47:24]
		31:24	reserved

    The output file will be in hex format.
*****************************************************************************/

#include <stdio.h>
#include "param.h"
#include "bitbuf.h"
#include "reader.h"
#include "writer.h"
#include "tp.h"


#define MAX_REMAP_PID	16


int main(int argc, char** argv)
{
    Param par;
    int i;
    char tsFile[256];
    char outCmdFile[256];
    char hexFile[256];
    int pcrCorrect;
    int pcrOffset[2];			// 0: base 31:0, 1: base 32 + ext 8:0
    int numRemapPid;
    int inPid[MAX_REMAP_PID];
    int outPid[MAX_REMAP_PID];
    int ccRestamp;
    FILE *tsFp;
    FILE *outCmdFp;
    FILE *hexFp;
    int tpCnt = 0;
    unsigned int tpBuf[TP_SIZE>>2];
    int outCmd[4];
    int ctrlWord[2];
    int sz;
    int cc = 0;
    int pidRemap;			// remap PID for this packet
    int pcrRestamp;			// restamp PCR for this packet
    int pid;
    int k;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "outcmd.par");

    getStringParam(&par, "MPEG-2 TS filename", tsFile, "mpeg2.ts");
    getIntParam(&par, "Correct PCR?", &pcrCorrect, 1);
    getIntParam(&par, "PCR offset base", &pcrOffset[0], 0);
    getIntParam(&par, "PCR offset extension", &pcrOffset[1], 0);
    getIntParam(&par, "Number of remapped PIDs", &numRemapPid, 1);
    for (i=0; i<numRemapPid; i++) {
        getIntParam(&par, "Input PID", &inPid[i], 16);
        getIntParam(&par, "Output PID", &outPid[i], 16);
    }
    getIntParam(&par, "Restamp CC?", &ccRestamp, 0);
    getStringParam(&par, "Output command filename", outCmdFile, "mpeg2.cmd");
    getStringParam(&par, "Output MPEG-2 hex filename", hexFile, "mpeg2.hex");
    endParam(&par);

    tsFp = fopen(tsFile, "r");
    if (tsFp == NULL) {
        printf("Error: Failed to open input file %s\n", tsFile);
        exit (-1);
    }

    /* Transport packet alignment */
    if (findTp(tsFp, TP_SIZE, 1)) {
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
    printf("Sync at byte %d\n", ftell(tsFp));

    outCmdFp = fopen(outCmdFile, "w");
    if (outCmdFp == NULL) {
        printf("Error: Failed to open output file %s\n", outCmdFile);
        exit (-1);
    }

    hexFp = fopen(hexFile, "w");
    if (hexFp == NULL) {
        printf("Error: Failed to open output hex file %s\n", hexFile);
        exit (-1);
    }

    ctrlWord[0] = ctrlWord[1] = 0;

    // MPEG packet processing loop
    while (1) {
        // Read next packet
        sz = fread(tpBuf, 1, TP_SIZE, tsFp);
        if (sz < TP_SIZE)  break;

        // PID remap
        pid = (tpBuf[0] >> 8) & 0x1fff;
        pidRemap = 0;
        for (k=0; k<numRemapPid; k++)
            if (pid == inPid[k]) {
                pidRemap = 1;
                break;
            }

        printf("\nTP %d: PID %d", tpCnt, pid);
        if (pidRemap)  printf(" -> %d", outPid[k]);

        // Check for PCR
        pcrRestamp = 0;
        if (pcrCorrect
                && (tpBuf[0] & 0x20)		// adaptation_field exists
                && ((tpBuf[1]>>24) > 0)		// adaptation_field_length > 0
                && (tpBuf[1] & 0x100000)) {	// PCR_flag set
            pcrRestamp = 1;
            printf(", Restamp PCR");
        }

        // Generate output command
        outCmd[0] = 0x00008000;
        if (pcrRestamp) {
            outCmd[0] |= pcrOffset[1] | (1 << 29);
            outCmd[1] = pcrOffset[0];
        }
        else
            outCmd[1] = 0;

        if (pidRemap)
            outCmd[0] |= (outPid[k] << 16) | (1 << 30);

        if (ccRestamp) {
            outCmd[0] |= (cc << 10) | (1 << 31);
            printf(", Restamp CC %d", cc);
        }

        outCmd[2] = ctrlWord[0]++;
        outCmd[3] = ctrlWord[1]++;

        printf("\nOutCmd: %08x %08x %08x %08x",
               outCmd[0], outCmd[1], outCmd[2], outCmd[3]);
        fprintf(outCmdFp, "%08x %08x %08x %08x\n",
                outCmd[0], outCmd[1], outCmd[2], outCmd[3]);

        // Generate MPEG hex data
        for (i=0; i<(TP_SIZE>>2); i++)
            fprintf(hexFp, "%c%08x", (i%8)? ' ':'\n', tpBuf[i]);

        // Update
        tpCnt++;
        cc++;			// Note: CC is not maintained correctly
        if (cc>15)  cc = 0;
    }

    fclose(outCmdFp);
    fclose(hexFp);
    printf("%d TP read.\n", tpCnt);
}
