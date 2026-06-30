/*****************************************************************************
    timing.c

    Timing reclovery simulation

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include "param.h"


#define	REF_FREQ		27000000
#define MAX_DRIFT_RATE		0.000060
				// 60 PPM according to MPEG-2

#define MAX_DRIFT_ADJ		0.075
				// 75 x 10^-3 Hz/sec according to MPEG-2


// Return a uniformly distributed random value between min and max
float unifRand(float min, float max)
{
    float temp = rand() / 32767.;	// between 0 and 1
    return (min + (max - min) * temp);
}


//////////////// Low pass filter (2-tap IIR) ////////////////
float ALPHA = 0.0001;

#if 1
double lowpassFilter(double in)
{
    static double out;
    out = out * (1-ALPHA) + in * ALPHA;
    return out;
}
#endif


//////////////// Low pass filter (FIR) ////////////////
#if 0

#define	NUM_TAPS		16

double lowpassFilter(double x)
{
    static int n = 0;
    static double sum = 0;
    static double val[NUM_TAPS];
    int tap, idx;

    if (n == 0) {
        for (tap=0; tap<NUM_TAPS; tap++)
            val[tap] = 0.;
    }

    idx = n % NUM_TAPS;
    sum -= val[idx];
    sum += x;

    val[idx] = x;
    n++;

    tap = (n < NUM_TAPS)? n : NUM_TAPS;
    return (sum/tap);
}
#endif


int main(int argc, char** argv)
{
    Param par;
    int   numTs;
    float tsPrd;		// timestamp period (according to source clock)
    float maxJitter;
    float temp;
    float loopGain;		// PLL loop gain

    unsigned long srcClk;	// source clock value
    unsigned long localClk;	// local clock value

    double srcFreq;		// source clock frequency
    double srcPrd;		// source clock period (in sec)
    int    srcTicks;		// average source ticks between timestamps
    unsigned int localTicks;	// local ticks btw current and prev timestamps
    double tsItvl;		// time between current and previous timestamps

    double sendTime;		// send time of timestamp
    double recTime;		// receive time of timestamp
    double prevRecTime;		// receive time of previous timestamp
    double jitter;		// network jitter
    int    diff;
    int    i;

    unsigned long vcoClk;	// software clock to track source clock

//    long drift;			// clock drift
    double vcoClk2;
    double drift;			// clock drift

    double fltDrift;		// filtered clock drift
    double driftRate;		// clock drift rate
    double driftRate0;		// clock drift rate
    double driftAdj;
    double driftAdj0;


    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "timing.par");

    commentParam(&par, "Clock Recovery Simulation");
    getIntParam(&par, "Number of timestamps", &numTs, 10000);
    getFloatParam(&par, "Timestamp period (sec)", &tsPrd, 0.1);
    getFloatParam(&par, "Source clock frequency (Hz)", &temp, 27000000.);
    getFloatParam(&par, "Max network jitter (sec)", &maxJitter, 0.05);
    getFloatParam(&par, "PLL loop gain parameter", &loopGain, 0.05);
    srcFreq = temp;
    endParam(&par);

//    printf("Src clock rate: %f Hz\n", srcFreq);
//    printf("Max Jitter: %f sec\n", maxJitter);
//    printf("PLL loop gain: %f\n\n", loopGain);

    srcClk = 0;
    localClk = 0;
    sendTime = 0.;
    prevRecTime = 0;
    srcPrd = 1./srcFreq;

    for (i=0; i<numTs; i++) {
        // Generate next timestamp from source
        srcTicks = (int)(REF_FREQ * tsPrd + 0.5);
        srcClk += srcTicks;
        tsItvl = srcTicks * srcPrd;
        sendTime += tsItvl;

        // Introduce network jitter
        temp = sin(unifRand(0., M_PI/2.));	// sinusodial jitter
        if (sendTime < prevRecTime)
            jitter = maxJitter - (sendTime + maxJitter - prevRecTime) * temp;
        else
            jitter = maxJitter * temp;
        recTime = sendTime + jitter;
        if (recTime <= prevRecTime)  recTime = prevRecTime;

        // Compute local arival timestamp
        localTicks = (unsigned int)((recTime - prevRecTime) * REF_FREQ + 0.5);
        localClk += localTicks;
        prevRecTime = recTime;
        diff = srcClk - localClk;
#if 0
        printf("\nTS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u", i, srcClk, sendTime, jitter, recTime, localClk);
#endif

        // Clock recovery
        if (i==0) {
            vcoClk = srcClk;
            vcoClk2 = vcoClk;
            fltDrift = 0.;
            driftRate = 0.;
        }
        else {
            vcoClk += (long)(localTicks * (1. + driftRate) + 0.5);
            vcoClk2 += localTicks * (1. + driftRate);
            if (fabs(vcoClk2 - vcoClk) > 100)  vcoClk2 = vcoClk;
        }

        drift = srcClk - vcoClk2;

        // filter clock drift
        fltDrift = lowpassFilter(drift);

//        printf("\n    dft %f, fltDft %f", drift, fltDrift);
temp = REF_FREQ * driftRate;
// printf(", chg %f", temp);

        driftAdj = fltDrift * loopGain / (tsPrd * REF_FREQ);
//        printf(", dftAdj %g", driftAdj);
#if 0
        driftAdj0 = driftAdj;
        if (driftAdj > MAX_DRIFT_ADJ)  driftAdj = MAX_DRIFT_ADJ;
        if (driftAdj < -MAX_DRIFT_ADJ)  driftAdj = -MAX_DRIFT_ADJ;
        if (driftAdj != driftAdj0)  printf(" -> %g", driftAdj);
#endif

        driftRate += driftAdj;
//        printf(", dftRate %g", driftRate);
#if 0
        driftRate0 = driftRate;
        if (driftRate > MAX_DRIFT_RATE)  driftRate = MAX_DRIFT_RATE;
        if (driftRate < -MAX_DRIFT_RATE)  driftRate = -MAX_DRIFT_RATE;
        if (driftRate != driftRate0)  printf(" -> %f", driftRate);
#endif

printf("%g\n", driftRate * 1e6);

    }
}

