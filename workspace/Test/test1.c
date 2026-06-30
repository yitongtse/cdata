#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

unsigned char func (int x)
{
    return x;
}

int main()
{
    int x = func(260);
    printf("x = %d\n", x);
    return 0;
}
