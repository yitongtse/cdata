#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "block.h"


/* PGM format:
        P5
        width height
        max_value
        [ width x height pixels, 1 byte each in binary ]

   File size = data size + 18 bytes
*/

/* Read PGM header from file
*/
int readPgmHdr(FILE *fp, int *width, int *height, int *maxValue)
{
    char temp[8];
    fscanf(fp, "%s", temp);
    if (strcmp(temp, "P5")) {
        printf("Error: File is not a PGM file\n");
        return -1;
    }
    fscanf(fp, "%d %d", width, height);
    fscanf(fp, "%d%c", maxValue, temp);
}


/* Read PGM data from file
*/
int readPgmData(FILE *fp, uchar *buf, int width, int height, int pitch)
{
    if (readUcharBlock(fp, buf, width, height, pitch)==-1) {
        printf("readPgmData failed\n");
        exit(-9205);
    }

	/* Rui_B */
	return(0);
	/* Rui_E */
}


/* Write PGM to file
*/
int writePgm(char *filename, uchar *buf, int width, int height,
             int maxValue, int pitch)
{
    FILE *fp;
    fp = fopen(filename, "w");
    if (fp==NULL) {
        printf("Failed to open file %s for writing!\n", filename);
        return 0;
    }
    fprintf(fp, "P5\n%4d %4d\n%4d\n", width, height, maxValue);
    writeUcharBlock(fp, buf, width, height, pitch);
    fclose(fp);
}

