/*****************************************************************************
    Palm Beach Packet Processing Benchmark

    This program accepts MPEG/UDP/IP data in the following format:
    Input contains cells of 256 bytes, each containing either a UDP/IP
    header, or an MPEG transport packet.  The UDP/IP header cell is also
    expected to contain an arrival timestamp in the last word.

    This program will produce a set of descriptors which instructs
    Palm Beach hardware to fetch the appropriate MPEG packets for output.
*****************************************************************************/

#include <stdio.h>
#include "def.h"

#define SUN				0	// for SUN simulation
#define	BENCHMARK			1	// for benchmark

#define TP_SIZE				188
#define PKTBUF_SIZE			256

#define MAX_QAM				24
#define MAX_PROG_PER_QAM		16
#define MAX_PROG			(MAX_QAM * MAX_PROG_PER_QAM)
#define MAX_PID_PER_PROG		4
#define MAX_PKTBUF			32768
#define MAX_TPINFO_PER_PROG		1024
#define MAX_SLOT			128
#define MAX_DESCRIPTOR			4096

#define	TPINFO_FLAG_HAS_PCR		0x01

#define PIDTYPE_VIDEO			1
#define PIDTYPE_AUDIO			2
#define PIDTYPE_PMT			3

// Constants for clock recovery
#define	A1	63
#define	A2	1
#define	A3	6
#define	B1	12
#define B2	20
#define	C1	3
#define	C2	1
#define	C3	2


// Transport packet info
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
    Uint32 numTp;		// number of TPs processed for this program
    Uint32 numPcr;		// number of PCRs processed for this program
    Uint32 prevPcr;             // previous PCR
    Uint32 prevPcrTpArvlTime;   // arrival time of previous PCR carrying TP
    Uint32 prevPcrTpIdx;        // index of previous PCR carrying TP
    Uint32 prevPcrTpOutTime;    // delivery time of previous PCR carrying TP
    Int32  tpIntrvl;            // TP interval
    Uint32 prevOutTime;         // delivery time of previous Tp
    Int32  drift;
    Int32  prevFltDrift;
    Int32  slope;		// clock frequency difference (scaled by 2^B1)
} ProgInfo;


// MUX table (per QAM)
typedef struct {
    Uint16 used;		// whether this slot is used or not
    Uint16 offset;		// offset of descriptor used
} MuxTable;


// Descriptor
typedef struct {
    Uint32 *srcAddr;
#if SUN
    Int16 pid;			// for debugging only
#endif
} Descriptor;


typedef struct {
    // IP header
    Uint32 ipHdr[5];

    // UDP header
    Uint16 srcPort;
    Uint16 dstPort;
    Uint16 length;
    Uint16 chksum;

    Uint32 reserved[56];
    Uint32 arvlTime;		// assume HW stamp arrival time here
} IpPkt;


typedef struct {
    Uint32 tpHdr;
    Uint32 tpPyld[46];
} MpegPkt;


// Packet Buffer - Cells of 256 bytes containing
//                 either IP/UDP header or MPEG TP
typedef struct {
    Uint8 data[PKTBUF_SIZE];
} PktBuf;



// Global variables
PktBuf pktBuf[MAX_PKTBUF];	// packet buffer pool
Uint32 pbRdIdx;			// packet buffer read index
Uint32 pbWrIdx;			// packet buffer write index

QamInfo    qamInfo[MAX_QAM];
ProgInfo   progInfo[MAX_QAM][MAX_PROG_PER_QAM];
TpInfo     tpInfo[MAX_QAM][MAX_PROG_PER_QAM][MAX_TPINFO_PER_PROG];

MuxTable   muxTbl[MAX_QAM][MAX_SLOT];
Descriptor descriptor[MAX_DESCRIPTOR];
Uint32     descripBase;		// descriptor base index for current
				// schedule window

Uint32     schStartTime;	// schedule window start time (local 27 MHz)
Uint32     schStopTime;		// schedule window stop time (local 27 MHz)

PktBuf     nullMpegPkt;		// special MPEG null packet for stuffing


// Important operating variables
int numQam = 2;			// actual number of QAMs used
int numProg = MAX_PROG_PER_QAM;	// actual number of programs per QAM used
int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
int schWin = 4;			// schedule window size in msec
int numSlot;			// number of slots in mux table
int numDescrip;			// number of descriptors in schedule window

int tpTime;			// duration between adjacent TP
				// (assuming 3.75 Mbps)

int timeBase = 0;		// time base adjustment to arrival time

FILE *timeRecFp;


#if BENCHMARK

#define	NUM_CHAN			2

// This structure contains all information of a channel for simulation
typedef struct {
    Uint16  udpDestPort;
    Uint16  numPid;		// max 4
    PidInfo pidMap[4];
} ChanInfo;

ChanInfo chanInfo[NUM_CHAN] = {
    { 0xc000, 4,
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
    },

    { 0xc002, 3,
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
      0, 0, 0
    }
};

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
    Int32 arvlDiff, pcrDiff, estDiff, fltDrift, diff;
    Int32 delTime, delTpIdx;

    if (!pProgInfo->numPcr) {
        pProgInfo->prevPcr = pTpInfo->pcr;
        pProgInfo->prevPcrTpArvlTime = arvlTime;
    }

    // Find delivery time of PCR carrying packet using clock recovery algo
    arvlDiff = arvlTime - pProgInfo->prevPcrTpArvlTime;
    pcrDiff = pTpInfo->pcr - pProgInfo->prevPcr;
				// Need to address wrap around problem later!!
    pProgInfo->drift += arvlDiff - pcrDiff;
    fltDrift = (A1 * pProgInfo->prevFltDrift + A2 * pProgInfo->drift) >> A3;
    printf("  Arvl %08x (%+d), PCR %08x (%+d)\n",
        arvlTime, arvlDiff, pTpInfo->pcr, pcrDiff);
    printf("  drift %d (%.2f ms), fltDft %d (%.2f ms)\n",
        pProgInfo->drift, pProgInfo->drift/27000., fltDrift, fltDrift/27000.);

    diff = fltDrift - pProgInfo->prevFltDrift;
    estDiff = (pProgInfo->slope * pcrDiff) >> B1;
    printf("  DriftRate: act %d, est %d, slope %d",
           diff, estDiff, pProgInfo->slope);

    pProgInfo->slope += ((diff - estDiff) << B1) >> B2;
    printf(" -> %d\n", pProgInfo->slope);

    pTpInfo->outTime = pProgInfo->prevOutTime + pcrDiff + estDiff;
    printf("  OutTime %08x\n", pTpInfo->outTime);

    // Adjust TP interval
    delTime = (Int32)(pTpInfo->outTime - pProgInfo->prevPcrTpOutTime);
    delTpIdx = (Int32)(pProgInfo->numTp - pProgInfo->prevPcrTpIdx + 1);
    printf("  TP intvl: %d", pProgInfo->tpIntrvl);
    pProgInfo->tpIntrvl = (C1 * pProgInfo->tpIntrvl + C2 * (delTime/delTpIdx))
                             >> C3;
    printf(" -> %d (%d), delTime %d, delTpIdx %d\n",
           pProgInfo->tpIntrvl, delTime/delTpIdx, delTime, delTpIdx);

    // Update
    pProgInfo->numPcr++;
    pProgInfo->prevPcr = pTpInfo->pcr;
    pProgInfo->prevPcrTpArvlTime = arvlTime;
    pProgInfo->prevPcrTpIdx = pProgInfo->numTp;
    pProgInfo->prevPcrTpOutTime = pTpInfo->outTime;
    pProgInfo->prevFltDrift = fltDrift;
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
    FILE* fp;

    // Open output file for clock recovery
    timeRecFp = fopen("clkRec", "w");

    // Get input data to memory
    fp = fopen("ip.dat", "r");
    if (fp==NULL) {
        printf("Error: Failed to open input file test.dat\n");
        exit (-1);
    }
    pbRdIdx = pbWrIdx = 0;

    memset(qamInfo, 0, sizeof(qamInfo));
    memset(progInfo, 0, sizeof(progInfo));
    memset(tpInfo, 0, sizeof(tpInfo));
    memset(muxTbl, 0, sizeof(muxTbl));
    memset(descriptor, 0, sizeof(descriptor));

    while (1) {
        sz = fread((Uint32*)&pktBuf[pbWrIdx++], 1, PKTBUF_SIZE, fp);
        if (sz < PKTBUF_SIZE || pbWrIdx > MAX_PKTBUF) {
            pbWrIdx--;
            break;
        }
    }

#if SUN
    printf("%d cells read\n", pbWrIdx);
    printf("QAM rate: %d bps\n", qamRate);
    printf("Schedule window size: %d ms\n", schWin);
#endif

    descripBase = 0;

    // Set up mux table
    numSlot = qamRate * schWin / (1000 * TP_SIZE * 8) + 1;
    numDescrip = numQam * numSlot;
    for (i=k=0; i<numSlot; i++)
        for (qamId=0; qamId<numQam; qamId++, k++)
            muxTbl[qamId][i].offset = k;

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
        }
    }

#if SUN
    printf("Number of slots in mux table: %d\n", qamInfo[0].numSlot);
    printf("Slot interval: %08x\n", qamInfo[0].slotIntrvl);
    printf("Mux table:\n");
    for (i=0; i<numSlot; i++)
        printf("  Slot %d: %d\n", i, muxTbl[0][i].offset);
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

#if SUN
        printf("\nUDP %04x, QAM %d, prog %d\n",
            chanInfo[i].udpDestPort, qamId, progId);
        for (k=0; k<pProgInfo->numPid; k++)
            printf("PID %d->%d, type %d\n", pProgInfo->pidMap[k].inPid,
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
    printf("\nInput: %08x -> %08x\n", schStartTime, schStopTime);
#endif

    // Process each packet buffer available
//    while (pbRdIdx < pbWrIdx) {
    while (1) {
        if (pbRdIdx == pbWrIdx) {
            printf("\n** Packet buffer pointer wrap around\n");
            pbRdIdx = 0;
            timeBase = arvlTime;
        }

        pIpPkt = (IpPkt*) &pktBuf[pbRdIdx++];

        arvlTime = pIpPkt->arvlTime + timeBase;
        if (arvlTime > schStopTime) {
            pbRdIdx--;
#if SUN
            printf("  Last PktBuf %d, arvlTime %08x\n", pbRdIdx, arvlTime);
#endif
            return;
        }

        numMpegPkt = pIpPkt->length / TP_SIZE;
        qamId = udpPort2qamId(pIpPkt->dstPort);
        progId = udpPort2progId(pIpPkt->dstPort);

#if SUN
        printf("  IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs\n",
            pbRdIdx-1, arvlTime, pIpPkt->dstPort, qamId, progId, numMpegPkt);
#endif

        // Process each MPEG transport packet in the IP packet
        for (i=0; i<numMpegPkt; i++) {
            pMpegPkt = (MpegPkt*) &pktBuf[pbRdIdx++];
            pProgInfo = &progInfo[qamId][progId];
            pTpInfo = &tpInfo[qamId][progId][pProgInfo->wrIdx++];
            if (pProgInfo->wrIdx >= MAX_TPINFO_PER_PROG)  pProgInfo->wrIdx = 0;
            pTpInfo->addr = (Uint32*)pMpegPkt;
            pProgInfo->numTp++;

            // PID mapping
            pid = (pMpegPkt->tpHdr >> 8) & 0x1fff;
            for (k=0; k<pProgInfo->numPid; k++) {
                if (pid == pProgInfo->pidMap[k].inPid) {
                    pTpInfo->outPid = pProgInfo->pidMap[k].outPid;
                    pTpInfo->pidType = pProgInfo->pidMap[k].pidType;
                    break;
                }
            }

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

                // Clock recovery
if (progId==1) {
                printf("\nTP %d-%d: %08x %08x\n", progId, pProgInfo->numTp,
                       pMpegPkt->tpHdr, pMpegPkt->tpPyld[0]);
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
}
            }
            else {
                // Compute delivery time
                findOutTime(pProgInfo, pTpInfo);
//            pTpInfo->outTime = arvlTime + i * tpTime;	// dummy delivery time

              pTpInfo->pcr = 0;
            }
if (progId==1)
  fprintf(timeRecFp, "%d %d %d\n", arvlTime, pTpInfo->outTime, pTpInfo->pcr);
        }
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
    int descripIdx;
    QamInfo*  pQamInfo;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;

#if SUN
    printf("\nMux:");
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

                pTpInfo = &tpInfo[qamId][progId][pProgInfo->rdIdx++];
                if (pProgInfo->rdIdx > MAX_TPINFO_PER_PROG)
                    pProgInfo->rdIdx = 0;

                // Assign a slot for next TP
                slotIdx = ((Int32)(pTpInfo->outTime - schStartTime))
                              / pQamInfo->slotIntrvl;
#if SUN
                printf("\n  TP (%0d): PID %d, type %d, outTime %08x, slot %d",
                    ((PktBuf*)pTpInfo->addr) - pktBuf, pTpInfo->outPid,
                    pTpInfo->pidType, pTpInfo->outTime, slotIdx);
#endif

                if (slotIdx >= pQamInfo->numSlot) {
#if SUN
                    printf("\nDone with program");
#endif
                    break;		// done with this program
                }

                if (slotIdx < 0) {
		    slotIdx = 0;	// should be in past schedule window!
#if SUN
                    printf(" -> %d", slotIdx);
#endif
                }

                // find an empty slot closest to slotIdx
                if (muxTbl[qamId][slotIdx].used) {
                    slotIdx = searchEmptySlot(qamId, slotIdx);
#if SUN
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

                // Special handling for PMT, etc.
                if (pTpInfo->pidType == PIDTYPE_PMT)
                    procPmtTp(pTpInfo);

                // Set up descriptor for this MPEG packet
                descripIdx = (descripBase + muxTbl[qamId][slotIdx].offset)
                                 % MAX_DESCRIPTOR;
                descriptor[descripIdx].srcAddr = pTpInfo->addr;
#if SUN
                descriptor[descripIdx].pid = pTpInfo->outPid;
#endif
            }
        }

        // Scan and fill null packets
        for (i=0; i<pQamInfo->numSlot; i++)
            if (!muxTbl[qamId][i].used) {
                descripIdx = (descripBase + muxTbl[qamId][i].offset)
                                 % MAX_DESCRIPTOR;
                descriptor[descripIdx].srcAddr = (Uint32*)&nullMpegPkt;
#if SUN
                descriptor[descripIdx].pid = -1;
#endif
            }
    }

#if SUN
    // Dump descriptor info
    printf("\n\nDescriptor:\n");
    for (i=0; i<numDescrip; i++) {
        descripIdx = (descripBase + i) % MAX_DESCRIPTOR;
        if (descriptor[descripIdx].pid > 0)
            printf("  %d (%d): %d\n", descripIdx, i,
                descriptor[descripIdx].pid);
    }
    printf("\n");
#endif

    descripBase += numDescrip;
}


int main(int argc, char** argv)
{
    int i;

    init();
    schStopTime = 0;

    for (i=0; i<400; i++) {
        // Update local time
        schStartTime = schStopTime;
        schStopTime = schStartTime + 27000 * 4;	// 4 ms (in 27 MHz clock ticks)

        inputTask();
        muxTask();
    }
}

