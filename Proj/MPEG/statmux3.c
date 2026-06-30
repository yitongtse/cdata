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


#define SYS_CLK			27000000
#define TP_BYTES		188
#define TP_BITS			(TP_BYTES << 3)
#define	VBV_SZ			1835008

#define MAX_PICS_PER_PROG	1024
#define MAX_LINE_WIDTH		128


int tpPerTbl;
int tblPerLahWin;
int tblPerSchWin;


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
int  tblIdx = 0;
int  schTblIdx;


PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
}


void dumpProgs()
{
    int i;
    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        printf("\nPID %d: msr %.0f, pn %d, DTS %08x",
            pProg->pid, pProg->msr, pProg->msrPn, pProg->msrDts);
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
        if (pn == pProg->inPn)  return pn - 1;
    }
}


// Calculate available input bytes for a program
//
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


// Estimate target picture size
//
int estTargetSize(PicStat* pStat, int outQs)
{
    int tgtSz;
    float deltaQs = outQs - pStat->avgQs;

    if (deltaQs<=0)  return pStat->origBytes;
    if (deltaQs>= 16)  return TP_BYTES;
    tgtSz = (int)((16 - deltaQs) * pStat->origBytes / 16);
    if (tgtSz < TP_BYTES)  tgtSz = TP_BYTES;
    return tgtSz;
}


// Rate reduce a program
//
void rateReduceProg(ProgInfo* pProg, int outQs)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = outQs? estTargetSize(pStat, outQs) : pStat->origBytes;
    }
}


// Find minimum send rate of a program
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
ProgInfo* assignTp()
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
        pProg->msrDts = getPicStat(pProg, pProg->msrPn)->dts;
        if (pProg->msrDts < dts) {
            dts = pProg->msrDts;
            idx = i;
        }
    }
    if (idx==-1)  printf("\n\nALL channels empty!!\n");
    return idx==-1? NULL : &prog[idx];
}


// Table assignment
//
void assignTable()
{
    int i;

    for (i=0; i<tpPerTbl; i++) {
        ProgInfo* pProg = assignTp();
printf("\n TP %d: PID %d", i, pProg->pid);
if (i==0)  dumpProgs();
        pProg->schTp++;
        pProg->schVbvLevel += TP_BITS;
        pProg->schLeftBytes -= TP_BYTES;

        // Check sent picture
        while (pProg->schLeftBytes < 0
                   && pProg->schOutPn < pProg->lahPn) {
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
    getIntParam(&par, "Lookahead interval (ms)", &lahWinSz, 1000);
    getStringParam(&par, "Output statistics filename", outFile, "stat.out");
    endParam(&par);

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

    schWinSz *= 27000;			// convert to 27 MHz
    prefetchSz *= 27000;
    lahWinSz *= 27000;
    curTime = curTimeExt = 0;

    // Adjust schedule window size to contain integer number of tables
    tpTime = (int)((float)SYS_CLK * TP_BITS / outBitrate + 0.5);
    outBitrate = (int)((float)SYS_CLK * TP_BITS / tpTime + 0.5);
    tpPerSchWin = (int)((float)schWinSz / tpTime + 0.5);
    tpPerLahWin = (int)((float)lahWinSz / tpTime + 0.5);
    schWinSz = tpTime * tpPerSchWin;
    lahWinSz = tpTime * tpPerLahWin;

    printf("\nTP time: %d (in 27 MHz)\n", tpTime);
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
            printf("\n  PID %d: in %d, lah %d, out %d",
                pProg->pid, pProg->inPn, pProg->lahPn, pProg->outPn);
        }


        // Rate conversion decision
        //
        printf("\n\nRate decision:");

        for (outQs=0; outQs<=30; outQs+=3) {
            int underflow;
            schTblIdx = tblIdx;
            schCurTime = curTime;

            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];

                // Estimate target size with output QS
                rateReduceProg(pProg, outQs);
                pProg->availBytes = calcAvailBytes(pProg);
                pProg->schTp = 0;

                // Copy variables for scheduling
                pProg->schOutPn = pProg->outPn;
                pProg->schDecPn = pProg->decPn;
                pProg->schVbvLevel = pProg->vbvLevel;
                pProg->schLeftBytes = tblIdx?
                    pProg->leftBytes : pProg->stat[0].tgtBytes;
            }


            for (i=0; i<tpPerSchWin; i++) {

                // Advance current time
                schCurTimeExt += tpTime;
                schCurTime += schCurTimeExt / 600;
                schCurTimeExt %= 600;
            }



            // Detail packet assignment
            for (i=0; i<tblPerLahWin; i++, schTblIdx++, schCurTime+=tblTime) {
                printf("\nQS %d, Tbl %d:", outQs, schTblIdx);
                assignTable();
                underflow = checkDecPic(schCurTime);
                if (underflow)  break;
            }
            if (!underflow)  break;
        }
        printf("\n\n  RC decision: QS %d", outQs);


        // Packet assignment
        //
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->schTp = 0;
            pProg->schOutPn = pProg->outPn;
            pProg->schDecPn = pProg->decPn;
            pProg->schVbvLevel = pProg->vbvLevel;
            pProg->schLeftBytes = tblIdx?
                pProg->leftBytes: pProg->stat[0].tgtBytes;
        }

        printf("\n\nActual packet assignment:");
        schCurTime = curTime;
        for (i=0; i<tblPerSchWin; i++, tblIdx++, schCurTime+=tblTime) {
//            printf("\nTbl %d", tblIdx);
            assignTable();
            checkDecPic(schCurTime);
        }

        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);

        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->vbvLevel = pProg->schVbvLevel;
            pProg->outPn = pProg->schOutPn;
            pProg->decPn = pProg->schDecPn;
            pProg->leftBytes = pProg->schLeftBytes;

            printf("\n    PID %d: %d TP, VBV %d, in %d, lah %d, "\
                "send %d:%d (%08x), DTS %08x, decPn %d",
                pProg->pid, pProg->schTp,
                pProg->vbvLevel, pProg->inPn-1, pProg->lahPn,
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

