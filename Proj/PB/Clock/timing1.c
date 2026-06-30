/*****************************************************************************
    timing1.c

    Timing reclovery simulation
    Note: this program is copied from timing.c for major modifications
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
float ALPHA;

#if 1
double lowpassFilter(double in)
{
    static double out;
    out = out * (1-ALPHA) + in * ALPHA;
    return out;
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

    double vcoClk2;
    double drift;			// clock drift

    double fltDrift;		// filtered clock drift
    double driftRate;		// clock drift rate
    double driftRate0;		// clock drift rate
    double driftAdj;
    double driftAdj0;


    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "timing1.par");

    commentParam(&par, "Clock Recovery Simulation");
    getIntParam(&par, "Number of timestamps", &numTs, 10000);
    getFloatParam(&par, "Timestamp period (sec)", &tsPrd, 0.1);
    getFloatParam(&par, "Source clock frequency (Hz)", &temp, 27000000.);
    getFloatParam(&par, "Max network jitter (sec)", &maxJitter, 0.05);
    getFloatParam(&par, "PLL loop gain parameter", &loopGain, 0.05);
    getFloatParam(&par, "Lowpass parameter", &ALPHA, 0.0001);
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
#if 1
        printf("TS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u\n", i, srcClk, sendTime, jitter, recTime, localClk);
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
            if (fabs(vcoClk2 - vcoClk) > 100) {
                printf("\n\n!!! vco adjust!!!\n");
                printf("  vcoClk: %g->%d\n\n\n", vcoClk2, vcoClk);
                vcoClk2 = vcoClk;
            }
        }

#if 1
        printf("    vcoClk2 %g, dft %g, fltDft %g, adj %g, dftRate %g\n",
               vcoClk2, drift, fltDrift, driftAdj, driftRate);
#endif

        drift = srcClk - vcoClk2;
        fltDrift = lowpassFilter(drift);
        driftAdj = fltDrift * loopGain / (tsPrd * REF_FREQ);
        driftRate += driftAdj;

#if 0
printf("%g\n", driftRate * 1e6);
#endif

    }
}

