h29777
s 00062/00017/00384
d D 1.13 03/04/15 10:09:50 ytse 13 12
c Update
e
s 00071/00013/00330
d D 1.12 03/04/11 15:42:05 ytse 12 11
c Backup before adding end-to-end simulation according to Cullen Jenning's suggestion.
e
s 00124/00136/00219
d D 1.11 02/05/24 12:26:50 ytse 11 10
c Update
e
s 00037/00009/00318
d D 1.10 02/04/15 15:48:00 ytse 10 9
c backup
e
s 00002/00002/00325
d D 1.9 02/02/25 17:53:46 ytse 9 8
c Update
e
s 00011/00018/00316
d D 1.8 02/01/21 11:02:33 ytse 8 7
c Update
e
s 00099/00041/00235
d D 1.7 02/01/18 13:54:51 ytse 7 6
c Update
e
s 00034/00021/00242
d D 1.6 02/01/18 10:53:44 ytse 6 5
c Fix mismatch with RM implementation
e
s 00140/00017/00123
d D 1.5 02/01/17 16:39:33 ytse 5 4
c Optimality test
e
s 00021/00006/00119
d D 1.4 02/01/17 14:04:19 ytse 4 3
c Optimize
e
s 00014/00008/00111
d D 1.3 02/01/17 13:37:26 ytse 3 2
c Fix mistake in requant formula
e
s 00015/00007/00104
d D 1.2 02/01/17 12:54:28 ytse 2 1
c Add parameters
e
s 00111/00000/00000
d D 1.1 02/01/17 11:49:16 ytse 1 0
c date and time created 02/01/17 11:49:16 by ytse
e
u
U
f e 0
t
T
I 1
// This program is used to study how to do coefficient requantization

#include <stdio.h>
#include <stdlib.h>
D 7

E 7
I 2
#include "param.h"
E 2

I 2

E 2
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


I 13
// TM5 quantization - for intra AC only!
int tm5InterAcQuant(int F, int qs, int W)
{
    int temp = (16*F + sign(F)*W/2) / W;
    return (temp / (2 * qs));
}


E 13
D 8
// Find amplitude (or absolute value)
int ampl(int x)
{
    return (x<0)? -x : x;
}


E 8
D 5
// MPEG2 dequantization (inter)
int dequant(int QF, int qs, int W)
E 5
I 5
// MPEG2 dequantization - intra
int intraDequant(int QF, int qs, int W)
E 5
{
I 5
    return shftTrunc((QF<<1) * W * qs, 5);
}


// MPEG2 dequantization - inter
int interDequant(int QF, int qs, int W)
{
E 5
    int sgn = sign(QF);
    return shftTrunc(((QF<<1) + sgn) * W * qs, 5);
}


D 5
// New requant
int requant(int QF1, int qs1, int qs2)
E 5
I 5
// New requant - intra
int intraRequant(int QF1, int qs1, int qs2)
E 5
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
D 4

E 4
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8
I 4

I 5
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
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8

E 5
E 4
    int QF2 = (r * val)<<1;
D 3
    QF2 += 1 - r;
E 3
I 3
    QF2 += r - (1<<scale);
E 3
    QF2 = shftRnd2Zero(QF2, scale+1);
I 4

E 4
    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


I 4
D 5
// Fast rquant
E 5
I 5
// Fast rquant - intra
E 5
//   Equivalent to New requant, but simplified
D 5
int requantFast(int QF1, int qs1, int qs2)
E 5
I 5
int intraRequantFast(int QF1, int qs1, int qs2)
E 5
{
    int scale = 13;
    int half = 1 << (scale-1);
    int r = (qs1<<scale) / qs2;
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8

I 5
    int QF2 = r * val + half;
D 7
    if (QF2>0)  QF2--;
E 7
I 7
    if (QF2)  QF2--;		// Note: QF2 is non negative!
E 7
    QF2 >>= scale;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


D 10
// Fast rquant - inter
E 10
I 10
// Fast requant - inter
E 10
//   Equivalent to New requant, but simplified
int interRequantFast(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int half = 1 << (scale-1);
    int r = (qs1<<scale) / qs2;
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8

E 5
    int QF2 = ((r * val)<<1) + r;
D 6
    if (QF2>0)  QF2--;
E 6
I 6
D 7
    if (QF2>0)  QF2--;		// Note: this seems to have little effect!
E 7
I 7
    if (QF2)  QF2--;		// Note: QF2 is non negative!
 				// Note: this seems to have little effect!
E 7
E 6
    QF2 >>= scale + 1;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


E 4
D 5
// RateMux requant
int requantRM(int QF1, int qs1, int qs2)
E 5
I 5
// RateMux requant - intra
int intraRequantRM(int QF1, int qs1, int qs2)
E 5
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
I 5
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8
E 5

I 5
    int QF2 = val * r;
D 6
    QF2 += (1<<11);
E 6
I 6
//    QF2 += (1<<11);		// new method (qtrScale)
    QF2 += (1<<12);		// old method (halfScale): actually better!
E 6
    QF2 >>= scale;

    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


// RateMux requant - inter
int interRequantRM(int QF1, int qs1, int qs2)
{
    int scale = 13;
    int r = (qs1<<scale) / qs2;
E 5
D 8
    int val = ampl(QF1);
E 8
I 8
    int val = abs(QF1);
E 8
I 5

E 5
    int QF2 = (val * r) << 1;
D 6
    QF2 += r + (1<<11);
E 6
I 6
    QF2 += r + (1<<12);
E 6
    QF2 >>= scale;
    if (QF2)  QF2 = (QF2-1) >> 1;
I 5

E 5
    if (QF1<0)  QF2 = -QF2;
    return QF2;
}


I 12
D 13
// This is the simple requant suggested by my patent reviewer!
E 12
I 3
D 5
#if 1
E 5
I 5

E 13
I 13
// This is the simple requant suggested by Cullen Jenning, my patent reviewer!
//
E 13
I 12
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


E 12
I 11
//----------------------------------------------------------------------------



E 11
D 9
#if 0
E 9
I 9
D 10
#if 1
E 10
I 10
#if 0
I 11
int main ()
{
    int i;
D 12
    for (i=-10; i<10; i++) {
        printf("%d: %d\n", i, shftRnd2Zero(i, 2));
E 12
I 12
    double t = -2;
    int t1, t2;

    for (i=0; i<50; i++) {
        t1 = (int)t;
        if (t<0)  t2 = (int)(t-0.4999);
        else if (t>0)  t2 = (int)(t+0.4999);
        printf("%g -> %d, %d\n", t, t1, t2);
        t += 0.1;
E 12
    }
}
#endif



D 12
#if 1
E 12
I 12
#if 0
E 12
E 11
E 10
E 9
E 5
E 3
D 2

main()
E 2
I 2
int main(int argc, char** argv)
E 2
{
D 3
#if 1

E 3
D 2
    int qs1 = 6;
    int qs2 = 12;
    int W = 16;

E 2
I 2
    Param par;
I 5
    int intraFlag;
E 5
    int qs1, qs2, W;
E 2
    int F1, F2, F3, QF1, QF2, QF3;
I 12
    int F4, QF4;
E 12

I 2
    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "requant.par");
    getIntParam(&par, "Original qscale?", &qs1, 4);
    getIntParam(&par, "New qscale?", &qs2, 8);
    getIntParam(&par, "Quantizer matrix value?", &W, 16);
I 5
    getIntParam(&par, "Intra MB?", &intraFlag, 0);
E 5
    endParam(&par);

D 3
    printf("QS: %d -> %d, W: %d\n", qs1, qs2, W);
E 3
I 3
D 5
    printf("QS: %d => %d, W: %d\n", qs1, qs2, W);
E 5
I 5
    printf("%s, QS %d => %d, W %d\n", intraFlag?"INTRA":"INTER", qs1, qs2, W);
E 5
E 3

E 2
D 12
    for (QF1=-30; QF1<=30; QF1++) {
E 12
I 12
D 13
    for (QF1=-20; QF1<=20; QF1++) {
E 13
I 13
    for (QF1=-10; QF1<=10; QF1++) {
E 13
E 12
D 5
        QF2 = requantRM(QF1, qs1, qs2);
D 4
        QF3 = requant(QF1, qs1, qs2);
E 4
I 4
        QF3 = requantFast(QF1, qs1, qs2);
E 4

        // Best requant
        F1 = dequant(QF1, qs1, W);
        F2 = dequant(QF2, qs2, W);
        F3 = dequant(QF3, qs2, W);
E 5
I 5
        if (intraFlag) {
D 12
            QF2 = intraRequantRM(QF1, qs1, qs2);
            QF3 = intraRequant(QF1, qs1, qs2);
E 12
I 12
            QF2 = intraRequant(QF1, qs1, qs2);
D 13
//            QF3 = intraRequantRM(QF1, qs1, qs2);
            QF4 = intraRequantSimple(QF1, qs1, qs2);
E 13
I 13
            QF3 = intraRequantRM(QF1, qs1, qs2);
E 13
E 12
            F1 = intraDequant(QF1, qs1, W);
            F2 = intraDequant(QF2, qs2, W);
D 12
            F3 = intraDequant(QF3, qs2, W);
E 12
I 12
D 13
//            F3 = intraDequant(QF3, qs2, W);
            F4 = intraDequant(QF4, qs2, W);
E 13
I 13
            F3 = intraDequant(QF3, qs2, W);
E 13
E 12
        }
        else {
D 12
            QF2 = interRequantRM(QF1, qs1, qs2);
            QF3 = interRequant(QF1, qs1, qs2);
E 12
I 12
            QF2 = interRequant(QF1, qs1, qs2);
D 13
//            QF3 = interRequantRM(QF1, qs1, qs2);
            QF4 = interRequantSimple(QF1, qs1, qs2);
E 13
I 13
            QF3 = interRequantRM(QF1, qs1, qs2);
E 13
E 12
            F1 = interDequant(QF1, qs1, W);
            F2 = interDequant(QF2, qs2, W);
D 12
            F3 = interDequant(QF3, qs2, W);
E 12
I 12
D 13
//            F3 = interDequant(QF3, qs2, W);
            F4 = interDequant(QF4, qs2, W);
E 13
I 13
            F3 = interDequant(QF3, qs2, W);
E 13
E 12
        }
I 12

D 13
#if 0
E 13
E 12
E 5
D 2
        printf("\n%3d->%4d  |  %3d->%4d(%4d)  |  %3d->%4d(%4d)",
E 2
I 2
D 3
        printf("%3d->%4d  |  %3d->%4d(%4d)  |  %3d->%4d(%4d)\n",
E 3
I 3
        printf("%3d => %4d  |  %3d => %4d (%3d)  |  %3d => %4d (%3d)\n",
E 3
E 2
            QF1, F1, QF2, F2, F2-F1, QF3, F3, F3-F1);
I 12
D 13
#else
        printf("%3d => %4d | %3d => %4d (%3d) | %3d => %4d (%3d)\n",
            QF1, F1, QF2, F2, F2-F1, QF4, F4, F4-F1);
#endif
E 13

E 12
    }
    printf("\n");
I 3
}
#endif
E 3

D 3
#else
E 3

I 11

E 11
I 3
D 12
#if 0
E 12
I 12
D 13
#if 1
E 13
I 13
#if 0
E 13
E 12
D 11
int main ()
{
D 4
    printf("%d\n", shftRnd2Zero(62, 4));

#if 0
E 4
E 3
    int i;
    for (i=-10; i<10; i++) {
D 3
        printf("%d: %d\n", i, shftRndDwn(i, 2));
E 3
I 3
        printf("%d: %d\n", i, shftRnd2Zero(i, 2));
E 3
    }
D 3

E 3
D 4
#endif
E 4
}
I 3
#endif
E 11
I 11
int QS[42] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
               30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60,
               62, 64, 72, 80, 88, 96, 104, 112 };
E 11
I 5


D 9
#if 1
E 9
I 9
D 10
#if 0
E 10
I 10
D 11
#if 1
E 11
I 11
int cnt;		// total cases tested
int goodCnt;		// cases with lowest mse
int tieGoodCnt;		// cases with tie mse and lower index
int tieBadCnt;		// cases with tie mse and higher index
int badZeroCnt;		// cases with higher mse and zero index
int badOtherCnt;	// cases with higher mse and nonzero index

float badZeroErr;	// avg MSE of cases with higher mse and zero index
float badOtherErr;	// avg MSE of cases with higher mse and nonzero index


E 11
E 10
E 9
D 6
int checkOptimality(int qs1, int qs2, int W, int QF1, int QF2)
E 6
I 6
D 7
int checkOptimality(int qs1, int qs2, int W, int QF1, int QF2, float* errFactor)
E 7
I 7
// Check optimality
D 11
int checkOptimality(int qs1, int qs2, int W, int QF1, int QF2,
                    int* bndyFlag, int* badTieFlag, float* errFactor)
E 11
I 11
//   qs1, QF1 is the original
//   qs2, QF2 is the requant result to be evaluated
//
void checkOptimality(int qs1, int qs2, int qf1, int qf2, int w)
E 11
E 7
E 6
{
D 7
    int QF3, F1, F2, F3, diff2, diff3;
    int flag = 0;
E 7
I 7
D 11
    int F1, F2, diff2, idx;
    int QF[2], F[2], diff[2];
    int badFlag;
E 7
I 6
    float temp;
E 11
I 11
    int f1, f2, fL, fH, qfL, qfH, diff, diffL, diffH, diffLH;
E 11
E 6

D 6
    F1 = interDequant(QF1, qs1, W);
    F2 = interDequant(QF2, qs2, W);
E 6
I 6
D 7
    *errFactor = 0.;

E 7
D 8
    F1 = intraDequant(QF1, qs1, W);
    F2 = intraDequant(QF2, qs2, W);
E 8
I 8
D 11
    F1 = interDequant(QF1, qs1, W);
    F2 = interDequant(QF2, qs2, W);
E 8
E 6
    diff2 = abs(F2 - F1);
E 11
I 11
    qfL = qf2 - 1;
    qfH = qf2 + 1;
E 11

I 12
#if 1
    f1 = intraDequant(qf1, qs1, w);
    f2 = intraDequant(qf2, qs2, w);
    fL = intraDequant(qfL, qs2, w);
    fH = intraDequant(qfH, qs2, w);
#else
E 12
D 7
    // Check for optimality
    QF3 = QF2 - 1;
D 6
    F3 = interDequant(QF3, qs2, W);
E 6
I 6
    F3 = intraDequant(QF3, qs2, W);
E 6
    diff3 = abs(F3 - F1);
    if ((diff3<diff2) || (diff3==diff2 && abs(QF3)<abs(QF2))) {
        flag = 1;
D 6
        printf("W %2d, QS %2d=>%2d: %3d -> %4d | %3d -> %4d (%3d)"\
            " | %3d -> %4d (%3d)\n",
            W, qs1, qs2, QF1, F1, QF2, F2, diff2, QF2-1, F3, diff3);
E 6
I 6
//        printf("W %2d, QS %2d=>%2d: %3d -> %4d | %3d -> %4d (%3d)"\
//            " | %3d -> %4d (%3d)\n",
//            W, qs1, qs2, QF1, F1, QF2, F2, diff2, QF2-1, F3, diff3);
        if (diff3>0)  *errFactor = (float)diff2 / diff3;
E 7
I 7
D 11
    QF[0] = QF2 - 1;
    QF[1] = QF2 + 1;
D 8
    F[0] = intraDequant(QF[0], qs2, W);
    F[1] = intraDequant(QF[1], qs2, W);
E 8
I 8
    F[0] = interDequant(QF[0], qs2, W);
    F[1] = interDequant(QF[1], qs2, W);
E 8
    diff[0] = abs(F[0] - F1);
    diff[1] = abs(F[1] - F1);
E 11
I 11
    f1 = interDequant(qf1, qs1, w);
    f2 = interDequant(qf2, qs2, w);
    fL = interDequant(qfL, qs2, w);
    fH = interDequant(qfH, qs2, w);
I 12
#endif
E 12
E 11

D 11
    badFlag = *bndyFlag = *badTieFlag = 0;
    *errFactor = 0.;
E 11
I 11
    diff = abs(f2 - f1);
    diffL = abs(fL - f1);
    diffH = abs(fH - f1);
    diffLH = (diffL < diffH)?  diffL : diffH;
E 11

    // Check for lowest requant error
D 11
    if (diff[0]<diff2) {
        if (!QF2)  *bndyFlag = 1;
        else  badFlag = 1;
        *errFactor = (float)diff2 / diff[0];
        idx = 0;
E 7
E 6
    }
I 7
    if (diff[1]<diff2) {
        if (!QF2)  *bndyFlag = 1;
        else  badFlag = 1;
        temp = (float)diff2 / diff[1];
        if (temp > *errFactor) {
            *errFactor = temp;
            idx = 1;
        }
    }
E 11
I 11
    cnt++;
E 11
E 7

D 7
    QF3 = QF2 + 1;
D 6
    F3 = interDequant(QF3, qs2, W);
E 6
I 6
    F3 = intraDequant(QF3, qs2, W);
E 6
    diff3 = abs(F3 - F1);
    if ((diff3<diff2) || (diff3==diff2 && abs(QF3)<abs(QF2))) {
        flag = 1;
D 6
        printf("W %2d, QS %2d=>%2d: %3d -> %4d | %3d -> %4d (%2d)"\
            " | %3d -> %4d (%2d)\n",
            W, qs1, qs2, QF1, F1, QF2, F2, diff2, QF2+1, F3, diff3);
E 6
I 6
//        printf("W %2d, QS %2d=>%2d: %3d -> %4d | %3d -> %4d (%2d)"\
//            " | %3d -> %4d (%2d)\n",
//            W, qs1, qs2, QF1, F1, QF2, F2, diff2, QF2+1, F3, diff3);
        if (diff3>0)  temp = (float)diff2 / diff3;
        if (temp > *errFactor)  *errFactor = temp;
E 7
I 7
D 11
    // Check for bad tie case
    if (diff[0]==diff2 && abs(QF[0])<abs(QF2)) {
        idx = 0;
        *badTieFlag = 1;
E 7
E 6
    }
I 7
    if (diff[1]==diff2 && abs(QF[1])<abs(QF2)) {
        idx = 0;
        *badTieFlag = 1;
    }
E 11
I 11
    if (diff < diffLH)
        // Lowest MSE case
        goodCnt++;
E 11
E 7

D 7
    return flag;
E 7
I 7
D 11
#if 1
    // Further examine bad case
    if (badFlag) {
        if (diff2-diff[idx] > 1) {
            printf("High error\n");
            printf("W %2d, QS %2d => %2d: %3d -> %4d | %3d -> %4d (%2d)"\
                " | %3d -> %4d (%2d)\n", W, qs1, qs2, QF1, F1, QF2, F2, diff2,
                QF[idx], F[idx], diff[idx]);
        }
        else if (abs(F2) > abs(F[idx])) {
            printf("Higher index\n");
            printf("W %2d, QS %2d => %2d: %3d -> %4d | %3d -> %4d (%2d)"\
                " | %3d -> %4d (%2d)\n", W, qs1, qs2, QF1, F1, QF2, F2, diff2,
                QF[idx], F[idx], diff[idx]);
        }
E 11
I 11
    else if (diff == diffLH) {
        // Tie MSE case
        if (diff < diffL)
            tieGoodCnt++;
        else
            tieBadCnt++;
E 11
    }
D 11
#endif
E 11

I 10
D 11
#if 1
      if (*bndyFlag) {
          int scale = 13;
          int half = 1 << (scale-1);
          int r = (qs1<<scale) / qs2;
          int val = abs(QF1);
E 11
I 11
    else {
        // Bad MSE case
        if (qf2 == 0) {
            badZeroCnt++;
            if (diffLH)  badZeroErr += (float)diff / diffLH;
E 11

D 11
          int QF3 = ((r * val)<<1) + r + (1<<scale);
          if (QF3)  QF3--;		// Note: QF2 is non negative!
          QF3 >>= scale + 1;
          if (QF1<0)  QF3 = -QF3;

          if (QF3 == QF2) {
              printf("Tie case in question\n");
              printf("W %2d, QS %2d => %2d: %3d -> %4d | %3d -> %4d (%2d)"\
                  " | %3d -> %4d (%2d)\n", W, qs1, qs2, QF1, F1, QF2, F2, diff2,
                  QF[idx], F[idx], diff[idx]);
           }
      }
#endif

E 11
E 10
#if 0
D 10
    if (badFlag || *bndyFlag || *badTieFlag)
E 10
I 10
D 11
//    if (badFlag || *bndyFlag || *badTieFlag) {
//    if (badFlag && !(*bndyFlag)) {
    if (*bndyFlag) {
E 10
        printf("W %2d, QS %2d => %2d: %3d -> %4d | %3d -> %4d (%2d)"\
D 10
            " | %3d -> %4d (%2d)\n", W, qs1, qs2, QF1, F1, QF2, F2, diff2,
E 10
I 10
            " | %3d -> %4d (%2d)", W, qs1, qs2, QF1, F1, QF2, F2, diff2,
E 10
            QF[idx], F[idx], diff[idx]);
I 10
        if (badFlag)  printf("    Bad value");
        if (*badTieFlag)  printf("    Bad tie");
        if (*bndyFlag)  printf("    Boundry");
        printf("\n");
    }
E 11
I 11
            printf("\nBad Zero: qs %d->%d, w %d, %d->%d, %d->%d (%d),  ",
                   qs1, qs2, w, qf1, f1, qf2, f2, diff);
            if (diffLH == diffL)
                printf("%d->%d (%d)", qfL, fL, diffL);
            else
                printf("%d->%d (%d)", qfH, fH, diffH);
E 11
E 10
#endif
I 11
        }
E 11

D 11
    return (badFlag || *bndyFlag || *badTieFlag);
E 11
I 11
        else {
            badOtherCnt++;
            if (diffLH)  badOtherErr += (float)diff / diffLH;

D 12
#if 1
E 12
I 12
#if 0
E 12
            printf("\nBad Other: qs %d->%d, w %d, %d->%d, %d->%d (%d),  ",
                   qs1, qs2, w, qf1, f1, qf2, f2, diff);
            if (diffLH == diffL)
                printf("%d->%d (%d)", qfL, fL, diffL);
            else
                printf("%d->%d (%d)", qfH, fH, diffH);
#endif
        }
    }
E 11
E 7
}

I 11

E 11
int main()
{
D 7
    int W, qs1, qs2, QF1, QF2, flag;
    int caseCnt = 0;
E 7
I 7
D 11
    int W, qs1, qs2, QF1, QF2, badFlag, bndyFlag, badTieFlag;
    int totBadCnt;
    int totCnt = 0;
E 7
    int badCnt = 0;
I 7
    int bndyCnt = 0;
    int badTieCnt = 0;
E 11
I 11
    int i, j, w, qs1, qs2, qf1, qf2;
    int temp;
E 11
E 7
D 6
    W = 16;
    qs1 = 8;

E 6
I 6
    float err;
D 7
    float totErr = 0;
E 7
I 7
D 11
    float totBndyErr = 0;
    float totBadErr = 0;
E 11
I 11

    cnt = goodCnt = tieGoodCnt = tieBadCnt = badZeroCnt = badOtherCnt = 0;
    badZeroErr = badOtherErr = 0.;
E 11
E 7
    
E 6
D 7
    for (qs1=2; qs1<=32; qs1+=2)
E 7
I 7
D 10
    for (qs1=2; qs1<=30; qs1+=2)
E 7
       for (W=8; W<=128; W++)
           for (qs2=qs1; qs2<32; qs2+=2)
               for (QF1=-30; QF1<=30; QF1++) {
E 10
I 10
D 11
    for (qs1=2; qs1<=62; qs1+=2)
       for (W=8; W<=80; W++)
           for (qs2=qs1; qs2<62; qs2+=2)
               for (QF1=-10; QF1<=10; QF1++) {
E 10
D 6
                   QF2 = interRequant(QF1, qs1, qs2);
                   flag = checkOptimality(qs1, qs2, W, QF1, QF2);
                   if (flag)  badCnt++;
E 6
I 6
D 8
                   QF2 = intraRequantFast(QF1, qs1, qs2);
E 8
I 8
                   QF2 = interRequantFast(QF1, qs1, qs2);
E 8
D 7
                   flag = checkOptimality(qs1, qs2, W, QF1, QF2, &err);
                   if (flag) {
                       badCnt++;
                       totErr += err;
E 7
I 7
                   badFlag = checkOptimality(qs1, qs2, W, QF1, QF2,
                                             &bndyFlag, &badTieFlag, &err);
                   if (badFlag) {
                       if (badTieFlag)  badTieCnt++;
                       else if (bndyFlag) {
                           bndyCnt++;
                           totBndyErr += err;
                       }
                       else {
                           badCnt++;
                           totBadErr += err;
                       }
E 7
                   }
E 6
D 7
                   caseCnt++;
E 7
I 7
                   totCnt++;
E 7
               }
I 7
    totBadCnt = badCnt + bndyCnt + badTieCnt;
E 11
I 11
    for (i=0; i<42; i+=2) {
        qs1 = QS[i];
        for (w=8; w<=128; w++) {
            for (j=i+1; j<42; j++) {
                qs2 = QS[j];
                for (qf1=-40; qf1<=40; qf1++) {
D 12
                    qf2 = interRequantFast(qf1, qs1, qs2);
E 12
I 12

//                    qf2 = intraRequantFast(qf1, qs1, qs2);
//                    qf2 = interRequantFast(qf1, qs1, qs2);
                    qf2 = intraRequantSimple(qf1, qs1, qs2);
//                    qf2 = interRequantSimple(qf1, qs1, qs2);

E 12
                    checkOptimality(qs1, qs2, qf1, qf2, w);
                }		// qf1
            }			// j - qs2
        }			// w
    }				// i - qs1
E 11
E 7
D 6
    printf("%d bad cases out of %d\n", badCnt, caseCnt);
E 6
I 6

D 7
    printf("\n%d bad cases out of %d (%f percents)\n",
           badCnt, caseCnt, 100.*badCnt/caseCnt);
    printf("Avg Err %f\n", totErr/badCnt);
E 7
I 7
D 11
    printf("\n%d total cases tested.\n", totCnt);
    printf("%d bad tie cases (%f percents).\n",
        badTieCnt, 100.*badTieCnt/totCnt);
    printf("%d boundary cases (%f percents).  Avg err %f\n",
        bndyCnt, 100.*bndyCnt/totCnt, totBndyErr/bndyCnt);
    printf("%d bad cases (%f percents).  Avg err %f\n",
        badCnt, 100.*badCnt/totCnt, totBadErr/badCnt);
    printf("Total %d bad cases (%f percents).  Avg err %f\n",
        totBadCnt, 100.*totBadCnt/totCnt, (totBadErr+totBndyErr)/totBadCnt);
E 11
I 11
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
E 11
E 7
E 6
}
#endif
I 13



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
E 13
I 11

E 11
E 5
E 3
E 1
