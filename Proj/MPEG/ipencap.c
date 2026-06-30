/*****************************************************************************
    File: ipencap.c

    Encapsulate a MPEG-2 transport stream into IP/UDP

    Note: output is in a format that is used by Palm Beach software benchmark,
          where each IP/UDP header, or MPEG-2 TP is in a separated cell
          of 256 bytes.
    Note: arrival time stamping is added to the last word in the cell.
          The time stamp is a 32 bit 27 MHz local clock.
    Note: added network jitter to the arrival time.
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
    char inFilename[256];
    char outFilename[256];
    int tpMode;
    int numIpPkt;
    int numPid;
    int inPid[8];
    int udpDestPort;
    int numTpPerIp;
    int tpSz;
    int pid;
    int sz;
    int bitrate;
    int i, j, k;
    unsigned int clkInc;
    unsigned int clk;
    unsigned int maxJitter;
    unsigned int prevArvlTime, minArvlTime, maxArvlTime, arvlTime;

    InFile inFile;
    unsigned short ipHdrBuf[128];
    unsigned int tpBuf[64];
    FILE* outFp;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipencap.par");
    getStringParam(&par, "Input MPEG-2 TS filename", inFilename, "test.in");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
    getIntParam(&par, "Bitrate (bps)", &bitrate, 1);
    getIntParam(&par, "UDP destination port number", &udpDestPort, 1024);
    getIntParam(&par, "Number of IP packets to generate", &numIpPkt, 1024);
    getIntParam(&par, "MPEG-2 packets per IP", &numTpPerIp, 7);
    getIntParam(&par, "PIDs to include (max 8)", &numPid, 3);
    for (i=0; i<numPid; i++)
        getIntParam(&par, "PID", &inPid[i], 16);
    getIntParam(&par, "Max network jitter (ms)", &maxJitter, 100);
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);

    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;
    openInFile(&inFile, inFilename, tpSz);
    setReaderEcho(&inFile.rdr, 0, stdout);

    /* Transport packet alignment */
    if (findTp(inFile.fp, tpSz, 1)) {
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
    printf("Sync at byte %d\n", ftell(inFile.fp));

    outFp = fopen(outFilename, "w");

    clkInc = 27000000. * 1504 / bitrate;	// clock increment per TP
    maxJitter *= 27000;				// in 27 MHz clock ticks

    prevArvlTime = 0;
    clk = numTpPerIp * clkInc;			// local clock
    minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
    maxArvlTime = clk + maxJitter;
    arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;

    // Prepare IP/UDP header
    memset(ipHdrBuf, 0, 256);
    ipHdrBuf[11] = udpDestPort;
    ipHdrBuf[12] = numTpPerIp * TP_SIZE;

    // Prepare TP buffer
    memset(tpBuf, 0, 256);

    for (i=0; i<numIpPkt; i++) {
        ipHdrBuf[126] = arvlTime >> 16;
        ipHdrBuf[127] = arvlTime & 0xFFFF;
        fwrite(ipHdrBuf, 1, 256, outFp);	// insert IP/UDP header

        for (j=0; j<numTpPerIp; j++) {
            while (1) {
                sz = fread(tpBuf, 1, TP_SIZE, inFile.fp);
                if (sz < TP_SIZE)  return;

                pid = (tpBuf[0] >> 8) & 0x1FFF;
                for (k=0; k<numPid; k++) {
                    if (pid == inPid[k])  break;
                }
                if (k<numPid)  break;		// TP to include found
            }

            fwrite(tpBuf, 1, 256, outFp);
        }

        clk += numTpPerIp * clkInc;		// update local clock
        prevArvlTime = arvlTime;
        minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
        maxArvlTime = clk + maxJitter;
        arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;
    }
}
