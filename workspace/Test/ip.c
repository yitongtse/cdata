#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>


int main (int argc, char** argv)
{
    unsigned char addr[sizeof(struct in6_addr)];
    int rc;
    int len;
    int i;

    if (argc != 3) {
        printf("Usage: %s [v4 | v6] <string>\n", argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "v4")) {
        printf("IPv4 address: %s\n", argv[2]);
        af = AF_INET;
        len = sizeof(in_addr_t);

    } else if (!strcmp(argv[1], "v6")) {
        printf("IPv6 address: %s\n", argv[2]);
        af = AF_INET6;
        len = sizeof(struct in6_addr);


    } else {
        printf("Unknown format %s\n", argv[1]);
        return 0;
    }

    rc = inet_pton(af, argv[2], addr);
    if (rc == 1) {
        printf("Binary:");
        for (i=0; i<len; i++) 
            printf(" %02x", addr[i]);
        printf("\n");

    } else if (rc == 0) {
        printf("Invalid address format\n");

    } else {
        printf("Error: %s\n", strerror(errno));
    }

    return 0;
}
