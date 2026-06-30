#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char** argv)
{
    int i;
    for (i=1; i<=120; i++) {
        printf("ssm SSM-%03d source 2.2.2.2 group 232.2.0.%d\n", i+120, i);
    }
    return 0;
}
