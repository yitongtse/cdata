#include <stdio.h>

typedef char            int8;
typedef unsigned char   uint8;
typedef short           int16;
typedef unsigned short  uint16;
typedef int             int32;
typedef unsigned int    uint32;


#define MASK_LO         0x00FF


int16 add32_with_carry (int16 x, int16 y, int16* carry)
{
    int16 z = x + y + *carry;
    *carry = (((x & y) | ((x ^ y) & (z ^ 0x8000))) >> 15) & 1;
    return z;
}


void mul32 (int16 x, int16 y, int16* hi, uint16* lo)
{
    int16 xh, xl, yh, yl, mid1, mid2, c1, c2;

    xh = x >> 8;
    xl = x & MASK_LO;
    yh = y >> 8;
    yl = y & MASK_LO;

    *lo = xl * yl;
    mid1 = xl * yh;
    mid2 = xh * yl;
    *hi = xh * yh;

    c1 = c2 = 0;
    *lo = add32_with_carry(*lo, mid1 << 8, &c1);
    *lo = add32_with_carry(*lo, mid2 << 8, &c2);
    *hi = add32_with_carry(*hi, mid1 >> 8, &c1);
    *hi = add32_with_carry(*hi, mid2 >> 8, &c2);
}


int main()
{
    int x, y;
    int16 x2, y2, h;
    uint16 l;
    int ans;

    for (x=-32768; x<=32767; x+=100) {
        for (y=-32768; y<=32767; y++) {
            x2 = (int16)x;
            y2 = (int16)y;
            mul32(x2, y2, &h, &l);

            ans = h;
            ans = (ans << 16) | l;

            if (ans != x * y) {
                printf("Fail: x %d, y %d, ans %d, %d (%x)\n",
                       x, y, ans, x * y, x * y);
                return;
            }
        }
    }

    printf("Test pased!\n");
}


