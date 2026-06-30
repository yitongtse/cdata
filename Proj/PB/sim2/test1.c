#define QNX			0
#define	DINK		1

#include <stdio.h>
#include <stdlib.h>

#if QNX
#include <sys/neutrino.h>
#include <inttypes.h>
#include <sys/syspage.h>
#endif

#if DINK
#include "bench.h"
#endif


#define	CPU_CLOCK_RATE		864000000

#define	DELAY				10000


int main(int argc, char** argv)
{
    int delay;
    int i, j;
    int temp=0;
    double time;
    
#if QNX
    uint64_t start, stop, cycles;
    int scale;
#endif

#if DINK
	time_entry_t start, stop;
	double cycles;
	
	enable_cache();
	enable_timer();
#endif

    delay = DELAY;

#if QNX
    start = ClockCycles();
#endif

#if DINK
	record_time(&start);	
#endif

    for (i=0; i<delay; i++)
        for (j=0; j<1000000; j++) {
            temp += j;
        }

#if QNX
    stop = ClockCycles();
    scale = CPU_CLOCK_RATE / SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    cycles = (stop - start) * scale;
    printf("Delay parameter: %d\n", delay);
    printf("Cycles: %lld\n", cycles);
    time = (double)cycles;
    printf("Time: %.2f sec\n", time/CPU_CLOCK_RATE);
    printf("CPU clock rate: %lld\n", SYSPAGE_ENTRY(qtime)->cycles_per_sec);
    printf("Result: %d\n", temp);
    return EXIT_SUCCESS;
#endif

#if DINK
	record_time(&stop);
	cycles = get_time_elapsed(&start, &stop);
	time = cycles / (double)CPU_CLOCK_RATE;
#endif

	return 0;
}
