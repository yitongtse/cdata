h43715
s 00000/00001/00173
d D 2.5 02/05/24 12:26:32 ytse 8 7
c Update
e
s 00000/00002/00174
d D 2.4 02/03/12 13:46:37 ytse 7 6
c Update
e
s 00013/00008/00163
d D 2.3 02/02/25 17:53:45 ytse 6 5
c Update
e
s 00002/00000/00169
d D 2.2 02/01/21 11:02:32 ytse 5 4
c Update
e
s 00000/00000/00169
d D 2.1 00/08/21 11:04:26 ytse 4 3
c Before supporting Windows
e
s 00029/00022/00140
d D 1.3 99/11/02 12:05:21 yitong 3 2
c Fix a bug on TP without payload
e
s 00001/00022/00161
d D 1.2 99/11/01 16:15:35 yitong 2 1
c Backup
e
s 00183/00000/00000
d D 1.1 99/10/29 15:58:11 yitong 1 0
c date and time created 99/10/29 15:58:11 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: tsfilter.c

    Extract from a transport stream a PID stream, PES stream,
    or an elementary stream

    Note: Output file will be aligned with the corresponding headers
          in all modes.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "bitbuf.h"
#include "reader.h"
#include "infile.h"
#include "writer.h"
#include "tp.h"
#include "pes.h"

#define	TP_SIZE_WORD	(TP_SIZE >> 2)

enum OutMode { PID_MODE=1, PES_MODE=2, ES_MODE=3 };


InFile inFile;
Reader fltIn;			// for filtered input
FILE *outFp;
I 6
int tpMode;
int tpSz;
E 6
int pid;
D 6
int mode;
E 6
I 6
int filterMode;			// filter mode
E 6
int syncFlag;                   // whether to sync to corresponding start code

int seekSeqHdr(Reader *in);


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
/* Note: We assume the input file is already aligned with
         transport packet boundary
*/
int getMorePayload()
{
    static int firstTime = 1;
    TpInfo tp;
    PesInfo pes;

    while (1) {
        if (firstTime) {
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
            printf("Sync found at byte %d\n", ftell(inFile.fp));
E 2
            firstTime = 0;
        }

        if (getByte(&inFile.rdr, "sync") != TP_SYNC) {
            printf("Lost TP SYNC!\n");
            exit(-1);
        }

        // Check for end of file
        if (feof(inFile.fp)) {
            fclose(outFp);
            exit(0);
        }

        getTpHdr(&tp, &inFile.rdr);
        if (tp.PID == pid) {

D 6
            if (mode==PID_MODE) {
E 6
I 6
            if (filterMode==PID_MODE) {
E 6
                fltIn.bitBuf.curPtr = inFile.buf;
                fltIn.bitBuf.bitPos = 32;
                flushReader(&inFile.rdr);
                syncFlag = 1;
                firstTime = 0;
                return 0;
            }

            /* Get adaptation field if needed */
D 3
            if (tp.adaptation_field_control & 2)  getTpAf(&tp, &inFile.rdr);
E 3
I 3
            if (tp.adaptation_field_control & 2) {
                getTpAf(&tp, &inFile.rdr);
            }
E 3

D 3
            if (mode==PES_MODE) {
                if (!syncFlag) {
                    if (tp.payload_unit_start_indicator) {
                        syncFlag = 1;
E 3
I 3
            if (tp.adaptation_field_control & 1) {
                /* TP has payload */
D 6
                if (mode==PES_MODE) {
E 6
I 6
                if (filterMode==PES_MODE) {
E 6
                    if (!syncFlag) {
                        if (tp.payload_unit_start_indicator) {
                            syncFlag = 1;
                        }
                        else {
                            /* skip this packet */
                            flushReader(&inFile.rdr);
                            continue;
                        }
E 3
                    }
D 3
                    else {
                        /* skip this packet */
                        flushReader(&inFile.rdr);
                        continue;
E 3
I 3
                    copyReader(&inFile.rdr, &fltIn);
                    flushReader(&inFile.rdr);
                    return 0;
                }

                if (tp.payload_unit_start_indicator) {
                    if (getBits(&inFile.rdr, 24, "")!=1) {
                        printf("Error: Start code for PES header expected."\
                               "  Ignored.\n");
E 3
                    }
I 3
                    getPesHdr(&pes, &inFile.rdr);
E 3
                }
D 3
                copyReader(&inFile.rdr, &fltIn);
                flushReader(&inFile.rdr);
                return 0;
            }
E 3

D 3
            if (tp.payload_unit_start_indicator) {
                if (getBits(&inFile.rdr, 24, "")!=1) {
                    printf("Error: Start code for PES header expected."\
                           "  Ignored.\n");
E 3
I 3
                if (inFile.rdr.bitBuf.curPtr < inFile.rdr.bitBuf.endPtr) {
                    copyReader(&inFile.rdr, &fltIn);
                    flushReader(&inFile.rdr);
                    return 0;
E 3
                }
D 3
                getPesHdr(&pes, &inFile.rdr);
E 3
            }
D 3

            copyReader(&inFile.rdr, &fltIn);
            flushReader(&inFile.rdr);
            return 0;
E 3
        }

        /* Skip this packet */
        flushReader(&inFile.rdr);
    }
}


int main(int argc, char** argv)
{
    Param par;
    char inFilename[256], outFilename[256];
D 8
    Writer out;
E 8
    int data;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsfilter.par");
    commentParam(&par, "");
    commentParam(&par, "This program extracts from a transport stream");
    commentParam(&par, "a PID stream, a PES stream, or an elementary stream");
    commentParam(&par, "");
    getStringParam(&par, "Input transport stream", inFilename, "test.in");
D 6
    getStringParam(&par, "Output filename", outFilename, "test.out");
E 6
I 6
    getIntParam(&par, "Input packet size mode (1-188 2-204)", &tpMode, 1);
E 6
    getIntParam(&par, "PID to extract", &pid, 0);
D 6
    getIntParam(&par, "Output mode (1-PID 2-PES 3-ES)", &mode, 3);
E 6
I 6
    getStringParam(&par, "Output filename", outFilename, "test.out");
    getIntParam(&par, "Output mode (1-PID 2-PES 3-ES)", &filterMode, 3);
E 6
    getIntParam(&par, "Sync to start code?", &syncFlag, 1);
    endParam(&par);

I 6
    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;

E 6
    syncFlag = !syncFlag;
D 6
    openInFile(&inFile, inFilename, TP_SIZE);
E 6
I 6
    openInFile(&inFile, inFilename, tpSz);
E 6
    initReader(&fltIn, getMorePayload, NULL);
    setBitBuf(&fltIn.bitBuf, inFile.buf+TP_SIZE_WORD,
              inFile.buf+TP_SIZE_WORD, 0);

    outFp = fopen(outFilename, "w");

D 6
    if (mode==ES_MODE) {
E 6
I 6
    if (filterMode==ES_MODE) {
E 6
I 5
D 7
/*
E 7
E 5
        if (seekSeqHdr(&fltIn)) {
            printf("EOF reached while searching for first "\
                   "sequence start code\n");
            exit (0);
        }
I 5
D 7
*/
E 7
E 5
        fputc(0, outFp);
        fputc(0, outFp);
        fputc(1, outFp);
        fputc(0xb3, outFp);
    }

    while (1) {
        data = getByte(&fltIn, "");
        fputc(data, outFp);
    }
}

E 1
