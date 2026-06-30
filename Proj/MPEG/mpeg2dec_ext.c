/*****************************************************************************
    File: mpeg2dec.c

    MPEG-2 video decoder
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FILENAME_SZ		128

#define	SCAN_QS			1


#include "def.h"

#ifdef UNIX
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
#include "display.h"
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
#endif


/* Note: The following variables are moved out from the main loop to here
         to avoid reserved memory problem.
*/
Reader *sysIn;         // system stream reader
Reader *vidIn;         // video elementary stream reader
PicInfo pic;
MbInfo mb;
RlInfo rl[6];          // Note: for 4:2:0 seq only

int tsFlag;
char comment[80];
int tpSz;
int tpCnt = 0;

InFile inFile;

int decodeFlag;
int displayFlag = 0;
int vidVerboseLev;

int pid;

/* For debugging purpose only */
int debugFlag;
int debugFrmNo;
int debugMba;
int debugTemp;
int dumpCoefFlag;
int prevDTS;    // previous DTS in 45 kHz form

int totQs, totSlcQs, qsCnt;  // to compute average picture quant
int qsCnt = 0;

#if DUMP_QSCALE
int qsFlag;
FILE *qsFp;
#endif

#if DUMP_COEF_MAP
int coefMapFlag;
FILE *coefMapFp;
#endif

#if SCAN_QS
// Approach 1: only scan 1st MB:
int totFstQs1, fstQsCnt1;

// Approach 2: scan up to N MBs, N = 2:
int seekQs2, totFstQs2, fstQsCnt2;

// Approach 3: scan until 1st MB containing DCT is found:
int seekQs3, totFstQs3, fstQsCnt3;
#endif

#if DUMP_TP_QS
int tpQsFlag;
int tpQsc, tpQs;
FILE *tpQsFp;
#endif


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


void reportStatus()
{
    if (tsFlag)  printf("\nTP %d  ", tpCnt);
    printf("\nES Byte %d\n\n", ftell(inFile.fp));
}


static int getMorePesPayload()
{
    static uint prevDts = 0;	// in 45 kHz
    static int firstTime = 1;
    TpInfo tp;
    PesInfo pes;
    while (1) {                 /* loop till next TP with right PID is found */
        if (firstTime) {
            if (findTp(inFile.fp, tpSz, 1)) {
                printf("\nFailed to find SYNC.");
                exit (-1);
            }
            printf("\nSync at byte %d", ftell(inFile.fp));
            getNextWord(sysIn);
            seekTp(&tp, sysIn, tpSz, 1);
            firstTime = 0;
        }
        else {
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
                    ++tpCnt, ftell(inFile.fp), vidIn->byteCnt);
            commentReader(sysIn, comment);
            if (getByte(sysIn, "sync") != TP_SYNC) {
                printf("\nLost of TP SYNC!");
                reportStatus();
                exit(-7855);
            }
        }
        if (sysIn->errFlag == EOF) {
            printf("\nEOF reached.\n");
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
                    printf("\nExpected start code prefix not found!  Ignored.");                getPesHdr(&pes, sysIn);
 
#if 0
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
#endif
            }
 
            /* Pass control back to ES parsing only if TP has payload */
            if (tp.adaptation_field_control & 1) {
#if DUMP_TP_QS
                if (tpQsFlag)
                    fprintf(tpQsFp, "%08d %3d %d %d\n", tpCnt, pid, tpQsc, tpQs);
#endif
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



/* Note: return first start code after picture data
   Assumed first slice start code already read
*/
static int decodePic(Reader *in, PicInfo *pic, MbInfo *mb, RlInfo rl[],
                     uchar *curFrm[], uchar *newRefFrm[], uchar *oldRefFrm[])
{
    int SC;
    int nextMba;
    int errCode;
    int mbaIncr;
    uchar **fwdRefFrm, **bwdRefFrm;
    int nBlks, nAcCoefs, nAcBits;
    int bitPos = getReaderBitPos(in);	// mark beginning of picture

//    printf("\n  TR %d", pic->temporal_reference);

#if DUMP_QSCALE
    if (qsFlag)
        fprintf(qsFp, "P %d\n", pic->frmNo);
#endif

    totQs = totSlcQs = qsCnt = 0;
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
    initSlc(pic, mb);
    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];

#if SCAN_QS
    fstQsCnt1 = totFstQs1 = 0;

    seekQs2 = 1;
    fstQsCnt2 = totFstQs2 = 0;

    seekQs3 = 1;
    fstQsCnt3 = totFstQs3 = 0;
#endif


    /* Get first MBA_increment */
    setReaderEcho(in, vidVerboseLev>=MB, NULL);
    mbaIncr = getMbaIncr(in, &SC);
    mb->index = mb->mbaRef + mbaIncr;
    if (mb->index != 0) {
        /* Either mbaIncr!=1 or another start code found */
        printf("\nError: Wrong first slice start position %d", mbaIncr);
        reportStatus();
        exit(-937);
    }

    while (1) {
        if (mb->index>=pic->nMb) {
            printf("\nError: Too many macroblocks in picture %d!",
                   pic->frmNo);
            printf("\n  MB index: %d    # MBs in picture: %d",
                   mb->index, pic->nMb);
            reportStatus();
            exit(-18);
        }

        /* Display second field */
        /* Note: assumes frame picture */
#ifdef UNIX
        if (displayFlag && !pic->progressive_sequence
            && mb->index==(pic->nMb>>1))	/* half picture */
            display_second_field();
#endif

        initMb(pic, mb);

        debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);
        if (debugFlag) {
            printf("\nFrm %d, MB %d:", debugFrmNo, debugMba);
            printf("\n  PMV: (%d,%d), (%d,%d), (%d,%d), (%d,%d)",
                   mb->pmv[0][0].x, mb->pmv[0][0].y,
                   mb->pmv[0][1].x, mb->pmv[0][1].y,
                   mb->pmv[1][0].x, mb->pmv[1][0].y,
                   mb->pmv[1][1].x, mb->pmv[1][1].y);
        }

        if (mbaIncr==1) {	/* Process coded macroblock */
            /* Read macroblock header (after mba_incr field) and coefficients */
            errCode = getMb(in, pic, mb, rl, &nBlks, &nAcCoefs, &nAcBits);
            if (errCode != -1) {
                printf("\nError when decoding MB %d block %d.",
                       mb->index, errCode);
                reportStatus();
                exit(-32);
            }

#if DUMP_QSCALE
            // Dump QSCALE info for rate control debugging
            if (qsFlag) {
                int quantScale = quantScaleTab[pic->q_scale_type] \
                                              [mb->quantiser_scale_code-1];
                int temp = pic->picture_coding_type==I_PIC? mb->type+1 :
                               (pic->picture_coding_type==P_PIC? mb->type+3 :
                               mb->type+10);
                fprintf(qsFp, "M %d %d %d %d\n", mb->index,
                        mb->quantiser_scale_code, quantScale, vidIn->byteCnt);
            }
#endif

#if DUMP_TP_QS
            if (tpQsFlag) {
                tpQsc = mb->quantiser_scale_code;
                tpQs = quantScaleTab[pic->q_scale_type][tpQsc-1];
            }
#endif

            // Compute average quant
            if (mb->pattern || mb->intra) {
                qsCnt++;
                totQs += quantScaleTab[pic->q_scale_type] \
                                      [mb->quantiser_scale_code-1];
#if SCAN_QS
                if (mb->pos.x==0) {
                    totFstQs1 += quantScaleTab[pic->q_scale_type] \
                                              [mb->quantiser_scale_code-1];
                    fstQsCnt1++;
                }

                if (seekQs2 && (mb->pos.x <= 16*(2-1))) {
                    totFstQs2 += quantScaleTab[pic->q_scale_type] \
                                              [mb->quantiser_scale_code-1];
                    fstQsCnt2++;
                    seekQs2 = 0;
                }

                if (seekQs3) {
                    totFstQs3 += quantScaleTab[pic->q_scale_type] \
                                              [mb->quantiser_scale_code-1];
                    fstQsCnt3++;
                    seekQs3 = 0;
                }
#endif
            }


            if (decodeFlag) {
                /* Get motion prediction component */
                if (!mb->intra) {
                    getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
                    if (debugFlag) {
                        printf("\nPrediction MB:\n");
                        printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                        16, 16, pic->nCol);
                    }
                }

                if (mb->pattern || mb->intra) {
                    /* Get DCT component */
                    invQuantScanDct(pic, mb, rl, curBlk, debugFlag);

                    /* Combine two components */
                    combineMb(pic, mb, curFrm, curBlk);

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
                }
            }    // decodeFlag
        }    // mbaIncr==1

        else if (mbaIncr>1) {	/* Process skipped macroblock */

#if DUMP_COEF_MAP
            if (coefMapFlag) {
                int i;
                for (i=0; i<6; i++)
                    fprintf(coefMapFp, "\n%d %d %d 00000:",
                            pic->frmNo, mb->index, i);
            }
#endif
            /* Resets for skipped macroblocks */
            if (pic->picture_coding_type==P_PIC) {
                resetMvPred(pic, mb);
                resetFwdMv(mb);
            }
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

            if (decodeFlag) {
                getPredMb(pic, mb, curFrm, fwdRefFrm, bwdRefFrm, 0);
                if (debugFlag) {
                    printf("\nPrediction MB:\n");
                    printUcharBlock(curFrm[Y] + mb->pos.y*pic->nCol + mb->pos.x,
                                    16, 16, pic->nCol);
                }
            }

#if DUMP_QSCALE
            // Dump QSCALE info for rate control debugging
              if (qsFlag) {
                  int quantScale = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
                  fprintf(qsFp, "M %d %d %d %d\n", mb->index,
                      mb->quantiser_scale_code, quantScale, vidIn->byteCnt);
              }
#endif
        }

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
                    initSlc(pic, mb);
                    totSlcQs += quantScaleTab[pic->q_scale_type]\
                                             [mb->quantiser_scale_code-1];
#if SCAN_QS
                    seekQs2 = seekQs3 = 1;
#endif

                    /* Get next MBA_incr */
                    setReaderEcho(in, vidVerboseLev>=MB, NULL);
                    mbaIncr = getMbaIncr(in, &SC);
                    if (mbaIncr==-1) {
                        printf("\nError: Illegal MBA increment found");
                        reportStatus();
                        exit(-346);
                    }

                    /* Compute and check slice start position */
                    if (mb->mbaRef + mbaIncr != mb->index) {
                        printf("\nError: Wrong slice start position: "\
                               "%d (%d, %d), expected %d", mb->mbaRef+mbaIncr,
                               mb->slice_vertical_position, mbaIncr, mb->index);
                        reportStatus();
                        exit(-721);
                    }
                }

                else if (pic->mbCnt<pic->nMb) {
						/* non-slice start code */
                    printf("\nError: unexpected start code 0x000001%02x "\
                           "within picture", SC);
                    printf("\n  current MB: %d", pic->mbCnt);
                }

                else switch (SC) {
                    case SEQ_START_CODE:
                    case GOP_START_CODE:
                    case PIC_START_CODE:
                    case SEQ_END_CODE:
                       {
                         int totBits = getReaderBitPos(in) - bitPos;
                         printf("\n  Total: %d bits, %d blks, AC: %d%c, %2.1f/blk",
                             totBits, nBlks, nAcBits*100/totBits, '%',
                             (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
                         return SC;
                       }

                    default:
                         printf("\nError: Unexpected start code 0x000001%02x "\
                                "at end of picture\n", SC);
                         return SC;
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    Param par;
    char bitstreamFile[FILENAME_SZ];    // bitstream filename
    char outSeqName[FILENAME_SZ];       // output sequence filename
    char origSeqName[FILENAME_SZ];      // original sequence filename
    char sysSyntaxFile[FILENAME_SZ];    // system syntax filename
    char vidSyntaxFile[FILENAME_SZ];    // video syntax filename
    int tpMode;
    int mseFlag = 0;
    int sysVerboseLev;

    uchar *curFrm[3];
    uchar *newRefFrm[3];
    uchar *oldRefFrm[3];
    uchar *origFrm[3];

    int state;
    int SC;			// start code
    int extId;			// extension ID
    int firstFrm;
    int bitMarker = 0;		// for picture size calculation

    long fileLoc;
    int skipBytes;

#if DUMP_QSCALE
    char qsFile[FILENAME_SZ];
#endif
#if DUMP_COEF_MAP
    char coefFile[FILENAME_SZ];
#endif
#if DUMP_TP_QS
    char tpQsFile[FILENAME_SZ];
#endif

    outSeqName[0] = 0;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2dec_ext.par");
    commentParam(&par, "This program decode a MPEG-2 video bitstream");
    commentParam(&par, "");
    getStringParam(&par, "Bitstream filename", bitstreamFile, "test.mpg2");

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
    getStringParam(&par, "Video syntax output file", vidSyntaxFile,
        "syntax.vid");
    getIntParam(&par, "First frame number", &firstFrm, 0);
    getIntParam(&par, "Decode bitstream?", &decodeFlag, 1);
    getCondStringParam(&par, "Decode?", decodeFlag, "Output sequence name",
        outSeqName, "");
    getCondIntParam(&par, "Decode?", decodeFlag, "Show output video?",
        &displayFlag, 0);
    commentParam(&par, "");
    commentParam(&par, "Distortion calculation:");
    getCondIntParam(&par, "Decode?", decodeFlag, "Compute MSE?", &mseFlag, 0);
    getCondStringParam(&par, "MSE?", mseFlag, "Original sequence name",
        origSeqName, "");
    commentParam(&par, "");
    commentParam(&par, "Debugging parameters:");
    getIntParam(&par, "Frame number", &debugFrmNo, -1);
    getIntParam(&par, "Macroblock number", &debugMba, -1);
    getIntParam(&par, "No. of bytes to skip", &skipBytes, 0);

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

#if DUMP_TP_QS
    getIntParam(&par, "Dump TP QS map?", &tpQsFlag, 0);
    getCondStringParam(&par, "TP QS?", tpQsFlag,
        "Output filename for TP QS map", tpQsFile, "test.tpqs");
#endif

    endParam(&par);

    debugTemp = vidVerboseLev;


#if DUMP_QSCALE
    if (qsFlag)
        if ((qsFp = fopen(qsFile, "wb")) == NULL) {
            printf("Error: Failed to open qs info file %s for output", qsFile);
            qsFlag = 0;
        }
#endif

#if DUMP_COEF_MAP
    if (coefMapFlag)
        if ((coefMapFp = fopen(coefMapFile, "wb")) == NULL) {
            printf("Error: Failed to open coef map file %s for output",
                   coefMapFile);
            coefMapFlag = 0;
        }
#endif

#if DUMP_TP_QS
    if (tpQsFlag)
        if ((tpQsFp = fopen(tpQsFile, "wb")) == NULL) {
            printf("Error: Failed to open TP QS file %s for output", tpQsFile);
            tpQsFlag = 0;
        }
#endif

    tpSz = (tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;

    /* MPEG-2 channel setup */
    openInFile(&inFile, bitstreamFile, tpSz);
    fseek(inFile.fp, skipBytes, SEEK_SET);
    printf("Input bitstream: %s (byte %d)", bitstreamFile, skipBytes);

    if (tsFlag) {
        /* Input is transport stream */
        uint* endPtr = inFile.buf + (TP_SIZE>>2);
        sysIn = &inFile.rdr;
        vidIn = (Reader*)malloc(sizeof(Reader));
        initReader(vidIn, getMorePesPayload, NULL);
        setBitBuf(&vidIn->bitBuf, endPtr, endPtr, 0);
        setReaderEcho(sysIn, sysVerboseLev, fopen(sysSyntaxFile, "wb"));
        printf("PID: %d", pid);
    }
    else {
        /* Input is video elementary stream */
        vidIn = &inFile.rdr;
    }
    setReaderEcho(vidIn, 0, fopen(vidSyntaxFile, "wb"));

    initVld();
    init_idct();
    initClipTab();

    /* To prevent memory deallocation */
    curFrm[Y] = curFrm[Cb] = curFrm[Cr] = 0;
    newRefFrm[Y] = newRefFrm[Cb] = newRefFrm[Cr] = 0;
    oldRefFrm[Y] = oldRefFrm[Cb] = oldRefFrm[Cr] = 0;
    if (mseFlag)
        origFrm[Y] = origFrm[Cb] = origFrm[Cr] = 0;

    pic.resetFlag = 1;
    pic.trBase = pic.nextTrBase = firstFrm;

    seekSeqHdr(vidIn);
    fileLoc = ftell(inFile.fp);
    state = SEQ;

    while (1) {
        switch (state) {
            case SEQ:
                printf("\n\n\nSEQ Header found");
                setReaderEcho(vidIn, vidVerboseLev>=SEQ, NULL);
                if (getSeqLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }

                /* One time initialization */
                if (pic.resetFlag) {
                    printf("\nInitialize buffers...");
                    initBuffers(&pic, curFrm);
                    initBuffers(&pic, newRefFrm);
                    initBuffers(&pic, oldRefFrm);
                    if (displayFlag) {
#ifdef UNIX
                        init_display("", &pic);
                        init_dither();
#else
			initDisplay (pic.nCol, pic.nRow);
#endif
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
                        printf("\nGOP or picture header expected!");
                        seekSeqHdr(vidIn);	/* resync */
                }
                break;

            case GOP:
                printf("\nGOP Header found");
                setReaderEcho(vidIn, vidVerboseLev>=GOP, NULL);
                if (getGopLayer(vidIn, &pic, &SC)) {
                    seekSeqHdr(vidIn);		/* resync */
                    break;
                }
                printf("\n  TC %02d:%02d:%02d:%02d, drop %d",
                       pic.time_code_hours, pic.time_code_minutes,
                       pic.time_code_seconds, pic.time_code_pictures,
                       pic.drop_frame_flag);

                /* Find next state */
                if (SC==PIC_START_CODE)
                    state = PIC;
                else {
                    printf("\nGOP or picture header expected!");
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
                    printf("\nError: Invalid slice start code 0x000001%02x "\
                           "found!", SC);
                    reportStatus();
                    exit(-283);
                }

                if (pic.picture_structure!=FRAME) {
                    printf("\nSorry!  Field picture not supported yet!");
                    reportStatus();
                    exit(-267);
                }

                sprintf(comment, "%s-%d", PIC_TYPE[pic.picture_coding_type],
                    pic.frmNo);
                commentReader(vidIn, "\nPic ");
                commentReader(vidIn, comment);
                commentReader(vidIn, "...");

                printf("\n\nDecoding %s...", comment);
                printf("\n  File loc %d", fileLoc);
                if (tsFlag)  printf(", TP %d", tpCnt);
printf("\nTFF %d, RFF %d", pic.top_field_first, pic.repeat_first_field);

                vidVerboseLev = pic.frmNo==debugFrmNo? 6 : debugTemp;

                /* Decode a picture */
                SC = decodePic(vidIn, &pic, &mb, rl, curFrm, newRefFrm,
                               oldRefFrm);
                fileLoc = ftell(inFile.fp);

#if 0
                printf("\n  qs %.1f, slcQs %.1f",
                    (qsCnt? (float)totQs/qsCnt : 0.),
                    (float)totSlcQs/pic.nMbRow);
#endif

#if SCAN_QS
                printf("\n  QS: Slc %.1f, Mb (%.1f, %d), (%.1f, %d),"\
                    " (%.1f, %d), (%.1f, %d)",
                    (float)totSlcQs/pic.nMbRow,
                    (qsCnt? (float)totQs/qsCnt : 0.), qsCnt,
                    (fstQsCnt1? (float)totFstQs1/fstQsCnt1 : 0.), fstQsCnt1,
                    (fstQsCnt2? (float)totFstQs2/fstQsCnt2 : 0.), fstQsCnt2,
                    (fstQsCnt3? (float)totFstQs3/fstQsCnt3 : 0.), fstQsCnt3);
#endif

                bitMarker = vidIn->byteCnt;

                if (vidIn->echoFlag)
                    fflush(vidIn->echoFp);  // flush video syntax

                if (mseFlag) {
                    float mse, mseY, mseCb, mseCr;
                    float psnrY, psnrCb, psnrCr;
                    int frmSz = pic.nCol * pic.nRow;

                    /* Compute MSE */
                    readFrm(origSeqName, &pic, origFrm);
                    mseY = blockTse(curFrm[Y], origFrm[Y], pic.nCol,
                                    pic.nRow, pic.nCol) / (float)frmSz;
                    mseCb = blockTse(curFrm[Cb], origFrm[Cb], pic.nCol>>1,
                                pic.nRow>>1, pic.nCol>>1) / (float)(frmSz>>2);
                    mseCr = blockTse(curFrm[Cr], origFrm[Cr], pic.nCol>>1,
                                pic.nRow>>1, pic.nCol>>1) / (float)(frmSz>>2);
                    mse = (mseY*4 + mseCb + mseCr) / 6;
                    printf("\n  MSE: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
                           mseY, mseCb, mseCr, mse);

                    // Compute PSNR in dB
                    //   PSNR = 10 x log_10(255^2 / mse)
                    //
                    psnrY = 48.1308 - 10 * log10(mseY);
                    psnrCb = 48.1308 - 10 * log10(mseCb);
                    psnrCr = 48.1308 - 10 * log10(mseCr);	
                    printf("\n  PSNR: Y %3.2f  Cb %3.2f  Cr %3.2f",
                           psnrY, psnrCb, psnrCr);
#if MB_MSE
                    if (mse) {
                        // Compute mse for each MB (Y only)
                        computeMbTse(curFrm[Y], origFrm[Y], pic.nCol, pic.nRow);
                    }
#endif
                }

                if (pic.picture_structure==FRAME || pic.secondFld) {
                    if (strlen(outSeqName))
                        saveFrm(outSeqName, &pic, curFrm);
                    if (pic.picture_coding_type!=B_PIC)
                        swapBuffers(curFrm, newRefFrm, oldRefFrm);
                    if (displayFlag) {
#ifdef UNIX
                        dither(pic.picture_coding_type==B_PIC? curFrm:oldRefFrm,
                               &pic);
#else
                        if (pic.picture_coding_type==B_PIC)
                            Display_Image(curFrm[0], curFrm[2], curFrm[1]);
                        else
                            Display_Image(oldRefFrm[0], oldRefFrm[2],
                                          oldRefFrm[1]);
#endif
                    }
                }

                /* Find next state */
                switch (SC) {
                    case SEQ_START_CODE: state = SEQ;  break;
                    case GOP_START_CODE: state = GOP;  break;
                    case PIC_START_CODE: state = PIC;  break;

                    case SEQ_END_CODE:
                        printf("\n\nEnd of sequence reached!!\n");
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

    if (displayFlag) {
#ifdef UNIX
	    exit_display();
#else
            closeDisplay();
#endif
    }
}

