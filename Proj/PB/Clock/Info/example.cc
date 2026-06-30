static const char SccsId[] = "%W% %G%";

/*********************************************************************
Title: timing.C

Description:
This program simulates timing recovery with a timestamp-based system.
The simulation expects incoming Program Clock Reference values
as well as samples (at the PCR arrival times) of a counter driven
by the local oscillator.

Given to Musa J on 9/28/94...
Given to Amanda C on 4/3/95...
Given to Tom C on 6/27/95...
*********************************************************************/


////////
//////// Include files
////////
#include <stdlib.h>
#include <unistd.h>
#include <stream.h>
#include <math.h>


////////
//////// Local type definition
////////
class LoopFilt {
public:
	LoopFilt(const int sz) : size(sz), buf(new int[sz]), count(0),
		index(0), sum(0.0) { }
	~LoopFilt() { delete [] buf; }
	void insert(const int);
	inline double avg() const;

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
{ return (sum/count); }


////////
//////// Main module
////////
main(int argc, char** argv)
{
	int srcMinusDiffCk = 0; // Diff between src and receive counters.
	double rcvF = params.natVCOFreq; // Source and receive freqs.

// Main loop.
	float gain = 0.1;
	LoopFilt* Filt = new LoopFilt(64);
	float compens = 0.0; // SET AT START-UP or TEST TIME.

		// The effect of the 'delete Filt; new LoopFilt(64);' lines
		// is to set all input values stored in the 64-tap FIR
		// filter to 0. The tap values all stay at 1/64.
		// It is important to do this when we switch from one
		// filter to another; otherwise the frequency of the local
		// oscillator jumps unacceptably.

		// The 'compens' variable stores a rarely-changing
		// adjustment to the local oscillator. The output
		// of (loop filter * gain) provides faster-changing
		// control of the oscillator.

	for(int iters = 1; iters < params.iters; iters++) {
		if (iters == 175) { // i.e. 175 PCR's received.
			compens += gain * Filt->avg();
			gain = 0.01;
			delete Filt;
			Filt = new LoopFilt(64);
		} else if (iters == 1000) { // i.e. 1000 PCR's received.
			compens += gain * Filt->avg();
			gain = 0.001;
			delete Filt;
			Filt = new LoopFilt(64);
		} else if (iters == 20000) { // i.e. 20k PCR's received.
			compens += gain * Filt->avg();
			gain = 0.0001;
			delete Filt;
			Filt = new LoopFilt(64);
		}

			// Update clock-difference value.
			// 'localCounter' IS FROM H/W.
		srcMinusDiffCk += newPCR - localCounter;

			// Update 'rcvF' via feedback loop.
		Filt->insert(srcMinusDiffCk);

			// Do some scaling here, to generate control value.
		rcvF = params.natVCOFreq + compens + Filt->avg() * gain;

	}

		// Clean up.
	delete Filt;
} // end main()
