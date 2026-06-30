#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


char *my_opts[] = {
#define READONLY    0
        "ro",
#define WRITEONLY   1
        "wo",
#define READWRITE   2
        "rw",
        NULL
};


int main (int argc, char** argv)
{
    int v;
    char *options, *value;
    extern char *optarg;
    extern int optind;
    char c;

    while ((c = getopt(argc, argv, "abo:")) != -1) {
        switch (c) {
        case 'a':
            printf("Option a\n");
            break;

        case 'b':
            printf("Option b\n");
            break;

        case 'o':
            options = optarg;
            while (*options != '\0') {
                switch (getsubopt(&options, my_opts, &value)) {
                case READONLY:
                    printf("Option ro\n");
                    if (value) {
                        v = strtol(value, NULL, 0);
                        printf("  value %d\n", v);
                    }
                    break;

                case WRITEONLY:
                    printf("Option wo\n");
                    if (value) {
                        v = strtol(value, NULL, 0);
                        printf("  value %d\n", v);
                    }
                    break;

                case READWRITE:
                    printf("Option rw\n");
                    if (value) {
                        v = strtol(value, NULL, 0);
                        printf("  value %d\n", v);
                    }
                    break;
                }
            }
            break;

        case '?':
            exit (-1);
        }
    }

    for (; optind<argc; optind++) {
        printf("argument %d: %s\n", optind, argv[optind]);
    }
}
