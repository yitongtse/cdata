#ifndef IMAGEIO_H
#define IMAGEIO_H

int readPgmHdr(FILE *fp, int *width, int *height, int *maxValue);

int readPgmData(FILE *fp, uchar *buf, int width, int height, int pitch);

int writePgm(char *filename, uchar *buf, int width, int height,
             int maxValue, int pitch);

#endif
