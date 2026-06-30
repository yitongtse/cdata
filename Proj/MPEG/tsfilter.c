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
int tpMode;
int tpSz;
int pid;
int filterMode;			// filter mode
int syncFlag;                   // whether to sync to corresponding start code

int seekSeqHdr(Reader *in);


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
            if (findTp(inFile.fp, tpSz, 1)) {
                printf("Failed to find SYNC.\n");
                exit (-1);
            }
            printf("Sync found at byte %d\n", ftell(inFile.fp));
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

            if (filterMode==PID_MODE) {
                fltIn.bitBuf.curPtr = inFile.buf;
                fltIn.bitBuf.bitPos = 32;
                flushReader(&inFile.rdr);
                syncFlag = 1;
                firstTime = 0;
                return 0;
            }

            /* Get adaptation field if needed */
            if (tp.adaptation_field_control & 2) {
                getTpAf(&tp, &inFile.rdr);
            }

            if (tp.adaptation_field_control & 1) {
                /* TP has payload */
                if (filterMode==PES_MODE) {
                    if (!syncFlag) {
                        if (tp.payload_unit_start_indicator) {
                            syncFlag = 1;
                        }
                        else {
                            /* skip this packet */
                            flushReader(&inFile.rdr);
                            continue;
                        }
                    }
                    copyReader(&inFile.rdr, &fltIn);
                    flushReader(&inFile.rdr);
                    return 0;
                }

                if (tp.payload_unit_start_indicator) {
                    if (getBits(&inFile.rdr, 24, "")!=1) {
                        printf("Error: Start code for PES header expected."\
                               "  Ignored.\n");
                    }
                    getPesHdr(&pes, &inFile.rdr);
                }

                if (inFile.rdr.bitBuf.curPtr < inFile.rdr.bitBuf.endPtr) {
                    copyReader(&inFile.rdr, &fltIn);
                    flushReader(&inFile.rdr);
                    return 0;
                }
            }
        }

        /* Skip this packet */
        flushReader(&inFile.rdr);
    }
}


int main(int argc, char** argv)
{
    Param par;
    char inFilename[256], outFilename[256];
    int data;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "tsfilter.par");
    commentParam(&par, "");
    commentParam(&par, "This program extracts from a transport stream");
    commentParam(&par, "a PID stream, a PES stream, or an elementary stream");
    commentParam(&par, "");
    getStringParam(&par, "Input transport stream", inFilename, "test.in");
    getIntParam(&par, "Input packet size mode (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "PID to extract", &pid, 0);
    getStringParam(&par, "Output filename", outFilename, "test.out");
    getIntParam(&par, "Output mode (1-PID 2-PES 3-ES)", &filterMode, 3);
    getIntParam(&par, "Sync to start code?", &syncFlag, 1);
    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;

    syncFlag = !syncFlag;
    openInFile(&inFile, inFilename, tpSz);
    initReader(&fltIn, getMorePayload, NULL);
    setBitBuf(&fltIn.bitBuf, inFile.buf+TP_SIZE_WORD,
              inFile.buf+TP_SIZE_WORD, 0);

    outFp = fopen(outFilename, "w");

    if (filterMode==ES_MODE) {
        if (seekSeqHdr(&fltIn)) {
            printf("EOF reached while searching for first "\
                   "sequence start code\n");
            exit (0);
        }
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

