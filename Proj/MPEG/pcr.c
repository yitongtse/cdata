/*****************************************************************************
    File: pcr.c
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


InFile inFile;


int main(int argc, char** argv)
{
    Param par;
    char inFilename[256];
    int tpMode;
    int numTp;
    TpInfo tp;
    int i, j;
    int size;
    uint hdr;
    int tpSz;
    int skipBytes;
    int temp;

    int bitrate;
    int pid;
    int tpIdx;
    uint prevPcr[2];	// follows PCR internal format convention
			// as defined in tp.h
    int prevTpIdx;	// TP index of previous PCR
    int tpPcrInc;	// PCR increment in 27 MHz units per TP
    int pcrDelta, expectedDelta;
    int pcrInterval;
    int jitter;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "pcr.par");
    getStringParam(&par, "Input filename", inFilename, "test.in");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "Bitrate (bps)", &bitrate, 27000000);
    getIntParam(&par, "PID", &pid, 16);
    getIntParam(&par, "Number of bytes to skip", &skipBytes, 0);
    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;
    openInFile(&inFile, inFilename, tpSz);
    fseek(inFile.fp, skipBytes, SEEK_SET);
    setReaderEcho(&inFile.rdr, 0, stdout);

    tpPcrInc = (int)(27e6 * 8 * 188 / bitrate + 0.5);
    printf("Bitstream: %s", inFilename);
    printf("\nPID: %d", pid);
    printf("\nBitrate: %d bps", bitrate);
    printf("\nTP PCR inc: %d\n", tpPcrInc);

    /* Transport packet alignment */
    if (findTp(inFile.fp, tpSz, 1)) {
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
    printf("\nSync at byte %d\n", ftell(inFile.fp));

    for (tpIdx=0; ; tpIdx++) {
        if (inFile.rdr.errFlag == EOF) {
            printf("\n\nEOF reached");
            exit (0);
        }

        if ((temp = getByte(&inFile.rdr, "sync")) != TP_SYNC)
            printf("Lost sync!  Sync byte becomes 0x%02x\n", temp);

        hdr = *inFile.rdr.bitBuf.curPtr;
        getTpHdr(&tp, &inFile.rdr);
        if (tp.PID==pid && tp.adaptation_field_control & 2) {
            getTpAf(&tp, &inFile.rdr);
            if (tp.PCR_flag) {
                // Process PCR
                pcrDelta = ((int)(tp.PCR[0] - prevPcr[0])) * 600;
                pcrDelta += (int)(tp.PCR[1] - prevPcr[1]);
                expectedDelta = (tpIdx - prevTpIdx) * tpPcrInc;
                jitter = pcrDelta - expectedDelta;
                pcrInterval = (int)(1e6*(tpIdx-prevTpIdx)*188*8 / bitrate);
                if (jitter > 30 || jitter < -30)
                  printf("\nTP %d: PCR %08x:%03x, itvl %d us, jitter %d",
                         tpIdx, tp.PCR[0], tp.PCR[1], pcrInterval, jitter);

                if (jitter > 100 || jitter < -100) {
                    printf("\nHDR %08x, AFC %x, S %x:",
                        hdr, tp.adaptation_field_control,
                        tp.payload_unit_start_indicator);
                    if (tp.adaptation_field_control & 2) {
                        printf("AF: sz %d", tp.adaptation_field_length,
                            tp.PCR_flag);
                        if (tp.PCR_flag)  printf(", PCR %08x", tp.PCR[0]);
                    }
                }

                // Update
                prevPcr[0] = tp.PCR[0];
                prevPcr[1] = tp.PCR[1];
                prevTpIdx = tpIdx;
            }
        }
        skipTp(&tp, &inFile.rdr, tpSz);
    }
}
