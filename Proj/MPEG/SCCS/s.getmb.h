h09495
s 00003/00002/00047
d D 2.2 02/02/19 11:12:05 ytse 3 2
c getMb() will return number of blocks, number of coefficients and number of bits for MB.
e
s 00000/00000/00049
d D 2.1 00/08/21 11:04:21 ytse 2 1
c Before supporting Windows
e
s 00049/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef GETMB_H
#define GETMB_H


extern int decVerboseLev;       /* decoder verbose level */


/* Constants
     The following constants are used in the run-level VLD decoding
*/
#define FIRST_COEF_P1   COEF_EOB
#define FIRST_COEF_M1   0

extern int runTab[111];
extern int levelTab[111];

/* Global variables */
extern Vld mba_vld;
extern Vld mbType_I_vld;
extern Vld mbType_P_vld;
extern Vld mbType_B_vld;
extern Vld cbp_vld;
extern Vld mv_vld;
extern Vld dmv_vld;
extern Vld lumaSize_vld;
extern Vld chromaSize_vld;
extern Vld coef0_vld;
extern Vld coef1_vld;

void initVld();


/* Macroblock header
*/
D 3
int getMb(Reader *in, PicInfo *pic, MbInfo *mb, RlInfo rl[]);
E 3
I 3
int getMb(Reader *in, PicInfo *pic, MbInfo *mb, RlInfo rl[],
          int* nBlks, int* nCoefs, int* nBits);
E 3
int getMbaIncr(Reader *in, int* SC);
int getMbHdr(Reader *in, PicInfo *pic, MbInfo *mb);
void getMv(Reader *in, int *mv, int f_code, int *predMv);
void getFrmMv(Reader *in, Coor *mv, int *f_code, Coor *predMv);
void getFldMv(Reader *in, Coor *mv, int *fldSel, int *f_code, Coor *predMv,
              int frame_picture);
void getDPMv(Reader *in, Coor *mv, Coor *dmv, int *f_code, Coor *predMv,
             int frame_picture);
int getBlkCoef(Reader* in, int intraFlag, Vld* dcSize_vld, Vld* coef_vld,
D 3
               RlInfo rl[], int mpeg2Flag);
E 3
I 3
               RlInfo rl[], int mpeg2Flag, int* nCoefs, int* nBits);
E 3


#endif

E 1
