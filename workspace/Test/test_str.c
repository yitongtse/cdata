#include <stdio.h>
#include <strings.h>


char *strs[] = {"foo", "bar", "Hello",  "World", ""};


int main()
{
    printf("size %d\n", sizeof(strs));

    int i;
    for (i=0; i<5; i++) {
        printf("%d: %s\n", i, strs[i]);
    }
}
