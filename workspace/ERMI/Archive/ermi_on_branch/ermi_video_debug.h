/*
 *-------------------------------------------------------------------
 * ermi_video_debug.h - Debugging declarations for ERMI-Video Interface
 *
 * Jan 2004,  Xiaomei Liu
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#ifndef __ERMI_VIDEO_DEBUG_H__
#define __ERMI_VIDEO_DEBUG_H__

/* 
 * Function to fire up ERMI debugging
 */
extern void ermi_debug_init(void);

/*
 * The actual debugging flags are defined in ermi_video_debug_flags.h.
 * We include that file twice, once to define the flags themselves
 * and once to define the indices that the parser uses.
 */
#include "ermi_video_debug_flags.h"
#define __DECLARE_DEBUG_NUMS__
#include "ermi_video_debug_flags.h"

#define ERMI_VIDEO_BUGINF       if (ermi_debug_ermi_video) buginf

#define ERMI_R_BUGINF           if (ermi_r_debug_ermi_r) buginf

#define ERMI_C_BUGINF           if (ermi_c_debug_ermi_c) buginf
#define ERMI_C_RTSP_API_BUGINF  if (ermi_c_debug_rtsp_api) buginf
#define ERMI_C_RTSP_SES_BUGINF  if (ermi_c_debug_rtsp_ses) buginf
#define ERMI_C_RTSP_SOCK_BUGINF if (ermi_c_debug_rtsp_sock) buginf
#define ERMI_C_PARSER_BUGINF    if (ermi_c_debug_parser) buginf
#define ERMI_C_PRSERROR_BUGINF  if (ermi_c_debug_parser_error) buginf
#define ERMI_C_PRSEBRIEF_BUGINF if (ermi_c_debug_parser_brief) buginf

#endif //__ERMI_VIDEO_DEBUG_H__
