h01116
s 00007/00001/00162
d D 1.3 03/02/28 15:19:19 ytse 3 2
c Update
e
s 00069/00027/00094
d D 1.2 02/12/03 12:02:03 ytse 2 1
c Support new data format
e
s 00121/00000/00000
d D 1.1 02/11/19 16:15:01 ytse 1 0
c date and time created 02/11/19 16:15:01 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: ipEncap.c

    Encapsulate a MPEG-2 transport stream into IP/UDP

D 2
    Note: Output is in a format that is used by Palm Beach software benchmark,
          where each IP/UDP header, or MPEG-2 TP is in a separated cell
          of 256 bytes.  Arrival time stamping is added to the last word
          in the cell.  The arrival time stamp contains simulated network
          jitter, and is a 32 bit 27 MHz local clock.
E 2
I 2
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
E 2
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
D 2
    int tpMode;
E 2
    int numIpPkt;
    int numPid;
    int inPid[8];
    int udpDestPort;
    int numTpPerIp;
D 2
    int tpSz;
E 2
I 2
    int inTpSz;
    int outTpSz;
    int ipHdrSz;
    int ipPktSz;
    int ipTrailSz;
E 2
    int pid;
    int sz;
    int bitrate;
    int i, j, k;
    unsigned int clkInc;
    unsigned int clk;
D 2
    unsigned int maxJitter;
E 2
I 2
    unsigned int seed, maxJitter;
E 2
    unsigned int prevArvlTime, minArvlTime, maxArvlTime, arvlTime;

D 2
    InFile inFile;
    unsigned short ipHdrBuf[128];
    unsigned int tpBuf[64];
E 2
I 2
    unsigned int  ipHdrBuf[64];
    unsigned int  tpBuf[64];
    unsigned char ipTrailBuf[256];
    FILE* inFp;
E 2
    FILE* outFp;

I 2

E 2
    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "ipEncap.par");
D 2
    getStringParam(&par, "Input MPEG-2 TS filename", inFilename, "test.in");
    getIntParam(&par, "Packet size type (1-188 2-204)", &tpMode, 1);
E 2
I 2
    getStringParam(&par, "Input MPEG TS filename", inFilename, "test.in");
    getIntParam(&par, "Input MPEG transport packet size", &inTpSz, 188);
E 2
    getIntParam(&par, "Bitrate (bps)", &bitrate, 1);
    getIntParam(&par, "UDP destination port number", &udpDestPort, 1024);
    getIntParam(&par, "Number of IP packets to generate", &numIpPkt, 1024);
I 2
    getIntParam(&par, "IP packet size", &ipPktSz, 1500);
    getIntParam(&par, "IP header size", &ipHdrSz, 40);
E 2
    getIntParam(&par, "MPEG-2 packets per IP", &numTpPerIp, 7);
I 2
    getIntParam(&par, "Output MPEG-2 packet size", &outTpSz, 188);
E 2
    getIntParam(&par, "PIDs to include (max 8)", &numPid, 3);
    for (i=0; i<numPid; i++)
        getIntParam(&par, "PID", &inPid[i], 16);
    getIntParam(&par, "Max network jitter (ms)", &maxJitter, 100);
I 2
    getIntParam(&par, "Random number generator seed", &seed, 0);
E 2
    getStringParam(&par, "Output filename", outFilename, "test.out");
    endParam(&par);

D 2
    tpSz = (tpMode==1)? TP_SIZE : TP_SIZE2;
    openInFile(&inFile, inFilename, tpSz);
    setReaderEcho(&inFile.rdr, 0, stdout);
E 2

I 2
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

E 2
    /* Transport packet alignment */
D 2
    if (findTp(inFile.fp, tpSz, 1)) {
E 2
I 2
    if (findTp(inFp, inTpSz, 1)) {
E 2
        printf("Failed to find SYNC.\n");
        exit (-1);
    }
D 2
    printf("Sync at byte %d\n", ftell(inFile.fp));
E 2
I 2
    printf("Sync at byte %d\n", ftell(inFp));
E 2

    outFp = fopen(outFilename, "w");

    clkInc = 27000000. * 1504 / bitrate;	// clock increment per TP
    maxJitter *= 27000;				// in 27 MHz clock ticks

    prevArvlTime = 0;
    clk = numTpPerIp * clkInc;			// local clock
    minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
    maxArvlTime = clk + maxJitter;
I 2
    srand(seed);
E 2
    arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;

    // Prepare IP/UDP header
D 2
    memset(ipHdrBuf, 0, 256);
    ipHdrBuf[11] = udpDestPort;
    ipHdrBuf[12] = numTpPerIp * TP_SIZE;
E 2
I 2
    memset(ipHdrBuf, 0, ipHdrSz);
    ipHdrBuf[0] = ipPktSz;
    ipHdrBuf[1] = ipHdrSz;
    ipHdrBuf[2] = numTpPerIp;
    ipHdrBuf[3] = outTpSz;
    ipHdrBuf[5] = udpDestPort;
    ipHdrBuf[6] = (numTpPerIp * TP_SIZE) << 16;		// Note: real data size
E 2

I 2
    // Prepare IP trailer
    memset(ipTrailBuf, 0, 256);

E 2
    // Prepare TP buffer
    memset(tpBuf, 0, 256);

    for (i=0; i<numIpPkt; i++) {
D 2
        ipHdrBuf[126] = arvlTime >> 16;
        ipHdrBuf[127] = arvlTime & 0xFFFF;
        fwrite(ipHdrBuf, 1, 256, outFp);	// insert IP/UDP header
E 2
I 2
        ipHdrBuf[4] = arvlTime;
        fwrite(ipHdrBuf, 1, ipHdrSz, outFp);	// insert IP/UDP header
E 2

        for (j=0; j<numTpPerIp; j++) {
            while (1) {
D 2
                sz = fread(tpBuf, 1, TP_SIZE, inFile.fp);
                if (sz < TP_SIZE)  return;
E 2
I 2
                sz = fread(tpBuf, 1, inTpSz, inFp);
D 3
                if (sz < inTpSz)  return;
E 3
I 3
                if (sz < inTpSz) {
                    printf("Warning: %d IP packets generated."\
                           "  Data may be truncated.\n", i);
                    return;
                }
E 3
E 2

                pid = (tpBuf[0] >> 8) & 0x1FFF;
                for (k=0; k<numPid; k++) {
                    if (pid == inPid[k])  break;
                }
                if (k<numPid)  break;		// TP to include found
            }

D 2
            fwrite(tpBuf, 1, 256, outFp);
E 2
I 2
            fwrite(tpBuf, 1, outTpSz, outFp);
E 2
        }

I 2
        if (ipTrailSz)
            fwrite(ipTrailBuf, 1, ipTrailSz, outFp);

E 2
        clk += numTpPerIp * clkInc;		// update local clock
        prevArvlTime = arvlTime;
        minArvlTime = (clk > prevArvlTime)? clk : prevArvlTime;
        maxArvlTime = clk + maxJitter;
        arvlTime = rand() / 32767. * (maxArvlTime - minArvlTime) + minArvlTime;
    }
I 3

    printf("%d IP packets generated.\n", numIpPkt);
E 3
}
E 1
