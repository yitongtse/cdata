/*
 *-------------------------------------------------------------------
 * ermi_video_debug.c - Debugging routines for ERMI-Video Control Module
 *
 * Feb 2004, Xiaomei Liu: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#include <sys_registry.h>
#include COMP_INC(kernel/debug, debug.h)

#include "ermi_video_debug.h"

/* Declare the initialized text for the debugging array */
#define __DECLARE_DEBUG_ARR__
#include "ermi_video_debug_flags.h"

void debug_ermi_command (parseinfo *csb);

/*
 * ermi_debug_all is registered to be called whenever anybody issues
 * a "debug all" or "undebug all" command... or whenever you want to
 * set the state of all the ERMI debug flags at once. The argument is
 * TRUE for "debug all", FALSE for "undebug all".
 */
void ermi_debug_all (boolean flag)
{
    int i;

    for (i = 0; ermi_debug_arr[i].var != (boolean *) NULL; i++) {
        *(ermi_debug_arr[i].var) = flag;
    }
}

void ermi_debug_set (boolean flag, int level)
{
    if (ermi_debug_arr[level].var != NULL) {
       *(ermi_debug_arr[level].var) = flag;
    }
}

/*
 * ermi_debug_show is called to display the values of all the ERMI
 * debugging variables.
 */
void ermi_debug_show (void)
{
    debug_show_flags(&(ermi_debug_arr[0]), "ERMI");
}
 
/*
 * ermi_debug_init is called at master process startup. It registers
 * the routines to maintain and display the ERMI debug flags, and
 * makes sure the flags are set to the appropriate values depending on
 * whether "debug all" is in effect when ERMI is started.
 */

void ermi_debug_init (void)
{
    /* 
     * Register for "debug all" and "show debug" events.
     */  
    reg_add_debug_all(ermi_debug_all, "ermi_debug_all");
    reg_add_debug_show(ermi_debug_show, "ermi_debug_show");

    /* 
     * Make sure the debug flags are right at startup. If "debug all"
     * is in effect when ERMI-Video control plane is initialized, we
     * want to turn on all the ERMI-Video debugging right now.
     */
    ermi_debug_all(debug_all_p());
}
