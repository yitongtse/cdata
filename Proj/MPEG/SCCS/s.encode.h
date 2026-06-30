h55965
s 00000/00000/00015
d D 2.1 00/08/21 11:04:20 ytse 3 2
c Before supporting Windows
e
s 00002/00002/00013
d D 1.2 99/10/29 17:07:57 yitong 2 1
c minor changes
e
s 00015/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef ENCODE_H
#define ENCODE_H


D 2
void extractMb(PicInfo *pic, MbInfo *mb, unsigned char *origFrm[], 
               unsigned char *predFrm[], short *blk[], int subFlag);
E 2
I 2
void extractMb(PicInfo *pic, MbInfo *mb, uchar *origFrm[], 
               uchar *predFrm[], short *blk[], int subFlag);
E 2

void fdctMb(short *blk[]);

void quantScanMb(PicInfo *pic, MbInfo *mb, short *curBlk[], RlInfo rl[]);

void updateDcDctPred(MbInfo *mb, RlInfo rl[]);


#endif
E 1
