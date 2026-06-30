#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


char* label = "Test";
char buf[10];


void show_addr (char *string)
{
    printf("Addr %08x: [%s]\n", (size_t)string, string);
}


int main()
{
    show_addr("Test");
    show_addr(label);
    show_addr("Test");
    sprintf(buf, "%s", label);
    show_addr(buf);
    return 0;
}
