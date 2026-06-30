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
#include "fifo.h"

#ifdef UNIX
#include "display.h"
#endif


#define FULL_DECODE		0
#define MAX_PICS		128


// Access unit info
//
typedef struct {
  int   picCnt;			// picture number (decode order)
  int   esSize;			// Elementary stream size of AU (in bits)
  int   size;			// total (system + ES) AU size (in bits)
  int   picType;		// picture type
  uint  dts;			// input DTS (in 45 kHz)
  uint  localDts;		// DTS mapped to local clock (in 45 kHz)
  uint  arvlTime;		// arrival time
  float avgQs;			// average quant scale
}
AuInfo;


// Decoder info
//
typedef struct {
  InFile inFile;		// input file
  Reader* sysIn;		// system stream reader
  Reader* vidIn;		// video elementary stream reader
  AuInfo* inAu;			// access unit info of current input picture
  AuInfo* outAu;		// access unit info of next output picture
  PicInfo pic;			// sequence, GOP and picture level info
  MbInfo  mb;			// slice and MB level info
  RlInfo  rl[6];		// runlength amplitude info

  uchar*  curFrm[3];		// current frame buffers
  uchar*  newRefFrm[3];		// new reference frame buffers
  uchar*  oldRefFrm[3];		// old reference frame buffers

  int     tsFlag;		// 0-ES, 1-TS
  int     tpSz;			// TP size (188 or 204)
  int     pid;			// PID
  int     state;		// decoder state
  int     SC;			// next start code
  int     picCnt;		// picture count (in coding order)
  int     firstTime;

  int     sysHdrBits;
  int     esHdrBits;
  int     picDataBits;

  // System layer info
  int     bitrate;		// transport stream bitrate
  int     tpCnt;		// number of TP processed so far
  int     tpTime;		// packet time in 27 MHz
  int     seekPcr;		// looking for first PCR?
  int     dtsFlag;		// whether DTS is present
  uint    dts;			// DTS (in 45 kHz)
  uint    localDts;		// DTS mapped to local clock (in 45 kHz)
  uint    tbOffset;		// timebase offset (in 45 kHz)

  // For bitrate measurment
  int     prevTpCnt;		// TP count of previous PCR carrying TP
  uint    prevPcr[2];		// previous PCR
}
Decoder;


// Global variables
char  bsFile[128];   		// bitstream filename
char  decLogFile[128];		// decoder log filename
char  statLogFile[128];		// picture stat log filename
char  vbvLogFile[128];		// VBV log filename
AuInfo  au[MAX_PICS];
Decoder dec;
char    comment[80];
int     vidVerboseLev = 0;
FILE*   decLogFp;
FILE*   statLogFp;
FILE*   vbvLogFp;
int     totQs, totSlcQs, qsCnt;
uint    clkBase = 0;		// local time in 45 kHz
int     clkExt = 0;		// local time in 27 MHz
int     pesPlSz;
int     vbvLevel;
int     vbvInterval;
int     vbvEnd = 0x7FFFFFFF;
int     vbvInSz;
int     vbvOutSz;
int     prevDts;		// DTS of previous picture
int     prevVbvLevel;
Fifo    auFifo;


void initDecoder(Decoder* dec)
{
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

    dec->tpTime = (double)27000000 * TP_SIZE * 8 / dec->bitrate;
}


void pcrUpdate(Decoder* dec, TpInfo* tp, uint localClk)
{
    if (dec->seekPcr) {
        dec->seekPcr = 0;
    }
    else {
        int deltaPcr = (tp->PCR[0] - dec->prevPcr[0]) * 600 +
                           (int)(tp->PCR[1] - dec->prevPcr[1]);
        float estRate = 27000000. * 8 * TP_SIZE
                            * (dec->tpCnt - dec->prevTpCnt) / deltaPcr;
        if (fabs(estRate - dec->bitrate)/dec->bitrate > 0.000016)
            printf("\nBitrate mismatch: Declared %d, Est %d bps, "\
                "diff %d", dec->bitrate, (int)estRate,
                (int)(estRate - dec->bitrate));
    }

    dec->tbOffset = localClk - tp->PCR[0];
    dec->prevPcr[0] = tp->PCR[0];
    dec->prevPcr[1] = tp->PCR[1];
    dec->prevTpCnt = dec->tpCnt;
}


void reportStatus(Decoder* dec)
{
    if (dec->tsFlag)  fprintf(decLogFp, "\nTP %d  ", dec->tpCnt);
    fprintf(decLogFp, "\nES Byte %d\n\n", ftell(dec->inFile.fp));
}


static int getMorePesPayload(Decoder* dec)
{
    TpInfo tp;
    PesInfo pes;

    while (1) {                 /* loop till next TP with right PID is found */
        if (dec->firstTime) {
            if (findTp(dec->inFile.fp, dec->tpSz, 1)) {
                fprintf(decLogFp, "\nFailed to find SYNC.");
                exit (-1);
            }
            fprintf(decLogFp, "\nSync at byte %d", ftell(dec->inFile.fp));
            getNextWord(dec->sysIn);
            seekTp(&tp, dec->sysIn, dec->tpSz, 1);
            dec->firstTime = 0;
        }
        else {
            dec->tpCnt++;
            sprintf(comment, "\n\nPacket %d at byte %d (ES byte %d):",
                    dec->tpCnt, ftell(dec->inFile.fp), dec->vidIn->byteCnt);
            commentReader(dec->sysIn, comment);
            if (getByte(dec->sysIn, "sync") != TP_SYNC) {
                fprintf(decLogFp, "\nLost of TP SYNC!");
                reportStatus(dec);
                exit(-7855);
            }

            // Update local clock
            clkExt += dec->tpTime;
            clkBase += clkExt / 600;
            clkExt = clkExt % 600;
        }

        if (dec->sysIn->errFlag == EOF) {
            printf("\nEOF reached.\n");
            exit(-2707);
        }
 
        /* Get transport packet header */
        getTpHdr(&tp, dec->sysIn);
 
        if (tp.PID == dec->pid) {
            /* Get adaptation field if needed */
            if (tp.adaptation_field_control & 2) {
                getTpAf(&tp, dec->sysIn);

                // Check for PCR
                if (tp.PCR_flag)
                    pcrUpdate(dec, &tp, clkBase);
            }
            dec->sysHdrBits += tp.nbytes << 3;
            pesPlSz = TP_SIZE - tp.nbytes;
 
            /* Get PES header if needed */
            if (tp.payload_unit_start_indicator) {
                if (getBits(dec->sysIn, 24, "start code prefix") != 1)
                    fprintf(decLogFp, "\nExpected start code prefix not found!"\
                            "  Ignored.");
                getPesHdr(&pes, dec->sysIn);
                dec->sysHdrBits += (9 + pes.PES_header_data_length) << 3;
                pesPlSz -= 9 + pes.PES_header_data_length;

                if (pes.PTS_DTS_flags) {
                    dec->dtsFlag = 1;
                    dec->dts = (pes.PTS_DTS_flags & 1)? pes.DTS[0] : pes.PTS[0];
                    dec->localDts = dec->dts + dec->tbOffset;
                }
            }

            // Update VBV level
            if (!(tp.adaptation_field_control & 1)) {
//                dec->sysHdrBits += pesPlSz << 3;
                pesPlSz = 0;
            }

            vbvLevel += pesPlSz << 3;
            vbvInSz += pesPlSz << 3;
            if (dec->picCnt && clkBase >= dec->outAu->localDts) {
                int rate = 5625. * (vbvLevel - prevVbvLevel)
                           / (dec->outAu->localDts - prevDts);
                vbvLevel -= dec->outAu->esSize;
                vbvOutSz += dec->outAu->esSize;
                fprintf(statLogFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x "\
                    "\t%d \t%d \t%d", dec->pid, dec->outAu->picCnt,
                    dec->outAu->picType, dec->outAu->size>>3, dec->outAu->avgQs,
                    dec->outAu->localDts, dec->outAu->arvlTime, vbvLevel>>3,
                    fifoFullness(&auFifo), rate);

                prevVbvLevel = vbvLevel;
                prevDts = dec->outAu->localDts;

                dec->outAu = &au[fifoDelete(&auFifo)];
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

        if (clkBase >= vbvEnd) {
            fprintf(vbvLogFp, "\n%08x \t%d \t%d \t%d",
                    vbvEnd, vbvLevel>>3, vbvInSz>>3, vbvOutSz>>3);
            vbvEnd += vbvInterval;
            vbvInSz = vbvOutSz = 0;
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
                        int totBits;
                        dec->picDataBits = getReaderBitPos(in) - bitPos;
                        totBits = dec->sysHdrBits + dec->esHdrBits + dec->picDataBits;
                        fprintf(logFp, "\n  Total: %d bits(%d, %d, %d), %d blks, "\
			     "AC: %d%c, %2.1f/blk", totBits, dec->sysHdrBits,
                             dec->esHdrBits, dec->picDataBits, nBlks,
                             nAcBits*100/dec->picDataBits, '%',
                             (nAcCoefs==0)? 0. : (float)nAcCoefs/nBlks);
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
    int bitPos = getReaderBitPos(dec->vidIn);

    // Get a AuInfo from FIFO for this input picture
    dec->inAu = &au[fifoAdd(&auFifo)];
    if (auFifo.status == FIFO_OVERFLOW) {
        fprintf(logFp, "\nError: AU FIFO overflow!\n");
        exit(-1);
    }
    if (!dec->picCnt)  dec->outAu = &au[fifoDelete(&auFifo)];

    dec->sysHdrBits = dec->esHdrBits = dec->picDataBits = 0;

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
               printf("\nTC %02d:%02d:%02d:%02d",
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
               if (!dec->picCnt)
                   fprintf(statLogFp, "\nTC %02d:%02d:%02d:%02d",
                       dec->pic.time_code_hours, dec->pic.time_code_minutes,
                       dec->pic.time_code_seconds, dec->pic.time_code_pictures);

               setReaderEcho(dec->vidIn, verboseLev>=PIC, NULL);
               if (getPicLayer(dec->vidIn, &dec->pic, &dec->SC)) {
                   seekSeqHdr(dec->vidIn);		/* resync */
                   break;
               }
               dec->esHdrBits = getReaderBitPos(dec->vidIn) - bitPos;

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
               if (dec->tsFlag)  fprintf(logFp, ", TP %d", dec->tpCnt);

               // Copy picture DTS to AuInfo
               dec->inAu->dts = dec->dts;
               dec->inAu->localDts = dec->localDts;

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
    int   skipBytes;
    int   tpMode;
    int   tcFlag;
    char  tc[12];
    int   tcVal, tcVal2;
    int   bitPos;
    int   frstHdrBits = 0;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2parse.par");
    commentParam(&par, "This program compare two MPEG-2 video bitstreams");
    commentParam(&par, "");
    getStringParam(&par, "Filename", bsFile, "test.mpg");
    getIntParam(&par, "Byte offset", &skipBytes, 0);
    getIntParam(&par, "Bitrate (bps)", &dec.bitrate, 27000000);
    getIntParam(&par, "Transport bitstream?", &dec.tsFlag, 0);
    getCondIntParam(&par, "TS?", dec.tsFlag,
        "Packet size mode (1-188 2-204)", &tpMode, 1);
    getCondIntParam(&par, "TS?", dec.tsFlag, "Video PID", &dec.pid, 0);
    getIntParam(&par, "VBV monitor interval (ms)", &vbvInterval, 100);
    getIntParam(&par, "Sync to time code?", &tcFlag, 0);
    getCondStringParam(&par, "Sync to TC?", tcFlag, "Time code", tc,
        "hh:mm:ss:pp");
    getStringParam(&par, "Decoder log file", decLogFile, "");
    getStringParam(&par, "Picture statistic log file", statLogFile, "");
    getStringParam(&par, "VBV log file", vbvLogFile, "");
    endParam(&par);

    if (!strlen(decLogFile)) {
#ifdef UNIX
        strcpy(decLogFile, "/dev/null");
#else
        strcpy(decLogFile, "dec.log");
#endif
    }
    decLogFp = fopen(decLogFile, "wb");

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


    /* MPEG-2 channel setup */
    dec.tpSz = (dec.tsFlag && tpMode==2)? TP_SIZE2 : TP_SIZE;

    openInFile(&dec.inFile, bsFile, dec.tpSz);
    fseek(dec.inFile.fp, skipBytes, SEEK_SET);
    fprintf(statLogFp, "Input bitstream %s (byte %d), PID %d",
            bsFile, skipBytes, dec.pid);

    if (dec.tsFlag) {
        /* Input is transport stream */
        uint* endPtr = dec.inFile.buf + (TP_SIZE>>2);
        dec.sysIn = &dec.inFile.rdr;
        dec.vidIn = (Reader*)malloc(sizeof(Reader));
        initReader(dec.vidIn, getMorePesPayload, &dec);
        setBitBuf(&dec.vidIn->bitBuf, endPtr, endPtr, 0);
        setReaderEcho(dec.sysIn, 0, NULL);
    }
    else {
        /* Input is video elementary stream */
        dec.vidIn = &dec.inFile.rdr;
    }
    setReaderEcho(dec.vidIn, 0, NULL);

    if (tcFlag) {
        char tmp;
        int tcHr, tcMin, tcSec, tcPic;
        sscanf(tc, "%d %c %d %c %d %c %d", &tcHr, &tmp, &tcMin, &tmp, &tcSec,
               &tmp, &tcPic);
        tcVal = ((tcHr * 60 + tcMin) * 60 + tcSec) * 30 + tcPic;
        fprintf(statLogFp, "\nSync to TC %s", tc);
    }

    initDecoder(&dec);

    fifoInit(&auFifo, MAX_PICS);

    if (seekSeqHdr(dec.vidIn)) {
        fprintf(statLogFp, "\nError: failed to find SEQ header\n");
        exit(-1);
    }

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
                    printf("\nTC %02d:%02d:%02d:%02d  skipped",
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

    vbvLevel = 0;		// empty VBV
//    vbvLevel = dec.vidIn->bitBuf.endPtr - dec.vidIn->bitBuf.curPtr;
    vbvInterval *= 45;		// in 45 kHz ticks
    vbvEnd = vbvInterval;
    vbvInSz = vbvOutSz = 0;

    initVld();
    init_idct();
    initClipTab();

    fprintf(statLogFp, "\nPID \tPicNo \tPicType\tPicSz \tAvgQs \tDTS "
           "\t\tArrival \tVbvLev \tVbvPic \tRate");
    fprintf(vbvLogFp, "\nTime    \tVbvLev \tVbvIn \tVbvOut");

    while (1) {
        decodeNextPic(&dec, 0, decLogFp);

        if (!dec.dtsFlag)
            fprintf(statLogFp, "\nPic %d: DTS missing!", dec.picCnt);

        if (!dec.picCnt)
            dec.esHdrBits += frstHdrBits;

        // Copy picture statistic to input AuInfo
        dec.inAu->picCnt = dec.picCnt;
        dec.inAu->esSize = dec.esHdrBits + dec.picDataBits;
        dec.inAu->size = dec.inAu->esSize + dec.sysHdrBits;
        dec.inAu->picType = dec.pic.picture_coding_type;
        dec.inAu->arvlTime = clkBase;
        dec.inAu->avgQs = qsCnt? (float)totQs/qsCnt : 0.;

        dec.picCnt++;
        dec.dtsFlag = 0;
    }
}

