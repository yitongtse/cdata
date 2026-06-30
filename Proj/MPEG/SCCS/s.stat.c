h20296
s 00001/00001/00040
d D 2.2 02/02/28 13:41:56 ytse 3 2
c update
e
s 00000/00000/00041
d D 2.1 00/08/21 11:04:25 ytse 2 1
c Before supporting Windows
e
s 00041/00000/00000
d D 1.1 99/10/29 15:58:11 yitong 1 0
c date and time created 99/10/29 15:58:11 by yitong
e
u
U
f e 0
t
T
I 1
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
 
D 3
    for (y=0; y<height; y++, buf1+=pitch, buf2+=pitch)
E 3
I 3
    for (y=height; y>0; y--, buf1+=pitch, buf2+=pitch)
E 3
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

E 1
