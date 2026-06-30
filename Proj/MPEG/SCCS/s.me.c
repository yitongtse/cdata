h56749
s 00000/00000/00419
d D 2.1 00/08/21 11:04:22 ytse 2 1
c Before supporting Windows
e
s 00419/00000/00000
d D 1.1 99/10/29 15:58:09 yitong 1 0
c date and time created 99/10/29 15:58:09 by yitong
e
u
U
f e 0
t
T
I 1
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "mpeg2.h"
#include "info.h"
#include "me.h"


Coor Corner8[8] = {
  {-1, 0}, {1, 0}, {0, -1}, {0, 1},
  {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
};


uchar halfPelSearchWin[(18*2-1)*(18*2-1)];
					/* half-pel search window */
					/* Note: search window is +-1 from
					         the full-pel result */


/* Note: tested! */
void setupFullSearchOffset(
    int winSz,
    int *noffset,
    Coor *offset[]
)
{
    Coor* ptr;
    int x, y, n, temp = winSz*2+1;
    *noffset = temp * temp;
    ptr = *offset = (Coor *)malloc(*noffset*sizeof(Coor));

    /* General offset in spiral order */
    setCoor(ptr++, 0, 0);
    for (n=1; n<=winSz; n++) {
        for (y=x=-n; x<=n; x++)  setCoor(ptr++, x, y);
        for (x=n, y=-n+1; y<=n; y++) setCoor(ptr++, x, y);
        for (y=n, x=n-1; x>=-n; x--) setCoor(ptr++, x, y);
        for (x=-n, y=n-1; y>-n; y--)  setCoor(ptr++, x, y);
    }
}


/* Compute Total Absolute Error */
/* If distortion is strictly less than input value of minDist, the call will
   return 1 and minDist will be updated.  Otherwise, the call will return 0
   with minDist untouched.

   refInc is the pointer increment from one pixel to its horizontal neighbor.
   refPitch is the pointer increment from one pixel to its vertical neighbor.

   The pixel increment for current block is assumed to be 1.
*/
int fastTae(
    uchar *refBlk,		/* reference block */
    int refInc,			/* pixel increment for reference block */
    int refPitch,		/* line pitch for reference block */
    uchar *curBlk,		/* current block */
    int curPitch,		/* line pitch for current block */
    int width,			/* block width */
    int height,			/* block height */
    int *minDist		/* min distortion so far */
)
{
    int x, y, diff, dist=0;

/*
printf("fastTae():\n");
printf("Current Block:\n");
printUcharBlock(curBlk, width, height, curPitch);
printf("Reference Block:\n");
printUcharBlock(refBlk, width, height, refPitch);
*/

    curPitch -= width;
    refPitch -= width*refInc;

    for (y=height; y>0; y--, curBlk+=curPitch, refBlk+=refPitch)
        for (x=width; x>0; x--, refBlk+=refInc) {
            diff = *curBlk++ - *refBlk;
            dist += (diff<0)? -diff : diff;
//printf("  c:%d r:%d diff:%d dist:%d\n", *(curBlk-1), *refBlk, diff, dist);
            if (dist>=*minDist) {
//printf("*** bailing out\n");
                return 0;
}
        }
    *minDist = dist;
    return 1;
}


/* Motion search engine */
/* Note: On entry, minDist should be set to the min. distortion found so far!
         If this is the beginning of a search, set it to the largest number.

   Returns 1 if a better offset can be found.  Otherwise returns 0.

   The parameter refResShf indicates the resolution of the reference frame:
   0 (<<0 = *1) means full-pel, 1 (<<1 = *2) means half-pel, etc.
   The refPitch is the increment from one line to the next (full-pel or
   half-pel) line.  Hence, the value of the pixel (dx, dy) away from *ptr is
   *(ptr + ((dx+dy*refPitch)<<refResShf)).
   Note: the use of refPitch is different from in other routines!

   The offsets in the offset list is alwasy assumed to be in the same
   resolution as the reference frame, indicated by refRes.

   Note: This version does NOT check motion vector outside picture boundary!!

*/
int motionSearch(
    uchar *refFrm,		/* reference frame */
    int refPitch,		/* line pitch for reference frame */
    int refResShf,		/* reference frame resolution shift parameter */
    uchar *curBlk,		/* current block */
    int curPitch,		/* line pitch for current block */
    int width,			/* block width */
    int height,			/* block height */
    int nOffset,		/* number of offsets to search */
    Coor *offset,		/* offset list */
    Coor *bestOffset,		/* best offset */
    int *minDist		/* min distortion */
)
{
    int i, bestOffsetIdx = -1;

//printf("motinSearch()\n");
    for (i=0; i<nOffset; i++) {
        if (fastTae(refFrm+offset[i].y*refPitch+offset[i].x, 1<<refResShf,
                refPitch<<refResShf, curBlk, curPitch, width, height, minDist))
            bestOffsetIdx = i;
//printf("  Offset %d: (%d,%d)  dist: %d\n",
//       i, offset[i].x, offset[i].y, *minDist);
    }

    if (bestOffsetIdx==-1) {
        setCoor(bestOffset, 0, 0);
//printf("No better offset found!\n");
        return 0;
    }
    else {
        setCoor(bestOffset, offset[bestOffsetIdx].x, offset[bestOffsetIdx].y);
//printf("Best offset %d: (%d,%d)  Min. dist: %d\n", bestOffsetIdx,
//       bestOffset->x, bestOffset->y, *minDist);
        return 1;
    }
}


/* Half-pel interpolation */
/*
   Input block is width x height.  Input pitch is pitch.
   Output block is (2*width-1) x (2*height-1).  Output pitch is (2*width-1).
   Note: Output block memory allocation is assumed to be done before the call.

        o + o + o + o
        + + + + + + +
        o + o + o + o
        + + + + + + +
        o + o + o + o
        + + + + + + +
        o + o + o + o

    o: input pixel in full-pel resolution
    o,+: output pixel in half-pel resolution
*/
/* Tested */
void interpolate(
    uchar *fullPelBlk,
    int pitch,
    int width,
    int height,
    uchar *halfPelBlk
)
{
    int x, y, sum1, sum2;

    for (y=height; y>1; y--, fullPelBlk+=pitch-width+1) {
        for (x=width; x>1; x--, fullPelBlk++) {
            *halfPelBlk++ = *fullPelBlk;
            *halfPelBlk++ = (*fullPelBlk + *(fullPelBlk+1) + 1) >> 1;
        }
        *halfPelBlk++ = *fullPelBlk;	/* rightmost pixel */
        fullPelBlk -= width-1;		/* retrace */

        sum1 = *fullPelBlk + *(fullPelBlk+pitch) + 1;
        for (x=width; x>1; x--) {
            fullPelBlk++;
            sum2 = *fullPelBlk + *(fullPelBlk+pitch) + 1;
            *halfPelBlk++ = sum1 >> 1;
            *halfPelBlk++ = (sum1 + sum2) >> 2;
            sum1 = sum2;
        }
        *halfPelBlk++ = sum1 >> 1;	/* right-most pixel */
    }

    /* Last line */
    for (x=width; x>1; x--, fullPelBlk++) {
        *halfPelBlk++ = *fullPelBlk;
        *halfPelBlk++ = (*fullPelBlk + *(fullPelBlk+1) + 1) >> 1;
    }
    *halfPelBlk++ = *fullPelBlk;	/* lower-right pixel */
    fullPelBlk -= width-1;		/* retrace */
}


void frameMe(
    PicInfo *pic,		/* picture info */
    MbInfo *mb,			/* macroblock info */
    uchar *origCurFrm,		/* original current frame */
    uchar *origRefFrm,		/* original reference frame */
    uchar *codedRefFrm,		/* coded reference frame */
    int nFrmFullPelOffset,	/* number of frame full-pel offsets to search */
    Coor *frmFullPelOffset,	/* frame full-pel offset list */
    int nFrmHalfPelOffset,	/* number of frame half-pel offsets to search */
    Coor *frmHalfPelOffset,	/* frame half-pel offset list */
    int nFldFullPelOffset,	/* number of field full-pel offsets to search */
    Coor *fldFullPelOffset,	/* field full-pel offset list */
    int nFldHalfPelOffset,	/* number of field half-pel offsets to search */
    Coor *fldHalfPelOffset,	/* field half-pel offset list */
    FrmMeResult* result
)
{
    Coor fldMv[2][2];		/* [cur_field][ref_field] */
    int fldDist[2][2];
    Coor mv, dmv;		/* full-pel and half-pel motion vectors */
    int frmPitch = pic->nCol;
    int fldPitch = frmPitch<<1;
    int mbOffset = mb->pos.y*frmPitch + mb->pos.x;


    /* Search for frame-based prediction */
    result->frmDist = LARGE;

    /* Full-pel search */
    motionSearch(origRefFrm+mbOffset, frmPitch, 0, origCurFrm+mbOffset,
        frmPitch, 16, 16, nFrmFullPelOffset, frmFullPelOffset, &mv, &result->frmDist);

    /* Half-pel search */
    interpolate(origRefFrm + mbOffset + (mv.y-1)*frmPitch + mv.x-1,
        frmPitch, 18, 18, halfPelSearchWin);
    motionSearch(halfPelSearchWin+2*(18*2-1)+2, 18*2-1, 1, origCurFrm+mbOffset,
        frmPitch, 16, 16, nFrmHalfPelOffset, frmHalfPelOffset, &dmv,
        &result->frmDist);
    setCoor(&mv, mv.x*2+dmv.x, mv.y*2+dmv.y);

    if (checkRange((mb->pos.x<<1)+mv.x, 0, (pic->nCol-16)<<1) ||
        checkRange((mb->pos.y<<1)+mv.y, 0, (pic->nRow-16)<<1))
        setCoor(&result->frmMv, 0, 0);	/* Note: may need to recompute dist. */
    else
        copyCoor(&result->frmMv, &mv);

    if (pic->frame_pred_frame_dct) {
        result->fldDist = result->dpDist = LARGE;
//        result->motion_type = FRAME_FRAME_BASED;
        return;
    }


    /* Search for field-based prediction
    */

    /* Predict top field from top field */
    fldDist[0][0] = LARGE;

    /* Full-pel search */
    motionSearch(origRefFrm+mbOffset, fldPitch, 0, origCurFrm+mbOffset,
        fldPitch, 16, 8, nFldFullPelOffset, fldFullPelOffset, &mv,
        &fldDist[0][0]);

    /* Half-pel search */
    interpolate(origRefFrm+mbOffset + (mv.y-1)*fldPitch + mv.x-1,
        fldPitch, 18, 10, halfPelSearchWin);
    motionSearch(halfPelSearchWin+2*(18*2-1)+2, 18*2-1, 1, origCurFrm+mbOffset,
        fldPitch, 16, 8, nFldHalfPelOffset, fldHalfPelOffset, &dmv,
        &fldDist[0][0]);
    setCoor(&mv, mv.x*2+dmv.x, mv.y*2+dmv.y);

    if (checkRange((mb->pos.x<<1)+mv.x, 0, (pic->nCol-16)<<1) ||
        checkRange(mb->pos.y+mv.y, 0, pic->nRow-16)) {
        fldDist[0][0] = LARGE;
        setCoor(&fldMv[0][0], 0, 0);	/* Note: Need to set mv to a safe value
					   since it may be used as a base for
					   bidirectional prediction in B frame
					*/
    }
    else
        copyCoor(&fldMv[0][0], &mv);


    /* Predict top field from bottom field */
    fldDist[0][1] = LARGE;

    /* Full-pel search */
    motionSearch(origRefFrm+mbOffset+frmPitch, fldPitch, 0, origCurFrm+mbOffset,
        fldPitch, 16, 8, nFldFullPelOffset, fldFullPelOffset, &mv,
        &fldDist[0][1]);

    /* Half-pel search */
    interpolate(origRefFrm+mbOffset+frmPitch + (mv.y-1)*fldPitch + mv.x-1,
        fldPitch, 18, 10, halfPelSearchWin);
    motionSearch(halfPelSearchWin+2*(18*2-1)+2, 18*2-1, 1, origCurFrm+mbOffset,
        fldPitch, 16, 8, nFldHalfPelOffset, fldHalfPelOffset, &dmv,
        &fldDist[0][1]);
    setCoor(&mv, mv.x*2+dmv.x, mv.y*2+dmv.y);

    if (checkRange((mb->pos.x<<1)+mv.x, 0, (pic->nCol-16)<<1) ||
        checkRange(mb->pos.y+mv.y, 0, pic->nRow-16)) {
        fldDist[0][1] = LARGE;
        setCoor(&fldMv[0][1], 0, 0);
    }
    else
        copyCoor(&fldMv[0][1], &mv);


    /* Predict bottom field from top field */
    fldDist[1][0] = LARGE;

    /* Full-pel search */
    motionSearch(origRefFrm+mbOffset, fldPitch, 0, origCurFrm+mbOffset+frmPitch,
        fldPitch, 16, 8, nFldFullPelOffset, fldFullPelOffset, &mv,
        &fldDist[1][0]);

    /* Half-pel search */
    interpolate(origRefFrm+mbOffset + (mv.y-1)*fldPitch + mv.x-1,
        fldPitch, 18, 10, halfPelSearchWin);
    motionSearch(halfPelSearchWin+2*(18*2-1)+2, 18*2-1, 1,
        origCurFrm+mbOffset+frmPitch, fldPitch, 16, 8, nFldHalfPelOffset,
        fldHalfPelOffset, &dmv, &fldDist[1][0]);
    setCoor(&mv, mv.x*2+dmv.x, mv.y*2+dmv.y);

    if (checkRange((mb->pos.x<<1)+mv.x, 0, (pic->nCol-16)<<1) ||
        checkRange(mb->pos.y+mv.y, 0, pic->nRow-16)) {
        fldDist[1][0] = LARGE;
        setCoor(&fldMv[1][0], 0, 0);
    }
    else
        copyCoor(&fldMv[1][0], &mv);


    /* Predict bottom field from bottom field */
    fldDist[1][1] = LARGE;

    /* Full-pel search */
    motionSearch(origRefFrm+mbOffset+frmPitch, fldPitch, 0,
        origCurFrm+mbOffset+frmPitch, 32, 16, 8, nFldFullPelOffset,
        fldFullPelOffset, &mv, &fldDist[1][1]);

    /* Half-pel search */
    interpolate(origRefFrm+mbOffset+frmPitch + (mv.y-1)*fldPitch + mv.x-1,
        fldPitch, 18, 10, halfPelSearchWin);
    motionSearch(halfPelSearchWin+2*(18*2-1)+2, 18*2-1, 1,
        origCurFrm+mbOffset+frmPitch, fldPitch, 16, 8, nFldHalfPelOffset,
        fldHalfPelOffset, &dmv, &fldDist[1][1]);
    setCoor(&mv, mv.x*2+dmv.x, mv.y*2+dmv.y);

    if (checkRange((mb->pos.x<<1)+mv.x, 0, (pic->nCol-16)<<1) ||
        checkRange(mb->pos.y+mv.y, 0, pic->nRow-16)) {
        fldDist[1][1] = LARGE;
        setCoor(&fldMv[1][1], 0, 0);
    }
    else
        copyCoor(&fldMv[1][1], &mv);


    /* Select reference parity for top field */
    if (fldDist[0][0] <= fldDist[0][1]) {
        result->fld1Sel = 0;
        result->fld1Mv = fldMv[0][0];
        result->fldDist = fldDist[0][0];
    }
    else {
        result->fld1Sel = 1;
        result->fld1Mv = fldMv[0][1];
        result->fldDist = fldDist[0][1];
    }

    /* Select reference parity for bottom field */
    if (fldDist[1][0] <= fldDist[1][1]) {
        result->fld2Sel = 0;
        result->fld2Mv = fldMv[1][0];
        result->fldDist += fldDist[1][0];
    }
    else {
        result->fld2Sel = 1;
        result->fld2Mv = fldMv[1][1];
        result->fldDist += fldDist[1][1];
    }

    /* Dual-prime not done yet! */
    result->dpDist = LARGE;
}


void frmMotionTypeDecision(FrmMeResult *result)
{
    /* Adding bias to distortions */
    result->fldDist += 100;

    result->motion_type = FRAME_FRAME_BASED;
    result->minDist = result->frmDist;

    if (result->fldDist < result->minDist) {
        result->motion_type = FRAME_FIELD_BASED;
        result->minDist = result->fldDist;
    }
}


void printFrmMeResult(FrmMeResult* result)
{
    printf("Frame-based prediction: mv=(%d,%d) \tdist=%d\n", result->frmMv.x,
           result->frmMv.y, result->frmDist);
    printf("Field-based prediction: mv1=(%d,%d):%d \tmv2=(%d,%d):%d" \
           " \tdist=%d\n", result->fld1Mv.x, result->fld1Mv.y, result->fld1Sel,
           result->fld2Mv.x, result->fld2Mv.y, result->fld2Sel,
           result->fldDist);
}
E 1
