/*
 *-------------------------------------------------------------------
 * exec_debug_ermi_video.h - debug command for Video Network line card
 *
 * March 2004, Xiaomei Liu: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

EOLS (debug_ermi_eol, debug_command, DEBUG_ERMI);

/*
 * debug ermi-video ermi-1 ...
 */
EOLS(debug_ermi_r_eol, debug_command, DEBUG_ERMI_R);
KEYWORD(debug_ermi_r, debug_ermi_r_eol, debug_ermi_eol,
	"ermi-1", "Generic ERMI-1 debug", PRIV_OPR | PRIV_HIDDEN);

/*
 * debug ermi-video ermi-2 [ rtsp [session|socket|api] |
 *                           parser | error | details | brief ]
 */
EOLS (debug_ermi_c_parser_eol, debug_command, DEBUG_ERMI_C_PARSER);
EOLS (debug_ermi_c_prserror_eol, debug_command, DEBUG_ERMI_C_PRSERROR);
EOLS (debug_ermi_c_prsebrief_eol, debug_command, DEBUG_ERMI_C_PRSEBRIEF);
 
KEYWORD(debug_ermi_c_parser_brief, debug_ermi_c_prsebrief_eol, no_alt,
        "brief", "Brief Debugs from RTSP Message Parser", PRIV_OPR);
 
KEYWORD(debug_ermi_c_parser_det, debug_ermi_c_parser_eol,
        debug_ermi_c_parser_brief,
        "details", "RTSP Message Parsing Details", PRIV_OPR);

KEYWORD(debug_ermi_c_prserror, debug_ermi_c_prserror_eol,
        debug_ermi_c_parser_det,
        "error", "RTSP Message Parsing Errors", PRIV_OPR);
  
KEYWORD(debug_ermi_c_parser, debug_ermi_c_prserror, debug_ermi_r,
        "parser", "Parser for ERMI-Video RTSP Messages", PRIV_OPR);

EOLS (debug_ermi_c_rtsp_ses_eol, debug_command, DEBUG_ERMI_C_RTSP_SES);
KEYWORD(debug_ermi_c_rtsp_ses, debug_ermi_c_rtsp_ses_eol, debug_ermi_c_parser,
        "rtsp session", "RTSP Session Management", PRIV_OPR);

EOLS (debug_ermi_c_rtsp_sock_eol, debug_command, DEBUG_ERMI_C_RTSP_SOCK);
KEYWORD(debug_ermi_c_rtsp_sock, debug_ermi_c_rtsp_sock_eol,
        debug_ermi_c_rtsp_ses,
        "rtsp socket", "RTSP Socket", PRIV_OPR);

EOLS (debug_ermi_c_rtsp_api_eol, debug_command, DEBUG_ERMI_C_RTSP_API);
KEYWORD(debug_ermi_c_rtsp, debug_ermi_c_rtsp_api_eol, debug_ermi_c_rtsp_sock,
        "rtsp api", "RTSP API", PRIV_OPR);

/*
 * debug ermi-video ermi-2
 */
EOLS (debug_ermi_c_eol, debug_command, DEBUG_ERMI_C);
KEYWORD(debug_ermi_c, debug_ermi_c_eol, debug_ermi_c_rtsp,
        "ermi-2", "Generic ERMI-2 debug", PRIV_OPR);

/*
 * debug ermi-video general
 */
EOLS (debug_ermi_video_eol, debug_command, DEBUG_ERMI_VIDEO);
KEYWORD(debug_ermi_video, debug_ermi_video_eol, debug_ermi_c,
        "general", "Generic ERMI-Video Protocol debug", PRIV_OPR);

/*
 * debug ermi-video
 */
KEYWORD_DEBUG(debug_ermi, debug_ermi_video, ALTERNATE,
              OBJ(pdb,1), ermi_debug_arr,
              "ermi-video", "ERMI-Video Control Plane",
              PRIV_OPR);

#undef  ALTERNATE
#define ALTERNATE       debug_ermi
