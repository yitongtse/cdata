/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : interpol.h
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-11-01  Jimmy Gou      Created.
 *    12-18-01  FL      replace alias.h by csl.h and oscover.h
 *    02-14-02  FL      IOS coding style
 *    03-14-02  FL      extern SEM_Obj semXbHpiInterpolWr;
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     interpol.h                                                       
 *                                                                         
 *     Data structure related to interpolation part of clock drifting 
 *     compensation
 *                                                                         
 ***************************************************************************/

#ifndef _H_INTERPOL_
#define _H_INTERPOL_

#include <sem.h>
#include <mbx.h>

#include "csl.h"
#include "oscover.h"

/* 
 * Constants
 */

#define NPKTS_INTERPOL_BUF      64

/* 
 * Data Types
 */
typedef struct INTERPOL_MSG {
    Uint16 cbIdx;
} INTERPOLMSG;

/* 
 * Configuration Generated Objects
 */
extern SEM_Obj semXbHpiInterpol;
extern SEM_Obj semXbHpiInterpolWr;
extern MBX_Obj mbxInterpol;     // Placeholder: make sure size and length are right

/* 
 * Function Prototypes
 */
Void tskInterpolFunc(Void);

#endif
