#ifndef ENCODE_H
#define ENCODE_H


void extractMb(PicInfo *pic, MbInfo *mb, uchar *origFrm[], 
               uchar *predFrm[], short *blk[], int subFlag);

void fdctMb(short *blk[]);

void quantScanMb(PicInfo *pic, MbInfo *mb, short *curBlk[], RlInfo rl[]);

void updateDcDctPred(MbInfo *mb, RlInfo rl[]);


#endif
