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
