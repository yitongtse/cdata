#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

char* get_sign (int state)
{
    if (state > 0) {
        return "+ve";
    }
    if (state < 0) {
        return "-ve";
    }
    return "";
}


int main (int argc, char** argv)
{
    int x;

    for (x=-3; x<=3; x++) {
        printf("%d: %s\n", x, get_sign(x));
    }
}
