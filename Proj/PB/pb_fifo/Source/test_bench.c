#include "test_bench.h"

static time_entry_t time_entry[MAX_ENTRY];
static int entry_count;
int tint = 0;

asm record_time(time_entry_t *record) {
loop:
   nofralloc
   mfspr r4,269         // Read TBH
   mfspr r5,268         // Read TBL
   mfspr r6,269         // Read TBH again
   subf. r6,r4,r6       // Did it increment between reading TBU and TBL
   bgt- loop            //  if it did, re-read
   stw r4,0(r3)
   stw r5,4(r3)
   blr
}

void init_time_records() {
   int i;
   
   entry_count=0;
   
   for(i=0;i<MAX_ENTRY;i++) {
      time_entry[i].hi = 0;
      time_entry[i].lo = 0;
   }
}

#if 0
void add_time_entry() {
   record_time(&time_entry[entry_count++]);
}
#endif

void time_entry_begin() {
   record_time( &time_entry[0]);
}

void time_entry_end() {
   record_time( &time_entry[1]);
}

time_entry_t *get_time(int entry) {
   return (&time_entry[entry]);
}

float get_time_elapsed(int start, int end) {
   float TH = (float)time_entry[end].hi - (float)time_entry[start].hi;
   float TL = (float)time_entry[end].lo - (float)time_entry[start].lo;
   return (TH*4294967296. + TL) * 16.;
}
