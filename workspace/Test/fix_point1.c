#include <stdio.h>

typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;


#define MASK_LO         0x0F


int8 add16_with_carry (int8 x, int8 y, int8* carry)
{
    int8 z = x + y + *carry;
    *carry = ((((x ^ y) & (z ^ 0x80)) | (x & y)) >> 7) & 1;
    return z;
}


void mult16 (int8 x, int8 y, int8* hi, uint8* lo)
{
    int8 xh, xl, yh, yl, mid1, mid2, c1, c2;

    xh = x >> 4;
    xl = x & MASK_LO;
    yh = y >> 4;
    yl = y & MASK_LO;

//printf("\nx: %2x.%01x  (%d)\n", (uint8)xh, xl, x);
//printf("y: %2x.%01x  (%d)\n", (uint8)yh, yl, y);

    *lo = xl * yl;
    mid1 = xl * yh;
    mid2 = xh * yl;
    *hi = xh * yh;
//printf("lo: %04x  %d\n", (uint8)*lo, *lo);
//printf("m1: %03x  %d\n", (uint8)mid1, mid1);
//printf("m2: %03x  %d\n", (uint8)mid2, mid2);
//printf("hi: %02x  %d\n", (uint8)*hi, *hi);

    c1 = c2 = 0;
    *lo = add16_with_carry(*lo, mid1 << 4, &c1);
    *lo = add16_with_carry(*lo, mid2 << 4, &c2);
    *hi = add16_with_carry(*hi, mid1 >> 4, &c1);
    *hi = add16_with_carry(*hi, mid2 >> 4, &c2);
//printf("Ans: %x %x\n", *hi, *lo);
}


int main()
{
    int x, y;
    int8 x2, y2, h;
    uint8 l;
    int ans;

    for (x=-128; x<=127; x++) {
        for (y=-128; y<=127; y++) {
            x2 = (int8)x;
            y2 = (int8)y;
            mult16(x2, y2, &h, &l);

            ans = h;
            ans = (ans << 8) | l;

            if (ans != x * y) {
                printf("Fail: x %d, y %d, ans %d, %d (%x)\n",
                       x, y, ans, x * y, x * y);
                return;
            }
        }
    }

    printf("Test pased!\n");
}


