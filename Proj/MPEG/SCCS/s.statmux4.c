h50592
s 00017/00006/00571
d D 1.4 02/07/12 16:31:32 ytse 4 3
c Align with statmux.c
e
s 00008/00007/00569
d D 1.3 02/07/12 16:08:06 ytse 3 2
c Backup
e
s 00006/00003/00570
d D 1.2 02/07/10 16:41:33 ytse 2 1
c Update
e
s 00573/00000/00000
d D 1.1 02/07/01 12:35:42 ytse 1 0
c date and time created 02/07/01 12:35:42 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: statmux4.c

    Statmux simulation (video only)
    New allocation algorithm

    Note:
      To handle partial picture at the turn of a schedule period, we store
      the number of bytes remaining in the last picture in ProgInfo::leftBytes.
      This value will always belong to the picture pointer to by outPn.

      The picture pointed to by outPn need special attention when the
      program's leftBytes>0, because this picture's outQs is determined
      in the previous schedule window.

      If we do not buffered enough picture before we do scheduling, we may
      send pictures that is not even arrived yet!  We temporarily solve this
      problem by increasing the distance between inPn and outPn.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"


#define SYS_CLK			27000000
#define TP_BYTES		188
#define TP_BITS			(TP_BYTES << 3)
#define	VBV_SZ			1835008

#define MAX_PICS_PER_PROG	1024
#define MAX_LINE_WIDTH		128



// Picture statistics
//
typedef struct {
  int   pid;			// PID
  int   picNo;			// picture number (decoder order) within PID
  int   origBytes;              // original picture size (in bytes)
  int   picType;                // picture type
  uint  dts;                    // DTS (mapped to local clock) (in 45 kHz)
  uint  arvlTime;               // arrival time
  float avgQs;                  // average quant scale
  float outQs;                  // output average quant scale
  int   tgtBytes;		// target size after reduction (in bytes)
  uint  dptTime;		// departure time
  int   accuPicSz;		// accumulated picture size (for MSR)
  int   deltaTime;		// delta time (for MSR)
  float msr;			// minimum send rate
  int   used;			// used flag
}
PicStat;


typedef struct {
  PicStat* stat;
  uint pid;
  int  inPn;			// picutre number of next input picture
  int  outPn;			// picture number of next output picture
  int  decPn;			// picture number of next decode picture
  int  msrPn;			// picture number which determines msr
  int  lahPn;			// lookahead picture number

  float rate;			// output rate
  int  schTp;			// number of TP scheduled for current period
  int  allocTp;			// number of TP actually allocated
				// for current period
  int  vbvLevel;		// VBV level (in bits)

  int  availBytes;		// available bytes to send for current
				// schedule period
  int  leftBytes;		// left size of the last picture
				// in previous schedule window
  uint msrDts;			// DTS of msr picture
  float msr;

  // Duplicated variables used in scheduling
  int  schOutPn;
  int  schDecPn;
  int  schVbvLevel;
  int  schAvailBytes;
  int  schLeftBytes;
}
ProgInfo;


// Global variables
int  progCnt;
ProgInfo* prog;
int  outBitrate;
FILE *inFp, *outFp;
int  numStat;
char line[MAX_LINE_WIDTH+1];

int  tpTime;			// TP time in 27 MHz

uint curTime;			// current time in 45 kHz
uint curTimeExt;		// current time extension in 27 MHz
uint schTime;			// end time in 45 kHz
uint lahTime;			// lookahead window end time in 45 kHz

uint schWinSz;			// schedule window size in 27 MHz
uint prefetchSz;		// prefetch window size in 27 MHz
uint prefetchTime;		// prefetch window end time in 27 MHz
uint lahWinSz;			// lookahead window size in 27 MHz

int  schCurTime;
int  schCurTimeExt;

int  tpPerSchWin;		// number of tables per schedule window
int  tpPerLahWin;		// number of tables per lookahead window
int  totSchTp;
int  outQs;


PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
}


void dumpProgs()
{
    int i;
    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        printf("\nPID %d: msr %.0f, pn %d, DTS %08x, schOut %d, VBV %d",
            pProg->pid, pProg->msr, pProg->msrPn, pProg->msrDts,
            pProg->schOutPn, pProg->vbvLevel);
    }
}


// Find program number based on PID
//   Returns NULL if PID not found
//
ProgInfo* pid2Prog(int pid)
{
    int i;
    for (i=0; i<progCnt; i++)
        if (prog[i].pid == pid)  return &prog[i];
    return NULL;
}


// Find the last picture whose DTS is less than the specified time
//
int dts2Pn(ProgInfo* pProg, uint time)
{
    int pn = pProg->outPn;
    while (1) {
        PicStat* pStat = getPicStat(pProg, pn++);
        if (pStat->dts > time)  return pStat->picNo - 1;
D 3
        if (pn == pProg->inPn)  return pn - 1;
E 3
I 3
        if (pn >= pProg->inPn)  return pn - 1;
E 3
    }
}


// Estimate target picture size
//
int estTargetSize(PicStat* pStat, int outQs)
{
#if 0
    int tgtSz;
    float deltaQs = outQs - pStat->avgQs;
    if (deltaQs<=0)  return pStat->origBytes;
    if (deltaQs>= 16)  return TP_BYTES;
    tgtSz = (int)((16 - deltaQs) * pStat->origBytes / 16);
    if (tgtSz < TP_BYTES)  tgtSz = TP_BYTES;
    return tgtSz;
#else
    return (int)((float)pStat->origBytes * pStat->avgQs / outQs);
#endif
}


// Rate reduce a program
//
void rateReduceProg(ProgInfo* pProg, int outQs)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
D 3
        pStat->tgtBytes = outQs? estTargetSize(pStat, outQs) : pStat->origBytes;
E 3
I 3
        pStat->tgtBytes = outQs > pStat->avgQs?
                              estTargetSize(pStat, outQs) : pStat->origBytes;
E 3
    }
}


// Find minimum send rate and the corresponding picture of a program
//   Returns 0 if there is no pictures in LAH.  Otherwise returns 1.
//
int findMsrPic(ProgInfo* pProg)
{
    int pn = pProg->schOutPn;
    int accuPicSz = pProg->schLeftBytes;
    float msr = 0;
    PicStat* pStat;

    if (accuPicSz > 0) {			// partially sent picture
        pStat = getPicStat(pProg, pn);
        pStat->accuPicSz = accuPicSz;
        pStat->deltaTime = pStat->dts - schCurTime;
        pStat->msr = 45000. * accuPicSz / pStat->deltaTime;
        msr = pStat->msr;
        pProg->msrPn = pStat->picNo;
        pn++;
    }

    while (pn < pProg->lahPn) {
        pStat = getPicStat(pProg, pn);
        accuPicSz += pStat->tgtBytes;
        pStat->accuPicSz = accuPicSz;
        pStat->deltaTime = pStat->dts - schCurTime;
        pStat->msr = 45000. * accuPicSz / pStat->deltaTime;
        if (pStat->msr > msr) {
            pProg->msrPn = pStat->picNo;
            msr = pStat->msr;
        }
        pn++;
    }
    pProg->msr = msr;
    return (msr>0);
}


// Transport packet assignment
//
ProgInfo* selectProg()
{
    int i;
    int idx = -1;
    uint dts = prefetchTime;
    ProgInfo* pProg;

    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
        if (pProg->schOutPn >= pProg->inPn-1) {
            printf("\n  *PID %d: schOutPn %d, inPn %d",
                   pProg->pid, pProg->schOutPn, pProg->inPn);
            continue;		// already sent all input pictures
        }

        if (!findMsrPic(pProg))
            continue;		// already pass LAH window

        if (pProg->schVbvLevel + TP_BITS >= VBV_SZ) {
//            printf("\nPID %d: VBV overflow prevention, VBV %d",
//                pProg->pid, pProg->schVbvLevel);
            continue;
        }

        pProg->msrDts = getPicStat(pProg, pProg->msrPn)->dts;
        if (pProg->msrDts < dts) {
            dts = pProg->msrDts;
            idx = i;
        }
    }

    if (idx==-1) {
        printf("\n\nAll channels empty!!\n");
        printf("\n  QS %d", outQs);
        dumpProgs();
    }

    return idx==-1? NULL : &prog[idx];
}


// Assign a TP to a program
//
void assignTp(ProgInfo* pProg, int echoFlag)
{
    PicStat* pStat;

#if 0
    printf("\n\nTime %08x", schCurTime);
    dumpProgs();
    printf("\nSelect PID %d", pProg->pid);
    system("clear");
#endif
//    printf("\n=> PID %d", pProg->pid);

    pProg->schTp++;
    pProg->schVbvLevel += TP_BITS;
    pProg->schLeftBytes -= TP_BYTES;

    // Check sent picture
    while (pProg->schLeftBytes <= 0) {
        if (pProg->schOutPn == pProg->lahPn) {
            // Last picture reached
            pProg->schVbvLevel += pProg->schLeftBytes;
            pProg->schLeftBytes = 0;
        }

        if (echoFlag) {
            pStat = getPicStat(pProg, pProg->schOutPn);
            pStat->dptTime = schCurTime;
            printf("\n  QS %d: Sent Pic %d-%d, tgtSz %d, schTp %d", outQs,
                pProg->pid, pStat->picNo, pStat->tgtBytes, pProg->schTp);
        }
        pStat = getPicStat(pProg, ++pProg->schOutPn);
        pProg->schLeftBytes += pStat->tgtBytes;

        pStat->outQs = outQs > pStat->avgQs?  outQs : pStat->avgQs;
    }
}


// Check for decoded picture
//     Return 0 if no VBV underflow.  Otherwise return 1
//
int checkDecPic(uint time, int statFlag, int echoFlag)
{
    int i, j;
    int underflow = 0;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        PicStat* pStat = getPicStat(pProg, pProg->schDecPn);

        if (pStat->dts <= time) {
            pProg->schVbvLevel -= pStat->tgtBytes << 3;
            pProg->schDecPn++;

D 2
            if (statFlag)
E 2
I 2
            if (statFlag) {
E 2
                fprintf(outFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x "\
                    "\t%d \t%d", pStat->pid, pStat->picNo, pStat->picType,
                    pStat->tgtBytes, pStat->outQs, pStat->dts,
                    pStat->dptTime, pProg->vbvLevel>>3,
                    (pProg->vbvLevel>>3) + pStat->tgtBytes);
I 2
                pStat->used = 0;
            }
E 2

            if (echoFlag)
                printf("\n  QS %d:  Dec Pic %d-%d, tgtSz %d, schTp %d,"\
                    " vbv %d", outQs, pProg->pid, pStat->picNo,
                    pStat->tgtBytes, pProg->schTp, pProg->schVbvLevel);

            if (pProg->schVbvLevel < 0) {
D 3
                if (echoFlag) {
                    printf("\tVBV underflow!");
                    dumpProgs();
                }
E 3
I 3
                printf("\n*VBV: QS %d, Dec Pic %d-%d, tgtSz %d, schTp %d,"\
                    " vbv %d", outQs, pProg->pid, pStat->picNo,
                    pStat->tgtBytes, pProg->schTp, pProg->schVbvLevel);
                dumpProgs();
E 3
                underflow = 1;
            }
        }
    }
    return underflow;
}


int main(int argc, char** argv)
{
    int i, j;
    Param par;
    char inFile[128], outFile[128];
    ProgInfo* pProg;
    PicStat* pStat;
    float totRate;
    int schIdx = 0;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "statmux4.par");
    getStringParam(&par, "Transport stream statistics filename",
        inFile, "test.stat");
    getIntParam(&par, "Number of video programs", &progCnt, 2);
    prog = malloc(progCnt * sizeof(ProgInfo));
    for (i=0; i<progCnt; i++)
        getIntParam(&par, "Video PID", &prog[i].pid, 0);
    getIntParam(&par, "Output bitrate (bps)", &outBitrate, 27000000);
    getIntParam(&par, "Schedule interval (ms)", &schWinSz, 250);
D 4
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
E 4
    getIntParam(&par, "Lookahead interval (ms)", &lahWinSz, 1000);
I 4
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
E 4
    getStringParam(&par, "Output statistics filename", outFile, "stat.out");
    endParam(&par);

I 4
    // Initialize scheduler
    schWinSz *= 27000;			// convert to 27 MHz
    lahWinSz *= 27000;
    prefetchSz *= 27000;
    curTime = curTimeExt = 0;

E 4
    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input stat file %s\n", inFile);
        exit(-1);
    }
    fgets(line, MAX_LINE_WIDTH, inFp);	// skip header line

    outFp = fopen(outFile, "wb");
    if (outFp == NULL) {
        printf("Error: Failed to open output stat file %s\n", outFile);
        exit(-1);
    }

D 4
    schWinSz *= 27000;			// convert to 27 MHz
    prefetchSz *= 27000;
    lahWinSz *= 27000;
    curTime = curTimeExt = 0;

E 4
    // Adjust schedule window size to contain integer number of tables
    tpTime = (int)((float)SYS_CLK * TP_BITS / outBitrate + 0.5);
    outBitrate = (int)((float)SYS_CLK * TP_BITS / tpTime + 0.5);
I 4
#if 0
E 4
    tpPerSchWin = (int)((float)schWinSz / tpTime + 0.5);
I 4
#else
  {
    // Force schedule window to contain multiple of 64 TPs to make
    // algorithm comparison easy
    int tblTime = tpTime * 64;
    int tblPerSchWin = (int)(schWinSz / tblTime + 0.5);
    tpPerSchWin = tblPerSchWin * 64;
  }
#endif
E 4
    tpPerLahWin = (int)((float)lahWinSz / tpTime + 0.5);
    schWinSz = tpTime * tpPerSchWin;
    lahWinSz = tpTime * tpPerLahWin;

    printf("\nTP time: %d (in 27 MHz)", tpTime);
    printf("\nOutput bitrate: %d bps", outBitrate);
    printf("\nSchedule window: %d ms, %d TPs", schWinSz/27000, tpPerSchWin);
    printf("\nLookahead window: %d ms, %d TPs", lahWinSz/27000, tpPerLahWin);

    // Initial programs
    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
        pProg->stat = malloc(MAX_PICS_PER_PROG * sizeof(PicStat));
        memset(pProg->stat, 0, MAX_PICS_PER_PROG * sizeof(PicStat));
        pProg->inPn = pProg->outPn = pProg->decPn = 0;
        pProg->vbvLevel = pProg->leftBytes = 0;
    }

    while (1) {
        // Update schedule window
        schTime = curTime + (curTimeExt + schWinSz) / 600;
        prefetchTime = curTime + (curTimeExt + prefetchSz) / 600;
        lahTime = curTime + (curTimeExt + lahWinSz) / 600;

        printf("\n\n---------------------------------------------------------------------------");
        printf("\n\nPeriod %d: current %08x, schedule %08x, lookahead %08x",
               schIdx, curTime, schTime, lahTime);

        // Collect picture statistics
        //
        while (1) {
            int pid;

            if (fgets(line, MAX_LINE_WIDTH, inFp) == NULL) {
                printf("\nEnd of stat file\n");
                return;
            }
            sscanf(line, "%d", &pid);

            // Map program from PID
            pProg = pid2Prog(pid);
            if (pProg == NULL)  continue;

            pStat = getPicStat(pProg, pProg->inPn);
            if (pStat->used) {
                printf("\nError: PicStat still being used (%d)!", pStat->used);
                exit (-1);
            }

            sscanf(line, "%d %d %d %d %f %x %x",
                &pStat->pid, &pStat->picNo, &pStat->picType, &pStat->origBytes,
                &pStat->avgQs, &pStat->dts, &pStat->arvlTime);
            pStat->used = 1;

            pProg->inPn++;

            if (pStat->arvlTime > prefetchTime)  break;
			// Note: we assume pictures in the input stat file
			//       are sorted by arrival time
        }

        printf("\n\nStat collection:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->lahPn = dts2Pn(pProg, lahTime);
            printf("\n  PID %d: in %d, lah %d, out %d, dec %d", pProg->pid,
                pProg->inPn, pProg->lahPn, pProg->outPn, pProg->decPn);
        }


        // Rate conversion decision
        //
        for (outQs=0; outQs<=30; outQs+=3) {
            int underflow;
            schCurTime = curTime;
            schCurTimeExt = curTimeExt;

            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];

                // Estimate target size with output QS
                rateReduceProg(pProg, outQs);
//                pProg->availBytes = calcAvailBytes(pProg);
                pProg->schTp = 0;

                // Copy variables for scheduling
                pProg->schOutPn = pProg->outPn;
                pProg->schDecPn = pProg->decPn;
                pProg->schVbvLevel = pProg->vbvLevel;
                pProg->schLeftBytes = curTime?
                    pProg->leftBytes : pProg->stat[0].tgtBytes;
            }


            for (i=0; i<tpPerLahWin; i++) {
                // Advance current time
                schCurTimeExt += tpTime;
                schCurTime += schCurTimeExt / 600;
                schCurTimeExt %= 600;

                pProg = selectProg();
D 3
if (pProg==NULL)  exit(0);
E 3
I 3
                if (pProg==NULL)  exit(0);
E 3
                assignTp(pProg, 0);
                underflow = checkDecPic(schCurTime, 0, 0);
                if (underflow)  break;
            }
            if (!underflow)  break;
        }
        printf("\n\nRC decision: QS %d", outQs);


        // Packet assignment
        //
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->schTp = 0;
            pProg->schOutPn = pProg->outPn;
            pProg->schDecPn = pProg->decPn;
            pProg->schVbvLevel = pProg->vbvLevel;
            pProg->schLeftBytes = curTime?
                pProg->leftBytes: pProg->stat[0].tgtBytes;
        }

        printf("\n\nActual packet assignment:");
        schCurTime = curTime;
        schCurTimeExt = curTimeExt;

        for (i=0; i<tpPerSchWin; i++) {
            // Advance current time
            schCurTimeExt += tpTime;
            schCurTime += schCurTimeExt / 600;
            schCurTimeExt %= 600;

            pProg = selectProg();
D 2
if (pProg==NULL)  exit(0);
E 2
I 2
            if (pProg==NULL)  exit(0);

E 2
            assignTp(pProg, 1);
            checkDecPic(schCurTime, 1, 1);
        }

        curTime = schCurTime;
        curTimeExt = schCurTimeExt;

        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);

        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->vbvLevel = pProg->schVbvLevel;
            pProg->outPn = pProg->schOutPn;
            pProg->decPn = pProg->schDecPn;
            pProg->leftBytes = pProg->schLeftBytes;

            printf("\n  PID %4d: %d TP, VBV %d, PN: in %d, lah %d, "\
D 2
                "send %d:%d, decPn %d", pProg->pid, pProg->schTp,
E 2
I 2
                "send %d:%d, dec %d", pProg->pid, pProg->schTp,
E 2
                pProg->vbvLevel, pProg->inPn-1, pProg->lahPn, pProg->outPn,
                pProg->leftBytes, pProg->decPn);
            printf("\n            DTS: out %08x, dec %08x",
                getPicStat(pProg, pProg->outPn)->dts,
                getPicStat(pProg, pProg->decPn)->dts);

            if (pProg->outPn >= pProg->inPn) {
                printf("\nSending picutre not already arrived yet!\n");
                exit (-1);
            }
            if (pProg->leftBytes < 0) {
                printf("\nLeftByte negative!\n");
                exit (-1);
            }
        }
    }

    printf("\n");
}

E 1
