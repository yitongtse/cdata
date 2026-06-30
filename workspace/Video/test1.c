#include <stdio.h>
#include <string.h>

#define xstr(s)  str(s)
#define str(s)  #s

int main(void)
{
    printf("Hello " xstr(USER) "!\n");
    return 0;
}
