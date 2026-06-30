h25757
s 00004/00003/00818
d D 1.7 02/12/09 15:46:09 ytse 7 6
c Reduce PktBuf size so we can fit more data into RAM
e
s 00050/00258/00771
d D 1.6 02/12/09 01:00:35 ytse 6 5
c Code to Shan for benchmark
e
s 00001/00000/01028
d D 1.5 02/12/05 17:28:09 ytse 5 4
c Update
e
s 00334/00085/00694
d D 1.4 02/12/03 11:57:30 ytse 4 3
c Added bitrate estimate, output command setup, Shan's benchmark code, and 30 programs in 2 QAM
e
s 00097/00044/00682
d D 1.3 02/12/03 11:53:05 ytse 3 2
c Use compact data format
e
s 00106/00004/00620
d D 1.2 02/12/03 11:50:47 ytse 2 1
c Update pb_dyn.c
e
s 00624/00000/00000
d D 1.1 02/11/18 14:47:24 ytse 1 0
c date and time created 02/11/18 14:47:24 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    Palm Beach Packet Processing Benchmark

D 4
    This program accepts MPEG/UDP/IP data in the following format:
    Input contains cells of 256 bytes, each containing either a UDP/IP
    header, or an MPEG transport packet.  The UDP/IP header cell is also
    expected to contain an arrival timestamp in the last word.

    This program will produce a set of descriptors which instructs
E 4
I 4
    This program accepts MPEG/UDP/IP data in the format as described
    in ipEncap2.c.  It will produce a set of descriptors which instructs
E 4
    Palm Beach hardware to fetch the appropriate MPEG packets for output.

    This scheduling algorithm used in the code is based on the MLC
    dynamic table method.
I 3

D 4
    Introduce latency between input window and mux schedule window.
E 4
I 4
    Changes:
      Use compact data format
      Introduce latency between input window and mux schedule window.
      Actual descriptor definition
      Integrate Shan's code in

    To do:
      Clock recovery
      Delivery time computation
      Descriptor/output command flush
I 5
D 6
      Input FPGA simulation
E 6
E 5

E 4
E 3
*****************************************************************************/

I 4
#define SUN				1	// for SUN simulation
#define MPC				0	// for PPC
#define IP_DATA				0


E 4
#include <stdio.h>

I 4
#if MPC
#include <stdlib.h>
#include <altivec.h>
#include "test_bench.h"
#include "cache_utils_7410.h"
#endif


E 4
// Basic type definitions
typedef char		Int8;
typedef short		Int16;
typedef int		Int32;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned int	Uint32;


D 4
#define SUN				1	// for SUN simulation
E 4
#define	BENCHMARK			1	// for benchmark

#define TP_SIZE				188
D 7
#define PKTBUF_SIZE			256
E 7
I 7
//#define PKTBUF_SIZE			256
#define PKTBUF_SIZE			64
E 7

#define MAX_QAM				24
#define MAX_PROG_PER_QAM		16
#define MAX_PROG			(MAX_QAM * MAX_PROG_PER_QAM)
#define MAX_PID_PER_PROG		4
D 6
#define MAX_PKTBUF			32768
E 6
I 6
#define MAX_PKTBUF			(16000 * 8)
E 6
#define MAX_TPINFO_PER_PROG		1024
D 4
#define MAX_SLOT			128
E 4
I 4
#define MAX_SLOT			140
E 4
#define MAX_DESCRIPTOR			4096
D 6

E 6
I 6
 
E 6
#define SCHWIN_SIZE			(4 * 27000)
					// schedule window size in 27 MHz ticks

I 3
#define SYSTEM_DELAY			(100 * 27000)
					// system delay in 27 MHz ticks

I 4
#define DEFAULT_CBR			3750000
					// default CBR rate of a program
#define	DEFAULT_TP_INTRVL		(27000000 / DEFAULT_CBR * 1504)
					// default TP delivery increment

E 4
E 3
#define	TPINFO_FLAG_HAS_PCR		0x01

#define PIDTYPE_VIDEO			1
#define PIDTYPE_AUDIO			2
#define PIDTYPE_PMT			3

I 4
// Dummy output command values
#define	CTRL_FLAG			0x012345
#define	TBOFFSET_BASE1			0x234567
#define	TBOFFSET_BASE2			0x456789
#define	TBOFFSET_EXT			0x6789ab
#define	SCRAMBLE_CTRLWORD		0x89abcd
E 4

I 4

E 4
D 3
// Transport packet info
E 3
I 3
// Transport packet info (16 bytes)
E 3
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
    Uint32  numTp;		// number of TPs processed for this program
D 4
    Uint32  prevOutTime;	// delivery time of previous Tp
E 4
I 4
    Uint32  prevPcrTpNum;       // TP number of previous PCR carrying TP
    Uint32  prevPcr;		// previous PCR value
E 4
    Int32   tpIntrvl;		// TP interval
I 4
    Uint32  prevOutTime;	// delivery time of previous Tp
E 4
} ProgInfo;


// MUX table (per QAM)
typedef struct {
    Uint16 used;		// whether this slot is used or not
    Uint16 offset;		// offset of descriptor used
} MuxTable;


D 3
// Descriptor
E 3
I 3
// Descriptor (16 bytes)
E 3
typedef struct {
I 4
    Uint32 cmd;

//    Uint32 byteCnt;
    Uint16 tpInfoIdx;		// for degging only
    Uint16 pid;			// for debugging only

E 4
    Uint32 *srcAddr;
D 3
#if SUN
    Int16 pid;			// for debugging only
#endif
E 3
I 3
D 4
    Uint32 pid;			// for debugging only
    Uint32 tpInfoIdx;		// for debugging only
    Uint32 reserved;
E 4
I 4
    void* next;
E 4
E 3
} Descriptor;


typedef struct {
    // IP header
    Uint32 ipHdr[5];

    // UDP header
    Uint16 srcPort;
    Uint16 dstPort;
    Uint16 length;
    Uint16 chksum;

D 7
    Uint32 reserved[56];
E 7
I 7
    Uint32 reserved[8];
E 7
D 3
    Uint32 arvlTime;		// assume HW stamp arrival time here
E 3
I 3
    Uint32 arvlTime;            // assume HW stamp arrival time here
E 3
} IpPkt;


typedef struct {
    Uint32 tpHdr;
    Uint32 tpPyld[46];
I 4

    // The following fields will be used by HW for output command
    Uint32 ctrlFlag;
    Uint32 outPid;
    Uint32 scrambleCtrlWord;
    Uint32 tbOffsetBase1;
    Uint32 tbOffsetBase2;
    Uint32 tbOffsetExt;
E 4
} MpegPkt;


// Packet Buffer - Cells of 256 bytes containing
//                 either IP/UDP header or MPEG TP
typedef struct {
    Uint8 data[PKTBUF_SIZE];
} PktBuf;



// Global variables
D 4
PktBuf pktBuf[MAX_PKTBUF];	// packet buffer pool
Uint32 pbRdIdx;			// packet buffer read index
Uint32 pbWrIdx;			// packet buffer write index
E 4
I 4
PktBuf     pktBuf[MAX_PKTBUF];	// packet buffer pool
Uint32     pbRdIdx;		// packet buffer read index
Uint32     pbWrIdx;		// packet buffer write index
E 4

QamInfo    qamInfo[MAX_QAM];
ProgInfo   progInfo[MAX_QAM][MAX_PROG_PER_QAM];
TpInfo     tpInfo[MAX_QAM][MAX_PROG_PER_QAM][MAX_TPINFO_PER_PROG];

MuxTable   muxTbl[MAX_QAM][MAX_SLOT];
Descriptor descriptor[MAX_DESCRIPTOR];
Uint32     descripBase;		// descriptor base index for current
				// schedule window

I 3
Uint32     inStartTime;		// input window start time (local 27 MHz)
Uint32     inStopTime;		// input window stop time (local 27 MHz)
E 3
Uint32     schStartTime;	// schedule window start time (local 27 MHz)
Uint32     schStopTime;		// schedule window stop time (local 27 MHz)

PktBuf     nullMpegPkt;		// special MPEG null packet for stuffing


// Important operating variables
D 6
int numQam = 2;			// actual number of QAMs used
int numProg = MAX_PROG_PER_QAM;	// actual number of programs per QAM used
D 3
int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
E 3
I 3
D 4
int qamRate = 52000000;		// QAM bitrate (assume 6 MHz 256 QAM)
E 3
int schWin = 4;			// schedule window size in msec
E 4
I 4

//int qamRate = 26970350;		// QAM bitrate (assume 6 MHz 64 QAM)
//int qamRate = 38810700;		// QAM bitrate (assume 6 MHz 256 QAM)
//int qamRate = 38500000;		// QAM bitrate (assume 8 MHz 64 QAM)
int qamRate = 51300000;		// QAM bitrate (assume 8 MHz 256 QAM)

E 6
int schWin = SCHWIN_SIZE / 27000;  // schedule window size in msec
E 4
int numSlot;			// number of slots in mux table
int numDescrip;			// number of descriptors in schedule window

int tpTime;			// duration between adjacent TP
				// (assuming 3.75 Mbps)


#if BENCHMARK

D 2
#define	NUM_CHAN		2
E 2
I 2
D 4
#define	NUM_CHAN		15
E 4
I 4
D 6
#define	NUM_CHAN		30
E 4
I 3
				// total number of channels
E 6
I 6
#include "config10x15.h"
E 6
E 3
E 2

I 3
D 4
#define	DATA_SIZE		932608
E 4
I 4
D 6
#define	DATA_SIZE		1024000
E 4
				// data size in bytes

E 3
#define NUM_SCHWIN		300
				// number of schedule windows to benchmark

#define START_TIME		0
				// local clock start time

// This structure contains all information of a channel for simulation
typedef struct {
    Uint16  udpDestPort;
    Uint16  numPid;		// max 4
    PidInfo pidMap[4];
} ChanInfo;

I 3

E 3
ChanInfo chanInfo[NUM_CHAN] = {
    { 0xc000, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
E 4
    },

    { 0xc002, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
E 4
      0, 0, 0
I 2
    },

    { 0xc004, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
E 4
    },

    { 0xc006, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc008, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
E 4
    },

    { 0xc00a, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc00c, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
E 4
    },

    { 0xc00e, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc010, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
E 4
    },

    { 0xc012, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc014, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
E 4
    },

    { 0xc016, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc018, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
E 4
    },

    { 0xc01a, 3,
D 4
      400, 128, PIDTYPE_VIDEO,
      401, 129, PIDTYPE_AUDIO,
      4128, 130, PIDTYPE_PMT,
E 4
I 4
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
E 4
      0, 0, 0
    },

    { 0xc01c, 4,
D 4
      64, 64, PIDTYPE_VIDEO,
      65, 65, PIDTYPE_AUDIO,
      66, 66, PIDTYPE_AUDIO,
      89, 67, PIDTYPE_PMT
E 4
I 4
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
    },


    { 0xc040, 4,
      64, 100, PIDTYPE_VIDEO,
      65, 101, PIDTYPE_AUDIO,
      66, 102, PIDTYPE_AUDIO,
      89, 103, PIDTYPE_PMT
    },

    { 0xc042, 3,
      400, 110, PIDTYPE_VIDEO,
      401, 111, PIDTYPE_AUDIO,
      4128, 112, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc044, 4,
      64, 120, PIDTYPE_VIDEO,
      65, 121, PIDTYPE_AUDIO,
      66, 122, PIDTYPE_AUDIO,
      89, 123, PIDTYPE_PMT
    },

    { 0xc046, 3,
      400, 130, PIDTYPE_VIDEO,
      401, 131, PIDTYPE_AUDIO,
      4128, 132, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc048, 4,
      64, 140, PIDTYPE_VIDEO,
      65, 141, PIDTYPE_AUDIO,
      66, 142, PIDTYPE_AUDIO,
      89, 143, PIDTYPE_PMT
    },

    { 0xc04a, 3,
      400, 150, PIDTYPE_VIDEO,
      401, 151, PIDTYPE_AUDIO,
      4128, 152, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc04c, 4,
      64, 160, PIDTYPE_VIDEO,
      65, 161, PIDTYPE_AUDIO,
      66, 162, PIDTYPE_AUDIO,
      89, 163, PIDTYPE_PMT
    },

    { 0xc04e, 3,
      400, 170, PIDTYPE_VIDEO,
      401, 171, PIDTYPE_AUDIO,
      4128, 172, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc050, 4,
      64, 180, PIDTYPE_VIDEO,
      65, 181, PIDTYPE_AUDIO,
      66, 182, PIDTYPE_AUDIO,
      89, 183, PIDTYPE_PMT
    },

    { 0xc052, 3,
      400, 190, PIDTYPE_VIDEO,
      401, 191, PIDTYPE_AUDIO,
      4128, 192, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc054, 4,
      64, 200, PIDTYPE_VIDEO,
      65, 201, PIDTYPE_AUDIO,
      66, 202, PIDTYPE_AUDIO,
      89, 203, PIDTYPE_PMT
    },

    { 0xc056, 3,
      400, 210, PIDTYPE_VIDEO,
      401, 211, PIDTYPE_AUDIO,
      4128, 212, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc058, 4,
      64, 220, PIDTYPE_VIDEO,
      65, 221, PIDTYPE_AUDIO,
      66, 222, PIDTYPE_AUDIO,
      89, 223, PIDTYPE_PMT
    },

    { 0xc05a, 3,
      400, 230, PIDTYPE_VIDEO,
      401, 231, PIDTYPE_AUDIO,
      4128, 232, PIDTYPE_PMT,
      0, 0, 0
    },

    { 0xc05c, 4,
      64, 240, PIDTYPE_VIDEO,
      65, 241, PIDTYPE_AUDIO,
      66, 242, PIDTYPE_AUDIO,
      89, 243, PIDTYPE_PMT
E 4
E 2
    }
};

I 3

E 6
E 3
int timeBase = 0;		// time base adjustment to arrival time
D 2
int numInputTp = 0;		// number of TPs processed in input task
E 2
I 2
int numInTp = 0;		// number of TPs processed in input task
E 2
int numMuxTp = 0;		// number of TPs processed in mux task
I 2
int numOutTp = 0;		// number of output TPs
I 4
int numNullTp = 0;		// number of null TP inserted
E 4
E 2

D 3
#endif
E 3
I 3
D 6
Uint8 rawData[DATA_SIZE];
E 6
I 6
#endif /* BENCHMARK */
E 6
E 3

I 3
D 6
#endif  // BENCHMARK
E 6
E 3

I 6
#if IP_DATA /* [ */ 
extern Uint32 rawData[DATA_SIZE_WORD];
#else 
Uint32 rawData[DATA_SIZE_WORD];
#endif /* ] */
E 6
I 3

I 6

E 6
I 4
#if MPC
static int inCycle = 0;
static int muxCycle = 0;
static int outCycle = 0;
I 6
static int totalCycle = 0;
E 6
#endif


E 4
E 3
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
I 3
    Uint32* rawPtr;
    Uint32 ipHdrSz, numTpPerIp, tpSz;
I 6
#if !IP_DATA
E 6
E 3
    FILE* fp;
I 6
#endif
E 6

I 4
D 6
#if !IP_DATA
E 6
I 6
#if !IP_DATA /* [ */
E 6
E 4
    // Get input data to memory
    fp = fopen("ip.dat", "r");
    if (fp==NULL) {
D 3
        printf("Error: Failed to open input file test.dat\n");
E 3
I 3
        printf("\nError: Failed to open input file ip.dat\n");
E 3
        exit (-1);
    }
I 3
    sz = fread(&rawData, 1, DATA_SIZE, fp);
    if (sz != DATA_SIZE) {
        printf("\nError: Failed to read data.  Only %d bytes read.\n", sz);
        exit (-1);
    }
I 4
D 6
#endif
E 6
I 6
#endif /* ] */
E 6
E 4
E 3

D 3
    pbRdIdx = pbWrIdx = 0;
    while (1) {
        sz = fread((Uint32*)&pktBuf[pbWrIdx++], 1, PKTBUF_SIZE, fp);
        if (sz < PKTBUF_SIZE || pbWrIdx > MAX_PKTBUF) {
            pbWrIdx--;
            break;
E 3
I 3
    pbRdIdx = 0;
    pbWrIdx = 0;
    sz = 0;
    rawPtr = (Uint32*)rawData;

    while (sz < DATA_SIZE) {
        sz += rawPtr[0];		// IP packet size
        ipHdrSz = rawPtr[1];		// IP header size
        numTpPerIp = rawPtr[2];		// number of TP in IP
        tpSz = rawPtr[3];		// TP size in data

        // Copy IP header
        memcpy(&pktBuf[pbWrIdx], rawPtr, ipHdrSz);
        ((IpPkt*)&pktBuf[pbWrIdx++])->arvlTime = rawPtr[4];
        rawPtr += ipHdrSz >> 2;

        // Copy all TPs
        for (i=0; i<numTpPerIp; i++) {
            memcpy(&pktBuf[pbWrIdx++], rawPtr, tpSz);
            rawPtr += tpSz >> 2;
E 3
        }
I 4

        if (pbWrIdx >= MAX_PKTBUF) {
#if SUN
            printf("\nError: run out of packet buffer!\n");
#endif
            exit(-1);
        }
E 4
    }

#if SUN
D 3
    printf("%d cells read\n", pbWrIdx);
    printf("QAM rate: %d bps\n", qamRate);
    printf("Schedule window size: %d ms\n", schWin);
E 3
I 3
    printf("\nRaw data size: %d bytes", DATA_SIZE);
    printf("\n%d cells read", pbWrIdx);
    printf("\nQAM rate: %d bps", qamRate);
    printf("\nSchedule window size: %d ms", schWin);
E 3
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
D 4
        for (qamId=0; qamId<numQam; qamId++, k++)
            muxTbl[qamId][i].offset = k;
E 4
I 4
        for (qamId=0; qamId<numQam; qamId++)
            muxTbl[qamId][i].offset = k++;
E 4

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
D 4
            pProgInfo->tpIntrvl = qamInfo[qamId].slotIntrvl * 3;
					// dummy value, for benchmark only
E 4
I 4
            pProgInfo->tpIntrvl = DEFAULT_TP_INTRVL;
E 4
        }
    }

#if SUN
D 3
    printf("Number of slots in mux table: %d\n", qamInfo[0].numSlot);
    printf("Slot interval: %08x\n", qamInfo[0].slotIntrvl);
    printf("Mux table:\n");
E 3
I 3
    printf("\nNumber of slots in mux table: %d", qamInfo[0].numSlot);
    printf("\nSlot interval: %08x", qamInfo[0].slotIntrvl);
#if 0
    printf("\nMux table:");
E 3
    for (i=0; i<numSlot; i++)
D 3
        printf("  Slot %d: %d\n", i, muxTbl[0][i].offset);
E 3
I 3
        printf("\n  Slot %d: %d", i, muxTbl[0][i].offset);
E 3
#endif
I 3
#endif
E 3


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

D 6
#if SUN
E 6
I 6
#if 0
E 6
D 3
        printf("\nUDP %04x, QAM %d, prog %d\n",
E 3
I 3
        printf("\n\nUDP %04x, QAM %d, prog %d",
E 3
            chanInfo[i].udpDestPort, qamId, progId);
        for (k=0; k<pProgInfo->numPid; k++)
D 3
            printf("PID %d->%d, type %d\n", pProgInfo->pidMap[k].inPid,
E 3
I 3
            printf("\nPID %d->%d, type %d", pProgInfo->pidMap[k].inPid,
E 3
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
D 3
    printf("\nInput: %08x -> %08x\n", schStartTime, schStopTime);
E 3
I 3
    printf("\n\nInput: %08x -> %08x", inStartTime, inStopTime);
E 3
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
D 3
        if (arvlTime > schStopTime) {
E 3
I 3
        if (arvlTime > inStopTime) {
E 3
            pbRdIdx--;
D 6
#if SUN
E 6
I 6
#if 0
E 6
D 3
            printf("  Last PktBuf %d, arvlTime %08x\n", pbRdIdx, arvlTime);
E 3
I 3
            printf("\n  Last PktBuf %d, arvlTime %08x", pbRdIdx, arvlTime);
E 3
#endif
            return;
        }

        numMpegPkt = pIpPkt->length / TP_SIZE;
        qamId = udpPort2qamId(pIpPkt->dstPort);
        progId = udpPort2progId(pIpPkt->dstPort);
        pProgInfo = &progInfo[qamId][progId];

D 6
#if SUN
E 6
I 6
#if 0
E 6
D 3
        printf("  IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs\n",
E 3
I 3
        printf("\n  IP %d: arvl %08x, Port %04x => QAM %d, Prog %d, %d TPs",
E 3
            pbRdIdx-1, arvlTime, pIpPkt->dstPort, qamId, progId, numMpegPkt);
#endif

        // Process each MPEG transport packet in the IP packet
        for (i=0; i<numMpegPkt; i++) {
            pMpegPkt = (MpegPkt*) &pktBuf[pbRdIdx++];
            pTpInfo = &tpInfo[qamId][progId][pProgInfo->wrIdx++];
            if (pProgInfo->wrIdx >= MAX_TPINFO_PER_PROG)  pProgInfo->wrIdx = 0;
            pTpInfo->addr = (Uint32*)pMpegPkt;
            pProgInfo->numTp++;
D 2
            numInputTp++;
E 2
I 2
            numInTp++;
E 2

I 4
            // Find delivery time
//            findOutTime(pProgInfo, pTpInfo);
            pTpInfo->outTime = arvlTime + i * tpTime;    // dummy delivery time
            pProgInfo->prevOutTime = pTpInfo->outTime;

E 4
            // PID mapping
            pid = (pMpegPkt->tpHdr >> 8) & 0x1fff;
            for (k=0; k<pProgInfo->numPid; k++) {
                if (pid == pProgInfo->pidMap[k].inPid) {
                    pTpInfo->outPid = pProgInfo->pidMap[k].outPid;
                    pTpInfo->pidType = pProgInfo->pidMap[k].pidType;
                    break;
                }
            }

I 4
            // Dropping all other unknown PIDs (including NULL TPs)
            if (k == pProgInfo->numPid)
                continue;

E 4
            // Parse TP for PCR flags and PCR
            if ((pMpegPkt->tpHdr & 0x20)   // adaptation field presents
                    && ((pMpegPkt->tpPyld[0] >> 24) > 0)  // AF size > 0
                    && (pMpegPkt->tpPyld[0] & 0x100000)) {    // PCR presents
I 4

E 4
                // Read PCR
                pcrBase = (pMpegPkt->tpPyld[0] & 0x3f) << 17;
                pcrBase |= pMpegPkt->tpPyld[1] >> 15;
                pcrExt = pMpegPkt->tpPyld[1] & 0x1ff;
                pTpInfo->pcr = pcrBase * 300 + pcrExt;
                pTpInfo->pcr += timeBase;	// for benchmark only

I 4
                // Rate estimation
                pProgInfo->tpIntrvl = (pTpInfo->pcr - pProgInfo->prevPcr)
                        / (pProgInfo->numTp - pProgInfo->prevPcrTpNum);
                pProgInfo->prevPcrTpNum = pProgInfo->numTp;
                pProgInfo->prevPcr = pTpInfo->pcr;
#if 0
                printf("\nCBR %d, TP intrvl %08x",
                       27000000/pProgInfo->tpIntrvl*1504, pProgInfo->tpIntrvl);
#endif

E 4
                // Clock recovery
                clockRecovery(pProgInfo, pTpInfo, arvlTime);
            }
D 4
            else {
E 4

D 4
                // Compute delivery time
//                pTpInfo->outTime = findOutTime(pTpInfo);
                pTpInfo->outTime = arvlTime + i * tpTime;	// dummy delivery time
            }
E 4
I 4
            // Setting output commands for hardware (dummy code)
            pMpegPkt->ctrlFlag = CTRL_FLAG;
            pMpegPkt->outPid = pTpInfo->outPid;
            pMpegPkt->scrambleCtrlWord = SCRAMBLE_CTRLWORD;
            pMpegPkt->tbOffsetBase1 = TBOFFSET_BASE1;
            pMpegPkt->tbOffsetBase2 = TBOFFSET_BASE2;
            pMpegPkt->tbOffsetExt = TBOFFSET_EXT;

            // NOTE: Need to flush output command to SDRAM!
E 4
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
D 3
    printf("\nMux:");
E 3
I 3
    printf("\n\nMux: %08x -> %08x", schStartTime, schStopTime);
E 3
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

D 3
                pTpInfo = &tpInfo[qamId][progId][pProgInfo->rdIdx++];
                if (pProgInfo->rdIdx > MAX_TPINFO_PER_PROG)
                    pProgInfo->rdIdx = 0;
E 3
I 3
                pTpInfo = &tpInfo[qamId][progId][pProgInfo->rdIdx];
E 3

                // Assign a slot for next TP
                slotIdx = ((Int32)(pTpInfo->outTime - schStartTime))
                              / pQamInfo->slotIntrvl;
D 6
#if SUN
E 6
I 6
#if 0
E 6
                printf("\n  TP (%0d): PID %d, type %d, outTime %08x, slot %d",
                    ((PktBuf*)pTpInfo->addr) - pktBuf, pTpInfo->outPid,
                    pTpInfo->pidType, pTpInfo->outTime, slotIdx);
#endif

                if (slotIdx >= pQamInfo->numSlot) {
D 3
#if SUN
                    printf("\nDone with program");
#endif
E 3
                    break;		// done with this program
                }

                if (slotIdx < 0) {
		    slotIdx = 0;	// should be in past schedule window!
D 6
#if SUN
E 6
I 6
#if 0
E 6
                    printf(" -> %d", slotIdx);
#endif
                }

                // find an empty slot closest to slotIdx
                if (muxTbl[qamId][slotIdx].used) {
                    slotIdx = searchEmptySlot(qamId, slotIdx);
D 6
#if SUN
E 6
I 6
#if 0
E 6
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
I 3
                descriptor[descripIdx].tpInfoIdx = pProgInfo->rdIdx;
E 3
                descriptor[descripIdx].pid = pTpInfo->outPid;
#endif
I 3

                // Update TpInfo read pointer
                pProgInfo->rdIdx++;
                if (pProgInfo->rdIdx > MAX_TPINFO_PER_PROG)
                    pProgInfo->rdIdx = 0;

E 3
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
    int descripIdx;
    QamInfo*  pQamInfo;
    ProgInfo* pProgInfo;
    TpInfo*   pTpInfo;

    for (qamId=0; qamId<numQam; qamId++) {
        pQamInfo = &qamInfo[qamId];

        // Scan and fill null packets
D 2
        for (i=0; i<pQamInfo->numSlot; i++)
E 2
I 2
        for (i=0; i<pQamInfo->numSlot; i++) {
E 2
            if (!muxTbl[qamId][i].used) {
                descripIdx = (descripBase + muxTbl[qamId][i].offset)
                                 % MAX_DESCRIPTOR;
                descriptor[descripIdx].srcAddr = (Uint32*)&nullMpegPkt;
I 4
                numNullTp++;
E 4
D 3
#if SUN
                descriptor[descripIdx].pid = -1;
#endif
E 3
            }
I 2

            numOutTp++;
        }
E 2
    }

I 4
    // NOTE: Need to flush descriptor to SDRAM!

E 4
D 6
#if SUN
E 6
I 6
#if 0
E 6
    // Dump descriptor info
D 3
    printf("\n\nDescriptor:\n");
E 3
I 3
    printf("\n\nOutput:");
E 3
    for (i=0; i<numDescrip; i++) {
        descripIdx = (descripBase + i) % MAX_DESCRIPTOR;
D 3
        if (descriptor[descripIdx].pid > 0)
            printf("  %d (%d): %d\n", descripIdx, i,
                descriptor[descripIdx].pid);
E 3
I 3
        if (descriptor[descripIdx].srcAddr != (Uint32*)&nullMpegPkt)
            printf("\n  %d (%d): TP %d - %d", descripIdx, i,
                descriptor[descripIdx].pid, descriptor[descripIdx].tpInfoIdx);
E 3
    }
D 3
    printf("\n");
E 3
#endif

    // Update descriptor base pointer
    descripBase += numDescrip;
}


I 6
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


E 6
int main(int argc, char** argv)
{
    int i;
I 3
    int qamId, progId;
E 3

I 4
#if MPC
    time_entry_t *entry1;
    time_entry_t *entry2;
    int cacheFlushed;

    prepare_L1_caches();
    init_time_records();
    enable_L1_caches();
#endif

E 4
    init();
D 3
    schStopTime = START_TIME;
E 3
I 3
    inStopTime = START_TIME;
    schStopTime = START_TIME - SYSTEM_DELAY;
E 3

    for (i=0; i<NUM_SCHWIN; i++) {
I 3
#if SUN
        printf("\n\n\nPeriod %d:", i);
I 6
#if 0
E 6
        for (qamId=0; qamId<numQam; qamId++)
            for (progId=0; progId<qamInfo[qamId].numProg; progId++) {
                ProgInfo* pProgInfo = &progInfo[qamId][progId];
                if (!pProgInfo->numPid)  continue;
                printf("\n  QAM %d, Prog %d: rd %d, wr %d",
                    qamId, progId, pProgInfo->rdIdx, pProgInfo->wrIdx);
            }
I 6
#endif
E 6
        printf("\n  Descriptor base: %d", descripBase);
#endif

E 3
        // Update local time
I 3
        inStartTime = inStopTime;
        inStopTime = inStartTime + SCHWIN_SIZE;
E 3
        schStartTime = schStopTime;
        schStopTime = schStartTime + SCHWIN_SIZE;

I 4
        // Input Task
#if MPC
        cacheFlushed = cache_flush();
        time_entry_begin();
#endif
E 4
        inputTask();
I 4
#if MPC
        time_entry_end();
        inCycle += get_time_elapsed(0,1);
#endif

        // Mux Task
#if MPC
        cacheFlushed = cache_flush();
        time_entry_begin();
#endif
E 4
        muxTask();
I 4
#if MPC
        time_entry_end();
        muxCycle += get_time_elapsed(0,1);
#endif

        // Output Task
#if MPC
I 6
D 7
        cacheFlushed = cache_flush();
E 7
I 7
//        cacheFlushed = cache_flush();
E 7
E 6
        time_entry_begin();
#endif
E 4
        outputTask();
I 4
#if MPC
        time_entry_end();
        outCycle += get_time_elapsed(0,1);
#endif
E 4
    }
I 2

#if SUN
    printf("\n\n\nSummary:");
D 4
    printf("\nInput : %d TPs", numInTp);
    printf("\nMux   : %d TPs", numMuxTp);
    printf("\nOutput: %d TPs\n\n", numOutTp);
E 4
I 4
    printf("\n  Input : %d TPs", numInTp);
    printf("\n  Mux   : %d TPs", numMuxTp);
D 6
    printf("\n  Output: %d TPs", numOutTp);
    printf("\n  Null  : %d TPs\n\n", numNullTp);
E 6
I 6
    printf("\n  Null  : %d TPs", numNullTp);
    printf("\n  Output: %d TPs\n\n", numOutTp);
E 6
E 4
#endif
I 4

#if MPC
    inCycle /= numInTp;
    muxCycle /= numMuxTp;
    outCycle /=  numMuxTp;
D 6
    totCycle = inCycle + muxCycle + outCycle;
E 6
I 6
    totalCycle = inCycle + muxCycle + outCycle;
E 6

    entry1 = get_time(0);
    entry2 = get_time(1);
#endif

    return 1;
E 4
E 2
}

E 1
