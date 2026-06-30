/*
 * msg_ermi_c.c - ERMI-2 Interface related error msg defines.
 *
 * Jan 2004 Ashok Bhaskar
 *
 * Copyright (c) 2004-2008 by cisco Systems, Inc.
 * All rights reserved.
 */

#define DEFINE_MESSAGES TRUE
#include "logger.h"


facdef(ERMI_C);                                   /* Define the facility */

msgdef_section("EdgeQAM-Control Error Messages");

msgdef(NOCREATE, ERMI_C, LOG_ERR, 0, "%s");
msgdef_explanation(
        "Unable to create or allocate a resource needed by the ERMI_C.");
msgdef_recommended_action(
        "Copy this error message and report it to Cisco Technical Support.");
msgdef_ddts_component("video-pb-ios");

msgdef(NORESOURCE, ERMI_C, LOG_ERR, 0, "%s");
msgdef_explanation(
        "A required resource is not available.");
msgdef_recommended_action(
        "Copy this error message and report it to Cisco Technical Support.");
msgdef_ddts_component("video-pb-ios");

msgdef(BADPARAM, ERMI_C, LOG_ERR, 0, "In %s( ):%s has invalid value %d.  %s");
msgdef_explanation(
        "The specified parameter or variable has a bad value.");
msgdef_recommended_action(
        "Copy this error message and report it to Cisco Technical Support.");
msgdef_ddts_component("video-pb-ios");

msgdef(OP_FAIL, ERMI_C, LOG_ERR, 0, "In %s( ):Failed to %s. Param=%d");
msgdef_explanation(
        "The specified operation failed.");
msgdef_recommended_action(
        "Copy this error message and report it to Cisco Technical Support.");
msgdef_ddts_component("video-pb-ios");

