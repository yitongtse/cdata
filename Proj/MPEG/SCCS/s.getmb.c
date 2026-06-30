h61868
s 00002/00002/00513
d D 2.5 02/05/24 12:26:31 ytse 6 5
c Update
e
s 00014/00010/00501
d D 2.4 02/03/07 16:40:44 ytse 5 4
c Update
e
s 00031/00000/00480
d D 2.3 02/02/25 17:53:46 ytse 4 3
c Update
e
s 00017/00006/00463
d D 2.2 02/02/19 11:12:05 ytse 3 2
c getMb() will return number of blocks, number of coefficients and number of bits for MB.
e
s 00000/00000/00469
d D 2.1 00/08/21 11:04:20 ytse 2 1
c Before supporting Windows
e
s 00469/00000/00000
d D 1.1 99/10/29 15:58:08 yitong 1 0
c date and time created 99/10/29 15:58:08 by yitong
e
u
U
f e 0
t
T
I 1
#include <stdio.h>
#include "util.h"
#include "reader.h"
#include "vld.h"
#include "mpeg2.h"
#include "info.h"
#include "gethdr.h"
#include "getmb.h"


I 4
D 5
#define	DUMP_COEF_MAP		0


E 5
E 4
/* Global variables
*/
static char comment[128];


/* DCT coefficient run and level tables
*/
int runTab[111] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  2,  2,
      2,  2,  2,  3,  3,  3,  3,  4,  4,  4,
      5,  5,  5,  6,  6,  6,  7,  7,  8,  8,
      9,  9, 10, 10, 11, 11, 12, 12, 13, 13,
     14, 14, 15, 15, 16, 16, 17, 18, 19, 20,
     21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
     31  
};

int levelTab[111] = {
      1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
     11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
     21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
     31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
     11, 12, 13, 14, 15, 16, 17, 18,  1,  2,
      3,  4,  5,  1,  2,  3,  4,  1,  2,  3,
      1,  2,  3,  1,  2,  3,  1,  2,  1,  2,
      1,  2,  1,  2,  1,  2,  1,  2,  1,  2,
      1,  2,  1,  2,  1,  2,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1  
};


/* VLD tables */
Vld mba_vld;
Vld mbType_I_vld;
Vld mbType_P_vld;
Vld mbType_B_vld;
Vld cbp_vld;
Vld mv_vld;
Vld dmv_vld;
Vld lumaSize_vld;
Vld chromaSize_vld;
Vld coef0_vld;
Vld coef1_vld;


I 4
#if DUMP_COEF_MAP
I 5
extern int coefMapFlag;
E 5
extern FILE *coefMapFp;
#endif


E 4
void initVld()
{
    buildVld(&mba_vld, mba_VLC, 4);
    buildVld(&mbType_I_vld, mbType_I_VLC, 2);
    buildVld(&mbType_P_vld, mbType_P_VLC, 6);
    buildVld(&mbType_B_vld, mbType_B_VLC, 6);
    buildVld(&cbp_vld, cbp_VLC, 8);
    buildVld(&mv_vld, mv_VLC, 6);
    buildVld(&dmv_vld, dmv_VLC, 2);
    buildVld(&lumaSize_vld, lumaSize_VLC, 5);
    buildVld(&chromaSize_vld, chromaSize_VLC, 5);
    buildVld(&coef0_vld, coef0_VLC, 8);
    buildVld(&coef1_vld, coef1_VLC, 8);
}


/* Get the macroblock_address_increment field from a macroblock header
   Returns MBA_incr read, or 0 is start code found
*/
int getMbaIncr(Reader *in, int* SC)
{
    int temp;
    int mbaIncr = 1;		/* use 1 here because the VLD index is actually
				   1 less than the real increment value */

    commentReader(in, "\n\nMacroblock header:");

    /* Macroblock address increment */
    temp = getVld(in, &mba_vld, "macroblock_address_increment");

    if (temp==MBA_SC) {
        /* 8 zeros read.  Start code expected */
        byteAlignReader(in);
        temp = (getByte(in, "") << 8) | getByte(in, "");
        while (temp!=1 && !in->errFlag)
            temp = ((temp<<8) | getByte(in, "")) & 0x00ffffff;
        *SC = getByte(in, "start_code");
        return 0;
    }

    while (temp==MBA_ESC) {
        mbaIncr += 33;
        temp = getVld(in, &mba_vld, "maroblock_address_increment");
    }

    mbaIncr += temp;
    return mbaIncr;
}


/* Get macroblock header right after the MBA_increment field */
int getMbHdr(Reader *in, PicInfo *pic, MbInfo* mb)
{
    int temp;

    sprintf(comment, "MBA=%d", mb->index);
    commentReader(in, comment);

    /* Macroblock modes
    */

    /* Macroblock type */
    switch (pic->picture_coding_type) {
        case I_PIC:
            mb->type = getVld(in, &mbType_I_vld, "macroblock_type");
            temp = mb->type + 1;
            break;

        case P_PIC:
            mb->type = getVld(in, &mbType_P_vld, "macroblock_type");
            temp = mb->type + 3;
            break;

        case B_PIC:
            mb->type = getVld(in, &mbType_B_vld, "macroblock_type");
            temp = mb->type + 10;
            break;
    }

if (temp>20)
  printf("Wrong MB type %d for picture type %d\n", temp, pic->picture_coding_type);

    commentReader(in, MB_TYPE[temp]);
    setMbAttrib(mb, temp);

    /* Prediction type */
    if (mb->motion_forward || mb->motion_backward) {
        if (pic->picture_structure==FRAME) {
            if (!pic->frame_pred_frame_dct) {
                mb->motion_type = getBits(in, 2, "frame_motion_type");
                commentReader(in, PRED_TYPE[mb->motion_type]);
            }
            else  mb->motion_type = FRAME_FRAME_BASED;
        }
        else {
            mb->motion_type = getBits(in, 2, "field_motion_type");
            commentReader(in, PRED_TYPE[mb->motion_type+4]);
        }
    }

    /* DCT type */
    if (pic->picture_structure==FRAME && pic->frame_pred_frame_dct==0
        && (mb->intra || mb->pattern)) {
        mb->dct_type = getBits(in, 1, "dct_type");
        commentReader(in, DCT_TYPE[mb->dct_type]);
    }
    else  mb->dct_type = 0;

    /* Quant */
    if (mb->quant)
        mb->quantiser_scale_code = getBits(in, 5, "quantiser_scale_code");

    /* Motion vectors */
    if (mb->motion_forward) {
        if (pic->picture_structure==FRAME) {	/* Frame picture */
            switch(mb->motion_type) {
                case FRAME_FRAME_BASED:
                    getFrmMv(in, &mb->mv[0][0], pic->f_code[0], &mb->pmv[0][0]);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;

                case FRAME_FIELD_BASED:
                    getFldMv(in, &mb->mv[0][0], &mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 1);
                    getFldMv(in, &mb->mv[0][1], &mb->fldSel[0][1],
                             pic->f_code[0], &mb->pmv[0][1], 1);
                    break;

                case FRAME_DUAL_PRIME:
                    getDPMv(in, &mb->mv[0][0], &mb->mv[0][1], pic->f_code[0],
                            &mb->pmv[0][0], 1);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;
            }
        }
        else {
            switch(mb->motion_type) {		/* Field picture */
                case FIELD_FIELD_BASED:
                    getFldMv(in, &mb->mv[0][0], &mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 0);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;

                case FIELD_16X8_MC:
                    getFldMv(in, &mb->mv[0][0], &mb->fldSel[0][0],
                             pic->f_code[0], &mb->pmv[0][0], 0);
                    getFldMv(in, &mb->mv[0][1], &mb->fldSel[0][1],
                             pic->f_code[0], &mb->pmv[0][1], 0);
                    break;

                case FIELD_DUAL_PRIME:
                    getDPMv(in, &mb->mv[0][0], &mb->mv[0][1], pic->f_code[0],
                            &mb->pmv[0][0], 0);
                    copyCoor(&mb->pmv[0][1], &mb->pmv[0][0]);
                    break;
            }
        }
    }

    if (mb->motion_backward) {			/* Frame picture */
        if (pic->picture_structure==FRAME) {
            switch(mb->motion_type) {
                case FRAME_FRAME_BASED:
                    getFrmMv(in, &mb->mv[1][0], pic->f_code[1], &mb->pmv[1][0]);
                    copyCoor(&mb->pmv[1][1], &mb->pmv[1][0]);
                    break;

                case FRAME_FIELD_BASED:
                    getFldMv(in, &mb->mv[1][0], &mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 1);
                    getFldMv(in, &mb->mv[1][1], &mb->fldSel[1][1],
                             pic->f_code[1], &mb->pmv[1][1], 1);
                    break;
            }
        }
        else {					/* Field picture */
            switch(mb->motion_type) {
                case FIELD_FIELD_BASED:
                    getFldMv(in, &mb->mv[1][0], &mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 0);
                    copyCoor(&mb->pmv[1][1], &mb->pmv[1][0]);
                    break;

                case FIELD_16X8_MC:
                    getFldMv(in, &mb->mv[1][0], &mb->fldSel[1][0],
                             pic->f_code[1], &mb->pmv[1][0], 0);
                    getFldMv(in, &mb->mv[1][1], &mb->fldSel[1][1],
                             pic->f_code[1], &mb->pmv[1][1], 0);
                    break;
            }
        }
    }

//    checkEOF(in, "macroblock header");

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
        mb->cbp = getVld(in, &cbp_vld, "coded_block_pattern_420") << 6;
        if (pic->chroma_format==CHROMA_FORMAT_422)
            mb->cbp |= getBits(in, 2, "coded_block_pattern_1") << 4;
        else if (pic->chroma_format==CHROMA_FORMAT_444)
            mb->cbp |= getBits(in, 6, "coded_block_pattern_2");
    }
    else if (mb->intra)  mb->cbp = 0x0FFF;
    else  mb->cbp = 0;

    return -1;
}


/* Returns -1 if MB is successfully decoded.
   Otherwise returns the block number which has error.
I 3
   The routine will update nBlk, nCoefs, and nBits.
E 3
*/
D 3
int getMb(Reader *in, PicInfo* pic, MbInfo* mb, RlInfo rl[])
E 3
I 3
int getMb(Reader *in, PicInfo* pic, MbInfo* mb, RlInfo rl[],
          int* nBlks, int* nCoefs, int* nBits)
E 3
{
D 3
    int i;
    int SC;
E 3
I 3
    int i, SC;
E 3
    Vld* coef_vld;

    SC = getMbHdr(in, pic, mb);
    if (SC!=-1)  return SC;

    /* Select VLD table for DCT coefficients */
    coef_vld = (pic->intra_vlc_format && mb->intra)? &coef1_vld : &coef0_vld;

    /* Decode all blocks */
    /* Note: We assume 4:2:0 here for the moment! */
    for (i=0; i<6; i++) {
I 4
#if DUMP_COEF_MAP
D 5
        fprintf(coefMapFp, "\n%d %d %d %d%d%d%d%d:", pic->frmNo, mb->index, i,
                mb->quant, mb->motion_forward, mb->motion_backward,
E 5
I 5
        if (coefMapFlag)
            fprintf(coefMapFp, "\n%d %d %d %d%d%d%d%d:", pic->frmNo, mb->index,
                i, mb->quant, mb->motion_forward, mb->motion_backward,
E 5
                mb->pattern, mb->intra);
#endif
E 4
        if ((mb->cbp>>(11-i)) & 1) {
            sprintf(comment, "\n\nBlock %d:", i);
            commentReader(in, comment);
D 3
            if (getBlkCoef(in, mb->intra, (i<4? &lumaSize_vld:&chromaSize_vld),
                           coef_vld, rl+i, pic->mpeg2Flag))
E 3
I 3
            (*nBlks)++;
            if (getBlkCoef(in, mb->intra,
                    (i<4? &lumaSize_vld : &chromaSize_vld), coef_vld,
                    rl+i, pic->mpeg2Flag, nCoefs, nBits))
E 3
                return i;
        }
    }

    return -1;
}


/* Get motion vector, do inverse prediction, and update corresp. predictor */
/* Note: mv will store the read motion vector value.
         The mv to be used for prediction will be in pmv.
*/
void getMv(Reader* in, int* mv, int f_code, int* predMv)
{
    int sign, residual=0;
    int res_size = f_code-1;
    int max = 16 << res_size;

    *mv = getVld(in, &mv_vld, "motion_code");
    if (*mv) {
        sign = getBits(in, 1, "motion_sign");
        if (res_size>0)  residual = getBits(in, res_size, "motion_residual");
        *mv = ((*mv-1)<<res_size) + residual + 1;
        if (sign)  *mv = -*mv;
    }

    /* Inverse motion vector prediction */
    *mv += *predMv;
    if (*mv >= max)  *mv -= max<<1;
    else if (*mv < -max)  *mv += max<<1;
    *predMv = *mv;				/* update predictor */
}


void getFrmMv(Reader *in, Coor *mv, int f_code[], Coor *predMv)
{
    getMv(in, &mv->x, f_code[0], &predMv->x);
    getMv(in, &mv->y, f_code[1], &predMv->y);
    sprintf(comment, "\n%46cMv_diff:(%d,%d)  Mv:(%d,%d)", ' ',
            mv->x, mv->y, predMv->x, predMv->y);
    commentReader(in, comment);
}


void getFldMv(Reader *in, Coor *mv, int *fldSel, int f_code[], Coor *predMv,
              int frame_picture)
{
    *fldSel = getBits(in, 1, "field_select");
    getMv(in, &mv->x, f_code[0], &predMv->x);
    if (frame_picture)  predMv->y >>= 1;
    getMv(in, &mv->y, f_code[1], &predMv->y);
    if (frame_picture)  predMv->y <<= 1;
    sprintf(comment, "\n%46cMv_diff:(%d,%d)  Mv:(%d,%d)", ' ',
            mv->x, mv->y, predMv->x, predMv->y);
    commentReader(in, comment);
}


/* Get dual-prime motion vectors */
void getDPMv(Reader *in, Coor *mv, Coor *dmv, int f_code[], Coor *predMv,
             int frame_picture)
{
    getMv(in, &mv->x, f_code[0], &predMv->x);
    dmv->x = getVld(in, &dmv_vld, "dmvector");
    if (frame_picture)  predMv->y >>= 1;
    getMv(in, &mv->y, f_code[1], &predMv->y);
    if (frame_picture)  predMv->y <<= 1;
    dmv->y = getVld(in, &dmv_vld, "dmvector");
}


/* Get block coefficients
   Return 0 if successful.  Otherwise return 1.
I 3
   The routine will update nCoefs and nBits.
   Note: nCoefs and nBits only count AC coefficients!
   
E 3
*/
int getBlkCoef(Reader* in, int intraFlag, Vld* dcSize_vld, Vld* coef_vld,
D 3
               RlInfo* rl, int mpeg2Flag)
E 3
I 3
               RlInfo* rl, int mpeg2Flag, int* nCoefs, int* nBits)
E 3
{
    int dc_size, dc_range;
    int run, level, sign;
    int coefIdx;			/* index of decoded coefficient (RLA) */
I 3
    int bitPos;				// mark input buffer
E 3

I 4
#if DUMP_COEF_MAP
    int coefPos = 0;			// coefficient position
#endif

E 4
    rl->count = 0;			/* initialize RlInfo */

    if (intraFlag) {
        /* Intra DC term */
        dc_size = getVld(in, dcSize_vld, "dct_dc_size");
        dc_range = 1 << dc_size;
        level = dc_size? getBits(in, dc_size, "dct_dc_differential") : 0;
        if (level < (dc_range>>1))  level -= dc_range - 1;
        addRl(rl, 0, level);
        sprintf(comment, "(DC:%d)", level);
        commentReader(in, comment);
I 4

#if DUMP_COEF_MAP
D 5
        fprintf(coefMapFp, " %d", coefPos++);
E 5
I 5
        if (coefMapFlag)
            fprintf(coefMapFp, " %d", coefPos++);
E 5
#endif
E 4
    }

I 3
D 6
    bitPos = getBitPos(in);		// mark input buffer
E 6
I 6
    bitPos = getReaderBitPos(in);	// mark input buffer
E 6

E 3
    while (1) {
        coefIdx = getVld(in, coef_vld, "coef");
//        checkEOF(in, "DCT coefficients");

        if (!rl->count) {
            /* Special handling for the first coefficient */
            if (coefIdx==FIRST_COEF_P1) {
                addRl(rl, 0, 1);
                commentReader(in, "(RUN:0 LEV:1)");
I 4
#if DUMP_COEF_MAP
D 5
            fprintf(coefMapFp, " %d", coefPos++);
E 5
I 5
                if (coefMapFlag)
                    fprintf(coefMapFp, " %d", coefPos++);
E 5
#endif
E 4
                continue;
            }
            else if (coefIdx==FIRST_COEF_M1) {
                addRl(rl, 0, -1);
                commentReader(in, "(RUN:0 LEV:-1)");
I 4
#if DUMP_COEF_MAP
D 5
            fprintf(coefMapFp, " %d", coefPos++);
E 5
I 5
                if (coefMapFlag)
                    fprintf(coefMapFp, " %d", coefPos++);
E 5
#endif
E 4
                continue;
            }
        }

        /* Check for EOB */
        if (coefIdx==COEF_EOB) {
            commentReader(in, "EOB");
I 3
D 6
            *nBits += getBitPos(in) - bitPos;
E 6
I 6
            *nBits += getReaderBitPos(in) - bitPos;
E 6
E 3
            return 0;
        }

        /* Check for escape */
        else if (coefIdx==COEF_ESC) {
            commentReader(in, "ESC");
            run = getBits(in, 6, "escaped run");

            if (mpeg2Flag) {
                /* MPEG-2 escape level */
                level = getBits(in, 12, "escape level");
                if (!(level & 0x7FF)) {
                    printf("Error: Illegal escape level %d\n", level);
                    return 1;
                }
                if (level>2047)  level -= 4096;
            }
            else {
                /* MPEG-1 escape level */
                printf("MPEG-1 mode not supported yet!\n");
                return 1;
            }

            sign = 0;		/* no sign bit following escaped level */
        }

        /* Check for error */
        else if (coefIdx==COEF_ERR) {
            printf("Too many 0's encountered while decoding a coefficient!\n");
            return 1;
        }

        /* Regular run-level */
        else {
            sign = getBits(in, 1, "sign");
            run = runTab[coefIdx];
            level = levelTab[coefIdx];
            if (sign)  level = -level;
        }

        sprintf(comment, "(RUN:%d LEV:%d)", run, level);
        commentReader(in, comment);
I 3
        (*nCoefs)++;
E 3

        if (addRl(rl, run, level)) {
            printf("Error: Too many coefficients in a block!\n");
            return 1;
        }
I 4
#if DUMP_COEF_MAP
D 5
        coefPos += run;
        fprintf(coefMapFp, " %d", coefPos++);
E 5
I 5
        if (coefMapFlag) {
            coefPos += run;
            fprintf(coefMapFp, " %d", coefPos++);
        }
E 5
#endif
E 4
    }
}
E 1
