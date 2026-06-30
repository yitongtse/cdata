h37985
s 00004/00003/00694
d D 2.20 04/04/27 22:23:25 ytse 28 27
c backup
e
s 00004/00000/00693
d D 2.19 04/01/09 11:18:12 ytse 27 26
c Backup
e
s 00004/00002/00689
d D 2.18 03/03/21 14:19:24 ytse 26 25
c backup
e
s 00011/00008/00680
d D 2.17 02/05/24 12:26:36 ytse 25 24
c Update
e
s 00000/00140/00688
d D 2.16 02/03/27 12:55:52 ytse 24 23
c Clean up options
e
s 00042/00034/00786
d D 2.15 02/03/27 12:47:54 ytse 23 22
c Add parse only option
e
s 00041/00009/00779
d D 2.14 02/03/26 12:16:07 ytse 22 21
c Update
e
s 00007/00006/00781
d D 2.13 02/03/21 13:28:41 ytse 21 20
c Backup for NEW2
e
s 00012/00007/00775
d D 2.12 02/03/19 12:42:21 ytse 20 19
c update
e
s 00055/00024/00727
d D 2.11 02/03/07 16:40:44 ytse 19 18
c Update
e
s 00026/00021/00725
d D 2.10 02/03/04 15:38:24 ytse 18 17
c Update
e
s 00097/00054/00649
d D 2.9 02/02/28 13:41:56 ytse 17 16
c update
e
s 00054/00090/00649
d D 2.8 02/02/26 17:12:53 ytse 16 15
c Improved MBA logic
e
s 00045/00059/00694
d D 2.7 02/02/25 17:53:46 ytse 15 14
c Update
e
s 00081/00014/00672
d D 2.6 02/02/19 11:13:22 ytse 14 13
c Update
e
s 00026/00010/00660
d D 2.5 02/01/21 11:02:46 ytse 13 12
c Update
e
s 00044/00009/00626
d D 2.4 01/01/15 12:39:23 ytse 12 11
c update
e
s 00008/00001/00627
d D 2.3 00/08/21 11:36:55 ytse 11 10
c Continuous decoding after end of sequence code
e
s 00084/00013/00544
d D 2.2 00/08/21 11:23:55 ytse 10 9
c Added support of Windows
e
s 00000/00000/00557
d D 2.1 00/08/21 11:04:23 ytse 9 8
c Before supporting Windows
e
s 00002/00002/00555
d D 1.8 00/08/21 10:55:45 ytse 8 7
c backup
e
s 00009/00001/00548
d D 1.7 00/05/17 13:43:05 ytse 7 6
c Print PCRs
e
s 00009/00004/00540
d D 1.6 00/02/14 17:09:23 ytse 6 5
c Improved error handling.  Fixed a bug in displaying interlaced sequence
e
s 00002/00000/00542
d D 1.5 00/01/14 14:58:22 ytse 5 4
c Update
e
s 00019/00001/00523
d D 1.4 99/11/16 16:25:10 yitong 4 3
c Add debugging support
e
s 00016/00001/00508
d D 1.3 99/11/02 12:05:08 yitong 3 2
c Improve error report
e
s 00005/00000/00504
d D 1.2 99/11/01 16:15:34 yitong 2 1
c Backup
e
s 00504/00000/00000
d D 1.1 99/10/29 15:58:10 yitong 1 0
c date and time created 99/10/29 15:58:10 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: mpeg2dec.c

    MPEG-2 video decoder
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
I 18
#include <math.h>
E 18
I 10

I 14
D 15
#define	DUMP_QSCALE	1
E 15
I 15
D 18
#define	DUMP_QSCALE		0
E 18
I 18
D 19
#define	DUMP_QSCALE		1
E 18
#define	DUMP_COEF_MAP		0
I 17
D 18
#define	MB_MSE			1
E 18
I 18
#define	MB_MSE			0
E 19
I 19
D 28
#define FILENAME_SZ		128
E 28
I 28
#define FILENAME_SZ	128
#define STAT		0
E 28
E 19
E 18
E 17
E 15

I 22
D 24
#define	GET_FIRST_QS		1


E 24
E 22
E 14
D 16
/* Rui_B */
E 16
#include "def.h"

D 25
#ifdef UNIX_ENV
E 25
I 25
#ifdef UNIX
E 25
E 10
#include "util.h"
#include "param.h"
I 4
#include "block.h"
E 4
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
#include "display.h"
D 16

E 16
I 10
#else
#include "win.h"
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
E 10
D 16

E 16
I 10
#endif
D 16
/* Rui_E */
E 16
E 10

I 10

D 16


E 16
E 10
/* Note: The following variables are moved out from the main loop to here
         to avoid reserved memory problem.
*/
Reader *sysIn;         // system stream reader
Reader *vidIn;         // video elementary stream reader
PicInfo pic;
MbInfo mb;
RlInfo rl[6];          // Note: for 4:2:0 seq only

I 3
int tsFlag;
E 3
char comment[80];
I 15
int tpSz;
E 15
int tpCnt = 0;

InFile inFile;

D 23
int displayFlag;
E 23
I 23
int decodeFlag;
int displayFlag = 0;
E 23
int vidVerboseLev;

int pid;

/* For debugging purpose only */
int debugFlag;
int debugFrmNo;
int debugMba;
int debugTemp;
I 15
int dumpCoefFlag;
E 15
I 12
D 14
int prevDTS;     // previous DTS in 45 kHz form
E 14
I 14
int prevDTS;    // previous DTS in 45 kHz form
E 14
E 12

I 14
int totQs, totSlcQs, qsCnt;  // to compute average picture quant
int qsCnt = 0;
E 14

I 19
D 24
#if DUMP_QSCALE
int qsFlag;
FILE *qsFp;
#endif
E 24

E 19
I 15
D 24
#if DUMP_COEF_MAP
I 19
int coefMapFlag;
E 19
FILE *coefMapFp;
#endif
E 15
I 14

I 22
#if GET_FIRST_QS
int fstMbInSlc;
int totFstQs;
int fstQsCnt;
#endif
E 22
I 15

I 22

E 22
I 17
#if MB_MSE
void computeMbTse(uchar* buf1, uchar* buf2, int width, int height)
{
    int k, x, y, offset, tse;

    for (k=y=0; y<height; y+=16)
        for (x=0; x<width; x+=16, k++) {
            offset = y * width + x;
            tse = blockTse(buf1+offset, buf2+offset, 16, 16, width);
            if (tse) {
                printf("\nMB %d: %d", k, tse);
#if 0
                printf("\nBlock 1:\n");
                printUcharBlock(buf1+offset, 16, 16, width);
                printf("\nBlock 2:\n");
                printUcharBlock(buf2+offset, 16, 16, width);
                return;
#endif
            }
        }
}
#endif


E 24
E 17
E 15
E 14
I 3
void reportStatus()
{
D 17
    if (tsFlag)  printf("TP %d  ", tpCnt);
    printf("ES Byte %d\n", ftell(inFile.fp));
E 17
I 17
    if (tsFlag)  printf("\nTP %d  ", tpCnt);
    printf("\nES Byte %d\n\n", ftell(inFile.fp));
E 17
}


E 3
static int getMorePesPayload()
{
I 12
D 13
static uint prevDts = 0;
E 13
I 13
D 15
static uint prevDts = 0;	// in 45 kHz
static uint prevDts2 = 0;	// in 90 kHz
E 13

E 15
I 15
    static uint prevDts = 0;	// in 45 kHz
E 15
E 12
    static int firstTime = 1;
D 20
    static int firstPesHdrFound = 0;
E 20
    TpInfo tp;
    PesInfo pes;
    while (1) {                 /* loop till next TP with right PID is found */
        if (firstTime) {
I 2
D 15
            if (findTp(inFile.fp, 1)) {
E 15
I 15
            if (findTp(inFile.fp, tpSz, 1)) {
E 15
D 17
                printf("Failed to find SYNC.\n");
E 17
I 17
                printf("\nFailed to find SYNC.");
E 17
                exit (-1);
            }
D 17
            printf("Sync at byte %d\n", ftell(inFile.fp));
E 17
I 17
            printf("\nSync at byte %d", ftell(inFile.fp));
E 17
E 2
            getNextWord(sysIn);
D 15
            seekTp(&tp, sysIn, 1);
E 15
I 15
            seekTp(&tp, sysIn, tpSz, 1);
E 15
            firstTime = 0;
        }
        else {
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
                    ++tpCnt, ftell(inFile.fp), vidIn->byteCnt);
            commentReader(sysIn, comment);
            if (getByte(sysIn, "sync") != TP_SYNC) {
D 17
                printf("Lost of TP SYNC!");
E 17
I 17
                printf("\nLost of TP SYNC!");
E 17
I 3
                reportStatus();
E 3
D 10
                exit(-1);
E 10
I 10
                exit(-7855);
E 10
            }
        }
        if (sysIn->errFlag == EOF) {
D 17
            printf("EOF reached.\n");
E 17
I 17
            printf("\nEOF reached.\n");
E 17
D 10
            exit(-1);
E 10
I 10
            exit(-2707);
E 10
        }
 
        /* Get transport packet header */
        getTpHdr(&tp, sysIn);
 
        if (tp.PID==pid) {
            /* Get adaptation field if needed */
D 7
            if (tp.adaptation_field_control & 2)  getTpAf(&tp, sysIn);
E 7
I 7
D 15
            if (tp.adaptation_field_control & 2) {
E 15
I 15
            if (tp.adaptation_field_control & 2)
E 15
                getTpAf(&tp, sysIn);
I 8
D 15
/*
E 8
                if (tp.PCR_flag)
                    printf("  PCR: base %01x%08x  ext %03x\n", tp.PCR[0]>>31,
                           (tp.PCR[0]<<1 | tp.PCR[1]>>9), tp.PCR[1] & 0x1FF);
I 8
*/
E 8
            }
E 15
E 7
 
            /* Get PES header if needed */
            if (tp.payload_unit_start_indicator) {
                if (getBits(sysIn, 24, "start code prefix") != 1)
D 17
                    printf("Expected start code prefix not found!  Ignored.\n");                getPesHdr(&pes, sysIn);
E 17
I 17
D 26
                    printf("\nExpected start code prefix not found!  Ignored.");                getPesHdr(&pes, sysIn);
E 26
I 26
                    printf("\nTP %d: Expected start code prefix not found!"\
                           "  Ignored.", tpCnt);
                getPesHdr(&pes, sysIn);
E 26
E 17
 
I 15
#if 1
E 15
                /* Print PTS/DTS if available */
                if (pes.PTS_DTS_flags & 2) {
D 12
                    printf("  PTS: %05x%04x\n",
E 12
I 12
D 15
#if 0
                    printf("\nPTS %05x%04x",
E 12
                           ((pes.PTS[1]<<14)|(pes.PTS[0]>>16)),
                           pes.PTS[0]&0xFFFF);
I 12
#else
D 13
                    printf("\nPTS %08x",
                           (uint)((pes.PTS[1]<<31) | (pes.PTS[0]>>1)));
E 13
I 13
                    printf("\n  PTS: %08x (45 kHz), %08x (90 kHz)",
                           (uint)((pes.PTS[1]<<31) | (pes.PTS[0]>>1)),
                           (uint)pes.PTS[0]);
E 13
#endif
E 15
I 15
                    printf("\n  PTS %01x%04x",
                        (pes.PTS[0]>>31), (pes.PTS[0]<<1)|(pes.PTS[1]&1));

E 15
E 12
                    if (pes.PTS_DTS_flags & 1)
D 12
                        printf("  DTS: %05x%04x\n",
E 12
I 12
D 15
#if 0
                        printf(", DTS %05x%04x",
E 12
                               ((pes.DTS[1]<<14)|(pes.DTS[0]>>16)),
                               pes.DTS[0]&0xFFFF);
I 12
#else
                        printf(", DTS %08x",
                               (uint)((pes.DTS[1]<<31) | (pes.DTS[0]>>1)));
#endif
E 15
I 15
                        printf(", DTS %01x%04x",
                            (pes.DTS[0]>>31), (pes.DTS[0]<<1)|(pes.DTS[1]&1));
E 15
E 12
                }
I 12

                /* Check for backward DTS */
                if (pes.PTS_DTS_flags & 2) {
D 15
                    uint dts = pes.PTS_DTS_flags==2?
                                   ((pes.PTS[1]<<31) | (pes.PTS[0]>>1))
                                   : ((pes.DTS[1]<<31) | (pes.DTS[0]>>1));
I 13
                    uint dts2 = pes.PTS_DTS_flags==2? pes.PTS[0] : pes.DTS[0];

E 15
I 15
                    uint dts = pes.PTS_DTS_flags==2? pes.PTS[0] : pes.DTS[0];
E 15
E 13
                    int diff = (int)(dts - prevDts);
D 13
                    printf("\n  dts %08x, prevDts %08x, diff %d",
E 13
I 13
D 15
                    printf("\n  45 kHz:  dts %08x, prevDts %08x, diff %d",
E 13
                           dts, prevDts, diff);
                    if (diff<0)  printf(", ## Backward DTS\n");
I 13

                    diff = (int)(dts2 - prevDts2);
                    printf("\n  90 kHz:  dts %08x, prevDts %08x, diff %d",
                           dts2, prevDts2, diff);
                    if (diff<0)  printf(", ## Backward DTS\n");

E 15
I 15
                    printf("\n  DTS diff (45 kHz) %d", diff);
                    if (diff<0)  printf(", ## Backward DTS");
E 15
E 13
                    prevDts = dts;
I 13
D 15
                    prevDts2 = dts2;
E 15
E 13
                }
D 15

                printf("\n");
E 15
E 12
            }
I 15
#endif
E 15
 
            /* Pass control back to ES parsing only if TP has payload */
            if (tp.adaptation_field_control & 1) {
                copyReader(sysIn, vidIn);
                flushReader(sysIn);
                return 0;
            }

            flushReader(sysIn);
        }
        else {
D 15
            skipTp(&tp, sysIn);
E 15
I 15
            skipTp(&tp, sysIn, tpSz);
E 15
        }
    }
}



/* Note: return first start code after picture data
   Assumed first slice start code already read
*/
static int decodePic(Reader *in, PicInfo *pic, MbInfo *mb, RlInfo rl[],
                     uchar *curFrm[], uchar *newRefFrm[], uchar *oldRefFrm[])
{
    int SC;
D 25
    int nextMba;
E 25
    int errCode;
D 16
    int mbaIncr = 0;
E 16
I 16
D 18
    int mbaRef = -1;
E 18
    int mbaIncr;
E 16
    uchar **fwdRefFrm, **bwdRefFrm;
I 14
    int nBlks, nAcCoefs, nAcBits;
D 22
    int bitPos = getBitPos(in);		// mark beginning of picture
E 22
I 22
    int bitPos = getReaderBitPos(in);	// mark beginning of picture
E 22
E 14

I 20
D 22
    printf("\n  TR %d", pic->temporal_reference);
E 22
I 22
//    printf("\n  TR %d", pic->temporal_reference);
E 22

E 20
I 14
D 24
#if DUMP_QSCALE
D 16
static FILE *qsFp;
static int dumpQsFlag = 0;
if (pic->frmNo == debugFrmNo) {
  qsFp = fopen("qsInfo", "w");
  dumpQsFlag = 1;
}
if (dumpQsFlag)
  fprintf(qsFp, "\nFrm %d\n", pic->frmNo);
E 16
I 16
D 19
    static FILE *qsFp;
    static int dumpQsFlag = 0;
    if (pic->frmNo == debugFrmNo) {
        qsFp = fopen("qsInfo", "w");
        dumpQsFlag = 1;
    }
    if (dumpQsFlag)
E 19
I 19
    if (qsFlag)
E 19
D 18
        fprintf(qsFp, "\nFrm %d\n", pic->frmNo);
E 18
I 18
D 21
        fprintf(qsFp, "PIC %d\n", pic->frmNo);
E 21
I 21
        fprintf(qsFp, "P %d\n", pic->frmNo);
E 21
E 18
E 16
#endif

E 24
D 16
totQs = totSlcQs = qsCnt = 0;

E 16
I 16
    totQs = totSlcQs = qsCnt = 0;
E 16
    nBlks = nAcCoefs = nAcBits = 0;
E 14
    pic->mbCnt = 0;

    /* Select reference frames */
    if (pic->picture_coding_type==B_PIC) {
        fwdRefFrm = oldRefFrm;  bwdRefFrm = newRefFrm;
    }
    else {	/* I or P picture */
        fwdRefFrm = newRefFrm;
    }

    /* Get first slice header */
    setReaderEcho(in, vidVerboseLev>=SLC, NULL);
    getSlcHdr(mb, in, 1);
I 22
    initSlc(pic, mb);
E 22
I 14
D 16
totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 16
I 16
    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 16

E 14
D 22
    initSlc(pic, mb);
E 22
I 22
D 24
#if GET_FIRST_QS
    fstQsCnt = totFstQs = 0;
    fstMbInSlc = 1;
#endif
E 22

I 22

E 24
E 22
    /* Get first MBA_increment */
    setReaderEcho(in, vidVerboseLev>=MB, NULL);
    mbaIncr = getMbaIncr(in, &SC);
D 16
    mb->index = -1 + mbaIncr;
    if (mbaIncr!=1) {
E 16
I 16
    mb->index = mb->mbaRef + mbaIncr;
    if (mb->index != 0) {
E 16
        /* Either mbaIncr!=1 or another start code found */
D 17
        printf("Error: Wrong first slice start position\n");
        printf("mbaIncr = %d\n", mbaIncr);
E 17
I 17
        printf("\nError: Wrong first slice start position %d", mbaIncr);
E 17
I 3
        reportStatus();
E 3
D 10
        exit(-1);
E 10
I 10
        exit(-937);
E 10
    }

    while (1) {
        if (mb->index>=pic->nMb) {
D 17
            printf("Error: Too many macroblocks in picture %d!\n",
E 17
I 17
            printf("\nError: Too many macroblocks in picture %d!",
E 17
                   pic->frmNo);
D 17
            printf("  MB index: %d    # MBs in picture: %d\n",
E 17
I 17
            printf("\n  MB index: %d    # MBs in picture: %d",
E 17
                   mb->index, pic->nMb);
I 3
            reportStatus();
E 3
D 10
            exit(-1);
E 10
I 10
            exit(-18);
E 10
        }

        /* Display second field */
        /* Note: assumes frame picture */
I 10
D 16
		/* Rui_B */
E 16
D 25
#ifdef UNIX_ENV
E 25
I 25
#ifdef UNIX
E 25
E 10
D 6
        if (displayFlag && mb->index==(pic->nMb>>1))	/* half picture */
E 6
I 6
        if (displayFlag && !pic->progressive_sequence
D 25
            && mb->index==(pic->nMb>>1))	/* half picture */
E 25
I 25
            && mb->index==(pic->nMb>>1)) {	/* half picture */
E 25
E 6
            display_second_field();
I 25
//printf("\n2nd field");
//getchar();
        }
E 25
I 10
#endif
D 16
		/* Rui_E */
E 16
E 10

        initMb(pic, mb);

        debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);
D 17
        if (debugFlag)
            printf("\nFrm %d, MB %d:\n", debugFrmNo, debugMba);
E 17
I 17
        if (debugFlag) {
            printf("\nFrm %d, MB %d:", debugFrmNo, debugMba);
            printf("\n  PMV: (%d,%d), (%d,%d), (%d,%d), (%d,%d)",
                   mb->pmv[0][0].x, mb->pmv[0][0].y,
                   mb->pmv[0][1].x, mb->pmv[0][1].y,
                   mb->pmv[1][0].x, mb->pmv[1][0].y,
                   mb->pmv[1][1].x, mb->pmv[1][1].y);
        }
E 17

        if (mbaIncr==1) {	/* Process coded macroblock */
            /* Read macroblock header (after mba_incr field) and coefficients */
D 14
            errCode = getMb(in, pic, mb, rl);
E 14
I 14
            errCode = getMb(in, pic, mb, rl, &nBlks, &nAcCoefs, &nAcBits);
E 14
            if (errCode != -1) {
D 17
                printf("Error when decoding MB %d block %d.\n",
E 17
I 17
                printf("\nError when decoding MB %d block %d.",
E 17
                       mb->index, errCode);
I 3
                reportStatus();
E 3
D 10
                exit(-1);
E 10
I 10
                exit(-32);
E 10
            }

I 14
D 24
#if DUMP_QSCALE
D 16
// Dump QSCALE info for rate control debugging
  if (dumpQsFlag) {
    int quantScale = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
    int temp = pic->picture_coding_type==I_PIC? mb->type+1 :
                   (pic->picture_coding_type==P_PIC? mb->type+3 : mb->type+10);
    fprintf(qsFp, "%14s %3d    \n", MB_TYPE[temp], quantScale);
  }
E 16
I 16
            // Dump QSCALE info for rate control debugging
D 19
            if (dumpQsFlag) {
E 19
I 19
            if (qsFlag) {
E 19
                int quantScale = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
                int temp = pic->picture_coding_type==I_PIC? mb->type+1 :
D 18
                           (pic->picture_coding_type==P_PIC? mb->type+3 : mb->type+10);
                fprintf(qsFp, "%14s %3d    \n", MB_TYPE[temp], quantScale);
E 18
I 18
                               (pic->picture_coding_type==P_PIC? mb->type+3 :
                               mb->type+10);
D 20
                fprintf(qsFp, "MB %3d %3d %3d    \n", mb->index,
E 20
I 20
D 21
                fprintf(qsFp, "MB %d %d %d\n", mb->index,
E 20
                        mb->quantiser_scale_code, quantScale);
E 21
I 21
                fprintf(qsFp, "M %d %d %d %d\n", mb->index,
                        mb->quantiser_scale_code, quantScale, vidIn->byteCnt);
E 21
E 18
            }
E 16
#endif

E 24
D 16
// Compute average quant
if (mb->pattern || mb->intra) {
    qsCnt++;
    totQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
}
E 16
I 16
            // Compute average quant
            if (mb->pattern || mb->intra) {
                qsCnt++;
D 22
                totQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 22
I 22
                totQs += quantScaleTab[pic->q_scale_type] \
                                      [mb->quantiser_scale_code-1];
D 24
#if GET_FIRST_QS
                if (fstMbInSlc) {
                    totFstQs += quantScaleTab[pic->q_scale_type] \
                                             [mb->quantiser_scale_code-1];
                    fstQsCnt++;
                    fstMbInSlc = 0;
                }
#endif
E 24
E 22
            }
E 16

I 22

E 22
E 14
D 23
            /* Get motion prediction component */
D 4
            if (!mb->intra)
E 4
I 4
            if (!mb->intra) {
E 4
                getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
I 4
                if (debugFlag) {
D 17
                    printf("Prediction MB:\n");
E 17
I 17
                    printf("\nPrediction MB:\n");
E 17
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
E 23
I 23
            if (decodeFlag) {
                /* Get motion prediction component */
                if (!mb->intra) {
                    getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
                    if (debugFlag) {
                        printf("\nPrediction MB:\n");
                        printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                        16, 16, pic->nCol);
                    }
E 23
                }
D 23
            }
E 23
E 4

D 23
            if (mb->pattern || mb->intra) {
                /* Get DCT component */
D 20
                invQuantScanDct(pic, mb, rl, curBlk);
E 20
I 20
                invQuantScanDct(pic, mb, rl, curBlk, debugFlag);
E 23
I 23
                if (mb->pattern || mb->intra) {
                    /* Get DCT component */
                    invQuantScanDct(pic, mb, rl, curBlk, debugFlag);
E 23
E 20

D 23
                /* Combine two components */
                combineMb(pic, mb, curFrm, curBlk);
E 23
I 23
                    /* Combine two components */
                    combineMb(pic, mb, curFrm, curBlk);
E 23
I 4

D 23
                if (debugFlag) {
                    printf("Reconstructed MB:\n");
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
I 17
                    printf("\n  PMV: (%d,%d), (%d,%d), (%d,%d), (%d,%d)",
                        mb->pmv[0][0].x, mb->pmv[0][0].y,
                        mb->pmv[0][1].x, mb->pmv[0][1].y,
                        mb->pmv[1][0].x, mb->pmv[1][0].y,
                        mb->pmv[1][1].x, mb->pmv[1][1].y);
E 23
I 23
                    if (debugFlag) {
                        printf("Reconstructed MB:\n");
                        printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                        16, 16, pic->nCol);
                        printf("\n  PMV: (%d,%d), (%d,%d), (%d,%d), (%d,%d)",
                            mb->pmv[0][0].x, mb->pmv[0][0].y,
                            mb->pmv[0][1].x, mb->pmv[0][1].y,
                            mb->pmv[1][0].x, mb->pmv[1][0].y,
                            mb->pmv[1][1].x, mb->pmv[1][1].y);
                    }
E 23
E 17
                }
E 4
D 23
            }  
E 23
I 23
            }    // decodeFlag
        }    // mbaIncr==1
E 23

D 23
        }
E 23
        else if (mbaIncr>1) {	/* Process skipped macroblock */
I 15
D 24

#if DUMP_COEF_MAP
D 19
            int i;
            for (i=0; i<6; i++)
                fprintf(coefMapFp, "\n%d %d %d 00000:",
                        pic->frmNo, mb->index, i);
E 19
I 19
            if (coefMapFlag) {
                int i;
                for (i=0; i<6; i++)
                    fprintf(coefMapFp, "\n%d %d %d 00000:",
                            pic->frmNo, mb->index, i);
            }
E 19
#endif
E 24
E 15
            /* Resets for skipped macroblocks */
            if (pic->picture_coding_type==P_PIC) {
                resetMvPred(pic, mb);
                resetFwdMv(mb);
            }
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

D 23
            getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
I 4
            if (debugFlag) {
D 17
                printf("Prediction MB:\n");
E 17
I 17
                printf("\nPrediction MB:\n");
E 17
                printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                16, 16, pic->nCol);
E 23
I 23
            if (decodeFlag) {
                getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
                if (debugFlag) {
                    printf("\nPrediction MB:\n");
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
                }
E 23
            }
I 14
D 24

#if DUMP_QSCALE
D 16
// Dump QSCALE info for rate control debugging
  if (dumpQsFlag) {
    int quantScale = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
    fprintf(qsFp, "%14s %3d    \n", "SKIP", quantScale);
  }
E 16
I 16
            // Dump QSCALE info for rate control debugging
D 19
              if (dumpQsFlag) {
E 19
I 19
              if (qsFlag) {
E 19
                  int quantScale = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
D 18
                  fprintf(qsFp, "%14s %3d    \n", "SKIP", quantScale);
E 18
I 18
D 20
                  fprintf(qsFp, "MB %3d %3d %3d\n", mb->index,
E 20
I 20
D 21
                  fprintf(qsFp, "MB %d %d %d\n", mb->index,
E 20
                          mb->quantiser_scale_code, quantScale);
E 21
I 21
                  fprintf(qsFp, "M %d %d %d %d\n", mb->index,
                      mb->quantiser_scale_code, quantScale, vidIn->byteCnt);
E 21
E 18
              }
E 16
#endif
E 24
E 14
E 4
        }

D 14

E 14
        /* Update */
        pic->mbCnt++;
        mb->index++;
        mbaIncr--;

        if (!mbaIncr) {
            mbaIncr = getMbaIncr(in, &SC);	/* get next MBA_increment */

            if (!mbaIncr) {		/* start code found */

                if (SC>=MIN_SLICE_START_CODE && SC<=MAX_SLICE_START_CODE) {
                    /* Slice start code found here */
                    setReaderEcho(in, vidVerboseLev>=SLC, NULL);
                    getSlcHdr(mb, in, SC);
I 14
D 16
totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 16
E 14
                    initSlc(pic, mb);
I 16
D 22
                    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 22
I 22
                    totSlcQs += quantScaleTab[pic->q_scale_type]\
                                             [mb->quantiser_scale_code-1];
D 24
#if GET_FIRST_QS
                    fstMbInSlc = 1;
#endif
E 24
E 22
E 16

                    /* Get next MBA_incr */
                    setReaderEcho(in, vidVerboseLev>=MB, NULL);
                    mbaIncr = getMbaIncr(in, &SC);
                    if (mbaIncr==-1) {
D 17
                        printf("Error: Illegal MBA increment found\n");
E 17
I 17
                        printf("\nError: Illegal MBA increment found");
E 17
I 3
                        reportStatus();
E 3
D 10
                        exit(-1);
E 10
I 10
                        exit(-346);
E 10
                    }

                    /* Compute and check slice start position */
D 16
                    if ((mb->slice_vertical_position-1)*
                        pic->nMbCol-1+mbaIncr != mb->index) {
E 16
I 16
                    if (mb->mbaRef + mbaIncr != mb->index) {
E 16
D 17
                        printf("Error: Wrong slice start position\n");
D 16
                        printf("slice vert pos: %d,  mbaIncr: %d\n",
                               mb->slice_vertical_position, mbaIncr);
                        printf("slice start at %d, expected at %d\n",
                               (mb->slice_vertical_position-1)
                               * pic->nMbCol-1+mbaIncr, mb->index);
E 16
I 16
                        printf("Slice start at %d (%d, %d), expected at %d\n",
                               mb->mbaRef+mbaIncr, mb->slice_vertical_position,
                               mbaIncr, mb->index);
E 17
I 17
                        printf("\nError: Wrong slice start position: "\
                               "%d (%d, %d), expected %d", mb->mbaRef+mbaIncr,
                               mb->slice_vertical_position, mbaIncr, mb->index);
E 17
E 16
I 3
                        reportStatus();
E 3
D 10
                        exit(-1);
E 10
I 10
                        exit(-721);
E 10
                    }
                }

                else if (pic->mbCnt<pic->nMb) {
						/* non-slice start code */
D 17
                    printf("Error: unexpected start code 0x000001%02x within "\
                           "picture\n", SC);
                    printf("  current MB: %d\n", pic->mbCnt);
E 17
I 17
                    printf("\nError: unexpected start code 0x000001%02x "\
                           "within picture", SC);
                    printf("\n  current MB: %d", pic->mbCnt);
E 17
                }
I 17

E 17
                else switch (SC) {
                    case SEQ_START_CODE:
I 14
D 18
#if DUMP_QSCALE
D 16
if (dumpQsFlag) {
  fflush(qsFp);
  fclose(qsFp);
  dumpQsFlag = 0;
}
E 16
I 16
                         if (dumpQsFlag) {
                             fflush(qsFp);
                             fclose(qsFp);
                             dumpQsFlag = 0;
                         }
E 16
#endif
E 18
E 14
                    case GOP_START_CODE:
                    case PIC_START_CODE:
                    case SEQ_END_CODE:
I 14
                       {
D 22
                         int totBits = getBitPos(in) - bitPos;
E 22
I 22
                         int totBits = getReaderBitPos(in) - bitPos;
I 27
D 28
#if 0
E 28
I 28
#if STAT
E 28
E 27
E 22
                         printf("\n  Total: %d bits, %d blks, AC: %d%c, %2.1f/blk",
                             totBits, nBlks, nAcBits*100/totBits, '%',
D 15
                             (float)nAcCoefs/nBlks);
E 15
I 15
                             (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
I 27
#endif
E 27
E 15
E 14
                         return SC;
I 14
                       }
E 14

                    default:
D 17
                         printf("Error: Unexpected start code 0x000001%02x at "\
                                "end of picture\n", SC);
E 17
I 17
                         printf("\nError: Unexpected start code 0x000001%02x "\
                                "at end of picture\n", SC);
E 17
I 6
                         return SC;
E 6
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    Param par;
D 19
    char bitstreamFile[128];    // bitstream filename
    char outSeqName[128];       // output sequence filename
    char origSeqName[128];      // original sequence filename
    char sysSyntaxFile[128];    // system syntax filename
    char vidSyntaxFile[128];    // video syntax filename
E 19
I 19
    char bitstreamFile[FILENAME_SZ];    // bitstream filename
    char outSeqName[FILENAME_SZ];       // output sequence filename
    char origSeqName[FILENAME_SZ];      // original sequence filename
    char sysSyntaxFile[FILENAME_SZ];    // system syntax filename
    char vidSyntaxFile[FILENAME_SZ];    // video syntax filename
E 19
I 15
    int tpMode;
E 15
D 3
    int tsFlag;
E 3
D 23
    int mseFlag;
E 23
I 23
    int mseFlag = 0;
E 23
    int sysVerboseLev;

    uchar *curFrm[3];
    uchar *newRefFrm[3];
    uchar *oldRefFrm[3];
    uchar *origFrm[3];

    int state;
    int SC;			// start code
D 25
    int extId;			// extension ID
E 25
    int firstFrm;
D 14
    int bitMarker;		// for picture size calculation
E 14
I 14
    int bitMarker = 0;		// for picture size calculation
E 14

    long fileLoc;
    int skipBytes;

I 19
D 24
#if DUMP_QSCALE
    char qsFile[FILENAME_SZ];
#endif
D 20
#if DUMP_QSCALE
E 20
I 20
#if DUMP_COEF_MAP
E 20
    char coefFile[FILENAME_SZ];
#endif

E 24
E 19
I 10
D 12
	/* Rui_B */
	outSeqName[0] = '\0';
	/* Rui_E */
E 12
I 12
D 16
    /* Rui_B */
E 16
D 21
    outSeqName[0] = '\0';
E 21
I 21
    outSeqName[0] = 0;
E 21
D 16
    /* Rui_E */
E 16
E 12

D 16

E 16
E 10
    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2dec.par");
    commentParam(&par, "This program decode a MPEG-2 video bitstream");
    commentParam(&par, "");
    getStringParam(&par, "Bitstream filename", bitstreamFile, "test.mpg2");

    getIntParam(&par, "Transport bitstream?", &tsFlag, 0);
I 15
    getCondIntParam(&par, "TS?", tsFlag, "Packet size mode (1-188 2-204)",
        &tpMode, 1);
E 15
    getCondIntParam(&par, "TS?", tsFlag, "Video PID", &pid, 0);
    getCondIntParam(&par, "TS?", tsFlag, "System syntax verbose level",
        &sysVerboseLev, 0);
    getCondStringParam(&par, "TS?", tsFlag, "System syntax output file",
        sysSyntaxFile, "syntax.sys");

    getIntParam(&par, "Video syntax verbose level (0:OFF 1:SEQ 2:GOP 3:PIC "\
        "4:SLC 5:MB 6:ALL)", &vidVerboseLev, 0);
    getStringParam(&par, "Video syntax output file", vidSyntaxFile,
        "syntax.vid");
D 23
    getStringParam(&par, "Output sequence name", outSeqName, "");
E 23
    getIntParam(&par, "First frame number", &firstFrm, 0);
D 23
    getIntParam(&par, "Show output video?", &displayFlag, 0);
E 23
I 23
    getIntParam(&par, "Decode bitstream?", &decodeFlag, 1);
    getCondStringParam(&par, "Decode?", decodeFlag, "Output sequence name",
        outSeqName, "");
    getCondIntParam(&par, "Decode?", decodeFlag, "Show output video?",
        &displayFlag, 0);
E 23
    commentParam(&par, "");
    commentParam(&par, "Distortion calculation:");
D 23
    getIntParam(&par, "Compute MSE?", &mseFlag, 0);
E 23
I 23
    getCondIntParam(&par, "Decode?", decodeFlag, "Compute MSE?", &mseFlag, 0);
E 23
    getCondStringParam(&par, "MSE?", mseFlag, "Original sequence name",
        origSeqName, "");
    commentParam(&par, "");
    commentParam(&par, "Debugging parameters:");
    getIntParam(&par, "Frame number", &debugFrmNo, -1);
    getIntParam(&par, "Macroblock number", &debugMba, -1);
    getIntParam(&par, "No. of bytes to skip", &skipBytes, 0);
I 19
D 24
#if DUMP_QSCALE
    getIntParam(&par, "Extract QS info?", &qsFlag, 0);
    getCondStringParam(&par, "Extract QS?", qsFlag,
        "Output filename for QS info", qsFile, "test.qs");
#endif
#if DUMP_COEF_MAP
    getIntParam(&par, "Extract coef map?", &coefMapFlag, 0);
    getCondStringParam(&par, "Extract coef map?", coefMapFlag,
        "Output filename for coef map", coefMapFile, "test.coef");
#endif
E 24
E 19
    endParam(&par);

I 7
D 8
printf("output sequence: %s, len: %d", outSeqName, strlen(outSeqName));
E 7

E 8
D 10
debugTemp = vidVerboseLev;
E 10
I 10
D 20
	debugTemp = vidVerboseLev;
E 20
I 20
    debugTemp = vidVerboseLev;
E 20
E 10

I 21
D 24

E 21
I 19
#if DUMP_QSCALE
    if (qsFlag)
        if ((qsFp = fopen(qsFile, "wb")) == NULL) {
            printf("Error: Failed to open qs info file %s for output", qsFile);
            qsFlag = 0;
        }
#endif

E 19
I 15
#if DUMP_COEF_MAP
D 19
    coefMapFp = fopen("coefMap", "w");
E 19
I 19
    if (coefMapFlag)
        if ((coefMapFp = fopen(coefMapFile, "wb")) == NULL) {
            printf("Error: Failed to open coef map file %s for output",
                   coefMapFile);
            coefMapFlag = 0;
        }
E 19
#endif

E 24
    tpSz = (tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;

E 15
    /* MPEG-2 channel setup */
D 15
    openInFile(&inFile, bitstreamFile, TP_SIZE);
E 15
I 15
    openInFile(&inFile, bitstreamFile, tpSz);
E 15
    fseek(inFile.fp, skipBytes, SEEK_SET);
I 20
    printf("Input bitstream: %s (byte %d)", bitstreamFile, skipBytes);

E 20
    if (tsFlag) {
        /* Input is transport stream */
D 15
        uint* endPtr = inFile.buf + (inFile.bufSz>>2);
E 15
I 15
        uint* endPtr = inFile.buf + (TP_SIZE>>2);
E 15
        sysIn = &inFile.rdr;
        vidIn = (Reader*)malloc(sizeof(Reader));
        initReader(vidIn, getMorePesPayload, NULL);
        setBitBuf(&vidIn->bitBuf, endPtr, endPtr, 0);
D 10
        setReaderEcho(sysIn, sysVerboseLev, fopen(sysSyntaxFile, "w"));
E 10
I 10
D 16
	/* Rui: open using "wb" instead of "w" */
E 16
        setReaderEcho(sysIn, sysVerboseLev, fopen(sysSyntaxFile, "wb"));
I 20
        printf("PID: %d", pid);
E 20
E 10
    }
    else {
        /* Input is video elementary stream */
        vidIn = &inFile.rdr;
    }
D 10
    setReaderEcho(vidIn, 0, fopen(vidSyntaxFile, "w"));
E 10
I 10
D 16
	/* Rui: open using "wb" instead of "w" */
E 16
    setReaderEcho(vidIn, 0, fopen(vidSyntaxFile, "wb"));
E 10

    initVld();
    init_idct();
    initClipTab();

    /* To prevent memory deallocation */
    curFrm[Y] = curFrm[Cb] = curFrm[Cr] = 0;
    newRefFrm[Y] = newRefFrm[Cb] = newRefFrm[Cr] = 0;
    oldRefFrm[Y] = oldRefFrm[Cb] = oldRefFrm[Cr] = 0;
I 17
    if (mseFlag)
        origFrm[Y] = origFrm[Cb] = origFrm[Cr] = 0;
E 17

    pic.resetFlag = 1;
    pic.trBase = pic.nextTrBase = firstFrm;
I 10

D 16
	/* Rui_B */
	fileLoc = ftell(inFile.fp);
	/* Rui_E */
E 16
E 10
    seekSeqHdr(vidIn);
D 16

fileLoc = ftell(inFile.fp);

E 16
I 16
    fileLoc = ftell(inFile.fp);
E 16
    state = SEQ;

D 16

E 16
    while (1) {
        switch (state) {
            case SEQ:
I 5
D 12
                printf("SEQ Header found\n");
E 12
I 12
D 17
                printf("\n\nSEQ Header found\n");
E 17
I 17
D 22
                printf("\n\nSEQ Header found");
E 22
I 22
                printf("\n\n\nSEQ Header found");
E 22
E 17
E 12
E 5
                setReaderEcho(vidIn, vidVerboseLev>=SEQ, NULL);
                if (getSeqLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }

                /* One time initialization */
                if (pic.resetFlag) {
D 17
                    printf("Initialize buffers...\n");
E 17
I 17
                    printf("\nInitialize buffers...");
E 17
                    initBuffers(&pic, curFrm);
                    initBuffers(&pic, newRefFrm);
                    initBuffers(&pic, oldRefFrm);
                    if (displayFlag) {
I 10
D 16
						/* Rui_B */
E 16
D 25
#ifdef UNIX_ENV
E 25
I 25
#ifdef UNIX
E 25
E 10
                        init_display("", &pic);
                        init_dither();
I 10
#else
D 16
						initDisplay (pic.nCol, pic.nRow);
E 16
I 16
			initDisplay (pic.nCol, pic.nRow);
E 16
#endif
D 16
						/* Rui_E */
E 16
E 10
                    }
                    if (mseFlag)
                        initBuffers(&pic, origFrm);

                    pic.resetFlag = 0;
                }

                /* Find next state */
                switch (SC) {
                    case GOP_START_CODE: state = GOP;  break;
                    case PIC_START_CODE: state = PIC;  break;
                    default:
D 17
                        printf("GOP or picture header expected!\n");
E 17
I 17
                        printf("\nGOP or picture header expected!");
E 17
                        seekSeqHdr(vidIn);	/* resync */
                }
                break;

            case GOP:
I 5
D 12
                printf("GOP Header found\n");
E 12
I 12
D 17
                printf("\n\nGOP Header found\n");
E 17
I 17
D 22
                printf("\n\nGOP Header found");
E 22
I 22
                printf("\nGOP Header found");
E 22
E 17
D 13
                printf("  TC %02d:%02d:%02d:%02d, drop %d\n\n",
                       pic.time_code_hours, pic.time_code_minutes,
                       pic.time_code_seconds, pic.time_code_pictures,
                       pic.drop_frame_flag);
E 13
E 12
E 5
                setReaderEcho(vidIn, vidVerboseLev>=GOP, NULL);
                if (getGopLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }
I 13
D 17
                printf("  TC %02d:%02d:%02d:%02d, drop %d\n\n",
E 17
I 17
D 22
                printf("\n  TC %02d:%02d:%02d:%02d, drop %d\n",
E 22
I 22
                printf("\n  TC %02d:%02d:%02d:%02d, drop %d",
E 22
E 17
                       pic.time_code_hours, pic.time_code_minutes,
                       pic.time_code_seconds, pic.time_code_pictures,
                       pic.drop_frame_flag);
E 13

                /* Find next state */
                if (SC==PIC_START_CODE)
                    state = PIC;
                else {
D 17
                    printf("GOP or picture header expected!\n");
E 17
I 17
                    printf("\nGOP or picture header expected!");
E 17
                    seekSeqHdr(vidIn);	/* resync */
                    state = SEQ;
                }
                break;

            case PIC:
                setReaderEcho(vidIn, vidVerboseLev>=PIC, NULL);
                if (getPicLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }

                if (SC!=1) {
D 17
                    printf("Error: Invalid slice start code 0x000001%02x "\
                           "found!\n", SC);
E 17
I 17
                    printf("\nError: Invalid slice start code 0x000001%02x "\
                           "found!", SC);
E 17
I 3
                    reportStatus();
E 3
D 10
                    exit(-1);
E 10
I 10
                    exit(-283);
E 10
                }

                if (pic.picture_structure!=FRAME) {
D 17
                    printf("Sorry!  Field picture not supported yet!\n");
E 17
I 17
                    printf("\nSorry!  Field picture not supported yet!");
E 17
I 3
                    reportStatus();
E 3
D 10
                    exit(-1);
E 10
I 10
                    exit(-267);
E 10
                }

D 12
                printf("\nDecoding %s %d...\n",
E 12
I 12
D 13
                printf("Decoding %s %d...\n",
E 12
                       PIC_TYPE[pic.picture_coding_type], pic.frmNo);
E 13
I 13
                sprintf(comment, "%s-%d", PIC_TYPE[pic.picture_coding_type],
                    pic.frmNo);
                commentReader(vidIn, "\nPic ");
                commentReader(vidIn, comment);
                commentReader(vidIn, "...");

D 14
                printf("Decoding %s...\n", comment);
E 13
D 12
                printf("File loc: %d\n", fileLoc);
E 12
I 12
                printf("  File loc %d, TP %d\n", fileLoc, tpCnt);
                printf("  Vbv_delay %04x", pic.vbv_delay);
                printf(", TR %d, TFF %d, RFF %d\n", pic.temporal_reference,
E 14
I 14
                printf("\n\nDecoding %s...", comment);
D 15
#if 0
E 15
D 20
                printf("\n  File loc %d, TP %d", fileLoc, tpCnt);
E 20
I 20
                printf("\n  File loc %d", fileLoc);
                if (tsFlag)  printf(", TP %d", tpCnt);
E 20
I 15
D 16
#if 0
E 15
                printf(", Vbv_delay %04x", pic.vbv_delay);
                printf(", TR %d, TFF %d, RFF %d", pic.temporal_reference,
E 14
                       pic.top_field_first, pic.repeat_first_field);
D 14
                printf("\n");
E 14
I 14
#endif
E 16
E 14
E 12

D 16
vidVerboseLev = pic.frmNo==debugFrmNo? 6 : debugTemp;
E 16
I 16
                vidVerboseLev = pic.frmNo==debugFrmNo? 6 : debugTemp;
E 16

                /* Decode a picture */
                SC = decodePic(vidIn, &pic, &mb, rl, curFrm, newRefFrm,
                               oldRefFrm);
                fileLoc = ftell(inFile.fp);
D 14
/*
                fflush(in.echoFp);
                printf("%d bytes.\n", ch.in.byteCnt - bitMarker);
                bitMarker = ch.in.byteCnt;
*/
E 14
I 14

I 27
D 28
#if 0
E 28
I 28
#if STAT
E 28
E 27
D 15
#if 0
printf("\n  %s %4d : size %5d qs %2d slcQs %2d",
    PIC_TYPE[pic.picture_coding_type], pic.frmNo, vidIn->byteCnt - bitMarker,
    totQs/qsCnt, totSlcQs/pic.nMbRow);
#else
printf("\n  qs %d, slcQs %d", totQs/qsCnt, totSlcQs/pic.nMbRow);
#endif
E 15
I 15
D 19
                printf("\n  qs %d, slcQs %d", (qsCnt? totQs/qsCnt : 0),
                       totSlcQs/pic.nMbRow);
E 19
I 19
                printf("\n  qs %.1f, slcQs %.1f",
                    (qsCnt? (float)totQs/qsCnt : 0.),
                    (float)totSlcQs/pic.nMbRow);
I 27
#endif
E 27
I 22
D 24
#if GET_FIRST_QS
                printf("\n  1st MB qs %.1f, cnt %d",
                    (fstQsCnt? (float)totFstQs/fstQsCnt : 0.), fstQsCnt);
#endif
E 24
E 22
E 19
E 15

                bitMarker = vidIn->byteCnt;

E 14
I 7
                if (vidIn->echoFlag)
                    fflush(vidIn->echoFp);  // flush video syntax
E 7

I 17
                if (mseFlag) {
D 18
                    int mse, mseY, mseCb, mseCr, frmSz;
E 18
I 18
                    float mse, mseY, mseCb, mseCr;
D 26
                    float psnrY, psnrCb, psnrCr;
E 26
I 26
                    double psnrY, psnrCb, psnrCr;
E 26
                    int frmSz = pic.nCol * pic.nRow;
E 18

                    /* Compute MSE */
                    readFrm(origSeqName, &pic, origFrm);
D 18
                    frmSz = pic.nCol * pic.nRow;
E 18
                    mseY = blockTse(curFrm[Y], origFrm[Y], pic.nCol,
D 18
                                    pic.nRow, pic.nCol) / frmSz;
E 18
I 18
                                    pic.nRow, pic.nCol) / (float)frmSz;
E 18
                    mseCb = blockTse(curFrm[Cb], origFrm[Cb], pic.nCol>>1,
D 18
                                pic.nRow>>1, pic.nCol>>1) / (frmSz>>2);
E 18
I 18
                                pic.nRow>>1, pic.nCol>>1) / (float)(frmSz>>2);
E 18
                    mseCr = blockTse(curFrm[Cr], origFrm[Cr], pic.nCol>>1,
D 18
                                pic.nRow>>1, pic.nCol>>1) / (frmSz>>2);
E 18
I 18
                                pic.nRow>>1, pic.nCol>>1) / (float)(frmSz>>2);
E 18
                    mse = (mseY*4 + mseCb + mseCr) / 6;
D 18

                    printf("\n  MSE: Y %d, Cb %d, Cr %d, Tot %d",
E 18
I 18
                    printf("\n  MSE: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
E 18
                           mseY, mseCb, mseCr, mse);
I 18

                    // Compute PSNR in dB
                    //   PSNR = 10 x log_10(255^2 / mse)
                    //
                    psnrY = 48.1308 - 10 * log10(mseY);
                    psnrCb = 48.1308 - 10 * log10(mseCb);
                    psnrCr = 48.1308 - 10 * log10(mseCr);	
                    printf("\n  PSNR: Y %3.2f  Cb %3.2f  Cr %3.2f",
                           psnrY, psnrCb, psnrCr);
E 18
D 24
#if MB_MSE
                    if (mse) {
                        // Compute mse for each MB (Y only)
                        computeMbTse(curFrm[Y], origFrm[Y], pic.nCol, pic.nRow);
                    }
#endif
E 24
                }

E 17
                if (pic.picture_structure==FRAME || pic.secondFld) {
                    if (strlen(outSeqName))
                        saveFrm(outSeqName, &pic, curFrm);
                    if (pic.picture_coding_type!=B_PIC)
                        swapBuffers(curFrm, newRefFrm, oldRefFrm);
D 16
                    if (displayFlag)
D 10
                        dither(pic.picture_coding_type==B_PIC? curFrm:oldRefFrm, &pic);
E 10
I 10

E 16
I 16
                    if (displayFlag) {
E 16
D 25
#ifdef UNIX_ENV
E 25
I 25
#ifdef UNIX
E 25
                        dither(pic.picture_coding_type==B_PIC? curFrm:oldRefFrm,
                               &pic);
#else
D 16
						if(pic.picture_coding_type==B_PIC)
							Display_Image (curFrm[0], curFrm[2], curFrm[1]);
						else
							Display_Image (oldRefFrm[0], oldRefFrm[2], oldRefFrm[1]);

E 16
I 16
                        if (pic.picture_coding_type==B_PIC)
                            Display_Image(curFrm[0], curFrm[2], curFrm[1]);
                        else
                            Display_Image(oldRefFrm[0], oldRefFrm[2],
                                          oldRefFrm[1]);
E 16
#endif
I 25
//printf("\n1st field");
//getchar();
E 25
I 16
                    }
E 16
E 10
                }

D 17
                if (mseFlag) {
                    int mse, mseY, mseCb, mseCr, frmSz;

                    /* Compute MSE */
                    readFrm(outSeqName, &pic, origFrm);
                    frmSz = pic.nCol * pic.nRow;
                    mseY = blockTse(curFrm[Y], origFrm[Y], pic.nRow,
                                    pic.nCol, pic.nCol) / frmSz;
                    mseCb = blockTse(curFrm[Cb], origFrm[Cb], pic.nRow>>1,
                                pic.nCol>>1, pic.nCol>>1) / (frmSz>>2);
                    mseCr = blockTse(curFrm[Cr], origFrm[Cr], pic.nRow>>1,
                                pic.nCol>>1, pic.nCol>>1) / (frmSz>>2);
                    mse = (mseY*4 + mseCb + mseCr) / 6;

                    printf("MSE: Y-%d  Cb-%d  Cr-%d  Tot-%d\n",
                           mseY, mseCb, mseCr, mse);
                }

E 17
                /* Find next state */
                switch (SC) {
                    case SEQ_START_CODE: state = SEQ;  break;
                    case GOP_START_CODE: state = GOP;  break;
                    case PIC_START_CODE: state = PIC;  break;

                    case SEQ_END_CODE:
D 11
                        printf("End of sequence reached.\n");
E 11
I 11
D 17
                        printf("\nEnd of sequence reached!!\n\n");
E 17
I 17
                        printf("\n\nEnd of sequence reached!!\n");
E 17
D 16
#if 0
E 11
                        return 0;
I 11
#else
E 16
                        /* Continuous to look for next sequence start code */
                        seekSeqHdr(vidIn);
                        state = SEQ;
                        break;
D 16
#endif
E 16
E 11

                    default:
D 6
                        printf("Sequence, GOP or picture header expected!\n");
                        seekSeqHdr(vidIn);	/* resync */
                        state = SEQ;
E 6
I 6
                        while (1) {
                            SC = getNextStartCode(vidIn);
                            if (SC==PIC_START_CODE)  state = PIC;  break;
                            if (SC==GOP_START_CODE)  state = GOP;  break;
                            if (SC==SEQ_START_CODE)  state = SEQ;  break;
                        }
E 6
                }
                break;
        }
    }
I 10

D 16
	/* Rui_B */
    if (displayFlag) 
	{
E 16
I 16
    if (displayFlag) {
E 16
D 25
#ifdef UNIX_ENV
E 25
I 25
#ifdef UNIX
E 25
	    exit_display();
#else
D 16
		closeDisplay();
E 16
I 16
            closeDisplay();
E 16
#endif
D 16
	}

E 16
I 16
    }
E 16
E 10
}

E 1
