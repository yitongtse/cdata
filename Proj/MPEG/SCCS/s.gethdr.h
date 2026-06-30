h30645
s 00000/00000/00044
d D 2.1 00/08/21 11:04:20 ytse 2 1
c Before supporting Windows
e
s 00044/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef GETHDR_H
#define GETHDR_H


/* General utilities
*/
int seekStartCode(Reader *in);
int getNextStartCode(Reader *in);
void getMarkerBit(Reader *in);
int getUserData(Reader *in);


/* Sequence layer headers
*/
int seekSeqHdr(Reader *in);
void getSeqHdr(PicInfo *pic, Reader *in);
void getSeqExtension(PicInfo *pic, Reader *in);
void getSeqExt(PicInfo *pic, Reader *in);
void getSeqDispExt(PicInfo *pic, Reader *in);
void getQuantMatrixExt(PicInfo *pic, Reader *in);
int getSeqLayer(Reader *in, PicInfo *pic, int *SC);


/* GOP layer headers
*/
void getGopHdr(PicInfo *pic, Reader *in);
int getGopLayer(Reader *in, PicInfo *pic, int *SC);


/* Picture layer headers
*/
void getPicHdr(PicInfo *pic, Reader *in);
void getPicCodeExt(PicInfo *pic, Reader *in);
void getPicExtension(PicInfo *pic, Reader *in);
void getPicDispExt(PicInfo *pic, Reader *in);
int getPicLayer(Reader *in, PicInfo *pic, int *SC);


/* Slice header
*/
void getSlcHdr(MbInfo *mb, Reader *in, int SC);


#endif
E 1
