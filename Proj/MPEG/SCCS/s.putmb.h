h35033
s 00000/00000/00041
d D 2.1 00/08/21 11:04:25 ytse 2 1
c Before supporting Windows
e
s 00041/00000/00000
d D 1.1 99/10/29 15:58:15 yitong 1 0
c date and time created 99/10/29 15:58:15 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef PUTMB_H
#define PUTMB_H


/* Global variables */
#define MAX_RUN         31

extern int coefOffsetTab[];
extern int maxLevelTab[];


extern Vlc mba_vlc;
extern Vlc mbType_I_vlc;
extern Vlc mbType_P_vlc;
extern Vlc mbType_B_vlc;
extern Vlc cbp_vlc;
extern Vlc mv_vlc;
extern Vlc dmv_vlc;
extern Vlc lumaSize_vlc;
extern Vlc chromaSize_vlc;
extern Vlc coef0_vlc;
extern Vlc coef1_vlc;


/*
    Macroblock header
*/
void initVlc();
void putMb(Writer *out, PicInfo *pic, MbInfo *mb, RlInfo rl[]);
int putMbHdr(Writer *out, PicInfo *pic, MbInfo *mb);
void putMv(Writer *out, int *mv, int f_code, int *predMv, int halfRange);
void putFrmMv(Writer *out, Coor *mv, int *f_code, Coor *predMv);
void putFldMv(Writer *out, Coor *mv, int fldSel, int *f_code, Coor *predMv,
              int frame_picture);
void putDPMv(Writer *out, Coor *mv, Coor *dmv, int *f_code, Coor *predMv,
             int frame_picture);
void putBlkCoef(Writer* out, int intraFlag, Vlc* dcSize_vlc, Vlc* coef_vlc,
                RlInfo* rl, int mpeg2Flag);


#endif
E 1
