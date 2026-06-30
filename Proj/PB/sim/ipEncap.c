/*****************************************************************************
    File: ipEncap.c

    Encapsulate a MPEG-2 transport stream into IP/UDP

    This program takes an MPEG transport stream and encapsulates it into
    an IP output stream.  User can specify IP header size, total IP packet
    size, number of TPs per IP, and distance between TPs in the same IP.
    Extra TP size and IP size will be zero stuffed at the end.

    This program is for software benchmark purpose only.  No actual IP
    header will be generated.  Instead, it inserts the following info
    to the IP header (index are for 32-bit word array):
        IP header[0]	IP packet size
        IP header[1]	IP header size
        IP header[2]	number of TPs in IP
        IP header[3]	TP packet size
        IP header[4]	arrival time
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
    int numIpPkt;
    int numPid;
    int inPid[8];
    int udpDestPort;
    int numTpPerIp;
    int inTpSz;
    int outTpSz;
    int ipHdrSz;
    int ipPktSz;
    int ipTrailSz;
    int pid;
    int sz;
    int bitrate;
    int i, j, k;
    unsigned int clkInc;
    unsigned int clk;
    unsigned int seed, maxJitter;
    unsigned int prevArvlTime, minArvlTime, maxArvlTime, arvlTime;

    unsigned int  ipHdrBuf[64];
    unsigned int  tpBuf[64];
    unsigned char ipTrailBuf[256];
    FILE* inFp;
    FILE* outFp;


    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipEncap.par");
    getStringParam(&par, "Input MPEG TS filename", inFilename, "test.in");
    getIntParam(&par, "Input MPEG transport packet size", &inTpSz, 188);
    getIntParam(&par, "Bitrate (bps)", &bitrate, 1);
    getIntParam(&par, "UDP destination port number", &udpDestPort, 1024);
    getIntParam(&par, "Number of IP packets to generate", &numIpPkt, 1024);
    getIntParam(&par, "IP packet size", &ipPktSz, 1500);
    getIntParam(&par, "IP header size", &ipHdrSz, 40);
    getIntParam(&par, "MPEG-2 packets per IP", &numTpPerIp, 7);
    getIntParam(&par, "Output MPEG-2 packet size", &outTpSz, 188);
    getIntParam(&par, "PIDs to include (max 8)", &numPid, 3);
    for (i=0; i<numPid; i++)
        getIntParam(&par, "PID", &inPid[i], 16);
    getIntParam(&par, "Max network jitter (ms)", &maxJitter, 100);
    getIntParam(&par, "Random number generator seed", &seed, 0);
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);


    if (ipHdrSz < 28)
        printf("Warning: IP packet header should be >= 28 bytes!\n");

    if (outTpSz < TP_SIZE)
        printf("Warning: Output MPEG packet should be >= 188 bytes!\n");

    ipTrailSz = ipPktSz - (ipHdrSz + outTpSz * numTpPerIp);
    if (ipTrailSz < 0) {
        printf("Error: IP packet size too small to fit data!");
        exit (-1);
    }

    inFp = fopen(inFilename, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open file %s\n", inFilename);
        exit (-1);
    }

    /* Transport packet alignment */
    if (findTp(inFp, inTpSz, 1)) {
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
    printf("Sync at byte %d\n", ftell(inFp));

    outFp = fopen(outFilename, "w");

    clkInc = 27000000. * 1504 / bitrate;	// clock increment per TP
    maxJitter *= 27000;				// in 27 MHz clock ticks

    prevArvlTime = 0;
    clk = numTpPerIp * clkInc;			// local clock
    minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
    maxArvlTime = clk + maxJitter;
    srand(seed);
    arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;

    // Prepare IP/UDP header
    memset(ipHdrBuf, 0, ipHdrSz);
    ipHdrBuf[0] = ipPktSz;
    ipHdrBuf[1] = ipHdrSz;
    ipHdrBuf[2] = numTpPerIp;
    ipHdrBuf[3] = outTpSz;
    ipHdrBuf[5] = udpDestPort;
    ipHdrBuf[6] = (numTpPerIp * TP_SIZE) << 16;		// Note: real data size

    // Prepare IP trailer
    memset(ipTrailBuf, 0, 256);

    // Prepare TP buffer
    memset(tpBuf, 0, 256);

    for (i=0; i<numIpPkt; i++) {
        ipHdrBuf[4] = arvlTime;
        fwrite(ipHdrBuf, 1, ipHdrSz, outFp);	// insert IP/UDP header

        for (j=0; j<numTpPerIp; j++) {
            while (1) {
                sz = fread(tpBuf, 1, inTpSz, inFp);
                if (sz < inTpSz) {
                    printf("Warning: %d IP packets generated."\
                           "  Data may be truncated.\n", i);
                    return;
                }

                pid = (tpBuf[0] >> 8) & 0x1FFF;
                for (k=0; k<numPid; k++) {
                    if (pid == inPid[k])  break;
                }
                if (k<numPid)  break;		// TP to include found
            }

            fwrite(tpBuf, 1, outTpSz, outFp);
        }

        if (ipTrailSz)
            fwrite(ipTrailBuf, 1, ipTrailSz, outFp);

        clk += numTpPerIp * clkInc;		// update local clock
        prevArvlTime = arvlTime;
        minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
        maxArvlTime = clk + maxJitter;
        arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;
    }

    printf("%d IP packets generated.\n", numIpPkt);
}
