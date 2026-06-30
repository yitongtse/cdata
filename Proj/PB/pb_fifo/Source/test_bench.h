#ifndef TEST_BENCH_H
#define TEST_BENCH_H

#define MAX_ENTRY 10

#define CPU_SPEED 400 // 400MHz CPU

#define MPX 0

#ifdef MPX
#define BUS_SPEED 133
#else // 107
#define BUS_SPEED 100
#endif

typedef struct {
   unsigned int hi;
   unsigned int lo;
} time_entry_t;

void init_time_records(void);
void add_time_entry(void);
time_entry_t *get_time(int entry);
float get_time_elapsed(int start, int end);
#endif