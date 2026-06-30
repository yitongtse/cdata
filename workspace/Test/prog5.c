#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

char line[16];

int main()
{
    char *token;

    printf("Enter a string: ");
    gets(line);

    token = strtok(line, "-");
    printf("Start: %d\n", atoi(token));

    token = strtok(NULL, ":");
    if (!token) {
        return 0;
    }
    printf("Stop: %d\n", atoi(token));

    token = strtok(NULL, "");
    if (!token) {
        return 0;
    }
    printf("Inc: %d\n", atoi(token));

    return 0;
}
