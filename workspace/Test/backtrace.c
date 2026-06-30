#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>


#define MAX_TRACE_FRAMES          32


void print_trace (void)
{
    void *array[MAX_TRACE_FRAMES];

    int nframe = backtrace(array, MAX_TRACE_FRAMES);
    printf("Number of frames in stack: %d\n", nframe);

    int i;
    char** strings = backtrace_symbols(array, nframe);
    for (i=0; i<nframe; i++) {
        printf("%s\n", strings[i]);
    }
}


void my_func (void)
{
    print_trace();
}


int main (void)
{
    my_func();
    return 0;
}
