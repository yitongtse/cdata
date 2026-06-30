/*****************************************************************************
    File: ipView.c

    Show content of a of MPEG/UDP/IP streams
    (format as described in ipEncap.c)

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


int main(int argc, char** argv)
{
    Param par;
    char inFilename[128];
    int buf[256];
    int ipCnt = 0;
    FILE* fp;
    int i, sz;
    int ipPktSz, ipHdrSz, numTpPerIp, inTpSz, arvlTime, udpDestPort;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipView.par");
    getStringParam(&par, "Input filename", inFilename, "test.in");
    endParam(&par);


    fp = fopen(inFilename, "r");

    while (1) {
        sz = fread(buf, 1, 28, fp);
        if (sz < 28) {
            printf("\nEnd of file reached.\n");
            exit (0);
        }

        ipPktSz = buf[0];
        ipHdrSz = buf[1];
        numTpPerIp = buf[2];
        inTpSz = buf[3];
        arvlTime = buf[4];
        udpDestPort = buf[5] & 0xFFFF;
        printf("\nIP %d: Arvl %08x, UDP %04x, TP %d",
               ipCnt++, arvlTime, udpDestPort, numTpPerIp);

        sz = ipHdrSz - 28;
        if (sz > 0)
            fread(buf, 1, sz, fp);

        for (i=0; i<numTpPerIp; i++) {
            fread(buf, 1, inTpSz, fp);
            printf("\n  TP %d: PID %d", i, (buf[0]>>8) & 0x1FFF);
        }

        sz = ipPktSz - ipHdrSz - inTpSz*numTpPerIp;
        if (sz > 0)
            fread(buf, 1, sz, fp);
    }
}
