#ifndef DECODE_H
#define DECODE_H


void getPredMb(PicInfo *pic, MbInfo *mb, uchar *curFrm[], 
         uchar *fwdRefFrm[], uchar *bwdRefFrm[], int second_field);

void reconst(uchar *src[], int sfield, uchar *dst[],
             int dfield, int pitch, int width, int height, int x, int y,
             int dx, int dy, int addFlag, int chroma_format);

void reconstComp(uchar *src, uchar *dst, int pitch, int width,
                 int height, int x, int y, int dx, int dy, int addFlag);

void invQuantScanDct(PicInfo *pic, MbInfo *mb, RlInfo rl[], short *blk[],
                     int debugFlag);

int invQuantScanInterBlk(RlInfo *rl, short *scanTab, int mquant, short *qmat,
                         short *blk);

int invQuantScanIntraBlk(RlInfo *rl, short *scanTab, int intra_dc_mult,
                         int mquant, short *qmat, short *blk);

void combineMb(PicInfo *pic, MbInfo *mb, uchar *curFrm[], short *blk[]);

void idct(short *block);
void init_idct();


#endif
