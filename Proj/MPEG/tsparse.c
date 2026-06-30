/*****************************************************************************
    File: tsparse.c

    Parse a transport stream

    Notes:

    To do:
      - need to do more accurate picture size measurement
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "reader.h"
#include "writer.h"
#include "infile.h"
#include "fifo.h"
#include "mpeg2.h"
#include "tp.h"
#include "pes.h"


#define	TP_SZ		188
#define	MAX_PICS	128


typedef struct
{
    unsigned int dts;	// picture DTS in system time base (45 kHz)
    int size;		// picture size in bytes
}
PicInfo;


typedef struct
{
    int pid;
    Fifo picFifo;
    PicInfo pic[MAX_PICS];
    int esBuf;			// 4-byte elementary stream buffer
    int tbOffset;		// timebase offset
    int seekPcr;		// looking for PCR?
    int hdrFlag;
    PicInfo* inPic;
    PicInfo* outPic;
    int vbvLevel;
    int picCnt;
}
ChInfo;


ChInfo ch;


void initChInfo(ChInfo* ch)
{
    int i;
    for (i=0; i<MAX_PICS; i++) {
        ch->pic[i].dts = 0x7FFFFFFF;
        ch->pic[i].size = 0;
    }
    fifoInit(&ch->picFifo, MAX_PICS);
    ch->seekPcr = 1;
    ch->hdrFlag = 0;
    ch->inPic = ch->outPic = &ch->pic[0];
    ch->vbvLevel = 0;
    ch->picCnt = -1;
}


void printPicFifo(ChInfo* ch)
{
    int i;

    printf("\nFIFO:");
    for (i=0; i<10; i++)
        printf("\n%d: dts %08x, sz %d", i, ch->pic[i].dts, ch->pic[i].size);
    printf("\n");
}


int main(int argc, char** argv)
{
    Param par;
    char tsFilename[256];
    InFile tsFile;
    TpInfo tp;
    int tpMode;
    PesInfo pes;
    int bitrate;
    int temp;
    int tpPlSz, pesPlSz;
    unsigned int clkBase = 0;	// system time in 45 kHz
    int clkExt = 0;		// system time in 27 MHz
    int pktTime;		// packet time in 27 MHz
    int pesHdrSz;
    int dts;
    int tpSz;
    int tpCnt = 0;
    int vbvFlag, rateFlag, picSzFlag;
    char vbvFilename[256], rateFilename[256], picSzFilename[256];
    FILE *vbvFp, *rateFp, *picSzFp;
    int rateWinSz;
    int rateTpCnt;
    int rateWinEnd;


    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsparse.par");
    getStringParam(&par, "Transport stream filename", tsFilename, "test.m2t");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "Bitrate (bps)", &bitrate, 4000000);
    getIntParam(&par, "Video PID", &ch.pid, 400);

    getIntParam(&par, "Measure VBV level?", &vbvFlag, 0);
    getCondStringParam(&par, "VBV?", vbvFlag, "Output file for VBV level",
        vbvFilename, "test.vbv");

    getIntParam(&par, "Measure bitrate?", &rateFlag, 0);
    getCondStringParam(&par, "bitrate?", rateFlag, "Output file for bitrate",
        rateFilename, "test.rate");
    getCondIntParam(&par, "bitrate?", rateFlag, "Measurement window (ms)",
        &rateWinSz, 500);

    getIntParam(&par, "Measure picture size?", &picSzFlag, 0);
    getCondStringParam(&par, "Picture size?", picSzFlag,
        "Output file for picture size", picSzFilename, "test.picSz");

    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;

    openInFile(&tsFile, tsFilename, tpSz);
    setReaderEcho(&tsFile.rdr, 0, stdout);

    // Compute time duration for a packet
    pktTime = (double)27000000 * tpSz * 8 / bitrate;

    // Initialization
    initChInfo(&ch);

    if (vbvFlag)
        vbvFp = fopen(vbvFilename, "w");

    if (rateFlag) {
        rateFp = fopen(rateFilename, "w");
        rateWinSz *= 45;	// convert ms into 45 kHz clock ticks
        rateWinEnd = rateWinSz;
        rateTpCnt = 0;
    }

    if (picSzFlag)
        picSzFp = fopen(picSzFilename, "w");
   

    // Main loop
    while (1) {
        // Check for SYNC byte
        temp = getByte(&tsFile.rdr, "sync");
        if (temp != TP_SYNC) {
            printf("\nTP %d: Lost sync!  Sync byte becomes 0x%02x",
                   tpCnt, temp);
            seekTp(&tp, &tsFile.rdr, tpSz, 1);
            printf("\nSync at byte %d", tsFile.rdr.byteCnt);
        }

        // Parse TP header
        getTpHdr(&tp, &tsFile.rdr);

        if (tp.PID == ch.pid) {
            if (rateFlag) {
                if (clkBase > rateWinEnd) {
                    // One rate measure window completed
                    fprintf(rateFp, "%d  %d\n", rateWinEnd, rateTpCnt);
                    rateWinEnd += rateWinSz;
                    rateTpCnt = 0;
                }
                rateTpCnt++;
            }

            if (tp.adaptation_field_control & 2) {
                getTpAf(&tp, &tsFile.rdr);

#if 1
                // Look for discontinuity_indicator
                if (tp.discontinuity_indicator) {
                    printf("\nTP %d: discontinuity point (%s PCR) found!",
                           tpCnt, tp.PCR_flag? "with":"without");
                    tp.discontinuity_indicator = 0;
                }
#endif

                // Check for PCR
                if (tp.PCR_flag) {
                    ch.seekPcr = 0;
                    ch.tbOffset = clkBase - tp.PCR[0];
//                    printf("\nPCR %08x, clk %08x, tbOffset %08x", tp.PCR[0],
//                           clkBase, ch.tbOffset);
                }
            }
            tpPlSz = TP_SIZE - tp.nbytes;

            if (ch.seekPcr) {
	        // Skip processing TP if the 1st PCR is not yet found
                skipTp(&tp, &tsFile.rdr, tpSz);
                continue;
            }

//            printf("\n%08x %d: TP sz %d", clkBase, tpCnt, tpPlSz);
//            if ((tp.adaptation_field_control & 2)  && tp.PCR_flag)
//                printf(" PCR %08x  offset %08x", tp.PCR[0], ch.tbOffset);

            // Check for PES header
            //   Note: we assume PES header is contained in 1 TP
            if (tp.payload_unit_start_indicator) {
                temp = getBits(&tsFile.rdr, 24, NULL);
                if (temp != 1)
                    printf("\nError: PES SC missing");
                getPesHdr(&pes, &tsFile.rdr);

                pesHdrSz = 9 + pes.PES_header_data_length;
                tp.nbytes += pesHdrSz;

//                printf("\n%08x %d: PES hdrSz %d", clkBase, tp.PID, pesHdrSz);
                if (pes.PTS_DTS_flags) {
                    dts = (pes.PTS_DTS_flags & 1)? pes.DTS[0] : pes.PTS[0];
                    dts += ch.tbOffset;
//                    printf(" DTS %08x->%08x", dts-ch.tbOffset, dts);
                }
            }
            pesPlSz = TP_SIZE - tp.nbytes;

            // Scan PES payload for start code
            byteAlignReader(&tsFile.rdr);
            for ( ; tp.nbytes<TP_SIZE; tp.nbytes++) {
                int scFlag = ((ch.esBuf & 0x00FFFFFF) == 1);
                temp = getByte(&tsFile.rdr, NULL);
                ch.esBuf = (ch.esBuf<<8) | temp;
                if (scFlag) {
//                    printf("\n\t\tSC %08x", ch.esBuf);

                    if (ch.hdrFlag) {
                        // Reading ES headers
                        if (temp == 1)  ch.hdrFlag = 0;
                    }
                    else {
                        if (temp==SEQ_START_CODE || temp==GOP_START_CODE
                            || temp==PIC_START_CODE) {
                            // Head of picture found
                            int idx = fifoAdd(&ch.picFifo);
                            ch.inPic = &ch.pic[idx];
                            ch.inPic->dts = dts;
                            ch.inPic->size = 0;
                            ch.hdrFlag = 1;
                            ch.picCnt++;
                            if (!ch.picCnt)
                                ch.vbvLevel = 0;    // Zero out VBV on 1st pic
//                          printf("\n\tPic %d: SC %02x, DTS %08x",
//                                 ch.picCnt, temp, dts);
//                          printPicFifo(&ch);
                        }
                    }
                }
            }

            ch.inPic->size += pesPlSz;

            // Update VBV level
            ch.vbvLevel += pesPlSz;
            if (clkBase >= ch.outPic->dts) {
                int idx;
//              printf(" DecPic %08x %d", ch.outPic->dts, ch.outPic->size);

                ch.vbvLevel -= ch.outPic->size;
                if (vbvFlag)
                    fprintf(vbvFp, "%d\t%d\n", ch.outPic->dts, ch.vbvLevel);

                if (picSzFlag)
                    fprintf(picSzFp, "%d\t%d\n", ch.outPic->dts, ch.outPic->size);

                ch.outPic->dts = 0x7FFFFFFF;
                ch.outPic->size = 0;
                idx = fifoDelete(&ch.picFifo);
                ch.outPic = &ch.pic[idx];


//              printPicFifo(&ch);
            }
//            printf(" VBV %d", ch.vbvLevel);

            if (vbvFlag)
                fprintf(vbvFp, "%d\t%d\n", clkBase, ch.vbvLevel);
        }

        flushReader(&tsFile.rdr);

        tpCnt++;
        if (!(tpCnt % 100000)) {
            printf(".");
            fflush(stdout);
        }

        // Check EOF
        if (tsFile.rdr.errFlag==EOF) {
            if (vbvFlag)  fclose(vbvFp);
            if (rateFlag)  fclose(rateFp);
            if (picSzFlag)  fclose(picSzFp);
            printf("\nEOF reached\n");
            exit (0);
        }

        // Update system clock
        clkExt += pktTime;
        clkBase += clkExt/600;
        clkExt = clkExt % 600;
    }
}

