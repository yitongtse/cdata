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

    Changes:
      UDP table lookup option

*****************************************************************************/

// Major compilation flags
//
#define SUN			1
#define QNX             	0

#define STAT			1
#define PREFETCH		1
#define UDP_LUT			1
				// Use lookup table instead of fix pattern

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
#define	SCRAMBLE_CTRLWORD_1		0x89abcd
#define	SCRAMBLE_CTRLWORD_2		0x89abcd


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
    Uint32 scrambleCtrlWord[2];
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

#if UDP_LUT
#define MAX_UDP_PORT		65536
Uint16 udpPortLut[MAX_UDP_PORT];
#endif

#if STAT
// Statistics
int numMuxCmp = 0;
int numPidCmp = 0;
int numUdpLu = 0;
int numPcr = 0;
Uint32 signature = 0;
#endif



#if QNX
void data_hint(void* addr)
{
    asm("dcbt 0,3");
}
#endif


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
    printf("\nPacket buffer starts at: %08x", pktBuf);
    printf("\nInput descriptor buffer starts at: %08x", inDsc);
    printf("\nOutput descriptor buffer starts at: %08x", outDsc);

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
        progId = udpPort2progId(chanInfo[i].udpDestPort);
#if UDP_LUT
        // Populate UDP look up table
        udpPortLut[chanInfo[i].udpDestPort] = (qamId << 8) | progId;
#endif
        qamInfo[qamId].numProg++;
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

#if 0
        printf("\n  Dsc %d, PktBuf %d, arvl %08x",
               inDscWr, ((PktBuf*)pIpHdr) - pktBuf, arvlTime);
#endif

        if (pDsc->cmdStat>>31) {
            printf("\nError: input descriptor ownership bit set!");
        }

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
    Uint16 udpMap;
    Uint32 tpHdr;

    if (verbose)
        printf("\n\nInput: %08x -> %08x", inStartTime, inStopTime);

    // Process each packet buffer available
    while (1) {
        Descriptor* pDsc = &inDsc[inDscRd];

        // Check ownership bit
        if (!(pDsc->cmdStat >> 31))
            return;

        pIpHdr = (IpHdr*)pDsc->srcAddr;
        arvlTime = pIpHdr->arvlTime + swArvlTimeAdj;
        numMpegPkt = pIpHdr->length / TP_SIZE;

#if UDP_LUT
        udpMap = udpPortLut[pIpHdr->dstPort];
        if (udpMap==0xFFFF) {
            printf("\nUnconfigured UDP port %d", pIpHdr->dstPort);
            goto next;
        }
        qamId = udpMap >> 8;
        progId = udpMap & 0xff;
#else
        qamId = udpPort2qamId(pIpHdr->dstPort);
        progId = udpPort2progId(pIpHdr->dstPort);
#endif

#if STAT
        numUdpLu++;
#endif
        pProgInfo = &progInfo[qamId][progId];

#if 0
        printf("\n  Dsc %d, IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs",
            inDscRd, ((PktBuf*)pDsc->srcAddr) - pktBuf, arvlTime,
            pIpHdr->dstPort, qamId, progId, numMpegPkt);
#endif


        // Check for descriptor wrap around
        if (++inDscRd >= MAX_IN_DESCRIPTOR)  inDscRd = 0;

#if PREFETCH && QNX
        // Prefetch next IP packet header
        ptr = (Uint8*)(&inDsc[inDscRd]);
        data_hint(ptr);
        data_hint(ptr+32);
#endif

        ptr = ((Uint8*)pDsc->srcAddr) + pIpHdr->ipHdrSz;

        // Process each MPEG transport packet in the IP packet
        for (i=0; i<numMpegPkt; i++, ptr+=DATA_TP_SIZE) {
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

            tpHdr = pMpegPkt->tpHdr;		// cache miss likely here
//            tpHdr = 0x47019010;

            // PID mapping
            pid = (tpHdr >> 8) & 0x1fff;	// cache miss here
            for (k=0; k<pProgInfo->numPid; k++) {
#if STAT
                numPidCmp++;
#endif
                if (pid == pProgInfo->pidMap[k].inPid) {
                    pTpInfo->outPid = pProgInfo->pidMap[k].outPid;
                    pTpInfo->pidType = pProgInfo->pidMap[k].pidType;
                    break;
                }
            }

            // Dropping all other unknown PIDs (including NULL TPs)
            if (k == pProgInfo->numPid) {
                numInNullTp++;
                continue;
            }

            pTpInfo->flags = 0;

            // Parse TP for PCR flags and PCR
            if ((tpHdr & 0x20)   // adaptation field presents
                    && ((pMpegPkt->tpPyld[0] >> 24) > 0)  // AF size > 0
                    && (pMpegPkt->tpPyld[0] & 0x100000)) {    // PCR presents

                // Read PCR
#if STAT
                numPcr++;
#endif
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

                // Clock recovery
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
            }

            // Special handling for PMT, etc.
            if (pTpInfo->pidType == PIDTYPE_PMT)
                procPmtTp(pTpInfo);
        }

next:
        // Check for input data wrap around
        if (pDsc->srcAddr == (Uint8*)&pktBuf[numPbUsed-1]) {
            swArvlTimeAdj = arvlTime;
        }

        // Prepare input descriptor for next use
        pDsc->srcAddr = (Uint8*)&pktBuf[pbRd];
        pDsc->cmdStat &= 0x7FFFFFFF;    // give ownership to HW
        if (++pbRd >= numPbUsed)  pbRd = 0;
    }
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
        pQamInfo->nextProg = -1;

        // Prepare all programs
        for (progId=0; progId<pQamInfo->numProg; progId++) {
            ProgInfo* pProgInfo = &progInfo[qamId][progId];

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
#if STAT
                    numMuxCmp++;
#endif
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
#if STAT
            signature += (outCmdIdx+1) * (pQamInfo->nextProg+2);
#endif

            if (pQamInfo->nextProg == -1 ||
                    (Int32)(pQamInfo->minOutTime - outTime) > 0) {
                // Output a null packet

                // Set up output command for hardware (dummy code)
                pOutCmd->ctrlFlag = CTRL_FLAG;

                // Assume no extra setup for output command descriptor

                // Set up descriptor for output packet
                outDsc[outDscIdx].srcAddr = (Uint8*)&nullMpegPkt;
                numNullTp++;
//                printf("null TP %d", numNullTp);
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
                pOutCmd->scrambleCtrlWord[0] = SCRAMBLE_CTRLWORD_1;
                pOutCmd->scrambleCtrlWord[1] = SCRAMBLE_CTRLWORD_2;

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

//            printf("Prog %d, TP %d", pQamInfo->nextProg, pProgInfo->numOutTp);

                pQamInfo->nextProg = -1;
            }

            numOutTp++;
        }
    }

    // Update descriptor base pointer
    outCmdBase += numOutCmd;
}


int main(int argc, char** argv)
{
    Param par;
    int i, j;
    int scale;
    int qamId, progId;
    char label[32];
    int inPid, outPid, pidType;
#if QNX
    time_entry_t start, stop;
#endif

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "par.10x15");
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
#if 0
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
#endif

        // Update local time
        inStartTime = inStopTime;
        inStopTime = inStartTime + SCHWIN_SIZE;
        schStartTime = schStopTime;
        schStopTime = schStartTime + SCHWIN_SIZE;


        // Simulate Input HW
        inputHwSim();

        // Input Task
#if QNX
//        cache_flush();
        get_time(&start);
#endif
        inputTask();
#if QNX
        get_time(&stop);
        inCycle += get_time_diff(&start, &stop);
#endif

        // Mux and Output Task
#if QNX
//        cache_flush();
        get_time(&start);
#endif
        muxTask();
#if QNX
        get_time(&stop);
        muxCycle += get_time_diff(&start, &stop);
#endif

    }

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

#if STAT
    printf("\nStatistics:");
    printf("\nUDP lookup : %d", numUdpLu);
    printf("\nPID compare: %d", numPidCmp);
    printf("\nMux compare: %d", numMuxCmp);
    printf("\nPCR count  : %d", numPcr);
    printf("\nSignature  : %08x\n\n", signature);
#endif

#if QNX
    totCycle = inCycle + muxCycle;
    printf("\nCycles: %f, %f, %f", inCycle, muxCycle, totCycle);
    printf("\nInput: %d TP, %f cycles/TP", numInTp, inCycle/numInTp);
    printf("\nMux  : %d TP, %f cycles/TP", numMuxTp, muxCycle/numInTp);
    printf("\nTotal: %f cycles/TP", numMuxTp, totCycle/numInTp);
#endif

    return EXIT_SUCCESS;
}

