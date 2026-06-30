h08432
s 00000/00000/00106
d D 2.1 00/08/21 11:04:22 ytse 3 2
c Before supporting Windows
e
s 00009/00009/00097
d D 1.2 99/10/29 17:07:58 yitong 2 1
c minor changes
e
s 00106/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
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
D 2
    unsigned char *refBlk,	/* reference block */
E 2
I 2
    uchar *refBlk,		/* reference block */
E 2
    int refInc,			/* pixel increment for reference block */
    int refPitch,		/* line pitch for reference block */
D 2
    unsigned char *curBlk,	/* current block */
E 2
I 2
    uchar *curBlk,		/* current block */
E 2
    int curPitch,		/* line pitch for current block */
    int widht,			/* block width */
    int height,			/* block height */
    int *minDist		/* min distortion so far */
);


/* Motion search engine */
int motionSearch(
D 2
    unsigned char *refFrm,	/* reference frame */
E 2
I 2
    uchar *refFrm,		/* reference frame */
E 2
    int refPitch,               /* line pitch for reference frame */
    int refResShf,              /* reference frame resolution shift parameter */
D 2
    unsigned char *curBlk,	/* current block */
E 2
I 2
    uchar *curBlk,		/* current block */
E 2
    int curPitch,		/* line pitch for current block */
    int widht,			/* block width */
    int height,			/* block height */
    int nOffset,		/* number of offsets to search */
    Coor *offset,		/* offset list */
    Coor *bestOffset,		/* best offset */
    int *minDist		/* min distortion */
);


void interpolate(
D 2
    unsigned char *fullPelBlk,
E 2
I 2
    uchar *fullPelBlk,
E 2
    int pitch,
    int width,
    int height,
D 2
    unsigned char *halfPelBlk
E 2
I 2
    uchar *halfPelBlk
E 2
);


void frameMe(
    PicInfo *pic,               /* picture info */
    MbInfo *mb,                 /* macroblock info */
D 2
    unsigned char *origCurFrm,  /* original current frame */
    unsigned char *origRefFrm,  /* original reference frame */
    unsigned char *codedRefFrm, /* coded reference frame */
E 2
I 2
    uchar *origCurFrm,  	/* original current frame */
    uchar *origRefFrm,  	/* original reference frame */
    uchar *codedRefFrm, 	/* coded reference frame */
E 2
    int nFrmFullPelOffset,      /* number of frame full-pel offsets to search */    Coor *frmFullPelOffset,     /* frame full-pel offset list */
    int nFrmHalfPelOffset,      /* number of frame half-pel offsets to search */    Coor *frmHalfPelOffset,     /* frame half-pel offset list */
    int nFldFullPelOffset,      /* number of field full-pel offsets to search */    Coor *fldFullPelOffset,     /* field full-pel offset list */
    int nFldHalfPelOffset,      /* number of field half-pel offsets to search */    Coor *fldHalfPelOffset,     /* field half-pel offset list */
    FrmMeResult* result
);


void frmMotionTypeDecision(FrmMeResult *result);


#endif
E 1
