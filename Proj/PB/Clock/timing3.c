/*****************************************************************************
    timing3.c

    Timing reclovery simulation

    Use VCO approach, but with VCO tracking offset only, not REF_FREQ
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include "param.h"


#define MINIMUM_FILTER		0

#define	REF_FREQ		27000000
#define MAX_DRIFT_RATE		0.000060
				// 60 PPM according to MPEG-2

#define MAX_DRIFT_ADJ		0.075
				// 75 x 10^-3 Hz/sec according to MPEG-2


//////////////// Random number generator ////////////////

// Return a uniformly distributed random value between min and max
float unifRand(float min, float max)
{
    float temp = rand() / 32767.;	// between 0 and 1
    return (min + (max - min) * temp);
}


//////////////// Minimum filter ////////////////

#define NUM_DELAYS		16


long minFilter(long in)
{
    static int numIn = 0;
    static int curIdx = 0;
    static long inVal[NUM_DELAYS];
    int i, numTaps;
    long min;

    // Insert current input into delay line
    inVal[curIdx] = in;
    numIn++;

    // Find minimum value in delay line
    numTaps = (numIn > NUM_DELAYS)? NUM_DELAYS : numIn;
    min = inVal[0];
    for (i=1; i<numTaps; i++)
        if (inVal[i] < min)  min = inVal[i];

    // Update current index
    if (++curIdx == NUM_DELAYS)  curIdx = 0;

    return min;
}


//////////////// Low pass filter (2-tap IIR) //////////////// 
float ALPHA;

#if 0
double lowpassFilter(double in)
{
    static double out;
    out = out * (1-ALPHA) + in * ALPHA;
    return out;
}
#endif


//////////////// Low pass filter (FIR) //////////////// 
#if 1
#define NUM_TAPS                1024

double val[NUM_TAPS];

double lowpassFilter(double x)
{
    static int n = 0;
    static double sum = 0;
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


//////////////// Main program //////////////// 

int main(int argc, char** argv)
{
    Param par;
    int   numTs;
    float tsPrd;		// timestamp period (according to source clock)
    float maxJitter;
    float temp;

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
    long   offset;
    long   minOffset;
    double adjOffset;
    double estOffset;
    double drift;
    double freqDiff;
    double K;
double prevEstOffset;

    int    i, j;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "timing3.par");

    commentParam(&par, "Clock Recovery Simulation");
    getIntParam(&par, "Number of timestamps", &numTs, 10000);
    getFloatParam(&par, "Timestamp period (sec)", &tsPrd, 0.1);
    getFloatParam(&par, "Source clock frequency (Hz)", &temp, 27000000.);
    getFloatParam(&par, "Max network jitter (sec)", &maxJitter, 0.1);
    getFloatParam(&par, "Low pass parameter", &ALPHA, 0.0001);
    srcFreq = temp;
    endParam(&par);

#if 0
    printf("Src clock rate: %f Hz\n", srcFreq);
    printf("Drift rate: %g\n", (srcFreq-REF_FREQ)/REF_FREQ);
    printf("Max Jitter: %f sec\n", maxJitter);
    printf("Low pass parameter: %f\n\n", ALPHA);
#endif

    srcClk = 0;
    localClk = 100000;
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
        temp = 1. - sin(unifRand(0., M_PI/2.));      // sinusodial jitter
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
#if 0
        printf("TS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u\n", i, srcClk, sendTime, jitter, recTime, localClk);
#endif


        //////////////// Clock Recovery ////////////////

        offset = localClk - srcClk;

#if MINIMUM_FILTER
        minOffset = minFilter(offset);
#else
        minOffset = offset;
#endif

        if (i==0)  estOffset = minOffset;
prevEstOffset = estOffset;
        drift = minOffset - estOffset;
        K = 2 * ALPHA / localTicks;
        freqDiff = lowpassFilter(K * drift);
        estOffset += freqDiff * localTicks;

#if 0
        printf("  ofst %d, minOfst %d, df %g, freqDf %g, adjOfst %g, estOfst %g\n",
               offset, minOffset, diff, freqDiff, adjOffset, estOffset);
#endif

#if 0
        if (freqDiff * 1e6 < -100) {
            printf("TS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
                   "revClk %u\n", i, srcClk, sendTime, jitter, recTime, localClk);
            printf("  os %d, minOs %d, estOs %g, df %g, freqDf %g, estOfst %g\n",
                   offset, minOffset, prevEstOffset, drift, freqDiff, estOffset);

            for (j=0; j<NUM_TAPS; j++)
                printf("%g ", val[j]);
            printf("\n\n");
        }
#endif

//        printf("%g %g\n", K*drift, freqDiff);
        printf("%g\n", freqDiff);
    }
}
