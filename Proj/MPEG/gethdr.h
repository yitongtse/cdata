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
