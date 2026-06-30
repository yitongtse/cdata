#include <stdio.h>

typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;


#if 0
int16 ring_mod (int16 idx, int16 size)
{
   return (idx + size) % size;
}


int32 find_avail_space (int16 rd, int16 wr)
{
    int32 space_left = 1020 - ring_mod(wr - rd, 1024);

    if (space_left < 0) {
        space_left = 0;
    }
    return space_left;
}


int main (void)
{
    int16 rd = 580;
    int16 wr = 92;
    uint16 space = find_avail_space(rd, wr);
    printf("space: %d\n", space);
    return 0;
}
#endif


int main (void)
{
    uint16 x = 10;
    int16 y = -x / 2;
    printf("Y = %d, %d, %u\n", y, -x, -x);
    return 0;
}
