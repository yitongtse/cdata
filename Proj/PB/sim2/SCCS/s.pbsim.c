h57163
s 00013/00007/00947
d D 1.10 03/03/07 13:49:34 ytse 10 9
c Update
e
s 00006/00002/00948
d D 1.9 03/03/06 18:02:37 ytse 9 8
c Update
e
s 00007/00007/00943
d D 1.8 03/03/06 17:16:23 ytse 8 7
c Version for new benchmark
e
s 00035/00005/00915
d D 1.7 03/03/06 15:11:47 ytse 7 6
c Added UDP lookup
e
s 00012/00010/00908
d D 1.6 03/03/06 10:38:13 ytse 6 5
c Update
e
s 00006/00012/00912
d D 1.5 03/03/05 14:45:51 ytse 5 4
c update
e
s 00054/00036/00870
d D 1.4 03/03/05 14:10:25 ytse 4 3
c Backup
e
s 00031/00027/00875
d D 1.3 03/03/03 14:32:33 ytse 3 2
c Update before studying dcbt
e
s 00000/00000/00902
d D 1.2 03/02/28 15:19:31 ytse 2 1
c Update
e
s 00902/00000/00000
d D 1.1 03/02/28 14:21:48 ytse 1 0
c date and time created 03/02/28 14:21:48 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    Palm Beach Packet Processing Benchmark (with input FPGA simulation)

    This program accepts MPEG/UDP/IP data in the format as described
    in ipEncap2.c.  It will produce a set of descriptors which instructs
    Palm Beach hardware to fetch the appropriate MPEG packets for output.

    The first-in-first-out scheduling algorithm is used here.

    To do:
      Clock recovery
      Delivery time computation
      Descriptor/output command flush
      Input FPGA simulation

I 7
    Changes:
      UDP table lookup option

E 7
*****************************************************************************/

// Major compilation flags
//
D 3
#define SUN				1
#define QNX             		0
E 3
I 3
D 4
#define SUN			1
#define QNX             	0
E 4
I 4
D 5
#define SUN			0
#define QNX             	1
E 5
I 5
D 7
#define SUN			1
#define QNX             	0
E 7
I 7
D 10
#define SUN			0
#define QNX             	1
E 10
I 10
#define SUN			1
#define QNX             	0
E 10
E 7
E 5
E 4
E 3

I 3
D 4
#define CHECK			0
E 4
I 4
D 5
#define STAT			1
#define PREFETCH		1
E 5
I 5
D 10
#define STAT			0
E 10
I 10
#define STAT			1
E 10
D 8
#define PREFETCH		0
I 7
#define UDP_LUT			0
E 8
I 8
#define PREFETCH		1
#define UDP_LUT			1
E 8
D 10
				// Use look up table instead of fix pattern
E 10
I 10
				// Use lookup table instead of fix pattern
E 10
E 7
E 5
E 4

I 7
D 10

E 10
E 7
E 3
#include <stdio.h>
#include <stdlib.h>

#if QNX
#include <sys/neutrino.h>
#include <inttypes.h>
#include <sys/syspage.h>
#include "bench.h"
#endif

#include "param.h"


// Basic type definitions
typedef char		Int8;
typedef short		Int16;
typedef int		Int32;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned int	Uint32;


#if QNX
static double inCycle = 0.;
static double muxCycle = 0.;
static double totCycle = 0.;
#endif


#define TP_SIZE				188
#define DATA_TP_SIZE			64
//#define PKTBUF_SIZE			1504
#define PKTBUF_SIZE			512

#define MAX_QAM				24
#define MAX_PROG_PER_QAM		16
#define MAX_PROG			(MAX_QAM * MAX_PROG_PER_QAM)
#define MAX_PID_PER_PROG		4
#define MAX_PKTBUF			16000
#define MAX_TPINFO_PER_PROG		1024
#define MAX_SLOT			140
#define MAX_IN_DESCRIPTOR		1024
#define MAX_OUTCMD			4096
#define MAX_OUT_DESCRIPTOR		(MAX_OUTCMD * 2)
#define MAX_DATA_SIZE			1024 * 1024

#define SCHWIN_SIZE			(4 * 27000)
					// schedule window size in 27 MHz ticks

#define SYSTEM_DELAY			(100 * 27000)
					// system delay in 27 MHz ticks

#define DEFAULT_CBR			3750000
					// default CBR rate of a program
#define	DEFAULT_TP_INTRVL		(27000000 / DEFAULT_CBR * 1504)
					// default TP delivery increment

#define	TPINFO_FLAG_HAS_PCR		0x01

#define PIDTYPE_VIDEO			1
#define PIDTYPE_AUDIO			2
#define PIDTYPE_PMT			3

// Dummy output command values
#define	CTRL_FLAG			0x012345
#define	TBOFFSET_BASE1			0x234567
#define	TBOFFSET_BASE2			0x456789
#define	TBOFFSET_EXT			0x6789ab
D 4
#define	SCRAMBLE_CTRLWORD		0x89abcd
E 4
I 4
#define	SCRAMBLE_CTRLWORD_1		0x89abcd
#define	SCRAMBLE_CTRLWORD_2		0x89abcd
E 4


// Transport packet info (16 bytes)
typedef struct {
    Uint8  flags;
    Uint8  pidType;
    Uint16 outPid;
    Uint8  *addr;
    Uint32 pcr;
    Uint32 outTime;		// delivery time
} TpInfo;
 

// PID info
typedef struct {
    Uint16 inPid;
    Uint16 outPid;
    Uint8  pidType;
} PidInfo;


// QAM info
typedef struct {
    Int32 numProg;		// number of programs in this QAM
    Int32 numSlot;		// number of slot used
    Int32 slotIntrvl;		// slot interval (in 27 MHz ticks)

    Uint32 minOutTime;          // minimal output time of all programs in QAM
    Int32  nextProg;            // program which assiciates to minOutTime
                                // (-1 means minOutTime is invalid)
} QamInfo;


// This structure contains all information of a channel for simulation
typedef struct {
    int  udpDestPort;
    int  numPid;             // max 4
    PidInfo pidMap[4];
} ChanInfo;


// Program info
typedef struct {
    Uint32  rdIdx;		// read index of corresponding TpInfo
    Uint32  wrIdx;		// write index of corresponding TpInfo
    Uint8   numPid;		// number of active PIDs
    PidInfo pidMap[MAX_PID_PER_PROG];	// PID mapping table

    // Clock recovery related variables
    Uint32  numInTp;		// number of TPs processed for this program
    Uint32  prevPcrTpNum;       // TP number of previous PCR carrying TP
    Uint32  prevPcr;		// previous PCR value
    Int32   tpIntrvl;		// TP interval
    Uint32  prevOutTime;	// delivery time of previous Tp
    Uint32  nextOutTime;        // delivery time of next TP
    Uint32  numOutTp;		// number of TPs output for this program
				// (debugging only)
} ProgInfo;


// MUX table (per QAM)
typedef struct {
    Uint16 used;		// whether this slot is used or not
    Uint16 offset;		// offset of descriptor used
} MuxTable;


// Descriptor (16 bytes)
typedef struct {
    Uint32 cmdStat;		// command / status
    Uint32 byteCnt;
    Uint8* srcAddr;
    void*  next;
} Descriptor;


typedef struct {
    // IP header (overloaded with internal data)
    Uint32 ipPktSz;
    Uint32 ipHdrSz;
    Uint32 numTpPerIp;
    Uint32 inTpSz;
    Uint32 arvlTime;

    // UDP header
    Uint16 srcPort;
    Uint16 dstPort;
    Uint16 length;
    Uint16 chksum;
} IpHdr;


typedef struct {
    Uint32 tpHdr;
    Uint32 tpPyld[46];
} MpegPkt;


// Output command format (need to align with HW!)
//
typedef struct {
    Uint32 ctrlFlag;
    Uint32 outPid;
    Uint32 outCc;		// for PSI TPs only
    Uint32 tbOffsetBase1;
    Uint32 tbOffsetBase2;
    Uint32 tbOffsetExt;
D 4
    Uint32 scrambleCtrlWord;
E 4
I 4
    Uint32 scrambleCtrlWord[2];
E 4
} OutCmd;


// IP Packet Buffer - Cells of 1500 bytes, large enough to hold one IP packet 
//
typedef struct {
    Uint8 data[PKTBUF_SIZE];
} PktBuf;


// Global variables
PktBuf     pktBuf[MAX_PKTBUF];	// packet buffer pool
Uint32     numPbUsed;		// number of packet buffer with data
Uint32     pbRd;		// packet buffer index for input
Uint32     pbWr;		// packet buffer index for output

ChanInfo   chanInfo[MAX_QAM * MAX_PROG_PER_QAM];
QamInfo    qamInfo[MAX_QAM];
ProgInfo   progInfo[MAX_QAM][MAX_PROG_PER_QAM];
TpInfo     tpInfo[MAX_QAM][MAX_PROG_PER_QAM][MAX_TPINFO_PER_PROG];

MuxTable   muxTbl[MAX_QAM][MAX_SLOT];
OutCmd     outCmd[MAX_OUTCMD];

Descriptor inDsc[MAX_IN_DESCRIPTOR];	// descriptors for input IP packets
Descriptor outDsc[MAX_OUT_DESCRIPTOR];	// descriptors for output command
					// and MPEG packets (interleaved)

Uint32     inDscWr;		// input descriptor write index for HW
Uint32     inDscRd;		// input descriptor read index for SW
Uint32     outDscIdx;		// current output descriptor index
Uint32     outCmdBase;		// 1st output command descriptor index for
				// current mux schedule window

Uint32     inStartTime;		// input window start time (local 27 MHz)
Uint32     inStopTime;		// input window stop time (local 27 MHz)
Uint32     schStartTime;	// schedule window start time (local 27 MHz)
Uint32     schStopTime;		// schedule window stop time (local 27 MHz)

PktBuf     nullMpegPkt;		// special MPEG null packet for stuffing

char       dataFile[128];
Uint32     rawData[1024*1024];
int        dataSize;
int        numSchWin;
int        startTime;
int        verbose;

int        numQam;
int        numProg;
int        qamRate;


// Important operating variables

int schWin = SCHWIN_SIZE / 27000;  // schedule window size in msec
int numSlot;			// number of slots in mux table
int numOutCmd;			// number of output commands in schedule window

int tpTime;			// duration between adjacent TP
				// (assuming 3.75 Mbps)



int hwArvlTimeAdj = 0;		// HW arrival time adjustment (for data reuse)
int swArvlTimeAdj = 0;		// SW arrival time adjustment (for data reuse)
int numInTp = 0;		// number of TPs processed in input task
int numMuxTp = 0;		// number of TPs processed in mux task
int numOutTp = 0;		// number of output TPs
int numNullTp = 0;		// number of null TP inserted
int numInNullTp = 0;		// number of input null TPs

I 7
#if UDP_LUT
#define MAX_UDP_PORT		65536
Uint16 udpPortLut[MAX_UDP_PORT];
#endif

E 7
I 3
D 4
#if CHECK
E 4
I 4
#if STAT
// Statistics
E 4
int numMuxCmp = 0;
int numPidCmp = 0;
int numUdpLu = 0;
I 4
int numPcr = 0;
I 10
Uint32 signature = 0;
E 10
E 4
#endif
E 3


I 4

I 6
#if QNX
E 6
void data_hint(void* addr)
{
    asm("dcbt 0,3");
}
I 6
#endif
E 6


E 4
// Get QAM ID from UDP port number
static __inline__ int udpPort2qamId(Uint16 port)
{
    return (port>>6) & 0xff;
}


// Get program (or session) ID from UDP port number
static __inline__ int udpPort2progId(Uint16 port)
{
    return (port>>1) & 0x1f;
}


// Clock recovery for every PCR carrying MPEG packet
void clockRecovery(ProgInfo* pProgInfo, TpInfo* pTpInfo, Uint32 arvlTime)
{
    pTpInfo->outTime = arvlTime;
    pProgInfo->prevOutTime = pTpInfo->outTime;
}


// Delivery time calculation for every MPEG packet
void findOutTime(ProgInfo* pProgInfo, TpInfo* pTpInfo)
{
    pTpInfo->outTime = pProgInfo->prevOutTime + pProgInfo->tpIntrvl;
    pProgInfo->prevOutTime = pTpInfo->outTime;
}


void init()
{
    int i, k;
    int qamId;
    int progId;
    int sz;
    ProgInfo* pProgInfo;
    Uint32* rawPtr;
    Uint32 ipHdrSz, numTpPerIp, tpSz;
    FILE* fp;

    // Read input data from file to memory (in raw format)
    printf("Reading data from file %s...", dataFile);
    fp = fopen(dataFile, "r");
    if (fp==NULL) {
        printf("\nError: Failed to open input file %s\n", dataFile);
        exit -1;
    }

    sz = fread(&rawData, 1, dataSize, fp);
    if (sz != dataSize) {
        printf("\nError: Failed to read data.  Only %d bytes read.\n", sz);
        exit -1;
    }

    // Copy data to packet buffer, converting from raw to actual input format
    pbRd = pbWr = numPbUsed = 0;
    sz = 0;
    rawPtr = (Uint32*)rawData;

    while (sz < dataSize) {
        Uint8* ptr = (Uint8*)&pktBuf[numPbUsed++];
        sz += rawPtr[0];		// IP packet size
        ipHdrSz = rawPtr[1];		// IP header size
        numTpPerIp = rawPtr[2];		// number of TP in IP
        tpSz = rawPtr[3];		// TP size in data

        // Copy IP header
        memcpy(ptr, rawPtr, ipHdrSz);
        rawPtr += ipHdrSz >> 2;
        ptr += ipHdrSz;

        // Copy all TPs
        for (i=0; i<numTpPerIp; i++) {
            memcpy(ptr, rawPtr, tpSz);
            rawPtr += tpSz >> 2;
            ptr += DATA_TP_SIZE;
        }

        if (numPbUsed >= MAX_PKTBUF) {
            printf("\nError: run out of packet buffer!\n");
            exit(-1);
        }
    }

    printf("\nRaw data size: %d bytes", dataSize);
    printf("\n%d cells read", numPbUsed);
    printf("\nQAM rate: %d bps", qamRate);
    printf("\nSchedule window size: %d ms", schWin);
I 7
    printf("\nPacket buffer starts at: %08x", pktBuf);
I 8
    printf("\nInput descriptor buffer starts at: %08x", inDsc);
    printf("\nOutput descriptor buffer starts at: %08x", outDsc);
E 8
E 7

    memset(qamInfo, 0, sizeof(qamInfo));
    memset(progInfo, 0, sizeof(progInfo));
    memset(tpInfo, 0, sizeof(tpInfo));
    memset(muxTbl, 0, sizeof(muxTbl));
    memset(inDsc, 0, sizeof(inDsc));
    memset(outDsc, 0, sizeof(outDsc));
    memset(outCmd, 0, sizeof(outCmd));

    // Set up input descriptors
    inDscRd = inDscWr = 0;
    for (i=0; i<MAX_IN_DESCRIPTOR; i++)
        inDsc[i].srcAddr = (Uint8*)&pktBuf[i];
    pbRd = MAX_IN_DESCRIPTOR;

    // Set up output descriptors
    outCmdBase = 0;
    for (i=0; i<MAX_IN_DESCRIPTOR; i+=2)
        outDsc[i].srcAddr = (Uint8*)&outCmd[i];

    // Set up mux table
    numSlot = qamRate * schWin / (1000 * TP_SIZE * 8) + 1;
    numOutCmd = numQam * numSlot;
    for (i=k=0; i<numSlot; i++)
        for (qamId=0; qamId<numQam; qamId++)
            muxTbl[qamId][i].offset = k++;

//    tpTime = 10814;
    tpTime = 2 * schWin * 27000 / numSlot;    // same as slotIntrvl x 2

    for (qamId=0; qamId<MAX_QAM; qamId++) {
        // Set up QamInfo
D 3
        qamInfo[qamId].numProg = numProg;
E 3
        qamInfo[qamId].numSlot = numSlot;
        qamInfo[qamId].slotIntrvl = schWin * 27000 / numSlot;

        // Set up ProgInfo
        for (progId=0; progId<MAX_PROG_PER_QAM; progId++) {
            pProgInfo = &progInfo[qamId][progId];
            pProgInfo->rdIdx = pProgInfo->wrIdx = 0;
            pProgInfo->numPid = 0;
            pProgInfo->tpIntrvl = DEFAULT_TP_INTRVL;
        }
    }

    printf("\nNumber of slots in mux table: %d", qamInfo[0].numSlot);
    printf("\nSlot interval: %08x", qamInfo[0].slotIntrvl);
#if 0
    printf("\nMux table:");
    for (i=0; i<numSlot; i++)
        printf("\n  Slot %d: %d", i, muxTbl[0][i].offset);
#endif

    // Set up programs
    for (i=0; i<numProg; i++) {
        qamId = udpPort2qamId(chanInfo[i].udpDestPort);
I 3
D 7
        qamInfo[qamId].numProg++;
E 7
E 3
        progId = udpPort2progId(chanInfo[i].udpDestPort);
I 7
#if UDP_LUT
        // Populate UDP look up table
        udpPortLut[chanInfo[i].udpDestPort] = (qamId << 8) | progId;
#endif
        qamInfo[qamId].numProg++;
E 7
        pProgInfo = &progInfo[qamId][progId];
        pProgInfo->numPid = chanInfo[i].numPid;
        for (k=0; k<pProgInfo->numPid; k++) {
            pProgInfo->pidMap[k].pidType = chanInfo[i].pidMap[k].pidType;
            pProgInfo->pidMap[k].inPid = chanInfo[i].pidMap[k].inPid;
            pProgInfo->pidMap[k].outPid = chanInfo[i].pidMap[k].outPid;
        }

#if 0
        printf("\n\nUDP %04x, QAM %d, prog %d",
            chanInfo[i].udpDestPort, qamId, progId);
        for (k=0; k<pProgInfo->numPid; k++)
            printf("\nPID %d->%d, type %d", pProgInfo->pidMap[k].inPid,
                pProgInfo->pidMap[k].outPid, pProgInfo->pidMap[k].pidType);
#endif
    }
}


// Special processing for PMT packet
void procPmtTp(TpInfo* pTpInfo)
{
}


// input HW simulation
//
void inputHwSim()
{
    if (verbose)
        printf("\n\nInput HW Sim: %08x - %08x", inStartTime, inStopTime);

    while (1) {
        Descriptor* pDsc = &inDsc[inDscWr];
        IpHdr* pIpHdr = (IpHdr*)pDsc->srcAddr;
        Uint32 arvlTime = pIpHdr->arvlTime + hwArvlTimeAdj;

D 4
#if SUN
E 4
#if 0
        printf("\n  Dsc %d, PktBuf %d, arvl %08x",
               inDscWr, ((PktBuf*)pIpHdr) - pktBuf, arvlTime);
#endif

        if (pDsc->cmdStat>>31) {
            printf("\nError: input descriptor ownership bit set!");
        }
D 4
#endif
E 4

        if (arvlTime > inStopTime)  return;

        pDsc->cmdStat |= 0x80000000;    // give ownership to CPU

        // Check for descriptor wrap around
        if (++inDscWr >= MAX_IN_DESCRIPTOR) {
            inDscWr = 0;
        }

        // Check for input data wrap around
        if (pDsc->srcAddr == (Uint8*)&pktBuf[numPbUsed-1]) {
            hwArvlTimeAdj = arvlTime;
            printf("\n\n**** Data wrap around ****");
        }
    }
}


// Input module
//
void inputTask(void)
{
    IpHdr*    pIpHdr;
    MpegPkt*  pMpegPkt;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;
    int qamId;
    int progId;
    int pid;
    int arvlTime;
    int numMpegPkt;
    int i, k;
    Uint32 pcrBase, pcrExt;
    Uint8* ptr;
I 7
    Uint16 udpMap;
I 9
    Uint32 tpHdr;
E 9
E 7
I 4
D 5
Uint32 temp;
E 5
E 4

    if (verbose)
        printf("\n\nInput: %08x -> %08x", inStartTime, inStopTime);

    // Process each packet buffer available
    while (1) {
        Descriptor* pDsc = &inDsc[inDscRd];

        // Check ownership bit
        if (!(pDsc->cmdStat >> 31))
            return;
D 4
 
E 4
I 4

E 4
        pIpHdr = (IpHdr*)pDsc->srcAddr;
        arvlTime = pIpHdr->arvlTime + swArvlTimeAdj;
        numMpegPkt = pIpHdr->length / TP_SIZE;
I 7

#if UDP_LUT
        udpMap = udpPortLut[pIpHdr->dstPort];
        if (udpMap==0xFFFF) {
            printf("\nUnconfigured UDP port %d", pIpHdr->dstPort);
            goto next;
        }
        qamId = udpMap >> 8;
        progId = udpMap & 0xff;
#else
E 7
        qamId = udpPort2qamId(pIpHdr->dstPort);
I 7
        progId = udpPort2progId(pIpHdr->dstPort);
#endif

E 7
I 3
D 4
#if CHECK
E 4
I 4
#if STAT
E 4
        numUdpLu++;
#endif
E 3
D 7
        progId = udpPort2progId(pIpHdr->dstPort);
E 7
        pProgInfo = &progInfo[qamId][progId];

#if 0
        printf("\n  Dsc %d, IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs",
            inDscRd, ((PktBuf*)pDsc->srcAddr) - pktBuf, arvlTime,
            pIpHdr->dstPort, qamId, progId, numMpegPkt);
#endif

I 4

D 5
        // Pre-fetch next IP packet header
E 5
        // Check for descriptor wrap around
        if (++inDscRd >= MAX_IN_DESCRIPTOR)  inDscRd = 0;
I 5

E 5
D 6
#if PREFETCH
E 6
I 6
#if PREFETCH && QNX
E 6
D 5
        // Prefetch next IP header
E 5
I 5
        // Prefetch next IP packet header
E 5
D 8
        data_hint(&inDsc[inDscRd]);
E 8
I 8
        ptr = (Uint8*)(&inDsc[inDscRd]);
        data_hint(ptr);
        data_hint(ptr+32);
E 8
#endif

E 4
        ptr = ((Uint8*)pDsc->srcAddr) + pIpHdr->ipHdrSz;

        // Process each MPEG transport packet in the IP packet
        for (i=0; i<numMpegPkt; i++, ptr+=DATA_TP_SIZE) {
I 4
D 6
#if PREFETCH
E 6
I 6
D 8
#if PREFETCH && QNX
E 6
            // Prefetch next TP header
            data_hint(ptr+DATA_TP_SIZE);
#endif
E 8
E 4
            pMpegPkt = (MpegPkt*)ptr;
            pTpInfo = &tpInfo[qamId][progId][pProgInfo->wrIdx++];
            if (pProgInfo->wrIdx >= MAX_TPINFO_PER_PROG)  pProgInfo->wrIdx = 0;
            pTpInfo->addr = (Uint8*)pMpegPkt;
            pProgInfo->numInTp++;
            numInTp++;

            // Find delivery time
//            findOutTime(pProgInfo, pTpInfo);
            pTpInfo->outTime = arvlTime + i * tpTime;    // dummy delivery time
            pProgInfo->prevOutTime = pTpInfo->outTime;

I 9
D 10
            tpHdr = pMpegPkt->tpHdr;
E 10
I 10
            tpHdr = pMpegPkt->tpHdr;		// cache miss likely here
E 10
//            tpHdr = 0x47019010;

E 9
            // PID mapping
D 4
            pid = (pMpegPkt->tpHdr >> 8) & 0x1fff;
E 4
I 4
D 9
            pid = (pMpegPkt->tpHdr >> 8) & 0x1fff;	// cache miss here
E 9
I 9
            pid = (tpHdr >> 8) & 0x1fff;	// cache miss here
E 9
E 4
            for (k=0; k<pProgInfo->numPid; k++) {
I 3
D 4
#if CHECK
E 4
I 4
#if STAT
E 4
                numPidCmp++;
#endif
E 3
                if (pid == pProgInfo->pidMap[k].inPid) {
                    pTpInfo->outPid = pProgInfo->pidMap[k].outPid;
                    pTpInfo->pidType = pProgInfo->pidMap[k].pidType;
                    break;
                }
            }
D 4
#if 0
            printf("\n  TP %d: PID %d", i, pid);
#endif
E 4

            // Dropping all other unknown PIDs (including NULL TPs)
            if (k == pProgInfo->numPid) {
                numInNullTp++;
                continue;
            }

            pTpInfo->flags = 0;

            // Parse TP for PCR flags and PCR
D 9
            if ((pMpegPkt->tpHdr & 0x20)   // adaptation field presents
E 9
I 9
            if ((tpHdr & 0x20)   // adaptation field presents
E 9
                    && ((pMpegPkt->tpPyld[0] >> 24) > 0)  // AF size > 0
                    && (pMpegPkt->tpPyld[0] & 0x100000)) {    // PCR presents

I 4
D 5
#if 0

E 5
E 4
                // Read PCR
I 4
#if STAT
                numPcr++;
#endif
E 4
                pcrBase = (pMpegPkt->tpPyld[0] & 0x3f) << 17;
                pcrBase |= pMpegPkt->tpPyld[1] >> 15;
                pcrExt = pMpegPkt->tpPyld[1] & 0x1ff;
                pTpInfo->flags |= TPINFO_FLAG_HAS_PCR;
                pTpInfo->pcr = pcrBase * 300 + pcrExt;
                pTpInfo->pcr += swArvlTimeAdj;	// for benchmark only

                // Rate estimation
                pProgInfo->tpIntrvl = (pTpInfo->pcr - pProgInfo->prevPcr)
                        / (pProgInfo->numInTp - pProgInfo->prevPcrTpNum);
                pProgInfo->prevPcrTpNum = pProgInfo->numInTp;
                pProgInfo->prevPcr = pTpInfo->pcr;
D 4
#if 0
                printf("\nCBR %d, TP intrvl %08x",
                       27000000/pProgInfo->tpIntrvl*1504, pProgInfo->tpIntrvl);
#endif
E 4

                // Clock recovery
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
I 4
D 5
#endif
E 5
E 4
            }

            // Special handling for PMT, etc.
            if (pTpInfo->pidType == PIDTYPE_PMT)
                procPmtTp(pTpInfo);
        }

I 7
next:
E 7
        // Check for input data wrap around
        if (pDsc->srcAddr == (Uint8*)&pktBuf[numPbUsed-1]) {
            swArvlTimeAdj = arvlTime;
D 4
#if 0
            printf("\n**** SW Arvl Time adj: %08x ****", swArvlTimeAdj);
#endif
E 4
        }

        // Prepare input descriptor for next use
        pDsc->srcAddr = (Uint8*)&pktBuf[pbRd];
        pDsc->cmdStat &= 0x7FFFFFFF;    // give ownership to HW
        if (++pbRd >= numPbUsed)  pbRd = 0;
D 4

        // Check for descriptor wrap around
        if (++inDscRd >= MAX_IN_DESCRIPTOR)
            inDscRd = 0;
E 4
    }
D 5

D 4
#if SUN
E 4
    printf("\nEnd of data reached.\n");
E 5
D 4
#endif
E 4
}


// MUX and output module
//
void muxTask(void)
{
    int qamId;
    int progId;
    int slotId;
    Uint32 outTime;
    TpInfo *pTpInfo;
    int outCmdIdx, outDscIdx;
    OutCmd *pOutCmd;

    if (verbose)
        printf("\n\nMux: %08x -> %08x", schStartTime, schStopTime);

    // Process all QAMs
    for (qamId=0; qamId<numQam; qamId++) {
        QamInfo* pQamInfo = &qamInfo[qamId];
I 4
        pQamInfo->nextProg = -1;
E 4

        // Prepare all programs
        for (progId=0; progId<pQamInfo->numProg; progId++) {
            ProgInfo* pProgInfo = &progInfo[qamId][progId];
D 4
            pQamInfo->nextProg = -1;
E 4

            if (pProgInfo->rdIdx == pProgInfo->wrIdx) {
                // No available TPs for this program
                pProgInfo->nextOutTime = schStopTime + 1;
            }
            else {
                pProgInfo->nextOutTime
                        = tpInfo[qamId][progId][pProgInfo->rdIdx].outTime;
            }
        }

        // Scan all output slots
        for (slotId=0, outTime=schStartTime; slotId<pQamInfo->numSlot;
                 slotId++, outTime+=pQamInfo->slotIntrvl) {

            if (pQamInfo->nextProg == -1) {
                pQamInfo->minOutTime = schStopTime + 1;

                // Find the program with minimal nextOutTime
                for (progId=0; progId<pQamInfo->numProg; progId++) {
                    ProgInfo* pProgInfo = &progInfo[qamId][progId];
                    if (pProgInfo->numPid == 0)  continue;
I 3
D 4
#if CHECK
E 4
I 4
#if STAT
E 4
                    numMuxCmp++;
#endif
E 3
                    if (pProgInfo->nextOutTime < pQamInfo->minOutTime) {
                        pQamInfo->minOutTime = pProgInfo->nextOutTime;
                        pQamInfo->nextProg = progId;
                    }
                }
            }

            // Find output command descriptor index for this MPEG packet
            outCmdIdx = (outCmdBase + muxTbl[qamId][slotId].offset)
                            % MAX_OUTCMD;
            outDscIdx = (outCmdIdx << 1) + 1;
            pOutCmd = &outCmd[outCmdIdx];
I 10
#if STAT
            signature += (outCmdIdx+1) * (pQamInfo->nextProg+2);
#endif
E 10

            if (pQamInfo->nextProg == -1 ||
                    (Int32)(pQamInfo->minOutTime - outTime) > 0) {
                // Output a null packet

                // Set up output command for hardware (dummy code)
                pOutCmd->ctrlFlag = CTRL_FLAG;

                // Assume no extra setup for output command descriptor

                // Set up descriptor for output packet
                outDsc[outDscIdx].srcAddr = (Uint8*)&nullMpegPkt;
                numNullTp++;
I 3
//                printf("null TP %d", numNullTp);
E 3
            }
            else {
                // Output a packet
                ProgInfo* pProgInfo = &progInfo[qamId][pQamInfo->nextProg];
                pTpInfo = &tpInfo[qamId][pQamInfo->nextProg][pProgInfo->rdIdx];

                // Set up output command for hardware (dummy code)
                pOutCmd->ctrlFlag = CTRL_FLAG;
                pOutCmd->outPid = pTpInfo->outPid;
                if (pTpInfo->flags | TPINFO_FLAG_HAS_PCR) {
                    pOutCmd->tbOffsetBase1 = TBOFFSET_BASE1;
                    pOutCmd->tbOffsetBase2 = TBOFFSET_BASE2;
                    pOutCmd->tbOffsetExt = TBOFFSET_EXT;
                }
D 4
                pOutCmd->scrambleCtrlWord = SCRAMBLE_CTRLWORD;
E 4
I 4
                pOutCmd->scrambleCtrlWord[0] = SCRAMBLE_CTRLWORD_1;
                pOutCmd->scrambleCtrlWord[1] = SCRAMBLE_CTRLWORD_2;
E 4

                // Assume no extra setup for output command descriptor

                // Set up descriptor for output packet
                outDsc[outDscIdx].srcAddr
                    = tpInfo[qamId][pQamInfo->nextProg][pProgInfo->rdIdx].addr;
#if 0
                pTpInfo = &tpInfo[qamId][pQamInfo->nextProg][pProgInfo->rdIdx];
                printf("\n %08x: %d (%d): TP %d - %d : %08x", outTime,
                    outDscIdx, slotId, pTpInfo->outPid, pProgInfo->rdIdx,
                    pTpInfo->outTime);
#endif

                // Update TpInfo read pointer
                pProgInfo->rdIdx++;
                if (pProgInfo->rdIdx > MAX_TPINFO_PER_PROG)
                    pProgInfo->rdIdx = 0;

                if (pProgInfo->rdIdx == pProgInfo->wrIdx) {
                    // End of program reached!
                    pProgInfo->nextOutTime = schStopTime + 1;
                }
                else {
                    pProgInfo->nextOutTime = tpInfo[qamId][pQamInfo->nextProg]\
                                                   [pProgInfo->rdIdx].outTime;
                }

                pProgInfo->numOutTp++;
                numMuxTp++;
I 3

//            printf("Prog %d, TP %d", pQamInfo->nextProg, pProgInfo->numOutTp);

E 3
                pQamInfo->nextProg = -1;
            }

            numOutTp++;
        }
    }

    // Update descriptor base pointer
    outCmdBase += numOutCmd;
}


D 3
#if QNX
#define CACHE_BLOCK_IN_WORD  8 // 32B=4WD
#define CACHE_SIZE           32768 // cache size in byte
#define CACHE_SIZE_X2_WORD   16384 // cache size x2 in word
unsigned int cache_stuffing[CACHE_SIZE_X2_WORD];

int cache_flush(void) {
  int i;
  unsigned int temp = 0;
  for (i=0; i<CACHE_SIZE_X2_WORD; i+=8 ) {
    temp += cache_stuffing[i];
  }

  if (temp > 0) {
    return 1;
  } else {
    return 0;
  }
}
#endif


E 3
int main(int argc, char** argv)
{
    Param par;
    int i, j;
    int scale;
D 3
    int verbose = 0;
E 3
    int qamId, progId;
    char label[32];
    int inPid, outPid, pidType;
#if QNX
    time_entry_t start, stop;
#endif

    if (argc>1)  beginParam(&par, argc, argv);
D 7
    else  readParam(&par, "pbsim.par");
E 7
I 7
    else  readParam(&par, "par.10x15");
E 7
    getStringParam(&par, "IP data file", dataFile, "in.dat");
    getIntParam(&par, "Data size (bytes)", &dataSize, 1024);
    getIntParam(&par, "Local start time", &startTime, 10);
    getIntParam(&par, "Number of schedule windows to simulate", &numSchWin, 10);
    getIntParam(&par, "Verbose", &verbose, 0);
    getIntParam(&par, "Number of QAMs", &numQam, 24);
    if(numProg > MAX_QAM) {
        printf("Error: too many QAMs\n");
        return EXIT_FAILURE;
    }
    getIntParam(&par, "QAM rate (bps)", &qamRate, 38810700);
    getIntParam(&par, "Number of input programs", &numProg, 2);
    if(numProg > MAX_QAM * MAX_PROG_PER_QAM) {
        printf("Error: too many input programs\n");
        return EXIT_FAILURE;
    }

    for (i=0; i<numProg; i++) {
        sprintf(label, "Program %d", i);
        commentParam(&par, "");
        commentParam(&par, label);
        getIntParam(&par, "UDP destination port number",
                    &chanInfo[i].udpDestPort, 4096);
        getIntParam(&par, "Number of PIDs", &chanInfo[i].numPid, 2);
        if (chanInfo[i].numPid > MAX_PID_PER_PROG) {
            printf("Error: too many QAMs\n");
            return EXIT_FAILURE;
        }

        for (j=0; j<chanInfo[i].numPid; j++) {
            getIntParam(&par, "Input PID", &inPid, 16);
            getIntParam(&par, "Output PID", &outPid, 16);
            getIntParam(&par, "PID type (1-Video 2-Audio, 3-PMT)", &pidType, 1);
            chanInfo[i].pidMap[j].inPid = inPid;
            chanInfo[i].pidMap[j].outPid = outPid;
            chanInfo[i].pidMap[j].pidType = pidType;
        }
    }
    endParam(&par);

    init();
    inStopTime = startTime;
    schStopTime = startTime - SYSTEM_DELAY;


    for (i=0; i<numSchWin; i++) {
I 10
#if 0
E 10
        if (verbose) {
            printf("\n\n\nPeriod %d:", i);
            for (qamId=0; qamId<numQam; qamId++)
                for (progId=0; progId<qamInfo[qamId].numProg; progId++) {
                    ProgInfo* pProgInfo = &progInfo[qamId][progId];
                    if (!pProgInfo->numPid)  continue;
                    printf("\n  QAM %d, Prog %d: rd %d, wr %d",
                        qamId, progId, pProgInfo->rdIdx, pProgInfo->wrIdx);
                }
            printf("\n  Descriptor: HW in %d, SW in %d, outBase %d",
                   inDscWr, inDscRd, outCmdBase);
        }
I 10
#endif
E 10

        // Update local time
        inStartTime = inStopTime;
        inStopTime = inStartTime + SCHWIN_SIZE;
        schStartTime = schStopTime;
        schStopTime = schStartTime + SCHWIN_SIZE;


        // Simulate Input HW
        inputHwSim();

        // Input Task
#if QNX
I 3
D 4
        cache_flush();
E 4
I 4
//        cache_flush();
E 4
E 3
        get_time(&start);
#endif
        inputTask();
#if QNX
        get_time(&stop);
        inCycle += get_time_diff(&start, &stop);
#endif

        // Mux and Output Task
#if QNX
I 3
D 4
        cache_flush();
E 4
I 4
//        cache_flush();
E 4
E 3
        get_time(&start);
#endif
        muxTask();
#if QNX
        get_time(&stop);
        muxCycle += get_time_diff(&start, &stop);
#endif
I 3

E 3
    }

D 6
#if QNX
    totCycle = inCycle + muxCycle;
    printf("\nCycles: %f, %f, %f", inCycle, muxCycle, totCycle);
    printf("\nInput: %d TP, %f cycles/TP", numInTp, inCycle/numInTp);
D 3
    printf("\nMux  : %d TP, %f cycles/TP", numMuxTp, muxCycle/numMuxTp);
E 3
I 3
    printf("\nMux  : %d TP, %f cycles/TP", numMuxTp, muxCycle/numInTp);
E 3
    printf("\nTotal: %f cycles/TP", numMuxTp, totCycle/numInTp);
#endif

E 6
    if (verbose) {
        printf("\n\nTP counts:");
        for (qamId=0; qamId<numQam; qamId++) {
            printf("\nQAM %d:", qamId);
            for (progId=0; progId<qamInfo[qamId].numProg; progId++) {
                ProgInfo* pProgInfo = &progInfo[qamId][progId];
                if (!pProgInfo->numPid)  continue;
                printf("\n  Prog %d: in %d, out %d",
                       progId, pProgInfo->numInTp, pProgInfo->numOutTp);
            }
        }
    }

    printf("\n\n\nSummary:");
    printf("\n  Input : %d TPs (%d NULL)", numInTp, numInNullTp);
    printf("\n  Mux   : %d TPs", numMuxTp);
    printf("\n  Null  : %d TPs", numNullTp);
    printf("\n  Output: %d TPs\n\n", numOutTp);
I 3
D 4
#if CHECK
    printf("\nUDP lookup: %d, PID compare: %d, Mux compare: %d\n\n",
           numUdpLu, numPidCmp, numMuxCmp);
E 4
I 4

#if STAT
    printf("\nStatistics:");
    printf("\nUDP lookup : %d", numUdpLu);
    printf("\nPID compare: %d", numPidCmp);
    printf("\nMux compare: %d", numMuxCmp);
D 10
    printf("\nPCR count  : %d\n\n", numPcr);
E 10
I 10
    printf("\nPCR count  : %d", numPcr);
    printf("\nSignature  : %08x\n\n", signature);
E 10
E 4
#endif
E 3

I 6
#if QNX
    totCycle = inCycle + muxCycle;
    printf("\nCycles: %f, %f, %f", inCycle, muxCycle, totCycle);
    printf("\nInput: %d TP, %f cycles/TP", numInTp, inCycle/numInTp);
    printf("\nMux  : %d TP, %f cycles/TP", numMuxTp, muxCycle/numInTp);
    printf("\nTotal: %f cycles/TP", numMuxTp, totCycle/numInTp);
#endif

E 6
    return EXIT_SUCCESS;
}

E 1
