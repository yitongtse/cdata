h38341
s 00011/00002/00594
d D 1.23 02/07/12 16:31:47 ytse 23 22
c Align with statmux4.c
e
s 00023/00042/00573
d D 1.22 02/07/12 16:03:23 ytse 22 21
c A good version
e
s 00096/00036/00519
d D 1.21 02/07/12 15:38:54 ytse 21 20
c After fixing schTp rounding problem
e
s 00036/00020/00519
d D 1.20 02/07/10 16:49:37 ytse 20 19
c Update
e
s 00086/00072/00453
d D 1.19 02/07/08 13:29:43 ytse 19 18
c Update
e
s 00109/00079/00416
d D 1.18 02/07/03 14:07:13 ytse 18 17
c Backup after adding tableUpdate().
e
s 00004/00000/00491
d D 1.17 02/07/01 15:30:02 ytse 17 16
c Changed image model
e
s 00026/00016/00465
d D 1.16 02/06/25 11:52:50 ytse 16 15
c Backup
e
s 00041/00007/00440
d D 1.15 02/06/21 12:48:03 ytse 15 14
c After fixing bug about resetting tgt size before MSR calculation for no RC.
e
s 00015/00008/00432
d D 1.14 02/06/21 01:45:39 ytse 14 13
c First working version!
e
s 00094/00058/00346
d D 1.13 02/06/21 01:32:40 ytse 13 12
c Backup before fixing leftByte problem
e
s 00048/00017/00356
d D 1.12 02/06/18 17:23:08 ytse 12 11
c Backup before backtracking for bugs
e
s 00099/00107/00274
d D 1.11 02/06/17 12:20:10 ytse 11 10
c Backup before adding overflow detection
e
s 00001/00006/00380
d D 1.10 02/06/14 11:27:45 ytse 10 9
c Backup before using integer arithmethics
e
s 00006/00004/00380
d D 1.9 02/06/12 17:05:01 ytse 9 8
c Fix VBV underflow problem
e
s 00061/00027/00323
d D 1.8 02/06/12 16:58:32 ytse 8 7
c Added VBV decoding simulation
e
s 00011/00012/00339
d D 1.7 02/06/12 15:47:42 ytse 7 6
c 
e
s 00068/00100/00283
d D 1.6 02/06/12 14:38:27 ytse 6 5
c Remove FIFO
e
s 00002/00002/00381
d D 1.5 02/06/12 13:17:36 ytse 5 4
c Backup before discarding fifo
e
s 00057/00028/00326
d D 1.4 02/06/11 15:56:49 ytse 4 3
c 
e
s 00021/00012/00333
d D 1.3 02/06/11 14:16:32 ytse 3 2
c 
e
s 00158/00084/00187
d D 1.2 02/06/04 17:45:21 ytse 2 1
c Backup
e
s 00271/00000/00000
d D 1.1 02/06/04 11:41:54 ytse 1 0
c date and time created 02/06/04 11:41:54 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: statmux.c

    Statmux simulation (video only)
I 16
    RateMux alike
E 16
I 2

    Note:
      To handle partial picture at the turn of a schedule period, we store
D 11
      the number of bytes remaining in the last picture in ProgInfo::leftSz.
E 11
I 11
      the number of bytes remaining in the last picture in ProgInfo::leftBytes.
E 11
D 14
      This value will always belong to the picture previous to the first
      picture being scheduled in current window.
E 14
I 14
      This value will always belong to the picture pointer to by outPn.
E 14
I 13

I 14
      The picture pointed to by outPn need special attention when the
      program's leftBytes>0, because this picture's outQs is determined
      in the previous schedule window.

E 14
      If we do not buffered enough picture before we do scheduling, we may
      send pictures that is not even arrived yet!  We temporarily solve this
      problem by increasing the distance between inPn and outPn.
I 18

D 19
      When leftByte == 0, outPn should point to the next picture to be sent,
      not the picture that has been sent!
E 19
I 19

    * After all packets of a picture have been sent out, outPn should always
      be advanced.  Therefore, outPn will always point to a picture with
      bytes to send.  However, if the picture has not been started yet, then
      leftBytes should be 0.  Otherwise leftBytes should be the remaining
      bytes of the picture to be sent.

E 19
E 18
E 13
E 2
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
D 6
#include "fifo.h"
E 6


I 18
#define SYS_CLK                 27000000
E 18
I 11
#define TP_BYTES		188
#define TP_BITS			(TP_BYTES << 3)
E 11
D 2
#define MAX_PICS_PER_PROG	128
E 2
I 2
D 5
#define MAX_PICS_PER_PROG	256
E 5
I 5
D 12
#define MAX_PICS_PER_PROG	1024
E 12
I 12
D 13
#define VBV_SZ			1835008
E 12
E 5
E 2
#define MAX_LINE_WIDTH		128
E 13
I 13
#define	VBV_SZ			1835008
I 18

E 18
E 13
I 12
#define MAX_PICS_PER_PROG	1024
I 13
#define MAX_LINE_WIDTH		128
E 13
E 12


// Picture statistics
//
typedef struct {
  int   pid;			// PID
  int   picNo;			// picture number (decoder order) within PID
D 11
  int   origSz;                 // original picture size (in bytes)
E 11
I 11
  int   origBytes;              // original picture size (in bytes)
E 11
  int   picType;                // picture type
  uint  dts;                    // DTS (mapped to local clock) (in 45 kHz)
  uint  arvlTime;               // arrival time
  float avgQs;                  // average quant scale
D 11
  int   tgtSz;			// target size after reduction (in bytes)
E 11
I 11
  int   tgtBytes;		// target size after reduction (in bytes)
E 11
I 4
  uint  dptTime;		// departure time
E 4
I 2
  int   used;			// used flag
I 21

  // The following fields are for debugging only
  int   accSz;			// accumulated picture size
  int   delta;			// DTS - PCR for MSR calculation
  int   msr;			// MSR for this picture
E 21
E 2
}
PicStat;


typedef struct {
I 6
  PicStat* stat;
E 6
  uint pid;
D 6
  int  inPicCnt;		// input picture count
  int  outPicCnt;		// output picture count

  PicStat* stat[MAX_PICS_PER_PROG];
  Fifo statFifo;

E 6
I 6
  int  inPn;			// picutre number of next input picture
  int  outPn;			// picture number of next output picture
I 8
  int  decPn;			// picture number of next decode picture
E 8
E 6
D 2
  int  rate;			// output rate
E 2
I 2
  int  msrPn;			// picture number which determines msr
D 18
  int  lookaheadPn;		// lookahead picture number
E 18
I 18
  int  lahPn;			// lookahead picture number
E 18
D 6
  int  schPn;			// scheduled picture number
E 6

D 3
  int  rate;			// output rate (msr)
E 3
I 3
D 8
  int  rate;			// output rate
E 8
I 8
  float rate;			// output rate
E 8
E 3
E 2
  int  schTp;			// number of TP scheduled for current period
I 12
D 13
  int  allocTp;			// number of actually allocated TP
				// for current table
E 13
I 13
  int  allocTp;			// number of TP actually allocated
				// for current period
  int  vbvLevel;		// VBV level (in bits)

  int  availBytes;		// available bytes to send for current
				// schedule period
E 13
E 12
I 2
D 11
  int  leftSz;			// left size of the last picture
E 11
I 11
  int  leftBytes;		// left size of the last picture
E 11
				// in previous schedule window
I 8
D 13
  int  vbvLevel;		// VBV level (in bits)
E 13
I 11
  uint decDts;			// next decode DTS
I 18
  int  allocBytes;		// number of allocated bytes in a table
  int  stuffBytes;		// number of stuff bytes in schedule period
I 20
  int  usedTp;			// total of TP used for a schedule window
I 21

  // The following fields are for debugging only
  float  actRate;			// actual rate
E 21
E 20
E 18
E 11
E 8
E 2
}
ProgInfo;


// Global variables
int  progCnt;
ProgInfo* prog;
int  outBitrate;
D 4
FILE *fp;
E 4
I 4
FILE *inFp, *outFp;
E 4
D 11
int numStat;
E 11
I 11
int  numStat;
E 11
D 6
PicStat* stat;
D 2
int  inId, outId;		// input and output IDs for stat
E 2
I 2
Fifo statFifo;
E 6
E 2
char line[MAX_LINE_WIDTH+1];

D 18
uint curTime;
uint schBeginTime;
uint schEndTime;
D 4
uint schInterval;
E 4
I 4
uint schWinSz;
E 4
D 6
uint lookaheadEndTime;
E 6
I 6
D 13
uint lookaheadTime;
E 13
I 13
uint prefetchSz;
uint prefetchTime;
E 13
E 6
D 4
uint lookaheadInterval;
E 4
I 4
uint lookaheadWinSz;
I 13
uint lookaheadTime;
E 18
I 18
int  tpTime;			// TP time in 27 MHz

uint curTime;                   // current time in 45 kHz
uint curTimeExt;                // current time extension in 27 MHz
uint schTime;                   // end time in 45 kHz
uint lahTime;                   // lookahead window end time in 45 kHz

uint schWinSz;                  // schedule window size in 27 MHz
uint prefetchSz;                // prefetch window size in 27 MHz
uint prefetchTime;              // prefetch window end time in 27 MHz
uint lahWinSz;                  // lookahead window size in 27 MHz

E 18
E 13
E 4
uint vbvMinLevel;
D 18
uint delay;
E 18

D 13
uint tpPerSchWin;		// number of TPs per schedule window
uint totSchTp;
E 13
I 13
int  tblPerSchWin;		// number of tables per schedule window
int  totSchTp;
E 13
I 3
int  outQs;
I 11
D 13
int  tpsPerTbl;			// schedule table size in TPs
E 13
I 13
int  tpPerTbl;			// schedule table size in TPs
E 13
E 11
E 3


I 16
PicStat* getPicStat(ProgInfo* pProg, int pn)
{
    return &pProg->stat[pn % MAX_PICS_PER_PROG];
}


E 16
// Find program number based on PID
D 18
//   Returns -1 if PID not found
E 18
I 18
//   Returns NULL if PID not found
E 18
//
D 18
int findProg(int pid)
E 18
I 18
ProgInfo* pid2Prog(int pid)
E 18
{
    int i;
    for (i=0; i<progCnt; i++)
D 18
        if (prog[i].pid == pid)  return i;
    return -1;
E 18
I 18
        if (prog[i].pid == pid)  return &prog[i];
    return NULL;
E 18
}

I 21

E 21
I 19
// Collect picture statistics
//
void collectPicStat(uint stopTime)
{
    int pid;
    ProgInfo* pProg;
    PicStat* pStat;
E 19

I 19
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

        if (!pProg->inPn)  pProg->decDts = pStat->dts;
        pProg->inPn++;

        if (pStat->arvlTime > stopTime)  break;
		// Note: we assume pictures in the input stat file
		//       are sorted by arrival time
    }
}


E 19
I 6
// Find the last picture whose DTS is less than the specified time
D 20
int findLastDtsPn(ProgInfo* pProg, uint time)
E 20
I 20
//
int dts2Pn(ProgInfo* pProg, uint time)
E 20
{
    int pn = pProg->outPn;
    while (1) {
D 16
        PicStat* pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 16
I 16
        PicStat* pStat = getPicStat(pProg, pn++);
E 16
        if (pStat->dts > time)  return pStat->picNo - 1;
D 21
        if (pn == pProg->inPn)  return pn - 1;
E 21
I 21
        if (pn >= pProg->inPn)  return pn - 1;
E 21
    }
}


I 15
void resetTargetSize(ProgInfo* pProg)
{
    int pn = pProg->outPn;
    if (pProg->leftBytes > 0)  pn++;
D 18
    while (pn <= pProg->lookaheadPn) {
E 18
I 18
    while (pn < pProg->inPn) {
E 18
D 16
        PicStat* pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 16
I 16
        PicStat* pStat = getPicStat(pProg, pn++);
E 16
        pStat->tgtBytes = pStat->origBytes;
    }
}


E 15
E 6
D 2
int calcMinSendRate(ProgInfo* prog)
E 2
I 2
D 8
int calcMinSendRate(ProgInfo* pProg)
E 8
I 8
float calcMinSendRate(ProgInfo* pProg)
E 8
E 2
{
D 2
    int i = prog->outPicId;
E 2
I 2
D 6
    int i;
E 6
I 6
    int pn = pProg->outPn;
E 6
E 2
    int totBits = vbvMinLevel;
I 21
    int delta;
E 21
    float sendRate = 0.;
    float rate;
I 2
    PicStat* pStat;
E 2

D 2
    while (1) {
        PicStat* stat = prog->stat[i];
        if (stat->dts > lookaheadEndTime)
            break;
E 2
I 2
    // Take care of left over from previous schedule period
D 11
    if (pProg->leftSz < 0) {
E 11
I 11
D 14
    if (pProg->leftBytes < 0) {
E 14
I 14
    if (pProg->leftBytes > 0) {
E 14
E 11
D 6
        i = fifoIdx(&pProg->statFifo, pProg->statFifo.rdIdx, -1);
E 6
I 6
D 16
        pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 16
I 16
        pStat = getPicStat(pProg, pn++);
E 16
E 6
D 11
        totBits -= pProg->leftSz << 3;		// Note: leftSz is negative
E 11
I 11
D 14
        totBits -= pProg->leftBytes << 3;	// Note: leftBytes is negative
E 14
I 14
        totBits += pProg->leftBytes << 3;
E 14
E 11
D 6
        sendRate = 45000. * totBits / (pProg->stat[i]->dts - curTime);
E 6
I 6
D 21
        sendRate = 45000. * totBits / (pStat->dts - curTime);
E 21
I 21
        delta = pStat->dts - curTime;
        rate = 45000. * totBits / delta;

#if 1
        pStat->accSz = totBits;
        pStat->delta = delta;
        pStat->msr = (int)(rate + 0.5);
#endif

        sendRate = rate;
E 21
I 15
        pProg->msrPn = pStat->picNo;
E 15
E 6
    }
E 2

D 2
        totBits += stat->tgtSz << 3;
        rate = 45000. * totBits / (stat->dts - curTime);
        if (rate > sendRate)  sendRate = rate;
E 2
I 2
D 6
    i = pProg->statFifo.rdIdx;
E 2

D 2
        if (stat->picNo == prog->inPicCnt-1)
            break;		// last input picture reached

        if (++i >= MAX_PICS_PER_PROG)  i = 0;
E 2
I 2
    while (1) {
        pStat = pProg->stat[i];
        if (pStat->dts > lookaheadEndTime)  break;
E 6
I 6
D 18
    while (pn <= pProg->lookaheadPn) {
E 18
I 18
    while (pn <= pProg->lahPn) {
E 18
D 16
        pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 16
I 16
        pStat = getPicStat(pProg, pn++);
E 16
E 6
D 11
        totBits += pStat->tgtSz << 3;
E 11
I 11
        totBits += pStat->tgtBytes << 3;
E 11
D 21
        rate = 45000. * totBits / (pStat->dts - curTime);
E 21
I 21
        delta = pStat->dts - curTime;
        rate = 45000. * totBits / delta;

#if 1
        pStat->accSz = totBits;
        pStat->delta = delta;
        pStat->msr = (int)(rate + 0.5);
#endif

E 21
        if (rate > sendRate) {
            sendRate = rate;
D 3
            pProg->msrPn = i;
E 3
I 3
            pProg->msrPn = pStat->picNo;
E 3
        }
D 6
        i = fifoIdx(&pProg->statFifo, i, +1);
        if (i == pProg->statFifo.wrIdx)  break;	// last input pic reached
E 6
E 2
    }
I 7

D 11
    printf("\n    PID %d: pic %d-%d, msr %d at pic %d", pProg->pid,
           pProg->outPn, pProg->lookaheadPn, (int)sendRate, pProg->msrPn);

E 11
E 7
I 2
D 6

    pProg->lookaheadPn = pStat->picNo - 1;
E 6
E 2
D 8
    return (int)sendRate;
E 8
I 8
    return sendRate;
E 8
}


I 13
int calcAvailBytes(ProgInfo* pProg)
{
D 18
    int availBytes = 0;
E 18
I 18
    int availBytes = pProg->leftBytes;
E 18
    int pn = pProg->outPn;
I 18
    if (availBytes > 0)  pn++;		// skip over partially sent picture
E 18
D 19

D 14
    if (pProg->leftBytes < 0) {
        availBytes = -pProg->leftBytes;
E 14
I 14
D 18
    if (pProg->leftBytes > 0) {
        availBytes = pProg->leftBytes;
E 14
        pn++;
    }

    while (pn < pProg->inPn)
E 18
I 18
//    while (pn < pProg->inPn)
    while (pn <= pProg->lahPn)
E 19
I 19
D 21
    while (pn < pProg->lahPn)
E 21
I 21
    while (pn <= pProg->lahPn)
E 21
E 19
E 18
D 16
        availBytes += pProg->stat[pn++ % MAX_PICS_PER_PROG].tgtBytes;
E 16
I 16
        availBytes += getPicStat(pProg, pn++)->tgtBytes;
E 16
D 19

E 19
    return availBytes;
}


E 13
D 2
int estTargetSize(PicStat* stat, int outQs)
E 2
I 2
int estTargetSize(PicStat* pStat, int outQs)
E 2
{
I 17
D 18
#if 0
E 18
I 18
D 21
#if 1
E 21
I 21
#if 0
E 21
E 18
E 17
D 2
    int deltaQs = outQs - stat->avgQs;
    if (deltaQs<=0)  return stat->origSz;
E 2
I 2
    float deltaQs = outQs - pStat->avgQs;
D 11
    if (deltaQs<=0)  return pStat->origSz;
E 11
I 11
    if (deltaQs<=0)  return pStat->origBytes;
E 11
E 2
D 3
    if (deltaQs>= 8)  return 0;
D 2
    return ((8 - deltaQs) * stat->origSz) >> 3;
E 2
I 2
    return ((int)((8 - deltaQs) * pStat->origSz)) >> 3;
E 3
I 3
    if (deltaQs>= 16)  return 0;
D 11
    return (int)((16 - deltaQs) * pStat->origSz / 16);
E 11
I 11
    return (int)((16 - deltaQs) * pStat->origBytes / 16);
I 17
#else
    return (int)((float)pStat->origBytes * pStat->avgQs / outQs);
#endif
E 17
E 11
E 3
E 2
}


D 2
void rcProg(ProgInfo* prog, int outQs)
E 2
I 2
D 8
void rateConvDecProg(ProgInfo* pProg, int outQs)
E 8
I 8
void rateReduceProg(ProgInfo* pProg, int outQs)
E 8
E 2
{
D 2
    int i = prog->outPicId;
E 2
I 2
D 6
    int i = pProg->statFifo.rdIdx;
E 2

    while (1) {
D 2
        PicStat* stat = prog->stat[i];
        if (stat->dts > schEndTime)
            break;
E 2
I 2
        PicStat* pStat = pProg->stat[i];
        if (pStat->dts > lookaheadEndTime)  break;
E 6
I 6
    int pn = pProg->outPn;
D 11
    if (pProg->leftSz < 0)  pn++;
E 11
I 11
D 14
    if (pProg->leftBytes < 0)  pn++;
E 14
I 14
    if (pProg->leftBytes > 0)  pn++;
E 14
E 11
D 13
    while (pn <= pProg->lookaheadPn) {
E 13
I 13
    while (pn < pProg->inPn) {
E 13
D 16
        PicStat* pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 16
I 16
        PicStat* pStat = getPicStat(pProg, pn++);
E 16
E 6
D 11
        pStat->tgtSz = estTargetSize(pStat, outQs);
E 11
I 11
D 21
        pStat->tgtBytes = estTargetSize(pStat, outQs);
E 21
I 21
        pStat->tgtBytes = outQs > pStat->avgQs?
                              estTargetSize(pStat, outQs) : pStat->origBytes;
E 21
E 11
D 6
        i = fifoIdx(&pProg->statFifo, i, +1);
        if (i == pProg->statFifo.wrIdx)  break;	// last input pic reached
E 6
    }
}
E 2

D 2
        stat->tgtSz = estTargetSize(stat, outQs);
E 2

I 18
D 19
void tableUpdate(ProgInfo* pProg)
E 19
I 19
void tableUpdate(ProgInfo *pProg)
E 19
{
D 19
    PicStat* pStat;
    int allocBytes = pProg->allocTp * TP_BYTES;
E 19
I 19
    PicStat *pStat;
    int allocBytes = pProg->allocTp * TP_BYTES;    // remaining allocated bytes
E 19

D 19
    if (pProg->availBytes < 0) {
        printf("\n\nAvailBytes %d negative!", pProg->availBytes);
        exit (-1);
    }

    if (pProg->availBytes == 0) {
E 19
I 19
    // Check to see if all pictures of the program has been sent
    if (pProg->outPn == pProg->inPn) {
        // No more picture to send
E 19
        pProg->stuffBytes += allocBytes;
I 20
D 21
printf("\n** Stufffing!!!");
E 21
I 21
        printf("\n** Stuffing!!!");
E 21
E 20
        return;
    }

D 19
    pProg->leftBytes -= allocBytes;
    pProg->availBytes -= allocBytes;

    while (pProg->leftBytes <= 0) {
        if (-pProg->leftBytes < allocBytes) {
            // Finish with current picture
            getPicStat(pProg, pProg->outPn++)->dptTime = curTime;
            printf("\n    Pic %d sent, next pic sz %d, left %d",
                pStat->picNo-1, pStat->tgtBytes, pProg->leftBytes);
        }

        if (pProg->leftBytes==0 || pProg->availBytes <= 0)  break;

E 19
I 19
    // See if we are right at a picture boundary
    if (pProg->leftBytes == 0) {
        // Start a new picture to output
E 19
        pStat = getPicStat(pProg, pProg->outPn);
D 19
        pProg->leftBytes += pStat->tgtBytes;
        pProg->availBytes -= pStat->tgtBytes;
E 19
I 19
        pProg->leftBytes = pStat->tgtBytes;
E 19
        if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
    }

D 19
    if (pProg->leftBytes < 0) {
        pProg->stuffBytes += -pProg->leftBytes;
        pProg->leftBytes = 0;
E 19
I 19
    while (allocBytes > 0) {
        int usedBytes = min(allocBytes, pProg->leftBytes);
        allocBytes -= usedBytes;
        pProg->leftBytes -= usedBytes;
I 20
        pProg->vbvLevel += usedBytes << 3;
E 20

        // Check for completion of a picture
        if (pProg->leftBytes == 0) {
            pStat = getPicStat(pProg, pProg->outPn);
            pStat->dptTime = curTime;
            printf("\n    Pic %d-%d sent, tgtSz %d, left %d",
                pStat->pid, pStat->picNo, pStat->tgtBytes, pProg->leftBytes);

            pProg->outPn++;             // advance output pointer

            if (pProg->outPn == pProg->inPn) {
                // No more picture to send
                pProg->stuffBytes += allocBytes;
I 20
D 21
printf("\n** Stufffing!!!");
E 21
I 21
                printf("\n** Stuffing!!!");
E 21
E 20
                return;
            }

            if (allocBytes > 0) {
                // Start a new picture to output
                pStat = getPicStat(pProg, pProg->outPn);
                pProg->leftBytes = pStat->tgtBytes;
                if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
            }
        }
E 19
    }
}


I 21
void printMinSendRate()
{
    int i;
    float totRate = 0;

    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        PicStat* pStat = getPicStat(pProg, pProg->msrPn);
        int availBytes = calcAvailBytes(pProg);
        printf("\n  PID %d: avg %d, msr %d (Pic %d, AP %d, delta %d)",
            pProg->pid, (int)(27000000.*8*availBytes/lahWinSz),
            (int)pProg->rate, pProg->msrPn, pStat->accSz, pStat->delta);
        totRate += pProg->rate;
    }
    printf("\n  Total rate = %d\n", (int)totRate);
}


E 21
E 18
D 2
        if (stat->picNo == prog->inPicCnt-1)
            break;		// last input picture reached
E 2
I 2
D 8
void pktSchProg(ProgInfo* pProg)
E 8
I 8
D 11
void scheduleProg(ProgInfo* pProg)
E 8
{
D 6
    int idx;
I 3
    PicStat* pStat;
E 6
I 6
    int pn = pProg->outPn;
E 6
I 4
    int schBytes = pProg->schTp * 188;
I 6
    PicStat* pStat;
E 6
E 4
E 3
E 2

D 2
        if (++i >= MAX_PICS_PER_PROG)  i = 0;
E 2
I 2
D 4
    if (pProg->leftSz > 0)  pProg->leftSz = 0;		// assumed stuffed
    if (pProg->leftSz < 0) {
E 4
I 4
    if (pProg->leftSz >= 0)
        pProg->leftSz = schBytes;		// assumed stuffed

    else {
        pProg->leftSz += schBytes;
D 6

E 4
        idx = fifoIdx(&pProg->statFifo, pProg->statFifo.rdIdx, -1);
D 4
        pProg->stat[idx]->used = 0;
E 4
I 4
        pStat = pProg->stat[idx];

E 6
I 6
        pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 6
        pStat->dptTime = curTime
            + ((float)(schBytes-pProg->leftSz)) * schWinSz / schBytes;
D 6

E 6
D 8
        fprintf(outFp, "\n%d \t %d \t%d \t%d \t%.1f \t%08x \t%08x",
            pStat->pid, pStat->picNo, pStat->picType, pStat->tgtSz,
            (pStat->avgQs>outQs? pStat->avgQs:outQs), pStat->dts,
            pStat->dptTime);
E 8
D 6

E 6
I 6
        pProg->outPn++;
E 6
D 8
        pStat->used = 0;
E 8
E 4
E 2
    }
I 2

D 4
    pProg->leftSz += pProg->schTp * 188;
E 4
D 3
    printf("\n  Sch PID %d: pic %d",
           pProg->pid, pProg->stat[pProg->statFifo.rdIdx]->picNo);
E 3
I 3
D 7
    printf("\n\nPID %d: scheduled %d TP, %d bytes",
D 4
           pProg->pid, pProg->schTp, pProg->schTp*188);
E 4
I 4
           pProg->pid, pProg->schTp, schBytes);
E 7
I 7
    printf("\n    PID %d: %d TP, %d bytes", pProg->pid, pProg->schTp, schBytes);
E 7
E 4
E 3

    while (1) {
D 6
        idx = fifoDelete(&pProg->statFifo);
D 3
        pProg->leftSz -= pProg->stat[idx]->tgtSz;
E 3
I 3
        pStat = pProg->stat[idx];
E 6
I 6
        pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
E 6
        pProg->leftSz -= pStat->tgtSz;
I 4
        if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
E 4
D 6

E 6
D 4
        printf("\n%d \t %d \t%d \t%d \t%.1f \t%08x \t%08x \t%d \t%d \t%d",
            pStat->pid, pStat->picNo, pStat->picType, pStat->tgtSz,
            (pStat->avgQs>outQs? pStat->avgQs:outQs), /* (float)outQs, */
            pStat->dts, 0, 0, 0, 0);
E 4
E 3
        if (pProg->leftSz < 0)  break;
I 4

        pStat->dptTime = curTime
            + ((float)(schBytes-pProg->leftSz)) * schWinSz / schBytes;
D 8

        fprintf(outFp, "\n%d \t %d \t%d \t%d \t%.1f \t%08x \t%08x",
            pStat->pid, pStat->picNo, pStat->picType, pStat->tgtSz,
            (pStat->avgQs>outQs? pStat->avgQs:outQs), pStat->dts,
            pStat->dptTime);

E 8
I 6
        pProg->outPn++;
E 6
E 4
D 3
        pProg->stat[idx]->used = 0;
E 3
I 3
D 8
        pStat->used = 0;
E 8
I 4
D 6

E 4
E 3
        if (fifoFullness(&pProg->statFifo)==0)  break;
E 6
I 6
        if (pn == pProg->inPn)  break;		// last input picture processed
E 6
    }
D 7

D 6
    pProg->schPn = idx;
E 6
D 3
    printf(" - %d, dts %08x, left %d", pProg->stat[pProg->statFifo.rdIdx]->picNo,
           pProg->stat[pProg->statFifo.rdIdx]->dts, pProg->leftSz);
E 3
I 3
    printf("\nLeft %d", pProg->leftSz);
E 7
I 7
    printf(", Pic %d, DTS %08x, %d bytes left",
           pProg->outPn, pStat->dts, -pProg->leftSz);
E 7
E 3
E 2
}


I 8
// Simulate VBV status for decoding of a program
//
void decodeProg(ProgInfo* pProg)
{
    int pn = pProg->decPn;
    int totDecPicBits = 0;
    float rate = pProg->rate / 45000.;
    int vbvLevel = 0;
    PicStat *pStat;

    while (1) {
        pStat = &pProg->stat[pn++ % MAX_PICS_PER_PROG];
        if (pStat->dts > curTime+schWinSz)  break;

        totDecPicBits += pStat->tgtSz << 3;
        vbvLevel = (int)(pProg->vbvLevel + (pStat->dts - curTime) * rate
                             - totDecPicBits);

        fprintf(outFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x \t%d",
            pStat->pid, pStat->picNo, pStat->picType, pStat->tgtSz,
            pStat->avgQs, pStat->dts, pStat->dptTime, vbvLevel);

        pStat->used = 0;
    }

D 9
    printf("\n    PID %d: Pic %d-%d", pProg->pid, pProg->decPn, pStat->picNo);
    pProg->decPn = pStat->picNo;
E 9
    pProg->vbvLevel += schWinSz * rate - totDecPicBits;
I 9
    printf("\n    PID %d: Pic %d-%d, VBV %d",
           pProg->pid, pProg->decPn, pStat->picNo, pProg->vbvLevel);
    pProg->decPn = pStat->picNo;
E 9
}


E 11
E 8
D 6
// Print program info
//
void printProgInfo()
{
    int i, j;
    ProgInfo* pProg;
    PicStat* pStat;

    printf("\nSchedule window: %08x to %08x", schBeginTime, schEndTime);
    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
        printf("\n\nProg %d: PID %d, %d pics", i, pProg->pid, pProg->inPicCnt);
        for (j=0; j<pProg->inPicCnt; j++) {
            pStat = pProg->stat[j];
            printf("\n  Pic %d: arvl %08x, dts %08x",
                   pStat->picNo, pStat->arvlTime, pStat->dts);
        }
    }
}


E 6
int main(int argc, char** argv)
{
    int i, j;
I 2
D 13
    int n = 0;
E 13
E 2
    Param par;
D 4
    char inFile[128];
E 4
I 4
    char inFile[128], outFile[128];
E 4
I 2
    ProgInfo* pProg;
I 6
    PicStat* pStat;
E 6
E 2
D 3
    int minSendRate, totRate, outQs;
E 3
I 3
D 8
    int minSendRate, totRate;
E 8
I 8
    float totRate;
I 11
D 13
    int bitsPerTbl;		// number of bits per table
E 13
I 13
    int bitPerTbl;		// number of bits per table
E 13
D 18
    int tblTime;		// table time in 45 kHz
E 18
I 18
    int tblTime;		// table time in 27 MHz
E 18
D 13
    int tblPerSchWin;		// number of tables per schedule window
E 13
D 12
int tblIdx = 0;
E 12
I 12
    int leftTp;
D 13
    int ovrflMargin;
E 13
I 13
    int vbvMargin;
I 21
    int remainTp, extraTp;
E 21
E 13
    int tblIdx = 0;
I 13
    int schIdx = 0;
D 18
// FILE *repFp;
E 18
E 13
E 12
E 11
E 8
E 3

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "statmux.par");
    getStringParam(&par, "Transport stream statistics filename",
        inFile, "test.stat");
    getIntParam(&par, "Number of video programs", &progCnt, 2);
    prog = malloc(progCnt * sizeof(ProgInfo));
D 6
    for (i=0; i<progCnt; i++) {
        ProgInfo* pProg = &prog[i];
        getIntParam(&par, "Video PID", &pProg->pid, 0);
D 2
        pProg->inPicCnt = pProg->outPicCnt
            = pProg->inPicId = pProg->outPicId = 0;
E 2
I 2
        fifoInit(&pProg->statFifo, MAX_PICS_PER_PROG);
D 3
        pProg->inPicCnt = pProg->outPicCnt = pProg->leftSz = 0;
E 3
I 3
D 4
        pProg->inPicCnt = pProg->outPicCnt = pProg->leftSz
            = pProg->vbvLevel = 0;
E 4
I 4
        pProg->inPicCnt = pProg->outPicCnt = pProg->leftSz = 0;
E 4
E 3
E 2
    }
E 6
I 6
    for (i=0; i<progCnt; i++)
        getIntParam(&par, "Video PID", &prog[i].pid, 0);
E 6
    getIntParam(&par, "Output bitrate (bps)", &outBitrate, 27000000);
D 4
    getIntParam(&par, "Schedule interval (ms)", &schInterval, 250);
    getIntParam(&par, "Lookahead interval (ms)", &lookaheadInterval, 1000);
E 4
I 4
    getIntParam(&par, "Schedule interval (ms)", &schWinSz, 250);
I 13
D 21
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
E 21
E 13
D 18
    getIntParam(&par, "Lookahead interval (ms)", &lookaheadWinSz, 1000);
E 4
    getIntParam(&par, "System delay (ms)", &delay, 0);
E 18
I 18
    getIntParam(&par, "Lookahead interval (ms)", &lahWinSz, 1000);
I 21
    getIntParam(&par, "Prefetch interval (ms)", &prefetchSz, 2000);
E 21
E 18
D 11
    getIntParam(&par, "VBV guard band (bytes)", &vbvMinLevel, 50*188);
E 11
I 11
    getIntParam(&par, "VBV guard band (bytes)", &vbvMinLevel, 50*TP_BYTES);
D 13
    getIntParam(&par, "Table size (TPs)", &tpsPerTbl, 64);
E 13
I 13
    getIntParam(&par, "Table size (TPs)", &tpPerTbl, 64);
E 13
E 11
I 4
    getStringParam(&par, "Output statistics filename", outFile, "stat.out");
E 4
    endParam(&par);

I 13
D 18
// repFp = fopen("rep", "wb");

E 18
E 13
I 11
D 23
    // Initial scheduler
E 23
I 23
    // Initialize scheduler
E 23
D 18
    schEndTime = 0;
    schWinSz *= 45;			// convert to 45 kHz
I 13
    prefetchSz *= 45;
E 13
    lookaheadWinSz *= 45;
    delay *= 45;
E 18
I 18
    schWinSz *= 27000;			// convert to 27 MHz
D 23
    prefetchSz *= 27000;
E 23
    lahWinSz *= 27000;
I 23
    prefetchSz *= 27000;
E 23
    curTime = curTimeExt = 0;
E 18

E 11
D 4
    fp = fopen(inFile, "r");
    if (fp == NULL) {
E 4
I 4
    inFp = fopen(inFile, "r");
    if (inFp == NULL) {
E 4
        printf("Error: Failed to open input stat file %s\n", inFile);
        exit(-1);
    }
D 4
    fgets(line, MAX_LINE_WIDTH, fp);	// skip header line
E 4
I 4
    fgets(line, MAX_LINE_WIDTH, inFp);	// skip header line
E 4

I 2
D 4
    tpPerSchWin = (int)((double)outBitrate * schInterval / (188 * 8 * 1000));
E 4
I 4
    outFp = fopen(outFile, "wb");
    if (outFp == NULL) {
        printf("Error: Failed to open output stat file %s\n", outFile);
        exit(-1);
    }
E 4

I 4
D 11
    tpPerSchWin = (int)((double)outBitrate * schWinSz / (188 * 8 * 1000));
E 11
I 11
    // Adjust schedule window size to contain integer number of tables
I 18
    tpTime = (int)((float)SYS_CLK * TP_BITS / outBitrate + 0.5);
    outBitrate = (int)((float)SYS_CLK * TP_BITS / tpTime + 0.5);
    tblTime = tpTime * tpPerTbl;
E 18
D 13
    bitsPerTbl = tpsPerTbl * TP_BITS;
    tblPerSchWin = (int)((double)outBitrate * schWinSz / (45000. * bitsPerTbl));
    tblTime = (int)(45000. * bitsPerTbl / outBitrate);
E 13
I 13
    bitPerTbl = tpPerTbl * TP_BITS;
D 18
    tblPerSchWin = (int)((double)outBitrate * schWinSz / (45000. * bitPerTbl));
    tblTime = (int)(45000. * bitPerTbl / outBitrate);
E 18
I 18
    tblPerSchWin = (int)(schWinSz / tblTime + 0.5);
E 18
E 13
    schWinSz = tblTime * tblPerSchWin;
D 13
    outBitrate = (int)(45000. * bitsPerTbl / tblTime);
E 13
I 13
D 18
    outBitrate = (int)(45000. * bitPerTbl / tblTime);
E 18
E 13
E 11

I 23
#if 1
    // Force lookahead window to contain multiples of TPs to make algorithm
    // comparison easy
  {
    int tpPerLahWin = (int)((float)lahWinSz / tpTime + 0.5);
    lahWinSz = tpTime * tpPerLahWin;
  }
#endif

E 23
E 4
D 18
    printf("Output bitrate: %d bps", outBitrate);
D 11
    printf("\nSystem delay: %d ms", delay);
D 4
    printf("\nSchedule window size: %d ms", schInterval);
    printf("\nLookahead window size: %d ms", lookaheadInterval);
E 4
I 4
    printf("\nSchedule window size: %d ms", schWinSz);
    printf("\nLookahead window size: %d ms", lookaheadWinSz);
E 4
    printf("\nTP per schedule window: %d", tpPerSchWin);
E 11
I 11
    printf("\nSystem delay: %d ms", delay/45);
    printf("\nSchedule window size: %d ms", schWinSz/45);
    printf("\nLookahead window size: %d ms", lookaheadWinSz/45);
E 18
I 18
    printf("\nTP time: %d (in 27 MHz)", tpTime);
    printf("\nOutput bitrate: %d bps", outBitrate);
E 18
D 13
    printf("\nTP per table: %d", tpsPerTbl);
E 13
    printf("\nTables per schdule window: %d", tblPerSchWin);
I 13
D 18
    printf("\nTP: %d per table, %d per schedule window",
           tpPerTbl, tpPerTbl*tblPerSchWin);
E 18
I 18
    printf("\nSchedule window: %d ms, %d tables, %d TPs",
           schWinSz/27000, tblPerSchWin, tpPerTbl*tblPerSchWin);
    printf("\nLookahead window size: %d ms", lahWinSz/27000);
E 18
E 13
E 11

E 2
D 11
    // Initial scheduler
    schEndTime = 0;
D 4
    schInterval *= 45;			// convert to 45 kHz
    lookaheadInterval *= 45;		// convert to 45 kHz
E 4
I 4
    schWinSz *= 45;			// convert to 45 kHz
    lookaheadWinSz *= 45;		// convert to 45 kHz
E 4

E 11
D 6
    // Allocate memory for PicStat buffer
    numStat = progCnt * MAX_PICS_PER_PROG;
    stat = malloc(numStat * sizeof(PicStat));
D 2
    inId = outId = 0;
E 2
I 2
    memset(stat, 0, numStat * sizeof(PicStat));
    fifoInit(&statFifo, numStat);
E 6
I 6
    // Initial programs
    for (i=0; i<progCnt; i++) {
        pProg = &prog[i];
        pProg->stat = malloc(MAX_PICS_PER_PROG * sizeof(PicStat));
        memset(pProg->stat, 0, MAX_PICS_PER_PROG * sizeof(PicStat));
D 8
        pProg->inPn = pProg->outPn = 0;
        pProg->leftSz = 0;
E 8
I 8
        pProg->inPn = pProg->outPn = pProg->decPn = 0;
D 11
        pProg->vbvLevel = pProg->leftSz = 0;
E 11
I 11
        pProg->vbvLevel = pProg->leftBytes = 0;
E 11
E 8
    }
E 6
E 2

D 2
    // Update schedule window
    schBeginTime = schEndTime;
    schEndTime = schBeginTime + schInterval;
    lookaheadEndTime = schBeginTime + lookaheadInterval;
    curTime = schBeginTime - delay;
E 2
D 6

E 6
D 2
    tpPerSchWin = (int)((double)outBitrate * schInterval / (188 * 8 * 1000));

    // Collect PicStats for next schedule period
E 2
    while (1) {
D 2
        ProgInfo* pProg;
        PicStat*  pStat = &stat[inId];
E 2
I 2
D 6
//    for (n=0; n<4; n++) {
E 6
        // Update schedule window
D 18
        schBeginTime = schEndTime;
D 4
        schEndTime = schBeginTime + schInterval;
        lookaheadEndTime = schBeginTime + lookaheadInterval;
E 4
I 4
D 8
        schEndTime = schBeginTime + schWinSz;
E 8
I 8
        schEndTime += schWinSz;
I 13
        prefetchTime = schBeginTime + prefetchSz;
E 13
E 8
D 6
        lookaheadEndTime = schBeginTime + lookaheadWinSz;
E 6
I 6
        lookaheadTime = schBeginTime + lookaheadWinSz;
E 6
E 4
        curTime = schBeginTime - delay;
E 18
I 18
        schTime = curTime + (curTimeExt + schWinSz) / 600;
        prefetchTime = curTime + (curTimeExt + prefetchSz) / 600;
        lahTime = curTime + (curTimeExt + lahWinSz) / 600;
E 18
E 2

D 2
        if (fgets(line, MAX_LINE_WIDTH, fp) == NULL) {
            printf("\nEnd of stat file\n");
            return;
        }
        sscanf(line, "%d %d %d %d %f %x %x",
            &pStat->pid, &pStat->picNo, &pStat->picType, &pStat->origSz,
            &pStat->avgQs, &pStat->dts, &pStat->arvlTime);
        pStat->tgtSz = pStat->origSz;
E 2
I 2
        printf("\n\n---------------------------------------------------------------------------");
D 7
        printf("\n\nSchedule period %d: PCR %08x, begin %08x, end %08x, lookahead %08x",
D 6
           n++, curTime, schBeginTime, schEndTime, lookaheadEndTime);
E 6
I 6
           n++, curTime, schBeginTime, schEndTime, lookaheadTime);
E 7
I 7
D 13
        printf("\n\nSchedule period %d:", n++);
E 13
I 13
D 18
        printf("\n\nSchedule period %d:", schIdx);
E 13
        printf("\n  PCR %08x, begin %08x, end %08x, lookahead %08x",
               curTime, schBeginTime, schEndTime, lookaheadTime);
E 18
I 18
        printf("\n\nPeriod %d: current %08x, schedule %08x, lookahead %08x",
               schIdx, curTime, schTime, lahTime);
E 18
E 7
E 6
E 2

D 2
        j = findProg(pStat->pid);
        if (j == -1)  continue;
E 2
I 2
D 18
        // Collect PicStats for next schedule period
E 18
I 18
D 19
        // Collect picture statistics
        //
E 18
        while (1) {
D 6
            ProgInfo* pProg;
            PicStat*  pStat = &stat[statFifo.wrIdx];
            if (pStat->used) {
                printf("\nError: PicStat still being used (%d)!", pStat->used);
                exit (-1);
            }
E 6
I 6
D 18
            int pid, progIdx;
E 18
I 18
            int pid;
E 19
I 19
        collectPicStat(prefetchTime);
E 19
E 18
E 6
E 2

I 20
        printf("\n\nStat collection:");
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->lahPn = dts2Pn(pProg, lahTime);
            printf("\n  PID %d: in %d, lah %d, out %d, dec %d", pProg->pid,
                pProg->inPn, pProg->lahPn, pProg->outPn, pProg->decPn);
        }

E 20
D 2
        pProg = &prog[j];
        pProg->stat[pProg->inPicId] = pStat;
        pProg->inPicCnt++;
        if (++pProg->inPicId == MAX_PICS_PER_PROG)  pProg->inPicId = 0;
E 2
I 2
D 4
            if (fgets(line, MAX_LINE_WIDTH, fp) == NULL) {
E 4
I 4
D 19
            if (fgets(line, MAX_LINE_WIDTH, inFp) == NULL) {
E 4
                printf("\nEnd of stat file\n");
                return;
            }
I 6
            sscanf(line, "%d", &pid);

            // Map program from PID
D 18
            progIdx = findProg(pid);
            if (progIdx == -1)  continue;
E 18
I 18
            pProg = pid2Prog(pid);
            if (pProg == NULL)  continue;
E 18

D 18
            pProg = &prog[progIdx];
E 18
D 16
            pStat = &pProg->stat[pProg->inPn % MAX_PICS_PER_PROG];
E 16
I 16
            pStat = getPicStat(pProg, pProg->inPn);
E 16
            if (pStat->used) {
                printf("\nError: PicStat still being used (%d)!", pStat->used);
                exit (-1);
            }

E 6
            sscanf(line, "%d %d %d %d %f %x %x",
D 11
                &pStat->pid, &pStat->picNo, &pStat->picType, &pStat->origSz,
E 11
I 11
                &pStat->pid, &pStat->picNo, &pStat->picType, &pStat->origBytes,
E 11
                &pStat->avgQs, &pStat->dts, &pStat->arvlTime);
I 6
            pStat->used = 1;
E 6
D 11
            pStat->tgtSz = pStat->origSz;
E 11
I 11
D 15
            pStat->tgtBytes = pStat->origBytes;
E 15

I 16
#if 0
            printf("\n  Pic %d-%d: ATS %08x, DTS %08x, sz %d", pProg->pid,
                pStat->picNo, pStat->arvlTime, pStat->dts, pStat->origBytes);
#endif

E 16
            if (!pProg->inPn)  pProg->decDts = pStat->dts;
E 11
I 6
            pProg->inPn++;
E 6
E 2

D 2
       if (++inId == numStat)  inId = 0;
E 2
I 2
D 6
            j = findProg(pStat->pid);
            if (j == -1)  continue;
E 2

D 2
       if (pStat->arvlTime > schEndTime)  break;
    }
E 2
I 2
            pStat->used = 1;
            pProg = &prog[j];
            pProg->stat[fifoAdd(&pProg->statFifo)] = pStat;
            pProg->inPicCnt++;
E 2

D 2
    // Compute minimum send rates
    totRate = 0;
    for (i=0; i<progCnt; i++) {
        prog[i].rate = calcMinSendRate(&prog[i]);
        totRate += prog[i].rate;
        printf("\nPID %d: %d pics, msr %d",
               prog[i].pid, prog[i].inPicCnt, prog[i].rate);
    }
    printf("\nTotal rate = %d", totRate);
E 2
I 2
            fifoAdd(&statFifo);
            if (pStat->arvlTime > lookaheadEndTime)  break;
E 6
I 6
D 13
            if (pStat->arvlTime > lookaheadTime)  break;
E 13
I 13
            if (pStat->arvlTime > prefetchTime)  break;
E 13
			// Note: we assume pictures in the input stat file
			//       are sorted by arrival time
E 6
        }
E 2

E 19
D 2
    // Rate conversion decision
    if (totRate > outBitrate) {
        for (outQs=2; outQs<=20; outQs+=2) {
            printf("\n\nRC: outQs = %d", outQs);
            totRate = 0;
            for (i=0; i<progCnt; i++) {
                rcProg(&prog[i], outQs);
                prog[i].rate = calcMinSendRate(&prog[i]);
                totRate += prog[i].rate;
                printf("\nPID %d: %d pics, msr %d",
                       prog[i].pid, prog[i].inPicCnt, prog[i].rate);
E 2
I 2
D 15
        // Compute minimum send rates
E 15
I 15
D 22
        // Compute minimum send rates (no RC)
E 15
I 3
        outQs = 0;
E 3
D 8
        totRate = 0;
E 8
I 8
        totRate = 0.;
I 20
D 21
        printf("\n\nMSR:");
E 21
I 21
        totSchTp = 0;
E 21
E 20
E 8
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
I 6
D 18
            pProg->lookaheadPn = findLastDtsPn(pProg, lookaheadTime);
E 18
I 18
D 20
            pProg->lahPn = findLastDtsPn(pProg, lahTime);
E 20
I 20
            pProg->lahPn = dts2Pn(pProg, lahTime);
E 20
E 18
I 15
            resetTargetSize(pProg);
E 15
E 6
            pProg->rate = calcMinSendRate(pProg);
I 11
D 20
            printf("\n    PID %d: pic %d-%d, msr %d at pic %d",
D 18
                pProg->pid, pProg->outPn, pProg->lookaheadPn,
E 18
I 18
                pProg->pid, pProg->outPn, pProg->lahPn,
E 18
                (int)pProg->rate, pProg->msrPn);
E 20
I 20
D 21
            printf("\n  PID %d: msr %d - %d",
                pProg->pid, pProg->msrPn, (int)pProg->rate);
E 20
I 15
#if 0
D 16
            pStat = &pProg->stat[pProg->msrPn % MAX_PICS_PER_PROG];
E 16
I 16
            pStat = getPicStat(pProg, pProg->msrPn);
E 16
            printf(" (orig %d, tgt %d)", pStat->origBytes, pStat->tgtBytes);
#endif
E 21
I 21
            pProg->schTp = (pProg->rate * tpPerTbl + outBitrate-1) / outBitrate;
E 21
E 15
E 11
            totRate += pProg->rate;
I 21
            totSchTp += pProg->schTp;
E 21
D 7
            printf("\n  PID %d: pic %d - %d, msr %d - %d", pProg->pid,
D 6
                   pProg->stat[pProg->statFifo.rdIdx]->picNo, 
                   pProg->lookaheadPn, pProg->msrPn, pProg->rate);
E 6
I 6
                pProg->outPn, pProg->lookaheadPn, pProg->msrPn, pProg->rate);
E 7
E 6
        }
D 8
        printf("\n  Total rate = %d", totRate);
E 8
I 8
D 21
        printf("\n  Total rate = %d", (int)totRate);
E 21
I 21
        remainTp = tpPerTbl - totSchTp;
E 22
I 22
        // Rate conversion decision
        for (outQs=0; outQs<=30; outQs++) {
            totRate = 0.;
            totSchTp = 0;
            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
                rateReduceProg(pProg, outQs);
                pProg->rate = calcMinSendRate(pProg);
                pProg->schTp = (pProg->rate * tpPerTbl + outBitrate - 1)
                                  / outBitrate;
                totRate += pProg->rate;
                totSchTp += pProg->schTp;
            }
            remainTp = tpPerTbl - totSchTp;
E 22
E 21
E 8

I 21
D 22
        printf("\n  No RC: total rate %d, %d TP/Tbl", (int)totRate, totSchTp);
        printf("\n\nMSR (no RC):");
        printMinSendRate();
E 22
I 22
            printf("\n  RC: outQs %d, rate %d, %d TP/Tbl",
                   outQs, (int)totRate, totSchTp);
E 22

E 21
D 22
        // Rate conversion decision
D 21
        if (totRate > outBitrate) {
D 11
            printf("\n\nRate conversion decision:");
E 11
D 5
            for (outQs=2; outQs<=20; outQs+=2) {
E 5
I 5
            for (outQs=2; outQs<=20; outQs++) {
E 21
I 21
        if (remainTp < 0) {
            for (outQs=2; outQs<=30; outQs++) {
E 21
E 5
D 12
                printf("\n\n  RC: outQs = %d", outQs);
E 12
D 8
                totRate = 0;
E 8
I 8
                totRate = 0.;
I 21
                totSchTp = 0;
E 21
E 8
                for (i=0; i<progCnt; i++) {
                    pProg = &prog[i];
D 8
                    rateConvDecProg(pProg, outQs);
E 8
I 8
                    rateReduceProg(pProg, outQs);
E 8
                    pProg->rate = calcMinSendRate(pProg);
I 15
D 21
#if 0
                    printf("\n    PID %d: pic %d-%d, msr %d at pic %d",
D 18
                        pProg->pid, pProg->outPn, pProg->lookaheadPn,
E 18
I 18
                        pProg->pid, pProg->outPn, pProg->lahPn,
E 18
                        (int)pProg->rate, pProg->msrPn);
D 16
                    pStat = &pProg->stat[pProg->msrPn % MAX_PICS_PER_PROG];
E 16
I 16
                    pStat = getPicStat(pProg, pProg->msrPn);
E 16
                    printf(" (orig %d, tgt %d)",
                           pStat->origBytes, pStat->tgtBytes);
#endif
E 21
I 21
                    pProg->schTp = (pProg->rate * tpPerTbl + outBitrate - 1)
                                      / outBitrate;
E 21
E 15
                    totRate += pProg->rate;
I 21
                    totSchTp += pProg->schTp;
E 21
D 7
                    printf("\n  PID %d: pic %d - %d, msr %d - %d", pProg->pid,
D 6
                           pProg->stat[pProg->statFifo.rdIdx]->picNo, 
                           pProg->lookaheadPn, pProg->msrPn, pProg->rate);
E 6
I 6
                        pProg->outPn, pProg->lookaheadPn, pProg->msrPn,
                        pProg->rate);
E 7
E 6
                }
D 10
                printf("\n  Total rate = %d", totRate);
E 10
I 10
D 12
                printf("\n  Total rate = %d", (int)totRate);
E 12
I 12
D 13
                printf("\n  RC: outQs %d, rate %d", outQs, (int)totRate);
E 13
I 13
D 21
                printf("\n  RC: outQs %d, Total rate %d", outQs, (int)totRate);
E 13
E 12
E 10
                if (totRate <= outBitrate)  break;
E 21
I 21
                printf("\n  RC: outQs %d, total rate %d, %d TP/Tbl",
                       outQs, (int)totRate, totSchTp);
                remainTp = tpPerTbl - totSchTp;
                if (remainTp >= 0)  break;
E 22
I 22
            if (remainTp >= 0 || outQs == 0) {
                printf("\n\nMSR (outQs %d):", outQs);
                printMinSendRate();
E 22
E 21
E 2
            }
I 11
D 13
            printf("\n\nRate conversion decision: QS %d", outQs);
E 13
I 13
D 21
            printf("\n\nRC decision: QS %d", outQs);
E 21
I 21
D 22
            printf("\nRC decision: QS %d", outQs);
            printf("\n\nMSR (RC):");
            printMinSendRate();
E 22
I 22

            if (remainTp >= 0)  break;
E 22
E 21
E 13
E 11
D 2
            printf("\nTotal rate = %d", totRate);
            if (totRate <= outBitrate)  break;
E 2
        }
I 4
D 13
        else printf("\nNo RC");
E 13
I 13
D 22
        else printf("\nRC decision: No");
E 22
I 22
        printf("\nRC decision: QS %d", outQs);
E 22
E 13
E 4

D 8
        // Packet allocation
E 8
I 8
D 11
        // Scheduling
E 8
I 2
D 7
        printf("\n\nPacket scheduling:");
E 7
I 7
        printf("\n\nScheduling:");
E 11
E 7
E 2
D 21
        totSchTp = 0;
E 21
I 21
        // Distribute the remaining TPs
D 22
        printf("\nRemaining TP: %d", remainTp);
E 22
        extraTp = (remainTp + progCnt - 1) / progCnt;
E 21
        for (i=0; i<progCnt; i++) {
I 21
            int n;
E 21
D 2
            prog[i].schTp = (int)((double)tpPerSchWin * prog[i].rate / totRate);
            totSchTp += prog[i].schTp;
E 2
I 2
            pProg = &prog[i];
I 8
D 9
            pProg->rate *= outBitrate / totRate;
E 8
            pProg->schTp = (int)((double)tpPerSchWin * pProg->rate / totRate);
E 9
I 9
D 11
            pProg->schTp = (int)((double)tpPerSchWin * pProg->rate / totRate
                                 + 0.5);
E 11
I 11
D 13
            pProg->schTp = (int)(tpsPerTbl * pProg->rate / totRate + 0.5);
E 13
I 13
D 21
            pProg->schTp = (int)(tpPerTbl * pProg->rate / totRate + 0.5);
E 13
E 11
E 9
            totSchTp += pProg->schTp;
E 21
I 13
            pProg->availBytes = calcAvailBytes(pProg);
D 21
            printf("\n  PID %d: avail %d", pProg->pid, pProg->availBytes);
E 21
I 20
            pProg->usedTp = 0;
I 21

            n = min(extraTp, remainTp);
            pProg->schTp += n;
            remainTp -= n;
E 21
E 20
E 13
I 9
D 11
            pProg->rate *= outBitrate / totRate;
E 9
D 8
            pktSchProg(pProg);
E 8
I 8
            scheduleProg(pProg);
E 11
E 8
E 2
        }
D 2
        printf("\nDifference TP: %d",  tpPerSchWin - totSchTp);
E 2
I 2
D 11
        printf("\n  Difference TP: %d",  tpPerSchWin - totSchTp);
E 11
I 11
D 13
        pProg->schTp += tpsPerTbl - totSchTp;
E 13
I 13
D 21
        pProg->schTp += tpPerTbl - totSchTp;
E 13
		    // adjust last channel's assignment for rounding error
E 21
I 21
D 22
        printf(" -> %d", remainTp);
E 22
E 21
E 11
I 8

I 21
D 22
        printf("\nPacket allocation:");
E 22
I 22
        printf("\n\nPacket allocation:");
E 22
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
            pProg->actRate = (float)pProg->schTp * outBitrate / tpPerTbl;
            printf("\n  PID %d: %d TP, rate %d, msr %d", pProg->pid,
                pProg->schTp, (int)pProg->actRate, (int)pProg->rate);
        }

E 21
D 10
totRate = 0;
for (i=0; i<progCnt; i++)
  totRate += prog[i].rate;
printf("\n## total rate %.2f", totRate);

E 10
D 11
        // Decoding
        printf("\n\nDecoding:");
E 11
I 11
        // Simulate VBV
D 13
        for (curTime+=tblTime; curTime<=schEndTime-delay; curTime+=tblTime, tblIdx++) {
D 12
//            printf("\nTbl %d: %08x", tblIdx, curTime);
E 12
I 12
            printf("\nTbl %d: %08x", tblIdx, curTime);
E 13
I 13
D 18
        for (j=0; j<tblPerSchWin; j++, tblIdx++, curTime+=tblTime) {
E 18
I 18
        for (j=0; j<tblPerSchWin; j++) {
E 18
D 20
            printf("\n\nTbl %d: %08x", tblIdx, curTime);
E 20
I 20
            // Advance current time
            curTimeExt += tblTime;
            curTime += curTimeExt / 600;
            curTimeExt %= 600;
            printf("\n\nTbl %d: %08x", tblIdx++, curTime);
E 20
E 13
E 12

I 12
            // Actual TP allocation
D 13
            leftTp = tpsPerTbl;
E 13
I 13
            leftTp = tpPerTbl;
E 13
E 12
            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
D 12
                if (!tblIdx)
E 12
I 12
D 13
#if 0
                ovrflMargin = (VBV_SZ - pProg->vbvLevel) / TP_BITS;
if (ovrflMargin < pProg->schTp)
  printf("\n  VBV Margin %d, sch TP %d", ovrflMargin, pProg->schTp);
                pProg->allocTp = pProg->schTp < ovrflMargin?
                                     pProg->schTp : ovrflMargin;
E 13
I 13
                vbvMargin = (VBV_SZ - pProg->vbvLevel) / TP_BITS;
                pProg->allocTp = min(pProg->schTp, vbvMargin);
                pProg->allocTp = min(pProg->allocTp,
                                         pProg->availBytes/TP_BYTES);
                if (pProg->allocTp != pProg->schTp)
                    printf("\n    PID %d: sch %d, VBV margin %d, avail %d",
                        pProg->pid, pProg->schTp, vbvMargin,
                        pProg->availBytes/TP_BYTES);
E 13
                leftTp -= pProg->allocTp;
D 13
#else
  pProg->allocTp = pProg->schTp;
  leftTp -= pProg->allocTp;
#endif
E 13
I 13
D 18
                pProg->availBytes -= pProg->allocTp * TP_BYTES;
E 18
E 13
            }
I 13
            if (leftTp)  printf("\n  Leftover TP: %d", leftTp);
E 13

D 13
            // Distribute leftover TPs
            // (TBD)
            if (leftTp)  printf("\nLeftover TP: %d", leftTp);

E 13
            for (i=0; i<progCnt; i++) {
                pProg = &prog[i];
D 13
                if (!tblIdx) {
E 13
I 13
D 18
                if (!tblIdx)
E 13
E 12
                    pProg->leftBytes = pProg->stat[0].tgtBytes;
E 18
I 18
                tableUpdate(pProg);
E 18
I 12
D 13
                    if (pProg->stat[0].avgQs < outQs)
                        pProg->stat[0].avgQs = outQs;
                }
E 13
E 12

D 12
                pProg->vbvLevel += pProg->schTp * TP_BITS;
                pProg->leftBytes -= pProg->schTp * TP_BYTES;
E 12
I 12
D 15
                pProg->vbvLevel += pProg->allocTp * TP_BITS;
E 15
D 18
                pProg->leftBytes -= pProg->allocTp * TP_BYTES;
E 18
I 15
D 20
                pProg->vbvLevel += pProg->allocTp * TP_BITS;
E 20
I 20
                pProg->usedTp += pProg->allocTp - pProg->stuffBytes / TP_BYTES;

E 20
                if (pProg->vbvLevel > VBV_SZ) {
                    printf("\nVBV overflow!");
D 20
                    exit (-1);
E 20
I 20
//                    exit (-1);
E 20
                }
E 15
E 12

D 12
//                printf("\n  PID %d: TP %d, vbv %d, left %d", pProg->pid,
//                       pProg->schTp, pProg->vbvLevel, pProg->leftBytes);
E 12
I 12
D 13
                printf("\n  PID %d: TP %d, vbv %d, left %d", pProg->pid,
                       pProg->allocTp, pProg->vbvLevel, pProg->leftBytes);
E 13
I 13
D 20
                printf("\n  PID %d: TP %d, vbv %d, left %d, avail %d",
E 20
I 20
                printf("\n  PID %d: TP %d, vbv %d, left %d, avail %d, stuff %d",
E 20
                    pProg->pid, pProg->allocTp, pProg->vbvLevel,
D 20
                    pProg->leftBytes, pProg->availBytes);
E 20
I 20
                    pProg->leftBytes, pProg->availBytes, pProg->stuffBytes);
E 20
E 13
E 12

D 13
                while (pProg->leftBytes < 0) {
E 13
I 13
D 18
                while (pProg->leftBytes < 0
D 15
                           && pProg->outPn < pProg->inPn-1) {
E 15
I 15
                           && pProg->outPn < pProg->inPn) {
E 15
E 13
D 16
                    pStat = &pProg->stat[pProg->outPn++ % MAX_PICS_PER_PROG];
E 16
I 16
                    pStat = getPicStat(pProg, pProg->outPn++);
E 16
                    pStat->dptTime = curTime;	// Note: not quite accurate!

D 16
                    pStat = &pProg->stat[pProg->outPn % MAX_PICS_PER_PROG];
E 16
I 16
                    pStat = getPicStat(pProg, pProg->outPn);
E 16
I 12
D 13
                    if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
E 13
E 12
                    pProg->leftBytes += pStat->tgtBytes;
I 15
                    if (pStat->avgQs < outQs)  pStat->avgQs = outQs;
E 15

D 12
//                    printf("\n    Pic %d sent, next pic sz %d, left %d",
//                           pStat->picNo-1, pStat->tgtBytes, pProg->leftBytes);
E 12
I 12
                    printf("\n    Pic %d sent, next pic sz %d, left %d",
                        pStat->picNo-1, pStat->tgtBytes, pProg->leftBytes);
E 12
                }

E 18
                if (curTime >= pProg->decDts) {
D 16
                    pStat = &pProg->stat[pProg->decPn++ % MAX_PICS_PER_PROG];
E 16
I 16
                    pStat = getPicStat(pProg, pProg->decPn++);
E 16
                    pProg->vbvLevel -= pStat->tgtBytes << 3;
D 12
//                    printf("\n    Pic %d decoded, DTS %08x, sz %d, vbv %d",
//                        pStat->picNo, pProg->decDts, pStat->tgtBytes,
//                        pProg->vbvLevel);
E 12
I 12
                    printf("\n    Pic %d decoded, DTS %08x, sz %d, vbv %d",
                        pStat->picNo, pProg->decDts, pStat->tgtBytes,
                        pProg->vbvLevel);
E 12
                    pStat->used = 0;

I 15
                    if (pProg->vbvLevel < 0) {
                        printf("\nVBV underflow!");
I 21
                        printf("\tPic MSR %d, Act rate %.0f",
                            pStat->msr, pProg->actRate);
E 21
//                        exit (-1);
                    }

E 15
D 16
                    pProg->decDts =
                        pProg->stat[pProg->decPn % MAX_PICS_PER_PROG].dts;
E 16
I 16
                    pProg->decDts = getPicStat(pProg, pProg->decPn)->dts;
E 16

                    fprintf(outFp, "\n%d \t%d \t%d \t%d \t%.1f \t%08x \t%08x "\
D 12
                        "\t%d", pStat->pid, pStat->picNo, pStat->picType,
E 12
I 12
D 13
                        "\t%d \t%d \t%d", pStat->pid, pStat->picNo, pStat->picType,
E 13
I 13
                        "\t%d \t%d", pStat->pid, pStat->picNo, pStat->picType,
E 13
E 12
                        pStat->tgtBytes, pStat->avgQs, pStat->dts,
D 12
                        pStat->dptTime, pProg->vbvLevel>>3);
E 12
I 12
                        pStat->dptTime, pProg->vbvLevel>>3,
D 13
                        (pProg->vbvLevel>>3) + pStat->tgtBytes,
                        pProg->outPn-pProg->decPn);
E 13
I 13
                        (pProg->vbvLevel>>3) + pStat->tgtBytes);
E 13
E 12
                }
            }
I 18
D 20

            // Advance current time
            curTimeExt += tblTime;
            curTime += curTimeExt / 600;
            curTimeExt %= 600;
            tblIdx++;
E 20
E 18
        }

D 13
        printf("\n\nScheduling:");
E 13
I 13
        printf("\n\nScheduling %d (%08x):", schIdx++, curTime);
E 13
E 11
        for (i=0; i<progCnt; i++) {
            pProg = &prog[i];
D 11
            decodeProg(pProg);
E 11
I 11
D 13
            printf("\n    PID %d: %d TP, VBV %d, left %d", pProg->pid,
                pProg->schTp, pProg->vbvLevel, pProg->leftBytes);
E 13
I 13
D 20
            printf("\n    PID %d: %d TP, VBV %d, in %d, lah %d, "\
                "send %d:%d (%08x), dec %d", pProg->pid, pProg->schTp,
E 20
I 20
            printf("\n  PID %4d: %d TP, VBV %d, PN: in %d, lah %d, "\
                "send %d:%d, dec %d", pProg->pid, pProg->usedTp,
E 20
D 18
                pProg->vbvLevel, pProg->inPn-1, pProg->lookaheadPn,
E 18
I 18
                pProg->vbvLevel, pProg->inPn-1, pProg->lahPn,
E 18
D 20
                pProg->outPn, pProg->leftBytes,
D 16
                pProg->stat[pProg->outPn % MAX_PICS_PER_PROG].dts,
                pProg->decPn);
E 16
I 16
                getPicStat(pProg, pProg->outPn)->dts, pProg->decPn);
E 20
I 20
                pProg->outPn, pProg->leftBytes, pProg->decPn);
E 20
E 16

I 20
            printf("\n            DTS: out %08x, dec %08x",
                getPicStat(pProg, pProg->outPn)->dts,
                getPicStat(pProg, pProg->decPn)->dts);

E 20
D 15
            if (pProg->outPn > pProg->inPn-1) {
D 14
                printf("\nError!");
E 14
I 14
                printf("\nError 1!");
E 15
I 15
D 19
            if (pProg->outPn >= pProg->inPn) {
E 19
I 19
            if (pProg->outPn > pProg->inPn) {
E 19
                printf("\nSending picutre not already arrived yet!\n");
E 15
E 14
                exit (-1);
            }
I 14
            if (pProg->leftBytes < 0) {
D 15
                printf("\nError 2!");
E 15
I 15
                printf("\nLeftByte negative!\n");
E 15
                exit (-1);
            }
E 14
E 13
E 11
        }
E 8
E 2
    }

    printf("\n");
}

E 1
