#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


/* Compute Total Squared Error (TSE) between two blocks
*/
int blockTse(uchar* buf1, uchar* buf2, int width, int height, int pitch)
{
    int x, y, diff, dist=0;
    pitch -= width;
 
    for (y=height; y>0; y--, buf1+=pitch, buf2+=pitch)
        for (x=width; x>0; x--) {
            diff = *buf1++ - *buf2++;
            dist += diff * diff;    
        }
    return dist; 
}


/* Compute variance of a block
*/
int blockVar(uchar* blk, int width, int height, int pitch)
{
    int x, y, value;
    int sum=0, sumSq=0;
    pitch -= width;

    /* Compute mean */
    for (y=height; y>0; y--, blk+=pitch)
        for (x=width; x>0; x--) {
            value = *blk++;
            sum += value;
            sumSq += value * value;
        }

    return sumSq - sum*sum/(width*height);
}

