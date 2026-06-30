#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef unsigned short uint16;
typedef unsigned int uint32;

uint16 data[4] = { 0x1234, 0x2468, 0x7531, 0xbeef };


void udp_checksum_part (uint16 *buf, int nbytes, uint32 *sum)
{
    while (nbytes > 1) {
        *sum += *buf++;
        if (*sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16)
        }
        nbytes -= 2;
    }
    if (nbytes == 1) {
        *sum += *((uint8*)buf);
    }
}


uint6 udp_checksum (uint32 sum)
{
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16)(~sum);
}


int main (void)
{
    uint16 sum = 0;
    checksum1(data, sizeof(data), &sum);
    sum = 0xFFFF ^ sum;
    printf("checksum: %04x\n", sum);

    sum = checksum2(data, sizeof(data));
    sum = 0xFFFF ^ sum;
    printf("checksum: %04x\n", sum);
}

------------------------

void udp_checksum_part (uint16 *buf, int nbytes, uint16 *sum)
{
    uint32 temp = *sum;

    while (nbytes > 1) {
        temp += *buf++;
        if (temp & 0x80000000) {
            temp = (temp & 0xFFFF) + (sum >> 16)
        }
        nbytes -= 2;
    }

    if (nbytes == 1) {
        temp += *((uint8*)buf);
    }

    while (temp >> 16) {
        temp = (temp & 0xFFFF) + (temp >> 16);
    }

    *sum = temp;
}
