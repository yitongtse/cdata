/*
   Copyright (c) 2006 by Cisco Systems, Inc.
   All rights reserved.

   File: bin2hex.c
   Converts a binary file to hex format.

 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#ifndef UNIX
    struct timespec show_delay = {0, 1000};
#endif


int main (int argc, char** argv)
{
    int fd;
    int size;
    int i;
    unsigned char buf[32];

    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Failed to open file %s\n", argv[1]);
    }

    while (1) {
        size = read(fd, buf, sizeof(buf));
        if (size <= 0) {
            return EXIT_SUCCESS;
        }

        for (i=0; i<size; i++) {
            printf("%02x", buf[i]);
        }
        printf("\n");

#ifndef UNIX
        nanosleep(&show_delay, NULL);
#endif
    }
}

