#include <stdio.h>
#include <stdint.h>

typedef char int8;
typedef unsigned char uint8;
typedef signed char sint8;


int main()
{
    uint8 a = 68;
    uint8 b = 192;

    int8 c = (int8)(a - b);
    printf("c = %d\n", c);

    sint8 d = (sint8)(a - b);
    printf("d = %d\n", d);
}
