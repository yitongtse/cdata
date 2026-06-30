#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#define SCALE_BASE_EXT          600
#define FRAC_BITS_TIME          16


typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;
typedef int64_t         int64;
typedef uint64_t        uint64;


typedef int64 mpeg_time_t;      // mpeg time format in F48.16
typedef int64 mpeg_timediff_t;  // mpeg time difference format in F24.16


typedef struct mpeg_clock_ {
    uint32 base;                // in 45 kHz clock ticks
    int16  ext;                 // in 27 MHz clock ticks, from 0 to 599
} mpeg_clock_t;


// Set mpeg_clock_t
void mpeg_clock_set (mpeg_clock_t *c, uint32 base, uint16 ext)
{
    c->base = base;
    c->ext = ext;
}


// Convert mpeg_clock_t to mpeg_time_t
mpeg_time_t mpeg_clock_to_time (mpeg_clock_t *c)
{
    return ((((int64)c->base) * SCALE_BASE_EXT + c->ext) << FRAC_BITS_TIME);
}


// Convert mpeg_time_t to base and extension parts
void mpeg_time_to_clock (mpeg_time_t t, mpeg_clock_t *c)
{
    t >>= FRAC_BITS_TIME;
    c->base = (uint32)(t / SCALE_BASE_EXT);
    c->ext = (int16)(t - c->base * SCALE_BASE_EXT);
    if (c->ext < 0) {
        c->base--;
        c->ext += SCALE_BASE_EXT;
    }
}


// TBO = pcr - local_clk
// pcr = TBO + local_clk
//
int main (int argc, char** argv)
{
    mpeg_clock_t pcr, clk, tbo, pcr2;
    mpeg_time_t pcr_time, clk_time, pcr2_time;
    mpeg_timediff_t tbo_time, tbo2_time;

    mpeg_clock_set(&pcr, 0x12345678, 123);
    mpeg_clock_set(&clk, 0x2000abcd, 78);

    pcr_time = mpeg_clock_to_time(&pcr);
    clk_time = mpeg_clock_to_time(&clk);

    tbo_time = pcr_time - clk_time;

    mpeg_time_to_clock(tbo_time, &tbo);
    tbo2_time = mpeg_clock_to_time(&tbo);

    pcr2_time = clk_time + tbo2_time;
    mpeg_time_to_clock(pcr2_time, &pcr2);

    printf("In PCR : %08x:%d\n", pcr.base, pcr.ext);
    printf("Out PCR: %08x:%d\n", pcr2.base, pcr2.ext);
    printf("TBO    : %08x:%d\n", tbo.base, tbo.ext);
}
