#include <stdio.h>
#include <stdlib.h>

int main ()
{
    unsigned char buf[188 * 10];

    FILE *fp = fopen("test.dat", "rb");
    if (fp == NULL) {
        perror("open");
        exit(-1);
    }
    while (1) {
        int n = fread(buf, 188, 10, fp);
        if (n < 10) {
            printf("Only %d read.  Rewinding...\n", n);
            rewind(fp);
            int n2 = fread(buf + 188*n, 188, 10-n, fp);

            if (n + n2 != 10) {
                printf("Fail to read data\n");
                exit(-1);
            }
            n += n2;
        }
        printf("%d TP read\n", n);
        sleep(1);
    }
    return 0;
}
