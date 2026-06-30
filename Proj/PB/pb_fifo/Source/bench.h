#ifndef TEST_BENCH_H
#define TEST_BENCH_H

#define SPEED_FACTOR	26
// 864 MHz CPU, 133 MHz bus => 864/133 * 4 = 26


typedef struct {
   unsigned int hi;
   unsigned int lo;
} time_entry_t;


void enable_timer(void);
asm void record_time(time_entry_t* record);
double get_time_elapsed(time_entry_t* start, time_entry_t* end);

#endif	// TEST_BENCH_H