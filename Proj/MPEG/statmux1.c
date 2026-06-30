/*****************************************************************************
    File: statmux1.c

    Statmux simulation (video only)
    RateMux alike

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


    * After all packets of a picture have been sent out, outPn should always
      be advanced.  Therefore, outPn will always point to a picture with
      bytes to send.  However, if the picture has not been started yet, then
      leftBytes should be 0.  Otherwise leftBytes should be the remaining
      bytes of the picture to be sent.

    Note: this is copied and modified from statmux.c to study Sangeeta's
          sectioned MSR investigation.

*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"


#define SYS_CLK                 27000000
#define TP_BYTES		188
#define TP_BITS			(TP_BYTES << 3)
#define	VBV_SZ			1835008

#define MAX_PICS_PER_PROG	1024
#define MAX_LINE_WIDTH		128

#define NUM_SECT		4


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
  uint  sendTime;		// time when first TP is sent
  uint  dptTime;		// departure time (time when last TP is sent)
  int   used;			// used flag

  // The following fields are for debugging only
  int   accSz;			// accumulated picture size
  int   delta;			// DTS - PCR for MSR calculation
  int   msr;			// MSR for this picture
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

  float rate;			// output rate (or MSR)
  int  schTp;			// number of TP scheduled for current period
  int  allocTp;			// number of TP actually allocated
				// for current period
  int  vbvLevel;		// VBV level (in bits)

  int  availBytes;		// available bytes to send for current
				// schedule period
  int  leftBytes;		// left size of the last picture
				// in previous schedule window
  uint decDts;			// next decode DTS
  int  allocBytes;		// number of allocated bytes in a table
  int  stuffBytes;		// number of stuff bytes in schedule period
  int  usedTp;			// total of TP used for a schedule window

  // The following fields are for debugging only
  float actRate;		// actual rate

  // The following fields are added for sectioned MSR investigation
  float sectMsr[NUM_SECT];	// MSRs for each section
  float leftMsr;		// MSR for leftover from prev schedule window
}
ProgInfo;


// Global variables
int  progCnt;
ProgInfo* prog;
int  outBitrate;
FILE *inFp, *statFp, *rateFp;
int  numStat;
char line[MAX_LINE_WIDTH+1];

int  tpTime;			// TP time in 27 MHz

uint curTime;                   // current time in 45 kHz
uint curTimeExt;                // current time extension in 27 MHz
uint prevTime;			// time of previous table
uint schTime;                   // end time in 45 kHz
uint lahTime;                   // lookahead window end time in 45 kHz

uint schWinSz;                  // schedule window size in 27 MHz
uint prefetchSz;                // prefetch window size in 27 MHz
uint prefetchTime;              // prefetch window end time in 27 MHz
uint lahWinSz;                  // lookahead window size in 27 MHz

uint vbvMinLevel;

int  tblPerSchWin;		// number of tables per schedule window
int  totSchTp;
int  outQs;
int  tpPerTbl;			// schedule table size in TPs

int  pcrOffset;			// initial PCR offset


PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
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


// Collect picture statistics
//
void collectPicStat(uint stopTime)
{
    int pid;
    ProgInfo* pProg;
    PicStat* pStat;

    while (1) {
        if (fgets(line, MAX_LINE_WIDTH, inFp) == NULL) {
            printf("\nEnd of stat file\n");
            exit (-1);
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

        if (!pProg->inPn) {
            pProg->decDts = pStat->dts;
            printf("\nPID %d: 1st DTS %08x", pProg->pid, pProg->decDts);
        }
        pProg->inPn++;

        if (pStat->arvlTime > stopTime)  break;
		// Note: we assume pictures in the input stat file
		//       are sorted by arrival time
    }
}


// Find the last picture whose DTS is less than the specified time
//
int dts2Pn(ProgInfo* pProg, uint time)
{
    int pn = pProg->outPn;
    while (1) {
        PicStat* pStat = getPicStat(pProg, pn++);
        if (pStat->dts > time)  return pStat->picNo - 1;
        if (pn >= pProg->inPn)  return pn - 1;
    }
}


void resetTargetSize(ProgInfo* pProg)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = pStat->origBytes;
    }
}


float calcMinSendRate(ProgInfo* pProg)
{
    int pn = pProg->outPn;
    int totBits = vbvMinLevel;
    int delta;
    float sendRate = 0.;
    float rate;
    PicStat* pStat;

    // Take care of left over from previous schedule period
    if (pProg->leftBytes > 0) {
        pStat = getPicStat(pProg, pn++);
        totBits += pProg->leftBytes << 3;
        delta = pStat->dts - curTime + pcrOffset;
        rate = 45000. * totBits / delta;
        pProg->leftMsr = sendRate = rate;
        pProg->msrPn = pStat->picNo;

#if 1
        pStat->accSz = totBits;
        pStat->delta = delta;
        pStat->msr = (int)(rate + 0.5);
#endif
    }
    else
        pProg->leftMsr = 0.;

    while (pn <= pProg->lahPn) {
        pStat = getPicStat(pProg, pn++);
        totBits += pStat->tgtBytes << 3;
        delta = pStat->dts - curTime + pcrOffset;
        rate = 45000. * totBits / delta;
        if (rate > sendRate) {
            sendRate = rate;
            pProg->msrPn = pStat->picNo;
        }

#if 1
        pStat->accSz = totBits;
        pStat->delta = delta;
        pStat->msr = (int)(rate + 0.5);
#endif
    }

    return sendRate;
}


float calcSectMinSendRate(ProgInfo* pProg, int* startPn, int endTime)
{
    float sectMsr = 0.;

    while (1) {
        PicStat* pStat = getPicStat(pProg, *startPn);
        if (pStat->dts > endTime)  break;
        if (pStat->msr > sectMsr)  sectMsr = pStat->msr;
        (*startPn)++;
    }
    return sectMsr;
}


int calcAvailBytes(ProgInfo* pProg)
{
    int availBytes = pProg->leftBytes;
    int pn = pProg->outPn;
    if (availBytes > 0)  pn++;		// skip over partially sent picture
    while (pn <= pProg->lahPn)
        availBytes += getPicStat(pProg, pn++)->tgtBytes;
    return availBytes;
}


int estTargetSize(PicStat* pStat, int outQs)
{
#if 0
    float deltaQs = outQs - pStat->avgQs;
    if (deltaQs<=0)  return pStat->origBytes;
    if (deltaQs>= 16)  return 0;
    return (int)((16 - deltaQs) * pStat->origBytes / 16);
#else
    return (int)((float)pStat->origBytes * pStat->avgQs / outQs);
#endif
}


void rateReduceProg(ProgInfo* pProg, int outQs)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
    while (pn < pProg->inPn) {
        PicStat* pStat = getPicStat(pProg, pn++);
        pStat->tgtBytes = outQs > pStat->avgQs?
                              estTargetSize(pStat, outQs) : pStat->origBytes;
    }
}


void tableUpdate(ProgInfo *pProg)
{
    PicStat *pStat;
    int allocBytes = pProg->allocTp * TP_BYTES;    // remaining allocated bytes

    // Check to see if all pictures of the program has been sent
    if (pProg->outPn == pProg->inPn) {
        // No more picture to send
        pProg->stuffBytes += allocBytes;
        printf("\n** Stuffing!!!");
        return;
    }

    pProg->usedTp += pProg->allocTp;

    // See if we are right at a picture boundary
    if (pProg->leftBytes == 0) {
        // Start a new picture to output
        pStat = getPicStat(pProg, pProg->outPn);
        pProg->leftBytes = pStat->tgtBytes;
        if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
    }

    while (allocBytes > 0) {
        int usedBytes = min(allocBytes, pProg->leftBytes);
        allocBytes -= usedBytes;
        pProg->leftBytes -= usedBytes;
        pProg->vbvLevel += usedBytes << 3;

        // Check for completion of a picture
        if (pProg->leftBytes == 0) {
            pStat = getPicStat(pProg, pProg->outPn);
            pStat->dptTime = curTime;
//            printf("\n    Pic %d-%d sent, tgtSz %d, left %d, DTS %08x",
//                pStat->pid, pStat->picNo, pStat->tgtBytes, pProg->leftBytes,
//                pStat->dts);

            pProg->outPn++;             // advance output pointer

            if (pProg->outPn == pProg->inPn) {
                // No more picture to send
                pProg->stuffBytes += allocBytes;
                pProg->usedTp -= allocBytes / TP_BYTES;

                printf("\n** Stuffing!!!");
                return;
            }

            if (allocBytes > 0) {
                // Start a new picture to output
                pStat = getPicStat(pProg, pProg->outPn);
                pProg->leftBytes = pStat->tgtBytes;
                pStat->sendTime = prevTime;
                if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
            }
        }
    }
}


void printMinSendRate()
{
    int i;
    float totRate = 0;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        PicStat* pStat = getPicStat(pProg, pProg->msrPn);
        int availBytes = calcAvailBytes(pProg);
        printf("\n  PID %4d: avg %d, msr %d (Pic %d, AP %d, delta %d)",
            pProg->pid, (int)(27000000.*8*availBytes/lahWinSz),
            (int)pProg->rate, pProg->msrPn, pStat->accSz, pStat->delta);
        totRate += pProg->rate;
    }
    printf("\n  Total rate = %d\n", (int)totRate);
}


void printMinSendRate2()
{
    int i;
    float totRate = 0;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        PicStat* pStat = getPicStat(pProg, pProg->msrPn);
        int availBytes = calcAvailBytes(pProg);
        printf("\n  PID %4d: avg %d, msr %d (Pic %d, AP %d, delta %d)",
            pProg->pid, (int)(27000000.*8*availBytes/lahWinSz),
            (int)pProg->rate, pProg->msrPn, pStat->accSz, pStat->delta);
        totRate += pProg->rate;
    }
}


int main(int argc, char** argv)
{
    int i, j;
    Param par;
    char inFile[128], statFile[128], rateFile[128];
    ProgInfo* pProg;
    PicStat* pStat;
    float totRate;
    int bitPerTbl;		// number of bits per table
    int tblTime;		// table time in 27 MHz
    int leftTp;
    int vbvMargin;
    int remainTp, extraTp;
    int tblIdx = 0;
    int schIdx = 0;
    float totMsr, totSectMsr[NUM_SECT], maxTotSectMsr;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "statmux1.par");
    getStringParam(&par, "Transport stream statistics filename",
        inFile, "test.stat");
    getIntParam(&par, "Number of video programs", &progCnt, 2);
    prog = malloc(progCnt * sizeof(ProgInfo));
    for (i=0; i<progCnt; i++)
        getIntParam(&par, "Video PID", &prog[i].pid, 0);
    getIntParam(&par, "Output bitrate (bps)", &outBitrate, 27000000);
    getIntParam(&par, "Schedule interval (ms)", &schWinSz, 250);
    getIntParam(&par, "Lookahead interval (ms)", &lahWinSz, 1000);
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
    getIntParam(&par, "PCR offset (ms)", &pcrOffset, 0);
    getIntParam(&par, "VBV guard band (bytes)", &vbvMinLevel, 50*TP_BYTES);
    getIntParam(&par, "Table size (TPs)", &tpPerTbl, 64);
    getStringParam(&par, "Output statistics filename", statFile, "stat.out");
    getStringParam(&par, "Output rate allocation filename", rateFile,
        "rate.out");
    endParam(&par);

    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
        printf("Error: Failed to open input stat file %s\n", inFile);
        exit(-1);
    }
    fgets(line, MAX_LINE_WIDTH, inFp);	// skip header line

    statFp = fopen(statFile, "wb");
    if (statFp == NULL) {
        printf("Error: Failed to open output stat file %s\n", statFile);
        exit(-1);
    }

    rateFp = fopen(rateFile, "wb");
    if (rateFp == NULL) {
        printf("Error: Failed to open output rate file %s\n", rateFile);
        exit(-1);
    }

    fprintf(statFp, "Stat file: %s", inFile);
    fprintf(statFp, "\nPID:");
    for (i=0; i<progCnt; i++)
        fprintf(statFp, " %d", prog[i].pid);
    fprintf(statFp, "\nOutrate %d, schedule %d, lookahead %d, prefetch %d, "\
        "tableSz %d", outBitrate, schWinSz, lahWinSz, prefetchSz, tpPerTbl);

    // Initialize scheduler
    schWinSz *= 27000;			// convert to 27 MHz
    lahWinSz *= 27000;
    prefetchSz *= 27000;
    curTime = curTimeExt = 0;
    prevTime = curTime;

    // Adjust schedule window size to contain integer number of tables
    tpTime = (int)((float)SYS_CLK * TP_BITS / outBitrate + 0.5);
    outBitrate = (int)((float)SYS_CLK * TP_BITS / tpTime + 0.5);
    tblTime = tpTime * tpPerTbl;
    bitPerTbl = tpPerTbl * TP_BITS;
    tblPerSchWin = (int)(schWinSz / tblTime + 0.5);
    schWinSz = tblTime * tblPerSchWin;

#if 1
    // Force lookahead window to contain multiples of TPs to make algorithm
    // comparison easy
  {
    int tpPerLahWin = (int)((float)lahWinSz / tpTime + 0.5);
    lahWinSz = tpTime * tpPerLahWin;
  }
#endif

    printf("\nTP time: %d (in 27 MHz)", tpTime);
    printf("\nOutput bitrate: %d bps", outBitrate);
    printf("\nTables per schdule window: %d", tblPerSchWin);
    printf("\nSchedule window: %d ms, %d tables, %d TPs",
           schWinSz/27000, tblPerSchWin, tpPerTbl*tblPerSchWin);
    printf("\nLookahead window size: %d ms", lahWinSz/27000);

    // Initial programs
    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
        pProg->stat = malloc(MAX_PICS_PER_PROG * sizeof(PicStat));
        memset(pProg->stat, 0, MAX_PICS_PER_PROG * sizeof(PicStat));
        pProg->inPn = pProg->outPn = pProg->decPn = 0;
        pProg->vbvLevel = pProg->leftBytes = 0;
        pProg->decDts = 0x7FFFFFFF;
    }

    while (1) {
        // Update schedule window
        schTime = curTime + (curTimeExt + schWinSz) / 600;
        prefetchTime = curTime + (curTimeExt + prefetchSz) / 600;
        lahTime = curTime + (curTimeExt + lahWinSz) / 600;

        printf("\n\n---------------------------------------------------------------------------");
        printf("\n\nPeriod %d: cur %08x, sch %08x, lah %08x, prefetch %08x",
               schIdx, curTime, schTime, lahTime, prefetchTime);

        collectPicStat(prefetchTime);

        printf("\n\nStat collection:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->lahPn = dts2Pn(pProg, lahTime);
            printf("\n  PID %4d: in %d, lah %d, out %d, dec %d", pProg->pid,
                pProg->inPn, pProg->lahPn, pProg->outPn, pProg->decPn);
        }

        // Rate conversion decision
        for (outQs=0; outQs<=30; outQs++) {
            totRate = 0.;
            totSchTp = 0;
            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
                rateReduceProg(pProg, outQs);
                pProg->rate = calcMinSendRate(pProg);

                if (!outQs) {
                    int sectPn = pProg->outPn;
                    for (j=0; j<NUM_SECT; j++) {
                        pProg->sectMsr[j] = calcSectMinSendRate(pProg,
                            &sectPn, curTime + lahWinSz*(j+1)/(600*NUM_SECT));
                    }
                    if (pProg->leftMsr > pProg->sectMsr[0])
                        pProg->sectMsr[0] = pProg->leftMsr;
//                    printf("\nPID %d: MSR %.1f -", pProg->pid, pProg->rate);
//                    for (j=0; j<NUM_SECT; j++)
//                        printf(" %.1f", pProg->sectMsr[j]);
                }

                pProg->schTp = (pProg->rate * tpPerTbl + outBitrate - 1)
                                  / outBitrate;
                totRate += pProg->rate;
                totSchTp += pProg->schTp;
            }
            remainTp = tpPerTbl - totSchTp;

            printf("\n  RC: outQs %d, rate %d, %d TP/Tbl",
                   outQs, (int)totRate, totSchTp);

            if (remainTp >= 0 || outQs == 0) {
                printf("\n\nMSR (outQs %d):", outQs);
                printMinSendRate();
            }

            if (!outQs) {
                totMsr = 0.;
                for (j=0; j<NUM_SECT; j++)  totSectMsr[j] = 0;

                for (i=0; i<progCnt; i++) {
                    pProg = &prog[i];
                    totMsr += pProg->rate;
                    for (j=0; j<NUM_SECT; j++) {
                        totSectMsr[j] += pProg->sectMsr[j];
                    }
                }

            }

            if (remainTp >= 0)  break;
        }

        printf("\nTime %08x, outQs %d, MSR %.0f -", curTime, outQs, totMsr);
        maxTotSectMsr = 0;
        for (j=0; j<NUM_SECT; j++) {
            printf(" %.0f", totSectMsr[j]);
            if (totSectMsr[j] > maxTotSectMsr)  maxTotSectMsr = totSectMsr[j];
        }
        printf(" : %.4f", (totMsr - maxTotSectMsr) / totMsr * 100.);


        // Distribute the remaining TPs
        extraTp = (remainTp + progCnt - 1) / progCnt;
        for (i=0; i<progCnt; i++) {
            int n;
            pProg = &prog[i];
            pProg->availBytes = calcAvailBytes(pProg);
            pProg->usedTp = 0;

            n = min(extraTp, remainTp);
            pProg->schTp += n;
            remainTp -= n;
        }

        printf("\n\nPacket allocation:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->actRate = (float)pProg->schTp * outBitrate / tpPerTbl;
            printf("\n  PID %4d: %d TP, rate %d, msr %d", pProg->pid,
                pProg->schTp, (int)pProg->actRate, (int)pProg->rate);
            fprintf(rateFp, "\n%d \t%08x \t%08x \t%d", pProg->pid, curTime,
                curTimeExt, pProg->schTp*tblPerSchWin*TP_BYTES);
        }

        // Simulate VBV
        for (j=0; j<tblPerSchWin; j++) {
            // Advance current time
            prevTime = curTime;
            curTimeExt += tblTime;
            curTime += curTimeExt / 600;
            curTimeExt %= 600;
//            printf("\n\nTbl %d: %08x", tblIdx++, curTime);

            // Actual TP allocation
            leftTp = tpPerTbl;
            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
                vbvMargin = (VBV_SZ - pProg->vbvLevel) / TP_BITS;
                pProg->allocTp = min(pProg->schTp, vbvMargin);
                pProg->allocTp = min(pProg->allocTp,
                                         pProg->availBytes/TP_BYTES);
                if (pProg->allocTp != pProg->schTp)
                    printf("\n    PID %d: sch %d, VBV margin %d, avail %d",
                        pProg->pid, pProg->schTp, vbvMargin,
                        pProg->availBytes/TP_BYTES);

                leftTp -= pProg->allocTp;
            }
            if (leftTp)  printf("\n  Leftover TP: %d", leftTp);

            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
                tableUpdate(pProg);

                if (pProg->vbvLevel > VBV_SZ) {
                    printf("\nVBV overflow!");
//                    exit (-1);
                }

//                printf("\n  PID %d: TP %d, vbv %d, left %d, avail %d, stuff %d",
//                    pProg->pid, pProg->allocTp, pProg->vbvLevel,
//                    pProg->leftBytes, pProg->availBytes, pProg->stuffBytes);

                if (curTime >= pProg->decDts) {
                    int avgRate;

                    pStat = getPicStat(pProg, pProg->decPn++);
                    pProg->vbvLevel -= pStat->tgtBytes << 3;
//                    printf("\n    Pic %d decoded, DTS %08x, sz %d, vbv %d",
//                        pStat->picNo, pProg->decDts, pStat->tgtBytes,
//                        pProg->vbvLevel);
                    pStat->used = 0;

                    if (pProg->vbvLevel < 0) {
                        printf("\nVBV underflow!");
                        printf("\tPic MSR %d, Act rate %.0f",
                            pStat->msr, pProg->actRate);
//                        exit (-1);
                    }

                    pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
                    avgRate = pStat->tgtBytes * 45000
                                  / (pStat->dptTime - pStat->sendTime);

                    fprintf(statFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x "\
                        "\t%d \t%d \t%d", pStat->pid, pStat->picNo,
                        pStat->picType, pStat->tgtBytes, pStat->avgQs,
                        pStat->dts, pStat->dptTime, pProg->vbvLevel>>3,
                        avgRate, pStat->origBytes);
                }
            }
        }

        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            printf("\n  PID %4d: %d TP, VBV %d, PN: in %d, lah %d, "\
                "send %d:%d, dec %d", pProg->pid, pProg->usedTp,
                pProg->vbvLevel, pProg->inPn-1, pProg->lahPn,
                pProg->outPn, pProg->leftBytes, pProg->decPn);

            printf("\n            DTS: out %08x, dec %08x",
                getPicStat(pProg, pProg->outPn)->dts,
                getPicStat(pProg, pProg->decPn)->dts);

            if (pProg->outPn > pProg->inPn) {
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

