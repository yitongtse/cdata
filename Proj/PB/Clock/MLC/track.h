/*****************************************************************************
 * Copyright (c) 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : trking.h
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-22-01  Jimmy Gou      Created.
 *    12-18-01	FL      replace alias.h by csl.h and oscover.h
 *    02-14-02  FL      IOS coding style
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     track.h                                                       
 *                                                                         
 *     Data structure related to tracking part of clock drifting compensation
 *                                                                         
 ***************************************************************************/

#ifndef _H_TRACK_
#define _H_TRACK_

#include <sem.h>
#include <mbx.h>

#include "csl.h"
#include "oscover.h"

/* 
 * Constants
 */

#define NPKTS_TRACK_BUF     64

/* 
 * Data Types
 */
typedef struct TRACK_MSG {
    Uint16 cbIdx;
} TRACKMSG;

/* 
 * Configuration Generated Objects
 */
extern SEM_Obj semXbHpiTrack;
extern MBX_Obj mbxTrack;        // Placeholder: make sure size and length are right

/* 
 * Function Prototypes
 */
Void tskTrackFunc(Void);

#endif
