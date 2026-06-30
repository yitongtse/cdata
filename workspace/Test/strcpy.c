#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ddi.h>

char line[32];
char name[6];


int main()
{
    int len;
    printf("Enter a string: ");
    gets(line);

    len = strlcpy(name, line, 6);
    printf("len %d, in %s\n", len, name);
    return 0;
}
