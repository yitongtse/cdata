/*
   Copyright (c) 2006 by Cisco Systems, Inc.
   All rights reserved.
  
   File: hex2bin.c
   Converts a hex file (generated with bin2hex) to binary format.

 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main (int argc, char** argv)
{
    FILE *fp_in, *fp_out;
    int size;
    int val;
    unsigned char byte;
    unsigned char buf[66];
    unsigned char *ptr;

    if (argc != 3) {
        printf("Usage: %s  hex_file  bin_file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fp_in = fopen(argv[1], "r");
    if (fp_in == NULL) {
        fprintf(stderr, "Failed to open hex file %s\n", argv[1]);
    }

    fp_out = fopen(argv[2], "w");
    if (fp_out == NULL) {
        fprintf(stderr, "Failed to open binary file %s\n", argv[2]);
    }

    while (1) {
        ptr = fgets(buf, sizeof(buf), fp_in);

        if (ptr == NULL) {
            return EXIT_SUCCESS;
        }

        while (*ptr && *ptr != 10) {
            val = *ptr - '0';
            if (val < 0 || val > 9) {
                val = *ptr - 'a' + 10;
            }
            ptr++;
            byte = val << 4;

            val = *ptr - '0';
            if (val < 0 || val > 9) {
                val = *ptr - 'a' + 10;
            }
            ptr++;
            byte |= val;

            fwrite(&byte, 1, 1, fp_out);
        }
    }
}

