#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


int main (int argc, char** argv)
{
    int i;
    int *ptr;
    int (*x)[10];
    int *y;
    int z[10];

    x = calloc(4, 4 * 10);

    ptr = (int*)x;    
    for (i=0; i<40; i++) {
        *ptr++ = i;
    }

    printf("Size of x is %d\n", sizeof(x));
    printf("Size of z is %d\n", sizeof(z));

    y = x[0];
    for (i=0; i<10; i++) {
        printf("%d: %d\n", i, y[i]);
    }
    y++;
    for (i=0; i<10; i++) {
        printf("%d: %d\n", i, y[i]);
    }

}
