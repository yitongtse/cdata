/*****************************************************************************
    File: tsparse2.c

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
#include "mpeg2.h"
#include "tp.h"
#include "pes.h"


#define	TP_SZ		188
#define	MAX_PICS	128


typedef struct
{
    int size;		// picture size in bytes
    int picType;	// picture type
    unsigned int dts;	// picture DTS in system time base (45 kHz)
}
PicInfo;


typedef struct
{
    int pid;
    int esBuf;			// 4-byte elementary stream buffer
    int tbOffset;		// timebase offset
    int seekPcr;		// looking for PCR?
    int hdrFlag;
    int picCnt;
}
ChInfo;


ChInfo ch;
PicInfo pic;


void initChInfo(ChInfo* ch)
{
    ch->seekPcr = 1;
    ch->hdrFlag = 0;
    ch->picCnt = -1;
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

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsparse2.par");
    getStringParam(&par, "Transport stream filename", tsFilename, "test.m2t");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "Bitrate (bps)", &bitrate, 4000000);
    getIntParam(&par, "Video PID", &ch.pid, 400);
    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;

    openInFile(&tsFile, tsFilename, tpSz);
    setReaderEcho(&tsFile.rdr, 0, stdout);

    // Compute time duration for a packet
    pktTime = (double)27000000 * tpSz * 8 / bitrate;

    // Initialization
    initChInfo(&ch);

    // Main loop
    while (1) {
        // Check for SYNC byte
        temp = getByte(&tsFile.rdr, "sync");
        if (temp != TP_SYNC)
            printf("\nLost sync!  Sync byte becomes 0x%02x", temp);

        // Parse TP header
        getTpHdr(&tp, &tsFile.rdr);
        if (tp.PID == ch.pid) {

            if (tp.adaptation_field_control & 2) {
                getTpAf(&tp, &tsFile.rdr);

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

            printf("\n%08x %d: TP sz %d", clkBase, tpCnt, tpPlSz);
            if ((tp.adaptation_field_control & 2)  && tp.PCR_flag)
                printf(" PCR %08x  offset %08x", tp.PCR[0], ch.tbOffset);

            // Check for PES header
            //   Note: we assume PES header is contained in 1 TP
            if (tp.payload_unit_start_indicator) {
                temp = getBits(&tsFile.rdr, 24, NULL);
                if (temp != 1)
                    printf("\nError: PES SC missing");
                getPesHdr(&pes, &tsFile.rdr);

                pesHdrSz = 9 + pes.PES_header_data_length;
                tp.nbytes += pesHdrSz;

                printf("\n%08x %d: PES hdrSz %d", clkBase, tp.PID, pesHdrSz);
                if (pes.PTS_DTS_flags) {
                    dts = (pes.PTS_DTS_flags & 1)? pes.DTS[0] : pes.PTS[0];
                    dts += ch.tbOffset;
                    printf(" DTS %08x->%08x", dts-ch.tbOffset, dts);
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
                    printf("\n\t\tSC %08x", ch.esBuf);

                    if (ch.hdrFlag) {
                        // Reading ES headers
                        if (temp == 1)  ch.hdrFlag = 0;
                    }
                    else {
                        if (temp==SEQ_START_CODE || temp==GOP_START_CODE
                            || temp==PIC_START_CODE) {
                            // Head of picture found
                            ch.hdrFlag = 1;
                            ch.picCnt++;
                            printf("\n\tPic %d: SC %02x, DTS %08x",
                                   ch.picCnt, temp, dts);
                        }
                    }
                }
            }
        }

        flushReader(&tsFile.rdr);
        tpCnt++;

        // Check EOF
        if (tsFile.rdr.errFlag==EOF) {
            printf("\nEOF reached\n");
            exit (0);
        }

        // Update system clock
        clkExt += pktTime;
        clkBase += clkExt/600;
        clkExt = clkExt % 600;
    }
}

