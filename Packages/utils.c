/* @(#)utils.c	1.1  07/15/98  @(#) */
/*
 *  ======== myPrintf ========
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "mydefs.h"

extern Uint32	showFlag;

void myPrintf( Uint32 cases, char * fmt, ...)
{
    va_list va;

    va_start(va, fmt);

    if ( showFlag & cases )
	vprintf( fmt, va );
	
    va_end(va);
}
