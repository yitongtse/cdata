h16990
s 00009/00005/00744
d D 1.16 03/03/21 14:19:38 ytse 16 15
c backup
e
s 00015/00008/00734
d D 1.15 02/06/10 17:42:19 ytse 15 14
c Update
e
s 00018/00000/00724
d D 1.14 02/06/05 14:26:18 ytse 14 13
c Update
e
s 00059/00027/00665
d D 1.13 02/06/05 13:58:54 ytse 13 12
c Various log files
e
s 00004/00007/00688
d D 1.12 02/06/05 12:44:51 ytse 12 11
c Fix bug due to fifo changes
e
s 00016/00010/00679
d D 1.11 02/06/05 12:12:08 ytse 11 10
c Update before fixing bug due to Fifo changes
e
s 00055/00001/00634
d D 1.10 02/05/28 14:47:43 ytse 10 9
c TC alignment
e
s 00013/00015/00622
d D 1.9 02/05/24 12:26:51 ytse 9 8
c Update
e
s 00040/00052/00597
d D 1.8 02/05/24 10:37:29 ytse 8 7
c Update
e
s 00073/00099/00576
d D 1.7 02/05/23 23:28:05 ytse 7 6
c Update
e
s 00059/00004/00616
d D 1.6 02/05/23 14:10:41 ytse 6 5
c Added VBV monitering
e
s 00003/00001/00617
d D 1.5 02/05/23 12:21:47 ytse 5 4
c Measure system header bits
e
s 00031/00026/00587
d D 1.4 02/05/22 17:51:14 ytse 4 3
c Backup
e
s 00018/00015/00595
d D 1.3 02/05/22 17:31:28 ytse 3 2
c Fix bitrate measurement
e
s 00144/00018/00466
d D 1.2 02/05/22 17:17:22 ytse 2 1
c Update
e
s 00484/00000/00000
d D 1.1 02/05/21 15:28:08 ytse 1 0
c date and time created 02/05/21 15:28:08 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: mpeg2parse.c

    MPEG-2 Parser
*****************************************************************************/
#ifndef UNIX
#include "win.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "def.h"
#include "util.h"
#include "param.h"
#include "block.h"
#include "imageio.h"
#include "stat.h"
#include "bitbuf.h"
#include "reader.h"
#include "writer.h"
#include "infile.h"
#include "vld.h"
#include "tp.h"
#include "pes.h"
#include "mpeg2.h"
#include "info.h"
#include "gethdr.h"
#include "getmb.h"
#include "decode.h"
I 6
#include "fifo.h"
E 6

#ifdef UNIX
#include "display.h"
#endif


I 3
D 7
#define MEASURE_BITRATE		1
I 6
#define MONITOR_VBV		1
E 7
E 6
E 3
#define FULL_DECODE		0
I 2
D 3
#define MEASURE_BITRATE		0
E 3
E 2
D 6
#define FILENAME_SZ		128
E 6
I 6
#define MAX_PICS		128
E 6


I 2
D 8
// Transport stream info
//
E 2
typedef struct {
I 2
  int bitrate;
  int tpSz;
  int tpCnt;
  int tpTime;			// packet time in 27 MHz
}
TsInfo;


E 8
// Access unit info
//
typedef struct {
D 3
  int seekPcr;
  int dtsFlag;			// whether DTS is present
  unsigned int tbOffset;	// timebase offset (in 45 kHz)
  unsigned int dts;		// input DTS (in 45 kHz)
  unsigned int localDts;	// DTS mapped to local clock (in 45 kHz)
E 3
I 3
D 7
  int  seekPcr;
  int  dtsFlag;			// whether DTS is present
  uint tbOffset;		// timebase offset (in 45 kHz)
D 4
  uint dts;		// input DTS (in 45 kHz)
  uint localDts;	// DTS mapped to local clock (in 45 kHz)
E 4
I 4
  uint dts;			// input DTS (in 45 kHz)
  uint localDts;		// DTS mapped to local clock (in 45 kHz)
E 4
E 3

#if MEASURE_BITRATE
D 3
  int prevTpCnt;		// TP count of previous PCR carrying TP
  int prevPcr;			// previous PCR
E 3
I 3
  int  prevTpCnt;		// TP count of previous PCR carrying TP
D 4
  uint prevPcr[2];			// previous PCR
E 4
I 4
  uint prevPcr[2];		// previous PCR
E 4
E 3
#endif
E 7
I 7
  int   picCnt;			// picture number (decode order)
D 8
  int   size;			// AU size in bits
E 8
I 8
  int   esSize;			// Elementary stream size of AU (in bits)
  int   size;			// total (system + ES) AU size (in bits)
E 8
  int   picType;		// picture type
  uint  dts;			// input DTS (in 45 kHz)
  uint  localDts;		// DTS mapped to local clock (in 45 kHz)
  uint  arvlTime;		// arrival time
  float avgQs;			// average quant scale
E 7
}
AuInfo;


// Decoder info
//
typedef struct {
E 2
D 4
  InFile inFile;	// input file
  Reader* sysIn;	// system stream reader
  Reader* vidIn;	// video elementary stream reader
I 2
  AuInfo* au;		// access unit info
E 2
  PicInfo pic;		// sequence, GOP and picture level info
  MbInfo  mb;		// slice and MB level info
  RlInfo  rl[6];	// runlength amplitude info
E 4
I 4
  InFile inFile;		// input file
  Reader* sysIn;		// system stream reader
  Reader* vidIn;		// video elementary stream reader
D 7
  AuInfo* au;			// access unit info
E 7
I 7
  AuInfo* inAu;			// access unit info of current input picture
  AuInfo* outAu;		// access unit info of next output picture
E 7
  PicInfo pic;			// sequence, GOP and picture level info
  MbInfo  mb;			// slice and MB level info
  RlInfo  rl[6];		// runlength amplitude info
E 4

D 4
  uchar*  curFrm[3];	// current frame buffers
  uchar*  newRefFrm[3];	// new reference frame buffers
  uchar*  oldRefFrm[3];	// old reference frame buffers
E 4
I 4
  uchar*  curFrm[3];		// current frame buffers
  uchar*  newRefFrm[3];		// new reference frame buffers
  uchar*  oldRefFrm[3];		// old reference frame buffers
E 4

D 4
  int     tsFlag;	// 0-ES, 1-TS
  int     tpSz;		// TP size
  int     pid;		// PID
  int     state;	// decoder state
  int     SC;		// next start code
I 2
  int     picCnt;	// picture count (in coding order)
E 4
I 4
  int     tsFlag;		// 0-ES, 1-TS
D 8
  int     tpSz;			// TP size
E 8
I 8
  int     tpSz;			// TP size (188 or 204)
E 8
  int     pid;			// PID
  int     state;		// decoder state
  int     SC;			// next start code
  int     picCnt;		// picture count (in coding order)
E 4
E 2
  int     firstTime;
I 2

  int     sysHdrBits;
  int     esHdrBits;
  int     picDataBits;
E 2
D 7
}
Decoder;
E 7

I 7
  // System layer info
D 8
  int     seekPcr;
E 8
I 8
  int     bitrate;		// transport stream bitrate
  int     tpCnt;		// number of TP processed so far
  int     tpTime;		// packet time in 27 MHz
  int     seekPcr;		// looking for first PCR?
E 8
  int     dtsFlag;		// whether DTS is present
  uint    dts;			// DTS (in 45 kHz)
  uint    localDts;		// DTS mapped to local clock (in 45 kHz)
  uint    tbOffset;		// timebase offset (in 45 kHz)
E 7

I 6
D 7
#if MONITOR_VBV
typedef struct {
    int  picCnt;
    uint dts;
    int  size;
E 7
I 7
  // For bitrate measurment
  int     prevTpCnt;		// TP count of previous PCR carrying TP
  uint    prevPcr[2];		// previous PCR
E 7
}
D 7
Stat;
#endif
E 7
I 7
Decoder;
E 7


E 6
// Global variables
I 13
char  bsFile[128];   		// bitstream filename
char  decLogFile[128];		// decoder log filename
char  statLogFile[128];		// picture stat log filename
char  vbvLogFile[128];		// VBV log filename
E 13
I 2
D 8
TsInfo  ts;
E 8
D 7
AuInfo  au;
E 7
I 7
AuInfo  au[MAX_PICS];
E 7
E 2
Decoder dec;
D 2
char comment[80];
int tpCnt = 0;
int vidVerboseLev = 0;
FILE* logFp;
int totQs, totSlcQs, qsCnt;
E 2
I 2
char    comment[80];
int     vidVerboseLev = 0;
D 13
FILE*   logFp;
E 13
I 13
FILE*   decLogFp;
FILE*   statLogFp;
FILE*   vbvLogFp;
E 13
int     totQs, totSlcQs, qsCnt;
I 6
uint    clkBase = 0;		// local time in 45 kHz
int     clkExt = 0;		// local time in 27 MHz
I 7
int     pesPlSz;
D 11
int     vbvLevel = 0;
E 11
I 11
int     vbvLevel;
I 14
int     vbvInterval;
int     vbvEnd = 0x7FFFFFFF;
int     vbvInSz;
int     vbvOutSz;
I 15
int     prevDts;		// DTS of previous picture
int     prevVbvLevel;
E 15
E 14
E 11
Fifo    auFifo;
E 7
E 6
E 2

I 2
D 3
unsigned int clkBase = 0;	// local time in 45 kHz
E 3
I 3
D 4
uint clkBase = 0;	// local time in 45 kHz
E 4
I 4
D 6
uint clkBase = 0;		// local time in 45 kHz
E 4
E 3
int  clkExt = 0;		// local time in 27 MHz
E 6
I 6
D 7
#if MONITOR_VBV
int    pesPlSz;
int    vbvLevel = 0;
Stat   stat[MAX_PICS];
Fifo   picStatFifo;
Stat*  outPicStat;
#endif
E 7
E 6
E 2

I 2
D 7

E 7
D 8
void initTs(TsInfo* ts)
E 8
I 8
void initDecoder(Decoder* dec)
E 8
{
D 8
    ts->tpTime = (double)27000000 * ts->tpSz * 8 / ts->bitrate;
E 8
I 8
    int i;

    dec->firstTime = 0;
    dec->pic.resetFlag = dec->seekPcr = 1;
    dec->tpCnt = 0;
    dec->pic.trBase = dec->pic.nextTrBase = dec->picCnt = 0;

    for (i=Y; i<Cr; i++) {
        dec->curFrm[i] = 0;
        dec->newRefFrm[i] = 0;
        dec->oldRefFrm[i] = 0;
    }

D 11
    dec->tpTime = (double)27000000 * dec->tpSz * 8 / dec->bitrate;
E 11
I 11
    dec->tpTime = (double)27000000 * TP_SIZE * 8 / dec->bitrate;
E 11
E 8
}


D 7
void initAuInfo(AuInfo* au)
E 7
I 7
void pcrUpdate(Decoder* dec, TpInfo* tp, uint localClk)
E 7
{
D 7
    au->seekPcr = 0;
}


D 3
void pcrUpdate(Decoder* dec, AuInfo* au, TpInfo* tp, unsigned int localClk)
E 3
I 3
void pcrUpdate(Decoder* dec, AuInfo* au, TpInfo* tp, uint localClk)
E 3
{
    au->seekPcr = 0;
    au->tbOffset = localClk - tp->PCR[0];

#if MEASURE_BITRATE
D 3
    if (tp->PCR[0] != au->prevPcr) {
        float estRate = ((float)(ts.tpCnt - au->prevTpCnt))
                  * ts.tpSz * 8 * 45000 / (tp->PCR[0] - au->prevPcr);
        if (abs(estRate - ts.bitrate) > 5000)
E 3
I 3
    if (tp->PCR[0] != au->prevPcr[0]) {
        int deltaPcr = (tp->PCR[0] - au->prevPcr[0]) * 600 +
                           (int)(tp->PCR[1] - au->prevPcr[1]);
        float estRate = 27000000. * 8 * ts.tpSz * (ts.tpCnt - au->prevTpCnt)
E 7
I 7
    if (dec->seekPcr) {
        dec->seekPcr = 0;
    }
    else {
        int deltaPcr = (tp->PCR[0] - dec->prevPcr[0]) * 600 +
                           (int)(tp->PCR[1] - dec->prevPcr[1]);
D 8
        float estRate = 27000000. * 8 * ts.tpSz * (ts.tpCnt - dec->prevTpCnt)
E 7
                        / deltaPcr;
D 5
        if (abs(estRate - ts.bitrate)/ts.bitrate > 0.000016)
E 5
I 5
        if (fabs(estRate - ts.bitrate)/ts.bitrate > 0.000016)
E 8
I 8
D 11
        float estRate = 27000000. * 8 * dec->tpSz
E 11
I 11
        float estRate = 27000000. * 8 * TP_SIZE
E 11
                            * (dec->tpCnt - dec->prevTpCnt) / deltaPcr;
        if (fabs(estRate - dec->bitrate)/dec->bitrate > 0.000016)
E 8
E 5
E 3
D 13
            printf("\nBitrate mismatch: Declared %d, Est %d bps, diff %d",
D 8
                   ts.bitrate, (int)estRate, (int)(estRate-ts.bitrate));
E 8
I 8
                dec->bitrate, (int)estRate, (int)(estRate - dec->bitrate));
E 13
I 13
D 15
            fprintf(statLogFp, "\nBitrate mismatch: Declared %d, Est %d bps, "\
E 15
I 15
            printf("\nBitrate mismatch: Declared %d, Est %d bps, "\
E 15
                "diff %d", dec->bitrate, (int)estRate,
                (int)(estRate - dec->bitrate));
E 13
E 8
D 7

D 3
        au->prevPcr = tp->PCR[0];
E 3
I 3
        au->prevPcr[0] = tp->PCR[0];
        au->prevPcr[1] = tp->PCR[1];
E 3
        au->prevTpCnt = ts.tpCnt;
E 7
    }
D 7
#endif
}
E 7

D 7

void mapDts(AuInfo* au, PesInfo* pes)
{
    au->dtsFlag = 1;
    au->dts = (pes->PTS_DTS_flags & 1)? pes->DTS[0] : pes->PTS[0];
    au->localDts = au->dts + au->tbOffset;
E 7
I 7
    dec->tbOffset = localClk - tp->PCR[0];
    dec->prevPcr[0] = tp->PCR[0];
    dec->prevPcr[1] = tp->PCR[1];
D 8
    dec->prevTpCnt = ts.tpCnt;
E 8
I 8
    dec->prevTpCnt = dec->tpCnt;
E 8
E 7
}


E 2
D 8
void initDecoder(Decoder* dec)
{
    int i;

    dec->pic.resetFlag = 1;
D 2
    dec->pic.trBase = dec->pic.nextTrBase = 0;
E 2
I 2
    dec->pic.trBase = dec->pic.nextTrBase = dec->picCnt = 0;
E 2

    for (i=Y; i<Cr; i++) {
        dec->curFrm[i] = 0;
        dec->newRefFrm[i] = 0;
        dec->oldRefFrm[i] = 0;
    }
}


E 8
void reportStatus(Decoder* dec)
{
D 2
    if (dec->tsFlag)  printf("\nTP %d  ", tpCnt);
E 2
I 2
D 8
    if (dec->tsFlag)  printf("\nTP %d  ", ts.tpCnt);
E 8
I 8
D 13
    if (dec->tsFlag)  printf("\nTP %d  ", dec->tpCnt);
E 8
E 2
    printf("\nES Byte %d\n\n", ftell(dec->inFile.fp));
E 13
I 13
    if (dec->tsFlag)  fprintf(decLogFp, "\nTP %d  ", dec->tpCnt);
    fprintf(decLogFp, "\nES Byte %d\n\n", ftell(dec->inFile.fp));
E 13
}


static int getMorePesPayload(Decoder* dec)
{
    TpInfo tp;
    PesInfo pes;

    while (1) {                 /* loop till next TP with right PID is found */
        if (dec->firstTime) {
I 2
D 8
            ts.tpCnt = 0;
E 8
E 2
            if (findTp(dec->inFile.fp, dec->tpSz, 1)) {
D 13
                printf("\nFailed to find SYNC.");
E 13
I 13
                fprintf(decLogFp, "\nFailed to find SYNC.");
E 13
                exit (-1);
            }
D 13
            printf("\nSync at byte %d", ftell(dec->inFile.fp));
E 13
I 13
            fprintf(decLogFp, "\nSync at byte %d", ftell(dec->inFile.fp));
E 13
            getNextWord(dec->sysIn);
            seekTp(&tp, dec->sysIn, dec->tpSz, 1);
            dec->firstTime = 0;
        }
        else {
I 2
D 8
            ts.tpCnt++;
E 8
I 8
            dec->tpCnt++;
E 8
E 2
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
D 2
                    ++tpCnt, ftell(dec->inFile.fp), dec->vidIn->byteCnt);
E 2
I 2
D 8
                    ts.tpCnt, ftell(dec->inFile.fp), dec->vidIn->byteCnt);
E 8
I 8
                    dec->tpCnt, ftell(dec->inFile.fp), dec->vidIn->byteCnt);
E 8
E 2
            commentReader(dec->sysIn, comment);
            if (getByte(dec->sysIn, "sync") != TP_SYNC) {
D 13
                printf("\nLost of TP SYNC!");
E 13
I 13
                fprintf(decLogFp, "\nLost of TP SYNC!");
E 13
                reportStatus(dec);
                exit(-7855);
            }
I 2

            // Update local clock
D 8
            clkExt += ts.tpTime;
E 8
I 8
            clkExt += dec->tpTime;
E 8
            clkBase += clkExt / 600;
            clkExt = clkExt % 600;
I 6
D 9

E 9
E 6
E 2
        }
I 2

E 2
        if (dec->sysIn->errFlag == EOF) {
D 13
            printf("\nEOF reached.\n");
E 13
I 13
D 15
            fprintf(statLogFp, "\nEOF reached.\n");
E 15
I 15
            printf("\nEOF reached.\n");
E 15
E 13
            exit(-2707);
        }
 
        /* Get transport packet header */
        getTpHdr(&tp, dec->sysIn);
 
        if (tp.PID == dec->pid) {
            /* Get adaptation field if needed */
D 2
            if (tp.adaptation_field_control & 2)
E 2
I 2
            if (tp.adaptation_field_control & 2) {
E 2
                getTpAf(&tp, dec->sysIn);
I 2

                // Check for PCR
                if (tp.PCR_flag)
D 7
                    pcrUpdate(dec, dec->au, &tp, clkBase);
E 7
I 7
                    pcrUpdate(dec, &tp, clkBase);
E 7
            }
I 5
            dec->sysHdrBits += tp.nbytes << 3;
I 6
            pesPlSz = TP_SIZE - tp.nbytes;
E 6
E 5
E 2
 
            /* Get PES header if needed */
            if (tp.payload_unit_start_indicator) {
                if (getBits(dec->sysIn, 24, "start code prefix") != 1)
D 2
                    printf("\nExpected start code prefix not found!  Ignored.");                getPesHdr(&pes, dec->sysIn);
E 2
I 2
D 13
                    printf("\nExpected start code prefix not found!  Ignored.");
E 13
I 13
                    fprintf(decLogFp, "\nExpected start code prefix not found!"\
                            "  Ignored.");
E 13
                getPesHdr(&pes, dec->sysIn);
I 5
                dec->sysHdrBits += (9 + pes.PES_header_data_length) << 3;
I 6
                pesPlSz -= 9 + pes.PES_header_data_length;
E 6
E 5

D 7
                if (pes.PTS_DTS_flags)
                    mapDts(dec->au, &pes);
E 7
I 7
                if (pes.PTS_DTS_flags) {
                    dec->dtsFlag = 1;
                    dec->dts = (pes.PTS_DTS_flags & 1)? pes.DTS[0] : pes.PTS[0];
                    dec->localDts = dec->dts + dec->tbOffset;
                }
E 7
E 2
            }
I 6

D 7
#if MONITOR_VBV
E 7
D 9
            {
                // Update VBV level
E 9
I 9
            // Update VBV level
D 16
            if (!(tp.adaptation_field_control & 1))  pesPlSz = 0;
E 16
I 16
            if (!(tp.adaptation_field_control & 1)) {
//                dec->sysHdrBits += pesPlSz << 3;
                pesPlSz = 0;
            }

E 16
            vbvLevel += pesPlSz << 3;
I 14
            vbvInSz += pesPlSz << 3;
E 14
            if (dec->picCnt && clkBase >= dec->outAu->localDts) {
I 15
                int rate = 5625. * (vbvLevel - prevVbvLevel)
                           / (dec->outAu->localDts - prevDts);
E 15
E 9
D 12
                int idx;
E 12
D 9
                if (!(tp.adaptation_field_control & 1))  pesPlSz = 0;
                vbvLevel += pesPlSz << 3;
D 7
                if (dec->picCnt && clkBase >= outPicStat->dts) {
                    vbvLevel -= outPicStat->size;
                    printf("\nVBV pic %d: time %08x, DTS %08x, vbv %d, %d pics",
                        outPicStat->picCnt, clkBase, outPicStat->dts, vbvLevel,
                        fifoFullness(&picStatFifo));
                    getFifo(&picStatFifo, &idx);
                    outPicStat = &stat[idx];
E 7
I 7
                if (dec->picCnt && clkBase >= dec->outAu->localDts) {
D 8
                    vbvLevel -= dec->outAu->size;
E 8
I 8
                    vbvLevel -= dec->outAu->esSize;
E 8
                    printf("\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x \t%d \t%d",
                        dec->pid, dec->outAu->picCnt, dec->outAu->picType,
                        dec->outAu->size>>3, dec->outAu->avgQs,
D 8
                        dec->outAu->localDts, dec->outAu->arvlTime, vbvLevel,
E 8
I 8
                        dec->outAu->localDts, dec->outAu->arvlTime, vbvLevel>>3,
E 8
                        fifoFullness(&auFifo));
                    getFifo(&auFifo, &idx);
                    dec->outAu = &au[idx];
E 7
                }
E 9
I 9
                vbvLevel -= dec->outAu->esSize;
I 14
                vbvOutSz += dec->outAu->esSize;
E 14
D 13
                printf("\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x \t%d \t%d",
                    dec->pid, dec->outAu->picCnt, dec->outAu->picType,
                    dec->outAu->size>>3, dec->outAu->avgQs,
E 13
I 13
                fprintf(statLogFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x "\
D 15
                    "\t%d \t%d", dec->pid, dec->outAu->picCnt,
E 15
I 15
                    "\t%d \t%d \t%d", dec->pid, dec->outAu->picCnt,
E 15
                    dec->outAu->picType, dec->outAu->size>>3, dec->outAu->avgQs,
E 13
                    dec->outAu->localDts, dec->outAu->arvlTime, vbvLevel>>3,
D 15
                    fifoFullness(&auFifo));
E 15
I 15
                    fifoFullness(&auFifo), rate);
E 15
D 11
                getFifo(&auFifo, &idx);
E 11
I 11
D 12
                idx = fifoDelete(&auFifo);
E 11
                dec->outAu = &au[idx];
E 12
I 12

I 15
                prevVbvLevel = vbvLevel;
                prevDts = dec->outAu->localDts;

E 15
                dec->outAu = &au[fifoDelete(&auFifo)];
E 12
E 9
            }
D 7
#endif
E 7
E 6
 
            /* Pass control back to ES parsing only if TP has payload */
            if (tp.adaptation_field_control & 1) {
                copyReader(dec->sysIn, dec->vidIn);
                flushReader(dec->sysIn);
                return 0;
            }

            flushReader(dec->sysIn);
        }
        else {
            skipTp(&tp, dec->sysIn, dec->tpSz);
        }
I 14

        if (clkBase >= vbvEnd) {
            fprintf(vbvLogFp, "\n%08x \t%d \t%d \t%d",
                    vbvEnd, vbvLevel>>3, vbvInSz>>3, vbvOutSz>>3);
            vbvEnd += vbvInterval;
            vbvInSz = vbvOutSz = 0;
        }
E 14
    }
}


/* Note: return first start code after picture data
   Assumed first slice start code already read
*/
static int decodePic(Decoder *dec, FILE* logFp)
{
    Reader* in = dec->vidIn;
    PicInfo* pic = &dec->pic;
    MbInfo* mb = &dec->mb;

    int errCode;
    int mbaIncr;
    uchar **fwdRefFrm, **bwdRefFrm;
    int nBlks, nAcCoefs, nAcBits;
    int bitPos = getReaderBitPos(in);	// mark beginning of picture

    totQs = totSlcQs = qsCnt = 0;
    nBlks = nAcCoefs = nAcBits = 0;
    pic->mbCnt = 0;

    /* Select reference frames */
    if (pic->picture_coding_type==B_PIC) {
        fwdRefFrm = dec->oldRefFrm;  bwdRefFrm = dec->newRefFrm;
    }
    else {	/* I or P picture */
        fwdRefFrm = dec->newRefFrm;
    }

    /* Get first slice header */
    setReaderEcho(in, vidVerboseLev>=SLC, NULL);
    getSlcHdr(mb, in, 1);
    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
    initSlc(pic, mb);

    /* Get first MBA_increment */
    setReaderEcho(in, vidVerboseLev>=MB, NULL);
    mbaIncr = getMbaIncr(in, &dec->SC);
    mb->index = mb->mbaRef + mbaIncr;
    if (mb->index != 0) {
        /* Either mbaIncr!=1 or another start code found */
        fprintf(logFp, "\nError: Wrong first slice start position %d", mbaIncr);
        reportStatus(dec);
        exit(-937);
    }

    while (1) {
        if (mb->index>=pic->nMb) {
            fprintf(logFp, "\nError: Too many macroblocks in picture %d!",
                   pic->frmNo);
            fprintf(logFp, "\n  MB index: %d    # MBs in picture: %d",
                   mb->index, pic->nMb);
            reportStatus(dec);
            exit(-18);
        }

        initMb(pic, mb);

        if (mbaIncr==1) {	/* Process coded macroblock */
            /* Read macroblock header (after mba_incr field) and coefficients */
            errCode = getMb(in, pic, mb, dec->rl, &nBlks, &nAcCoefs, &nAcBits);
            if (errCode != -1) {
                fprintf(logFp, "\nError when decoding MB %d block %d.",
                       mb->index, errCode);
                reportStatus(dec);
                exit(-32);
            }

            // Compute average quant scale
            if (mb->pattern || mb->intra) {
                qsCnt++;
                totQs += quantScaleTab[pic->q_scale_type]\
                                      [mb->quantiser_scale_code-1];
            }

#if FULL_DECODE
            /* Get motion prediction component */
            if (!mb->intra)
                getPredMb(pic, mb, dec->curFrm, fwdRefFrm, bwdRefFrm, 0);

            if (mb->pattern || mb->intra) {
                /* Get DCT component */
                invQuantScanDct(pic, mb, dec->rl, curBlk, 0);

                /* Combine two components */
                combineMb(pic, mb, dec->curFrm, curBlk);
            }  
#endif
        }
        else if (mbaIncr>1) {	/* Process skipped macroblock */
            /* Resets for skipped macroblocks */
            if (pic->picture_coding_type==P_PIC) {
                resetMvPred(pic, mb);
                resetFwdMv(mb);
            }
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

#if FULL_DECODE
            getPredMb(pic, mb, dec->curFrm, fwdRefFrm, bwdRefFrm, 0);
#endif
        }

        /* Update */
        pic->mbCnt++;
        mb->index++;
        mbaIncr--;

        if (!mbaIncr) {
            mbaIncr = getMbaIncr(in, &dec->SC);	/* get next MBA_increment */

            if (!mbaIncr) {		/* start code found */

                if (dec->SC>=MIN_SLICE_START_CODE
                        && dec->SC<=MAX_SLICE_START_CODE) {
                    /* Slice start code found here */
                    setReaderEcho(in, vidVerboseLev>=SLC, NULL);
                    getSlcHdr(mb, in, dec->SC);
                    totSlcQs += quantScaleTab[pic->q_scale_type]\
                                             [mb->quantiser_scale_code-1];
                    initSlc(pic, mb);

                    /* Get next MBA_incr */
                    setReaderEcho(in, vidVerboseLev>=MB, NULL);
                    mbaIncr = getMbaIncr(in, &dec->SC);
                    if (mbaIncr==-1) {
                        fprintf(logFp, "\nError: Illegal MBA increment found");
                        reportStatus(dec);
                        exit(-346);
                    }

                    /* Compute and check slice start position */
                    if (mb->mbaRef + mbaIncr != mb->index) {
                        fprintf(logFp, "\nError: Wrong slice start position: "\
                               "%d (%d, %d), expected %d", mb->mbaRef+mbaIncr,
                               mb->slice_vertical_position, mbaIncr, mb->index);
                        reportStatus(dec);
                        exit(-721);
                    }
                }

                else if (pic->mbCnt<pic->nMb) { /* non-slice start code */
                    fprintf(logFp, "\nError: unexpected start code "\
                        "0x000001%02x within picture", dec->SC);
                    fprintf(logFp, "\n  current MB: %d", pic->mbCnt);
                }

                else {
                    if (dec->SC==PIC_START_CODE || dec->SC==SEQ_START_CODE
                        || dec->SC==GOP_START_CODE || dec->SC==SEQ_END_CODE) {
D 2
                        int totBits = getReaderBitPos(in) - bitPos;
                        fprintf(logFp, "\n  Total: %d bits, %d blks, AC: %d%c,"\
                             " %2.1f/blk", totBits, nBlks, nAcBits*100/totBits,
                             '%', (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
E 2
I 2
                        int totBits;
                        dec->picDataBits = getReaderBitPos(in) - bitPos;
D 16
                        totBits = dec->esHdrBits + dec->picDataBits;
                        fprintf(logFp, "\n  Total: %d bits(%d, %d), %d blks, "\
			     "AC: %d%c, %2.1f/blk", totBits, dec->esHdrBits,
                             dec->picDataBits, nBlks,
E 16
I 16
                        totBits = dec->sysHdrBits + dec->esHdrBits + dec->picDataBits;
                        fprintf(logFp, "\n  Total: %d bits(%d, %d, %d), %d blks, "\
			     "AC: %d%c, %2.1f/blk", totBits, dec->sysHdrBits,
                             dec->esHdrBits, dec->picDataBits, nBlks,
E 16
                             nAcBits*100/dec->picDataBits, '%',
                             (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
E 2
                    }
                    return dec->SC;
                }
            }
        }
    }
}


int decodeNextPic(Decoder *dec, int verboseLev, FILE* logFp)
{
    long fileLoc = ftell(dec->inFile.fp);
I 2
    int bitPos = getReaderBitPos(dec->vidIn);
I 7
D 12
    int idx;
E 12
E 7
E 2

I 7
    // Get a AuInfo from FIFO for this input picture
D 11
    if (putFifo(&auFifo, &idx) == FIFO_OVERFLOW) {
E 11
I 11
D 12
    idx = fifoAdd(&auFifo);
E 12
I 12
    dec->inAu = &au[fifoAdd(&auFifo)];
E 12
    if (auFifo.status == FIFO_OVERFLOW) {
E 11
D 13
        printf("\nError: AU FIFO overflow!\n");
E 13
I 13
        fprintf(logFp, "\nError: AU FIFO overflow!\n");
E 13
        exit(-1);
    }
D 12
    dec->inAu = &au[idx];
    if (!dec->picCnt)  dec->outAu = dec->inAu;
E 12
I 12
    if (!dec->picCnt)  dec->outAu = &au[fifoDelete(&auFifo)];
E 12

E 7
I 2
    dec->sysHdrBits = dec->esHdrBits = dec->picDataBits = 0;

E 2
    /* Find next state based on next SC */
    switch (dec->SC) {
      case SEQ_START_CODE: dec->state = SEQ;  break;
      case GOP_START_CODE: dec->state = GOP;  break;
      case PIC_START_CODE: dec->state = PIC;  break;
      default: fprintf(logFp, "\nError: Unrecognized SC %d", dec->SC);
               return (-1);
    }

    while (1) {
        switch (dec->state) {
          case SEQ:
               fprintf(logFp, "\n\n\nSEQ Header found");
               setReaderEcho(dec->vidIn, verboseLev>=SEQ, NULL);
               if (getSeqLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }

               /* One time initialization */
               if (dec->pic.resetFlag) {
                   fprintf(logFp, "\nInitialize buffers...");
                   initBuffers(&dec->pic, dec->curFrm);
                   initBuffers(&dec->pic, dec->newRefFrm);
                   initBuffers(&dec->pic, dec->oldRefFrm);
                   dec->pic.resetFlag = 0;
               }

               /* Find next state */
               switch (dec->SC) {
                   case GOP_START_CODE: dec->state = GOP;  break;
                   case PIC_START_CODE: dec->state = PIC;  break;
                   default:
                       fprintf(logFp, "\nGOP or picture header expected!");
                       seekSeqHdr(dec->vidIn);	/* resync */
               }
               break;

          case GOP:
               fprintf(logFp, "\nGOP Header found");
               setReaderEcho(dec->vidIn, verboseLev>=GOP, NULL);
               if (getGopLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }
D 15
               fprintf(logFp, "\n  TC %02d:%02d:%02d:%02d",
E 15
I 15
               printf("\nTC %02d:%02d:%02d:%02d",
E 15
                   dec->pic.time_code_hours, dec->pic.time_code_minutes,
                   dec->pic.time_code_seconds, dec->pic.time_code_pictures);

               /* Find next state */
               if (dec->SC==PIC_START_CODE)
                   dec->state = PIC;
               else {
                   fprintf(logFp, "\nGOP or picture header expected!");
                   seekSeqHdr(dec->vidIn);	/* resync */
                   dec->state = SEQ;
               }
               break;

          case PIC:
I 10
               if (!dec->picCnt)
D 13
                   printf("\nTC %02d:%02d:%02d:%02d",
E 13
I 13
D 15
                   fprintf(logFp, "\nTC %02d:%02d:%02d:%02d",
E 15
I 15
                   fprintf(statLogFp, "\nTC %02d:%02d:%02d:%02d",
E 15
E 13
                       dec->pic.time_code_hours, dec->pic.time_code_minutes,
                       dec->pic.time_code_seconds, dec->pic.time_code_pictures);

E 10
               setReaderEcho(dec->vidIn, verboseLev>=PIC, NULL);
               if (getPicLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }
I 2
               dec->esHdrBits = getReaderBitPos(dec->vidIn) - bitPos;
E 2

               if (dec->SC!=1) {
                   fprintf(logFp, "\nError: Invalid slice start code "\
                       "0x000001%02x found!", dec->SC);
                   reportStatus(dec);
                   exit(-283);
               }

               if (dec->pic.picture_structure!=FRAME) {
                   fprintf(logFp, "\nSorry!  Field picture not supported yet!");
                   reportStatus(dec);
                   exit(-267);
               }

               sprintf(comment, "%s-%d",
                   PIC_TYPE[dec->pic.picture_coding_type], dec->pic.frmNo);
               commentReader(dec->vidIn, "\nPic ");
               commentReader(dec->vidIn, comment);
               commentReader(dec->vidIn, "...");

               fprintf(logFp, "\n\nDecoding %s...", comment);
               fprintf(logFp, "\n  File loc %d", fileLoc);
D 2
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", tpCnt);
E 2
I 2
D 8
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", ts.tpCnt);
E 8
I 8
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", dec->tpCnt);
E 8
E 2

I 7
               // Copy picture DTS to AuInfo
               dec->inAu->dts = dec->dts;
               dec->inAu->localDts = dec->localDts;

E 7
               /* Decode a picture */
               dec->SC = decodePic(dec, logFp);

               fprintf(logFp, "\n  qs %.1f, slcQs %.1f",
                   (qsCnt? (float)totQs/qsCnt : 0.),
                   (float)totSlcQs/dec->pic.nMbRow);

               if (dec->pic.picture_coding_type!=B_PIC)
                   swapBuffers(dec->curFrm, dec->newRefFrm, dec->oldRefFrm);

               return 0;
        }
    }
}


int main(int argc, char** argv)
{
    Param par;
D 6
    char  bsFile[FILENAME_SZ];   	// bitstream 1 filename
E 6
I 6
D 13
    char  bsFile[128];   	// bitstream filename
E 13
E 6
    int   skipBytes;
    int   tpMode;
I 10
    int   tcFlag;
    char  tc[12];
    int   tcVal, tcVal2;
    int   bitPos;
    int   frstHdrBits = 0;
E 10

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2parse.par");
    commentParam(&par, "This program compare two MPEG-2 video bitstreams");
    commentParam(&par, "");
    getStringParam(&par, "Filename", bsFile, "test.mpg");
    getIntParam(&par, "Byte offset", &skipBytes, 0);
I 2
D 8
    getIntParam(&par, "Bitrate (bps)", &ts.bitrate, 27000000);
E 8
I 8
    getIntParam(&par, "Bitrate (bps)", &dec.bitrate, 27000000);
E 8
E 2
    getIntParam(&par, "Transport bitstream?", &dec.tsFlag, 0);
    getCondIntParam(&par, "TS?", dec.tsFlag,
        "Packet size mode (1-188 2-204)", &tpMode, 1);
    getCondIntParam(&par, "TS?", dec.tsFlag, "Video PID", &dec.pid, 0);
I 14
    getIntParam(&par, "VBV monitor interval (ms)", &vbvInterval, 100);
E 14
I 10
    getIntParam(&par, "Sync to time code?", &tcFlag, 0);
    getCondStringParam(&par, "Sync to TC?", tcFlag, "Time code", tc,
        "hh:mm:ss:pp");
I 13
    getStringParam(&par, "Decoder log file", decLogFile, "");
    getStringParam(&par, "Picture statistic log file", statLogFile, "");
    getStringParam(&par, "VBV log file", vbvLogFile, "");
E 13
E 10
    endParam(&par);

I 13
    if (!strlen(decLogFile)) {
E 13
I 11
#ifdef UNIX
E 11
D 2
    logFp = stdout;
E 2
I 2
D 13
//    logFp = stdout;
    logFp = fopen("/dev/null", "wb");
E 13
I 13
        strcpy(decLogFile, "/dev/null");
E 13
I 11
#else
D 13
    logFp = fopen("dec.log", "wb");
E 13
I 13
        strcpy(decLogFile, "dec.log");
E 13
#endif
I 13
    }
    decLogFp = fopen(decLogFile, "wb");
E 13
E 11
E 2

I 13
    if (!strlen(statLogFile)) {
#ifdef UNIX
        strcpy(statLogFile, "/dev/null");
#else
        strcpy(statLogFile, "stat.log");
#endif
    }
    statLogFp = fopen(statLogFile, "wb");

    if (!strlen(vbvLogFile)) {
#ifdef UNIX
        strcpy(vbvLogFile, "/dev/null");
#else
        strcpy(vbvLogFile, "vbv.log");
#endif
    }
    vbvLogFp = fopen(vbvLogFile, "wb");


E 13
    /* MPEG-2 channel setup */
D 2
    dec.tpSz = (dec.tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;
E 2
I 2
D 8
    ts.tpSz = dec.tpSz = (dec.tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;
E 8
I 8
    dec.tpSz = (dec.tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;
E 8

E 2
    openInFile(&dec.inFile, bsFile, dec.tpSz);
    fseek(dec.inFile.fp, skipBytes, SEEK_SET);
D 4
    printf("\nInput bitstream %s (byte %d)", bsFile, skipBytes);
E 4
I 4
D 11
    printf("Input bitstream %s (byte %d)", bsFile, skipBytes);
I 8
    printf("\nPID \tPicNo \tPicType\tPicSz \tAvgQs \tDTS \t\tArrival "\
           "\tVbvLev \tVbvPic");
E 11
I 11
D 13
    printf("Input bitstream %s (byte %d), PID %d", bsFile, skipBytes, dec.pid);
E 13
I 13
    fprintf(statLogFp, "Input bitstream %s (byte %d), PID %d",
            bsFile, skipBytes, dec.pid);
E 13
E 11
E 8
E 4

    if (dec.tsFlag) {
        /* Input is transport stream */
        uint* endPtr = dec.inFile.buf + (TP_SIZE>>2);
        dec.sysIn = &dec.inFile.rdr;
        dec.vidIn = (Reader*)malloc(sizeof(Reader));
        initReader(dec.vidIn, getMorePesPayload, &dec);
        setBitBuf(&dec.vidIn->bitBuf, endPtr, endPtr, 0);
        setReaderEcho(dec.sysIn, 0, NULL);
D 8
        printf("\nPID %d", dec.pid);
E 8
    }
    else {
        /* Input is video elementary stream */
        dec.vidIn = &dec.inFile.rdr;
    }
    setReaderEcho(dec.vidIn, 0, NULL);

I 10
    if (tcFlag) {
        char tmp;
        int tcHr, tcMin, tcSec, tcPic;
        sscanf(tc, "%d %c %d %c %d %c %d", &tcHr, &tmp, &tcMin, &tmp, &tcSec,
               &tmp, &tcPic);
        tcVal = ((tcHr * 60 + tcMin) * 60 + tcSec) * 30 + tcPic;
D 13
        printf("\nSync to TC %s", tc);
E 13
I 13
        fprintf(statLogFp, "\nSync to TC %s", tc);
E 13
    }

E 10
I 2
D 8
    initTs(&ts);
E 8
D 7
    initAuInfo(&au);
E 7
E 2
    initDecoder(&dec);
I 2
D 7
    dec.au = &au;
E 7
E 2

I 6
D 7
#if MONITOR_VBV
    initFifo(&picStatFifo, MAX_PICS);
#endif
E 7
I 7
D 11
    initFifo(&auFifo, MAX_PICS);
E 11
I 11
    fifoInit(&auFifo, MAX_PICS);
E 11
E 7

E 6
    if (seekSeqHdr(dec.vidIn)) {
D 13
        printf("\nError: failed to find SEQ header\n");
E 13
I 13
        fprintf(statLogFp, "\nError: failed to find SEQ header\n");
E 13
        exit(-1);
    }
I 10

    if (tcFlag) {
        // Time code sync
        while (1) {
            bitPos = getReaderBitPos(dec.vidIn);
            getSeqLayer(dec.vidIn, &dec.pic, &dec.SC);
            if (dec.SC == GOP_START_CODE) {
                getGopLayer(dec.vidIn, &dec.pic, &dec.SC);
                tcVal2 = ((dec.pic.time_code_hours * 60 +
                           dec.pic.time_code_minutes) * 60 +
                           dec.pic.time_code_seconds) * 30 +
                           dec.pic.time_code_pictures;
                if (tcVal2 < tcVal) {
D 13
                    printf("\nTC %02d:%02d:%02d:%02d  skipped",
E 13
I 13
D 15
                    fprintf(statLogFp, "\nTC %02d:%02d:%02d:%02d  skipped",
E 15
I 15
                    printf("\nTC %02d:%02d:%02d:%02d  skipped",
E 15
E 13
                        dec.pic.time_code_hours, dec.pic.time_code_minutes,
                        dec.pic.time_code_seconds, dec.pic.time_code_pictures);
                    seekSeqHdr(dec.vidIn);
                    continue;
                }

                dec.SC = PIC_START_CODE;
                break;
            }
        }
        frstHdrBits = getReaderBitPos(dec.vidIn) - bitPos;
    }
    else
        dec.SC = SEQ_START_CODE;

D 11

E 11
E 10
I 9
    vbvLevel = 0;		// empty VBV
I 11
//    vbvLevel = dec.vidIn->bitBuf.endPtr - dec.vidIn->bitBuf.curPtr;
I 14
    vbvInterval *= 45;		// in 45 kHz ticks
    vbvEnd = vbvInterval;
    vbvInSz = vbvOutSz = 0;
E 14
E 11
E 9
D 10
    dec.SC = SEQ_START_CODE;
E 10

    initVld();
    init_idct();
    initClipTab();

I 11
D 13
    printf("\nPID \tPicNo \tPicType\tPicSz \tAvgQs \tDTS \t\tArrival "\
           "\tVbvLev \tVbvPic");
E 13
I 13
    fprintf(statLogFp, "\nPID \tPicNo \tPicType\tPicSz \tAvgQs \tDTS "
D 15
           "\t\tArrival \tVbvLev \tVbvPic");
E 15
I 15
           "\t\tArrival \tVbvLev \tVbvPic \tRate");
E 15
I 14
    fprintf(vbvLogFp, "\nTime    \tVbvLev \tVbvIn \tVbvOut");
E 14
E 13

E 11
D 2
    while (1)
E 2
I 2
    while (1) {
D 8
        int totalBits;
E 8
E 2
D 13
        decodeNextPic(&dec, 0, logFp);
E 13
I 13
        decodeNextPic(&dec, 0, decLogFp);
E 13
I 2
D 4

#if 1
E 4
D 8
        totalBits = dec.sysHdrBits + dec.esHdrBits + dec.picDataBits;
E 8
D 4
        printf("\nPID %d, Pic %d, Type %d, Sz %d, Qs %.1f, DTS %08x",
E 4
I 4

I 6
D 7
#if MONITOR_VBV
        {
            // Add picture statistics to FIFO
            int idx;
            putFifo(&picStatFifo, &idx);
            stat[idx].picCnt = dec.picCnt;
            stat[idx].dts = au.localDts;
            stat[idx].size = totalBits;
            if (!dec.picCnt)  outPicStat = &stat[idx];
        }
#endif
E 7
I 7
        if (!dec.dtsFlag)
D 13
            printf("\nPic %d: DTS missing!", dec.picCnt);
E 13
I 13
            fprintf(statLogFp, "\nPic %d: DTS missing!", dec.picCnt);
E 13
E 7

I 10
        if (!dec.picCnt)
            dec.esHdrBits += frstHdrBits;

E 10
E 6
D 7
#if 0
        printf("\nPID %d, Pic %d, Type %d, Sz %d, Qs %.1f, DTS %08x, arvl %08x",
E 4
            dec.pid, dec.picCnt, dec.pic.picture_coding_type, totalBits>>3,
D 4
            (qsCnt? (float)totQs/qsCnt : 0.), au.localDts);
E 4
I 4
            (qsCnt? (float)totQs/qsCnt : 0.), au.localDts, clkBase);
#else
        printf("\n%d\t%d\t%d\t%d\t%.1f\t%08x\t%08x",
            dec.pid, dec.picCnt, dec.pic.picture_coding_type, totalBits>>3,
            (qsCnt? (float)totQs/qsCnt : 0.), au.localDts, clkBase);
E 7
I 7
        // Copy picture statistic to input AuInfo
        dec.inAu->picCnt = dec.picCnt;
D 8
        dec.inAu->size = totalBits;
E 8
I 8
        dec.inAu->esSize = dec.esHdrBits + dec.picDataBits;
        dec.inAu->size = dec.inAu->esSize + dec.sysHdrBits;
E 8
        dec.inAu->picType = dec.pic.picture_coding_type;
        dec.inAu->arvlTime = clkBase;
        dec.inAu->avgQs = qsCnt? (float)totQs/qsCnt : 0.;
E 7

D 7
#endif
E 4
        if (!au.dtsFlag)
            printf("\t** DTS missing!");
D 4
#endif
E 4

E 7
        dec.picCnt++;
D 7
        au.dtsFlag = 0;
E 7
I 7
        dec.dtsFlag = 0;
E 7
    }
E 2
}

E 1
