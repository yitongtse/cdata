/*****************************************************************************
    Copyright (c) 2003 by Cisco Systems, Inc.
    All rights reserved.


    File: param.h

    Date	Who	Modification
    --------	------	------------------------------------------------------
    03-26-03	ytse	Created.


    This module defines a structure "Param" and corresponding routines to
    support program parameter passing.  It can be used in one of the following
    ways:

      Command line argument format:
          program  -help		// generate parameter template
          program  [-echo]  -inter	// enter parameter interactively
          program  [-echo]  param_file	// read parameters from param_file
					// (and echo variable value)

    Parameter file format:
        Each line contains either a comment or a parameter.
        A line with only an opening * is a comment line.
        A line with an opening and a matching * has a parameter.  The parameter 
        is found after the closing *.  Anything between the *s are comments.

        E.g.:
        * Comment
        * Input value (int) * 100
        * Input filename (string) * in.txt
        * [Input value > 0] Output value (int) * 2
        * [Input value > 0] Output filename (string) * out.txt

    Note: Shell script can be set up in the following way:

    #! /bin/csh
    program /dev/stdin << EOD
    [Program template]
    EOD
*****************************************************************************/

#ifndef __PARAM_H__
#define __PARAM_H__


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

#endif	// __PARAM_H__
