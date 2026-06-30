#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#define MAX     ((1 << 20) - 1)

typedef int             int32;
typedef unsigned     uint32;


int32 func1 (uint32 x, uint32 y)
{
    return ((int32)((x << 12) - (y << 12))) >> 12;
}


int32 func2 (uint32 x, uint32 y)
{
    return ((int32)((x - y) << 12)) >> 12;
}


int main (int argc, char **argv)
{
    uint32 x, y;
    for (x = 0; x < MAX * 32; x += 1000) {
        for (y = 0; y < MAX * 32; y += 1000) {
            int32 a = func1(x, y);
            int32 b = func2(x, y);
            if (a != b)  printf("x %d, y %d: a %d, b %d\n", x, y, a, b);
        }
    }
}

