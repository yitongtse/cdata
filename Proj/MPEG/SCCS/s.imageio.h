h37412
s 00000/00000/00011
d D 2.1 00/08/21 11:04:21 ytse 2 1
c Before supporting Windows
e
s 00011/00000/00000
d D 1.1 99/10/29 15:58:13 yitong 1 0
c date and time created 99/10/29 15:58:13 by yitong
e
u
U
f e 0
t
T
I 1
#ifndef IMAGEIO_H
#define IMAGEIO_H

int readPgmHdr(FILE *fp, int *width, int *height, int *maxValue);

int readPgmData(FILE *fp, uchar *buf, int width, int height, int pitch);

int writePgm(char *filename, uchar *buf, int width, int height,
             int maxValue, int pitch);

#endif
E 1
