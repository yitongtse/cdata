#include <stdio.h>
#include <string.h>


typedef union {
  int i;
  unsigned int ui;
  char c;
  unsigned int ip_v6[4];
} data;


int main (void)
{
    data x;
    memset(&x, 0, sizeof(x));
    x.i = 0x123456;

    printf("i: %x\n", x.i);
    printf("u: %ux\n", x.ui);
    printf("c: %c\n", x.c);
    printf("ipv6: %08x-%08x-%08x-%08x\n",
           x.ip_v6[0], x.ip_v6[1], x.ip_v6[2], x.ip_v6[3]);

    return 0;
}
