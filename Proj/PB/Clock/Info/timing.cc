static const char SccsId[] = "%W% %G%";

/*********************************************************************
Title: timing.C

Description:
This program simulates timing recovery with a timestamp-based system.
The simulation models a source counter (with small random frequency
variations as specified by the MPEG2 systems spec.) that occasionally
sends timestamps (i.e. counter values) to a receiver. The times
at which timestamps are sent are chosen at random, roughly using the
scheduling constraints of the Denali multiplexer.

The receiver has a VCO and feedback loop (with FIR loop filter) that
tracks the incoming timestamps. The simulation prints the difference
between the source and receiver counter, and the difference between
the source and receiver oscillator frequencies.

Revisions:
Paul Haskell		12/29/93		Creation date
Paul Haskell		12/30/93		Added more params.

Arguments:


Comments:
*********************************************************************/


////////
//////// Include files
////////
#include <stdlib.h>
#include <unistd.h>
#include <stream.h>
#include <math.h>
#include "cli.h"
#include "param_list.h"
#include "randclass.h"


////////
//////// Local type definition
////////
template <class T> inline T min(const T& a, const T& b)
{ return ((a) < (b) ? (a) : (b)); }

template <class T> inline T max(const T& a, const T& b)
{ return ((a) > (b) ? (a) : (b)); }


class argList {
public:
	int iters; // how long to run the simulation
	float natVCOFreq; // natural VCO frequency
	float maxJitter; // maximum transmission channel delay jitter
	int ticksPerTstamp;

	argList() : iters(300), natVCOFreq(27.001e+6),
		ticksPerTstamp(2700000) {}
};


class LoopFilt {
public:
	LoopFilt(const int sz) : size(sz), buf(new int[sz]), count(0),
		index(0), sum(0.0) { }
	~LoopFilt() { delete [] buf; }
	void insert(const int);
	inline double avg() const;
	double lopass() const;

protected:
	double sum;
	const int size;
	int count, index;
	int* buf;
};


void LoopFilt::insert(const int val)
{
	if (++count > size) {
		count = size;
		sum -= buf[index];
	}
	buf[index] = val;
	sum += val;
	index = (index + 1) % size;
} // end LoopFilt::insert()


inline double LoopFilt::avg() const
{ return (sum/size); }


//// 32-bit Remez LPF with cutoff from 0 to 0.1 radians.
static float LPF[] = {
	0.0111, 0.0084, 0.0112, 0.0145, 0.0181, 0.0219, 0.0260,
	0.0301, 0.0341, 0.0380, 0.0416, 0.0448, 0.0476, 0.0497,
	0.0511, 0.0519, 0.0519, 0.0511, 0.0497, 0.0476, 0.0448,
	0.0416, 0.0380, 0.0341, 0.0301, 0.0260, 0.0219, 0.0181,
	0.0145, 0.0112, 0.0084, 0.0111};


double LoopFilt::lopass() const
{
	if (count < size) { return avg(); }

	double outp = 0.0;
	for(int i = 0; i < index; i++) {
		outp += LPF[index-i] * buf[i];
	}
	for(; i < count; i++) {
		outp += LPF[count+index-i] * buf[i];
	}
	return outp; // same normaliz as avg()
} // end LoopFilt::lopass()


////////
//////// Local function protos
////////
void BuildList(argList&, int, char**);
int NextSched(unifRandClass&, const int);
double SrcFreq(unifRandClass&);


//////
////// Global objects
//////
/**** Sinusoidal jitter ***/
double jitter(const double deltaT)
{
	static unifRandClass urc(2.0 * M_PI * 0.8, 2.0 * M_PI * 1.2);
	static double theta = 0.0;

	theta += deltaT * urc();

	// return (0.5e-3 * sin(theta));
	return (0.5e-4 * sin(theta));
}


////////
//////// Main module
////////
main(int argc, char** argv)
{
// Parse command line.
	argList params;
	BuildList(params, argc, argv);

// Initial stuff.
	unifRandClass urc(-1.0, 1.0); // random #s in the range (-1, 1).
	/**
	normalRandClass jitter(0.0, params.maxJitter * params.maxJitter);
	**/

	int srcMinusDiffCk = 0; // Diff between src and receive counters.
	double srcF, rcvF = params.natVCOFreq; // Source and receive freqs.
	double t = 0.0; // time at which current timestamp is generated.
	double rcvT = 0.0; // time of most recent receiver tick, when
						// a timestamp has arrived.

// Main loop.
	double err = 0.0;
	double max = 0.0;
	double gain = 0.1;
	LoopFilt* Filt = new LoopFilt(64);
	double compens = 0.0;

	for(int iters = 1; iters < params.iters; iters++) {
		if (iters == 175) {
			compens += gain * Filt->avg();
			gain = 0.01;
			delete Filt;
			Filt = new LoopFilt(64);
		} else if (iters == 1000) { // sinusoidal(1000), gaussian(500)
			compens += gain * Filt->avg();
			gain = 0.001;
			delete Filt;
			Filt = new LoopFilt(64);
		} else if (iters == 20000) { // gaussian(12k), sinusoidal(20k)
			compens += gain * Filt->avg();
			gain = 0.0001;
			delete Filt;
			Filt = new LoopFilt(64);
		}

			// Choose source frequency for this inter-tstamp interval.
		srcF = SrcFreq(urc);
			// Choose # clock-ticks till this timestamp.
		int srcTicks = NextSched(urc, params.ticksPerTstamp);
			// Calc time of current timestamp.
		double deltaT = srcTicks/srcF;
		t += deltaT;

			// Calculate network jitter to timestamp receive time.
			// Calc number of receiver clock-ticks up to this tstamp.
		err += (deltaT * (srcF - rcvF));

			// the '0.5' is new.
		int rcvTicks = int( (t + jitter(deltaT) - rcvT) * rcvF + 0.5);
		rcvT += rcvTicks / rcvF;
			// Update clock-difference value.
		srcMinusDiffCk += srcTicks - rcvTicks;
			// Update rcvF via feedback loop.
		Filt->insert(srcMinusDiffCk);
		rcvF = params.natVCOFreq + compens + Filt->avg() * gain;

			// Update counter error.
		if (fabs(err) > max) {
			max = fabs(err);
			// cout << "new max = " << max << " at time " << t << "\n";
		}

			// Print status.
		cout << t << "\t" << srcF - rcvF << "\t" << srcF - 27e6 << "\n";
		// cout << t << "\t" << err << "\n";
		/****
		if (iters > params.iters / 5) {
			err += (srcF - rcvF) * (srcF - rcvF);
		}
		****/
	}
	cerr << "max counter is " << max << "\n";

		// Clean up.
	delete Filt;
} // end main()


////////
//////// Local functions
////////
/* BuildList
 *
 * Read command-line arguments into 'args' structure.
 *
 * Results:
 *	'args' is modified.
 *
 * Side effects:
 *	None.
 */
void BuildList(argList& args, int c, char** v)
{
	param_list list;
	list.add_int("Iterations", "-i", args.iters, 300);
	list.add_float("Natural VCO Frequency", "-vf", args.natVCOFreq,
			27.001e+6);
	list.add_float("Max network jitter", "-j", args.maxJitter, 1.0e-3);
	list.add_int("Clock ticks per timestamp", "-t",
			args.ticksPerTstamp, 2700000);
	list.scan(c, v);
	list.free_mem();
} // end BuildList()


/* SrcFreq
 *
 * Return random source clock frequency, nominally 27MHz,
 * within MPEG2 jitter tolerance.
 *
 * Side Effects:
 *	Local static variables modified.
 */
double SrcFreq(unifRandClass& urc)
{
	const double NominalFreq = 27.0e+06;

#if 1
		// max variation 75e-4 Hz/timestep = 75e-3 Hz/sec.
	const double SrcVar = 75e-4;
	// const double SrcVar = 75e-5; // LESS THAN ALLOWED!

	static double freq = NominalFreq;

	freq += SrcVar * urc();
	if (freq > 27000540) { freq = 27000540; }
	if (freq < 26999460) { freq = 26999460; }

	return freq;
#else

	return NominalFreq;
#endif
} // end SrcFreq()


/* NextSched
 *
 * Return (random) number of clock ticks till next timestamp.
 *
 * Side Effects:
 *	None.
 */
int NextSched(unifRandClass& urc, const int ticksPerTstamp)
{
#if 0
	double rval = fabs(urc()); // No negative scheduling times.

		// Inter-packet variation can be up to 3x nominal spacing.
		// But, current tstamp cannot come before the previous one.
		// We model inter-timestamp spacing COARSELY.
	return (2 * ticksPerTstamp * rval);
#else
	return ticksPerTstamp;
#endif
} // end NextSched()
