#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define		NUM_TAPS		10
#define		VOLTAGE_AVE_FAC		0.09259
#define		VOLTAGE_FREQ_FAC	1080.0
#define		PER_SRC			3.70373703703704e-08
#define		NAT_FREQ		2.700135e+07
#define		K			0.1
#define		FREQ_VCXO		2.70013e+07
#define		TT			0.0
#define		TCH			0.0
#define		MST			270000
#define		MCALC			1000
#define		TXCTR			0
#define		LOCCTRCH		0
#define		NUMSTAMPS		10000
#define		SAMP_VAR		0.0
#define		RAN_JITTER		5.0
#define		PER_JITTER_PER		1
#define		PER_JITTER_AMP		0.0
#define		PER_SRC_PER		1000.0
#define		PER_SRC_AMP		0.0


// Contents of loop filter FIR delay storage
static int num_vals = 0;
static long difs[NUM_TAPS];

// Subroutine to implement loop filter, D/A, zero order hold
double loop_filter (long timestampdif);


int main ()
{
    double per_src;
    double nat_freq;
    double v;
    double freq_vcxo;
    double per_vcxo;
    double tt;
    double tr;
    double tch;
    long mst;
    long mcalc;
    long timestamp;
    long localctr;
    long txctr;
    long locctrch;
    long num_rcv_clks;
    double delta;

    int num_stats = 0;
    double min_err = 0.0;
    double max_err = 0.0;
    double err2 = 0.0;
    double errmean = 0.0;

    int num_stamp;
    long jitter;
    int i;

    // Initialze values
    per_src = PER_SRC;
    nat_freq = NAT_FREQ;
    freq_vcxo = FREQ_VCXO;
    tt = TT;
    tch = TCH;
    mst = MST;
    mcalc = MCALC;
    txctr = TXCTR;
    locctrch = LOCCTRCH;
    per_vcxo = 1.0 / FREQ_VCXO;


    // Main loop
    for (num_stamp=0; num_stamp < NUMSTAMPS; num_stamp++) {
        long this_mst;

        // Calculate time at which next timestamp is issued, adding any
        // variation in timestamp issue time
        delta = 2.0 * (((double)rand())/32767.0 - 0.5);
				// delta in range of [-1, 1]
        this_mst = (1.0 + SAMP_VAR*delta) * mst;

        delta = sin((2.0 * M_PI) * (1.0/PER_SRC_PER) * num_stamp);
        per_src = PER_SRC + PER_SRC_AMP*delta;

        tt += this_mst * per_src;
        txctr += this_mst;

        // Calculate received time
        num_rcv_clks = (int)((tt - tch)/per_vcxo);
        tr = tch + per_vcxo*num_rcv_clks;
        if (tr < tt) {
            tr += per_vcxo;
            num_rcv_clks++;
        }

        // Determine local counter value at time of received timestamp
        // (but just copy timestamp over for first received timestamp.)

        // Determine new VCXO frequency (don't update for first received
        // timestamp).  Modify by adding "jitter" so that received timestamp
        // is off by a small amount.
        delta = 2.0 * (((double)rand())/32767.0 - 0.5);
        jitter = (long)(RAN_JITTER * delta);

        // Modify by adding a periodic "jitter" component
        delta = sin((2.0 * M_PI) * (1.0/PER_JITTER_PER) * num_stamp);
        jitter += (long)(PER_JITTER_AMP * delta);

        timestamp = txctr + jitter;

        if (num_stamp == 0) {
            v = 0.0;
            localctr = timestamp;
				// assume zero phasse difference initially
        }

        // To make default acquisition strategy, comment out next section.
        // To make compound acquisition strategy, leave it in.

        else if (num_stamp == 1) {
            localctr = locctrch + num_rcv_clks;
            v = VOLTAGE_AVE_FAC * (timestamp - localctr);
            localctr = -NUM_TAPS * (timestamp - localctr) + timestamp;
        }

        else {
            localctr = locctrch + num_rcv_clks;
            v = loop_filter(timestamp - localctr); 
        }

        // Calculate time at which VCXO frequency is changed.
        tch = tr + mcalc * per_vcxo;
        locctrch = localctr + mcalc;

        freq_vcxo = nat_freq + VOLTAGE_FREQ_FAC * v;
        per_vcxo = 1.0 / freq_vcxo;

#if 0
        // Print results
        printf("time = %10.6g sec\n", tch);
        printf(" filter contents: ");
        for (i=0; i<NUM_TAPS; i++)
            printf("%4d ", difs[i]);
        printf("\n");
        printf(" output voltate: %16.12g\n", v);
        printf(" new frequency: %24.18g\n", freq_vcxo);
        printf(" frequency error: %16.12g\n", freq_vcxo - 1.0/per_src);
        printf("\n");
#endif

        // Collect statistics after acquisition time
        if (num_stamp > 100) {
            double err = freq_vcxo - 1.0/PER_SRC;
            if (err > max_err)  max_err = err;
            if (err < min_err)  min_err = err;
            errmean += err;
            err2 += err * err;
            num_stats++;
        }
    }

    // Print error statistics
    errmean /= num_stats;
    err2 /= num_stats;
    err2 -= errmean * errmean;
    printf("min %1f, max %1f, mean %1f, var %1f\n",
           min_err, max_err, errmean, err2);
}


double loop_filter (long timestampdif)
{
    int tap;
    double ave;
    double ret_val;

    // Initialize delay stoage on first time through
    if (num_vals == 0)
        for (tap=0; tap<NUM_TAPS; tap++)
            difs[tap] = 0;

    // Push values through the delay storage and add the new value
    for (tap=NUM_TAPS-1; tap>0; tap--)
        difs[tap] = difs[tap-1];
    difs[0] = timestampdif;
    num_vals = (num_vals == NUM_TAPS) ? NUM_TAPS : num_vals + 1;

    // Compute average over those elemetns that are in the delay storage
    ave = 0.0;
    for (tap=0; tap<num_vals; tap++)
        ave += difs[tap];
    ave /= num_vals;

    ave *= K;

    // Clip to nominal range of -2.5V to +2.5V
    ret_val = VOLTAGE_AVE_FAC * ave;
    if (ret_val < -2.5)  ret_val = -2.5;
    if (ret_val > 2.5)  ret_val = 2.5;

    return (ret_val);
}
