// This program is used to study how to do coefficient requantization

#include <stdio.h>
#include <stdlib.h>
#include "param.h"


// Division with truncation using shift
int shftTrunc(int x, int s)
{
    if (x<0)  x += (1<<s) - 1;
    return x >> s;
}


// Division with round down using shift
// (same as rounding, except middle point are rounding toward zero)
int shftRnd2Zero(int x, int s)
{
    int half = 1 << (s-1);
    x += half;
    if (x>0)  x--;
    return x>>s;
}


// Find sign
int sign(int x)
{
    return (x<0)? -1 : ((x==0)? 0 : 1);
}


// TM5 quantization - for intra AC only!
int tm5InterAcQuant(int F, int qs, int W)
{
    int temp = (16*F + sign(F)*W/2) / W;
    return (temp / (2 * qs));
}


// MPEG2 dequantization - intra
int intraDequant(int QF, int qs, int W)
{
    return shftTrunc((QF<<1) * W * qs, 5);
}


// MPEG2 dequantization - inter
int interDequant(int QF, int qs, int W)
{
    int sgn = sign(QF);
    return shftTrunc(((QF<<1) + sgn) * W * qs, 5);
}


// New requant - intra
int intraRequant(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = r * val;
    QF2 = shftRnd2Zero(QF2, scale);

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// New requant - inter
int interRequant(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = (r * val)<<1;
    QF2 += r - (1<<scale);
    QF2 = shftRnd2Zero(QF2, scale+1);

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// Fast rquant - intra
//   Equivalent to New requant, but simplified
int intraRequantFast(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int half = 1 << (scale-1);
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = r * val + half;
    if (QF2)  QF2--;		// Note: QF2 is non negative!
    QF2 >>= scale;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// Fast requant - inter
//   Equivalent to New requant, but simplified
int interRequantFast(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int half = 1 << (scale-1);
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = ((r * val)<<1) + r;
    if (QF2)  QF2--;		// Note: QF2 is non negative!
 				// Note: this seems to have little effect!
    QF2 >>= scale + 1;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// RateMux requant - intra
int intraRequantRM(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = val * r;
//    QF2 += (1<<11);		// new method (qtrScale)
    QF2 += (1<<12);		// old method (halfScale): actually better!
    QF2 >>= scale;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// RateMux requant - inter
int interRequantRM(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
    int val = abs(QF1);

    int QF2 = (val * r) << 1;
    QF2 += r + (1<<12);
    QF2 >>= scale;
    if (QF2)  QF2 = (QF2-1) >> 1;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// This is the simple requant suggested by Cullen Jenning, my patent reviewer!
//
int intraRequantSimple(int QF1, int qs1, int qs2)
{
    double temp;
    double sgn = 0;

    if (QF1>0)  sgn = 0.4999;
    else if (QF1<0)  sgn = -0.4999;
    temp = (double)QF1;
    temp = (temp * qs1/qs2);
    return (int)temp;
}


int interRequantSimple(int QF1, int qs1, int qs2)
{
    double temp;
    double sgn = 0;

    if (QF1>0)  sgn = 0.4999;
    else if (QF1<0)  sgn = -0.4999;
    temp = (double)QF1;
    temp = ((temp + sgn) * qs1/qs2);
    return (int)temp;
}


//----------------------------------------------------------------------------



#if 0
int main ()
{
    int i;
    double t = -2;
    int t1, t2;

    for (i=0; i<50; i++) {
        t1 = (int)t;
        if (t<0)  t2 = (int)(t-0.4999);
        else if (t>0)  t2 = (int)(t+0.4999);
        printf("%g -> %d, %d\n", t, t1, t2);
        t += 0.1;
    }
}
#endif



#if 0
int main(int argc, char** argv)
{
    Param par;
    int intraFlag;
    int qs1, qs2, W;
    int F1, F2, F3, QF1, QF2, QF3;
    int F4, QF4;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "requant.par");
    getIntParam(&par, "Original qscale?", &qs1, 4);
    getIntParam(&par, "New qscale?", &qs2, 8);
    getIntParam(&par, "Quantizer matrix value?", &W, 16);
    getIntParam(&par, "Intra MB?", &intraFlag, 0);
    endParam(&par);

    printf("%s, QS %d => %d, W %d\n", intraFlag?"INTRA":"INTER", qs1, qs2, W);

    for (QF1=-10; QF1<=10; QF1++) {
        if (intraFlag) {
            QF2 = intraRequant(QF1, qs1, qs2);
            QF3 = intraRequantRM(QF1, qs1, qs2);
            F1 = intraDequant(QF1, qs1, W);
            F2 = intraDequant(QF2, qs2, W);
            F3 = intraDequant(QF3, qs2, W);
        }
        else {
            QF2 = interRequant(QF1, qs1, qs2);
            QF3 = interRequantRM(QF1, qs1, qs2);
            F1 = interDequant(QF1, qs1, W);
            F2 = interDequant(QF2, qs2, W);
            F3 = interDequant(QF3, qs2, W);
        }

        printf("%3d => %4d  |  %3d => %4d (%3d)  |  %3d => %4d (%3d)\n",
            QF1, F1, QF2, F2, F2-F1, QF3, F3, F3-F1);

    }
    printf("\n");
}
#endif



#if 0
int QS[42] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
               30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60,
               62, 64, 72, 80, 88, 96, 104, 112 };


int cnt;		// total cases tested
int goodCnt;		// cases with lowest mse
int tieGoodCnt;		// cases with tie mse and lower index
int tieBadCnt;		// cases with tie mse and higher index
int badZeroCnt;		// cases with higher mse and zero index
int badOtherCnt;	// cases with higher mse and nonzero index

float badZeroErr;	// avg MSE of cases with higher mse and zero index
float badOtherErr;	// avg MSE of cases with higher mse and nonzero index


// Check optimality
//   qs1, QF1 is the original
//   qs2, QF2 is the requant result to be evaluated
//
void checkOptimality(int qs1, int qs2, int qf1, int qf2, int w)
{
    int f1, f2, fL, fH, qfL, qfH, diff, diffL, diffH, diffLH;

    qfL = qf2 - 1;
    qfH = qf2 + 1;

#if 1
    f1 = intraDequant(qf1, qs1, w);
    f2 = intraDequant(qf2, qs2, w);
    fL = intraDequant(qfL, qs2, w);
    fH = intraDequant(qfH, qs2, w);
#else
    f1 = interDequant(qf1, qs1, w);
    f2 = interDequant(qf2, qs2, w);
    fL = interDequant(qfL, qs2, w);
    fH = interDequant(qfH, qs2, w);
#endif

    diff = abs(f2 - f1);
    diffL = abs(fL - f1);
    diffH = abs(fH - f1);
    diffLH = (diffL < diffH)?  diffL : diffH;

    // Check for lowest requant error
    cnt++;

    if (diff < diffLH)
        // Lowest MSE case
        goodCnt++;

    else if (diff == diffLH) {
        // Tie MSE case
        if (diff < diffL)
            tieGoodCnt++;
        else
            tieBadCnt++;
    }

    else {
        // Bad MSE case
        if (qf2 == 0) {
            badZeroCnt++;
            if (diffLH)  badZeroErr += (float)diff / diffLH;

#if 0
            printf("\nBad Zero: qs %d->%d, w %d, %d->%d, %d->%d (%d),  ",
                   qs1, qs2, w, qf1, f1, qf2, f2, diff);
            if (diffLH == diffL)
                printf("%d->%d (%d)", qfL, fL, diffL);
            else
                printf("%d->%d (%d)", qfH, fH, diffH);
#endif
        }

        else {
            badOtherCnt++;
            if (diffLH)  badOtherErr += (float)diff / diffLH;

#if 0
            printf("\nBad Other: qs %d->%d, w %d, %d->%d, %d->%d (%d),  ",
                   qs1, qs2, w, qf1, f1, qf2, f2, diff);
            if (diffLH == diffL)
                printf("%d->%d (%d)", qfL, fL, diffL);
            else
                printf("%d->%d (%d)", qfH, fH, diffH);
#endif
        }
    }
}


int main()
{
    int i, j, w, qs1, qs2, qf1, qf2;
    int temp;
    float err;

    cnt = goodCnt = tieGoodCnt = tieBadCnt = badZeroCnt = badOtherCnt = 0;
    badZeroErr = badOtherErr = 0.;
    
    for (i=0; i<42; i+=2) {
        qs1 = QS[i];
        for (w=8; w<=128; w++) {
            for (j=i+1; j<42; j++) {
                qs2 = QS[j];
                for (qf1=-40; qf1<=40; qf1++) {

//                    qf2 = intraRequantFast(qf1, qs1, qs2);
//                    qf2 = interRequantFast(qf1, qs1, qs2);
                    qf2 = intraRequantSimple(qf1, qs1, qs2);
//                    qf2 = interRequantSimple(qf1, qs1, qs2);

                    checkOptimality(qs1, qs2, qf1, qf2, w);
                }		// qf1
            }			// j - qs2
        }			// w
    }				// i - qs1

    if (goodCnt + tieGoodCnt + tieBadCnt + badZeroCnt + badOtherCnt != cnt)
        printf("\nCounting problem!");

    printf("\nCases Tested: %d", cnt);
    printf("\nLowest MSE cases: %d (%.2f)", goodCnt, 100.*goodCnt/cnt);

    temp = tieGoodCnt + tieBadCnt;

    printf("\n\nTie MSE cases: %d (%.2f)", temp, 100.*temp/cnt);
    printf("\nGood Tie cases: %d (%.2f)", tieGoodCnt, 100.*tieGoodCnt/cnt);
    printf("\nBad Tie cases: %d (%.2f)", tieBadCnt, 100.*tieBadCnt/cnt);

    temp = badZeroCnt + badOtherCnt;
    err = badZeroErr + badOtherErr;
    printf("\n\nBad MSE cases: %d (%.2f)", temp, 100.*temp/cnt);
    printf("\n  Avg err ratio: %.2f", err/temp);

    printf("\nBad zero cases: %d (%.2f)", badZeroCnt, 100.*badZeroCnt/cnt);
    if (badZeroErr)
        printf("\n  Avg err ratio: %.2f", badZeroErr/badZeroCnt);
    printf("\nBad other cases: %d (%.2f)", badOtherCnt, 100.*badOtherCnt/cnt);
    if (badOtherErr)
        printf("\n  Avg err ratio: %.2f\n\n", badOtherErr/badOtherCnt);
}
#endif



#if 1
// This is the end-to-end quantization error comparision test
// suggested by Cullen Jenning, my patent reviewer.
//
int main(int argc, char** argv)
{
    Param par;
    int intraFlag;
    int qs1, qs2, W;
    int F1;			// original value
    int QF1, QF2, QF3;		// quantized values
    int DF1, DF2, DF3;		// dequantized values

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "requant.par");
    getIntParam(&par, "Original qscale?", &qs1, 4);
    getIntParam(&par, "New qscale?", &qs2, 8);
    getIntParam(&par, "Quantizer matrix value?", &W, 16);
    getIntParam(&par, "Intra MB?", &intraFlag, 0);
    endParam(&par);

    printf("%s, QS %d => %d, W %d\n", intraFlag?"INTRA":"INTER", qs1, qs2, W);

    for (F1=0; F1<=512; F1++) {
        if (intraFlag) {
        }
        else {
            QF1 = tm5InterAcQuant(F1, qs1, W);
            QF2 = interRequantFast(QF1, qs1, qs2);
            QF3 = interRequantRM(QF1, qs1, qs2);

            DF1 = interDequant(QF1, qs1, W);
            DF2 = interDequant(QF2, qs2, W);
            DF3 = interDequant(QF3, qs2, W);
        }

        printf("%3d => %4d => %3d (%3d)  ", F1, QF1, DF1, DF1-F1);
        printf("|  %3d => %4d (%3d)  ", QF2, DF2, DF2-F1);
        printf("|  %3d => %4d (%3d)\n", QF3, DF3, DF3-F1);
    }
    printf("\n");
}
#endif

