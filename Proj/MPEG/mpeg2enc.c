/*****************************************************************************
    File: mpeg2enc.c

    MPEG-2 video encoder
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "imageio.h"
#include "stat.h"
#include "bitbuf.h"
#include "reader.h"
#include "writer.h"
#include "outfile.h"
#include "vlc.h"
#include "tp.h"
#include "pes.h"
#include "mpeg2.h"
#include "info.h"
#include "puthdr.h"
#include "putmb.h"
#include "me.h"
#include "encode.h"
#include "decode.h"



void encodeIPic(Writer *out, PicInfo *pic, MbInfo *mb);
void encodePPic(Writer *out, PicInfo *pic, MbInfo *mb);
void encodeBPic(Writer *out, PicInfo *pic, MbInfo *mb,
                int useFwdRef, int useBwdRef);


/* Global data structure */
Writer *sysOut;                 // system stream writer
Writer *vidOut;                 // video elementary stream writer
PicInfo pic;
MbInfo mb;
RlInfo rl[6];			// Note: for 4:2:0 seq only

OutFile outFile;

int vidVerboseLev;

int pid;
				/* Note: Assume 4:2:0 sequence! */
uchar *origFrm[3];		/* original input frame */
uchar *curFrm[3];		/* current frame (reconstructed) */
uchar *newRefFrm[3];		/* new reference frame (coded) */
uchar *oldRefFrm[3];		/* old reference frame (coded) */

uchar *origNewRefLuma;		/* new luma ref. frame (original) */
uchar *origOldRefLuma;		/* old luma ref. frame (original) */

/* Motion search */
int nFrmFullPelOffset;
int nFrmHalfPelOffset;
int nFldFullPelOffset;
int nFldHalfPelOffset;
Coor *frmFullPelOffset;
Coor *frmHalfPelOffset;
Coor *fldFullPelOffset;
Coor *fldHalfPelOffset;
FrmMeResult fwdMeResult, bwdMeResult;


char comment[128];
int qs_code;


/* Mode decision thresholds */
int T1 = 300;			/* replenishment threshold */
int T2 = 400;			/* intra penalty */
int T3 = 100;			/* MC penalty */


/* For debugging purpose only */
int debugFlag;
int debugFrmNo;
int debugMba;
int debugTemp;

uchar fullpelBuf[4*4];
uchar halfpelBuf[7*7];


int main(int argc, char** argv)
{
    Param par;
    char inSeqName[128], recSeqName[128];
    char bitstreamFile[128];
    char sysSyntaxFile[128], vidSyntaxFile[128];
    int sysVerboseLev;
    int firstFrm, lastFrm, frm;
    int bitRate;
    int M, N;
    int i, j;
    int width, height, maxValue;
    int frmSz;
    int useFwdRef;		/* use forward reference frame in B picture? */
    int tsFlag;
    int startupFlag = 1;

    initPicInfo(&pic);

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "mpeg2enc.par");
    commentParam(&par, "MPEG-2 Video Encoder Simulation:");
    commentParam(&par, "");
    getStringParam(&par, "Input sequence name", inSeqName, "test%04d%s.pix");
    getIntParam(&par, "First frame", &firstFrm, 0);
    getIntParam(&par, "Last frame", &lastFrm, 1);
    getIntParam(&par, "Frame width", &pic.horizontal_size, 704);
    getIntParam(&par, "Frame height", &pic.vertical_size, 480);
    getStringParam(&par, "Reconstructed seuqence name", recSeqName,
        "test%04d%s.rec");
    getStringParam(&par, "Bitstream filename", bitstreamFile, "test.mpg2");
    getIntParam(&par, "Transport bitstream?", &tsFlag, 0);
    getCondIntParam(&par, "TS?", tsFlag, "Video PID", &pid, 0);
    getCondIntParam(&par, "TS?", tsFlag, "System syntax verbose level",
        &sysVerboseLev, 0);
    getCondStringParam(&par, "TS?", tsFlag, "System syntax output file",
        sysSyntaxFile, "syntax.sys");
    getIntParam(&par, "Verbose level (0:OFF 1:SEQ 2:GOP 3:PIC 4:SLC 5:MB "\
        "6:ALL)", &vidVerboseLev, 2);
    getStringParam(&par, "Video syntax output file", vidSyntaxFile,
        "syntax.vid");

    commentParam(&par, "");
    commentParam(&par, "Encoding parameters:");
//    getIntParam(&par, "Bit rate (bits/sec)", &bitRate, 1500000);
    getIntParam(&par, "Fixed quantiser code", &qs_code, 4);
    getIntParam(&par, "I-picture interval", &N, 15);
    getIntParam(&par, "Anchor picture interval", &M, 3);
    getIntParam(&par, "Top field first?", &pic.top_field_first, 1);
    getIntParam(&par, "Use frame prediction and DCT?",
                &pic.frame_pred_frame_dct, 0);

    commentParam(&par, "");
    commentParam(&par, "Debugging parameters:");
    getIntParam(&par, "Frame number", &debugFrmNo, -1);
    getIntParam(&par, "Macroblock number", &debugMba, -1);
    endParam(&par);

debugTemp = vidVerboseLev;

    /* Set up motion search parameters */
    setupFullSearchOffset(2, &nFrmFullPelOffset, &frmFullPelOffset);
    setupFullSearchOffset(1, &nFrmHalfPelOffset, &frmHalfPelOffset);
    setupFullSearchOffset(2, &nFldFullPelOffset, &fldFullPelOffset);
    setupFullSearchOffset(1, &nFldHalfPelOffset, &fldHalfPelOffset);


    /* Channel setup */
    openOutFile(&outFile, bitstreamFile, TP_SIZE);
    if (tsFlag) {
        // Output transport stream
        printf("Mode currently not supported.\n");
        exit (-1);
    }
    else {
        // Output video elementary stream
        vidOut = &outFile.wtr;
    }
    setWriterEcho(vidOut, 1, fopen(vidSyntaxFile, "w"));

    pic.mpeg2Flag = 1;
    initVlc();
    init_idct();
    initClipTab();

    /* To prevent memory deallocation */
    curFrm[Y] = curFrm[Cb] = curFrm[Cr] = 0;
    newRefFrm[Y] = newRefFrm[Cb] = newRefFrm[Cr] = 0;
    oldRefFrm[Y] = oldRefFrm[Cb] = oldRefFrm[Cr] = 0;

    pic.resetFlag = 1;
    pic.trBase = pic.nextTrBase = firstFrm;

    /* Set up sequence info */
    pic.aspect_ratio_information = 2;		/* 4:3 */
    pic.frame_rate_code = 4;			/* 29.97Hz */
    pic.bit_rate = bitRate/400;
    pic.vbv_buffer_size = 112;			/* Note: Arbitrarily set here */
    pic.constrained_parameters_flag = 0;
    pic.load_intra_quantiser_matrix = 0;
    pic.load_non_intra_quantiser_matrix = 0;
    pic.profile_and_level_indication = 0x48;	/* MP@ML */
    pic.progressive_sequence = 0;
    pic.chroma_format = CHROMA_FORMAT_420;
    pic.low_delay = 0;
    pic.frame_rate_extension_n = 0;
    pic.frame_rate_extension_d = 0;
    pic.video_format = 2;			/* NTSC */
    pic.color_description = 0;
    initSeq(&pic);
    initQuantMatrices(&pic);
    initBuffers(&pic, curFrm);
    initBuffers(&pic, oldRefFrm);
    initBuffers(&pic, newRefFrm);

    /* Set up original frame buffers */
    frmSz = pic.nCol * pic.nRow;
    if (!(origFrm[Y] = (uchar*)malloc(frmSz)) ||
        !(origFrm[Cb] = (uchar*)malloc(frmSz>>2)) ||
        !(origFrm[Cr] = (uchar*)malloc(frmSz>>2))) {
        printf("Error: Failed to allocate buffers for original frames.\n");
        exit(-1);
    }

    /* Set up picture info */
    pic.full_pel_forward_vector = 0;
    pic.full_pel_backward_vector = 0;
    pic.f_code[0][0] = pic.f_code[0][1]
        = pic.f_code[1][0] = pic.f_code[1][1] = 4;
    pic.intra_dc_precision = 0;
    pic.picture_structure = FRAME;
    pic.concealment_motion_vectors = 0;
    pic.q_scale_type = 0;
    pic.intra_vlc_format = 0;
    pic.alternate_scan = 0;
    pic.repeat_first_field = 0;
    pic.chroma_420_type = pic.progressive_frame = 0;
    pic.composite_display_flag = 0;

    /* Set up macroblock info */
    mb.dct_type = 0;				/* frame DCT */


    while (1) {

        /* Set up GOP info */
        pic.closed_gop = startupFlag;	/* 1st GOP closed */
        pic.broken_link = 0;
        pic.drop_frame_flag = 1;
        pic.time_code_hours = 0;
        pic.time_code_minutes = 0;
        pic.time_code_seconds = 0;
        pic.time_code_pictures = 0;
        initGop(&pic);
        useFwdRef = !pic.closed_gop;

        /* Process each GOP */
        for (i=M-1; i<N; i+=M) {

            /* Anchor units */
            pic.picture_coding_type = (i==M-1)? I_PIC : P_PIC;
            pic.temporal_reference = i;
            initPic(&pic);
            if (pic.frmNo>lastFrm) {
                putSeqEndCode(vidOut);
                putBits(vidOut, 32, 0, NULL);
                closeOutFile(&outFile);
                return 0;
            }
            readFrm(inSeqName, &pic, origFrm);

vidVerboseLev = pic.frmNo==debugFrmNo? 6 : debugTemp;

            if (i==M-1) {
		/* Note: we generate sequence and GOP header here so that
                         there will be no dangling headers when the last frame
                         has been reached.
                */
                setWriterEcho(vidOut, vidVerboseLev>=SEQ, NULL);
                putSeqHdr(&pic, vidOut);
                putSeqExt(&pic, vidOut);

                setWriterEcho(vidOut, vidVerboseLev>=GOP, NULL);
                putGopHdr(&pic, vidOut);

                encodeIPic(vidOut, &pic, &mb);
            }
            else
                encodePPic(vidOut, &pic, &mb);

            startupFlag = 0;

            if (strlen(recSeqName))
                saveFrm(recSeqName, &pic, curFrm);
            swapBuffers(curFrm, newRefFrm, oldRefFrm);
            fflush(vidOut->echoFp);

            /* B pictures */
            for (j=M-1; j>0; j--) {
                pic.picture_coding_type = B_PIC;
                pic.temporal_reference = i - j;
                initPic(&pic);
                readFrm(inSeqName, &pic, origFrm);
                encodeBPic(vidOut, &pic, &mb, useFwdRef, 1);
                if (strlen(recSeqName))
                    saveFrm(recSeqName, &pic, curFrm);
                fflush(vidOut->echoFp);
            }
            useFwdRef = 1;
        }
    }
}



void encodeIPic(Writer *out, PicInfo *pic, MbInfo *mb)
{
    int i;
    int mbTypeCnt[2];

    printf("Encoding frame %d as I-picture\n", pic->frmNo);

    for (i=0; i<2; i++)  mbTypeCnt[i] = 0;
    setWriterEcho(out, vidVerboseLev>=PIC, NULL);
    sprintf(comment, "\nFrame %d:", pic->frmNo);
    commentWriter(out, comment);
    putPicHdr(pic, out);
    putPicCodeExt(pic, out);

    mb->mbaIncr = 1 ;
    mb->type = I_INTRA;
    mb->quant = 0;
    mb->motion_forward = 0;
    mb->motion_backward = 0;
    mb->pattern = 0;
    mb->intra = 1;

    for (mb->index=0; mb->index<pic->nMb; mb->index++) {
debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);

        initMb(pic, mb);

        /* Reset at beginning of slice */
        if (!mb->pos.x) {
            mb->slice_vertical_position = (mb->pos.y>>4) + 1;
            initSlc(pic, mb);
        }

        /* Encoding */
        extractMb(pic, mb, origFrm, curFrm, curBlk, 0);
        fdctMb(curBlk);
        mb->quantiser_scale_code = qs_code;
        quantScanMb(pic, mb, curBlk, rl);

        mbTypeCnt[mb->type]++;

        /* Bitstream generation */
        if (!mb->pos.x) {
            /* Slice header */
            setWriterEcho(out, vidVerboseLev>=SLC, NULL);
            putSlcHdr(mb, out);
            mb->mbaIncr = (mb->pos.x>>4) + 1;
        }

        /* Macroblock header and block data */
        setWriterEcho(out, vidVerboseLev>=MB, NULL);
        putMb(out, pic, mb, rl);

        /* Reconstruction */
        invQuantScanDct(pic, mb, rl, curBlk, debugFlag);
        combineMb(pic, mb, curFrm, curBlk);
    }

    printf("\nMacroblock type statistics:\n");
    printf("  INTRA: %d\n", mbTypeCnt[0]);
    printf("  INTRA_Q: %d\n\n", mbTypeCnt[1]);
}


void encodePPic(Writer *out, PicInfo *pic, MbInfo *mb)
{
    int i;
    int codedMbCnt, mbTypeCnt[7];
    int mbOffset;
    int tse0, tseBest, var;

    printf("Encoding frame %d as P-picture\n", pic->frmNo);

    for (i=0; i<7; i++)  mbTypeCnt[i] = 0;
    setWriterEcho(out, vidVerboseLev>=PIC, NULL);
    sprintf(comment, "\nFrame %d:", pic->frmNo);
    commentWriter(out, comment);
    putPicHdr(pic, out);
    putPicCodeExt(pic, out);

    mb->mbaIncr = 1 ;
    mb->motion_backward = 0;


    for (mb->index=0; mb->index<pic->nMb; mb->index++) {
        debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);
        if (debugFlag)
            printf("\nFrm %d, MB %d:\n", debugFrmNo, debugMba);

        initMb(pic, mb);
        mb->intra = mb->pattern = mb->motion_forward = mb->quant = 0;

        /* Reset at beginning of slice */
        if (!mb->pos.x) {
            mb->slice_vertical_position = (mb->pos.y>>4) + 1;
            initSlc(pic, mb);
        }
 
        /* Compute TSE0 */
        mbOffset = mb->pos.y*pic->nCol + mb->pos.x;
        tse0 = blockTse(*origFrm+mbOffset, *newRefFrm+mbOffset, 16, 16,
                        pic->nCol);

        /* Mode decision */
        if (tse0 < T1) {
            /* SKIPPED */
            mb->type = SKIPPED;
            resetFwdMv(mb);
            setDefaultMotion(pic, mb);
            getPredMb(pic, mb, curFrm, newRefFrm, oldRefFrm, 0);
        }
        else {
            /* Motion estimation */
            frameMe(pic, mb, *origFrm, *newRefFrm, *newRefFrm,
                    nFrmFullPelOffset, frmFullPelOffset, nFrmHalfPelOffset,
                    frmHalfPelOffset, nFldFullPelOffset, fldFullPelOffset,
                    nFldHalfPelOffset, fldHalfPelOffset, &fwdMeResult);
            frmMotionTypeDecision(&fwdMeResult);

            /* Copy motion search result to MbInfo */
            switch (mb->motion_type = fwdMeResult.motion_type) {
                case FRAME_FRAME_BASED:
                     copyCoor(&mb->mv[0][0], &fwdMeResult.frmMv);
                     break;

                case FRAME_FIELD_BASED:
                     mb->fldSel[0][0] = fwdMeResult.fld1Sel;
                     copyCoor(&mb->mv[0][0], &fwdMeResult.fld1Mv);
                     mb->fldSel[0][1] = fwdMeResult.fld2Sel;
                     copyCoor(&mb->mv[0][1], &fwdMeResult.fld2Mv);
                     break;
            }

            /* Form motion prediction in current frame */
            getPredMb(pic, mb, curFrm, newRefFrm, oldRefFrm, 0);

            /* Compute TSEbest */
            tseBest = blockTse(*origFrm+mbOffset, *curFrm+mbOffset, 16, 16,
                               pic->nCol);

            if (tseBest < T1) {
                /* MC_REP */
                mb->motion_forward = 1;  mb->cbp = 0;
            }
            else {
                /* Compute macroblock luma variance */
                var = blockVar(*origFrm+mbOffset, 16, 16, pic->nCol);

                if (tseBest > var + T2) {
                    /* INTRA */
                    mb->intra = 1;
                }
                else if (tse0 >= tseBest+T3) {
                    /* MC_DPCM */
                    mb->motion_forward = mb->pattern = 1;
                }
                else {
                    /* DPCM */
                    /* Note: works only for frame picture! */
                    mb->pattern = 1;
                    mb->motion_type = FRAME_FRAME_BASED;
                    setCoor(&mb->mv[0][0], 0, 0);

                    /* Re-fetch prediction */
                    getPredMb(pic, mb, curFrm, newRefFrm, oldRefFrm, 0);
                }
            }

            if (mb->pattern || mb->intra) {
                /* Rate control (currently use fixed quant) */
                mb->quant = 0;		/* Should be set by rate control */

                extractMb(pic, mb, origFrm, curFrm, curBlk, !mb->intra);
                fdctMb(curBlk);
                mb->quantiser_scale_code = qs_code;
                quantScanMb(pic, mb, curBlk, rl);
            }

            /* Set macroblock attributes according to cbp */
            if (!mb->intra) {
                if (mb->cbp)  mb->pattern = 1;
                else  mb->pattern = mb->quant = 0;
            }

            /* Set macroblock type */
            if (mb->intra)
                mb->type = mb->quant? P_INTRA_Q : P_INTRA;
            else if (mb->motion_forward)
                mb->type = mb->pattern? (mb->quant? P_MC_DPCM_Q : P_MC_DPCM)
                                        : P_MC_REP;
            else
                mb->type = mb->pattern? (mb->quant? P_DPCM_Q : P_DPCM)
                                        : SKIPPED;
        }

        /* Force coded macroblock at beginning and end of slice */
        if (mb->type==SKIPPED && (mb->pos.x==0 || mb->pos.x==pic->nCol-16)) {
            mb->type = P_MC_REP;
            mb->motion_forward = 1;
            mb->cbp = 0;
            resetFwdMv(mb);
            setDefaultMotion(pic, mb);
        }

        if (mb->type==SKIPPED) {
            /* Resets for skipped macroblocks */
            resetMvPred(pic, mb);
            resetFwdMv(mb);
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);

            mb->mbaIncr++;
        }
        else {
            mbTypeCnt[mb->type]++;

            /* Bitstream generation
            */
            if (!mb->pos.x) {
                /* Slice header */
                setWriterEcho(out, vidVerboseLev>=SLC, NULL);
                putSlcHdr(mb, out);
                mb->mbaIncr = (mb->pos.x>>4) + 1;
            }

            /* Macroblock header and block data */
            setWriterEcho(out, vidVerboseLev>=MB, NULL);
            putMb(out, pic, mb, rl);

            /* Reconstruction
            */
            /* Get DCT component */
            if (mb->pattern || mb->intra)
                invQuantScanDct(pic, mb, rl, curBlk, debugFlag);

            /* Combine two components */
            combineMb(pic, mb, curFrm, curBlk);

            mb->mbaIncr = 1;
        }
    }

    /* Report */
    for (i=codedMbCnt=0; i<7; i++)  codedMbCnt += mbTypeCnt[i];

    printf("\nMacroblock type statistics:\n");
    printf("  SKIPPED: %d\n", pic->nMb-codedMbCnt);
    printf("  MC_REP: %d\n", mbTypeCnt[2]);
    printf("  DPCM: %d\n", mbTypeCnt[1]);
    printf("  DPCM_Q: %d\n", mbTypeCnt[5]);
    printf("  MC_DPCM: %d\n", mbTypeCnt[0]);
    printf("  MC_DPCM_Q: %d\n", mbTypeCnt[4]);
    printf("  INTRA: %d\n", mbTypeCnt[3]);
    printf("  INTRA_Q: %d\n\n", mbTypeCnt[6]);
}


void encodeBPic(Writer *out, PicInfo *pic, MbInfo *mb,
                int useFwdRef, int useBwdRef)
{
    int i;
    int codedMbCnt, mbTypeCnt[11];
    int mbOffset;
    int bdirFrmDist, bdirFldDist, minDist;
    int tse0, tseBest, var;

    printf("Encoding frame %d as B-picture\n", pic->frmNo);

    for (i=0; i<11; i++)  mbTypeCnt[i] = 0;
    setWriterEcho(out, vidVerboseLev>=PIC, NULL);
    sprintf(comment, "\nFrame %d:", pic->frmNo);
    commentWriter(out, comment);
    putPicHdr(pic, out);
    putPicCodeExt(pic, out);

    mb->mbaIncr = 1;
    resetMvPred(pic, mb);
    resetDcDctPred(pic, mb);
    mb->motion_forward = mb->motion_backward = 0;


    for (mb->index=0; mb->index<pic->nMb; mb->index++) {
        debugFlag = (pic->frmNo==debugFrmNo && mb->index==debugMba);
        if (debugFlag)
            printf("\nFrm %d, MB %d:\n", debugFrmNo, debugMba);

        initMb(pic, mb);
        mbOffset = mb->pos.y*pic->nCol + mb->pos.x;

        mb->intra = mb->pattern = mb->quant = 0;
            /* Note: motion_forward and motion_backward are not reset here
                     since they are need for skipped MBs.
            */

        /* Reset at beginning of slice */
        if (!mb->pos.x) {
            mb->slice_vertical_position = (mb->pos.y>>4) + 1;
            initSlc(pic, mb);
        }
 
        /* Compute TSE0 */
        /* Note: TSE0 is defined as the total squared error between the input
                 MB and the reconstructed MB assuming coded as skipped MB.
        */
        if (mb->motion_forward || mb->motion_backward) {
            /* Reconstruced as skipped macroblock */
            mb->motion_type = FRAME_FRAME_BASED;  /* Note: frame picture only */
            copyCoor(&mb->mv[0][0], &mb->pmv[0][0]);
            copyCoor(&mb->mv[1][0], &mb->pmv[1][0]);
            getPredMb(pic, mb, curFrm, oldRefFrm, newRefFrm, 0);
            tse0 = blockTse(*origFrm+mbOffset, *curFrm+mbOffset, 16, 16,
                            pic->nCol);
        }
        else  tse0 = LARGE;

        /* Mode decision */
        if (tse0 < T1) {
            mb->type = SKIPPED;			/* SKIPPED */
        }
        else {
            /* Motion estimation */
            /* Forward */
            if (useFwdRef)
                frameMe(pic, mb, *origFrm, *oldRefFrm, *oldRefFrm,
                        nFrmFullPelOffset, frmFullPelOffset, nFrmHalfPelOffset,
                        frmHalfPelOffset, nFldFullPelOffset, fldFullPelOffset,
                        nFldHalfPelOffset, fldHalfPelOffset, &fwdMeResult);
            else
                fwdMeResult.frmDist = fwdMeResult.fldDist = LARGE;

            /* Backward */
            if (useBwdRef)
                frameMe(pic, mb, *origFrm, *newRefFrm, *newRefFrm,
                        nFrmFullPelOffset, frmFullPelOffset, nFrmHalfPelOffset,
                        frmHalfPelOffset, nFldFullPelOffset, fldFullPelOffset,
                        nFldHalfPelOffset, fldHalfPelOffset, &bwdMeResult);
            else
                fwdMeResult.frmDist = fwdMeResult.fldDist = LARGE;

            /* bidirectional */
            if (useFwdRef && useBwdRef) {
                reconstComp(*oldRefFrm, *curFrm, pic->nCol, 16, 16, mb->pos.x,
                    mb->pos.y, fwdMeResult.frmMv.x, fwdMeResult.frmMv.y, 0);
                reconstComp(*newRefFrm, *curFrm, pic->nCol, 16, 16, mb->pos.x,
                    mb->pos.y, bwdMeResult.frmMv.x, bwdMeResult.frmMv.y, 1);
                bdirFrmDist = blockTse(*origFrm+mbOffset, *curFrm+mbOffset,
                                       16, 16, pic->nCol);

                reconstComp(*oldRefFrm+fwdMeResult.fld1Sel*pic->nCol, *curFrm,
                    pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                    fwdMeResult.fld1Mv.x, fwdMeResult.fld1Mv.y, 0);
                reconstComp(*newRefFrm+bwdMeResult.fld1Sel*pic->nCol, *curFrm,
                    pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                    bwdMeResult.fld1Mv.x, bwdMeResult.fld1Mv.y, 1);
                reconstComp(*oldRefFrm+fwdMeResult.fld2Sel*pic->nCol,
                    *curFrm+pic->nCol, pic->nCol<<1, 16, 8, mb->pos.x,
                    mb->pos.y>>1, fwdMeResult.fld2Mv.x, fwdMeResult.fld2Mv.y,
                    0);
                reconstComp(*newRefFrm+bwdMeResult.fld2Sel*pic->nCol,
                    *curFrm+pic->nCol, pic->nCol<<1, 16, 8, mb->pos.x,
                    mb->pos.y>>1, bwdMeResult.fld2Mv.x, bwdMeResult.fld2Mv.y,
                    1);
                bdirFldDist = blockTse(*origFrm+mbOffset, *curFrm+mbOffset,
                                       16, 16, pic->nCol);
            }
            else
                bdirFrmDist = bdirFldDist = LARGE;

            /* Motion type decision
            */
            /* Adding bias to distortions */
            fwdMeResult.fldDist += 100;
            bwdMeResult.fldDist += 100;
            bdirFrmDist += 100;
            bdirFldDist += 300;

            minDist = fwdMeResult.frmDist;
            mb->motion_type = FRAME_FRAME_BASED;
            mb->motion_forward = 1;  mb->motion_backward = 0;
            copyCoor(&mb->mv[0][0], &fwdMeResult.frmMv);
            copyCoor(&mb->mv[1][0], &bwdMeResult.frmMv);

            if (bwdMeResult.frmDist<minDist) {
                minDist = bwdMeResult.frmDist;
                mb->motion_forward = 0;  mb->motion_backward = 1;
            }

            if (bdirFrmDist<minDist) {
                minDist = bdirFrmDist;
                mb->motion_forward = 1;  mb->motion_backward = 1;
            }

            if (fwdMeResult.fldDist<minDist) {
                minDist = fwdMeResult.fldDist;
                mb->motion_type = FRAME_FIELD_BASED;
                mb->motion_forward = 1;  mb->motion_backward = 0;
                mb->fldSel[0][0] = fwdMeResult.fld1Sel;
                copyCoor(&mb->mv[0][0], &fwdMeResult.fld1Mv);
                mb->fldSel[0][1] = fwdMeResult.fld2Sel;
                copyCoor(&mb->mv[0][1], &fwdMeResult.fld2Mv);
            }

            if (bwdMeResult.fldDist<minDist) {
                minDist = bwdMeResult.fldDist;
                mb->motion_type = FRAME_FIELD_BASED;
                mb->motion_forward = 0;  mb->motion_backward = 1;
                mb->fldSel[1][0] = bwdMeResult.fld1Sel;
                copyCoor(&mb->mv[1][0], &fwdMeResult.fld1Mv);
                mb->fldSel[1][1] = bwdMeResult.fld2Sel;
                copyCoor(&mb->mv[1][1], &fwdMeResult.fld2Mv);
            }

            if (bdirFldDist<minDist) {
                minDist = bdirFldDist;
                mb->motion_type = FRAME_FIELD_BASED;
                mb->motion_forward = 1;  mb->motion_backward = 1;
                mb->fldSel[0][0] = fwdMeResult.fld1Sel;
                copyCoor(&mb->mv[0][0], &fwdMeResult.fld1Mv);
                mb->fldSel[0][1] = fwdMeResult.fld2Sel;
                copyCoor(&mb->mv[0][1], &fwdMeResult.fld2Mv);
                mb->fldSel[1][0] = bwdMeResult.fld1Sel;
                copyCoor(&mb->mv[1][0], &fwdMeResult.fld1Mv);
                mb->fldSel[1][1] = bwdMeResult.fld2Sel;
                copyCoor(&mb->mv[1][1], &fwdMeResult.fld2Mv);
            }


            /* Form motion prediction in current frame */
            getPredMb(pic, mb, curFrm, oldRefFrm, newRefFrm, 0);

            /* Compute TSEbest */
            tseBest = blockTse(*origFrm+mbOffset, *curFrm+mbOffset, 16, 16,
                               pic->nCol);

            if (tseBest < T1) {
                mb->cbp = 0;			/* MC_REP */
            }
            else {
                /* Compute macroblock luma variance */
                var = blockVar(*origFrm+mbOffset, 16, 16, pic->nCol);

                if (tseBest > var + T2) {
                    /* INTRA */
                    mb->intra = 1;
                    mb->motion_forward = mb->motion_backward = 0;
                }
                else {
                    /* MC_DPCM */
                    mb->pattern = 1;
                }
            }

            if (mb->pattern || mb->intra) {
                /* Rate control (currently use fixed quant) */
                mb->quant = 0;		/* Should be set by rate control */

                extractMb(pic, mb, origFrm, curFrm, curBlk, !mb->intra);
                fdctMb(curBlk);
                mb->quantiser_scale_code = qs_code;
                quantScanMb(pic, mb, curBlk, rl);
            }

            /* Set macroblock attributes according to cbp */
            if (!mb->intra) {
                if (mb->cbp)  mb->pattern = 1;
                else  mb->pattern = mb->quant = 0;
            }

            /* Set macroblock type */
            if (mb->intra)
                mb->type = mb->quant? B_INTRA_Q : B_INTRA;
            else if (mb->motion_forward) {
                if (mb->motion_backward)
                    mb->type = mb->pattern? (mb->quant? B_MC_DPCM_FB_Q :
                               B_MC_DPCM_FB) : B_MC_REP_FB;
                else
                    mb->type = mb->pattern? (mb->quant? B_MC_DPCM_F_Q :
                               B_MC_DPCM_F) : B_MC_REP_F;
            }
            else /* if (mb->motion_backward) */
                    mb->type = mb->pattern? (mb->quant? B_MC_DPCM_B_Q :
                               B_MC_DPCM_B) : B_MC_REP_B;
        }


        /* Force coded macroblock at beginning and end of slice */
        if (mb->type==SKIPPED && (mb->pos.x==0 || mb->pos.x==pic->nCol-16)) {
            mb->type = mb->motion_forward? (mb->motion_backward? B_MC_REP_FB :
                       B_MC_REP_F) : B_MC_REP_B;
            mb->cbp = 0;
            setDefaultMotion(pic, mb);
        }

        if (mb->type==SKIPPED) {
            /* Resets for skipped macroblocks */
            setDefaultMotion(pic, mb);
            resetDcDctPred(pic, mb);
            mb->mbaIncr++;
        }
        else {
            mbTypeCnt[mb->type]++;

            /* Bitstream generation
            */
            if (!mb->pos.x) {
                /* Slice header */
                putSlcHdr(mb, out);
                mb->mbaIncr = (mb->pos.x>>4) + 1;
            }

            /* Macroblock header and block data */
            setWriterEcho(out, vidVerboseLev>=MB, NULL);
            putMb(out, pic, mb, rl);

            /* Reconstruction
            */
            /* Get DCT component */
            if (mb->pattern || mb->intra)
                invQuantScanDct(pic, mb, rl, curBlk, debugFlag);

            /* Combine two components */
            combineMb(pic, mb, curFrm, curBlk);

            mb->mbaIncr = 1;
        }
    }

    /* Report */
    for (i=codedMbCnt=0; i<11; i++)  codedMbCnt += mbTypeCnt[i];

    printf("\nMacroblock type statistics:\n");
    printf("  SKIPPED: %d\n", pic->nMb-codedMbCnt);
    printf("  MC_REP_FB: %d\n", mbTypeCnt[0]);
    printf("  MC_DPCM_FB: %d\n", mbTypeCnt[1]);
    printf("  MC_REP_B: %d\n", mbTypeCnt[2]);
    printf("  MC_DPCM_B: %d\n", mbTypeCnt[3]);
    printf("  MC_REP_F: %d\n", mbTypeCnt[4]);
    printf("  MC_DPCM_F: %d\n", mbTypeCnt[5]);
    printf("  INTRA: %d\n", mbTypeCnt[6]);
    printf("  MC_DPCM_FB_Q: %d\n", mbTypeCnt[7]);
    printf("  MC_DPCM_F_Q: %d\n", mbTypeCnt[8]);
    printf("  MC_DPCM_B_Q: %d\n", mbTypeCnt[9]);
    printf("  INTRA_Q: %d\n\n", mbTypeCnt[10]);
}

