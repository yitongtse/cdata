h35555
s 00000/00002/00068
d D 2.2 03/02/25 11:24:22 ytse 3 2
c update
e
s 00000/00000/00070
d D 2.1 00/08/21 11:04:24 ytse 2 1
c Before supporting Windows
e
s 00070/00000/00000
d D 1.1 99/10/29 15:58:14 yitong 1 0
c date and time created 99/10/29 15:58:14 by yitong
e
u
U
f e 0
t
T
I 1
/*****************************************************************************

    File: param.h

  This module defines a structure "Param" and corresponding routines to
  support program parameter passing.  It can be used in one of the following
  ways:

    Command line argument format:
        program  -help			// generate parameter template
        program  [-echo]  -inter	// enter parameter interactively
        program  [-echo]  param_file	// read parameters from param_file
					// (and echo variable value)


Parameter file format:

* Comment
* Input value (int) * 100
* Input filename (string) * in.txt
* [Input value > 0] Output value (int) * 2
* [Input value > 0] Output filename (string) * out.txt

Note:
  Shell script can be set up in the following way:

  program.sh contains:

#! /bin/csh
program /dev/stdin << EOD
[Program template]
EOD

*****************************************************************************/

#ifndef PARAM_H
#define PARAM_H

D 3
#include "util.h"
E 3

D 3

E 3
/* Program parameter structure
*/
typedef struct Param {
    int	helpFlag;
    int	interFlag;
    int	echoFlag;
    FILE *fp;
} Param;


void beginParam(Param *par, int argc, char **argv);
void readParam(Param *par, char *filename);
void endParam(Param *par);

void commentParam(Param *par, char *comment);

void getIntParam(Param *par, char *label, int *var, int defaultVal);
void getFloatParam(Param *par, char *label, float *var, float defaultVal);
void getStringParam(Param *par, char *label, char *var, char *defaultVal);

void getCondIntParam(Param *par, char *condLabel, int cond,
    char *label, int *var, int defaultVal);
void getCondFloatParam(Param *par, char *condLabel, int cond,
    char *label, float *var, float defaultVal);
void getCondStringParam(Param *par, char *condLabel, int cond,
    char *label, char *var, char *defaultVal);


#endif
E 1
