/*
 * Copyright (c) 2006-2008 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "param.h"


#define MAX_LINE_WIDTH 128

char par_line[MAX_LINE_WIDTH];


static void help (char* prog)
{
    printf("Usage:\n" \
           "    %s  -help                ; to generate parameter template\n" \
           "    %s  [-echo]  -inter      ; to enter parameter interactively\n" \
           "    %s  [-echo]  param_file  ; to read parameters from file\n" \
           "                             ; -echo to turn on echo\n",
           prog, prog, prog);
}


static char* skip_to_param (param_t* par, char* label)
{
    char* str;
    while (1) {
        /* get parameter */
        if (fgets(par_line, MAX_LINE_WIDTH, par->fp)==NULL) {
            printf("Error: EOF reached while reading parameter: %s\n", label);
            exit(-1);
        }

        if (*par_line != '*') {
            return par_line;
        }
        str = strchr(par_line+1, '*');    // search for matching '*'
        if (str!=NULL) {
            return str+1;
        }
    }
}


void param_begin (param_t* par, int argc, char** argv)
{
    int i;
    int fileOpened;

    fileOpened = 0;

    if (argc==1) {
        help(argv[0]);
        exit(-1);
    }

    par->help_flag = par->inter_flag = par->echo_flag = 0;

    for (i=1; i<argc; i++) {
        if (argv[i][0]=='-') {
            // Options
            if (!strcmp(argv[i], "-help")) {
                par->help_flag = 1;

            } else if (!strcmp(argv[i], "-inter")) {
                par->inter_flag = 1;

            } else if (!strcmp(argv[i], "-echo")) {
                par->echo_flag = 1;

            } else {
                printf("Error: Illegal options %s in command par_line!\n",
                       argv[i]);
                exit(-1);
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
                exit(-1);
            }
            fileOpened = 1;
        }
    }

    if (!(par->help_flag || par->inter_flag || fileOpened)) {
        help(argv[0]);
        exit(-1);
    }
}


void param_read (param_t* par, char* filename)
{
    par->help_flag = par->inter_flag = par->echo_flag = 0;
    par->fp = fopen(filename, "r");
    if (par->fp==NULL) {
        printf("Error: Failed to open parameter file %s!\n", filename);
        exit(-1);
    }
}


void param_end (param_t* par)
{
    if (par->help_flag) {
        exit(0);
    }
    if (par->inter_flag) {
        return;
    }

    while (1) {
        // get parameter
        if (fgets(par_line, MAX_LINE_WIDTH, par->fp)==NULL) {
            return;
        }
        if (*par_line != '*' || strchr(par_line+1, '*') != NULL) {
            break;
        }
    }
    printf("Additional parameters detected.  Ignored.\n");
}


void param_comment (param_t* par, char* comment)
{
    if (par->help_flag || par->inter_flag) {
        printf("* %s\n", comment);
    }
}


void param_get_int (param_t* par, char* label, int* var, int default_val)
{
    if (par->help_flag) {
        printf("* %s (int) * %d\n", label, default_val);
        *var = default_val;
        return;
    }

    // Get parameter
    if (par->inter_flag) {
        printf("%s (int %d): ", label, default_val);
        fgets(par_line, MAX_LINE_WIDTH, stdin);
        if (strlen(par_line)) {
            sscanf(par_line, "%d", var);
        } else {
            *var = default_val;
        }
    } else {
        sscanf(skip_to_param(par, label), "%d", var);
    }

    if (par->echo_flag) {
        printf("* %s (int) * %d\n", label, *var);
    }
}


void param_get_int64 (param_t* par, char* label, int64_t* var,
                      int64_t default_val)
{
    if (par->help_flag) {
        printf("* %s (int64) * %lld\n", label, default_val);
        *var = default_val;
        return;
    }

    // Get parameter
    if (par->inter_flag) {
        printf("%s (int %lld): ", label, default_val);
        fgets(par_line, MAX_LINE_WIDTH, stdin);
        if (strlen(par_line)) {
            sscanf(par_line, "%lld", var);
        } else {
            *var = default_val;
        }
    } else {
        sscanf(skip_to_param(par, label), "%lld", var);
    }

    if (par->echo_flag) {
        printf("* %s (int) * %lld\n", label, *var);
    }
}


void param_get_float (param_t* par, char* label, float* var, float default_val)
{
    if (par->help_flag) {
        printf("* %s (float) * %f\n", label, default_val);
        *var = default_val;
        return;
    }

    // Get parameter
    if (par->inter_flag) {
        printf("%s (float %f): ", label, default_val);
        fgets(par_line, MAX_LINE_WIDTH, stdin);
        if (strlen(par_line)) {
            sscanf(par_line, "%f", var);
        } else {
            *var = default_val;
        }
    } else {
        sscanf(skip_to_param(par, label), "%f", var);
    }

    if (par->echo_flag) {
        printf("* %s (float) * %f\n", label, *var);
    }
}


void param_get_string (param_t* par, char* label, char* var, char* default_val)
{
    if (par->help_flag) {
        printf("* %s (string) * %s\n", label, default_val);
        strcpy(var, default_val);
        return;
    }

    // Get parameter
    //    Note: should be modified to take all till EOL
    if (par->inter_flag) {
        printf("%s (string %s): ", label, default_val);
        fgets(par_line, MAX_LINE_WIDTH, stdin);
        if (strlen(par_line)) {
            sscanf(par_line, "%s", var);
        } else {
            strcpy(var, default_val);
        }

    } else {
        sscanf(skip_to_param(par, label), "%s", var);
    }

    if (par->echo_flag) {
        printf("* %s (string) * %s\n", label, var);
    }
}


void param_get_cond_int (param_t* par, char* condLabel, int cond,
                         char* label, int* var, int default_val)
{
    if (par->help_flag) {
        printf("* [%s] %s (int) * %d\n", condLabel, label, default_val);
        *var = default_val;
        return;
    }

    if (cond) {
        // Get parameter
        if (par->inter_flag) {
            printf("[%s] %s (int %d): ", condLabel, label, default_val);
            fgets(par_line, MAX_LINE_WIDTH, stdin);
            if (strlen(par_line)) {
                sscanf(par_line, "%d", var);
            } else {
                *var = default_val;
            }

        } else {
            sscanf(skip_to_param(par, label), "%d", var);
        }

        if (par->echo_flag) {
            printf("* [%s] %s (int) * %d\n", condLabel, label, *var);
        }
    }
}


void param_get_cond_float (param_t* par, char* condLabel, int cond,
                           char* label, float* var, float default_val)
{
    if (par->help_flag) {
        printf("* [%s] %s (float) * %f\n", condLabel, label, default_val);
        *var = default_val;
        return;
    }

    if (cond) {
        // Get parameter
        if (par->inter_flag) {
            printf("[%s] %s (float %f): ", condLabel, label, default_val);
            fgets(par_line, MAX_LINE_WIDTH, stdin);
            if (strlen(par_line)) {
                sscanf(par_line, "%f", var);
            } else {
                *var = default_val;
            }

        } else {
            sscanf(skip_to_param(par, label), "%f", var);
        }

        if (par->echo_flag) {
            printf("* [%s] %s (float) * %f\n", condLabel, label, *var);
        }
    }
}


void param_get_cond_string (param_t* par, char* condLabel, int cond,
                            char* label, char* var, char* default_val)
{
    if (par->help_flag) {
        printf("* [%s] %s (string) * %s\n", condLabel, label, default_val);
        strcpy(var, default_val);
        return;
    }

    if (cond) {
        // Get parameter
        if (par->inter_flag) {
            printf("[%s] %s (float %s): ", condLabel, label, default_val);
            fgets(par_line, MAX_LINE_WIDTH, stdin);
            if (strlen(par_line)) {
                sscanf(par_line, "%s", var);
            } else {
                strcpy(var, default_val);
            }

        } else {
            sscanf(skip_to_param(par, label), "%s", var);
        }

        if (par->echo_flag) {
            printf("* [%s] %s (int) * %s\n", condLabel, label, var);
        }
    }
}


/* Sample program

char* arg[] = {"", "-echo", "-inter"};

void main (int argc, char** argv)
{
    param_t par;
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
