#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>
#include <sys/types.h>


int put_header (char **ptr, char *header, char *format, ...)
{
    va_list arg_list;

    sprintf(*ptr, "%s: ", 
    va_start(arg_list, format);

    vsnprintf(*ptr, 256, "%s: " format, arg_list);

    va_end(arg_list);
}


int main()
{
    char buf[256];
    char *ptr = buf;

    int len = put_header(&ptr, "Cseq", "%d", 1234);
    printf("Size: %d\n", len);
    prnitf("Msg: %s\n", buf);
}
