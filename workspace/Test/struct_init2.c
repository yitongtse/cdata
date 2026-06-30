#include <stdio.h>

typedef unsigned int uint32;

typedef struct {
    uint32 f1 : 1;
    uint32 f2 : 4;
    uint32 f3 : 7;
    uint32 f4 : 20;
} key;


void print_key(key *k)
{
    printf("f1 %d, f2 %d, f3 %d, f4 %d\n", k->f1, k->f2, k->f3, k->f4);
}


int main()
{
    key k[2] = {{1, 2, 4, 6}, {0, 1, 0, 3}};
    print_key(&k[0]);
    print_key(&k[1]);
    return 0;
}
