#include <stdio.h>
#include <stdint.h>

int main()
{
    int x = 0x7FFFFFFF;
    int y = 10;
    long long z = (long long)x + y;
    printf("z: %ld\n", z);
}
