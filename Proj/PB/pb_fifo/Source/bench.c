#include "bench.h"

/*
 * Icache and Dcache enable
 */
void enable_cache( void ) {
  	register unsigned long temp;
	__isync( );
	asm { mfspr temp, 1008 }	// read HID0 to temp
	__isync( );
	temp |= 0x0000c000;			// enable Icache and Dcache
	__isync( );
	asm { mtspr 1008,temp }		// write back to HID0
    __isync();
}


/*
 * Time base enable
 */
void enable_timer( void ) {
  	register unsigned long temp;
	__isync( );
	asm { mfspr temp, 1008 }	// read HID0 to temp
	__isync( );
	temp |= 0x04000000;			// set TBEN
	__isync( );
	asm { mtspr 1008,temp }		// write back to HID0
    __isync();                  
}



asm void record_time(time_entry_t *record) {
/*
 * Need to turn off optimization to prevent the code to be rearranged
 */
#pragma push
#pragma optimization_level 0	// lowest level of optimization
#pragma scheduling off			// turn off default scheduler setting

loop:
   nofralloc
   mfspr r4,269         // Read TBH
   mfspr r5,268         // Read TBL
   mfspr r6,269         // Read TBH again
   subf. r6,r4,r6       // Did it increment between reading TBU and TBL
   bgt- loop            // if it did, re-read
   stw r4,0(r3)
   stw r5,4(r3)
   blr
   
#pragma pop				// return to original pragma setting
}


double get_time_elapsed(time_entry_t* start, time_entry_t* end)
{
   float TH = (float)end->hi - (float)start->hi;
   float TL = (float)end->lo - (float)start->lo;
   return (TH*4294967296. + TL) * SPEED_FACTOR;
}
