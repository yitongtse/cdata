#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#define check_range(x, nbits)                                             \
    if ( ((x) >= 0? (x) : -(x)) & (~((((uint64_t)1) << (nbits)) - 1)) ) {   \
        printf("Range violation in %s:%d (%016llx)\n",                    \
               __FILE__, __LINE__, (int64_t)(x));                           \
    }


int64_t rshift64 (int64_t hi, uint64_t lo, int pos)
{
    if (pos < 64) {
        check_range(hi, pos);
        return (hi << (64 - pos)) | (lo >> pos);

    } else {
        check_range(hi, 128 - pos);
        return (hi >> (pos - 64));
    }
}


int main (int argc, char** argv)
{
    int i;
    int64_t lo = 0x123456789abcdef0;
    uint64_t hi = 0x11223344;

    for (i=0; i<=72; i+=4) {
        printf("%d: %llx\n", i, rshift64(hi, lo, i));
    }
}
