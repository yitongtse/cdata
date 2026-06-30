#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>


char addr_str[] = "1.2.3.4";
unsigned int addr[4];
char addr_buf[INET_ADDRSTRLEN];


int main (int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <IPv4_or_v6_addr>\n", argv[0]);
        return -1;
    }

    printf("Inet address string: %s\n", argv[1]);

    int af = AF_INET;           // default
    int rc = inet_pton(af, argv[1], addr);
    if (!rc) {
        af = AF_INET6;
        rc = inet_pton(af, argv[1], addr);
        if (!rc) {
            printf("Error: not a valid IP address\n", argv[1]);
            return -1;
        }
    }

    if (af == AF_INET) {
        printf("Addr: %08x", ntohl(addr[0]));
        printf(" (%d ... %d)", (addr[0]>>24) & 0xFF, addr[0] & 0xFF);
    } else {
        printf("Addr: %08x-%08x-%08x-%08x, ver %d\n\n",
               ntohl(addr[0]), ntohl(addr[1]), ntohl(addr[2]), ntohl(addr[3]));
    }
    printf(", ver %d\n\n", af);

    return 0;
}
