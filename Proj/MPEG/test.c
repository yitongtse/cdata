#include <stdio.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>


unsigned int get_ip_addr(char* dotted)
{
    int i, x;
    unsigned int addr = 0;

    char* s = (char*)strtok(dotted, ".");

    for (i=0; i<4; i++) {
        if (s == NULL) {
            printf("Input format error\n");
            return -1;
        }
        sscanf(s, "%d", &x);
        if (x<0 || x>255) {
            printf("Number error\n");
            return -1;
        }
        addr = (addr << 8) | x;
        s = (char*)strtok(NULL, ".");
    }

    return addr;
}


int main(int argc, char** argv)
{
    unsigned int addr;

    printf("Dotted IP address: %s\n", argv[1]);

//    addr = inet_addr(argv[1]);  
    addr = get_ip_addr(argv[1]);
    printf("Binary IP address: %x\n", addr);
}
