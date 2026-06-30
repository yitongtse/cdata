#include <stdlib.h>
#include <inttypes.h>


int x = 10;


int main (void)
{
    size_t p = (size_t)&x;

    printf("Addr: %"PRIuPTR"\n", p);
}
