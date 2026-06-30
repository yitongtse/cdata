h59129
s 00135/00000/00000
d D 1.1 02/02/25 17:54:49 ytse 1 0
c date and time created 02/02/25 17:54:49 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: pesCheck.c

    Check PES level syntax of a transport stream
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


InFile inFile;
Reader rdr;
int pid;
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
            if (findTp(inFile.fp, 1)) {
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
        if (feof(inFile.fp))
            exit(0);

        getTpHdr(&tp, &inFile.rdr);
        if (tp.PID == pid) {

            if (mode==PID_MODE) {
                rdr.bitBuf.curPtr = inFile.buf;
                rdr.bitBuf.bitPos = 32;
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
                if (mode==PES_MODE) {
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
                    copyReader(&inFile.rdr, &rdr);
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
                    copyReader(&inFile.rdr, &rdr);
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
    char filename[256];

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "pesCheck.par");
    commentParam(&par, "");
    commentParam(&par, "This program checks PES level syntax of "\
                       "a transport stream");
    getStringParam(&par, "Input transport stream", filename, "test.in");
    getIntParam(&par, "PID to examine", &pid, 0);
    endParam(&par);

    syncFlag = !syncFlag;
    openInFile(&inFile, inFilename, TP_SIZE);
    initReader(&rdr, getMorePayload, NULL);
    setBitBuf(&rdr.bitBuf, inFile.buf+TP_SIZE_WORD, inFile.buf+TP_SIZE_WORD, 0);

    // Get first PES
}

E 1
