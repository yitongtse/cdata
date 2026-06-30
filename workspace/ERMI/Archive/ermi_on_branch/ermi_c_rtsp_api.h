/* 
 *-------------------------------------------------------------------
 * ermi_c_rtsp_api.h - RTSP server interface declarations
 *
 * December 2003 Linda Hua
 * Copyright (c) 2003-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#ifndef _ERMI_C_RTSP_API_H_
#define _ERMI_C_RTSP_API_H_

#include "ermi_c_rtsp_def.h"
#include "ermi_c_rtsp_sock.h"
#include "ermi_c_rtsp_session.h"
#include "ermi_video_debug.h"

/* Initialize RTSP Server
 */
ermi_status rtsp_init_server(ipaddrtype local_addr, ushort listen_port,
                             app_func_t app_fn);

ermi_c_peer_t *ermi_c_get_rtsp_conn_peer(rtsp_conn_t *rtsp_conn);

/* Send a Request message out of a Server or Client connection
 */
ermi_status rtsp_send_request(rtsp_session_t *ses, rtsp_msg_t *rtsp_msg);

/* Send a Response message out of a Server or Client connection
 */
ermi_status rtsp_send_response(rtsp_session_t *ses, rtsp_msg_t *rtsp_msg);

/* Close a session on Server
 */
ermi_status rtsp_server_close_session(rtsp_session_t *ses);

/* Close all RTSP sessions, clean up RTSP data in the given rtsp_conn. 
 * Then free the rtsp_conn. 
 * Works for both Client and Server connections
 */
void rtsp_close_conn(rtsp_conn_t *rtsp_conn);

/*
 * Close a session on Client
 */ 
ermi_status rtsp_client_close_session(rtsp_session_t *ses); 

/* Get a rtsp_msg structure
 */
rtsp_msg_t* rtsp_new_msg(void);

/* Free a rtsp_msg structure
 */
void rtsp_free_msg(rtsp_msg_t* msg);

/*****************************************************/

const char *ermi_c_rtsp_method_str (ushort mcode);

void ermi_c_rtsp_init (void);

const char *ermi_c_rtsp_msg_type_str(rp_msg_type_e);

extern boolean ermi_c_rtsp_enabled;

ermi_c_conn_t *ermi_c_local_init(ermi_c_que_msg_e type, app_func_t app_fn);

ermi_status rtsp_send_msg(ermi_c_conn_t *conn, rtsp_msg_t *rtsp_msg);

ermi_status ermi_c_rtsp_resp_enque(rtsp_msg_t *rtsp_resp_msg);

process ermi_c_master_proc(void);

process ermi_c_resp_process(void);


#ifdef NK_NOT_USED
ermi_status \
ermi_c_create_send_announce(lc_event_type_t event_type,
                            scm_session_id scm_sess_id,
                            uint32 last_state, uint32 new_state,
                            uint32 err_code);
#endif // NK_NOT_USED

boolean rfgw_ermi_sg_add_server(rfgw_video_server_group *video_sg, ipaddrtype);

#define ERMI_TRACE { ERMI_C_BUGINF("\nERMI Trace: %s():%d", \
                                   __FUNCTION__, __LINE__); }

#endif /* #define _ERMI_C_RTSP_API_H_ */

