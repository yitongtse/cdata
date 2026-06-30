#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


unsigned int x, y;
unsigned int x2, y2;
int diff, diff2;


int main(int argc, char** argv)
{
    printf("X = ");  scanf("%d", &x);
//    printf("Y = ");  scanf("%d", &y);

    x2 = x << 28;


    for (y=0; y<15; y++) {
        y2 = y << 28;

        diff = (int)((y - x) << 28);
        diff >>= 28;

        diff2 = (int)(y2 - x2);
        diff2 >>= 28;

        printf("X = %d, Y = %d: Diff %d, Diff2 %d\n", x, y, diff, diff2);
    }
}

