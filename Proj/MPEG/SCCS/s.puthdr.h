h03656
s 00000/00000/00036
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00036/00000/00000
d D 1.1 99/10/29 15:58:15 yitong 1 0
c date and time created 99/10/29 15:58:15 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef PUTHDR_H
#define PUTHDR_H


/* General utilities
*/
void putStartCode(Writer *out, int SC);


/* Sequence layer headers
*/
void putSeqHdr(PicInfo *pic, Writer *out);
void putSeqExt(PicInfo *pic, Writer *out);
void putSeqDispExt(PicInfo *pic, Writer *out);
void putQuantMatrixExt(PicInfo *pic, Writer *out);
void putSeqEndCode(Writer *out);


/* GOP layer headers
*/
void putGopHdr(PicInfo *pic, Writer *out);


/* Picture layer headers
*/
void putPicHdr(PicInfo *pic, Writer *out);
void putPicCodeExt(PicInfo *pic, Writer *out);
void putPicDispExt(PicInfo *pic, Writer *out);


/* Slice header
*/
void putSlcHdr(MbInfo *mb, Writer *out);


#endif
E 1
