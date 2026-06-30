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
