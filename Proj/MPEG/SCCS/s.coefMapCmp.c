h37425
s 00114/00000/00000
d D 1.1 02/02/20 11:05:05 ytse 1 0
c date and time created 02/02/20 11:05:05 by ytse
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: coefMapCmp.c

    Compare two coefficient maps to see if the second map
    is a subset of the first map
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "param.h"
#include "reader.h"
#include "writer.h"
#include "infile.h"
#include "tp.h"


#define MAX_LINE_WIDTH		128
#define EOD			-1


int getNextInt(char** ptr)
{
    char* ptr2 = *ptr;
    while (*ptr2>='0' && *ptr2<='9')  ptr2++;
    while (*ptr2 == ' ')  ptr2++;
    *ptr = ptr2;
    return (*ptr2=='\n' || *ptr2==0)?  EOD : atoi(ptr2);
}


#if 0
void parse(char* ptr)
{
    int n;

    while (*++ptr != ':');	// seek to ':'
    ptr++;

    while (1) {
        n = getNextInt(&ptr);
        if (n == NULL)  return;
        printf("%d\n", n);
    }
}
#endif


int subsetTest(char* map1, char* map2)
{
    int n1, n2;

    while (*++map1 != ':');
    map1++;
    while (*++map2 != ':');
    map2++;

    while (1) {
        n2 = getNextInt(&map2);
        if (n2 == EOD)  return 1;

        do { 
            n1 = getNextInt(&map1);
            if (n1 == EOD)  return 0;
        } while (n1 < n2);

        if (n1 > n2)  return 0;
    }
}


int main(int argc, char** argv)
{
    Param par;
    char origMap[MAX_LINE_WIDTH], recodeMap[MAX_LINE_WIDTH];
    FILE *fp1, *fp2;
    int pic = -1;
    int pic2;

    if (argc>1)  beginParam(&par, argc, argv);
    else  readParam(&par, "coefMapCmp.par");
    getStringParam(&par, "Original coef map", origMap, "orig.coef");
    getStringParam(&par, "Recoded coef map", recodeMap, "recode.coef");
    endParam(&par);

    fp1 = fopen(origMap, "r");
    fp2 = fopen(recodeMap, "r");

    while (1) {
        if (fgets(origMap, MAX_LINE_WIDTH, fp1) == NULL) {
            printf("End of orig map\n");
            return;
        }

        if (fgets(recodeMap, MAX_LINE_WIDTH, fp2) == NULL) {
            printf("End of recode map\n");
            return;
        }

#if 0
        sscanf(origMap, "%d", &pic2);
        if (pic2 != pic) {
            pic = pic2;
            printf("Comparing picture %d", pic);
        }
#endif

        if (!subsetTest(origMap, recodeMap)) {
            printf("Orig:   %s", origMap);
            printf("Recode: %s", recodeMap);
        }
    }
}

E 1
