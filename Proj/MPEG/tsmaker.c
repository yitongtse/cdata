/*****************************************************************************
    File: tsmaker.c

    Transport stream maker

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


int byteCnt = 0;
int tpCnt = 0;

FILE *inFp, *lenFp;
FILE *outFp, *syntaxFp;
Writer wtr;
uint tpBuf[TP_SIZE_WORD];    // TP buffer
TpInfo tp;


void nextTp()
{
    int payloadSz;

    while (1) {
        fscanf(lenFp, "%d", &payloadSz);
        if (feof(lenFp)) {
            rewind(lenFp);
            fscanf(lenFp, "%d", &payloadSz);
        }

        if (payloadSz >= TP_SIZE-4) {
            payloadSz = TP_SIZE - 4;
            tp.adaptation_field_control = 1;
        }
        else if (payloadSz>0) {
            tp.adaptation_field_control = 3;
            tp.adaptation_field_length = TP_SIZE - 5 - payloadSz;
        }
        else {
            tp.adaptation_field_control = 2;
            tp.adaptation_field_length = TP_SIZE - 5;
        }

        setBitBuf(&wtr.bitBuf, tpBuf, tpBuf+(TP_SIZE>>2), 32);
        wtr.bitPos = 32;

        putTpHdr(&tp, &wtr);
        if (tp.adaptation_field_control & 2)
            putTpAf(&tp, &wtr);
        tp.continuity_counter++;

        if (!wtr.bitPos) {
            *wtr.bitBuf.curPtr++ = wtr.bitBuf.buffer;
            wtr.bitPos = 32;
            wtr.bitBuf.buffer = 0;
            wtr.byteCnt += 4;
        }
        wtr.bitBuf.bitPos = wtr.bitPos;

        if (payloadSz)  return;
        fwrite(tpBuf, 1, TP_SIZE, outFp);
    }
}


int writeTp()
{
    int fillSize = wtr.bitBuf.endPtr - wtr.bitBuf.curPtr;
    if (fillSize > 0) {
        /* Right fill buffer with 0 */
        memset(wtr.bitBuf.curPtr, 0, fillSize<<2);
    }
    fwrite(tpBuf, 1, TP_SIZE, outFp);
    nextTp();
    return 0;
}


int main(int argc, char** argv)
{
    Param par;
    char inFilename[256], lenParFilename[256];
    char outFilename[256], syntaxFilename[256];
    int pid;
    int data;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsmaker.par");
    commentParam(&par, "This program packetize an input file "\
                       "into a transport stream.");
    commentParam(&par, "The payload length is specified in a parameter file.");
    commentParam(&par, "");
    getStringParam(&par, "Input filename", inFilename, "test.in");
    getIntParam(&par, "PID", &pid, 0);
    getStringParam(&par, "Payload size parameter filename", lenParFilename,
        "len_par");
    getStringParam(&par, "Output transport stream filename", outFilename,
        "test.out");
    getStringParam(&par, "Output syntax filename", syntaxFilename,
        "syntax.out");
    endParam(&par);


    if ((inFp = fopen(inFilename, "r")) == NULL) {
        printf("Failed to open input file %s\n", inFilename);
        exit(-1);
    }
    if ((lenFp = fopen(lenParFilename, "r")) == NULL) {
        printf("Failed to open input file %s\n", lenParFilename);
        exit(-1);
    }
    if ((outFp = fopen(outFilename, "w")) == NULL) {
        printf("Failed to open output file %s\n", outFilename);
        exit(-1);
    }
    if ((syntaxFp = fopen(syntaxFilename, "w")) == NULL) {
        printf("Failed to open output syntax file %s", syntaxFilename);
        exit(-1);
    }

    initWriter(&wtr, writeTp, NULL);
    setWriterEcho(&wtr, 1, syntaxFp);

    initTp(&tp);
    tp.PID = pid;

    nextTp();    // create the first output TP

    while (1) {
        if ((data = fgetc(inFp)) == EOF)  break;
        putByte(&wtr, data, "");
    }

    flushWriter(&wtr);
}

