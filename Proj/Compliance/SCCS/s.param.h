h23502
s 00068/00000/00000
d D 1.1 08/09/06 00:25:44 ytse 1 0
c date and time created 08/09/06 00:25:44 by ytse
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
