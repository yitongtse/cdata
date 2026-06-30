#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int main()
{
    int a[10];
    int *p = a;
    int s;

    printf("p: %08xl\n", (long)p);
    s = sizeof(p++);
    printf("s: %d\n", s);
    printf("p: %08xl\n", (long)p);
}
