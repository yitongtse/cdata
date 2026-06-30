/*
  Testing of function
    strstr();
    strsep();
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


//char line[] = "This is a line\r\n";
//char line[] = "This is a line\n";
char line[] = "";


int main()
{
    char* ptr = strstr(line, "\r\n");
    printf("line %08x, ptr %08x\n", (size_t)line, (size_t)ptr);
    return 0;
}
