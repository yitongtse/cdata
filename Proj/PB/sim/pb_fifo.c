/*****************************************************************************
    Palm Beach Packet Processing Benchmark

    This program accepts MPEG/UDP/IP data in the format as described
    in ipEncap2.c.  It will produce a set of descriptors which instructs
    Palm Beach hardware to fetch the appropriate MPEG packets for output.

    This first-in-first-out scheduling algorithm is used here.

    Changes:
      Use compact data format
      Introduce latency between input window and mux schedule window.
      Actual descriptor definition
      Integrate Shan's code in

    To do:
      Clock recovery
      Delivery time computation
      Descriptor/output command flush

*****************************************************************************/

#define SUN				1	// for SUN simulation
#define MPC             		0	// for PPC
#define IP_DATA         		0


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
#define FIFO_SCHEDULE			1

#define TP_SIZE				188
//#define PKTBUF_SIZE			256
#define PKTBUF_SIZE			64

#define MAX_QAM				24
#define MAX_PROG_PER_QAM		16
#define MAX_PROG			(MAX_QAM * MAX_PROG_PER_QAM)
#define MAX_PID_PER_PROG		4
#define MAX_PKTBUF			(16000 * 8)
#define MAX_TPINFO_PER_PROG		1024
#define MAX_SLOT			140
#define MAX_DESCRIPTOR			4096

#define SCHWIN_SIZE			(4 * 27000)
					// schedule window size in 27 MHz ticks

#define SYSTEM_DELAY			(100 * 27000)
					// system delay in 27 MHz ticks

#define DEFAULT_CBR                     3750000
                                        // default CBR rate of a program
#define DEFAULT_TP_INTRVL               (27000000 / DEFAULT_CBR * 1504)
                                        // default TP delivery increment

#define	TPINFO_FLAG_HAS_PCR		0x01

#define PIDTYPE_VIDEO			1
#define PIDTYPE_AUDIO			2
#define PIDTYPE_PMT			3

// Dummy output command values
#define CTRL_FLAG                       0x012345
#define TBOFFSET_BASE1                  0x234567
#define TBOFFSET_BASE2                  0x456789
#define TBOFFSET_EXT                    0x6789ab
#define SCRAMBLE_CTRLWORD               0x89abcd


// Transport packet info (16 bytes)
typedef struct {
    Uint8  flags;
    Uint8  pidType;
    Uint16 outPid;
    Uint32 *addr;
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
    Int32  numProg;		// number of programs in this QAM
    Int32  numSlot;		// number of slot used
    Int32  slotIntrvl;		// slot interval (in 27 MHz ticks)

#if FIFO_SCHEDULE
    Uint32 minOutTime;		// minimal output time of all programs in QAM
    Int32  nextProg;		// program which assiciates to minOutTime
				// (-1 means minOutTime is invalid)
#endif
} QamInfo;


// Program info
typedef struct {
    Uint32  rdIdx;		// read index of corresponding TpInfo
    Uint32  wrIdx;		// write index of corresponding TpInfo
    Uint8   numPid;		// number of active PIDs
    PidInfo pidMap[MAX_PID_PER_PROG];	// PID mapping table

    // Clock recovery related variables
    Uint32  numTp;		// number of TPs processed for this program
    Uint32  prevPcrTpNum;       // TP number of previous PCR carrying TP
    Uint32  prevPcr;            // previous PCR value
    Int32   tpIntrvl;           // TP interval
    Uint32  prevOutTime;        // delivery time of previous Tp

#if FIFO_SCHEDULE
    Uint32  nextOutTime;	// delivery time of next TP
#endif
} ProgInfo;


// MUX table (per QAM)
typedef struct {
    Uint16 used;		// whether this slot is used or not
    Uint16 offset;		// offset of descriptor used
} MuxTable;


// Descriptor (16 bytes)
typedef struct {
    Uint32 cmd;

//    Uint32 byteCnt;
    Uint16 tpInfoIdx;           // for degging only
    Uint16 pid;                 // for debugging only

    Uint32 *srcAddr;
    void* next;
} Descriptor;


typedef struct {
    // IP header
    Uint32 ipHdr[5];

    // UDP header
    Uint16 srcPort;
    Uint16 dstPort;
    Uint16 length;
    Uint16 chksum;

    Uint32 reserved[8];
    Uint32 arvlTime;            // assume HW stamp arrival time here
} IpPkt;


typedef struct {
    Uint32 tpHdr;
    Uint32 tpPyld[46];

    // The following fields will be used by HW for output command
    Uint32 ctrlFlag;
    Uint32 outPid;
    Uint32 scrambleCtrlWord;
    Uint32 tbOffsetBase1;
    Uint32 tbOffsetBase2;
    Uint32 tbOffsetExt;
} MpegPkt;


// Packet Buffer - Cells of 256 bytes containing
//                 either IP/UDP header or MPEG TP
typedef struct {
    Uint8 data[PKTBUF_SIZE];
} PktBuf;



// Global variables
PktBuf     pktBuf[MAX_PKTBUF];	// packet buffer pool
Uint32     pbRdIdx;		// packet buffer read index
Uint32     pbWrIdx;		// packet buffer write index

QamInfo    qamInfo[MAX_QAM];
ProgInfo   progInfo[MAX_QAM][MAX_PROG_PER_QAM];
TpInfo     tpInfo[MAX_QAM][MAX_PROG_PER_QAM][MAX_TPINFO_PER_PROG];

MuxTable   muxTbl[MAX_QAM][MAX_SLOT];
Descriptor descriptor[MAX_DESCRIPTOR];
Uint32     descripBase;		// descriptor base index for current
				// schedule window

Uint32     inStartTime;		// input window start time (local 27 MHz)
Uint32     inStopTime;		// input window stop time (local 27 MHz)
Uint32     schStartTime;	// schedule window start time (local 27 MHz)
Uint32     schStopTime;		// schedule window stop time (local 27 MHz)

PktBuf     nullMpegPkt;		// special MPEG null packet for stuffing


// Important operating variables
int schWin = SCHWIN_SIZE / 27000;  // schedule window size in msec
int numSlot;			// number of slots in mux table
int numDescrip;			// number of descriptors in schedule window

int tpTime;			// duration between adjacent TP
				// (assuming 3.75 Mbps)


#if BENCHMARK

#include "config10x15.h"

int timeBase = 0;		// time base adjustment to arrival time
int numInTp = 0;		// number of TPs processed in input task
int numMuxTp = 0;		// number of TPs processed in mux task
int numOutTp = 0;		// number of output TPs
int numNullTp = 0;		// number of null TP inserted

#endif /* BENCHMARK */


#if IP_DATA /* [ */ 
extern Uint32 rawData[DATA_SIZE_WORD];
#else 
Uint32 rawData[DATA_SIZE_WORD];
#endif /* ] */


#if MPC
static int inCycle = 0;
static int muxCycle = 0;
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

#if !IP_DATA /* [ */
    // Get input data to memory
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
#endif /* ] */

    pbRdIdx = 0;
    pbWrIdx = 0;
    sz = 0;
    rawPtr = (Uint32*)rawData;

    while (sz < DATA_SIZE) {
        sz += rawPtr[0];                // IP packet size
        ipHdrSz = rawPtr[1];            // IP header size
        numTpPerIp = rawPtr[2];         // number of TP in IP
        tpSz = rawPtr[3];               // TP size in data

        // Copy IP header
        memcpy(&pktBuf[pbWrIdx], rawPtr, ipHdrSz);
        ((IpPkt*)&pktBuf[pbWrIdx++])->arvlTime = rawPtr[4];
        rawPtr += ipHdrSz >> 2;

        // Copy all TPs
        for (i=0; i<numTpPerIp; i++) {
            memcpy(&pktBuf[pbWrIdx++], rawPtr, tpSz);
            rawPtr += tpSz >> 2;
        }

        if (pbWrIdx >= MAX_PKTBUF) {
#if SUN
            printf("\nError: run out of packet buffer!\n");
#endif
            exit(-1);
        }
    }

#if SUN
    printf("\nRaw data size: %d bytes", DATA_SIZE);
    printf("\n%d cells read", pbWrIdx);
    printf("\nQAM rate: %d bps", qamRate);
    printf("\nSchedule window size: %d ms", schWin);
#endif

    memset(qamInfo, 0, sizeof(qamInfo));
    memset(progInfo, 0, sizeof(progInfo));
    memset(tpInfo, 0, sizeof(tpInfo));
    memset(muxTbl, 0, sizeof(muxTbl));
    memset(descriptor, 0, sizeof(descriptor));
    descripBase = 0;

    // Set up mux table
    numSlot = qamRate * schWin / (1000 * TP_SIZE * 8) + 1;
    numDescrip = numQam * numSlot;
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


// Special processing for PMT packet
void procPmtTp(TpInfo* pTpInfo)
{
}


// Input module
//
void inputTask(void)
{
    IpPkt*    pIpPkt;
    MpegPkt*  pMpegPkt;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;
    int qamId;
    int progId;
    int pid;
    static int arvlTime;
    int numMpegPkt;
    int i, k;
    Uint32 pcrBase, pcrExt;

#if SUN
    printf("\n\nInput: %08x -> %08x", inStartTime, inStopTime);
#endif

    // Process each packet buffer available
//    while (pbRdIdx < pbWrIdx) {
    while (1) {
        if (pbRdIdx == pbWrIdx) {
#if SUN
            printf("\n** Packet buffer pointer wrap around\n");
#endif
            pbRdIdx = 0;
            timeBase = arvlTime;
        }

        pIpPkt = (IpPkt*) &pktBuf[pbRdIdx++];

        arvlTime = pIpPkt->arvlTime + timeBase;
        if (arvlTime > inStopTime) {
            pbRdIdx--;
#if 0
            printf("\n  Last PktBuf %d, arvlTime %08x", pbRdIdx, arvlTime);
#endif
            return;
        }

        numMpegPkt = pIpPkt->length / TP_SIZE;
        qamId = udpPort2qamId(pIpPkt->dstPort);
        progId = udpPort2progId(pIpPkt->dstPort);
        pProgInfo = &progInfo[qamId][progId];

#if 0
        printf("\n  IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs",
            pbRdIdx-1, arvlTime, pIpPkt->dstPort, qamId, progId, numMpegPkt);
#endif

        // Process each MPEG transport packet in the IP packet
        for (i=0; i<numMpegPkt; i++) {
            pMpegPkt = (MpegPkt*) &pktBuf[pbRdIdx++];
            pTpInfo = &tpInfo[qamId][progId][pProgInfo->wrIdx++];
            if (pProgInfo->wrIdx >= MAX_TPINFO_PER_PROG)  pProgInfo->wrIdx = 0;
            pTpInfo->addr = (Uint32*)pMpegPkt;
            pProgInfo->numTp++;
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

            // Dropping all other unknown PIDs (including NULL TPs)
            if (k == pProgInfo->numPid)
                continue;

            // Parse TP for PCR flags and PCR
            if ((pMpegPkt->tpHdr & 0x20)   // adaptation field presents
                    && ((pMpegPkt->tpPyld[0] >> 24) > 0)  // AF size > 0
                    && (pMpegPkt->tpPyld[0] & 0x100000)) {    // PCR presents

                // Read PCR
                pcrBase = (pMpegPkt->tpPyld[0] & 0x3f) << 17;
                pcrBase |= pMpegPkt->tpPyld[1] >> 15;
                pcrExt = pMpegPkt->tpPyld[1] & 0x1ff;
                pTpInfo->pcr = pcrBase * 300 + pcrExt;
                pTpInfo->pcr += timeBase;	// for benchmark only

                // Rate estimation
                pProgInfo->tpIntrvl = (pTpInfo->pcr - pProgInfo->prevPcr)
                        / (pProgInfo->numTp - pProgInfo->prevPcrTpNum);
                pProgInfo->prevPcrTpNum = pProgInfo->numTp;
                pProgInfo->prevPcr = pTpInfo->pcr;
#if 0
                printf("\nCBR %d, TP intrvl %08x",
                       27000000/pProgInfo->tpIntrvl*1504, pProgInfo->tpIntrvl);
#endif

                // Clock recovery
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
            }

            // Setting output commands for hardware (dummy code)
            pMpegPkt->ctrlFlag = CTRL_FLAG;
            pMpegPkt->outPid = pTpInfo->outPid;
            pMpegPkt->scrambleCtrlWord = SCRAMBLE_CTRLWORD;
            pMpegPkt->tbOffsetBase1 = TBOFFSET_BASE1;
            pMpegPkt->tbOffsetBase2 = TBOFFSET_BASE2;
            pMpegPkt->tbOffsetExt = TBOFFSET_EXT;

            // NOTE: Need to flush output command to SDRAM!

#if 0
            printf("\n  TP %d-%d: %08x", pTpInfo->outPid, pProgInfo->wrIdx,
                pTpInfo->outTime);
#endif
        }
    }

#if SUN
    printf("\nEnd of data reached.\n");
#endif
}


// MUX and output module
//
void muxTask(void)
{
    int qamId;
    int progId;
    int slotId;
    int descripIdx;
    Uint32 outTime;
    TpInfo *pTpInfo;

#if SUN
    printf("\n\nMux: %08x -> %08x", schStartTime, schStopTime);
#endif

    // Process all QAMs
    for (qamId=0; qamId<numQam; qamId++) {
        QamInfo* pQamInfo = &qamInfo[qamId];

        // Prepare all programs
        for (progId=0; progId<pQamInfo->numProg; progId++) {
            ProgInfo* pProgInfo = &progInfo[qamId][progId];
            pQamInfo->nextProg = -1;

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
                    if (pProgInfo->nextOutTime < pQamInfo->minOutTime) {
                        pQamInfo->minOutTime = pProgInfo->nextOutTime;
                        pQamInfo->nextProg = progId;
                    }
                }
            }

            descripIdx = (descripBase + muxTbl[qamId][slotId].offset)
                             % MAX_DESCRIPTOR;

            if (pQamInfo->nextProg == -1 ||
                    (Int32)(pQamInfo->minOutTime - outTime) > 0) {
                // Output a null packet
                descriptor[descripIdx].srcAddr = (Uint32*)&nullMpegPkt;
                numNullTp++;
            }
            else {
                // Output a packet
                ProgInfo* pProgInfo = &progInfo[qamId][pQamInfo->nextProg];
                descriptor[descripIdx].srcAddr
                    = tpInfo[qamId][pQamInfo->nextProg][pProgInfo->rdIdx].addr;
#if 0
                pTpInfo = &tpInfo[qamId][pQamInfo->nextProg][pProgInfo->rdIdx];
                printf("\n %08x: %d (%d): TP %d - %d : %08x", outTime,
                    descripIdx, slotId, pTpInfo->outPid, pProgInfo->rdIdx,
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

                numMuxTp++;
                pQamInfo->nextProg = -1;
            }

            numOutTp++;
        }
    }

    // Update descriptor base pointer
    descripBase += numDescrip;
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
        printf("\n  Descriptor base: %d", descripBase);
#endif

        // Update local time
        inStartTime = inStopTime;
        inStopTime = inStartTime + SCHWIN_SIZE;
        schStartTime = schStopTime;
        schStopTime = schStartTime + SCHWIN_SIZE;

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
    }

#if SUN
    printf("\n\n\nSummary:");
    printf("\n  Input : %d TPs", numInTp);
    printf("\n  Mux   : %d TPs", numMuxTp);
    printf("\n  Null  : %d TPs", numNullTp);
    printf("\n  Output: %d TPs\n\n", numOutTp);
#endif

#if MPC
    inCycle /= numInTp;
    muxCycle /= numMuxTp;
    totalCycle = inCycle + muxCycle;

    entry1 = get_time(0);
    entry2 = get_time(1);
#endif

    return 1;
}

