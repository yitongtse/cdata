/*****************************************************************************
  Copyright (c) 2006, 2008 by Cisco Systems, Inc.
  All rights reserved.

  File: param.h

  This module defines a structure "param" and corresponding routines to
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

#ifndef __PARAM_H__
#define __PARAM_H__


#include <inttypes.h>


/* Program parameter structure
*/
typedef struct param_ {
    int	help_flag;
    int	inter_flag;
    int	echo_flag;
    FILE *fp;
} param_t;


void param_begin(param_t *par, int argc, char **argv);
void param_read(param_t *par, char *filename);
void param_end(param_t *par);

void param_comment(param_t *par, char *comment);

void param_get_int(param_t *par, char *label, int *var, int default_val);
void param_get_int64(param_t *par, char *label, int64_t *var,
                     int64_t default_val);
void param_get_float(param_t *par, char *label, float *var, float default_val);
void param_get_string(param_t *par, char *label, char *var, char *default_val);

void param_get_cond_int(param_t *par, char *cond_label, int cond,
        char *label, int *var, int default_val);
void param_get_cond_float(param_t *par, char *cond_label, int cond,
        char *label, float *var, float default_val);
void param_get_cond_string(param_t *par, char *cond_label, int cond,
        char *label, char *var, char *default_val);


#endif  // __PARAM_H__
