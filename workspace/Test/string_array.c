#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>


char *fruit[] = { "apple", "orange", "banana" };

int main()
{
    int i;
    for (i=0; i<3; i++) {
        printf("%d: %s\n", i, fruit[i]);
    }
}
