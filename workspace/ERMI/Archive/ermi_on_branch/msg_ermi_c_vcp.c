/*
 *------------------------------------------------------------------
 * msg_ermi_c_vcp.c : ERMI Video ERMI-2 related - SysLog Error Messages
 *
 * January, 2004 Linda Hua; Sep 2008 Neeraj Khurana
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#define DEFINE_MESSAGES TRUE
#include "logger.h"


/* error for the master process */
facdef(VCP);             /* Define the facility */

msgdef_section("Video Network Control Plane Master Process Error Messages");

msgdef(SWERR, VCP, LOG_ERR, 0, "%s %d %s");
msgdef_explanation(
        "software error info");
msgdef_recommended_action(
        "Contact tech support");
msgdef_ddts_component("video-pb-ios");

msgdef(RSCERR, VCP, LOG_ERR, 0, "%s");
msgdef_explanation(
         "Resource over allocation");
msgdef_recommended_action(
        "Contact tech support");
msgdef_ddts_component("video-pb-ios");

/* error for the RTSP */

facdef(RTSP);
msgdef_section("Video Network Control Plane RTSP Error Messages");

msgdef(SWERR, RTSP, LOG_ERR, 0, "%s %d %s");
msgdef_explanation(
        "Software error info");
msgdef_recommended_action(
        "Contact tech support");
msgdef_ddts_component("video-pb-ios");

msgdef(DATAERR, RTSP, LOG_ERR, 0, "%s %s");
msgdef_explanation(
        "Rtsp data error info");
msgdef_recommended_action(
        "Contact tech support");
msgdef_ddts_component("video-pb-ios");


msgdef(RSCERR, RTSP, LOG_ERR, 0, "%s");
msgdef_explanation(
        "Resource over allocation");
msgdef_recommended_action(
        "Contact tech support");
msgdef_ddts_component("video-pb-ios");

