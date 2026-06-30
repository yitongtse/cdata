h42625
s 00052/00017/00129
d D 1.2 02/12/31 18:52:34 ytse 2 1
c Good version
e
s 00146/00000/00000
d D 1.1 02/12/31 11:46:50 ytse 1 0
c date and time created 02/12/31 11:46:50 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    timing2.c

    Timing reclovery simulation
    According to Wayne Lee's paper
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


I 2
//////////////// Random number generator ////////////////

E 2
// Return a uniformly distributed random value between min and max
float unifRand(float min, float max)
{
    float temp = rand() / 32767.;	// between 0 and 1
    return (min + (max - min) * temp);
}


//////////////// Minimum filter ////////////////

D 2
#define NUM_DELAYS		64
E 2
I 2
#define NUM_DELAYS		16
E 2

D 2
double minFilter(double in)
E 2
I 2
long minFilter(long in)
E 2
{
    static int numIn = 0;
D 2
    static double inVal[NUM_DELAYS];
E 2
I 2
    static long inVal[NUM_DELAYS];
E 2
    static int curIdx = 0;
    int i, numTaps;
D 2
    double min;
E 2
I 2
    long min;
E 2

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
I 2

    return min;
E 2
}


//////////////// Low pass filter //////////////// 

D 2
float K;
E 2
I 2
float ALPHA;
E 2

double lowpassFilter(double in)
{
    static double out;
D 2
    out = out * (1-K) + in * K;
E 2
I 2
    out = out * (1-ALPHA) + in * ALPHA;
E 2
    return out;
}


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
D 2
    int    diff;
E 2
I 2
    long   offset;
    long   minOffset;
    double adjOffset;
    double estOffset;
    double diff;
    double freqDiff;
    double K;

E 2
    int    i;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "timing2.par");

    commentParam(&par, "Clock Recovery Simulation");
    getIntParam(&par, "Number of timestamps", &numTs, 10000);
    getFloatParam(&par, "Timestamp period (sec)", &tsPrd, 0.1);
    getFloatParam(&par, "Source clock frequency (Hz)", &temp, 27000000.);
    getFloatParam(&par, "Max network jitter (sec)", &maxJitter, 0.05);
D 2
    getFloatParam(&par, "Low pass parameter", &K, 0.000000001);
E 2
I 2
    getFloatParam(&par, "Low pass parameter", &ALPHA, 0.000000001);
E 2
    srcFreq = temp;
    endParam(&par);

I 2
#if 0
E 2
    printf("Src clock rate: %f Hz\n", srcFreq);
I 2
    printf("Drift rate: %g\n", (srcFreq-REF_FREQ)/REF_FREQ);
E 2
    printf("Max Jitter: %f sec\n", maxJitter);
D 2
    printf("Low pass parameter: %f\n\n", K);
E 2
I 2
    printf("Low pass parameter: %f\n\n", ALPHA);
#endif
E 2

D 2

E 2
    srcClk = 0;
D 2
    localClk = 0;
E 2
I 2
    localClk = 100000;
E 2
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
D 2
        temp = sin(unifRand(0., M_PI/2.));      // sinusodial jitter
E 2
I 2
        temp = 1. - sin(unifRand(0., M_PI/2.));      // sinusodial jitter
E 2
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
D 2
        diff = srcClk - localClk;
#if 1
        printf("\nTS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u", i, srcClk, sendTime, jitter, recTime, localClk);
E 2
I 2
#if 0
        printf("TS %d: srcClk %u, sendTime %f, jitter %f, arvlTime %f, "\
               "revClk %u\n", i, srcClk, sendTime, jitter, recTime, localClk);
E 2
#endif
D 2
    }
E 2


I 2
        //////////////// Clock Recovery ////////////////

        // Apply minimum filter
        offset = localClk - srcClk;
        minOffset = minFilter(offset);

        // PLL
        if (i==0)  estOffset = minOffset;
        diff = minOffset - estOffset;
        K = 2 * ALPHA / localTicks;
        freqDiff = lowpassFilter(K * diff);
        adjOffset = freqDiff * localTicks;
        estOffset += adjOffset;

#if 0
//        printf("%d %d %g %g\n", offset, minOffset, freqDiff, estOffset);
        printf("  ofst %d, minOfst %d, df %g, freqDf %g, adjOfst %g, estOfst %g\n",
               offset, minOffset, diff, freqDiff, adjOffset, estOffset);
#endif

#if 1
        printf("%g\n", freqDiff);
#endif
    }
E 2
}
E 1
