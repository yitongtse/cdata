#define SIGN_64         (1 << 63)
#define MASK_LO         0x00000000FFFFFFFF


// Add two 64-bit integers with carry-in
// Return the sum, and update carry with carry-out
//
int64 add64_w_carry (int64 x, int64 y, int* carry)
{
    int64 z = x + y + *carry;
    *carry = (((x & y) | ((x ^ y) & (z ^ SIGN_64))) >> 63) & 1;
    return z;
}


// Multiply two 64-bit integers to form a 128-bit result 
//
void mul64 (int64 x, int64 y, int64* hi, uint64* lo)
{
    int64 xh, xl, yh, yl, mid1, mid2;
    int c1, c2;

    xh = x >> 32;
    xl = x & MASK_LO;
    yh = y >> 32;
    yl = y & MASK_LO;

    *lo = xl * yl;
    mid1 = xl * yh;
    mid2 = xh * yl;
    *hi = xh * yh;

    c1 = c2 = 0;
    *lo = add64_w_carry(*lo, mid1 << 32, &c1);
    *lo = add64_w_carry(*lo, mid2 << 32, &c2);
    *hi = add64_w_carry(*hi, mid1 >> 32, &c1);
    *hi = add64_w_carry(*hi, mid2 >> 32, &c2);
}


// Right shift a 128-bit integer to get a 64-bit result
//
int64 rshift64 (int64* hi, uint64* lo, int pos)
{
    return (hi << (64 - pos)) | (lo >> pos);
}
