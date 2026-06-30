h22965
s 00020/00012/00165
d D 1.4 03/01/03 13:15:14 ytse 4 3
c Update
e
s 00035/00032/00142
d D 1.3 02/12/31 10:43:11 ytse 3 2
c Backup
e
s 00103/00012/00071
d D 1.2 02/12/30 13:17:15 ytse 2 1
c Backup
e
s 00083/00000/00000
d D 1.1 02/12/13 16:12:18 ytse 1 0
c date and time created 02/12/13 16:12:18 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    timing.c

    Timing reclovery simulation

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
I 2
#include <math.h>
E 2
#include "util.h"
#include "param.h"


#define	REF_FREQ		27000000
I 2
#define MAX_DRIFT_RATE		0.000060
				// 60 PPM according to MPEG-2
E 2

I 3
#define MAX_DRIFT_ADJ		0.075
				// 75 x 10^-3 Hz/sec according to MPEG-2
E 3

I 3

E 3
I 2
// Return a uniformly distributed random value between min and max
float unifRand(float min, float max)
{
    float temp = rand() / 32767.;	// between 0 and 1
    return (min + (max - min) * temp);
}


D 3
#define	NUM_TAPS		64
E 3
I 3
D 4
#define	NUM_TAPS		2
E 4
I 4
#define	NUM_TAPS		16
E 4
E 3

D 4
double loopFilter(long x)
E 4
I 4
double loopFilter(double x)
E 4
{
    static int n = 0;
D 4
    static long sum = 0;
    static long val[NUM_TAPS];
E 4
I 4
    static double sum = 0;
    static double val[NUM_TAPS];
E 4
    int tap, idx;

    if (n == 0) {
        for (tap=0; tap<NUM_TAPS; tap++)
D 4
            val[tap] = 0;
E 4
I 4
            val[tap] = 0.;
E 4
    }

    idx = n % NUM_TAPS;
    sum -= val[idx];
    sum += x;
//printf("%d: %d, sum %d\n", n, x, sum);

    val[idx] = x;
    n++;

    tap = (n < NUM_TAPS)? n : NUM_TAPS;
D 4
    return (((double)sum) / tap);
E 4
I 4
    return (sum/tap);
E 4
}


E 2
int main(int argc, char** argv)
{
    Param par;
    int   numTs;
D 2
    float tsPer;		// timestamp period (according to source clock)
E 2
I 2
    float tsPrd;		// timestamp period (according to source clock)
E 2
    float maxJitter;
    float temp;
I 2
D 3
    float K1;
    float gain;			// PLL loop gain
E 3
I 3
    float loopGain;		// PLL loop gain
E 3
E 2

    unsigned long srcClk;	// source clock value
    unsigned long localClk;	// local clock value

    double srcFreq;		// source clock frequency
D 2
    double srcPer;		// source clock period (in sec)
E 2
I 2
    double srcPrd;		// source clock period (in sec)
E 2
    int    srcTicks;		// average source ticks between timestamps
    unsigned int localTicks;	// local ticks btw current and prev timestamps
    double tsItvl;		// time between current and previous timestamps

    double sendTime;		// send time of timestamp
    double recTime;		// receive time of timestamp
    double prevRecTime;		// receive time of previous timestamp
    double jitter;		// network jitter
I 2
    int    diff;
E 2
    int    i;

I 2
D 3
    double vcoClk;		// software clock to track source clock
    double drift;		// clock drift
E 3
I 3
    unsigned long vcoClk;	// software clock to track source clock
D 4
    long drift;			// clock drift
E 4
I 4

//    long drift;			// clock drift
    double vcoClk2;
    double drift;			// clock drift

E 4
E 3
    double fltDrift;		// filtered clock drift
    double driftRate;		// clock drift rate
D 3
double driftRate0;
double driftRate2;
    unsigned long vcoClk2;
E 3
I 3
    double driftRate0;		// clock drift rate
    double driftAdj;
    double driftAdj0;
E 3
E 2

I 2

E 2
    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "timing.par");

    commentParam(&par, "Clock Recovery Simulation");
    getIntParam(&par, "Number of timestamps", &numTs, 10000);
D 2
    getFloatParam(&par, "Timestamp period (sec)", &tsPer, 0.1);
E 2
I 2
    getFloatParam(&par, "Timestamp period (sec)", &tsPrd, 0.1);
E 2
    getFloatParam(&par, "Source clock frequency (Hz)", &temp, 27000000.);
    getFloatParam(&par, "Max network jitter (sec)", &maxJitter, 0.05);
I 2
D 3
//    getFloatParam(&par, "Low pass parameter", &K1, 0.05);
    getFloatParam(&par, "PLL loop gain parameter", &gain, 0.05);
E 3
I 3
    getFloatParam(&par, "PLL loop gain parameter", &loopGain, 0.05);
E 3
E 2
    srcFreq = temp;
    endParam(&par);

I 2
    printf("Src clock rate: %f Hz\n", srcFreq);
    printf("Max Jitter: %f sec\n", maxJitter);
D 3
    printf("PLL loop gain: %f\n\n", gain);
E 3
I 3
    printf("PLL loop gain: %f\n\n", loopGain);
E 3

E 2
    srcClk = 0;
    localClk = 0;
    sendTime = 0.;
    prevRecTime = 0;
D 2
    srcPer = 1./srcFreq;
E 2
I 2
    srcPrd = 1./srcFreq;
E 2

    for (i=0; i<numTs; i++) {
        // Generate next timestamp from source
D 2
        srcTicks = (int)(REF_FREQ * tsPer + 0.5);
E 2
I 2
        srcTicks = (int)(REF_FREQ * tsPrd + 0.5);
E 2
        srcClk += srcTicks;
D 2
        tsItvl = srcTicks * srcPer;
E 2
I 2
        tsItvl = srcTicks * srcPrd;
E 2
        sendTime += tsItvl;

        // Introduce network jitter
D 2
        jitter =  maxJitter * (rand()/32767.);
E 2
I 2
        temp = sin(unifRand(0., M_PI/2.));	// sinusodial jitter
        if (sendTime < prevRecTime)
            jitter = maxJitter - (sendTime + maxJitter - prevRecTime) * temp;
        else
            jitter = maxJitter * temp;
E 2
        recTime = sendTime + jitter;
        if (recTime <= prevRecTime)  recTime = prevRecTime;

I 2
        // Compute local arival timestamp
E 2
        localTicks = (unsigned int)((recTime - prevRecTime) * REF_FREQ + 0.5);
        localClk += localTicks;
        prevRecTime = recTime;
D 2

E 2
I 2
        diff = srcClk - localClk;
E 2
#if 1
D 2
        printf("TS %d: val %d, arvlTs %d, sendTime %f, recTime %f\n",
               i, srcClk, localClk, sendTime, recTime);
#else
        printf("%u %u\n", srcClk, localClk);
E 2
I 2
D 3
        printf("TS %d: srcClk %ud, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %ud\n", i, srcClk, sendTime, jitter, recTime, localClk);
E 3
I 3
        printf("\nTS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u", i, srcClk, sendTime, jitter, recTime, localClk);
E 3
E 2
#endif

I 2
        // Clock recovery
        if (i==0) {
D 3
            vcoClk = (double)srcClk;
E 3
I 3
            vcoClk = srcClk;
I 4
            vcoClk2 = vcoClk;
E 4
E 3
            fltDrift = 0.;
            driftRate = 0.;
        }
D 4
        else
E 4
I 4
        else {
E 4
D 3
//            vcoClk += localTicks * (1. + driftRate);
            vcoClk += localTicks * (1. + driftRate2);
E 3
I 3
            vcoClk += (long)(localTicks * (1. + driftRate) + 0.5);
I 4
            vcoClk2 += localTicks * (1. + driftRate);
            if (fabs(vcoClk2 - vcoClk) > 100)  vcoClk2 = vcoClk;
        }
E 4
E 3

D 3
        drift = srcClk - vcoClk;
E 3
I 3
D 4
        drift = (long)(srcClk - vcoClk);
E 4
I 4
        drift = srcClk - vcoClk2;
E 4
E 3

        // filter clock drift
D 3
//        fltDrift = (1.-K1) * fltDrift + K1 * drift;
        fltDrift = loopFilter(drift);
E 3
I 3
D 4
        fltDrift = loopFilter((double)drift);
E 4
I 4
        fltDrift = loopFilter(drift);
E 4
E 3

D 3
        driftRate += fltDrift * gain;
driftRate2 = driftRate / (tsPrd * REF_FREQ);
E 3
I 3
D 4
        printf("\n    dft %d, fltDft %f", drift, fltDrift);
temp = localTicks * driftRate;
E 4
I 4
        printf("\n    dft %f, fltDft %f", drift, fltDrift);
temp = REF_FREQ * driftRate;
E 4
printf(", chg %f", temp);
E 3

D 3
        printf("    dft %f, fltDft %f, dftRate %f\n",
               drift, fltDrift, driftRate2);
E 3
I 3
        driftAdj = fltDrift * loopGain / (tsPrd * REF_FREQ);
        printf(", dftAdj %g", driftAdj);
#if 0
        driftAdj0 = driftAdj;
        if (driftAdj > MAX_DRIFT_ADJ)  driftAdj = MAX_DRIFT_ADJ;
        if (driftAdj < -MAX_DRIFT_ADJ)  driftAdj = -MAX_DRIFT_ADJ;
        if (driftAdj != driftAdj0)  printf(" -> %g", driftAdj);
#endif
E 3

D 3
        // Clamping driftRate
        if (driftRate2 > MAX_DRIFT_RATE)  driftRate2 = MAX_DRIFT_RATE;
        if (driftRate2 < -MAX_DRIFT_RATE)  driftRate2 = -MAX_DRIFT_RATE;

E 3
I 3
        driftRate += driftAdj;
        printf(", dftRate %g", driftRate);
E 3
#if 0
D 3
        vcoClk2 = (unsigned long)vcoClk;
        printf("%d: PCR %d, ATS %d, CTS %d\n", i, srcClk, localClk, vcoClk2);
        printf("    dft %f, fltDft %f, Rd %f -> %f\n",
            drift, fltDrift, driftRate0, driftRate);
        printf("    diff: %d -> %d\n", localClk - srcClk, vcoClk2 - srcClk);
E 3
I 3
        driftRate0 = driftRate;
        if (driftRate > MAX_DRIFT_RATE)  driftRate = MAX_DRIFT_RATE;
        if (driftRate < -MAX_DRIFT_RATE)  driftRate = -MAX_DRIFT_RATE;
        if (driftRate != driftRate0)  printf(" -> %f", driftRate);
E 3
#endif
E 2
    }
}

E 1
