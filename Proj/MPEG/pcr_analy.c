/*****************************************************************************
    File: tsview.c
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


#define MAX_INT  0x7fffffff


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

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsview.par");
    getStringParam(&par, "Input filename", inFilename, "test.in");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "Number of TP packets to examine (0 for EOF)", &numTp, 1);
    getIntParam(&par, "Number of bytes to skip", &skipBytes, 0);
    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;
    openInFile(&inFile, inFilename, tpSz);
    fseek(inFile.fp, skipBytes, SEEK_SET);
    setReaderEcho(&inFile.rdr, 0, stdout);

    /* Transport packet alignment */
    if (findTp(inFile.fp, tpSz, 1)) {
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
    printf("Sync at byte %d\n", ftell(inFile.fp));
    if (!numTp) {
        numTp = MAX_INT;
    }

    printf(" No.  PID C A S   Header   Payload\n");
    for (i=0; i<numTp; i++) {
        if ((temp = getByte(&inFile.rdr, "sync")) != TP_SYNC)
            printf("Lost sync!  Sync byte becomes 0x%02x\n", temp);

        hdr = *inFile.rdr.bitBuf.curPtr;
        temp = getTpHdr(&tp, &inFile.rdr);
        if (temp) {
            printf("getTpHdr error: %d\n", temp);
            exit(0);
        }
        printf("%3d  %4x %x %x %x  %08x  ", i, tp.PID, tp.continuity_counter,
               tp.adaptation_field_control, tp.payload_unit_start_indicator,
               hdr);
        if (tp.adaptation_field_control & 2) {
            getTpAf(&tp, &inFile.rdr);
            printf("AF: sz %d", tp.adaptation_field_length, tp.PCR_flag);
            if (tp.PCR_flag)  printf(", PCR %08x", tp.PCR[0]);
            printf("\n                           ");
        }
        size = TP_SIZE - tp.nbytes;	/* remaining payload size */
        if (size>16)  size = 16;
        for (j=0; j<size; j++)
            printf("%02x ", getByte(&inFile.rdr, ""));
        printf("...\n");
        tp.nbytes += size;
        skipTp(&tp, &inFile.rdr, tpSz);

#if 1
        if (tp.discontinuity_indicator) {
            printf("Discontinuity\n");
            tp.discontinuity_indicator = 0;
        }
#endif
    }
}
