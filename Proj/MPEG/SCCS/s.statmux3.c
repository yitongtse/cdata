h11423
s 00101/00112/00426
d D 1.4 02/06/27 17:34:27 ytse 4 3
c Backup
e
s 00010/00012/00528
d D 1.3 02/06/27 15:27:49 ytse 3 2
c Update
e
s 00030/00012/00510
d D 1.2 02/06/26 17:52:23 ytse 2 1
c Backup after fixing bug
e
s 00522/00000/00000
d D 1.1 02/06/26 15:28:53 ytse 1 0
c date and time created 02/06/26 15:28:53 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: statmux3.c

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
  int  lookaheadPn;		// lookahead picture number

  float rate;			// output rate
  int  schTp;			// number of TP scheduled for current period
  int  allocTp;			// number of TP actually allocated
				// for current period
  int  vbvLevel;		// VBV level (in bits)

  int  availBytes;		// available bytes to send for current
				// schedule period
  int  leftBytes;		// left size of the last picture
				// in previous schedule window
D 3
  uint decDts;			// next decode DTS
E 3
  uint msrDts;			// DTS of msr picture
I 2
  float msr;
E 2

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

uint curTime;
uint schBeginTime;
uint schEndTime;
uint schWinSz;
uint prefetchSz;
uint prefetchTime;
uint lookaheadWinSz;
uint lookaheadTime;
uint delay;

int schCurTime;

int  tblPerSchWin;		// number of tables per schedule window
int  tblPerLahWin;		// number of tables per lookahead window
int  totSchTp;
int  outQs;
int  tpPerTbl;			// schedule table size in TPs

I 2
int  tblIdx = 0;
int  schTblIdx;
E 2

I 2

E 2
PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
}


// Find program number based on PID
//   Returns NULL if PID not found
//
D 4
ProgInfo* findProg(int pid)
E 4
I 4
ProgInfo* pid2Prog(int pid)
E 4
{
    int i;
    for (i=0; i<progCnt; i++)
        if (prog[i].pid == pid)  return &prog[i];
    return NULL;
}


// Find the last picture whose DTS is less than the specified time
D 4
int findLastDtsPn(ProgInfo* pProg, uint time)
E 4
I 4
//
int dts2Pn(ProgInfo* pProg, uint time)
E 4
{
    int pn = pProg->outPn;
    while (1) {
        PicStat* pStat = getPicStat(pProg, pn++);
        if (pStat->dts > time)  return pStat->picNo - 1;
        if (pn == pProg->inPn)  return pn - 1;
    }
}


D 4
void resetTargetSize(ProgInfo* pProg)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn <= pProg->lookaheadPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = pStat->origBytes;
    }
}


E 4
I 4
// Calculate available input bytes for a program
//
E 4
int calcAvailBytes(ProgInfo* pProg)
{
    int availBytes = 0;
    int pn = pProg->outPn;

    if (pProg->leftBytes > 0) {
        availBytes = pProg->leftBytes;
        pn++;
    }

    while (pn < pProg->inPn)
        availBytes += getPicStat(pProg, pn++)->tgtBytes;

    return availBytes;
}


I 4
// Estimate target picture size
//
E 4
int estTargetSize(PicStat* pStat, int outQs)
{
I 2
    int tgtSz;
E 2
    float deltaQs = outQs - pStat->avgQs;
I 2

E 2
    if (deltaQs<=0)  return pStat->origBytes;
D 2
    if (deltaQs>= 16)  return 0;
    return (int)((16 - deltaQs) * pStat->origBytes / 16);
E 2
I 2
    if (deltaQs>= 16)  return TP_BYTES;
    tgtSz = (int)((16 - deltaQs) * pStat->origBytes / 16);
    if (tgtSz < TP_BYTES)  tgtSz = TP_BYTES;
    return tgtSz;
E 2
}


I 4
// Rate reduce a program
//
E 4
void rateReduceProg(ProgInfo* pProg, int outQs)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = outQs? estTargetSize(pStat, outQs) : pStat->origBytes;
    }
}


D 4
void findMsr(ProgInfo* pProg)
E 4
I 4
// Find minimum send rate of a program
//
void findMsrPic(ProgInfo* pProg)
E 4
{
    int pn = pProg->schOutPn;
D 2
    int accuPicSz = pProg->leftBytes;
E 2
I 2
    int accuPicSz = pProg->schLeftBytes;
E 2
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

    while (pn < pProg->lookaheadPn) {
I 2
        pStat = getPicStat(pProg, pn);
E 2
        accuPicSz += pStat->tgtBytes;
        pStat->accuPicSz = accuPicSz;
        pStat->deltaTime = pStat->dts - schCurTime;
        pStat->msr = 45000. * accuPicSz / pStat->deltaTime;
D 2
        if (pStat->msr < msr) {
E 2
I 2
        if (pStat->msr > msr) {
E 2
            pProg->msrPn = pStat->picNo;
            msr = pStat->msr;
        }
I 2
        pn++;
E 2
    }
I 2
    pProg->msr = msr;
E 2
}


D 4
ProgInfo* selectProg()
E 4
I 4
// Transport packet assignment
//
ProgInfo* assignTp()
E 4
{
D 4
    int i, idx;
E 4
I 4
    int i;
    int idx = -1;
E 4
    uint dts = schBeginTime + prefetchTime;
    ProgInfo* pProg;

    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
D 4
        findMsr(pProg);
E 4
I 4
        if (pProg->schOutPn >= pProg->inPn-1) {
            printf("\n  *PID %d: schOutPn %d, inPn %d",
                   pProg->pid, pProg->schOutPn, pProg->inPn);
            continue;
        }
        findMsrPic(pProg);
E 4
        pProg->msrDts = getPicStat(pProg, pProg->msrPn)->dts;
        if (pProg->msrDts < dts) {
            dts = pProg->msrDts;
            idx = i;
        }
    }
D 4
    return &prog[idx];
E 4
I 4
if (idx==-1)  printf("\n\nALL channels empty!!\n");
    return idx==-1? NULL : &prog[idx];
E 4
}


I 4
// Table assignment
//
void assignTable()
{
    int i;

    for (i=0; i<tpPerTbl; i++) {
        ProgInfo* pProg = assignTp();
printf("\n TP %d: PID %d", i, pProg->pid);
        pProg->schTp++;
        pProg->schVbvLevel += TP_BITS;
        pProg->schLeftBytes -= TP_BYTES;

        // Check sent picture
        while (pProg->schLeftBytes < 0
                   && pProg->schOutPn < pProg->lookaheadPn) {
            PicStat* pStat = getPicStat(pProg, pProg->schOutPn++);
            printf("\n  QS %d: Sent Pic %d-%d, tgtSz %d, schTp %d",
                outQs, pProg->pid, pStat->picNo, pStat->tgtBytes,
                pProg->schTp);
            pProg->schLeftBytes += getPicStat(pProg, pProg->schOutPn)->tgtBytes;
        }
    }
}


// Check for decoded picture
//     Return 0 if no VBV underflow.  Otherwise return 1
//
int checkDecPic(uint time)
{
    int i, j;
    int underflow = 0;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        PicStat* pStat = getPicStat(pProg, pProg->schDecPn);
        if (pStat->dts <= time) {
            pProg->schVbvLevel -= pStat->tgtBytes << 3;
            pProg->schDecPn++;
            printf("\n  QS %d:  Dec Pic %d-%d, tgtSz %d, schTp %d,"\
                " vbv %d", outQs, pProg->pid, pStat->picNo,
                pStat->tgtBytes, pProg->schTp, pProg->schVbvLevel);
            if (pProg->schVbvLevel < 0) {
                printf("\tVBV underflow!");
                underflow = 1;
            }
        }
    }
    return underflow;
}


E 4
int main(int argc, char** argv)
{
    int i, j;
    Param par;
    char inFile[128], outFile[128];
    ProgInfo* pProg;
    PicStat* pStat;
    float totRate;
    int bitPerTbl;		// number of bits per table
    int tblTime;		// table time in 45 kHz
    int leftTp;
    int vbvMargin;
D 2
    int tblIdx = 0;
    int schTblIdx;
E 2
    int schIdx = 0;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "statmux3.par");
    getStringParam(&par, "Transport stream statistics filename",
        inFile, "test.stat");
    getIntParam(&par, "Number of video programs", &progCnt, 2);
    prog = malloc(progCnt * sizeof(ProgInfo));
    for (i=0; i<progCnt; i++)
        getIntParam(&par, "Video PID", &prog[i].pid, 0);
    getIntParam(&par, "Output bitrate (bps)", &outBitrate, 27000000);
    getIntParam(&par, "Schedule interval (ms)", &schWinSz, 250);
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
    getIntParam(&par, "Lookahead interval (ms)", &lookaheadWinSz, 1000);
    getIntParam(&par, "System delay (ms)", &delay, 0);
    getStringParam(&par, "Output statistics filename", outFile, "stat.out");
    endParam(&par);

    // Initial scheduler
    schEndTime = 0;
    schWinSz *= 45;			// convert to 45 kHz
    prefetchSz *= 45;
    lookaheadWinSz *= 45;
    delay *= 45;

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

    // Adjust schedule window size to contain integer number of tables
    tpPerTbl = 64;
    bitPerTbl = tpPerTbl * TP_BITS;
    tblPerSchWin = (int)((double)outBitrate * schWinSz / (45000. * bitPerTbl));
    tblPerLahWin = (int)(tblPerSchWin * lookaheadWinSz / schWinSz);
    tblTime = (int)(45000. * bitPerTbl / outBitrate);
    schWinSz = tblTime * tblPerSchWin;
    outBitrate = (int)(45000. * bitPerTbl / tblTime);

    printf("Output bitrate: %d bps", outBitrate);
    printf("\nSystem delay: %d ms", delay/45);
    printf("\nSchedule window size: %d ms", schWinSz/45);
    printf("\nLookahead window size: %d ms", lookaheadWinSz/45);
    printf("\nTables per schdule window: %d", tblPerSchWin);
    printf("\nTP: %d per table, %d per schedule window",
           tpPerTbl, tpPerTbl*tblPerSchWin);

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
        schBeginTime = schEndTime;
        schEndTime += schWinSz;
        prefetchTime = schBeginTime + prefetchSz;
        lookaheadTime = schBeginTime + lookaheadWinSz;
        curTime = schBeginTime - delay;

        printf("\n\n---------------------------------------------------------------------------");
        printf("\n\nSchedule period %d:", schIdx);
        printf("\n  PCR %08x, begin %08x, end %08x, lookahead %08x",
               curTime, schBeginTime, schEndTime, lookaheadTime);

D 3
        // Collect PicStats for next schedule period
E 3
I 3
        // Collect picture statistics
        //
E 3
        while (1) {
            int pid;

            if (fgets(line, MAX_LINE_WIDTH, inFp) == NULL) {
                printf("\nEnd of stat file\n");
                return;
            }
            sscanf(line, "%d", &pid);

            // Map program from PID
D 4
            pProg = findProg(pid);
E 4
I 4
            pProg = pid2Prog(pid);
E 4
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

D 3
            if (!pProg->inPn)  pProg->decDts = pStat->dts;
E 3
            pProg->inPn++;

            if (pStat->arvlTime > prefetchTime)  break;
			// Note: we assume pictures in the input stat file
			//       are sorted by arrival time
        }

        printf("\n\nStat collection:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
D 2
            printf("\n  PID %d: in %d", pProg->pid, pProg->inPn);
E 2
I 2
D 4
            pProg->lookaheadPn = findLastDtsPn(pProg, lookaheadTime);
E 4
I 4
            pProg->lookaheadPn = dts2Pn(pProg, lookaheadTime);
E 4
            printf("\n  PID %d: in %d, lah %d, out %d",
                pProg->pid, pProg->inPn, pProg->lookaheadPn, pProg->outPn);
E 2
        }

I 3

E 3
        // Rate conversion decision
I 3
        //
I 4
        printf("\n\nRate decision:");

E 4
E 3
        for (outQs=0; outQs<=30; outQs+=3) {
D 4
            int underflow = 0;
E 4
I 4
            int underflow;
E 4
            schTblIdx = tblIdx;
            schCurTime = curTime;

            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];

                // Estimate target size with output QS
                rateReduceProg(pProg, outQs);
                pProg->availBytes = calcAvailBytes(pProg);
                pProg->schTp = 0;
D 3
                pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
E 3

                // Copy variables for scheduling
                pProg->schOutPn = pProg->outPn;
                pProg->schDecPn = pProg->decPn;
                pProg->schVbvLevel = pProg->vbvLevel;
                pProg->schLeftBytes = tblIdx?
                    pProg->leftBytes : pProg->stat[0].tgtBytes;
            }

            // Detail packet assignment
            for (i=0; i<tblPerLahWin; i++, schTblIdx++, schCurTime+=tblTime) {
D 2
//                printf("\nTbl %d: %08x", schTblIdx, schCurTime);
E 2
I 2
D 4
//                printf("\nTime %08x, QS %d", schCurTime, outQs);
E 2

                for (j=0; j<tpPerTbl; j++) {
                    pProg = selectProg();
D 2
//                    printf("\n  PID %d: vbv %d",
//                        pProg->pid, pProg->schVbvLevel);

E 2
                    pProg->schTp++;
                    pProg->schVbvLevel += TP_BITS;
                    pProg->schLeftBytes -= TP_BYTES;

                    if (pProg->schLeftBytes < 0) {
                        pStat = getPicStat(pProg, pProg->schOutPn++);
                        printf("\n  QS %d: Sent Pic %d-%d, tgtSz %d, schTp %d",
                            outQs, pProg->pid, pStat->picNo, pStat->tgtBytes,
                            pProg->schTp);
                        pStat = getPicStat(pProg, pProg->schOutPn);
                        pProg->schLeftBytes += pStat->tgtBytes;
                    }
                }

I 2
#if 0
                for (j=0; j<progCnt; j++) {
                    pProg = &prog[j];
                    printf("\n  PID %d: msr %.0f, Pn %d, dts %08x, schTp %d",
                        pProg->pid, pProg->msr, pProg->msrPn, pProg->msrDts,
                        pProg->schTp);
                }
#endif

E 2
                // Check for decoded pictures
                for (j=0; j<progCnt; j++) {
                    pProg = &prog[j];
D 3
                    if (schCurTime >= pProg->decDts) {
E 3
I 3
                    if (schCurTime >= getPicStat(pProg, pProg->schDecPn)->dts) {
E 3
                        pStat = getPicStat(pProg, pProg->schDecPn++);
                        pProg->schVbvLevel -= pStat->tgtBytes << 3;
D 2
                        printf("\n  QS %d: Dec Pic %d-%d, tgtSz %d, schTp %d, "\
E 2
I 2
D 3
                        printf("\n  QS %d:  Dec Pic %d-%d, tgtSz %d, schTp %d, "\
E 2
                            "vbv %d", outQs, pProg->pid, pStat->picNo,
E 3
I 3
                        printf("\n  QS %d:  Dec Pic %d-%d, tgtSz %d, schTp %d,"\
                            " vbv %d", outQs, pProg->pid, pStat->picNo,
E 3
                            pStat->tgtBytes, pProg->schTp, pProg->schVbvLevel);
I 2
D 3
                        pProg->decDts = getPicStat(pProg, pProg->schDecPn)->dts;
E 3
E 2

                        if (pProg->schVbvLevel < 0) {
                            int k;
                            printf("\tVBV underflow!");
                            underflow = 1;
                            pProg->schVbvLevel += pStat->tgtBytes<<3;

                            for (k=0; k<progCnt; k++)
                                printf("\n* PID %d: vbv %d",
                                    prog[k].pid, prog[k].schVbvLevel);

                            pProg->schVbvLevel -= pStat->tgtBytes<<3;
                            break;
                        }
                    }
                }
E 4
I 4
                printf("\nQS %d, Tbl %d:", outQs, schTblIdx);
                assignTable();
                underflow = checkDecPic(schCurTime);
E 4
                if (underflow)  break;
            }
            if (!underflow)  break;
        }
D 4
        printf("\n  RC decision: QS %d", outQs);
E 4
I 4
        printf("\n\n  RC decision: QS %d", outQs);
E 4


D 3
        // Packet assignment phase
E 3
I 3
        // Packet assignment
        //
E 3
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->schTp = 0;
            pProg->schOutPn = pProg->outPn;
D 4
				// Note: findMinVbvProg() uses schVbvLevel
E 4
I 4
            pProg->schDecPn = pProg->decPn;
E 4
            pProg->schVbvLevel = pProg->vbvLevel;
D 3
            pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
E 3
D 4
            if (!tblIdx)  pProg->leftBytes = pProg->stat[0].tgtBytes;
E 4
I 4
            pProg->schLeftBytes = tblIdx?
                pProg->leftBytes: pProg->stat[0].tgtBytes;
E 4
        }

D 4
        for (i=0; i<tblPerSchWin; i++, tblIdx++, curTime+=tblTime) {
            for (j=0; j<tpPerTbl; j++) {
                pProg = selectProg();
                pProg->schTp++;
                pProg->schVbvLevel += TP_BITS;
                pProg->leftBytes -= TP_BYTES;

                if (pProg->leftBytes < 0) {
                    pStat = getPicStat(pProg, pProg->schOutPn++);
                    pStat->dptTime = curTime;	// Note: not accurate!
                    printf("\n  Sent Pic %d-%d, tgtSz %d, schTp %d",
                        pProg->pid, pStat->picNo, pStat->tgtBytes,
                        pProg->schTp);

                    pStat = getPicStat(pProg, pProg->schOutPn);
                    pProg->leftBytes += pStat->tgtBytes;
                }
            }

            // Check for decoded pictures
            for (j=0; j<progCnt; j++) {
                pProg = &prog[j];
D 3
                if (curTime >= pProg->decDts) {
E 3
I 3
                if (curTime >= getPicStat(pProg, pProg->decPn)->dts) {
E 3
                    pStat = getPicStat(pProg, pProg->decPn++);
                    pProg->schVbvLevel -= pStat->tgtBytes << 3;
                    printf("\n  Dec Pic %d-%d, tgtSz %d, schTp %d, vbv %d",
                        pProg->pid, pStat->picNo, pStat->tgtBytes,
                        pProg->schTp, pProg->schVbvLevel);

                    if (pProg->schVbvLevel < 0)
                        printf("\tVBV underflow!");
                    pStat->used = 0;
D 3
                    pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
E 3
                }
            }
E 4
I 4
        printf("\n\nActual packet assignment:");
        schCurTime = curTime;
        for (i=0; i<tblPerSchWin; i++, tblIdx++, schCurTime+=tblTime) {
//            printf("\nTbl %d", tblIdx);
            assignTable();
            checkDecPic(schCurTime);
E 4
        }

        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);

        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->vbvLevel = pProg->schVbvLevel;
            pProg->outPn = pProg->schOutPn;
I 4
            pProg->decPn = pProg->schDecPn;
            pProg->leftBytes = pProg->schLeftBytes;
E 4

            printf("\n    PID %d: %d TP, VBV %d, in %d, lah %d, "\
D 4
                "send %d:%d (%08x), dec %d", pProg->pid, pProg->schTp,
                pProg->vbvLevel, pProg->inPn-1, pProg->schOutPn,
E 4
I 4
                "send %d:%d (%08x), DTS %08x, decPn %d",
                pProg->pid, pProg->schTp,
                pProg->vbvLevel, pProg->inPn-1, pProg->lookaheadPn,
E 4
                pProg->outPn, pProg->leftBytes,
                getPicStat(pProg, pProg->outPn)->dts, pProg->decPn);

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
