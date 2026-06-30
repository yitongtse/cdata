/*****************************************************************************
    File: mpeg2cmp.c

    Compare two MPEG-2 bitstreams
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FILENAME_SZ		128

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


typedef struct {
  InFile inFile;	// input file
  Reader* sysIn;	// system stream reader
  Reader* vidIn;	// video elementary stream reader
  PicInfo pic;		// sequence, GOP and picture level info
  MbInfo  mb;		// slice and MB level info
  RlInfo  rl[6];	// runlength amplitude info

  uchar*  curFrm[3];	// current frame buffers
  uchar*  newRefFrm[3];	// new reference frame buffers
  uchar*  oldRefFrm[3];	// old reference frame buffers

  int     tsFlag;	// 0-ES, 1-TS
  int     tpSz;		// TP size
  int     pid;		// PID
  int     state;	// decoder state
  int     SC;		// next start code
  int     firstTime;
}
Decoder;


Decoder dec[2];
int k;			// decoder index
char comment[80];
int tpCnt = 0;
int vidVerboseLev = 0;
FILE* logFp[2];
int totQs, totSlcQs, qsCnt;


void initDecoder(Decoder* dec)
{
    int i;

    dec->pic.resetFlag = 1;
    dec->pic.trBase = dec->pic.nextTrBase = 0;

    for (i=Y; i<Cr; i++) {
        dec->curFrm[i] = 0;
        dec->newRefFrm[i] = 0;
        dec->oldRefFrm[i] = 0;
    }
}


void reportStatus(Decoder* dec)
{
    if (dec->tsFlag)  printf("\nTP %d  ", tpCnt);
    printf("\nES Byte %d\n\n", ftell(dec->inFile.fp));
}


static int getMorePesPayload(Decoder* dec)
{
    TpInfo tp;
    PesInfo pes;

    while (1) {                 /* loop till next TP with right PID is found */
        if (dec->firstTime) {
            if (findTp(dec->inFile.fp, dec->tpSz, 1)) {
                printf("\nFailed to find SYNC.");
                exit (-1);
            }
            printf("\nSync at byte %d", ftell(dec->inFile.fp));
            getNextWord(dec->sysIn);
            seekTp(&tp, dec->sysIn, dec->tpSz, 1);
            dec->firstTime = 0;
        }
        else {
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
                    ++tpCnt, ftell(dec->inFile.fp), dec->vidIn->byteCnt);
            commentReader(dec->sysIn, comment);
            if (getByte(dec->sysIn, "sync") != TP_SYNC) {
                printf("\nLost of TP SYNC!");
                reportStatus(dec);
                exit(-7855);
            }
        }
        if (dec->sysIn->errFlag == EOF) {
            printf("\nEOF reached.\n");
            exit(-2707);
        }
 
        /* Get transport packet header */
        getTpHdr(&tp, dec->sysIn);
 
        if (tp.PID == dec->pid) {
            /* Get adaptation field if needed */
            if (tp.adaptation_field_control & 2)
                getTpAf(&tp, dec->sysIn);
 
            /* Get PES header if needed */
            if (tp.payload_unit_start_indicator) {
                if (getBits(dec->sysIn, 24, "start code prefix") != 1)
                    printf("\nExpected start code prefix not found!  Ignored.");                getPesHdr(&pes, dec->sysIn);
            }
 
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

            /* Get motion prediction component */
            if (!mb->intra)
                getPredMb(pic, mb, dec->curFrm, fwdRefFrm, bwdRefFrm, 0);

            if (mb->pattern || mb->intra) {
                /* Get DCT component */
                invQuantScanDct(pic, mb, dec->rl, curBlk, 0);

                /* Combine two components */
                combineMb(pic, mb, dec->curFrm, curBlk);
            }  

        }
        else if (mbaIncr>1) {	/* Process skipped macroblock */
            /* Resets for skipped macroblocks */
            if (pic->picture_coding_type==P_PIC) {
                resetMvPred(pic, mb);
                resetFwdMv(mb);
            }
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

            getPredMb(pic, mb, dec->curFrm, fwdRefFrm, bwdRefFrm, 0);
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
                        int totBits = getReaderBitPos(in) - bitPos;
                        fprintf(logFp, "\n  Total: %d bits, %d blks, AC: %d%c,"\
                             " %2.1f/blk", totBits, nBlks, nAcBits*100/totBits,
                             '%', (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
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
               fprintf(logFp, "\n  TC %02d:%02d:%02d:%02d",
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
               setReaderEcho(dec->vidIn, verboseLev>=PIC, NULL);
               if (getPicLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }

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
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", tpCnt);

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


// Compares picture info of two pictures
// Returns 1 if different
//
void comparePics(PicInfo* pic1, PicInfo* pic2)
{
    if (pic1->frmNo != pic2->frmNo)
        printf("\nPicture number mismatch: %d, %d",
            pic1->frmNo, pic2->frmNo);

    if (pic1->picture_coding_type != pic2->picture_coding_type)
        printf("\nPicture coding type mismatch: %d, %d",
            pic1->picture_coding_type, pic2->picture_coding_type);

    if ((pic1->nRow != pic2->nRow) || (pic1->nCol != pic2->nCol))
        printf("\nPicture resolution mismatch: %dx%d, %dx%d",
            pic1->nRow, pic1->nCol, pic2->nRow, pic2->nCol);

    if (pic1->temporal_reference != pic2->temporal_reference)
        printf("\nPicture temporal reference mismatch: %d, %d",
            pic1->temporal_reference, pic2->temporal_reference);

    // Compare time code
    if (pic1->picture_coding_type == I_PIC) {
        if ((pic1->time_code_hours != pic2->time_code_hours) ||
            (pic1->time_code_minutes != pic2->time_code_minutes) ||
            (pic1->time_code_seconds != pic2->time_code_seconds) ||
            (pic1->time_code_pictures != pic2->time_code_pictures))
        printf("\nTime code mismatch: %02d:%02d:%02d:%02d, %02d:%02d:%02d:%02d",
            pic1->time_code_hours, pic1->time_code_minutes,
            pic1->time_code_seconds, pic1->time_code_pictures,
            pic2->time_code_hours, pic2->time_code_minutes,
            pic2->time_code_seconds, pic2->time_code_pictures);
    }
}


int main(int argc, char** argv)
{
    Param par;
    char  bsFile[2][FILENAME_SZ];   	// bitstream 1 filename
    char  initTc[12];
    int   tcAlign;
    int   tcVal, tcVal2;
    int   skipBytes[2];
    int   tpMode[2];
    int   i;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2cmp.par");
    commentParam(&par, "This program compare two MPEG-2 video bitstreams");

    commentParam(&par, "");
    commentParam(&par, "Bitstream 1 info:");
    getStringParam(&par, "Filename", bsFile[0], "test1.mpg");
    getIntParam(&par, "Byte offset", &skipBytes[0], 0);
    getIntParam(&par, "Transport bitstream?", &dec[0].tsFlag, 0);
    getCondIntParam(&par, "TS?", dec[0].tsFlag,
        "Packet size mode (1-188 2-204)", &tpMode[0], 1);
    getCondIntParam(&par, "TS?", dec[0].tsFlag, "Video PID", &dec[0].pid, 0);

    commentParam(&par, "");
    commentParam(&par, "Bitstream 2 info:");
    getStringParam(&par, "Filename", bsFile[1], "test2.mpg");
    getIntParam(&par, "Byte offset", &skipBytes[1], 0);
    getIntParam(&par, "Transport bitstream?", &dec[1].tsFlag, 0);
    getCondIntParam(&par, "TS?", dec[1].tsFlag,
        "Packet size mode (1-188 2-204)", &tpMode[1], 1);
    getCondIntParam(&par, "TS?", dec[0].tsFlag, "Video PID", &dec[1].pid, 0);

    commentParam(&par, "");
    commentParam(&par, "Alignment:");
    getIntParam(&par, "Use time code alignment?", &tcAlign, 0);
    getCondStringParam(&par, "TC align?", tcAlign, "Stating TC",
        initTc, "hh:mm:ss:pp");
    endParam(&par);


    logFp[0] = stdout;
#ifdef UNIX
    logFp[1] = fopen("/dev/null", "wb");
#else
    logFp[1] = stdout;
#endif

    if (tcAlign) {
        int tcHr, tcMin, tcSec, tcPic;
        char tmp;
        sscanf(initTc, "%d %c %d %c %d %c %d", &tcHr, &tmp, &tcMin, &tmp,
            &tcSec, &tmp, &tcPic);
        tcVal = ((tcHr * 60 + tcMin) * 60 + tcSec) * 30 + tcPic;
        printf("\nInit TC %s", initTc);
    }


    /* MPEG-2 channel setup */
    for (i=0; i<2; i++) {
        dec[i].tpSz = (dec[i].tsFlag && tpMode[i]==2)? TP_SIZE2 : TP_SIZE;
        openInFile(&dec[i].inFile, bsFile[i], dec[i].tpSz);
        fseek(dec[i].inFile.fp, skipBytes[i], SEEK_SET);
        printf("\nInput bitstream %d: %s (byte %d)",
               i, bsFile[i], skipBytes[i]);

        if (dec[i].tsFlag) {
            /* Input is transport stream */
            uint* endPtr = dec[i].inFile.buf + (TP_SIZE>>2);
            dec[i].sysIn = &dec[i].inFile.rdr;
            dec[i].vidIn = (Reader*)malloc(sizeof(Reader));
            initReader(dec[i].vidIn, getMorePesPayload, &dec[i]);
            setBitBuf(&dec[i].vidIn->bitBuf, endPtr, endPtr, 0);
            setReaderEcho(dec[i].sysIn, 0, NULL);
            printf("PID: %d", dec[i].pid);
        }
        else {
            /* Input is video elementary stream */
            dec[i].vidIn = &dec[i].inFile.rdr;
        }
        setReaderEcho(dec[i].vidIn, 0, NULL);

        initDecoder(&dec[i]);

        while (1) {
            if (seekSeqHdr(dec[i].vidIn)) {
                printf("\nError: failed to find SEQ header\n");
                exit(-1);
            }

            if (tcAlign) {
                // TC alignment
                getSeqLayer(dec[i].vidIn, &dec[i].pic, &dec[i].SC);

                /* One time initialization */
                if (dec[i].pic.resetFlag) {
                    initBuffers(&dec[i].pic, dec[i].curFrm);
                    initBuffers(&dec[i].pic, dec[i].newRefFrm);
                    initBuffers(&dec[i].pic, dec[i].oldRefFrm);
                    dec[i].pic.resetFlag = 0;
                }

                if (dec[i].SC == GOP_START_CODE) {
                    getGopLayer(dec[i].vidIn, &dec[i].pic, &dec[i].SC);
                    tcVal2 = ((dec[i].pic.time_code_hours * 60 + 
                               dec[i].pic.time_code_minutes) * 60 + 
                              dec[i].pic.time_code_seconds) * 30 + 
                             dec[i].pic.time_code_pictures;

                    if (tcVal2 < tcVal) {
                        fprintf(stdout, "\nTC %02d:%02d:%02d:%02d  skipped",
                                dec[i].pic.time_code_hours,
                                dec[i].pic.time_code_minutes,
                                dec[i].pic.time_code_seconds,
                                dec[i].pic.time_code_pictures);
                        continue;
                    }

                    fprintf(stdout, "\n  TC %02d:%02d:%02d:%02d  Pass",
                       dec->pic.time_code_hours, dec->pic.time_code_minutes,
                       dec->pic.time_code_seconds, dec->pic.time_code_pictures);
                    dec[i].SC = PIC_START_CODE;
                    break;
                }
            }
            else {
                dec[i].SC = SEQ_START_CODE;
                break;
            }
        }
    }

    initVld();
    init_idct();
    initClipTab();

    while (1) {
        int nRow, nCol, frmSz;
        float mse, mseY, mseCb, mseCr;
        double psnr, psnrY, psnrCb, psnrCr;
        uchar **frmA, **frmB;

        // Decode next picture for each bitstream
        for (i=0; i<2; i++)
            decodeNextPic(&dec[i], 0, logFp[i]);

#if 0
        sprintf(comment, "%s-%d", PIC_TYPE[dec[0].pic.picture_coding_type],
                dec[0].pic.frmNo);
        printf("\n\nDecoding %s...", comment);
#endif

        // Compare two pictures
        comparePics(&dec[0].pic, &dec[1].pic);

        nRow = dec[0].pic.nRow;
        nCol = dec[0].pic.nCol;
        frmSz = nRow * nCol;
        if (dec[0].pic.picture_coding_type==B_PIC) {
            frmA = dec[0].curFrm;
            frmB = dec[1].curFrm;
        }
        else {
            frmA = dec[0].newRefFrm;
            frmB = dec[1].newRefFrm;
        }

        // Compute MSE
        mseY = blockTse(frmA[Y], frmB[Y], nCol, nRow, nCol) / (float)frmSz;
        mseCb = blockTse(frmA[Cb], frmB[Cb], nCol>>1, nRow>>1, nCol>>1)
                    / (float)(frmSz>>2);
        mseCr = blockTse(frmA[Cr], frmB[Cr], nCol>>1, nRow>>1, nCol>>1)
                    / (float)(frmSz>>2);
        mse = (mseY*4 + mseCb + mseCr) / 6;
        printf("\n  MSE: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
               mseY, mseCb, mseCr, mse);

        // Compute PSNR in dB
        //   PSNR = 10 x log_10(255^2 / mse)
        psnrY = 48.1308 - 10 * log10(mseY);
        psnrCb = 48.1308 - 10 * log10(mseCb);
        psnrCr = 48.1308 - 10 * log10(mseCr);
        psnr = 48.1308 - 10 * log10(mse);
        printf("\n  PSNR: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
               psnrY, psnrCb, psnrCr, psnr);
    }
}

