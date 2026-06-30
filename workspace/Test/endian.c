#include <stdio.h>
#include <string.h>

typedef unsigned char           uint8;
typedef unsigned short          uint16;
typedef unsigned int            uint32;

uint8 buf[20];


typedef struct __attribute__((packed, aligned(1))){
    uint8  a;
    uint16 b;
    uint32 c;
//    uint8 d;
} data_t;


void dump(void)
{
    int i;
    for (i=0; i<20; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

int main()
{
    data_t* data = (data_t*)buf;
    memset(data, 0, 20);
    dump();

    data->a = 0xa1; 
    dump();

    data->b = 0xb1b2; 
    dump();

    data->c = 0xc1c2c3c4;
    dump();

//    data->d = 0xd1;
//    dump();

    data++;
    data->a = 0x81; 
    dump();

    data++;
    data->a = 0x91; 
    dump();

    return 0;
}

