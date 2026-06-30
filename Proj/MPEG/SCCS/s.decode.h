h12872
s 00002/00001/00028
d D 2.2 02/03/21 13:28:39 ytse 4 3
c Backup for NEW2
e
s 00000/00000/00029
d D 2.1 00/08/21 11:04:19 ytse 3 2
c Before supporting Windows
e
s 00005/00006/00024
d D 1.2 99/10/29 17:07:57 yitong 2 1
c minor changes
e
s 00030/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef DECODE_H
#define DECODE_H


D 2
void getPredMb(PicInfo *pic, MbInfo *mb, unsigned char *curFrm[], 
         unsigned char *fwdRefFrm[], unsigned char *bwdRefFrm[],
         int second_field);
E 2
I 2
void getPredMb(PicInfo *pic, MbInfo *mb, uchar *curFrm[], 
         uchar *fwdRefFrm[], uchar *bwdRefFrm[], int second_field);
E 2

D 2
void reconst(unsigned char *src[], int sfield, unsigned char *dst[],
E 2
I 2
void reconst(uchar *src[], int sfield, uchar *dst[],
E 2
             int dfield, int pitch, int width, int height, int x, int y,
             int dx, int dy, int addFlag, int chroma_format);

D 2
void reconstComp(unsigned char *src, unsigned char *dst, int pitch, int width,
E 2
I 2
void reconstComp(uchar *src, uchar *dst, int pitch, int width,
E 2
                 int height, int x, int y, int dx, int dy, int addFlag);

D 4
void invQuantScanDct(PicInfo *pic, MbInfo *mb, RlInfo rl[], short *blk[]);
E 4
I 4
void invQuantScanDct(PicInfo *pic, MbInfo *mb, RlInfo rl[], short *blk[],
                     int debugFlag);
E 4

int invQuantScanInterBlk(RlInfo *rl, short *scanTab, int mquant, short *qmat,
                         short *blk);

int invQuantScanIntraBlk(RlInfo *rl, short *scanTab, int intra_dc_mult,
                         int mquant, short *qmat, short *blk);

D 2
void combineMb(PicInfo *pic, MbInfo *mb, unsigned char *curFrm[], short *blk[]);
E 2
I 2
void combineMb(PicInfo *pic, MbInfo *mb, uchar *curFrm[], short *blk[]);
E 2

void idct(short *block);
void init_idct();


#endif
E 1
