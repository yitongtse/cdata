h19414
s 00001/00001/00659
d D 1.6 03/03/21 14:19:55 ytse 6 5
c backup
e
s 00067/00023/00593
d D 1.5 02/05/24 12:26:46 ytse 5 4
c Update
e
s 00038/00005/00578
d D 1.4 02/04/08 17:13:32 ytse 4 3
c Backup before adding TC alignment
e
s 00011/00018/00572
d D 1.3 02/03/21 13:28:41 ytse 3 2
c Backup for NEW2
e
s 00106/00053/00484
d D 1.2 02/03/13 14:26:07 ytse 2 1
c Update
e
s 00537/00000/00000
d D 1.1 02/03/13 12:47:23 ytse 1 0
c date and time created 02/03/13 12:47:23 by ytse
e
u
U
f e 0
t
T
I 1
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

D 5
#ifdef UNIX_ENV
E 5
I 5
#ifdef UNIX
E 5
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
D 3
int totQs, totSlcQs, qsCnt;  // to compute average picture quant
int qsCnt = 0;
E 3
I 2
D 5
FILE* logFp;
FILE* nullFp;
E 5
I 5
FILE* logFp[2];
E 5
I 4
int totQs, totSlcQs, qsCnt;
E 4
E 2


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
D 2
static int decodePic(Decoder *dec)
E 2
I 2
static int decodePic(Decoder *dec, FILE* logFp)
E 2
{
    Reader* in = dec->vidIn;
    PicInfo* pic = &dec->pic;
    MbInfo* mb = &dec->mb;

D 5
    int nextMba;
E 5
    int errCode;
    int mbaIncr;
    uchar **fwdRefFrm, **bwdRefFrm;
    int nBlks, nAcCoefs, nAcBits;
D 4
    int bitPos = getBitPos(in);		// mark beginning of picture
E 4
I 4
    int bitPos = getReaderBitPos(in);	// mark beginning of picture
E 4

I 4
    totQs = totSlcQs = qsCnt = 0;
E 4
D 3
    totQs = totSlcQs = qsCnt = 0;
E 3
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
I 4
    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 4
D 3
    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];

E 3
    initSlc(pic, mb);

    /* Get first MBA_increment */
    setReaderEcho(in, vidVerboseLev>=MB, NULL);
    mbaIncr = getMbaIncr(in, &dec->SC);
    mb->index = mb->mbaRef + mbaIncr;
    if (mb->index != 0) {
        /* Either mbaIncr!=1 or another start code found */
D 2
        printf("\nError: Wrong first slice start position %d", mbaIncr);
E 2
I 2
        fprintf(logFp, "\nError: Wrong first slice start position %d", mbaIncr);
E 2
        reportStatus(dec);
        exit(-937);
    }

    while (1) {
        if (mb->index>=pic->nMb) {
D 2
            printf("\nError: Too many macroblocks in picture %d!",
E 2
I 2
            fprintf(logFp, "\nError: Too many macroblocks in picture %d!",
E 2
                   pic->frmNo);
D 2
            printf("\n  MB index: %d    # MBs in picture: %d",
E 2
I 2
            fprintf(logFp, "\n  MB index: %d    # MBs in picture: %d",
E 2
                   mb->index, pic->nMb);
            reportStatus(dec);
            exit(-18);
        }

        initMb(pic, mb);

        if (mbaIncr==1) {	/* Process coded macroblock */
            /* Read macroblock header (after mba_incr field) and coefficients */
            errCode = getMb(in, pic, mb, dec->rl, &nBlks, &nAcCoefs, &nAcBits);
            if (errCode != -1) {
D 2
                printf("\nError when decoding MB %d block %d.",
E 2
I 2
                fprintf(logFp, "\nError when decoding MB %d block %d.",
E 2
                       mb->index, errCode);
                reportStatus(dec);
                exit(-32);
            }

I 4
            // Compute average quant scale
            if (mb->pattern || mb->intra) {
                qsCnt++;
                totQs += quantScaleTab[pic->q_scale_type]\
                                      [mb->quantiser_scale_code-1];
            }

E 4
D 3
            // Compute average quant
            if (mb->pattern || mb->intra) {
                qsCnt++;
                totQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
            }

E 3
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
I 4
                    totSlcQs += quantScaleTab[pic->q_scale_type]\
                                             [mb->quantiser_scale_code-1];
E 4
                    initSlc(pic, mb);
D 3
                    totSlcQs += quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
E 3

                    /* Get next MBA_incr */
                    setReaderEcho(in, vidVerboseLev>=MB, NULL);
                    mbaIncr = getMbaIncr(in, &dec->SC);
                    if (mbaIncr==-1) {
D 2
                        printf("\nError: Illegal MBA increment found");
E 2
I 2
                        fprintf(logFp, "\nError: Illegal MBA increment found");
E 2
                        reportStatus(dec);
                        exit(-346);
                    }

                    /* Compute and check slice start position */
                    if (mb->mbaRef + mbaIncr != mb->index) {
D 2
                        printf("\nError: Wrong slice start position: "\
E 2
I 2
                        fprintf(logFp, "\nError: Wrong slice start position: "\
E 2
                               "%d (%d, %d), expected %d", mb->mbaRef+mbaIncr,
                               mb->slice_vertical_position, mbaIncr, mb->index);
                        reportStatus(dec);
                        exit(-721);
                    }
                }

D 2
                else if (pic->mbCnt<pic->nMb) {
						/* non-slice start code */
                    printf("\nError: unexpected start code 0x000001%02x "\
                           "within picture", dec->SC);
                    printf("\n  current MB: %d", pic->mbCnt);
E 2
I 2
                else if (pic->mbCnt<pic->nMb) { /* non-slice start code */
                    fprintf(logFp, "\nError: unexpected start code "\
                        "0x000001%02x within picture", dec->SC);
                    fprintf(logFp, "\n  current MB: %d", pic->mbCnt);
E 2
                }

D 2
                else switch (dec->SC) {
                    case SEQ_START_CODE:
                    case GOP_START_CODE:
                    case PIC_START_CODE:
                    case SEQ_END_CODE:
                       {
                         int totBits = getBitPos(in) - bitPos;
                         printf("\n  Total: %d bits, %d blks, AC: %d%c, %2.1f/blk",
                             totBits, nBlks, nAcBits*100/totBits, '%',
                             (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
                         return dec->SC;
                       }

                    default:
                         printf("\nError: Unexpected start code 0x000001%02x "\
                                "at end of picture\n", dec->SC);
                         return dec->SC;
E 2
I 2
                else {
                    if (dec->SC==PIC_START_CODE || dec->SC==SEQ_START_CODE
                        || dec->SC==GOP_START_CODE || dec->SC==SEQ_END_CODE) {
D 4
                        int totBits = getBitPos(in) - bitPos;
E 4
I 4
                        int totBits = getReaderBitPos(in) - bitPos;
E 4
                        fprintf(logFp, "\n  Total: %d bits, %d blks, AC: %d%c,"\
                             " %2.1f/blk", totBits, nBlks, nAcBits*100/totBits,
                             '%', (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
                    }
                    return dec->SC;
E 2
                }
            }
        }
    }
}


D 2
int decodeNextPic(Decoder *dec, int verboseLev)
E 2
I 2
int decodeNextPic(Decoder *dec, int verboseLev, FILE* logFp)
E 2
{
    long fileLoc = ftell(dec->inFile.fp);

    /* Find next state based on next SC */
    switch (dec->SC) {
      case SEQ_START_CODE: dec->state = SEQ;  break;
      case GOP_START_CODE: dec->state = GOP;  break;
      case PIC_START_CODE: dec->state = PIC;  break;
D 2
      default: printf("\nError: Unrecognized SC %d", dec->SC);  return (-1);
E 2
I 2
      default: fprintf(logFp, "\nError: Unrecognized SC %d", dec->SC);
               return (-1);
E 2
    }

    while (1) {
        switch (dec->state) {
          case SEQ:
D 2
               printf("\n\nSEQ Header found");
E 2
I 2
D 4
               fprintf(logFp, "\n\nSEQ Header found");
E 4
I 4
               fprintf(logFp, "\n\n\nSEQ Header found");
E 4
E 2
               setReaderEcho(dec->vidIn, verboseLev>=SEQ, NULL);
               if (getSeqLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }

               /* One time initialization */
               if (dec->pic.resetFlag) {
D 2
                   printf("\nInitialize buffers...");
E 2
I 2
                   fprintf(logFp, "\nInitialize buffers...");
E 2
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
D 2
                       printf("\nGOP or picture header expected!");
E 2
I 2
                       fprintf(logFp, "\nGOP or picture header expected!");
E 2
                       seekSeqHdr(dec->vidIn);	/* resync */
               }
               break;

          case GOP:
D 2
               printf("\n\nGOP Header found");
E 2
I 2
D 4
               fprintf(logFp, "\n\nGOP Header found");
E 4
I 4
               fprintf(logFp, "\nGOP Header found");
E 4
E 2
               setReaderEcho(dec->vidIn, verboseLev>=GOP, NULL);
               if (getGopLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }
D 2
               printf("\n  TC %02d:%02d:%02d:%02d, drop %d\n",
E 2
I 2
D 4
               fprintf(logFp, "\n  TC %02d:%02d:%02d:%02d, drop %d\n",
E 4
I 4
D 5
               fprintf(logFp, "\n  TC %02d:%02d:%02d:%02d, drop %d",
E 5
I 5
               fprintf(logFp, "\n  TC %02d:%02d:%02d:%02d",
E 5
E 4
E 2
                   dec->pic.time_code_hours, dec->pic.time_code_minutes,
D 5
                   dec->pic.time_code_seconds, dec->pic.time_code_pictures,
                   dec->pic.drop_frame_flag);
E 5
I 5
                   dec->pic.time_code_seconds, dec->pic.time_code_pictures);
E 5

               /* Find next state */
               if (dec->SC==PIC_START_CODE)
                   dec->state = PIC;
               else {
D 2
                   printf("\nGOP or picture header expected!");
E 2
I 2
                   fprintf(logFp, "\nGOP or picture header expected!");
E 2
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
D 2
                   printf("\nError: Invalid slice start code 0x000001%02x "\
                          "found!", dec->SC);
E 2
I 2
                   fprintf(logFp, "\nError: Invalid slice start code "\
                       "0x000001%02x found!", dec->SC);
E 2
                   reportStatus(dec);
                   exit(-283);
               }

               if (dec->pic.picture_structure!=FRAME) {
D 2
                   printf("\nSorry!  Field picture not supported yet!");
E 2
I 2
                   fprintf(logFp, "\nSorry!  Field picture not supported yet!");
E 2
                   reportStatus(dec);
                   exit(-267);
               }

               sprintf(comment, "%s-%d",
D 2
                   PIC_TYPE[dec->pic.picture_coding_type], dec[0].pic.frmNo);
E 2
I 2
                   PIC_TYPE[dec->pic.picture_coding_type], dec->pic.frmNo);
E 2
               commentReader(dec->vidIn, "\nPic ");
               commentReader(dec->vidIn, comment);
               commentReader(dec->vidIn, "...");

D 2
               printf("\n\nDecoding %s...", comment);
               printf("\n  File loc %d", fileLoc);
               if (dec->tsFlag)  printf(", TP %d", tpCnt);
E 2
I 2
               fprintf(logFp, "\n\nDecoding %s...", comment);
               fprintf(logFp, "\n  File loc %d", fileLoc);
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", tpCnt);
E 2

               /* Decode a picture */
D 2
               dec->SC = decodePic(&dec[0]);
E 2
I 2
               dec->SC = decodePic(dec, logFp);
E 2

I 4
               fprintf(logFp, "\n  qs %.1f, slcQs %.1f",
                   (qsCnt? (float)totQs/qsCnt : 0.),
                   (float)totSlcQs/dec->pic.nMbRow);

E 4
D 2
               printf("\n  qs %.1f, slcQs %.1f",
E 2
I 2
               if (dec->pic.picture_coding_type!=B_PIC)
                   swapBuffers(dec->curFrm, dec->newRefFrm, dec->oldRefFrm);

D 3
               fprintf(logFp, "\n  qs %.1f, slcQs %.1f",
E 2
                   (qsCnt? (float)totQs/qsCnt : 0.),
                   (float)totSlcQs/dec->pic.nMbRow);

E 3
               return 0;
        }
    }
}


I 2
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


E 2
int main(int argc, char** argv)
{
    Param par;
    char  bsFile[2][FILENAME_SZ];   	// bitstream 1 filename
I 4
    char  initTc[12];
    int   tcAlign;
D 5
    int   tcVal;
E 5
I 5
    int   tcVal, tcVal2;
E 5
E 4
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

I 4
    commentParam(&par, "");
    commentParam(&par, "Alignment:");
    getIntParam(&par, "Use time code alignment?", &tcAlign, 0);
    getCondStringParam(&par, "TC align?", tcAlign, "Stating TC",
        initTc, "hh:mm:ss:pp");
E 4
    endParam(&par);


I 2
D 5
    logFp = stdout;
    nullFp = fopen("/dev/null", "wb");
E 5
I 5
    logFp[0] = stdout;
#ifdef UNIX
    logFp[1] = fopen("/dev/null", "wb");
#else
    logFp[1] = stdout;
#endif
E 5

I 4
    if (tcAlign) {
        int tcHr, tcMin, tcSec, tcPic;
        char tmp;
        sscanf(initTc, "%d %c %d %c %d %c %d", &tcHr, &tmp, &tcMin, &tmp,
            &tcSec, &tmp, &tcPic);
        tcVal = ((tcHr * 60 + tcMin) * 60 + tcSec) * 30 + tcPic;
I 5
        printf("\nInit TC %s", initTc);
E 5
    }


E 4
E 2
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
D 5
        if (seekSeqHdr(dec[i].vidIn)) {
            printf("\nError: failed to find SEQ header\n");
            exit(-1);
E 5
I 5

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
E 5
        }
D 5
        dec[i].SC = SEQ_START_CODE;
E 5
    }

    initVld();
    init_idct();
    initClipTab();

    while (1) {
        int nRow, nCol, frmSz;
        float mse, mseY, mseCb, mseCr;
I 3
D 6
        float psnr, psnrY, psnrCb, psnrCr;
E 6
I 6
        double psnr, psnrY, psnrCb, psnrCr;
E 6
E 3
I 2
D 5
        uchar **frm0, **frm1;
E 5
I 5
        uchar **frmA, **frmB;
E 5
E 2

        // Decode next picture for each bitstream
        for (i=0; i<2; i++)
D 2
            decodeNextPic(&dec[i], 0);
E 2
I 2
D 5
            decodeNextPic(&dec[i], 0, i==0? nullFp : logFp);
E 5
I 5
            decodeNextPic(&dec[i], 0, logFp[i]);
E 5
E 2

I 2
#if 0
        sprintf(comment, "%s-%d", PIC_TYPE[dec[0].pic.picture_coding_type],
                dec[0].pic.frmNo);
        printf("\n\nDecoding %s...", comment);
#endif

        // Compare two pictures
        comparePics(&dec[0].pic, &dec[1].pic);

E 2
D 3
        // Compute MSE
E 3
        nRow = dec[0].pic.nRow;
        nCol = dec[0].pic.nCol;
        frmSz = nRow * nCol;
I 2
        if (dec[0].pic.picture_coding_type==B_PIC) {
D 5
            frm0 = dec[0].curFrm;
            frm1 = dec[1].curFrm;
E 5
I 5
            frmA = dec[0].curFrm;
            frmB = dec[1].curFrm;
E 5
        }
        else {
D 5
            frm0 = dec[0].newRefFrm;
            frm1 = dec[1].newRefFrm;
E 5
I 5
            frmA = dec[0].newRefFrm;
            frmB = dec[1].newRefFrm;
E 5
        }
E 2

I 3
        // Compute MSE
E 3
D 2
        mseY = blockTse(dec[0].curFrm[Y], dec[1].curFrm[Y], nCol, nRow, nCol)
                   / (float)frmSz;
        mseCb = blockTse(dec[0].curFrm[Cb], dec[1].curFrm[Cb], nCol>>1,
                   nRow>>1, nCol>>1) / (float)(frmSz>>2);
        mseCr = blockTse(dec[0].curFrm[Cr], dec[1].curFrm[Cr], nCol>>1,
                   nRow>>1, nCol>>1) / (float)(frmSz>>2);
E 2
I 2
D 5
        mseY = blockTse(frm0[Y], frm1[Y], nCol, nRow, nCol) / (float)frmSz;
        mseCb = blockTse(frm0[Cb], frm1[Cb], nCol>>1, nRow>>1, nCol>>1)
E 5
I 5
        mseY = blockTse(frmA[Y], frmB[Y], nCol, nRow, nCol) / (float)frmSz;
        mseCb = blockTse(frmA[Cb], frmB[Cb], nCol>>1, nRow>>1, nCol>>1)
E 5
                    / (float)(frmSz>>2);
D 5
        mseCr = blockTse(frm0[Cr], frm1[Cr], nCol>>1, nRow>>1, nCol>>1)
E 5
I 5
        mseCr = blockTse(frmA[Cr], frmB[Cr], nCol>>1, nRow>>1, nCol>>1)
E 5
                    / (float)(frmSz>>2);
E 2
        mse = (mseY*4 + mseCb + mseCr) / 6;
D 3

E 3
        printf("\n  MSE: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
               mseY, mseCb, mseCr, mse);
I 3

        // Compute PSNR in dB
        //   PSNR = 10 x log_10(255^2 / mse)
        psnrY = 48.1308 - 10 * log10(mseY);
        psnrCb = 48.1308 - 10 * log10(mseCb);
        psnrCr = 48.1308 - 10 * log10(mseCr);
        psnr = 48.1308 - 10 * log10(mse);
        printf("\n  PSNR: Y %.2f  Cb %.2f  Cr %.2f  Tot %.2f",
               psnrY, psnrCb, psnrCr, psnr);
E 3
    }
}

E 1
