#include <stdio.h>
#include <stdlib.h>


unsigned int rightmost_set_bit (unsigned int x)
{
    unsigned int x2 = -x;
    return (x & x2);
}


int main (int argc, char** argv)
{
    int i;
    unsigned int data = 0xab;

    for (i=0; i<10; i++) {
        printf("%08x: %08x\n", data, rightmost_set_bit(data));
        data <<= 1;
    }
    return 0;
}
