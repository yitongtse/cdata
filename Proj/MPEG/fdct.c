
// Cosine multiplier constants
static const double c0 = 0.98078528;
static const double c1 = 0.92387953;
static const double c2 = 0.83146961;
static const double c3 = 0.70710678;

// Sine multiplier constants
static const double s0 = 0.19509032;
static const double s1 = 0.38268343;
static const double s2 = 0.55557023;


static double tmp_8[8];
static double tmp_8x8[8][8];



// 8-point Fast Forward DCT
//
void fdct_8 (double* a)
{
    double x0, x1, x2, x3, x4, x5, x6, x7, y0, y1, y2, y3, y4, y5, y6, y7;

    // Step 1
    x0 = a[0] + a[7];
    x1 = a[1] + a[6];
    x2 = a[2] + a[5];
    x3 = a[3] + a[4];
    x4 = a[3] - a[4];
    x5 = a[2] - a[5];
    x6 = a[1] - a[6];
    x7 = a[0] - a[7];

    // Step 2
    y0 = x0 + x3;
    y1 = x1 + x2;
    y2 = x1 - x2;
    y3 = x0 - x3;

    y4 = x4;
    y5 = (x6 - x5) * c3;
    y6 = (x6 + x5) * c3;
    y7 = x7;

    // Step 3
    x0 = (y0 + y1) * c3;
    x1 = y2 * s1 + y3 * c1;
    x2 = (y0 - y1) * c3;
    x3 = y3 * s1 - y2 * c1;

    x4 = y4 + y5;
    x5 = y4 - y5;
    x6 = y7 - y6;
    x7 = y7 + y6;

    // Step 4
    a[0] = x0 / 2;
    a[1] = (x4 * s0 + x7 * c0) / 2;
    a[2] = x1 / 2;
    a[3] = (x6 * c2 - x5 * s2) / 2;
    a[4] = x2 / 2;
    a[5] = (x5 * c2 + x6 * s2) / 2;
    a[6] = x3 / 2;
    a[7] = (x7 * s0 - x4 * c0) / 2;
}


// Fast forward DCT for an 8x8 block
//
void fdct(short* block)
{
    int i, j;
    short *ptr;

    // Row transform
    for (i=0, ptr=block; i<8; i++) {
        for (j=0; j<8; j++)  tmp_8[j] = *ptr++;
        fdct_8(tmp_8);
        for (j=0; j<8; j++)  tmp_8x8[j][i] = tmp_8[j];
    }

    // Column transform
    for (i=0; i<8; i++) {
        fdct_8(tmp_8x8[i]);
        for (j=0, ptr=block+i; j<8; j++, ptr+=8)
            *ptr = (short)tmp_8x8[i][j];
    }
}
