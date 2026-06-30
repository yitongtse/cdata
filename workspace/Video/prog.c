#include <stdio.h>
#include <stdint.h>


int main()
{
#ifdef __LITTLE_ENDIAN__
    printf("Hello Big endian\n");
#else
    printf("Hello little endian\n");
#endif
    return 0;
}
