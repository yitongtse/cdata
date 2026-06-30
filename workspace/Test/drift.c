#include <stdio.h>
#include <stdint.h>


typedef int64_t mpeg_time_t;

#define FRAC_BITS_TIME          20
#define SCALE_BASE_EXT          600


uint32_t mpeg_time_to_base (mpeg_time_t t)
{
    return (uint32_t)((t >> FRAC_BITS_TIME) / SCALE_BASE_EXT);
}


int main (void)
{
    int i;
    mpeg_time_t x = 1;

    for (i=0; i<64; i++, x <<= 1) {
        printf("%d: %lld, %d\n", i, x, mpeg_time_to_base(x));
    }

    return 0;
}
