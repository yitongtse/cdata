h10619
s 00005/00001/00075
d D 2.6 04/10/05 15:41:26 ytse 8 7
c Check EOF condition
e
s 00003/00000/00073
d D 2.5 03/03/21 14:17:41 ytse 7 6
c sccs tsparse.c
e
s 00007/00003/00066
d D 2.4 02/02/25 17:53:45 ytse 6 5
c Update
e
s 00002/00003/00067
d D 2.3 02/01/21 11:02:33 ytse 5 4
c Update
e
s 00000/00000/00070
d D 2.2 01/01/15 12:39:13 ytse 4 3
c update
e
s 00000/00000/00070
d D 2.1 00/08/21 11:04:26 ytse 3 2
c Before supporting Windows
e
s 00001/00022/00069
d D 1.2 99/11/01 16:15:35 yitong 2 1
c Backup
e
s 00091/00000/00000
d D 1.1 99/10/29 15:58:12 yitong 1 0
c date and time created 99/10/29 15:58:12 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: tsview.c
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "reader.h"
#include "writer.h"
#include "infile.h"
#include "tp.h"


InFile inFile;


D 2
/* Return 0 if successful.  Return EOF if end of file reached.
*/
int findTp(FILE *fp, int syncCnt)
{
    int i;

    while (1) {
        // Find the first SYNC candidate
        while (fgetc(fp) != TP_SYNC) {
            if (feof(fp))  return EOF;
        }

        for (i=1; i<syncCnt; i++) {
            fseek(fp, TP_SIZE-1, SEEK_CUR);
            if (fgetc(fp) != TP_SYNC)  break;
        }
        fseek(fp, -1, SEEK_CUR);
        return 0;
    }
}


E 2
int main(int argc, char** argv)
{
    Param par;
    char inFilename[256];
I 6
    int tpMode;
E 6
    int numTp;
    TpInfo tp;
    int i, j;
    int size;
    uint hdr;
I 6
    int tpSz;
I 7
    int skipBytes;
E 7
E 6
    int temp;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsview.par");
    getStringParam(&par, "Input filename", inFilename, "test.in");
I 6
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
E 6
    getIntParam(&par, "Number of TP packets to examine", &numTp, 1);
I 7
    getIntParam(&par, "Number of bytes to skip", &skipBytes, 0);
E 7
    endParam(&par);

D 6
    openInFile(&inFile, inFilename, TP_SIZE);
E 6
I 6
    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;
    openInFile(&inFile, inFilename, tpSz);
I 7
    fseek(inFile.fp, skipBytes, SEEK_SET);
E 7
E 6
    setReaderEcho(&inFile.rdr, 0, stdout);

    /* Transport packet alignment */
D 6
    if (findTp(inFile.fp, 1)) {
E 6
I 6
    if (findTp(inFile.fp, tpSz, 1)) {
E 6
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
I 2
    printf("Sync at byte %d\n", ftell(inFile.fp));
E 2

    printf(" No.  PID C A S   Header   Payload\n");
    for (i=0; i<numTp; i++) {
        if ((temp = getByte(&inFile.rdr, "sync")) != TP_SYNC)
            printf("Lost sync!  Sync byte becomes 0x%02x\n", temp);

        hdr = *inFile.rdr.bitBuf.curPtr;
D 8
        getTpHdr(&tp, &inFile.rdr);
E 8
I 8
        temp = getTpHdr(&tp, &inFile.rdr);
        if (temp) {
            printf("getTpHdr error: %d\n", temp);
            exit(0);
        }
E 8
        printf("%3d  %4x %x %x %x  %08x  ", i, tp.PID, tp.continuity_counter,
               tp.adaptation_field_control, tp.payload_unit_start_indicator,
               hdr);
        if (tp.adaptation_field_control & 2) {
            getTpAf(&tp, &inFile.rdr);
D 5
            printf("AF size: %d  PCR flag: %d",
                   tp.adaptation_field_length, tp.PCR_flag);
            if (tp.PCR_flag)  printf("  PCR: 0x08x", tp.PCR[0]);
E 5
I 5
            printf("AF: sz %d", tp.adaptation_field_length, tp.PCR_flag);
            if (tp.PCR_flag)  printf(", PCR %08x", tp.PCR[0]);
E 5
            printf("\n                           ");
        }
        size = TP_SIZE - tp.nbytes;	/* remaining payload size */
        if (size>16)  size = 16;
        for (j=0; j<size; j++)
            printf("%02x ", getByte(&inFile.rdr, ""));
        printf("...\n");
        tp.nbytes += size;
D 6
        skipTp(&tp, &inFile.rdr);
E 6
I 6
        skipTp(&tp, &inFile.rdr, tpSz);
E 6
    }
}
E 1
