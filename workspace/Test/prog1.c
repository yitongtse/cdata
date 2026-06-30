#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>


typedef struct {
    int x : 14;
} data_t;


int main()
{
    data_t data;
    data.x = 0x3FFF;
    printf("%d\n", data.x);
}
