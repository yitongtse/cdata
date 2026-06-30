h18695
s 00103/00016/00898
d D 1.6 02/05/24 12:26:29 ytse 6 5
c Update
e
s 00044/00005/00870
d D 1.5 02/03/21 13:28:41 ytse 5 4
c Backup for NEW2
e
s 00066/00006/00809
d D 1.4 02/03/19 14:23:43 ytse 4 3
c update
e
s 00001/00002/00814
d D 1.3 02/03/07 16:40:44 ytse 3 2
c Update
e
s 00187/00021/00629
d D 1.2 02/03/04 15:38:24 ytse 2 1
c Update
e
s 00650/00000/00000
d D 1.1 02/02/28 14:50:09 ytse 1 0
c date and time created 02/02/28 14:50:09 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: mpeg2rq.c

    MPEG-2 requantization
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
#include "outfile.h"
#include "vlc.h"
#include "vld.h"
#include "tp.h"
#include "pes.h"
#include "mpeg2.h"
#include "info.h"
#include "gethdr.h"
#include "puthdr.h"
#include "getmb.h"
#include "putmb.h"
#include "decode.h"


#define FULL_DECODE		0
I 6
#define CHECK_SKIPPED_B_MB	1
E 6
I 2
#define RC_FROM_FILE		1
I 6
#define RMX_REQUANT		0
E 6
E 2


/* Note: The following variables are moved out from the main loop to here
         to avoid reserved memory problem.
*/
Reader *sysIn;         // system stream reader
Reader *vidIn;         // video elementary stream reader
Writer *vidOut;	       // video elementary stream writer
PicInfo pic;
MbInfo mb;
RlInfo rl[6];          // Note: for 4:2:0 seq only

int tsFlag;
char comment[80];
int tpSz;
int tpCnt = 0;

InFile inFile;
OutFile outFile;

int vidVerboseLev;
int pid;

/* Additional data structures for recoding */
Coor pmv[2][2];
D 4
int dc_dct_pred[3];
I 2
int inQsc;		// input quantiser scale code
int outQsc;		// output quantiser scale code
E 4
I 4
int  dc_dct_pred[3];
int  inQsc;		// input quantiser scale code
int  outQsc;		// output quantiser scale code
D 5
int  qscAdj;
E 5
I 5
int  qscAdj, qscMinAdj, qscMaxAdj;
int  adjWin;
E 5
E 4
E 2

I 2
#if RC_FROM_FILE
D 4
char qsInfoFilename[256];
E 4
I 4
char  qsInfoFilename[256];
E 4
FILE* qsFp;
char  label[16];
D 5
int   rcFrmNo, rcMba, rcQsc, rcQs;
E 5
I 5
int   rcFrmNo, rcMba, rcQsc, rcQs, rcByteCnt;
int   rcMinDiff, rcMaxDiff;
int   rcOldQscTot, rcNewQscTot;
E 5
#endif
E 2

/* For debugging purpose only */
int debugFlag;
int debugFrmNo;
int debugMba;
int debugTemp;
int dumpCoefFlag;
int prevDTS;    // previous DTS in 45 kHz form

I 6
int inTotQs, inTotSlcQs, inQsCnt;
int outTotQs, outTotSlcQs, outQsCnt;
int inTotBits, outTotBits;
int skippedBMbCnt;
E 6

I 2

//// Requant code ////////////////////////////////////////////////////////////


static void rcPreMb(PicInfo *pic, MbInfo *mb)
{
#if RC_FROM_FILE
I 5
    int rcPrevQsc = rcQsc;
    int diff = vidOut->byteCnt - rcByteCnt;

E 5
    // Read quant info from data file
D 5
    fscanf(qsFp, "%s %d %d %d", label, &rcMba, &rcQsc, &rcQs);
E 5
I 5
    fscanf(qsFp, "%s %d %d %d %d", label, &rcMba, &rcQsc, &rcQs, &rcByteCnt);
E 5
    if (rcMba != mb->index) {
        printf("\nError: MBA from quant scale data file (%d) "\
               "not matching (%d)!\n", rcMba, mb->index);
        exit(-1);
    }
    outQsc = rcQsc;
#else
D 4
    outQsc = mb->quantiser_scale_code + 1;
    if (outQsc>31)  outQsc = 31;
E 4
I 4
    outQsc = mb->quantiser_scale_code;
E 4
#endif
I 4

I 5
D 6
    if (mb->index==0) {
        printf("\n  rcQs: %d->%d", rcOldQscTot, rcNewQscTot);
        printf("\n  inBits %d, outBits %d", rcByteCnt, vidOut->byteCnt);
E 6
I 6
    if (mb->index==0)
E 6
        rcOldQscTot = rcNewQscTot = 0;
D 6
    }
E 6

    if (diff < rcMinDiff)  rcMinDiff = diff;
    if (diff > rcMaxDiff)  rcMaxDiff = diff;

    if (rcQsc!=rcPrevQsc || !mb->pos.x) {
        // Recalculate QSC adjustment
        if (diff > adjWin)  qscAdj = 1;
        else if (diff >= 0) {
                 if (qscAdj==-1)  qscAdj = 0;
             }
        else if (diff >= -adjWin) {
                 if (qscAdj==1)  qscAdj = 0;
             }
        else  qscAdj = -1;
I 6
//        printf("\nMB %d: diff %d, qscAdj %d", mb->index, diff, qscAdj);
E 6
    }

    if (qscAdj < qscMinAdj)  qscAdj = qscMinAdj;
    else if (qscAdj > qscMaxAdj)  qscAdj = qscMaxAdj;

E 5
    // Adjustment
    outQsc += qscAdj;

    if (outQsc<1)  outQsc = 1;
    else if (outQsc>31)  outQsc = 31;
I 5

    rcOldQscTot += rcQsc;
    rcNewQscTot += outQsc;
E 5
E 4
}


static void rcPostMb(PicInfo *pic, MbInfo *mb)
{
}


I 4
D 6
#if 1
E 6
I 6
#if RMX_REQUANT
E 6

// OLD requant formula
E 4
D 6
static int requantRl(RlInfo* rl, int qs1, int qs2, int intraFlag)
E 6
I 6
static void requantRl(RlInfo* rl, int qs1, int qs2, int intraFlag)
E 6
{
    int i;			// input index
    int j;			// output index
    int level;
    int run = 0;

    // Requant variables
    int scale = 13;
    int half = 1 << (scale-1);
I 4
    int quarter = 1 << (scale-2);
E 4
    int r = (qs1<<scale) / qs2;

    i = j = intraFlag? 1 : 0;	// do not touch intra DC

    for ( ; i<rl->count; i++) {
        // Requantization formula
        level = r * abs(rl->level[i]);
        if (intraFlag) {	// Intra case
I 4
            level += quarter;
            level >>= scale;
        }
        else {			// Inter case
            level = (level<<1) + r + half;
            level >>= scale;
            if (level)  level = (level-1) >> 1;
        }
        if (rl->level[i] < 0)  level = -level;

        if (level) {
            rl->run[j] = run + rl->run[i];
            rl->level[j] = level;
            run = 0;
            j++;
        }
        else {
            run += rl->run[i] + 1;
        }
    }
    rl->count = j;
}

#else

// NEW requant formula
D 6
static int requantRl(RlInfo* rl, int qs1, int qs2, int intraFlag)
E 6
I 6
static void requantRl(RlInfo* rl, int qs1, int qs2, int intraFlag)
E 6
{
    int i;			// input index
    int j;			// output index
    int level;
    int run = 0;

    // Requant variables
    int scale = 13;
    int half = 1 << (scale-1);
    int r = (qs1<<scale) / qs2;

    i = j = intraFlag? 1 : 0;	// do not touch intra DC

    for ( ; i<rl->count; i++) {
        // Requantization formula
        level = r * abs(rl->level[i]);
        if (intraFlag) {	// Intra case
E 4
            level += half;
I 4
            if (level)  level--;
E 4
            level >>= scale;
        }
        else {			// Inter case
            level = (level<<1) + r;
I 4
            if (level)  level--;
E 4
            level >>= scale + 1;
        }
        if (rl->level[i] < 0)  level = -level;

        if (level) {
            rl->run[j] = run + rl->run[i];
            rl->level[j] = level;
            run = 0;
            j++;
        }
        else {
            run += rl->run[i] + 1;
        }
    }
    rl->count = j;
}

I 4
#endif
E 4

I 4

E 4
// Recode a macroblock
//   - Returns 1 if MB is coded
//
static int recodeMb(PicInfo *pic, MbInfo *mb, RlInfo rl[],
                    int qsOld, int qsNew, int* slcFlag)
{
    static int prevQsc = -1;	// previous quantiser_scale_code
I 6
    static int prev_motion_type = 0;
    static int prev_motion_forward;
    static int prev_motion_backward;
E 6

    int i;
    int flag;
    int cbpMask = 1 << 11;
    *slcFlag = !mb->pos.x;

    for (i=0; i<6; i++) {
        if (mb->intra || (mb->cbp & cbpMask)) {
            requantRl(&rl[i], qsOld, qsNew, mb->intra);
            if (!rl[i].count)  mb->cbp ^= cbpMask;
        }
        cbpMask >>= 1;
    }

    if (!mb->intra && !mb->cbp)  mb->pattern = 0;

    mb->quant = (mb->intra || mb->pattern)
                    && (mb->quantiser_scale_code != prevQsc);
I 6

E 6
    if (mb->quant)  prevQsc = mb->quantiser_scale_code;

    if (*slcFlag)  mb->quant = 0;

D 6
    if (isSkippedMb(mb)) {
E 6
I 6
    if (pic->picture_coding_type == B_PIC) {

#if CHECK_SKIPPED_B_MB
        // Check for skipped macroblocks in B pictures
        //   Note: only frame picture is considered here!
        if (mb->pos.x && mb->pos.x != pic->nCol-16
            && !mb->intra && !mb->pattern
            && mb->motion_type == FRAME_FRAME_BASED
            && mb->motion_forward == prev_motion_forward
            && mb->motion_backward == prev_motion_backward
            && (!mb->motion_forward ||
                   (mb->pmv[0][0].x == mb->mv[0][0].x &&
                    mb->pmv[0][0].y == mb->mv[0][0].y))
            && (!mb->motion_backward ||
                   (mb->pmv[1][0].x == mb->mv[1][0].x &&
                    mb->pmv[1][0].y == mb->mv[1][0].y))
            && prev_motion_type == FRAME_FRAME_BASED	// sufficient condition
           ) {
            mb->type = SKIPPED;
            skippedBMbCnt++;
            return 0;
        }
        else {
            prev_motion_type = mb->motion_type;
            prev_motion_forward = mb->motion_forward;
            prev_motion_backward = mb->motion_backward;
        }
#endif

    }
    else if (isSkippedMb(mb)) {
E 6
        if (mb->pos.x!=0 && mb->pos.x!=pic->nCol-16) {
            mb->type = SKIPPED;
            return 0;
        }

        // Forced coded macroblock at begining or end of slice
        //   Note: independent of the slice structure in the source,
        //         we will always use one row of macroblock as a slice
        if (debugFlag)  printf("\nMB at slice boundary forced coded");
        mb->motion_forward = 1;
        resetFwdMv(mb);
        resetMvPred(pic, mb);
        setDefaultMotion(pic, mb);
    }

    // Set macroblock type
    flag = setMbType(mb, pic->picture_coding_type);
    if (debugFlag)
        printf("\nAttrib: %d %d %d %d %d", mb->quant, mb->motion_forward,
               mb->motion_backward, mb->pattern, mb->intra);

    if (flag == -1) {
        printf("\nFailed to set macroblock type\n");
        printf("Attrib: %d %d %d %d %d\n", mb->quant, mb->motion_forward,
               mb->motion_backward, mb->pattern, mb->intra);
        exit(-1);
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////////




E 2
void reportStatus()
{
    if (tsFlag)  printf("TP %d  ", tpCnt);
    printf("ES Byte %d\n", ftell(inFile.fp));
}


static int getMorePesPayload()
{
    static uint prevDts = 0;	// in 45 kHz
    static int firstTime = 1;
    static int firstPesHdrFound = 0;
    TpInfo tp;
    PesInfo pes;
    while (1) {                 /* loop till next TP with right PID is found */
        if (firstTime) {
            if (findTp(inFile.fp, tpSz, 1)) {
                printf("Failed to find SYNC.\n");
                exit (-1);
            }
            printf("Sync at byte %d\n", ftell(inFile.fp));
            getNextWord(sysIn);
            seekTp(&tp, sysIn, tpSz, 1);
            firstTime = 0;
        }
        else {
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
                    ++tpCnt, ftell(inFile.fp), vidIn->byteCnt);
            commentReader(sysIn, comment);
            if (getByte(sysIn, "sync") != TP_SYNC) {
                printf("Lost of TP SYNC!");
                reportStatus();
                exit(-7855);
            }
        }
        if (sysIn->errFlag == EOF) {
            printf("EOF reached.\n");
            exit(-2707);
        }
 
        /* Get transport packet header */
        getTpHdr(&tp, sysIn);
 
        if (tp.PID==pid) {
            /* Get adaptation field if needed */
            if (tp.adaptation_field_control & 2)
                getTpAf(&tp, sysIn);
 
            /* Get PES header if needed */
            if (tp.payload_unit_start_indicator) {
                if (getBits(sysIn, 24, "start code prefix") != 1)
                    printf("Expected start code prefix not found!  Ignored.\n");                getPesHdr(&pes, sysIn);
 
                /* Print PTS/DTS if available */
                if (pes.PTS_DTS_flags & 2) {
                    printf("\n  PTS %01x%04x",
                        (pes.PTS[0]>>31), (pes.PTS[0]<<1)|(pes.PTS[1]&1));

                    if (pes.PTS_DTS_flags & 1)
                        printf(", DTS %01x%04x",
                            (pes.DTS[0]>>31), (pes.DTS[0]<<1)|(pes.DTS[1]&1));
                }

                /* Check for backward DTS */
                if (pes.PTS_DTS_flags & 2) {
                    uint dts = pes.PTS_DTS_flags==2? pes.PTS[0] : pes.DTS[0];
                    int diff = (int)(dts - prevDts);
                    printf("\n  DTS diff (45 kHz) %d", diff);
                    if (diff<0)  printf(", ## Backward DTS");
                    prevDts = dts;
                }
            }
 
            /* Pass control back to ES parsing only if TP has payload */
            if (tp.adaptation_field_control & 1) {
                copyReader(sysIn, vidIn);
                flushReader(sysIn);
                return 0;
            }

            flushReader(sysIn);
        }
        else {
            skipTp(&tp, sysIn, tpSz);
        }
    }
}


D 2
static void rcPreMb(PicInfo *pic, MbInfo *mb)
{
}


static void rcPostMb(PicInfo *pic, MbInfo *mb)
{
}


static int recodeMb(PicInfo *pic, MbInfo *mb, RlInfo rl[], int* slcFlag)
{
    *slcFlag = !mb->pos.x;
    return 1;
}


E 2
/* Note: return first start code after picture data
   Assumed first slice start code already read

   Note: mb->mbaIncr is used by putMbHdr(), but not getMb()!
*/
static int recodePic(Reader *in, Writer *out, PicInfo *pic, MbInfo *mb,
    RlInfo rl[], uchar *curFrm[], uchar *newRefFrm[], uchar *oldRefFrm[])
{
    int SC;
D 6
    int nextMba;
E 6
    int errCode;
    int mbaIncr = 0;
    uchar **fwdRefFrm, **bwdRefFrm;
    int nBlks, nAcCoefs, nAcBits;
D 6
    int bitPos = getBitPos(in);		// mark beginning of picture
E 6
I 6
    int inBitPos = getReaderBitPos(in);	// mark beginning of input picture
    int outBitPos = getWriterBitPos(out); // mark beginning of output picture
E 6
    int slcFlag;			// whether slice starts
    int codedFlag;			// whether requanted MB is coded
I 2
    int inQs, outQs;
    int prevCodedMba;
E 2

I 6
    inTotQs = inTotSlcQs = inQsCnt = 0;
    outTotQs = outTotSlcQs = outQsCnt = 0;
    skippedBMbCnt = 0;

E 6
I 2
#if RC_FROM_FILE
    fscanf(qsFp, "%s %d", label, &rcFrmNo);
    if (rcFrmNo != pic->frmNo) {
        printf("\nError: frame no in quant scale data file (%d) "\
               "not matching (%d)!\n", rcFrmNo, pic->frmNo);
    }
#endif

E 2
    nBlks = nAcCoefs = nAcBits = 0;
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
I 6
    inTotSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 6
    initSlc(pic, mb);

    /* Get first MBA_increment */
    setReaderEcho(in, vidVerboseLev>=MB, NULL);
    mbaIncr = getMbaIncr(in, &SC);
    if (mbaIncr!=1) {
        /* Either mbaIncr!=1 or another start code found */
        printf("Error: Wrong first slice start position\n");
        printf("mbaIncr = %d\n", mbaIncr);
        reportStatus();
        exit(-937);
    }

    mb->mbaIncr = mbaIncr;
    mb->index = 0;

    while (1) {
        if (mb->index>=pic->nMb) {
            printf("Error: Too many macroblocks in picture %d!\n",
                   pic->frmNo);
            printf("  MB index: %d    # MBs in picture: %d\n",
                   mb->index, pic->nMb);
            reportStatus();
            exit(-18);
        }
        initMb(pic, mb);

        // Backup mv predictors
        pmv[0][0] = mb->pmv[0][0];
        pmv[0][1] = mb->pmv[0][1];
        pmv[1][0] = mb->pmv[1][0];
        pmv[1][1] = mb->pmv[1][1];
        dc_dct_pred[0] = mb->dc_dct_pred[0];
        dc_dct_pred[1] = mb->dc_dct_pred[1];
        dc_dct_pred[2] = mb->dc_dct_pred[2];

        debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);
        if (debugFlag)
            printf("\nFrm %d, MB %d:\n", debugFrmNo, debugMba);

        if (mbaIncr==1) {	/* Process coded macroblock */
            /* Read macroblock header (after mba_incr field) and coefficients */
            errCode = getMb(in, pic, mb, rl, &nBlks, &nAcCoefs, &nAcBits);
            if (errCode != -1) {
                printf("Error when decoding MB %d block %d.\n",
                       mb->index, errCode);
                reportStatus();
                exit(-32);
            }

I 6
            // Compute input average quant
            if (mb->pattern || mb->intra) {
                inQsCnt++;
                inTotQs += quantScaleTab[pic->q_scale_type]\
                                        [mb->quantiser_scale_code-1];
            }

E 6
#if FULL_DECODE
            /* Get motion prediction component */
            if (!mb->intra) {
                getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
                if (debugFlag) {
                    printf("Prediction MB:\n");
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
                }
            }

            if (mb->pattern || mb->intra) {
                /* Get DCT component */
                invQuantScanDct(pic, mb, rl, curBlk);

                /* Combine two components */
                combineMb(pic, mb, curFrm, curBlk);

                if (debugFlag) {
                    printf("Reconstructed MB:\n");
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
                }
            }  
#endif

            // Restore mv predictors
            mb->pmv[0][0] = pmv[0][0];
            mb->pmv[0][1] = pmv[0][1];
            mb->pmv[1][0] = pmv[1][0];
            mb->pmv[1][1] = pmv[1][1];
            mb->dc_dct_pred[0] = dc_dct_pred[0];
            mb->dc_dct_pred[1] = dc_dct_pred[1];
            mb->dc_dct_pred[2] = dc_dct_pred[2];

I 2
            inQsc = mb->quantiser_scale_code;	// backup input QSC
            inQs = quantScaleTab[pic->q_scale_type][inQsc-1];
E 2
            rcPreMb(pic, mb);
D 2
            codedFlag = recodeMb(pic, mb, rl, &slcFlag);
E 2
I 2
            mb->quantiser_scale_code = outQsc;
            outQs = quantScaleTab[pic->q_scale_type][outQsc-1];
E 2

D 2
            if (slcFlag)
E 2
I 2
            codedFlag = recodeMb(pic, mb, rl, inQs, outQs, &slcFlag);

            if (slcFlag) {
E 2
                putSlcHdr(mb, out);
I 6
                outTotSlcQs += quantScaleTab[pic->q_scale_type]\
                                            [mb->quantiser_scale_code-1];
E 6
I 2
                prevCodedMba = (mb->slice_vertical_position-1)*pic->nMbCol - 1;
            }
E 2

            if (codedFlag) {
I 2
                mb->mbaIncr = mb->index - prevCodedMba;
E 2
                putMb(out, pic, mb, rl);
                mb->mbaIncr = 0;
I 2
                prevCodedMba = mb->index;
I 6

                // Compute average quant
                outQsCnt++;
                outTotQs += quantScaleTab[pic->q_scale_type]\
                                         [mb->quantiser_scale_code-1];

E 6
E 2
            }
            rcPostMb(pic, mb);
I 2

            mb->quantiser_scale_code = inQsc;	// restore input QSC
E 2
        }

        else if (mbaIncr>1) {	/* Process skipped macroblock */
I 2
#if RC_FROM_FILE
D 5
            fscanf(qsFp, "%s %d %d %d", label, &rcMba, &rcQsc, &rcQs);
E 5
I 5
            fscanf(qsFp, "%s %d %d %d %d", label, &rcMba, &rcQsc, &rcQs,
                   &rcByteCnt);
E 5
            if (rcMba != mb->index) {
                printf("\nError: MBA from quant scale data file (%d) "\
                       "not matching (%d) for SKIP!\n", rcMba, mb->index);
                exit(-1);
            }
#endif
E 2
            /* Resets for skipped macroblocks */
            if (pic->picture_coding_type==P_PIC) {
                resetMvPred(pic, mb);
                resetFwdMv(mb);
            }
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

#if FULL_DECODE
            getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
            if (debugFlag) {
                printf("Prediction MB:\n");
                printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                16, 16, pic->nCol);
            }
#endif
        }

        /* Update */
        pic->mbCnt++;
        mb->index++;
        mbaIncr--;

        if (!mbaIncr) {
            mbaIncr = getMbaIncr(in, &SC);	/* get next MBA_increment */

            if (mbaIncr) {
                mb->mbaIncr += mbaIncr;
            }
            else {		/* start code found */

                if (SC>=MIN_SLICE_START_CODE && SC<=MAX_SLICE_START_CODE) {
                    /* Slice start code found here */
                    setReaderEcho(in, vidVerboseLev>=SLC, NULL);
                    getSlcHdr(mb, in, SC);
I 6
                    inTotSlcQs += quantScaleTab[pic->q_scale_type]\
                                               [mb->quantiser_scale_code-1];
E 6
                    initSlc(pic, mb);

                    /* Get next MBA_incr */
                    setReaderEcho(in, vidVerboseLev>=MB, NULL);
                    mbaIncr = getMbaIncr(in, &SC);
                    if (mbaIncr==-1) {
                        printf("Error: Illegal MBA increment found\n");
                        reportStatus();
                        exit(-346);
                    }

                    /* Compute and check slice start position */
                    if (mb->mbaRef + mbaIncr != mb->index) {
                        printf("Error: Wrong slice start position: "\
                               "%d (%d, %d), expected %d\n", mb->mbaRef+mbaIncr,
                               mb->slice_vertical_position, mbaIncr, mb->index);
                        reportStatus();
                        exit(-721);
                    }

                    mb->mbaIncr += mbaIncr;
                }

                else if (pic->mbCnt<pic->nMb) {
						/* non-slice start code */
                    printf("Error: unexpected start code 0x000001%02x within "\
                           "picture\n", SC);
                    printf("  current MB: %d\n", pic->mbCnt);
                }
                else switch (SC) {
                    case SEQ_START_CODE:
                    case GOP_START_CODE:
                    case PIC_START_CODE:
                    case SEQ_END_CODE:
I 6
                       {
                         static int prevRefTotBits = 0;

                         int inBits = getReaderBitPos(in) - inBitPos;
                         int outBits = getWriterBitPos(out) - outBitPos;
                         int refTotBits = rcByteCnt << 3;

                         inTotBits += inBits;
                         outTotBits += outBits;
                         printf("\n  Pic: %d -> %d (%d)", inBits, outBits,
                             refTotBits - prevRefTotBits);
                         printf("\n  Tot: %d -> %d (%d)", inTotBits,
                             outTotBits, refTotBits);
                         if (pic->picture_coding_type == B_PIC)
                             printf("\n  Skipped %d MBs", skippedBMbCnt);
                         prevRefTotBits = refTotBits;

E 6
                         return SC;
I 6
                       }
E 6

                    default:
                         printf("Error: Unexpected start code 0x000001%02x at "\
                                "end of picture\n", SC);
                         return SC;
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    Param par;
    char inFilename[128];       // input bitstream filename
    char outFilename[128];      // output bitstream filename
    char sysSyntaxFile[128];    // system syntax filename
    char inVidSynFile[128];     // input video syntax filename
    char outVidSynFile[128];    // input video syntax filename
    int tpMode;
    int mseFlag;
    int sysVerboseLev;

    uchar *curFrm[3];
    uchar *newRefFrm[3];
    uchar *oldRefFrm[3];
    uchar *origFrm[3];

    int state;
    int SC;			// start code
D 6
    int extId;			// extension ID
E 6
D 3
    int firstFrm;
E 3
    int bitMarker = 0;		// for picture size calculation

    long fileLoc;
    int skipBytes;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2rq.par");
    commentParam(&par, "This program re-quantizes an MPEG-2 video bitstream");
    commentParam(&par, "");
    getStringParam(&par, "Input bitstream filename", inFilename, "in.mpg2");

    getIntParam(&par, "Transport bitstream?", &tsFlag, 0);
    getCondIntParam(&par, "TS?", tsFlag, "Packet size mode (1-188 2-204)",
        &tpMode, 1);
    getCondIntParam(&par, "TS?", tsFlag, "Video PID", &pid, 0);
    getCondIntParam(&par, "TS?", tsFlag, "System syntax verbose level",
        &sysVerboseLev, 0);
    getCondStringParam(&par, "TS?", tsFlag, "System syntax output file",
        sysSyntaxFile, "syntax.sys");
    getIntParam(&par, "Video syntax verbose level (0:OFF 1:SEQ 2:GOP 3:PIC "\
        "4:SLC 5:MB 6:ALL)", &vidVerboseLev, 0);
    getStringParam(&par, "Input video syntax file", inVidSynFile,
        "syntax.vid");
    commentParam(&par, "");
    getStringParam(&par, "Requanted bitstream filename", outFilename,
        "out.mpg2");
I 4

E 4
    getStringParam(&par, "Output video syntax file", outVidSynFile,
        "out.syn");
    commentParam(&par, "");
    commentParam(&par, "Distortion calculation:");
    getIntParam(&par, "Compute MSE?", &mseFlag, 0);
    commentParam(&par, "");
    commentParam(&par, "Debugging parameters:");
    getIntParam(&par, "Frame number", &debugFrmNo, -1);
    getIntParam(&par, "Macroblock number", &debugMba, -1);
    getIntParam(&par, "No. of bytes to skip", &skipBytes, 0);
I 2
#if RC_FROM_FILE
    getStringParam(&par, "QS info filename", qsInfoFilename, "qsInfo");
I 5
    getIntParam(&par, "Min quant scale code adjustment (-1, 0)", &qscMinAdj, 0);
    getIntParam(&par, "Max quant scale code adjustment (0, 1)", &qscMaxAdj, 0);
    getIntParam(&par, "Adjustment window", &adjWin, 0);
E 5
#endif
I 4
D 5
    getIntParam(&par, "Quant scale code adjustment", &qscAdj, 0);
E 5
E 4
E 2
    endParam(&par);

I 2
#if RC_FROM_FILE
    qsFp = fopen(qsInfoFilename, "r");
    if (qsFp == NULL) {
        printf("\nError: Failed to read quant scale data file %s\n",
               qsInfoFilename);
        exit (-1);
    }
I 5
    rcQsc = -1;
    rcMinDiff = rcMaxDiff = 0;
E 5
#endif

E 2
    debugTemp = vidVerboseLev;

    tpSz = (tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;

    /* Input channel setup */
    openInFile(&inFile, inFilename, tpSz);
    fseek(inFile.fp, skipBytes, SEEK_SET);
    if (tsFlag) {
        /* Input is transport stream */
        uint* endPtr = inFile.buf + (TP_SIZE>>2);
        sysIn = &inFile.rdr;
        vidIn = (Reader*)malloc(sizeof(Reader));
        initReader(vidIn, getMorePesPayload, NULL);
        setBitBuf(&vidIn->bitBuf, endPtr, endPtr, 0);
        setReaderEcho(sysIn, sysVerboseLev, fopen(sysSyntaxFile, "wb"));
    }
    else {
        /* Input is video elementary stream */
        vidIn = &inFile.rdr;
    }
    setReaderEcho(vidIn, 0, fopen(inVidSynFile, "wb"));

    // Output channel setup
    openOutFile(&outFile, outFilename, TP_SIZE);
    vidOut = &outFile.wtr;
D 2
    setWriterEcho(vidOut, 1, fopen(outVidSynFile, "wb"));
E 2
I 2
    setWriterEcho(vidOut, 0, fopen(outVidSynFile, "wb"));
E 2

    initVld();
    initVlc();
    init_idct();
    initClipTab();

    /* To prevent memory deallocation */
    curFrm[Y] = curFrm[Cb] = curFrm[Cr] = 0;
    newRefFrm[Y] = newRefFrm[Cb] = newRefFrm[Cr] = 0;
    oldRefFrm[Y] = oldRefFrm[Cb] = oldRefFrm[Cr] = 0;

    pic.resetFlag = 1;
D 3
    pic.trBase = pic.nextTrBase = firstFrm;
E 3
I 3
    pic.trBase = pic.nextTrBase = 0;
E 3

    seekSeqHdr(vidIn);
    fileLoc = ftell(inFile.fp);

I 6
    inTotBits = outTotBits = 0;

E 6
    state = SEQ;

    while (1) {
        switch (state) {
            case SEQ:
D 6
                printf("\n\nSEQ Header found\n");
E 6
I 6
                printf("\n\n\nSEQ Header found\n");
E 6
                setReaderEcho(vidIn, vidVerboseLev>=SEQ, NULL);
                if (getSeqLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }

                // Copy sequence layer headers to output
                putSeqHdr(&pic, vidOut);
                putSeqExt(&pic, vidOut);
                flushOutFile(&outFile);

                /* One time initialization */
                if (pic.resetFlag) {
                    printf("Initialize buffers...\n");
                    initBuffers(&pic, curFrm);
                    initBuffers(&pic, newRefFrm);
                    initBuffers(&pic, oldRefFrm);
                    if (mseFlag)
                        initBuffers(&pic, origFrm);
D 6

E 6
I 6
                    printf("Frame size: %d x %d", pic.horizontal_size,
                        pic.vertical_size);
E 6
                    pic.resetFlag = 0;
                }

                /* Find next state */
                switch (SC) {
                    case GOP_START_CODE: state = GOP;  break;
                    case PIC_START_CODE: state = PIC;  break;
                    default:
                        printf("GOP or picture header expected!\n");
                        seekSeqHdr(vidIn);	/* resync */
                }
                break;

            case GOP:
D 6
                printf("\n\nGOP Header found\n");
E 6
I 6
                printf("\nGOP Header found\n");
E 6
                setReaderEcho(vidIn, vidVerboseLev>=GOP, NULL);
                if (getGopLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }

                // Copy GOP header to output
                putGopHdr(&pic, vidOut);

D 6
                printf("  TC %02d:%02d:%02d:%02d, drop %d\n\n",
E 6
I 6
                printf("  TC %02d:%02d:%02d:%02d, drop %d",
E 6
                       pic.time_code_hours, pic.time_code_minutes,
                       pic.time_code_seconds, pic.time_code_pictures,
                       pic.drop_frame_flag);

                /* Find next state */
                if (SC==PIC_START_CODE)
                    state = PIC;
                else {
                    printf("GOP or picture header expected!\n");
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

                // Copy picture layer headers to output
                putPicHdr(&pic, vidOut);
                putPicCodeExt(&pic, vidOut);
                fflush(outFile.fp);

                if (SC!=1) {
                    printf("Error: Invalid slice start code 0x000001%02x "\
                           "found!\n", SC);
                    reportStatus();
                    exit(-283);
                }

                if (pic.picture_structure!=FRAME) {
                    printf("Sorry!  Field picture not supported yet!\n");
                    reportStatus();
                    exit(-267);
                }

                sprintf(comment, "%s-%d", PIC_TYPE[pic.picture_coding_type],
                    pic.frmNo);
                commentReader(vidIn, "\nPic ");
                commentReader(vidIn, comment);
                commentReader(vidIn, "...");

D 2
                printf("\n\nDecoding %s...", comment);
E 2
I 2
                printf("\n\nRecoding %s...", comment);
E 2
                printf("\n  File loc %d, TP %d", fileLoc, tpCnt);

                vidVerboseLev = pic.frmNo==debugFrmNo? 6 : debugTemp;

                /* Decode a picture */
                SC = recodePic(vidIn, vidOut, &pic, &mb, rl, curFrm, newRefFrm,
                               oldRefFrm);
                fileLoc = ftell(inFile.fp);
I 6
                printf("\n  qs: %.1f -> %.1f",
                    (inQsCnt? (float)inTotQs/inQsCnt : 0.),
                    (outQsCnt? (float)outTotQs/outQsCnt : 0.));
                printf("\n  slcQs: %.1f -> %.1f", (float)inTotSlcQs/pic.nMbRow,
                    (float)outTotSlcQs/pic.nMbRow);
                printf("\n  RC: diff %d (%d -> %d), qsAdj %d",
                    vidOut->byteCnt - rcByteCnt, rcMinDiff, rcMaxDiff, qscAdj);

E 6
                bitMarker = vidIn->byteCnt;
I 5
D 6
printf("\n  RC diff: %d, %d, %d", rcMinDiff, rcMaxDiff, qscAdj);
E 6
E 5

                if (vidIn->echoFlag)
                    fflush(vidIn->echoFp);  // flush video syntax

                if (pic.picture_structure==FRAME || pic.secondFld) {
                    if (pic.picture_coding_type!=B_PIC)
                        swapBuffers(curFrm, newRefFrm, oldRefFrm);
                }

                if (mseFlag) {
                    int mse, mseY, mseCb, mseCr, frmSz;

                    /* Compute MSE */
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

                /* Find next state */
                switch (SC) {
                    case SEQ_START_CODE: state = SEQ;  break;
                    case GOP_START_CODE: state = GOP;  break;
                    case PIC_START_CODE: state = PIC;  break;

                    case SEQ_END_CODE:
                        printf("\nEnd of sequence reached!!\n\n");
                        /* Continuous to look for next sequence start code */
                        seekSeqHdr(vidIn);
                        state = SEQ;
                        break;

                    default:
                        while (1) {
                            SC = getNextStartCode(vidIn);
                            if (SC==PIC_START_CODE)  state = PIC;  break;
                            if (SC==GOP_START_CODE)  state = GOP;  break;
                            if (SC==SEQ_START_CODE)  state = SEQ;  break;
                        }
                }
                break;
        }
    }
}

E 1
