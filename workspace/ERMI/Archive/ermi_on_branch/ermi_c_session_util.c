/*
 *---------------------------------------------------------------------------
 * ermi_c_session_util.c
 * Routines for - ERMI session management utility functions.
 *
 * July 2008, Neeraj Khurana
 *
 * Copyright (c) 2008 by Cisco Systems, Inc.
 * All rights reserved.
 *
 *---------------------------------------------------------------------------
 */

#include "ermi_c_rtsp_api.h"

/*
 * Callback to be invoked when SCM has session create response ready.
 */
void rfgw_ermi_video_rsp_msg_callback (rtsp_session_t *rtsp_sess_ctx)
{
    // DO NOT CALL:
    // rfgw_video_rsp_msg_callback(RFGW_VIDEO_PROTOCOL_ERMI, rtsp_sess_ctx);

    
}

