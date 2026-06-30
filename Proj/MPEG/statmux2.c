/*****************************************************************************
    File: statmux2.c

    Statmux simulation (video only)
    VBV based allocation

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
  uint decDts;			// next decode DTS

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

int  tblPerSchWin;		// number of tables per schedule window
int  tblPerLahWin;		// number of tables per lookahead window
int  totSchTp;
int  outQs;
int  tpPerTbl;			// schedule table size in TPs


PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
}


// Find program number based on PID
//   Returns NULL if PID not found
//
ProgInfo* findProg(int pid)
{
    int i;
    for (i=0; i<progCnt; i++)
        if (prog[i].pid == pid)  return &prog[i];
    return NULL;
}


ProgInfo* findMinVbvProg()
{
    int idx = -1;
    int minVbvLevel = VBV_SZ + 1;
    int i;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        int level;

        if (pProg->schVbvLevel + TP_BITS > VBV_SZ)
            continue;			// overflow prevention

        if (pProg->schOutPn >= pProg->inPn-1)
            continue;			// empty channel prevention

        level = pProg->schVbvLevel
                    - (getPicStat(pProg, pProg->schDecPn)->tgtBytes << 3);
					// VBV level after next picture removal
        if (level < minVbvLevel) {
            minVbvLevel = level;
            idx = i;
        }
    }

    if (idx==-1)  printf("\n\nALL channel buffers are full!!");
    return idx==-1? NULL : &prog[idx];
}


// Find the last picture whose DTS is less than the specified time
int findLastDtsPn(ProgInfo* pProg, uint time)
{
    int pn = pProg->outPn;
    while (1) {
        PicStat* pStat = getPicStat(pProg, pn++);
        if (pStat->dts > time)  return pStat->picNo - 1;
        if (pn == pProg->inPn)  return pn - 1;
    }
}


void resetTargetSize(ProgInfo* pProg)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn <= pProg->lookaheadPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = pStat->origBytes;
    }
}


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


int estTargetSize(PicStat* pStat, int outQs)
{
    float deltaQs = outQs - pStat->avgQs;
    if (deltaQs<=0)  return pStat->origBytes;
    if (deltaQs>= 16)  return 0;
    return (int)((16 - deltaQs) * pStat->origBytes / 16);
}


void rateReduceProg(ProgInfo* pProg, int outQs)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = outQs? estTargetSize(pStat, outQs) : pStat->origBytes;
    }
}


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
    int tblIdx = 0;
    int schTblIdx;
    int schIdx = 0;
    int schCurTime;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "statmux2.par");
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

        // Collect PicStats for next schedule period
        while (1) {
            int pid;

            if (fgets(line, MAX_LINE_WIDTH, inFp) == NULL) {
                printf("\nEnd of stat file\n");
                return;
            }
            sscanf(line, "%d", &pid);

            // Map program from PID
            pProg = findProg(pid);
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

            if (!pProg->inPn)  pProg->decDts = pStat->dts;
            pProg->inPn++;

            if (pStat->arvlTime > prefetchTime)  break;
			// Note: we assume pictures in the input stat file
			//       are sorted by arrival time
        }

        printf("\n\nStat collection:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            printf("\n  PID %d: in %d", pProg->pid, pProg->inPn);
        }

        // Rate conversion decision
        for (outQs=0; outQs<=30; outQs+=3) {
            int underflow = 0;
            schTblIdx = tblIdx;
            schCurTime = curTime;

            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];

                // Estimate target size with output QS
                rateReduceProg(pProg, outQs);
                pProg->availBytes = calcAvailBytes(pProg);
                pProg->schTp = 0;
                pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;

                // Copy variables for scheduling
                pProg->schOutPn = pProg->outPn;
                pProg->schDecPn = pProg->decPn;
                pProg->schVbvLevel = pProg->vbvLevel;
                pProg->schLeftBytes = tblIdx?
                    pProg->leftBytes : pProg->stat[0].tgtBytes;
            }

            // Detail packet assignment
            for (i=0; i<tblPerLahWin; i++, schTblIdx++, schCurTime+=tblTime) {
//                printf("\nTbl %d: %08x", schTblIdx, schCurTime);

                for (j=0; j<tpPerTbl; j++) {
                    pProg = findMinVbvProg();
//                    printf("\n  PID %d: vbv %d",
//                        pProg->pid, pProg->schVbvLevel);

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

                // Check for decoded pictures
                for (j=0; j<progCnt; j++) {
                    pProg = &prog[j];
                    if (schCurTime >= pProg->decDts) {
                        pStat = getPicStat(pProg, pProg->schDecPn++);
                        pProg->schVbvLevel -= pStat->tgtBytes << 3;
                        printf("\n  QS %d: Dec Pic %d-%d, tgtSz %d, schTp %d, "\
                            "vbv %d", outQs, pProg->pid, pStat->picNo,
                            pStat->tgtBytes, pProg->schTp, pProg->schVbvLevel);
                        pProg->decDts = getPicStat(pProg, pProg->schDecPn)->dts;

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
                if (underflow)  break;
            }
            if (!underflow)  break;
        }
        printf("\n  RC decision: QS %d", outQs);


        // Packet assignment phase
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->schTp = 0;
            pProg->schOutPn = pProg->outPn;
				// Note: findMinVbvProg() uses schVbvLevel
            pProg->schVbvLevel = pProg->vbvLevel;
            pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
            if (!tblIdx)  pProg->leftBytes = pProg->stat[0].tgtBytes;
        }

        for (i=0; i<tblPerSchWin; i++, tblIdx++, curTime+=tblTime) {
            for (j=0; j<tpPerTbl; j++) {
                pProg = findMinVbvProg();
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
                if (curTime >= pProg->decDts) {
                    pStat = getPicStat(pProg, pProg->decPn++);
                    pProg->schVbvLevel -= pStat->tgtBytes << 3;
                    printf("\n  Dec Pic %d-%d, tgtSz %d, schTp %d, vbv %d",
                        pProg->pid, pStat->picNo, pStat->tgtBytes,
                        pProg->schTp, pProg->schVbvLevel);

                    if (pProg->schVbvLevel < 0)
                        printf("\tVBV underflow!");
                    pStat->used = 0;
                    pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
                }
            }
        }

        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);

        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->vbvLevel = pProg->schVbvLevel;
            pProg->outPn = pProg->schOutPn;

            printf("\n    PID %d: %d TP, VBV %d, in %d, lah %d, "\
                "send %d:%d (%08x), dec %d", pProg->pid, pProg->schTp,
                pProg->vbvLevel, pProg->inPn-1, pProg->schOutPn,
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

