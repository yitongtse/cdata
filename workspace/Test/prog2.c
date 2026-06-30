#include <stdio.h>
#include <stdint.h>


void foo (const char **ptr)
{
    printf("Addr: %08x\n", (size_t)(*ptr));
    (*ptr) += 1;
}


int main()
{
    char buffer[128];
    char* buf1 = buffer;
    char* buf2 = buffer;
    const char* buf3 = buffer;

    foo(&buf1);
    foo((const char **)&buf2);
    foo(&buf3);

    return 0;
}
