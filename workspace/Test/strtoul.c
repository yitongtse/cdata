#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char line[] = "10 100 1000";

int main()
{
    unsigned long x;
    char* str;
    char *tmp;

    str = strtok_r(line, " ", &tmp);
    x = strtoul(str, NULL, 0);
    printf("S1: %u\n", x);

    str = strtok_r(NULL, " ", &tmp);
    x = strtoul(str, NULL, 0);
    printf("S2: %u\n", x);

    str = strtok_r(NULL, " ", &tmp);
    x = strtoul(str, NULL, 0);
    printf("S3: %u\n", x);

    str = strtok_r(NULL, " ", &tmp);
    if (str == NULL) {
        printf("Str is null\n");
        return 0;
    }

    x = strtoul(str, NULL, 0);
    printf("S4: %u\n", x);

    return 0;
}

