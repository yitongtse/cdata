#include <stdio.h>
#include "util.h"
#include "bitbuf.h"
#include "writer.h"
#include "vlc.h"
#include "mpeg2.h"
#include "info.h"
#include "putmb.h"

/* Global variables
*/

/* Offset into the coefficient Huffman code table
     E.g. the code index is given by (cofOffsetTab[run]+level)
          provided that (run<=MAX_RUN) && (level<max_level[run]),
          MAX_RUN = 31.
*/
int coefOffsetTab[32] = {
      -1,  39,  57,  62,  66,  69,  72,  75,  77,  79,
      81,  83,  85,  87,  89,  91,  93,  95,  96,  97,
      98,  99, 100, 101, 102, 103, 104, 105, 106, 107,
     108, 109
};
 
 
/* Max level table
*/
int maxLevelTab[32] = {
     40, 18,  5,  4,  3,  3,  3,  2,  2,  2,
      2,  2,  2,  2,  2,  2,  2,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1
};


/* VLC tables */
Vlc mba_vlc;
Vlc mbType_I_vlc;
Vlc mbType_P_vlc;
Vlc mbType_B_vlc;
Vlc cbp_vlc;
Vlc mv_vlc;
Vlc dmv_vlc;
Vlc lumaSize_vlc;
Vlc chromaSize_vlc;
Vlc coef0_vlc;
Vlc coef1_vlc;

static char comment[16];


void initVlc()
{
    loadVlc(&mba_vlc, mba_VLC);
    loadVlc(&mbType_I_vlc, mbType_I_VLC);
    loadVlc(&mbType_P_vlc, mbType_P_VLC);
    loadVlc(&mbType_B_vlc, mbType_B_VLC);
    loadVlc(&cbp_vlc, cbp_VLC);
    loadVlc(&mv_vlc, mv_VLC);
    loadVlc(&dmv_vlc, dmv_VLC);
    loadVlc(&lumaSize_vlc, lumaSize_VLC);
    loadVlc(&chromaSize_vlc, chromaSize_VLC);
    loadVlc(&coef0_vlc, coef0_VLC);
    loadVlc(&coef1_vlc, coef1_VLC);
}


void putMb(Writer *out, PicInfo *pic, MbInfo *mb, RlInfo rl[])
{
    int i;
    Vlc* coef_vlc;

    putMbHdr(out, pic, mb);

    /* Select VLD table for DCT coefficients */
    coef_vlc = (pic->intra_vlc_format && mb->intra)? &coef1_vlc : &coef0_vlc;

    /* Assume 4:2:0 here! */
    for (i=0; i<6; i++)
        if ((mb->cbp>>(11-i)) & 1) {
            sprintf(comment, "\n\nBlock %d:", i);
            commentWriter(out, comment);
            putBlkCoef(out, mb->intra, (i<4? &lumaSize_vlc:&chromaSize_vlc),
                       coef_vlc, rl+i, pic->mpeg2Flag);
        }
}


/* Returns -1 if MB header is found. */
int putMbHdr(Writer *out, PicInfo *pic, MbInfo* mb)
{
    int temp;
    int SC;

    commentWriter(out, "\n\nMacroblock header:");

    /* Macroblock address increment */
    temp = mb->mbaIncr - 1;
    while (temp>33) {
        putVlc(out, &mba_vlc, 33, "macroblock_address_increment");
        temp -= 33;
    }
    putVlc(out, &mba_vlc, temp, "macroblock_address_increment");
    sprintf(comment, "MBA=%d", mb->index);
    commentWriter(out, comment);


    /* Macroblock modes
    */

    /* Macroblock type */
    switch (pic->picture_coding_type) {
        case I_PIC:
            putVlc(out, &mbType_I_vlc, mb->type, "macroblock_type");
            temp = mb->type + 1;
            break;

        case P_PIC:
            putVlc(out, &mbType_P_vlc, mb->type, "macroblock_type");
            temp = mb->type + 3;
            break;

        case B_PIC:
            putVlc(out, &mbType_B_vlc, mb->type, "macroblock_type");
            temp = mb->type + 10;
            break;
    }
    commentWriter(out, MB_TYPE[temp]);

    /* Prediction type */
    if (!pic->frame_pred_frame_dct &&
        (mb->motion_forward || mb->motion_backward)) {
        if (pic->picture_structure==FRAME) {
            putBits(out, 2, mb->motion_type, "frame_motion_type");
            commentWriter(out, PRED_TYPE[mb->motion_type]);
        }
        else {
            putBits(out, 2, mb->motion_type, "field_motion_type");
            commentWriter(out, PRED_TYPE[mb->motion_type+4]);
        }
    }

    /* DCT type */
    if (pic->picture_structure==FRAME && !pic->frame_pred_frame_dct
        && (mb->intra || mb->pattern)) {
        putBits(out, 1, mb->dct_type, "dct_type");
        commentWriter(out, DCT_TYPE[mb->dct_type]);
    }

    /* Quant */
    if (mb->quant)
        putBits(out, 5, mb->quantiser_scale_code, "quantiser_scale_code");

    /* Motion vectors */
    if (mb->motion_forward) {
        if (pic->picture_structure==FRAME) {
            switch(mb->motion_type) {
                case FRAME_FRAME_BASED:
                    putFrmMv(out, &mb->mv[0][0], pic->f_code[0],
                             &mb->pmv[0][0]);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;

                case FRAME_FIELD_BASED:
                    putFldMv(out, &mb->mv[0][0], mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 1);
                    putFldMv(out, &mb->mv[0][1], mb->fldSel[0][1],
                             pic->f_code[0], &mb->pmv[0][1], 1);
                    break;

                case FRAME_DUAL_PRIME:
                    putDPMv(out, &mb->mv[0][0], &mb->mv[0][1], pic->f_code[0],
                            &mb->pmv[0][0], 1);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;
            }
        }
        else {
            switch(mb->motion_type) {
                case FIELD_FIELD_BASED:
                    putFldMv(out, &mb->mv[0][0], mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 0);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;

                case FIELD_16X8_MC:
                    putFldMv(out, &mb->mv[0][0], mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 0);
                    putFldMv(out, &mb->mv[0][1], mb->fldSel[0][1],
                             pic->f_code[0], &mb->pmv[0][1], 0);
                    break;

                case FIELD_DUAL_PRIME:
                    putDPMv(out, &mb->mv[0][0], &mb->mv[0][1], pic->f_code[0],
                            &mb->pmv[0][0], 0);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;
            }
        }
    }

    if (mb->motion_backward) {
        if (pic->picture_structure==FRAME) {
            switch(mb->motion_type) {
                case FRAME_FRAME_BASED:
                    putFrmMv(out, &mb->mv[1][0], pic->f_code[1],
                             &mb->pmv[1][0]);
                    copyCoor(&mb->pmv[1][1], &mb->pmv[1][0]);
                    break;

                case FRAME_FIELD_BASED:
                    putFldMv(out, &mb->mv[1][0], mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 1);
                    putFldMv(out, &mb->mv[1][1], mb->fldSel[1][1],
                             pic->f_code[1], &mb->pmv[1][1], 1);
                    break;
            }
        }
        else {
            switch(mb->motion_type) {
                case FIELD_FIELD_BASED:
                    putFldMv(out, &mb->mv[1][0], mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 0);
                    copyCoor(&mb->pmv[1][1], &mb->pmv[1][0]);
                    break;

                case FIELD_16X8_MC:
                    putFldMv(out, &mb->mv[1][0], mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 0);
                    putFldMv(out, &mb->mv[1][1], mb->fldSel[1][1],
                             pic->f_code[1], &mb->pmv[1][1], 0);
                    break;
            }
        }
    }

    /* Reset if necessary */
    if (mb->intra)  resetMvPred(pic, mb);
    else  resetDcDctPred(pic, mb);

    if (pic->picture_coding_type==P_PIC && !mb->motion_forward) {
        setDefaultMotion(pic, mb);
        resetFwdMv(mb);
        resetMvPred(pic, mb);
    }

    /* Coded block pattern */
    if (mb->pattern) {
        putVlc(out, &cbp_vlc, mb->cbp>>6, "coded_block_pattern_420");
        if (pic->chroma_format==CHROMA_FORMAT_422)
            putBits(out, 2, (mb->cbp>>4) & 0xF, "coded_block_pattern_1");
        else if (pic->chroma_format==CHROMA_FORMAT_444)
            putBits(out, 6, mb->cbp & 0x3F, "coded_block_pattern_2");
    }

    return -1;
}


/* Note: halfRange should be 1 for vertical component of field mv
         in frame picture.  Otherwise it should be 0.
*/
void putMv(Writer* out, int *mv, int f_code, int *predMv, int halfRange)
{
    int res_size = f_code-1;
    int max = 16 << res_size;

    /* Motion vector prediction */
    int mvDiff = *mv - *predMv;
    if (mvDiff >= max)  mvDiff -= max<<1;
    else if (mvDiff < -max)  mvDiff += max<<1;

    *predMv = *mv;			/* predictor update */

    sprintf(comment, "Mv:%d  Mv_diff:%d", *mv, mvDiff);
    if (!mvDiff) putVlc(out, &mv_vlc, mvDiff, "motion_code");
    else {
        int sign = 0;
        int residual;
        if (mvDiff<0)  sign = 1, mvDiff = -mvDiff;
        residual = --mvDiff & ((1<<res_size)-1);
        mvDiff = (mvDiff>>res_size) + 1;
        putVlc(out, &mv_vlc, mvDiff, "motion_code");
        putBits(out, 1, sign, "motion_sign");
        if (res_size>0)  putBits(out, res_size, residual, "motion_residual");
    }
    commentWriter(out, comment);
}


void putFrmMv(Writer* out, Coor* mv, int f_code[], Coor *predMv)
{
    putMv(out, &mv->x, f_code[0], &predMv->x, 0);
    putMv(out, &mv->y, f_code[1], &predMv->y, 0);
}


void putFldMv(Writer* out, Coor* mv, int fldSel, int f_code[],
              Coor *predMv, int frame_picture)
{
    putBits(out, 1, fldSel, "field_select");
    putMv(out, &mv->x, f_code[0], &predMv->x, 0);
    if (frame_picture)  predMv->y >>= 1;
    putMv(out, &mv->y, f_code[1], &predMv->y, frame_picture);
    if (frame_picture)  predMv->y <<= 1;
}


/* Put dual-prime motion vectors */
void putDPMv(Writer* out, Coor* mv, Coor* dmv, int f_code[], Coor *predMv,
             int frame_picture)
{
    putMv(out, &mv->x, f_code[0], &predMv->x, 0);
    putVlc(out, &dmv_vlc, dmv->x, "dmvector");
    putMv(out, &mv->y, f_code[1], &predMv->y, frame_picture);
    putVlc(out, &dmv_vlc, dmv->y, "dmvector");
}


/* Put block coefficients */
void putBlkCoef(Writer* out, int intraFlag, Vlc* dcSize_vlc, Vlc* coef_vlc,
                RlInfo* rl, int mpeg2Flag)
{
    int dc_size, dc_range;
    int run, level, sign;
    int i = 0;

    if (intraFlag) {
        /* Intra DC term */
        level = rl->level[0];
        sprintf(comment, "(DC:%d)", level);

        if (level>=0)  sign = 0;
        else  sign = 1, level = -level;

        for (dc_size=0, dc_range=1; level>=dc_range; dc_size++, dc_range<<=1);

        putVlc(out, dcSize_vlc, dc_size, "dct_dc_size");
        if (dc_size)
            putBits(out, dc_size, sign? dc_range-1-level:level,
                    "dct_dc_differential");
        commentWriter(out, comment);
        i++;
    }

    /* Sebsequent DCT coefficients */
    for ( ; i<rl->count; i++) {
        run = rl->run[i];
        level = rl->level[i];
        sprintf(comment, "(RUN:%d LEV:%d)", run, level);

        if (level>=0)  sign = 0;
        else  sign = 1, level = -level;

        /* Check for escape condition */
        if (run>MAX_RUN || level>maxLevelTab[run]) {
            /* Escape code */
            putVlc(out, coef_vlc, COEF_ESC, "escape");
            commentWriter(out, "ESC");
            putBits(out, 6, run, "escaped run");
            if (mpeg2Flag)
                putBits(out, 12, (sign? -level:level) & 0xFFF, "escaped level");
            else
                printf("Error: MPEG-1 mode not supported yet!\n");
        }
        else {
            if (i==0 && run==0 && level==1)
                /* Special handling for first coefficient==1 */
                putBits(out, 1, 1, "run_level");
            else
                putVlc(out, coef_vlc, coefOffsetTab[run]+level, "coef");

            putBits(out, 1, sign, "sign");
        }
        commentWriter(out, comment);
    }

    /* EOB */
    putVlc(out, coef_vlc, COEF_EOB, "EOB");
    commentWriter(out, "EOB");
}

