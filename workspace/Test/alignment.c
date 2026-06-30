#include <stdio.h>

int buf[16];

int main()
{
    char *ptr1 = buf;
    int *ptr2;

    ptr1++;
    ptr2 = (int*)ptr1;
    printf("ptr1 %08x, ptr2 %08x\n", (int)ptr1, (int)ptr2);
    *ptr2 = 0;

    return 0;
}
