#include <stdio.h>
#include "util.h"
#include "reader.h"
#include "mpeg2.h"
#include "info.h"
#include "decode.h"
#include "block.h"

static void copyBlk(short *src, uchar *dst, int dstPitch);
static void addBlk(short *src, uchar *dst, int dstPitch);


void invQuantScanDct(PicInfo* pic, MbInfo* mb, RlInfo rl[], short *blk[],
                     int debugFlag)
{
    int i, cc;
    int qs = quantScaleTab[pic->q_scale_type][mb->quantiser_scale_code-1];
    short* scanTab = pic->alternate_scan? altScanTab : zigzagScanTab;
    short* qmat;

    if (mb->intra) {
	/* Intra macroblock processing */
        int intra_dc_mult = 8 >> pic->intra_dc_precision;
        qmat = pic->intra_quantiser_matrix;	/* Note: 4:2:0 seq only */

        for (i=0; i<6; i++) {
            /* Inverse DC prediction (for non-zero DC) */
            if (!rl[i].run[0]) {
                cc = i<4? 0 : (i-4)%2+1;
                mb->dc_dct_pred[cc] += rl[i].level[0];
                rl[i].level[0] = mb->dc_dct_pred[cc];
            }

            /* Inverse quantize and scan */
            invQuantScanIntraBlk(rl+i, scanTab, intra_dc_mult, qs, qmat,
                                 blk[i]);

            /* Inverse DCT */
            idct(blk[i]);
        }
    }
    else {
	/* Inter macroblock processing */
        qmat = pic->non_intra_quantiser_matrix;	/* Note: 4:2:0 seq only */

        for (i=0; i<6; i++) {
            if ((mb->cbp >> (11-i)) & 1) {
                if (debugFlag) {
                    printf("Block %d:\n", i);
                    printf("Runlength:\n");
                    printRlInfo(rl+i);
                }

                /* Inverse quantize and scan */
                invQuantScanInterBlk(rl+i, scanTab, qs, qmat, blk[i]);

                if (debugFlag) {
                    printf("Before IDCT:\n");
                    printShortBlock(blk[i], 8, 8, 8);
                }

                /* Inverse DCT */
                idct(blk[i]);

                if (debugFlag) {
                    printf("After IDCT:\n");
                    printShortBlock(blk[i], 8, 8, 8);
                }
            }
        }
    }
}



int invQuantScanInterBlk(RlInfo* rl, short* scanTab, int mquant, short* qmat,
                         short* blk)
{
    int i, j, k;
    short temp, sum=0;

    for (i=j=0; i<rl->count; i++, j++) {
        for (k=rl->run[i]; k>0; k--)
            blk[*scanTab++] = 0;	/* zeroing out */
        j += rl->run[i];
        temp = rl->level[i];
        temp = (((temp<<1)+(temp>0? 1:-1))*qmat[*scanTab]*mquant) / 32;
			/* Note: not equivalent to >>4 for negative values! */

#if !IMPRECISE
        clip(&temp, -2048, 2047);
        sum ^= temp;
#endif

        blk[*scanTab++] = temp;
    }
    for ( ; j<64; j++)
        blk[*scanTab++] = 0;		/* zeroing out till end of block */

#if !IMPRECISE
    /* Mismatch control */
    if (!(sum&1))  blk[63] ^= 1;
#endif

	/* Rui_B */
	return (0);
	/* Rui_E */
}


int invQuantScanIntraBlk(RlInfo* rl, short* scanTab, int intra_dc_mult,
                         int mquant, short* qmat, short* blk)
{
    int i=0, j, k;
    short temp, sum=0;

    /* Special handling for intra DC term */
    if (!rl->run[0])			/* non-zero DC */
        sum = blk[*scanTab++] = rl->level[i++]*intra_dc_mult;

    for (j=i; i<rl->count; i++, j++) {
        for (k=rl->run[i]; k>0; k--)
            blk[*scanTab++] = 0;	/* zeroing out */
        j += rl->run[i];

        temp = (rl->level[i]*qmat[*scanTab]*mquant) / 16;
			/* Note: not equivalent to >>4 for negative values! */

#if !IMPRECISE
        clip(&temp, -2048, 2047);
        sum ^= temp;
#endif

        blk[*scanTab++] = temp;
    }
    for ( ; j<64; j++)
        blk[*scanTab++] = 0;		/* zeroing out till end of block */

#if !IMPRECISE
    /* Mismatch control */
    if (!(sum&1))  blk[63] ^= 1;
#endif

	/* Rui_B */
	return(0);
	/* RUi_E */
}


void combineMb(PicInfo *pic, MbInfo *mb, uchar *curFrm[], short *blk[])
{
    int i;
    uchar *dst;
    int fldShift, fldAdjust, dstPitch, dx, dy, offset;

    if (pic->picture_structure==FRAME) {
        fldShift = 0;
        fldAdjust = 0;
        dstPitch = pic->nCol << mb->dct_type;
    }
    else {
        fldShift = 1;
        fldAdjust = pic->picture_structure==TOP_FIELD? 0 : 1;
        dstPitch = pic->nCol << 1;
    }

    for (i=0; i<4; i++) {
        if (mb->dct_type)	/* Field DCT */
            dy = mb->pos.y + (i>>1);
        else			/* Field DCT */
            dy = (mb->pos.y + ((i&2)<<2)) << fldShift + fldAdjust;

        dx = mb->pos.x + ((i&1)<<3);
        dst = curFrm[Y] + dy*pic->nCol + dx;

        if (mb->intra)
            copyBlk(blk[i], dst, dstPitch);
        else if (mb->pattern && ((mb->cbp >> (11-i)) & 1))
            addBlk(blk[i], dst, dstPitch);
    }

    /* Note: Assume 4:2:0! */
    dstPitch = pic->nCol >> 1;
    dy = (mb->pos.y << fldShift) >> 1;
    dx = mb->pos.x >> 1;
    offset = dy*(pic->nCol>>1) + dx;

    for ( ; i<6; i++) {
        dst = curFrm[i-3] + offset;
        if (mb->intra)
            copyBlk(blk[i], dst, dstPitch);
        else if (mb->pattern && ((mb->cbp >> (11-i)) & 1))
            addBlk(blk[i], dst, dstPitch);
    }
}


/* Note: assume src pitch is 8 */
static void copyBlk(short *src, uchar *dst, int dstPitch)
{
    int i;
    for (i=8; i>0; i--, src+=8, dst+=dstPitch) {
        dst[0] = clipTab[src[0]];
        dst[1] = clipTab[src[1]];
        dst[2] = clipTab[src[2]];
        dst[3] = clipTab[src[3]];
        dst[4] = clipTab[src[4]];
        dst[5] = clipTab[src[5]];
        dst[6] = clipTab[src[6]];
        dst[7] = clipTab[src[7]];
    }
}


/* Note: assume src pitch is 8 */
static void addBlk(short *src, uchar *dst, int dstPitch)
{
    int i;
    for (i=8; i>0; i--, src+=8, dst+=dstPitch) {
        dst[0] = clipTab[dst[0] + src[0]];
        dst[1] = clipTab[dst[1] + src[1]];
        dst[2] = clipTab[dst[2] + src[2]];
        dst[3] = clipTab[dst[3] + src[3]];
        dst[4] = clipTab[dst[4] + src[4]];
        dst[5] = clipTab[dst[5] + src[5]];
        dst[6] = clipTab[dst[6] + src[6]];
        dst[7] = clipTab[dst[7] + src[7]];
    }
}
