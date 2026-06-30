/* 
 *-------------------------------------------------------------------
 * ermi_c_rtsp_session.h
 *
 * December 2003 Linda Hua
 * Copyright (c) 2003-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
 /*
  * RTSP server interface declarations
  */

#ifndef _ERMI_C_RTSP_SESSION_H_
#define _ERMI_C_RTSP_SESSION_H_

#include INTERNAL_INC(sup/src/vscm/video_session_api.h)

/* is the session addr valid, called by parser 
   rtsp_conn: the parent connection of the session
   sessionID: the sessionID from incoming msg
*/
rtsp_session_t *rtsp_find_session(uint32 sessionID);

/* create the session and save it in array ses_tbl in the connection 
   rtsp_conn: the parent connection
   return the newly created session
 */
rtsp_session_t *rtsp_add_session(rtsp_conn_t *rtsp_conn); 

video_operation_result
ermi_c_rtsp_delete_session(rtsp_session_t *session, uint32 out_id,
                           uint32 msg_seq_id);

/*
 * ermi_c_itoa
 *
 * Convert integer to ascii string for any base.
 *
 * Parameters:
 * val - int to be converted (duh)
 * str - place to put it
 * base - any base you please
 *
 * Returns:
 * void
 */
void ermi_c_itoa(int val, char *str, int base);

/* rtsp svr will process the parsed message 
   based on the received rtsp msg, sessoin id and index provided 
   by parser, create a new session or find the corresponding session,
   then pass it to application by calling conn->app_msg_func()
   conn: the parent connection
   rtsp_msg: received and parsed rtsp mag.
 */
void ermi_c_rtsp_svr_process_msg(ermi_c_conn_t *conn, void *rtsp_msg);

/* These functions are called when a session timeout timer expires */
void rtsp_handle_ses_resp_timeout(mgd_timer *the_timer, void *context);
void rtsp_handle_ses_keepalive_timeout(mgd_timer *the_timer, void *context);

#endif /* #define _ERMI_C_RTSP_SESSION_H_ */
