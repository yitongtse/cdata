/*****************************************************************************
 * Copyright (c) 2001, 2002 by Cisco Systems, Inc.
 * All rights reserved.
 *****************************************************************************
 *    File name : trkloop.h
 *
 *    Date      Who     Modification
 *    --------  ---     ------------------------------------------------------
 *    05-11-01  Jimmy Gou      Created.
 *
 *****************************************************************************/

/***************************************************************************
 *                                                                         
 *     trkloop.h                                                       
 *                                                                         
 *     Things related to the tracking loop of clock compensation 
 *                                                                         
 ***************************************************************************/

#ifndef _H_TRKLOOP_
#define _H_TRKLOOP_

#include <std.h>
#include <mbx.h>
#include <sem.h>

#include "alias.h"
#include "cbinfo.h"


/* 
 * Constants
 */

// See the Matlab algorithm simulation for the interpretation of
// these constants
#if 0
#define EXPO_ALPHA 		(-15)
#define EXPO_BETA		(-15)
#define EXPO_ASCALE		41

#define ABS_EXPO_ALPHA 		15
#define ABS_EXPO_BETA		15
#define ABS_EXPO_ASCALE		41


#define IIR_SCALE 	16

#else

// #define EXPO_ALPHA 		(-15)
#define EXPO_ALPHA 		(-18)
#define EXPO_BETA		(-28)
#define EXPO_ASCALE		41

// #define ABS_EXPO_ALPHA 		15
#define ABS_EXPO_ALPHA 		18
#define ABS_EXPO_BETA		28
#define ABS_EXPO_ASCALE		41


#define IIR_SCALE 	16

#endif


/* 
 * global variables
 */


/* 
 * Function Prototypes
 */
void TrackLoopStateInit(Clock *send, Clock *deliv, int indx);
void TrkLoop(int cb, int indx, Clock *arrv, Clock *send);

#endif



