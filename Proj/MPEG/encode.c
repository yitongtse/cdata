#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "param.h"
#include "util.h"
#include "bitbuf.h"
#include "writer.h"
#include "vlc.h"
#include "mpeg2.h"
#include "info.h"
#include "puthdr.h"
#include "putmb.h"


static void extractBlk(uchar *src1, uchar *src2, short *dst,
                       int srcPitch, int subFlag);
static int quantIntraBlk(short *blk, int intra_dc_mult, int mquant,
                         short *qmat);
static int quantInterBlk(short *blk, int mquant, short *qmat);
static void scanBlk(RlInfo *rl, short *scanTab, short *blk, int intraFlag);

void fdct(short* block);



void extractMb(PicInfo *pic, MbInfo *mb, uchar *origFrm[], uchar *predFrm[],
               short *blk[], int subFlag)
{
    int i;
    uchar *src1, *src2;
    int fldShift, fldAdjust, srcPitch, dx, dy, offset;
 
    if (pic->picture_structure==FRAME) {
        fldShift = 0;
        fldAdjust = 0;
        srcPitch = pic->nCol << mb->dct_type;
    }
    else {
        fldShift = 1;
        fldAdjust = pic->picture_structure==TOP_FIELD? 0 : 1;
        srcPitch = pic->nCol << 1;
    }

    for (i=0; i<4; i++) {
        if (mb->dct_type)       /* Field DCT */
            dy = mb->pos.y + (i>>1);
        else                    /* Field DCT */
            dy = (mb->pos.y + ((i&2)<<2)) << fldShift + fldAdjust;
 
        dx = mb->pos.x + ((i&1)<<3);
        offset = dy*pic->nCol + dx;
        src1 = origFrm[Y] + offset;
        src2 = predFrm[Y] + offset;

        extractBlk(src1, src2, blk[i], srcPitch, subFlag);
    }

    /* Note: Assume 4:2:0! */
    srcPitch = pic->nCol >> 1;
    dy = (mb->pos.y << fldShift) >> 1;
    dx = mb->pos.x >> 1;
    offset = dy*(pic->nCol>>1) + dx;
 
    for ( ; i<6; i++) {
        src1 = origFrm[i-3] + offset;
        src2 = predFrm[i-3] + offset;
        extractBlk(src1, src2, blk[i], srcPitch, subFlag);
    }
}


/* subFlag == 0:  dst = src1
   subFlag == 1:  dst = src1 - src2
*/
static void extractBlk(uchar *src1, uchar *src2, short *dst,
                       int srcPitch, int subFlag)
{
    int x, y;
    srcPitch -= 8;

    if (subFlag)
        for (y=0; y<8; y++, src1+=srcPitch, src2+=srcPitch)
            for (x=0; x<8; x++)
                *dst++ = *src1++ - *src2++;
    else
        for (y=0; y<8; y++, src1+=srcPitch)
            for (x=0; x<8; x++)
                *dst++ = *src1++;
}


/* Note: assume 4:2:0! */
void fdctMb(short *blk[])
{
    int i;
    for (i=0; i<6; i++)
        fdct(blk[i]);
}


void quantScanMb(PicInfo *pic, MbInfo *mb, short *curBlk[], RlInfo rl[])
{
    int i, cc;
    short *qmat;
    int qs = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
    short *scanTab = pic->alternate_scan? altScanTab : zigzagScanTab;

    if (mb->intra) {
        int dc_dct_pred[3];		/* local copy for encoder */
        int intra_dc_mult = 8 >> pic->intra_dc_precision;
        qmat = pic->intra_quantiser_matrix;	/* Note: 4:2:0 seq only */
        for (cc=0; cc<3; cc++)
            dc_dct_pred[cc] = mb->dc_dct_pred[cc];

        for (i=0; i<6; i++) {
            quantIntraBlk(curBlk[i], intra_dc_mult, qs, qmat);
            scanBlk(rl+i, scanTab, curBlk[i], 1);

            /* DC prediction (for non-zero DC) */
            /* Note: predictors in MbInfo is not updated since their original
	             values are needed for reconstruction */
            if (!rl[i].run[0]) {
                cc = i<4? 0 : (i-4)%2+1;
                rl[i].level[0] -= dc_dct_pred[cc];
                dc_dct_pred[cc] += rl[i].level[0];
            }
        }
        mb->cbp = 0x0FFF;
    }
    else {
        qmat = pic->non_intra_quantiser_matrix;	/* Note: 4:2:0 seq only */
        mb->cbp = 0;				/* reset cbp */
        for (i=0; i<6; i++)
            if (quantInterBlk(curBlk[i], qs, qmat)) {
                scanBlk(rl+i, scanTab, curBlk[i], 0);
                mb->cbp |= 1 << (11-i);
            }
    }
}


static int quantIntraBlk(short *blk, int intra_dc_mult, int mquant, short *qmat)
{
    int i, x, d;

    /* DC term */
    x = *blk;
    if (x<0)  x = -x;
    d = intra_dc_mult;
    x = (x + (d>>1)) / d;
    *blk = *blk<0? -x : x;

    /* AC terms */
    for (i=63; i>0; i--) {
        x = *++blk;
        if (x<0)  x = -x;
        d = mquant * *++qmat;
        x = ((x<<4) + ((3*d)>>3)) / d;
        if (x>2047)  x = 2047;			/* clipping */
						/* Note: MPEG-2 only */
        *blk = *blk<0? -x : x;
    }
}


static int quantInterBlk(short *blk, int mquant, short *qmat)
{
    int i, x, d;
    int nonZero = 0;

    for (i=64; i>0; i--, blk++) {
        x = *blk;
        if (x<0)  x = -x;
        d = *qmat++;
        x = ((x<<5) + (d>>1)) / d;
        x /= (mquant<<1);
        if (x>2047)  x = 2047;			/* clipping */
						/* Note: MPEG-2 only */
        if (x)  nonZero = 1;
        *blk = *blk<0? -x : x;
    }
    return nonZero;
}


static void scanBlk(RlInfo *rl, short *scanTab, short *blk, int intraFlag)
{
    int i=0, j=0, run=0;

    if (intraFlag) {
        /* Always assign DC term to an RL, even when DC=0 */
        /* Note: otherwise putBlkCoef() will not work when DC=0 */
        rl->run[j] = 0;
        rl->level[j++] = blk[*scanTab++];
        i++;
    }
    for ( ; i<64; i++) {
        if (rl->level[j]=blk[*scanTab++])  rl->run[j++]=run, run=0;
        else  run++;
    }
    rl->count = j;
}


/* Update DC predictors by doing DC prediction
   Note: Since the encoding routines will not update DC predictors, this step
         is necessary for the encoder if reconstruction is not performed.
   Note: 4:2:0 only!
*/
void updateDcDctPred(MbInfo *mb, RlInfo rl[])
{
    if (!rl[0].run[0])  mb->dc_dct_pred[Y] += rl[0].level[0];
    if (!rl[1].run[0])  mb->dc_dct_pred[Y] += rl[1].level[0];
    if (!rl[2].run[0])  mb->dc_dct_pred[Y] += rl[2].level[0];
    if (!rl[3].run[0])  mb->dc_dct_pred[Y] += rl[3].level[0];
    if (!rl[4].run[0])  mb->dc_dct_pred[Cb] += rl[4].level[0];
    if (!rl[5].run[0])  mb->dc_dct_pred[Cr] += rl[5].level[0];
}

