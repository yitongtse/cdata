static const char SccsId[] = "@(#)acq.cc	1.1 6/24/94";

/**********************************************************************/
//
//	Title:
//
//		acq.cc
//
//	Description:
//
//		Calculate acquisition time, frequency-error std. dev.,
//		and frequency drift-rate std. dev. of a given PLL
//		loop filter.
//
//
//	Revisions:
//
//		Haskell		6/23/94		creadtion date
//
//
//	Argument or Option List:
//
//	Comments:
//
/**********************************************************************/


//////
////// Include files
//////
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "levelstream.h"
#include "complex.h"
#include "cli.h"
#include "param_list.h"


//////
////// Local type declarations
//////
class argList {
public:
	int length;
	int filtLen;
	float gain;
	float Tts;
};


//////
////// Local functions
//////
/* AcquireXfer()
 *
 * In the array 'p', return 'LEN' samples of the z transform
 * version of one of the loop transfer functions. The function
 * returned depends on 'TYPE'.
 */
void AcquireXfer(const int TYPE, complex* p, const int LEN,
	complex* FiltTaps, const int FILTLEN, const float gain,
	const float tSamp)
{
	complex norm(0, 2 * M_PI / LEN);

// Calculate z xform at equally spaced points around the unit circle.
	for(int freq = 0; freq < LEN; freq++) {
		complex z(exp(norm*freq));
		complex zinv(exp(-1.0*norm*freq));
		complex zpow(1);
		complex numer;
		for(int k = 0; k < FILTLEN; k++) {
			numer += FiltTaps[k] * zpow;
			zpow *= zinv;
		}

		numer *= gain;
		complex denom(z - 1 + numer*tSamp);

		switch (TYPE) {
			case 1:
				numer *= complex(z - 1.0);
				break;
			case 2:
				denom *= complex(z * tSamp);
				numer *= complex(z - 1.0);
				break;
			case 3:
				numer *= z;
				break;
			default:
				cerr << "Bad TYPE arg\n"; exit(1);
				break;
		} // end switch()

		p[freq] = numer / denom;

	} // end for(freq)
} // end AcquireXfer()


void BuildList(argList& params, int argc, char** argv)
{
	param_list list;
	list.add_int("Iterations", "-n", params.length, 1024);
	list.add_int("Filter length", "-f", params.filtLen, 64);
	list.add_float("Loop gain", "-g", params.gain, 0.0001);
	list.add_float("Clk sample period", "-t", params.Tts, 0.1);
	list.scan(argc, argv);
	list.free_mem();
}


//////
////// Main module
//////
main(int argc, char** argv)
{
// Command line arguments.
	argList params;
	BuildList(params, argc, argv);

// Build FIR loop filter.
	complex* FiltTaps = new complex[params.filtLen];
	const complex one(1);
	for(int i = 0; i < params.filtLen; i++) {
		FiltTaps[i] = one / params.filtLen;
	}

// Storage array for transfer functions.
	complex* p = new complex[params.length];

// Noise analysis.
	AcquireXfer(1, p, params.length, FiltTaps, params.filtLen,
			params.gain, params.Tts);
	cerr << "Done generating freq-noise transfer function.\n";

	double noise = 0.0;
	for(i = 0; i < params.length; i++) { noise += mag2(p[i]); }
	noise /= params.length;
	cerr << "Frequency noise multiplier is " << noise << "\n";

// Clock-slew analysis.
	AcquireXfer(2, p, params.length, FiltTaps, params.filtLen,
			params.gain, params.Tts);
	cerr << "Done generating clock-slew transfer function.\n";

	noise = 0.0;
	for(i = 0; i < params.length; i++) { noise += mag2(p[i]); }
	noise /= params.length;
	cerr << "Clock-slew noise multiplier is " << noise << "\n";

// Acquisition analysis.
	AcquireXfer(3, p, params.length, FiltTaps, params.filtLen,
			params.gain, params.Tts);
	cerr << "Done generating acquisition transfer function.\n";

	ifft(p, params.length);
	cerr << "Done with IFFT\n";

// Integrate to get ramp response.
	p[0] = real(p[0]);
	i = 0;
	do {
		i++;
		p[i] = p[i-1] + real(p[i]);

		// cout << i << " " << mag(p[i]) << "\n";

	} while ((real(p[i]) < 0.99/params.Tts) && (i < params.length));
	cerr << i << "\n";

	delete [] p;
	delete [] FiltTaps;
} // end main()
