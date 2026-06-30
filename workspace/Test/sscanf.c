#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main (int argc, char** argv)
{
    int x = -1;
    char* ptr = argv[1];

    int rc = sscanf(ptr, "Gi%d", &x);
    printf("rc %d, x %d\n", rc, x);
}

