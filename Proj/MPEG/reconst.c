#include <stdio.h>
#include "util.h"
#include "mpeg2.h"
#include "info.h"
#include "decode.h"


/* Note: This file is modified so that mv now is the actual value after
         inverse mv prediction, not the PMV value.  The PMV value is different
         from the mv value in case of field motion vector in frame picture!
*/

void reconst(uchar *src[], int sfield, uchar *dst[], int dfield, int pitch,
             int width, int height, int x, int y, int dx, int dy, int addFlag,
             int chroma_format);

void reconstComp(uchar *src, uchar *dst, int pitch, int width, int height,
                 int x, int y, int dx, int dy, int addFlag);

void getPredMb(
    PicInfo *pic,
    MbInfo *mb,
    uchar *curFrm[],
    uchar *fwdRefFrm[],
    uchar *bwdRefFrm[],
    int second_field		/* 1 if this is a second field in a frame */
)
{
    int m, cur_field;
    uchar **predFrm;	/* Note: not sure if this will work or not */
    Coor *mvs;		/* motion vectors in a particular direction */
    Coor oppMv;		/* mv of opposite parity prediction for dual-prime */
    int curParity;
    int addFlag = 0;

    if (mb->motion_forward || pic->picture_coding_type==P_PIC) {
				/* Note: this routine should not be called
				         for INTRA macroblock in P-picture */
        mvs = mb->mv[0];		/* forward motion vectors */

        if (pic->picture_structure==FRAME) {	/* Frame picture */
            switch (mb->motion_type) {

                case FRAME_FRAME_BASED:		/* Frame-based prediction */
                     reconst(fwdRefFrm, 0, curFrm, 0, pic->nCol, 16, 16,
                         mb->pos.x, mb->pos.y, mvs[0].x, mvs[0].y, 0,
                         pic->chroma_format);
                     break;

                case FRAME_FIELD_BASED:		/* Field-based prediction */
                     /* Top field */
                     reconst(fwdRefFrm, mb->fldSel[0][0], curFrm, 0,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                         mvs[0].x, mvs[0].y, 0, pic->chroma_format);

                     /* Bottom field */
                     reconst(fwdRefFrm, mb->fldSel[0][1], curFrm, 1,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                         mvs[1].x, mvs[1].y, 0, pic->chroma_format);
                     break;

                case FRAME_DUAL_PRIME:		/* Dual-prime prediction */
                     /* Predict top field from top field */
                     reconst(fwdRefFrm, 0, curFrm, 0, pic->nCol<<1, 16, 8,
                         mb->pos.x, mb->pos.y>>1, mvs[0].x, mvs[0].y, 0,
                         pic->chroma_format);

                     /* Predict bottom field from bottom field */
                     reconst(fwdRefFrm, 1, curFrm, 1, pic->nCol<<1, 16, 8,
                         mb->pos.x, mb->pos.y>>1, mvs[0].x, mvs[0].y, 0,
                         pic->chroma_format);

                     /* Predict and add top field from bottom field */
                     m = pic->top_field_first? 1 : 3;
                     oppMv.x = mvs[0].x;    oppMv.y = mvs[0].y;
                     oppMv.x = ((m*oppMv.x+(oppMv.x>0))>>1) + mvs[1].x;
                     oppMv.y = ((m*oppMv.y+(oppMv.y>0))>>1) + mvs[1].y - 1;
                     reconst(fwdRefFrm, 1, curFrm, 0, pic->nCol<<1, 16, 8,
                         mb->pos.x, mb->pos.y>>1, oppMv.x, oppMv.y, 1,
                         pic->chroma_format);

                     /* Predict and add bottom field from top field */
                     m = pic->top_field_first? 3 : 1;
                     oppMv.x = mvs[0].x;    oppMv.y = mvs[0].y>>1;
                     oppMv.x = ((m*oppMv.x+(oppMv.x>0))>>1) + mvs[1].x;
                     oppMv.y = ((m*oppMv.y+(oppMv.y>0))>>1) + mvs[1].y + 1;
                     reconst(fwdRefFrm, 0, curFrm, 1, pic->nCol<<1, 16, 8,
                         mb->pos.x, mb->pos.y>>1, oppMv.x, oppMv.y, 1,
                         pic->chroma_format);
                     break;
            }
        }

        else {		/* Field picture */
            cur_field = pic->picture_structure==BOTTOM_FIELD;
            predFrm = (pic->picture_coding_type==P_PIC && second_field &&
                       cur_field!=mb->fldSel[0][0])? curFrm : bwdRefFrm;

            switch (mb->motion_type) {
                case FIELD_FIELD_BASED:		/* Field-based prediction */
                     reconst(predFrm, mb->fldSel[0][0], curFrm, cur_field,
                         pic->nCol<<1, 16, 16, mb->pos.x, mb->pos.y,
                         mvs[0].x, mvs[0].y, 0, pic->chroma_format);
                     break;

                case FIELD_16X8_MC:		/* 16x8 prediction */
                     /* Upper half */
                     reconst(predFrm, mb->fldSel[0][0], curFrm, cur_field,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y,
                         mvs[0].x, mvs[0].y, 0, pic->chroma_format);

                     /* Lower half */
                     predFrm = (pic->picture_coding_type==P_PIC &&
                                second_field && cur_field!=mb->fldSel[0][0])?
                               curFrm : bwdRefFrm;
                     reconst(predFrm, mb->fldSel[0][1], curFrm, cur_field,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y+8,
                         mvs[1].x, mvs[1].y, 0, pic->chroma_format);
                     break;

                case FIELD_DUAL_PRIME:		/* Dual-prime prediction */
                     /* Predict from same parity field */
                     reconst(fwdRefFrm, cur_field, curFrm, cur_field,
                         pic->nCol<<1, 16, 16, mb->pos.x, mb->pos.y,
                         mvs[0].x, mvs[0].y, 0, pic->chroma_format);

                     /* Predict and add from opposite parity field */
                     predFrm = second_field? curFrm : fwdRefFrm;
                     oppMv.x = mvs[0].x;    oppMv.y = mvs[0].y>>1;
                     oppMv.x = ((oppMv.x+(oppMv.x>0))>>1) + mvs[1].x;
                     oppMv.y = ((oppMv.y+(oppMv.y>0))>>1) + mvs[1].y
                               + (pic->picture_structure==TOP_FIELD)? -1:1;
                     reconst(predFrm, !cur_field, curFrm, cur_field,
                         pic->nCol<<1, 16, 16, mb->pos.x, mb->pos.y,
                         oppMv.x, oppMv.y, 1, pic->chroma_format);

                     break;
            }
        }

        addFlag = 1;
    }


    if (mb->motion_backward) {
        mvs = mb->mv[1];		/* backward motion vectors */

        if (pic->picture_structure==FRAME) {	/* Frame picture */
            switch (mb->motion_type) {

                case FRAME_FRAME_BASED:		/* Frame-based prediction */
                     reconst(bwdRefFrm, 0, curFrm, 0, pic->nCol, 16, 16,
                         mb->pos.x, mb->pos.y, mvs[0].x, mvs[0].y, addFlag,
                         pic->chroma_format);
                     break;

                case FRAME_FIELD_BASED:		/* Field-based prediction */
                     /* Top field */
                     reconst(bwdRefFrm, mb->fldSel[1][0], curFrm, 0,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                         mvs[0].x, mvs[0].y, addFlag, pic->chroma_format);

                     /* Bottom field */
                     reconst(bwdRefFrm, mb->fldSel[1][1], curFrm, 1,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y>>1,
                         mvs[1].x, mvs[1].y, addFlag, pic->chroma_format);
                     break;
            }
        }

        else {		/* Field picture */
            switch (mb->motion_type) {
                case FIELD_FIELD_BASED:		/* Field-based prediction */
                     reconst(bwdRefFrm, mb->fldSel[1][0], curFrm, curParity,
                         pic->nCol<<1, 16, 16, mb->pos.x, mb->pos.y, mvs[0].x,
                         mvs[0].y, addFlag, pic->chroma_format);
                     break;

                case FIELD_16X8_MC:		/* 16x8 prediction */
                     /* Upper half */
                     reconst(bwdRefFrm, mb->fldSel[1][0], curFrm, curParity,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y, mvs[0].x,
                         mvs[0].y, addFlag, pic->chroma_format);
                     /* Lower half */
                     reconst(bwdRefFrm, mb->fldSel[1][1], curFrm, curParity,
                         pic->nCol<<1, 16, 8, mb->pos.x, mb->pos.y+8, mvs[1].x,
                         mvs[1].y, addFlag, pic->chroma_format);
                     break;
            }
        }
    }
}


void reconst(
    uchar *src[],		/* source buffers */
    int sfield,			/* source field number (0 or 1) */
    uchar *dst[],		/* destination buffers */
    int dfield,			/* destination field number */
    int pitch,			/* line pitch (buffer width) */
    int width, int height,	/* block size */
    int x, int y,		/* block position */
    int dx, int dy,		/* prediction block offset */
    int addFlag,		/* add prediction to destination? */
    int chroma_format		/* chroma format */
)
{
    /* Y component */
    reconstComp(src[Y]+(sfield? pitch>>1:0), dst[Y]+(dfield? pitch>>1:0),
                pitch, width, height, x, y, dx, dy, addFlag);

    if (chroma_format!=CHROMA_FORMAT_444)
        pitch>>=1, width>>=1, x>>=1, dx/=2;
    if (chroma_format==CHROMA_FORMAT_420)
        height>>=1, y>>=1, dy/=2;

    /* Cb component */
    reconstComp(src[Cb]+(sfield? pitch>>1:0), dst[Cb]+(dfield? pitch>>1:0),
                pitch, width, height, x, y, dx, dy, addFlag);

    /* Cr component */
    reconstComp(src[Cr]+(sfield? pitch>>1:0), dst[Cr]+(dfield? pitch>>1:0),
                pitch, width, height, x, y, dx, dy, addFlag);
}


/* Note: this routine has been modified so that the pixel reconstruction
         in mpeg2play is used here.  (It used reconstruction in mpeg2decode
         before).  However, we still need to verify whether this is 100%
         MPEG-2 compliant.
*/
void reconstComp(
    uchar *src,			/* source buffer */
    uchar *dst,			/* destination buffer */
    int pitch,			/* line pitch (buffer width) */
    int width, int height,	/* block width and height */
    int x, int y,		/* block position (top-left corner) */
    int dx, int dy,		/* prediction block offset */
    int addFlag			/* add prediction to destination? */
)
{
    int dx_int, dx_h, dy_int, dy_h;	/* integer/half components of offset */
    uchar *s, *d;
    int i, j;
    int t1, t2;

    dx_int = dx >> 1;
    dx_h = dx & 1;
    dy_int = dy >> 1;
    dy_h = dy & 1;

    s = src + (y+dy_int)*pitch + (x+dx_int);
    d = dst + y*pitch + x;

    if (!dx_h && !dy_h) {
        if (addFlag)
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1; j>=0; j--)
                    d[j] = (uint)(d[j] + s[j] + 1) >> 1;

        else
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1; j>=0; j--)
                    d[j] = s[j];
    }

    else if (!dx_h && dy_h) {
        if (addFlag)
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1; j>=0; j--) {
                    d[j] = (d[j] + ((uint)(s[j]+s[j+pitch]+1)>>1) + 1) >> 1;
                }
        else
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1; j>=0; j--)
                    d[j] = (uint)(s[j]+s[j+pitch]+1)>>1;
    }

    else if (dx_h && !dy_h) {
        if (addFlag)
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1, t1=s[width]; j>=0; j--) {
                    d[j] = (d[j] + ((uint)(t1+(t2=s[j])+1)>>1) + 1) >> 1;
                    j--;
                    d[j] = (d[j] + ((uint)(t2+(t1=s[j])+1)>>1) + 1) >> 1;
                }
        else
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1, t1=s[width]; j>=0; j--) {
                    d[j] = (uint)(t1+(t2=s[j])+1) >> 1;
                    j--;
                    d[j] = (uint)(t2+(t1=s[j])+1) >> 1;
                }
    }

    else {	/* if (dx_h && dy_h) */
        if (addFlag)
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1, t1=s[width]+s[width+pitch]; j>=0; j--) {
                    d[j] = (d[j] + ((uint)(t1+(t2=s[j]+s[j+pitch])+2)>>2)+1)>>1;
                    j--;
                    d[j] = (d[j] + ((uint)(t2+(t1=s[j]+s[j+pitch])+2)>>2)+1)>>1;
                }
        else
            for (i=height; i>0; i--, s+=pitch, d+=pitch)
                for (j=width-1, t1=s[width]+s[width+pitch]; j>=0; j--) {
                    d[j] = (uint)(t1+(t2=s[j]+s[j+pitch])+2) >> 2;
                    j--;
                    d[j] = (uint)(t2+(t1=s[j]+s[j+pitch])+2) >> 2;
                }
    }
}
