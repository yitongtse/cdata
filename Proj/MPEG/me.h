#ifndef ME_H
#define ME_H

#define	LARGE		(1<<28)


typedef struct FrmMeResult
{
    Coor frmMv;
    int fld1Sel;
    Coor fld1Mv;
    int fld2Sel;
    Coor fld2Mv;
    Coor dpMv;
    Coor dmv;
    int frmDist;
    int fldDist;
    int dpDist;
    int minDist;
    int motion_type;

} FrmMeResult;

void printfFrmMeResult(FrmMeResult* result);


typedef struct FldMeResult
{
    int fldSel;
    Coor fldMv;
    int UpperSel;
    Coor UpperMv;
    int LowerSel;
    Coor LowerMv;
    Coor dpMv;
    Coor dmv;
    int fldDist;
    int mc16x8Dist;
    int dpDist;

} FldMeResult;


void setupFullSearchOffset(
    int winSz,
    int *noffset,
    Coor *offset[]
);


/* Compute Total Absolute Error */
int fastTae(
    uchar *refBlk,		/* reference block */
    int refInc,			/* pixel increment for reference block */
    int refPitch,		/* line pitch for reference block */
    uchar *curBlk,		/* current block */
    int curPitch,		/* line pitch for current block */
    int widht,			/* block width */
    int height,			/* block height */
    int *minDist		/* min distortion so far */
);


/* Motion search engine */
int motionSearch(
    uchar *refFrm,		/* reference frame */
    int refPitch,               /* line pitch for reference frame */
    int refResShf,              /* reference frame resolution shift parameter */
    uchar *curBlk,		/* current block */
    int curPitch,		/* line pitch for current block */
    int widht,			/* block width */
    int height,			/* block height */
    int nOffset,		/* number of offsets to search */
    Coor *offset,		/* offset list */
    Coor *bestOffset,		/* best offset */
    int *minDist		/* min distortion */
);


void interpolate(
    uchar *fullPelBlk,
    int pitch,
    int width,
    int height,
    uchar *halfPelBlk
);


void frameMe(
    PicInfo *pic,               /* picture info */
    MbInfo *mb,                 /* macroblock info */
    uchar *origCurFrm,  	/* original current frame */
    uchar *origRefFrm,  	/* original reference frame */
    uchar *codedRefFrm, 	/* coded reference frame */
    int nFrmFullPelOffset,      /* number of frame full-pel offsets to search */    Coor *frmFullPelOffset,     /* frame full-pel offset list */
    int nFrmHalfPelOffset,      /* number of frame half-pel offsets to search */    Coor *frmHalfPelOffset,     /* frame half-pel offset list */
    int nFldFullPelOffset,      /* number of field full-pel offsets to search */    Coor *fldFullPelOffset,     /* field full-pel offset list */
    int nFldHalfPelOffset,      /* number of field half-pel offsets to search */    Coor *fldHalfPelOffset,     /* field half-pel offset list */
    FrmMeResult* result
);


void frmMotionTypeDecision(FrmMeResult *result);


#endif
