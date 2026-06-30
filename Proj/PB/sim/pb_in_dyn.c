/*****************************************************************************
    Palm Beach Packet Processing Benchmark (with input FPGA simulation)

    This program accepts MPEG/UDP/IP data in the format as described
    in ipEncap2.c.  It will produce a set of descriptors which instructs
    Palm Beach hardware to fetch the appropriate MPEG packets for output.

    This scheduling algorithm used in the code is based on the MLC
    dynamic table method.

    Changes:
      Use compact data format
      Introduce latency between input window and mux schedule window.
      Actual descriptor definition
      Integrate Shan's code in

    To do:
      Clock recovery
      Delivery time computation
      Descriptor/output command flush
      Input FPGA simulation

*****************************************************************************/

#define SUN				1	// for SUN simulation
#define MPC				0	// for PPC
#define IP_DATA				0


#include <stdio.h>

#if MPC
#include <stdlib.h>
#include <altivec.h>
#include "test_bench.h"
#include "cache_utils_7410.h"
#endif


// Basic type definitions
typedef char		Int8;
typedef short		Int16;
typedef int		Int32;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned int	Uint32;


#define	BENCHMARK			1	// for benchmark

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
#define	SCRAMBLE_CTRLWORD		0x89abcd


// PID info
typedef struct {
    Uint16 inPid;
    Uint16 outPid;
    Uint8  pidType;
} PidInfo;


// Transport packet info (16 bytes)
typedef struct {
    Uint8  flags;
    Uint8  pidType;
    Uint16 outPid;
    Uint8  *addr;
    Uint32 pcr;
    Uint32 outTime;		// delivery time
} TpInfo;
 

// QAM info
typedef struct {
    Int32 numProg;		// number of programs in this QAM
    Int32 numSlot;		// number of slot used
    Int32 slotIntrvl;		// slot interval (in 27 MHz ticks)
} QamInfo;


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
    Uint32 scrambleCtrlWord;

#if BENCHMARK
    // The following fields are for benchmark debugging only
    Uint16 qamId;
    Uint16 progId;
    Uint16 tpInfoIdx;		// for degging only
#endif
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


// Important operating variables

int schWin = SCHWIN_SIZE / 27000;  // schedule window size in msec
int numSlot;			// number of slots in mux table
int numOutCmd;			// number of output commands in schedule window

int tpTime;			// duration between adjacent TP
				// (assuming 3.75 Mbps)


#if BENCHMARK

#include "config10x15.h"

int hwArvlTimeAdj = 0;		// HW arrival time adjustment (for data reuse)
int swArvlTimeAdj = 0;		// SW arrival time adjustment (for data reuse)
int numInTp = 0;		// number of TPs processed in input task
int numMuxTp = 0;		// number of TPs processed in mux task
int numOutTp = 0;		// number of output TPs
int numNullTp = 0;		// number of null TP inserted

#endif  // BENCHMARK


#if IP_DATA /* [ */ 
extern Uint32 rawData[DATA_SIZE_WORD];
#else 
Uint32 rawData[DATA_SIZE_WORD];
#endif /* ] */


#if MPC
static int inCycle = 0;
static int muxCycle = 0;
static int outCycle = 0;
static int totalCycle = 0;
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


void dumpIpInfo()
{
    int i;

    for (i=0; i<MAX_PKTBUF; i++) {
        Uint32* ptr = (Uint32*)&pktBuf[i];
        printf("\nBuf %d: Arvl %08x", i, ptr[4]);
    }
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
#if !IP_DATA
    FILE* fp;
#endif

#if !IP_DATA
    // Read input data from file to memory (in raw format)
    fp = fopen("ip.dat", "r");
    if (fp==NULL) {
        printf("\nError: Failed to open input file ip.dat\n");
        exit (-1);
    }
    sz = fread(&rawData, 1, DATA_SIZE, fp);
    if (sz != DATA_SIZE) {
        printf("\nError: Failed to read data.  Only %d bytes read.\n", sz);
        exit (-1);
    }
#endif

    // Copy data to packet buffer, converting from raw to actual input format
    pbRd = pbWr = numPbUsed = 0;
    sz = 0;
    rawPtr = (Uint32*)rawData;

    while (sz < DATA_SIZE) {
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
#if SUN
            printf("\nError: run out of packet buffer!\n");
#endif
            exit(-1);
        }
    }

#if SUN
    printf("\nRaw data size: %d bytes", DATA_SIZE);
    printf("\n%d cells read", numPbUsed);
    printf("\nQAM rate: %d bps", qamRate);
    printf("\nSchedule window size: %d ms", schWin);
#endif

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
        qamInfo[qamId].numProg = numProg;
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

#if SUN
    printf("\nNumber of slots in mux table: %d", qamInfo[0].numSlot);
    printf("\nSlot interval: %08x", qamInfo[0].slotIntrvl);
#if 0
    printf("\nMux table:");
    for (i=0; i<numSlot; i++)
        printf("\n  Slot %d: %d", i, muxTbl[0][i].offset);
#endif
#endif

    // Set up programs
    for (i=0; i<NUM_CHAN; i++) {
        qamId = udpPort2qamId(chanInfo[i].udpDestPort);
        progId = udpPort2progId(chanInfo[i].udpDestPort);
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


// Returns index of the closet available slot to slotIdx
// Returns -1 if no slots available
//
int searchEmptySlot(int qamId, int slotIdx)
{
    int adj, idx, dir;
    int numSlot = qamInfo[qamId].numSlot;

    for (adj=1; 1; adj++) {
        dir = 2;
        idx = slotIdx + adj;
        if (idx > numSlot)  dir--;
        else if (!muxTbl[qamId][idx].used)  return idx;

        idx = slotIdx - adj;
        if (idx < 0)  dir--;
        else if (!muxTbl[qamId][idx].used)  return idx;

        if (dir == 0)  return -1;
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
#if SUN
    printf("\n\nInput HW Sim: %08x - %08x", inStartTime, inStopTime);
#endif

    while (1) {
        Descriptor* pDsc = &inDsc[inDscWr];
        IpHdr* pIpHdr = (IpHdr*)pDsc->srcAddr;
        Uint32 arvlTime = pIpHdr->arvlTime + hwArvlTimeAdj;

#if SUN
#if 0
        printf("\n  Dsc %d, PktBuf %d, arvl %08x",
               inDscWr, ((PktBuf*)pIpHdr) - pktBuf, arvlTime);
#endif

        if (pDsc->cmdStat>>31) {
            printf("\nError: input descriptor ownership bit set!");
        }
#endif

        if (arvlTime > inStopTime)  return;

        pDsc->cmdStat |= 0x80000000;    // give ownership to CPU

        // Check for descriptor wrap around
        if (++inDscWr >= MAX_IN_DESCRIPTOR) {
            inDscWr = 0;
        }

        // Check for input data wrap around
        if (pDsc->srcAddr == (Uint8*)&pktBuf[numPbUsed-1]) {
            hwArvlTimeAdj = arvlTime;
#if SUN
            printf("\n\n**** Data wrap around ****");
#endif
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

#if SUN
    printf("\n\nInput: %08x -> %08x", inStartTime, inStopTime);
#endif

    // Process each packet buffer available
    while (1) {
        Descriptor* pDsc = &inDsc[inDscRd];

        // Check ownership bit
        if (!(pDsc->cmdStat >> 31))
            return;
 
        pIpHdr = (IpHdr*)pDsc->srcAddr;
        arvlTime = pIpHdr->arvlTime + swArvlTimeAdj;
        numMpegPkt = pIpHdr->length / TP_SIZE;
        qamId = udpPort2qamId(pIpHdr->dstPort);
        progId = udpPort2progId(pIpHdr->dstPort);
        pProgInfo = &progInfo[qamId][progId];

#if 0
        printf("\n  Dsc %d, IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs",
            inDscRd, ((PktBuf*)pDsc->srcAddr) - pktBuf, arvlTime,
            pIpHdr->dstPort, qamId, progId, numMpegPkt);
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

            // PID mapping
            pid = (pMpegPkt->tpHdr >> 8) & 0x1fff;
            for (k=0; k<pProgInfo->numPid; k++) {
                if (pid == pProgInfo->pidMap[k].inPid) {
                    pTpInfo->outPid = pProgInfo->pidMap[k].outPid;
                    pTpInfo->pidType = pProgInfo->pidMap[k].pidType;
                    break;
                }
            }
#if 0
            printf("\n  TP %d: PID %d", i, pid);
#endif

            // Dropping all other unknown PIDs (including NULL TPs)
            if (k == pProgInfo->numPid)
                continue;

            pTpInfo->flags = 0;

            // Parse TP for PCR flags and PCR
            if ((pMpegPkt->tpHdr & 0x20)   // adaptation field presents
                    && ((pMpegPkt->tpPyld[0] >> 24) > 0)  // AF size > 0
                    && (pMpegPkt->tpPyld[0] & 0x100000)) {    // PCR presents

                // Read PCR
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
#if 0
                printf("\nCBR %d, TP intrvl %08x",
                       27000000/pProgInfo->tpIntrvl*1504, pProgInfo->tpIntrvl);
#endif

                // Clock recovery
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
            }

            // Special handling for PMT, etc.
            if (pTpInfo->pidType == PIDTYPE_PMT)
                procPmtTp(pTpInfo);
        }

        // Check for input data wrap around
        if (pDsc->srcAddr == (Uint8*)&pktBuf[numPbUsed-1]) {
            swArvlTimeAdj = arvlTime;
#if 0
            printf("\n**** SW Arvl Time adj: %08x ****", swArvlTimeAdj);
#endif
        }

        // Prepare input descriptor for next use
        pDsc->srcAddr = (Uint8*)&pktBuf[pbRd];
        pDsc->cmdStat &= 0x7FFFFFFF;    // give ownership to HW
        if (++pbRd >= numPbUsed)  pbRd = 0;

        // Check for descriptor wrap around
        if (++inDscRd >= MAX_IN_DESCRIPTOR)
            inDscRd = 0;
    }

#if SUN
    printf("\nEnd of data reached.\n");
#endif
}


// MUX module
//
void muxTask(void)
{
    int i;
    int qamId, progId;
    int slotIdx;
    int outCmdIdx;
    int outDscIdx;
    QamInfo*  pQamInfo;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;
    OutCmd*   pOutCmd;

#if SUN
    printf("\n\nMux: %08x -> %08x", schStartTime, schStopTime);
#endif

    // Process each QAM
    for (qamId=0; qamId<numQam; qamId++) {
        pQamInfo = &qamInfo[qamId];

        // Declare all slots to be not used
        for (i=0; i<pQamInfo->numSlot; i++)
            muxTbl[qamId][i].used = 0;

        // Scheduling for each program
        for (progId=0; progId<pQamInfo->numProg; progId++) {
            pProgInfo = &progInfo[qamId][progId];
            if (!pProgInfo->numPid)  continue;

            // Process all MPEG packets whose delivery time falls
            // in current scheduling window
            while (1) {
                int availTpInfo = pProgInfo->wrIdx - pProgInfo->rdIdx;
                if (availTpInfo < -MAX_TPINFO_PER_PROG/2)
                    availTpInfo += MAX_TPINFO_PER_PROG;
                if (availTpInfo <= 0)  break;	// all TpInfo processed

                pTpInfo = &tpInfo[qamId][progId][pProgInfo->rdIdx];

                // Assign a slot for next TP
                slotIdx = ((Int32)(pTpInfo->outTime - schStartTime))
                              / pQamInfo->slotIntrvl;

                if (slotIdx >= pQamInfo->numSlot) {
                    break;		// done with this program
                }

#if 0
                printf("\n  TP %0d: PID %d, type %d, outTime %08x, slot %d",
                    pProgInfo->rdIdx, pTpInfo->outPid, pTpInfo->pidType,
                    pTpInfo->outTime, slotIdx);
#endif

                if (slotIdx < 0) {
		    slotIdx = 0;	// should be in past schedule window!
#if 0
                    printf(" -> %d", slotIdx);
#endif
                }

                // find an empty slot closest to slotIdx
                if (muxTbl[qamId][slotIdx].used) {
                    slotIdx = searchEmptySlot(qamId, slotIdx);
#if 0
                    printf(" -> %d", slotIdx);
#endif
                    if (slotIdx == -1) {
#if SUN
                        printf("\n  All slots used up");
#endif
                        break;		// all slots used up
                    }
                }

                // Mark the slot used
                muxTbl[qamId][slotIdx].used = 1;

                // Find output command descriptor index for this MPEG packet
                outCmdIdx = (outCmdBase + muxTbl[qamId][slotIdx].offset)
                                 % MAX_OUTCMD;
                outDscIdx = (outCmdIdx << 1) + 1;

                // Set up output command for hardware (dummy code)
                pOutCmd = &outCmd[outCmdIdx];
                pOutCmd->ctrlFlag = CTRL_FLAG;
                pOutCmd->outPid = pTpInfo->outPid;
                if (pTpInfo->flags | TPINFO_FLAG_HAS_PCR) {
                    pOutCmd->tbOffsetBase1 = TBOFFSET_BASE1;
                    pOutCmd->tbOffsetBase2 = TBOFFSET_BASE2;
                    pOutCmd->tbOffsetExt = TBOFFSET_EXT;
                }
                pOutCmd->scrambleCtrlWord = SCRAMBLE_CTRLWORD;

                // Put debugging info to OutCmd
                pOutCmd->qamId = qamId;
                pOutCmd->progId = progId;
                pOutCmd->tpInfoIdx = pProgInfo->rdIdx;

                // Assume no extra setup for output command descriptor

                // Set up descriptor for output MPEG packet
                outDsc[outDscIdx].srcAddr = pTpInfo->addr;

                // Update TpInfo read pointer
                pProgInfo->rdIdx++;
                if (pProgInfo->rdIdx > MAX_TPINFO_PER_PROG)
                    pProgInfo->rdIdx = 0;

                pProgInfo->numOutTp++;
                numMuxTp++;
            }
        }    // Program loop
    }	// QAM loop
}


// Output Module
//
void outputTask(void)
{
    int i;
    int qamId;
    int outCmdIdx;
    int outDscIdx;
    QamInfo*  pQamInfo;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;
    OutCmd*   pOutCmd;
    Descriptor* pDsc;

    for (qamId=0; qamId<numQam; qamId++) {
        pQamInfo = &qamInfo[qamId];

        // Scan and fill null packets
        for (i=0; i<pQamInfo->numSlot; i++) {
            if (!muxTbl[qamId][i].used) {
                outCmdIdx = (outCmdBase + muxTbl[qamId][i].offset)
                                 % MAX_OUTCMD;
                outDscIdx = (outCmdIdx << 1) + 1;

                // Set up output command for hardware (dummy code)
                outCmd[outCmdIdx].ctrlFlag = CTRL_FLAG;

                // Set up descriptor to output NULL MPEG packet
                outDsc[outDscIdx].srcAddr = (Uint8*)&nullMpegPkt;

                numNullTp++;
            }
            numOutTp++;
        }
    }

    // NOTE: Need to flush descriptor to SDRAM!

#if 0
    // Dump descriptor info
    printf("\n\nOutput:");
    for (i=0; i<numOutCmd; i++) {
        outCmdIdx = (outCmdBase + i) % MAX_OUTCMD;
        outDscIdx = (outCmdIdx << 1) + 1;
        pOutCmd = &outCmd[outCmdIdx];
        pDsc = &outDsc[outDscIdx];

        if (pDsc->srcAddr != (Uint8*)&nullMpegPkt)
            printf("\n  Slot %d, OutCmd %d, QAM %d, Prog %d, PID %d, TP %d",
                i, outCmdIdx, pOutCmd->qamId, pOutCmd->progId,
                pOutCmd->outPid, pOutCmd->tpInfoIdx);
    }
#endif

    // Update descriptor base pointer
    outCmdBase += numOutCmd;
}


#if MPC
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


int main(int argc, char** argv)
{
    int i;
    int qamId, progId;

#if MPC
    time_entry_t *entry1;
    time_entry_t *entry2;
    int cacheFlushed;

    prepare_L1_caches();
    init_time_records();
    enable_L1_caches();
#endif

    init();
    inStopTime = START_TIME;
    schStopTime = START_TIME - SYSTEM_DELAY;

    for (i=0; i<NUM_SCHWIN; i++) {
#if SUN
        printf("\n\n\nPeriod %d:", i);
#if 0
        for (qamId=0; qamId<numQam; qamId++)
            for (progId=0; progId<qamInfo[qamId].numProg; progId++) {
                ProgInfo* pProgInfo = &progInfo[qamId][progId];
                if (!pProgInfo->numPid)  continue;
                printf("\n  QAM %d, Prog %d: rd %d, wr %d",
                    qamId, progId, pProgInfo->rdIdx, pProgInfo->wrIdx);
            }
#endif
        printf("\n  Descriptor: HW in %d, SW in %d, outBase %d",
               inDscWr, inDscRd, outCmdBase);
#endif

        // Update local time
        inStartTime = inStopTime;
        inStopTime = inStartTime + SCHWIN_SIZE;
        schStartTime = schStopTime;
        schStopTime = schStartTime + SCHWIN_SIZE;


        // Simulate Input HW
        inputHwSim();

        // Input Task
#if MPC
        cacheFlushed = cache_flush();
        time_entry_begin();
#endif
        inputTask();
#if MPC
        time_entry_end();
        inCycle += get_time_elapsed(0,1);
#endif

        // Mux Task
#if MPC
        cacheFlushed = cache_flush();
        time_entry_begin();
#endif
        muxTask();
#if MPC
        time_entry_end();
        muxCycle += get_time_elapsed(0,1);
#endif

        // Output Task
#if MPC
//        cacheFlushed = cache_flush();
        time_entry_begin();
#endif
        outputTask();
#if MPC
        time_entry_end();
        outCycle += get_time_elapsed(0,1);
#endif
    }

#if SUN
#if 1
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
#endif

    printf("\n\n\nSummary:");
    printf("\n  Input : %d TPs", numInTp);
    printf("\n  Mux   : %d TPs", numMuxTp);
    printf("\n  Null  : %d TPs", numNullTp);
    printf("\n  Output: %d TPs\n\n", numOutTp);
#endif

#if MPC
    inCycle /= numInTp;
    muxCycle /= numMuxTp;
    outCycle /=  numMuxTp;
    totalCycle = inCycle + muxCycle + outCycle;

    entry1 = get_time(0);
    entry2 = get_time(1);
#endif

    return 1;
}

