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
