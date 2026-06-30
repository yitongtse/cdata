/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_session.c create/add/remove/close a session
 *
 * January 2004, Linda Hua: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#include COMP_INC(network, address.h)
#include COMP_INC(posix, stdlib.h)
#include COMP_INC(kernel/memory, chunk.h)
#include IOS_INC(socket/socket.h)
#include <sched.h>
#include COMP_INC(posix, assert.h)
#include COMP_INC(cisco, ciscolib.h)
#include <logger.h>
#include "ermi_video_debug.h"
#include "ermi_video_hash.h"
#include "ermi_c_master.h"

#include "ermi_c_global.h"
#include "msg_ermi_c_vcp.c"
#define NEW_DEF
#include "ermi_c_rtsp_api.h"
#include "ermi_c_rtsp_session.h"

#include INTERNAL_INC(sup/src/vscm/video_session_api.h)
#include INTERNAL_INC(sup/src/vclient/session_main.h)
#include INTERNAL_INC(sup/src/vclient/session_util.h)

/*
 *
 * Used by itoa to convert integer to ascii string for any base. 
 */
static 
void ermi_c_inttoasc (unsigned val, char *str, int base)
{
    char tmp[11];
    char sign = ' ';
    char *right;

    tmp[10] = '\0';
    right = &tmp[9];

    if (base < 0) {
        if ((int) val < 0) {
            val = - ((int) val);
            sign = '-';
        }
        base = -base;
    }

    do {
        *right-- = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [val % base];
    } while ((val /= base) != 0);

    if (sign != ' ') {  /* don't shove in an empty space for non-negative */
      *str++ = sign;
    }
      right++;
    while (*right)
        {
            *str++ = *right++;
        }
    *str = '\0';
} 

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
void ermi_c_itoa (int val, char *str, int base)
{
    ermi_c_inttoasc(val, str, base);
}

/* Number of bytes to malloc. Add 100 bytes as a safety margin
 * TODO need to size this better
 */
#define MAX_SESSIONLIST_LEN  ( (10+MAX_SESSID_LEN)*1024 + 100)

static char sessionlist_str[]  = "clab-session-list: ";

/* Builds a session_list in response to a GET_PARAMETER.
 * Returns a pointer to the session_list.
 */
char * ermi_c_rtsp_get_sessionlist (char *session_group, int count)
{
    int j;
    rtsp_conn_t *rtsp_conn;
    int num_sess;
    char *buf;
    char *pb;
    rtsp_session_t *ses;
    int len;

    rfgw_video_server_group *video_sg;
    list_element *sg_elem;
    ermi_c_peer_t *peer;

    buf = malloc(MAX_SESSIONLIST_LEN);
    if (buf == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "Failed to create - sessionlist buffer");
        return NULL;
    }
    pb = buf;

    memcpy(pb, sessionlist_str, len = strlen(sessionlist_str));
    pb += len;
   
    /* Find amongst all active ERMI-Video Server group */
    FOR_ALL_DATA_IN_LIST(&rfgw_video_sg_list, sg_elem, video_sg) {

        if ((video_sg->video_protocol != RFGW_VIDEO_PROTOCOL_ERMI) ||
            (video_sg->state != RFGW_VIDEO_SERVER_GROUP_ACTIVE)) {
            continue;
        }

        peer = ermi_c_get_peer_erm_info(video_sg);


        if (peer == NULL) {
            errmsg(&msgsym(SWERR, RTSP), "Peer info Not found");
            continue;
        }

        ERMI_TRACE;

        if (peer->conn.type != RTSP_SVR_CONN) {
            continue;
        }

        rtsp_conn = (rtsp_conn_t *)(peer->conn.prot_info);
        if (rtsp_conn == NULL) {
            errmsg(&msgsym(SWERR, RTSP), "RTSP Conn - Not found");
            continue;
        }

        num_sess = (RTSP_SESSION_TABLE_LEN - peer->conn.ses_index_list.count);
        ERMI_C_RTSP_SES_BUGINF("\n(%s) Found %d sessions.",
                               video_sg->sg_name, num_sess);
        if (num_sess <= 0) {
            continue;
        }

        for (j = 0; j < RTSP_SESSION_TABLE_LEN; j++) {
            ses = peer->conn.ses_tbl[j];
            if (!ses) {
                continue;
            }

            /* If 'session_group' is non-NULL, filter on session_group value */
            if (session_group == NULL ||
                (strlen(session_group) == 0) ||
                (strcmp(session_group, ses->session_group) == 0)) {

                ERMI_C_RTSP_SES_BUGINF("\n%d: %s: SessRec: %#X tbl[%d]=%d (%s)",
                                       j, session_group, ses, ses->index,
                                       ses->session_id, ses->session_group);

                len = strlen(ses->clientsession_id);
                
                /* Make sure there's enough space in the buffer */
                if ( ((pb+(SESSID_LEN8+2)+len) - buf) >= MAX_SESSIONLIST_LEN) {
                    errmsg(&msgsym(RSCERR, RTSP), "Session List is too long");
                    *pb = 0; /* end the string */
                    return buf;
                }

                pb += snprintf(pb, SESSID_LEN8, "%08X", ses->session_id);
                *pb++ = ':';
                memcpy(pb, ses->clientsession_id, len);
                pb += len;
                *pb++ = ';'; /* Add a space */
                *pb++ = ' '; /* Add a space */
            }
        }
    }

    *pb = 0; /* string delimiter */
    return buf; 
}

/* create a new session */
static rtsp_session_t *rtsp_new_session (void)
{
    rtsp_session_t *rtsp_ses = chunk_malloc(rtsp_session_chunk);
    if (rtsp_ses == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "rtsp_session_chunk malloc failed");
        return (NULL);
    }

    /* init session_id as 000000000000*/
    rtsp_ses->session_id = 0;
    rtsp_ses->clientsession_id[0] = 0;
    rtsp_ses->session_group[0] = 0;

    /* init the timer, context is the session */
    mgd_timer_init_leaf(&rtsp_ses->resp_timer,
                        &ermi_c_parent_timer,
                        RTSP_RESPONSE_TIMER, rtsp_ses, FALSE);

#ifdef SUPPORT_RTSP_CLIENT_KEEPALIVE
    mgd_timer_init_leaf(&rtsp_ses->keepalive_timer,
                        &ermi_c_parent_timer,
                        RTSP_KEEPALIVE_TIMER, rtsp_ses, FALSE);
#endif
    
    return (rtsp_ses);
}

/* free the session */
static void rtsp_free_session (rtsp_session_t *ses)
{
    return;
}

/* create the session and save it in ses_tbl in the rtsp_connection */
rtsp_session_t *rtsp_add_session (rtsp_conn_t *rtsp_conn)
{
    rtsp_session_t *rtsp_ses;
    int index;

    /* create the session */
    rtsp_ses = rtsp_new_session();

    if (rtsp_ses == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "rtsp_session_chunk malloc failed");
        return (NULL);
    }

    /* get the session index */
    index = (int)list_dequeue(&rtsp_conn->conn->ses_index_list);
    if (index >= RTSP_SESSION_TABLE_LEN ) {
        errmsg(&msgsym(RSCERR, RTSP), "rtsp_new session failed");
        rtsp_free_session(rtsp_ses);
        return (NULL);
    }

    /* check the ses_tbl slot */
    if (rtsp_conn->conn->ses_tbl[index] != NULL) {
        errmsg(&msgsym(SWERR, RTSP), "session with index", index, 
               "already exists");
        list_enqueue(&(rtsp_conn->conn->ses_index_list), NULL, (void *)index);
        rtsp_free_session(rtsp_ses);
        return (NULL);
    }

    assert(rtsp_conn->conn->peer);
    rtsp_ses->peer_erm = rtsp_conn->conn->peer;

    /* Reset the session_id, will be assigned by the VSCM API call, later */
    rtsp_ses->session_id = VIDEO_SCM_NO_SESSION_ID;
    ERMI_C_RTSP_SES_BUGINF("\nERMI-C Server: setup a session %08X @ index"
                           " for client %i, @ session_index = %d",
                           index, rtsp_conn->conn->src_addr, index);

    /* Save the allocated index entry into the ses_tbl */
    rtsp_ses->index = index;
    rtsp_conn->conn->ses_tbl[index] = rtsp_ses;

    return (rtsp_ses);
}


/* is this a valid session?  */
rtsp_session_t *rtsp_find_session (/* rtsp_conn_t *rtsp_conn, */
                                   uint32 sessionID)
{
    rtsp_session_t *session_ctx; 

    session_ctx = (rtsp_session_t *)scm_video_get_sess_ctx(VSESSION_TYPE_OUTPUT,
                                                     (scm_session_id)sessionID);

    return (session_ctx);
}

boolean ermi_c_rtsp_send_nosession_response (ermi_c_conn_t *conn, 
                                             rtsp_msg_t *rtsp_msg,
                                             ulong resp_retcode)
{
    rtsp_msg_t *new_msg;
    
    new_msg = rtsp_new_msg();
    if (new_msg == NULL) {
        return FALSE;
    }
    new_msg->msg_type = RTSP_RESPONSE;
    new_msg->cseq = rtsp_msg->cseq;
    new_msg->resp_retcode = RTSP_RET_SESSION_NOT_FOUND;
    new_msg->resp_retcode = resp_retcode;
    new_msg->sessionID = rtsp_msg->sessionID;
    RPM_SETBIT(new_msg->valid, RP_BIT_SESSIONID);
    if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_CLIENTSESSIONID)) {
        strcpy(new_msg->clientsessionID, rtsp_msg->clientsessionID);
        RPM_SETBIT(new_msg->valid, RP_BIT_CLIENTSESSIONID);
    } 
    
    rtsp_send_msg(conn, new_msg);
    return TRUE;
}

/* rtsp svr will process the parsed message */
void ermi_c_rtsp_svr_process_msg (ermi_c_conn_t *conn, void *msg)
{
    rtsp_session_t *ses;
    rtsp_conn_t *rtsp_conn;
    rtsp_msg_t *new_msg;
    rtsp_msg_t *rtsp_msg = (rtsp_msg_t *)msg;
    char constr[40]; /* for temp storage */
    char *content_ptr = NULL;
    ermi_status status;

    assert(conn != NULL);
    assert(rtsp_msg != NULL);

    rtsp_conn = (rtsp_conn_t *)conn->prot_info;

    ERMI_C_RTSP_SES_BUGINF("\nrtsp server at port %d received msg %s from %i",
                 rtsp_conn->conn->port,
                 ermi_c_rtsp_method_str(ERMI_C_GET_METHOD(rtsp_msg->msg_type)),
                 rtsp_conn->conn->dest_addr);

    switch(rtsp_msg->msg_type) {

    case RTSP_SETUP:
        /* create new session */
        ses = rtsp_add_session(rtsp_conn);
        if (ses == NULL) {
            /* no more memory, cannot handle it */
            errmsg(&msgsym(SWERR, RTSP), "Can't add session");
            rtsp_free_msg(rtsp_msg);
            return;
        }
        ses->recv_cseq = rtsp_msg->cseq;
        ses->state = RTSP_SESSION_READY;

        /*
         * If the incoming SETUP message contains URL, ClientSessionId and 
         * SessionGroupToken headers, store the values of those parameters
         */
        if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_URL)) {
            sstrncpy(ses->url, rtsp_msg->url, MAX_URL_LEN);
        }

        if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_CLIENTSESSIONID)) {
            sstrncpy(ses->clientsession_id, rtsp_msg->clientsessionID,
                     MAX_SESSID_LEN);
        }
        if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_SESSGRP)) {
            sstrncpy(ses->session_group, rtsp_msg->session_group,
                     MAX_SESSGRP_LEN);
        }
        break;

    case RTSP_SETPARAMETER:
        /* not a new session, session id must be present */
        ses = (rtsp_session_t *)rtsp_find_session(rtsp_msg->sessionID);
        if (ses == NULL) {
            /* Clear retry count and restart timer */
            rtsp_conn->timer_retry_count = 0;
            mgd_timer_start(&rtsp_conn->conn_timer, ERMI_C_RTSP_CONN_TIMEOUT);
            ermi_c_rtsp_send_nosession_response(conn, rtsp_msg, RTSP_RET_OK);
            rtsp_free_msg(rtsp_msg);
            return;
        }
        ses->recv_cseq = rtsp_msg->cseq;
        break;

    case RTSP_GETPARAMETER:
        if (rtsp_msg->content.ptr) {
            /* Build a content string in response */
            if (strncmp(rtsp_msg->content.ptr, "clab-connection-timeout", 
                        strlen("clab-connection-timeout")) == 0) {
                sprintf(constr, "connection_timeout: %d", ERMI_C_CONN_TIMEOUT);
                content_ptr = malloc(strlen(constr));
                if (content_ptr) {
                    memcpy(content_ptr, constr, strlen(constr));
                }
            } else if (strncmp(rtsp_msg->content.ptr, "clab-session-list",
                               strlen("clab-session-list")) == 0) {
                char *session_group = NULL;

                if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_SESSGRP)) {
                    session_group = rtsp_msg->session_group;
                }

                content_ptr = ermi_c_rtsp_get_sessionlist(session_group, 0);
            }
            else if (strncmp(rtsp_msg->content.ptr, "clab-sessiongroup-list",
                             strlen("clab-sessiongroup-list")) == 0) {

                if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_SESSGRP)) {
                    content_ptr = ermi_c_rtsp_get_sessionlist(
                                                rtsp_msg->session_group_list,
                                                MAX_SESSGRP_LIST_SIZE);
                }
            }
        }

        ermi_c_stats_incr_rcvd_success(ERMI_C_GET_METHOD(rtsp_msg->msg_type));

        /* build a message to send out */
        new_msg = rtsp_new_msg();
        if (new_msg == NULL) { /* nothing more we can do. bail */
            ermi_c_stats_incr_send_err(ERMI_C_GET_METHOD(rtsp_msg->msg_type));
            rtsp_free_msg(rtsp_msg);
            if (content_ptr) {
                free(content_ptr);
            }
            return;
        }

        new_msg->msg_type = RTSP_RESPONSE;
        sstrncpy(new_msg->url, rtsp_msg->url, MAX_URL_LEN);
        new_msg->resp_retcode = RTSP_RET_OK;
        new_msg->cseq = rtsp_msg->cseq;
        new_msg->session_ptr = rtsp_msg->session_ptr;

        if (content_ptr) {
            new_msg->content.ptr = content_ptr;
            new_msg->content.len = strlen(content_ptr);
        }

        status = rtsp_send_msg(conn, new_msg); 

        /* Free the rtsp_msg that we consumed, and the reply msg we created */
        rtsp_free_msg(rtsp_msg);
        rtsp_free_msg(new_msg);

        return;

    case RTSP_TEARDOWN:
        /* not a new session,session id must be present */
        ses = (rtsp_session_t *)rtsp_find_session(rtsp_msg->sessionID);
        if (ses == NULL) {
            errmsg(&msgsym(DATAERR, RTSP), "Bad session id in TEARDOWN", 
                   rtsp_msg->sessionID);
            ermi_c_rtsp_send_nosession_response(conn, rtsp_msg,
                                                RTSP_RET_SESSION_NOT_FOUND);
            goto rtsp_svr_process_err;
        }
        ses->recv_cseq = rtsp_msg->cseq;
        /* state change */
        if (ses->state == RTSP_SESSION_READY) {
            ses->state = RTSP_SESSION_IDLE;
        }
        break;

    case RTSP_RESPONSE:
        /* RTSP_RESPONSE is only rcvd for sent ANNOUNCE Requests. */

        /* search for the session based on the cseq */
        /* get the request-pending session */
        ERMI_TRACE;
        ses = vcp_find_hash_obj(rtsp_conn->req_table, &rtsp_msg->cseq);
        if (ses == NULL) {
            /* Did not find matching Request */
            ERMI_C_RTSP_SES_BUGINF("\n Unexpected Response with CSeq %d",
                                   rtsp_msg->cseq);
            goto rtsp_svr_process_err;
            return;
        }

        /* stop the timer if response is back */
        if (mgd_timer_running(&ses->resp_timer)) {
              mgd_timer_stop(&ses->resp_timer);
        }

        /* take out the session from the req_table */
        vcp_del_hash_obj(rtsp_conn->req_table, ses); 
        ses->request_pending = FALSE;

        /* we only expect a response for an ANNOUNCE request */
        if (ses->sent_method != RTSP_ANNOUNCE) {
            errmsg(&msgsym(DATAERR, RTSP), 
                       "Received unexpected response after message", 
                        rtsp_msg->msg_type);
            return;
        }
        ERMI_TRACE;
        break;

    default:
        errmsg(&msgsym(DATAERR, RTSP),
               "Received unexpected rtsp message type", rtsp_msg->msg_type);
        goto rtsp_svr_process_err;
        return;
    }

    /* save the cur_session */
    rtsp_conn->cur_session = ses;

    /* forward the msg to app */
    if (conn->app_msg_func == NULL) {        

        /* Need to figure out which app the msg must go to.
         * We do this based on the content of the msg
         */
        if (!RPM_TESTBIT(rtsp_msg->valid, RP_BIT_FTAG) ||
            (rtsp_msg->ftag != FTAG_ERMI)) {
            ERMI_C_RTSP_SES_BUGINF("\nNo Feature-tag found. Msg sent to ERM");
        }

        conn->app_msg_func = rtsps_app_func;
        if (conn->app_msg_func == NULL) {
            errmsg(&msgsym(DATAERR, RTSP), 
                   "Incoming message dropped. App is not initialized.", 
                   rtsp_msg->msg_type);
            goto rtsp_svr_process_err;
        }
    }

    ermi_c_stats_incr_rcvd_success(ERMI_C_GET_METHOD(rtsp_msg->msg_type));

    conn->app_msg_func(ses, rtsp_msg);        

    return;

rtsp_svr_process_err:

    ermi_c_stats_incr_rcvd_err(ERMI_C_GET_METHOD(rtsp_msg->msg_type));
    rtsp_free_msg(rtsp_msg);
}


/* close the session */
ermi_status rtsp_server_close_session (rtsp_session_t *rtsp_ses)
{   rtsp_conn_t *rtsp_conn;
    int   index;
    boolean bret;
 
    assert(rtsp_ses != NULL);
    assert(rtsp_ses->peer_erm != NULL);

    if (rtsp_ses == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "Bad pointer in rtsp_close_session",
               0, "rtsp_ses");
        return (ERMI_RTSP_ERROR);
    }

    rtsp_conn = RTSP_GET_SESSION_CONN(rtsp_ses);
    if (ERMI_C_GET_END_TYPE(rtsp_conn->conn->type) == RTSP_SVR_CONN) {
        ERMI_C_RTSP_SES_BUGINF("\nrtsp server at port %d close session %08X"
                        " from %i", rtsp_conn->conn->port,
                                                 rtsp_ses->session_id,
                                                 rtsp_conn->conn->src_addr);
    } else {
        ERMI_C_RTSP_SES_BUGINF("\nrtsp client at port %d close session %08X"
                        " to %i", rtsp_conn->conn->port,
                                                 rtsp_ses->session_id,
                                                 rtsp_conn->conn->src_addr);
    }

    /* delete the session from req_table */
    if (rtsp_ses->request_pending) {
        vcp_del_hash_obj(rtsp_conn->req_table, rtsp_ses); 
        rtsp_ses->request_pending = FALSE;
    }

    index = rtsp_ses->index;
    rtsp_conn->conn->ses_tbl[index] = NULL;

    /* recycle the index */
    list_enqueue(&(rtsp_conn->conn->ses_index_list), NULL, (void *)index);

    bret = ermi_c_rtsp_delete_session(rtsp_ses, rtsp_ses->session_id, 0);
    if (!bret) {
        errmsg(&msgsym(SWERR, RTSP), "Delete failed in rtsp_close_session",
               0, "rtsp_ses->conn");
    }

    /* Make sure that session timers are stopped */
    mgd_timer_stop(&rtsp_ses->resp_timer);
#ifdef SUPPORT_RTSP_CLIENT_KEEPALIVE
    mgd_timer_stop(&rtsp_ses->keepalive_timer);
#endif

    ERMI_C_RTSP_SES_BUGINF("\nERMI: Deleting RTSP session id: %#X [%08X] from "
                           "ses_tbl[%d]",
                           rtsp_ses, rtsp_ses->session_id, rtsp_ses->index);

    rtsp_free_session(rtsp_ses);
    return (ERMI_OK);
}

/* close the session */
ermi_status rtsp_client_close_session(rtsp_session_t *rtsp_ses)
{
    return rtsp_server_close_session(rtsp_ses);
}

/* When a request is sent out after milliseconds, the session resp_timer
   will go off, this API will be invoked by ermi_c_master.
 */
void rtsp_handle_ses_resp_timeout (mgd_timer *the_timer, 
                                        void *context)
{
    rtsp_session_t *ses = (rtsp_session_t *)context;
    rtsp_msg_t *new_msg;
    ermi_c_conn_t *conn;
    rtsp_conn_t *rtsp_conn;
 
    if (ses == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "Bad session context",
               0, "in rtsp response timer");
        return;
    }

    /* delete the session from req_table */
    rtsp_conn = RTSP_GET_SESSION_CONN(ses);

    if (ses->request_pending) {
        vcp_del_hash_obj(rtsp_conn->req_table, ses); 
        ses->request_pending = FALSE;
    }

    conn = rtsp_conn->conn;
    new_msg = rtsp_new_msg( );
    if (new_msg == NULL) {
        return;        
    }
    new_msg->msg_type = RTSP_RESP_TIMEOUT;
    buginf("\nsession %08X response time out", ses->session_id);
    /* notify app that this session timed out */
    assert(conn->app_msg_func != NULL);
    conn->app_msg_func(ses, new_msg);
    return;
}

/* When keepalive timer expires, RTSP client needs to send a request to make
 * sure that the remote server does not tear down the session.
 */
void rtsp_handle_ses_keepalive_timeout (mgd_timer *the_timer, 
                                        void *context)
{
    rtsp_session_t *ses = (rtsp_session_t *)context;
    rtsp_msg_t *rtsp_msg;
 
    if (ses == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "Bad session context",
               0, "in rtsp keepalive timer");
        return;
    }
    
    ERMI_C_RTSP_SES_BUGINF("\n %s: Keepalive Timeout." 
                        "Sending GET_PARAMETER Request", __FUNCTION__);

    rtsp_msg = rtsp_new_msg();
    if (rtsp_new_msg == NULL) {
        return;
    }

    /* Send a GET_PARAMETER request */
    rtsp_msg->msg_type = RTSP_GETPARAMETER;
    strcpy(rtsp_msg->url, ses->url); 
    rtsp_send_request(ses, rtsp_msg);
}

