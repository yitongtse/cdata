h28996
s 00011/00009/00302
d D 2.3 02/03/21 13:28:40 ytse 4 3
c Backup for NEW2
e
s 00019/00007/00292
d D 2.2 00/08/21 11:23:55 ytse 3 2
c Added support of Windows
e
s 00000/00000/00299
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00299/00000/00000
d D 1.1 99/10/29 15:58:10 yitong 1 0
c date and time created 99/10/29 15:58:10 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************
    File: param.c
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "param.h"

#define MAX_LINE_WIDTH 128

char line[MAX_LINE_WIDTH];


static void help(char* prog)
{
    printf("Usage:\n" \
           "    %s  -help                ; to generate parameter template\n" \
           "    %s  [-echo]  -inter      ; to enter parameter interactively\n" \
           "    %s  [-echo]  param_file  ; to read parameters from file\n" \
           "                             ; -echo to turn on echo\n",
           prog, prog, prog);
}


static char* skipToParam(Param* par, char* label)
{
    char* str;
    while (1) {
        /* get parameter */
        if (fgets(line, MAX_LINE_WIDTH, par->fp)==NULL) {
            printf("Error: EOF reached while reading parameter: %s\n", label);
D 3
            exit(-1);
E 3
I 3
            exit(-390);
E 3
        }

        if (*line != '*')  return line;
        str = strchr(line+1, '*');	// search for matching '*'
        if (str!=NULL)  return str+1;
    }
}


void beginParam(Param* par, int argc, char** argv)
{
    int i;
    int fileOpened;

    fileOpened = 0;

    if (argc==1) {
        help(argv[0]);
D 3
        exit(-1);
E 3
I 3
        exit(-294);
E 3
    }

    par->helpFlag = par->interFlag = par->echoFlag = 0;

    for (i=1; i<argc; i++) {
        if (argv[i][0]=='-') {
            // Options
            if (!strcmp(argv[i], "-help"))  par->helpFlag = 1;
            else if (!strcmp(argv[i], "-inter"))  par->interFlag = 1;
            else if (!strcmp(argv[i], "-echo"))  par->echoFlag = 1;
            else {
                printf("Error: Illegal options %s in command line!\n", argv[i]);
D 3
                exit(-1);
E 3
I 3
                exit(-2706);
E 3
            }
        }
        else {
            // File name detected
            if (fileOpened) {
                printf("Error: More than one parameter filename detected!\n");
                exit (-1);
            }
            par->fp = fopen(argv[i], "r");
            if (par->fp==NULL) {
                printf("Error: Failed to open parameter file %s!\n", argv[i]);
D 3
                exit(-1);
E 3
I 3
                exit(-30);
E 3
            }
            fileOpened = 1;
        }
    }

    if (!(par->helpFlag || par->interFlag || fileOpened)) {
        help(argv[0]);
D 3
        exit(-1);
E 3
I 3
        exit(-389);
E 3
    }
}


void readParam(Param* par, char* filename)
{
    par->helpFlag = par->interFlag = par->echoFlag = 0;
    par->fp = fopen(filename, "r");
    if (par->fp==NULL) {
        printf("Error: Failed to open parameter file %s!\n", filename);
D 3
        exit(-1);
E 3
I 3
        exit(-735);
E 3
    }
}


void endParam(Param* par)
{
    if (par->helpFlag)  exit(0);
    if (par->interFlag)  return;

    while (1) {
        // get parameter
        if (fgets(line, MAX_LINE_WIDTH, par->fp)==NULL)  return;
        if (*line != '*' || strchr(line+1, '*') != NULL)  break;
    }
    printf("Additional parameters detected.  Ignored.\n");
}


void commentParam(Param* par, char* comment)
{
    if (par->helpFlag || par->interFlag)
        printf("* %s\n", comment);
}


void getIntParam(Param* par, char* label, int* var, int defaultVal)
{
    if (par->helpFlag) {
        printf("* %s (int) * %d\n", label, defaultVal);
        *var = defaultVal;
        return;
    }

    // Get parameter
    if (par->interFlag) {
        printf("%s (int %d): ", label, defaultVal);
        gets(line);
        if (strlen(line))  sscanf(line, "%d", var);
        else  *var = defaultVal;
    }
    else
        sscanf(skipToParam(par, label), "%d", var);

    if (par->echoFlag)
        printf("* %s (int) * %d\n", label, *var);
}


void getFloatParam(Param* par, char* label, float* var, float defaultVal)
{
    if (par->helpFlag) {
        printf("* %s (float) * %f\n", label, defaultVal);
        *var = defaultVal;
        return;
    }

    // Get parameter
    if (par->interFlag) {
        printf("%s (float %f): ", label, defaultVal);
        gets(line);
        if (strlen(line))  sscanf(line, "%f", var);
        else  *var = defaultVal;
    }
    else
        sscanf(skipToParam(par, label), "%f", var);

    if (par->echoFlag)
        printf("* %s (float) * %f\n", label, *var);
}


void getStringParam(Param* par, char* label, char* var, char* defaultVal)
{
I 3
	int i;
E 3
    if (par->helpFlag) {
        printf("* %s (string) * %s\n", label, defaultVal);
        strcpy(var, defaultVal);
        return;
    }

    // Get parameter
    //    Note: should be modified to take all till EOL
    if (par->interFlag) {
        printf("%s (string %s): ", label, defaultVal);
        gets(line);
        if (strlen(line))  sscanf(line, "%s", var);
        else  strcpy(var, defaultVal);

    }
    else
D 3
        sscanf(skipToParam(par, label), "%s", var);
E 3
I 3
D 4
//        sscanf(skipToParam(par, label), "%s", var);
		// Rui_B
E 4
I 4
#if 1
        sscanf(skipToParam(par, label), "%s", var);
#else
        // Rui
E 4
        strcpy(var,skipToParam(par, label));
E 3

I 3
D 4
		i = strlen(var)-1;
		while (var[i] == 0x0a || var[i] == ' ') i--;
		var[++i] = '\0';
E 4
I 4
	i = strlen(var)-1;
	while (var[i] == 0x0a || var[i] == ' ') i--;
	var[++i] = '\0';
E 4

D 4
		i = 0;
		while (var[i] == ' ') i++;
		strcpy(var,var+i);
		// Rui_E
E 4
I 4
	i = 0;
	while (var[i] == ' ') i++;
	strcpy(var,var+i);
#endif
E 4

E 3
    if (par->echoFlag)
        printf("* %s (string) * %s\n", label, var);
}


void getCondIntParam(Param* par, char* condLabel, int cond,
                    char* label, int* var, int defaultVal)
{
    if (par->helpFlag) {
        printf("* [%s] %s (int) * %d\n", condLabel, label, defaultVal);
        *var = defaultVal;
        return;
    }

    if (cond) {
        // Get parameter
        if (par->interFlag) {
            printf("[%s] %s (int %d): ", condLabel, label, defaultVal);
            gets(line);
            if (strlen(line))  sscanf(line, "%d", var);
            else  *var = defaultVal;
        }
        else
            sscanf(skipToParam(par, label), "%d", var);

        if (par->echoFlag)
            printf("* [%s] %s (int) * %d\n", condLabel, label, *var);
    }
}


void getCondFloatParam(Param* par, char* condLabel, int cond,
                        char* label, float* var, float defaultVal)
{
    if (par->helpFlag) {
        printf("* [%s] %s (float) * %f\n", condLabel, label, defaultVal);
        *var = defaultVal;
        return;
    }

    if (cond) {
        // Get parameter
        if (par->interFlag) {
            printf("[%s] %s (float %f): ", condLabel, label, defaultVal);
            gets(line);
            if (strlen(line))  sscanf(line, "%f", var);
            else  *var = defaultVal;
        }
        else
            sscanf(skipToParam(par, label), "%f", var);

        if (par->echoFlag)
            printf("* [%s] %s (float) * %f\n", condLabel, label, *var);
    }
}


void getCondStringParam(Param* par, char* condLabel, int cond,
                        char* label, char* var, char* defaultVal)
{
    if (par->helpFlag) {
        printf("* [%s] %s (string) * %s\n", condLabel, label, defaultVal);
        strcpy(var, defaultVal);
        return;
    }

    if (cond) {
        // Get parameter
        if (par->interFlag) {
            printf("[%s] %s (float %s): ", condLabel, label, defaultVal);
            gets(line);
            if (strlen(line))  sscanf(line, "%s", var);
            else  strcpy(var, defaultVal);
        }
        else
            sscanf(skipToParam(par, label), "%s", var);

        if (par->echoFlag)
            printf("* [%s] %s (int) * %s\n", condLabel, label, var);
    }
}


/* Sample program

char* arg[] = {"", "-echo", "-inter"};

void main(int argc, char** argv)
{
    Param par;
    int i1, i2;
    float f1, f2;
    char infile[10], outfile[10];


//    readParam(&par, "test.par");
    beginParam(&par, 3, arg);
//    beginParam(&par, argc, argv);
    commentParam(&par, "User input:");
    getStringParam(&par, "Input filename", infile, "test.in");
    getStringParam(&par, "Output filename", outfile, "test.out");
    getIntParam(&par, "Input i1", &i1, 10);
    getCondIntParam(&par, "i1>0", (i1>0), "Input i2", &i2, 20);
    getFloatParam(&par, "Input f1", &f1, 1.2);
    getCondFloatParam(&par, "f1>0", (f1>0), "Input f2", &f2, 3.4);
    endParam(&par);

    printf("Program starts:\n");
    printf("Program ends:\n");
}

*/
E 1
