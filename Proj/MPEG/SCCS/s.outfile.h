h53041
s 00000/00000/00027
d D 2.1 00/08/21 11:04:23 ytse 2 1
c Before supporting Windows
e
s 00027/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
  File: outfile.h
        Output file structure
*****************************************************************************/
#ifndef OUTFILE_H
#define OUTFILE_H


/* Output file structure
*/
typedef struct OutFile
{
    FILE   *fp;
    Writer wtr;
    uint   *buf;
    int    bufSz;
}
OutFile;


void openOutFile(OutFile *outFile, char *filename, int bufSz);
int writeFile(OutFile *outFile);
void flushOutFile(OutFile *outFile);
void closeOutFile(OutFile *outFile);


#endif
E 1
