#include <stdio.h>
#include "bench.h"

#define	LOAD_DATA		1

#define	DATA_SIZE		122880
				// data size in bytes

// Global variables
#if LOAD_DATA
#include "ip_1x2z.h"
#endif

int* dataBuf = (int*)0x07000000;	// hardware coded address


int find_check_sum(int* ptr, int nbytes)
{
	int chksum = 0;
	for ( ; nbytes>0; nbytes-=4)
	    chksum ^= *ptr++;
	return chksum;
}


void main()
{
	int chksum;
	int cycles;
	time_entry_t start, stop;
	
    enable_timer();	
	enable_cache();
	
#if LOAD_DATA
    memcpy(dataBuf, rawData, DATA_SIZE);
#endif

	record_time(&start);	

	chksum = find_check_sum(dataBuf, DATA_SIZE);

	record_time(&stop);
	
	cycles = (int)get_time_elapsed(&start, &stop);
}
