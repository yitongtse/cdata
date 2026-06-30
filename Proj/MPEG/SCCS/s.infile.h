h47604
s 00000/00000/00026
d D 2.1 00/08/21 11:04:21 ytse 2 1
c Before supporting Windows
e
s 00026/00000/00000
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
  File: infile.h
        Input file structure
*****************************************************************************/
#ifndef INFILE_H
#define INFILE_H


/* Input file structure
*/
typedef struct InFile
{
    FILE   *fp;
    Reader rdr;
    uint   *buf;
    int    bufSz;
}
InFile;


void openInFile(InFile *inFile, char *filename, int bufSz);
int readFile(InFile *inFile);
void closeInFile(InFile *inFile);


#endif
E 1
