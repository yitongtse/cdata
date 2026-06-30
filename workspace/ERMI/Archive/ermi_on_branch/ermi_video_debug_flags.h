/*
 *-------------------------------------------------------------------
 * ermi_video_debug_flags.h - Debugging flag declarations for ERMI-Video Support
 *
 * Feb 2004, Xiaomei Liu: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#include "../../ui/debug_macros.h"

/*
 * Now define the actual flags and the array that points to them
 */
DEBUG_ARRDECL(ermi_debug_arr)

DEBUG_FLAG(ermi_debug,            DEBUG_ERMI, "ERMI-Video Control Plane")
DEBUG_FLAG(ermi_debug_ermi_video, DEBUG_ERMI_VIDEO, "ERMI-Video General debug")

DEBUG_FLAG(ermi_debug_ermi_r, DEBUG_ERMI_R, "ERMI-Video ERMI-1 interface debug")

DEBUG_FLAG(ermi_c_debug_parser, DEBUG_ERMI_C_PARSER, "ERMI-2 debug parser details")
DEBUG_FLAG(ermi_c_debug_parser_error, DEBUG_ERMI_C_PRSERROR, "ERMI-2 debug parsererror")
DEBUG_FLAG(ermi_c_debug_parser_brief, DEBUG_ERMI_C_PRSEBRIEF, "ERMI-2 debug parser brief")
DEBUG_FLAG(ermi_c_debug_rtsp_api, DEBUG_ERMI_C_RTSP_API, "ERMI-2 debug rtsp api")
DEBUG_FLAG(ermi_c_debug_rtsp_ses, DEBUG_ERMI_C_RTSP_SES, "ERMI-2 debug rtsp ses")
DEBUG_FLAG(ermi_c_debug_rtsp_sock, DEBUG_ERMI_C_RTSP_SOCK, "ERMI-2 debug rtsp sock")
DEBUG_FLAG(ermi_c_debug_ermi_c, DEBUG_ERMI_C, "ERMI-Video interface debug")

DEBUG_ARRDONE

