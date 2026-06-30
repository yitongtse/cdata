/*
 *-------------------------------------------------------------------
 * ermi_c_rtsp_api.c  APIs that provides rtsp client/server services
 *
 * January 2004, Linda Hua: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by Cisco Systems, Inc.
 * All rights reserved.
 *-------------------------------------------------------------------
 */
#include COMP_INC(network, address.h)
#include COMP_INC(kernel/memory, chunk.h)
#include <sys/util/random.h>
#include COMP_INC(kernel, clock.h)
#include <socket/socket.h>
#include <sched.h>
#include <assert.h>
#include <logger.h>
#include COMP_INC(kernel, subsys.h)
#include "ermi_c_global.h"
#include "ermi_c_master.h"
#include "ermi_video_hash.h"
#include "ermi_video_debug.h"

#include INTERNAL_INC(sup/src/vscm/video_session_api.h)
#include INTERNAL_INC(sup/src/vclient/session_main.h)

#include "ermi_c_rtsp_def.h"
#include "ermi_c_rtsp_api.h"
#include "ermi_c_rtsp_session.h"
#include "ermi_c_rtsp_sock.h"
#include "msg_ermi_c_vcp.c"
#include "ermi_c_rtsp_parser.h"
#include "ermi_video_main.h"


/* global rtsp data */
chunk_type *rtsp_conn_chunk;
chunk_type *rtsp_session_chunk;
chunk_type *rtsp_msg_chunk;

app_func_t rtsps_app_func;

/* Size of the hash tables
 * Use primes for good hash behavior
 */
#define RTSPC_SESS_TABLE_SIZE 1009
#define RTSPC_REQ_TABLE_SIZE 59

int test_rtsp = 0;
boolean ermi_c_rtsp_enabled;

rfgw_ermi_c_stats_tab_t rfgw_ermi_c_stats;

char *rtsp_method_str[] = {
   "unknown method",
   "SETUP",
   "TEARDOWN",
   "SETPARAMETER",
   "RESPONSE",
   "ANNOUNCE",
   "GETPAMAMETER",
   "TIMEOUT EVENT",
   "CONNECTION ERROR",
};

extern void video_rtsp_parser_test(void);

void ermi_c_rtsp_subsys_init (subsystype *subsys)
{
    ermi_c_rtsp_enabled = FALSE;
}

ermi_c_peer_t *ermi_c_get_rtsp_conn_peer (rtsp_conn_t *rtsp_conn)
{
    if (rtsp_conn) {
        assert(rtsp_conn->conn);
        assert(rtsp_conn->conn->peer);
        assert(rtsp_conn->conn->peer->erm_info);
        return (rtsp_conn->conn->peer->erm_info->ermi_c_info);
    }
    return NULL;
}

extern ermi_status
ermi_c_create_send_announce (lc_event_type_t lc_event_type,
                             scm_session_id scm_sess_id,
                             uint32 last_state,
                             uint32 new_state,
                             uint32 error_code);

/*
 * RTSP Connection timeout, send ANNOUNCE
 */
void rtsp_handle_conn_keepalive_timeout (mgd_timer *timer, void *context)
{
    rtsp_conn_t *rtsp_conn = (rtsp_conn_t *)context;
    // ermi_c_conn_t *conn = rtsp_conn->conn;
    
    rtsp_conn->timer_retry_count++;
    /* If retries exhausted send an ANNOUNCE */
    if (rtsp_conn->timer_retry_count >= ERMI_C_RTSP_CONN_TIMER_RETRY_COUNT) { 
        ermi_c_create_send_announce(ERMI_C_LC_SESSION_EVENT, 0,
                                    1, 0, 1);
        return;
    }

    /* Restart the timer */
    mgd_timer_start(&rtsp_conn->conn_timer, ERMI_C_RTSP_CONN_TIMEOUT);

}

void ermi_c_rtsp_init (void)
{

    if (ermi_c_rtsp_enabled) {
        return;
    }

    /* if connect times out, close the connection */
    if (ermi_c_timer_init(RTSP_CONNECT_TIMER, rtsp_handle_sock_conn_timeout)
                          != ERMI_OK) {
        errmsg(&msgsym(RSCERR, RTSP),
                  "Failed to init rtsp resp timer");
        return;
    }

    if (ermi_c_timer_init(RTSP_RESPONSE_TIMER, rtsp_handle_ses_resp_timeout)
                          != ERMI_OK) {
        errmsg(&msgsym(RSCERR, RTSP),
                  "Failed to init rtsp resp timer");
        return;
    }

    if (ermi_c_timer_init(RTSP_CONN_KEEPALIVE_TIMER,
                          rtsp_handle_conn_keepalive_timeout)
                          != ERMI_OK) {
        errmsg(&msgsym(RSCERR, RTSP),
                  "Failed to init rtsp keepalive timer");
        return;
    }

#ifdef SUPPORT_RTSP_CLIENT_KEEPALIVE
    if (ermi_c_timer_init(RTSP_KEEPALIVE_TIMER, rtsp_handle_ses_keepalive_timeout)
                          != ERMI_OK) {
        errmsg(&msgsym(RSCERR, RTSP),
                  "Failed to init rtsp keepalive timer");
        return;
    }
#endif
    
    /* per connection */
    rtsp_conn_chunk = chunk_create(sizeof(rtsp_conn_t),
                                   MAX_ERMI_SERVERS*MAX_ERMI_MGMT_CONN,
                                   CHUNK_FLAGS_DYNAMIC,
                                   NULL, 0,
                                   "rtsp conn");
    assert(rtsp_conn_chunk != NULL);
    if (rtsp_conn_chunk == NULL) {
        errmsg(&msgsym(RSCERR, RTSP),
                  "Failed to create rtsp_conn_chunk");
        return;    
    }    

    /* per session */
    rtsp_session_chunk = chunk_create(sizeof(rtsp_session_t),
                                      RTSP_MAX_NUM_SESSIONS,
                                      CHUNK_FLAGS_DYNAMIC,
                                      NULL, 0,
                                      "rtsp session");
    assert(rtsp_session_chunk != NULL);
    if (rtsp_session_chunk == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "Failed to create rtsp_session_chunk");
        return;    
    }

    /* per msg */
    rtsp_msg_chunk = chunk_create(sizeof(rtsp_msg_t),
				  RTSP_MAX_NUM_SESSIONS, /*one msg per session*/
                                  CHUNK_FLAGS_DYNAMIC,
                                  NULL, 0,
                                  "rtsp msg");
    assert(rtsp_msg_chunk != NULL);
    if (rtsp_msg_chunk == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "Failed to create rtsp_msg_chunk");
        return;    
    }

    rfgw_global_statistics[RFGW_VIDEO_PROTOCOL_ERMI] = &rfgw_ermi_c_stats;
    ermi_c_stats_init();

    ermi_c_rtsp_enabled = TRUE;

    return;
}

void rtsp_free_msg (rtsp_msg_t *msg)
{
    if (msg->content.ptr) {
	free(msg->content.ptr);
    }

    if (msg->sessionlist) {
	free(msg->sessionlist);
    }	
    
    chunk_free(rtsp_msg_chunk, msg);
    return;
}

rtsp_msg_t *rtsp_new_msg (void)
{
    rtsp_msg_t *pb;

    pb = chunk_malloc(rtsp_msg_chunk);
    if (pb == NULL) {
	return NULL;
    }

    memset(pb, 0, sizeof(rtsp_msg_t));

    return pb;
}

const char *ermi_c_rtsp_method_str(ushort mcode)
{
    if (mcode < METHOD_SETUP || mcode > MSG_TYPE_LAST) {
        return ("unknown method");
    }

    return (rtsp_method_str[mcode]);
}

/* Parser gives us the content_length, and a pointer to the content buffer.
 * The buffer may contain all the content bytes, or some of the content 
 * bytes. Copy the content bytes in to the msg buffer. 
 * Return TRUE if all the content bytes were received.
 * Return FALSE if there are pending content bytes.
 */
boolean rtsp_copy_content_bytes (rtsp_conn_t *rtsp_conn, rtsp_msg_t *rtsp_msg)
{
    int count;

    if (rtsp_conn->in_size < rtsp_conn->remlen) {
	/* did not get all the content bytes.
	 * save what we got. 
	 */
	count = rtsp_conn->in_size;
	memcpy(rtsp_msg->content.ptr + rtsp_msg->content.curlen, 
	       rtsp_conn->read_pointer, count);
	rtsp_msg->content.curlen += count;
	rtsp_conn->remlen -= count;
	rtsp_conn->read_pointer += count;
	rtsp_conn->in_size = 0;
	return FALSE; /* did not get all the bytes */
    }
    
    count = rtsp_conn->remlen;
    memcpy(rtsp_msg->content.ptr + rtsp_msg->content.curlen, 
	   rtsp_conn->read_pointer, count);
    rtsp_msg->content.curlen += count;
    rtsp_conn->remlen = 0;
    rtsp_conn->in_size -= count;
    rtsp_conn->read_pointer += count;

    return TRUE;
}


/* parse the received rtsp msg and then process it and forward it to app 
 */
void rtsp_pmh_process_message (ermi_c_conn_t *conn)
{
    rtsp_msg_t *rtsp_msg;
    ermi_status status;
    rtsp_conn_t *rtsp_conn;
    int count;   /* number of chars parsed */
    
    assert(conn);
    rtsp_conn = (rtsp_conn_t *)conn->prot_info;
    assert(rtsp_conn);

    /* after we read from the socket, read_pointer always points to the
     * first byte of in_buf.
     * After we process all messages in the buf, we move the incomplete 
     * message to the beginning of the in_buf, and read from the socket again.
     */

    /* keep parsing the msgs in the input buffer */
    while (rtsp_conn->in_size > 0) {

	if (rtsp_conn->remlen > 0){
	    /* We were waiting for content bytes. 
	     * Deal with that first. 
	     */
	    if (rtsp_copy_content_bytes(rtsp_conn, rtsp_conn->saved_rtsp_msg) 
		== FALSE) {
		/* Still incomplete */
		return;
	    }

            if (conn->type == RTSP_SVR_CONN) {
                ermi_c_rtsp_svr_process_msg(conn, rtsp_conn->saved_rtsp_msg);
	    }
	    
	    rtsp_conn->saved_rtsp_msg = NULL;
	}

        rtsp_msg = rtsp_new_msg();
        if (rtsp_msg == NULL) {
            errmsg(&msgsym(RSCERR, RTSP), "rtsp_msg_chunk malloc failed");
            break;
	}
        status = ermi_c_rtsp_parse_message(rtsp_conn->read_pointer,
                                           rtsp_conn->in_size,
                                           rtsp_msg, 
                                           &count);
        if (status == ERMI_OK) {
            /* if count == 0, ignore it, read some more from socket */
            if (count == 0) {
                rtsp_free_msg(rtsp_msg);
                return;
	    }
  
            /* adjust the starting point of the buf */
            rtsp_conn->read_pointer += count;
            rtsp_conn->in_size -= count;
            if (conn->type == RTSP_SVR_CONN) {
                ermi_c_rtsp_svr_process_msg(conn, rtsp_msg);
	    }

	} else if (status == ERMI_PARSE_CONTENT_AFTER_MSG) {
	    rtsp_conn->in_size -= count;
	    rtsp_conn->read_pointer += count;

	    if (rtsp_msg->content.len < ERMI_C_MAX_CONTENT_LEN) {
		/* allocate memory to hold content */
		rtsp_msg->content.ptr = malloc(rtsp_msg->content.len);
	    }

	    if ( rtsp_msg->content.len >= ERMI_C_MAX_CONTENT_LEN ||
		 rtsp_msg->content.ptr == NULL) {
		errmsg(&msgsym(SWERR, RTSP), "Can't handle content len ", 
		       rtsp_msg->content.len, "bytes");
		rtsp_free_msg(rtsp_msg);
		return;
	    }

	    /* Don't give rtsp_msg to app until all the content comes in.
	     * Hold on to the rtsp_msg structure
	     */
	    rtsp_conn->saved_rtsp_msg = rtsp_msg;
	    rtsp_conn->remlen = rtsp_msg->content.len;
	    if (rtsp_copy_content_bytes(rtsp_conn, rtsp_conn->saved_rtsp_msg)
		== FALSE) {
		/* Return and wait for more socket input */
		return;
	    } else {
		/* Received complete msg plus content.
		 * Pass it up
		 */
		if (conn->type == RTSP_SVR_CONN) {
		    ermi_c_rtsp_svr_process_msg(conn, rtsp_conn->saved_rtsp_msg);
		}
	    
		rtsp_conn->saved_rtsp_msg = NULL;
	    }
	
	} else if (status == ERMI_PARSE_INCOMPLETE_MSG) {
            /* message is not complete yet, go back and read more from the
	       socket */
            rtsp_free_msg(rtsp_msg);
	    return;
	} else { /* (status == ERMI_C_PARSE_ERROR) */
            /* adjust the starting point of the buf, parser needs 
               to make sure that the count provided by parser is correct 
               although the msg is invalid, the count has to be  > 0, 
               otherwise, we're stuck in the infinite loop 
            */
            assert(count > 0);
            rtsp_conn->read_pointer += count;
            rtsp_conn->in_size -= count;
            rtsp_free_msg(rtsp_msg);
	}
    }
}

void rtsp_process_conn_evt (ermi_c_conn_t *conn, conn_event_e evt) 
{
    rtsp_conn_t *rtsp_conn;

    /* find the rtsp_conn */
    rtsp_conn = (rtsp_conn_t *)conn->prot_info;

    if (evt == SOCKET_ERROR) {

        ERMI_C_RTSP_API_BUGINF("\nSOCKET_ERROR for fd %d", conn->fd);

        /* socket_read() returns 0 or -1, socket is closed or broken 
           send event nofication to app, let app invoke
	   rtsp_server_close_session( ) to
           close all sessions in this connection. 
        */
        rtsp_close_conn(rtsp_conn);
        socket_close(conn->fd);
        conn->fd = INVALID_FD;
        conn->state = SOCK_IDLE;
    }
}

/* there is a socket event for RTSP */
void rtsp_process_conn_msg (ermi_c_conn_t *conn, ulong socket_event)
{
    ermi_status status;
    rtsp_conn_t *rtsp_conn;
    boolean read_event = FALSE;
    boolean write_event = FALSE;

    assert(conn != NULL);
    if (conn == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "null pointer conn", conn, ".");
        return;
    }

    rtsp_conn = (rtsp_conn_t *)conn->prot_info;

    if (socket_event & SOCKET_READ_EVENT)
	read_event = TRUE;
    if (socket_event & SOCKET_WRITE_EVENT)
	write_event = TRUE;
    if (!read_event && !write_event) {
	errmsg(&msgsym(SWERR, RTSP), "Unknown socket event type", 
	       socket_event,".");
	return;
    }

    /* If socket state was PENDING, now it is CONNECTED.
     * If there's queued data, send it.
     */
    if (conn->state == SOCK_CONNECT_PENDING) {
	conn->state = SOCK_CONNECTED;
	
	/* stop the connect timer */ 
	mgd_timer_stop(&rtsp_conn->conn_timer);
	
	if (read_event) {	
	    rtsp_sock_clear(rtsp_conn->conn->fd, SOCKET_READ_EVENT);
	    read_event = FALSE; /* already handled */
	}
    }

    /* handle WRITE event */
    if (write_event) {
        ERMI_C_RTSP_API_BUGINF("\nSOCKET_WRITE EVENT for fd %d", 
                              rtsp_conn->conn->fd);

        status = rtsp_handle_sock_write(rtsp_conn);
        if (status == ERMI_RTSP_SOCK_ERROR) {
    	    rtsp_process_conn_evt(conn, SOCKET_ERROR);
        }
    }

    /* handle READ event */
    if (read_event) {

        ERMI_C_RTSP_API_BUGINF("\nSOCKET_READ EVENT for fd %d", 
			    rtsp_conn->conn->fd);

        status = rtsp_sock_read(rtsp_conn);
        /* parser depends on the NUL terminator to parse */
        rtsp_conn->in_buf[rtsp_conn->in_size] = '\0';
        if (status == ERMI_OK) {
            /* rtsp_pmh_process_message has to save cseq in rtsp_conn
             * if it was a SETUP request
             */
            rtsp_pmh_process_message(conn);
            /* no matter how much text is left in the in_buf, 
               move it to the beginning of in_buf */
            if (rtsp_conn->in_size > 0) {
		memmove(rtsp_conn->in_buf, rtsp_conn->read_pointer, 
                        rtsp_conn->in_size);
	    } 
            rtsp_conn->read_pointer = rtsp_conn->in_buf;
            return;
        } else {
            /* got error */
            rtsp_process_conn_evt(conn, SOCKET_ERROR);
            return;
        }
    }

    return;
}

/*
 * Initialize RTSP Server
 *
 * INPUTS
 *	local_addr:  local IP address.  
 *	listen_port: port to listen on
 *      app_fn:      function to call to handle incoming msg
 *            The prototype is void (*app_func_t)(void *ses, void *prot_msg)
 *            The first argument is (rtsp_session_t *), 
 *	      the second argument is (rtsp_msg_t *)
 * OUTPUTS
 *	None
 *      
 * RETURNS
 *	ERMI_OK if successful 
 *
 * NOTES
 * An EQAM (ERMI-2) stack uses this function to register a callback handler
 * with the RTSP server.
 * After parsing an incoming SETUP message for a new session, the RTSP
 * server determines the feature-tag in the "Require" header. If the value is
 * "ermi", then it calls the ERMI-2 handler; else ignores.
 */
ermi_status rtsp_init_server (ipaddrtype local_addr, ushort listen_port,
                              app_func_t app_fn)
{
    int listen_fd;

    /* We assume that callback_handler for the application. */
    rtsps_app_func = app_fn; /* store the callback handler */
    
    ERMI_TRACE;

    /* Check - if RTSP Server is already initialized */
    if ( ermi_c_svr_conn.fd != INVALID_FD && ermi_c_svr_conn.is_listen_fd ) {
	/*
         * Multiple apps may listen on the same port. That's ok, Just inform OK.
	 */
        errmsg(&msgsym(SWERR, RTSP), "RTSP Server on listen port", 
		listen_port, "already initialized");
        return (ERMI_OK);
    }

    /* socket open, bind, listen */
    listen_fd = rtsp_svr_sock_open(local_addr, listen_port); 
    if (listen_fd == INVALID_FD) {
        return (ERMI_RTSP_ERROR);
    }

    ERMI_TRACE;

    /* Initialize common global data struct connection table on re-starting */
    memset(&ermi_c_svr_conn, 0, sizeof(ermi_c_conn_t));

    /* save event handler and msg handler */
    ermi_c_svr_conn.prot_evt_func = rtsp_process_conn_evt;
    ermi_c_svr_conn.prot_msg_func = rtsp_process_conn_msg;
    ermi_c_svr_conn.app_msg_func  = NULL;    /* will be init-ed on recv
					      * of 1st message from client */
    ermi_c_svr_conn.type  = RTSP_SVR_CONN;
    ermi_c_svr_conn.state = SOCK_CONNECTED;
    ermi_c_svr_conn.port  = listen_port;
    ermi_c_svr_conn.fd    = listen_fd;
    ermi_c_svr_conn.is_listen_fd = TRUE;
    ermi_c_svr_conn.peer  = NULL; /* No Pre-set peer, ever, for generic conn */

    video_rtsp_parser_test();

    return (ERMI_OK);
}

/* 
 * for use by the hash table APIs, when table-key is Cseq
 */
void* ermi_c_rtspc_cseq_id_func (void* obj)
{
    rtsp_session_t *ses;
    
    ses = obj;
    return &(ses->send_cseq);
}

/* 
 * for use by the hash table APIs, when table-key is session-id
 */
void* ermi_c_rtspc_session_id_func (void* obj)
{
    rtsp_session_t *ses;
    
    ses = obj;
    return &(ses->session_id);
}

/*
 * Init ERMI-C session tables.
 */
static boolean ermi_c_init_session_tables (ermi_c_conn_t *conn)
{
    list_header *list;
    int index;

    assert(conn);
    assert(conn->prot_info);

    /* initialize the session index list */
    if (!LIST_VALID(&conn->ses_index_list)) { // NK_TBD - Correct the check

        list = list_create(&(conn->ses_index_list), 0, "session index list", 
                           LIST_FLAGS_AUTOMATIC);
        if (list == NULL) {
            errmsg(&msgsym(RSCERR, RTSP), "Failed to create ses_index_list");
            return (FALSE);    
        }    

        for (index = 0; index < RTSP_SESSION_TABLE_LEN; index++) {
	    if (!list_enqueue(&(conn->ses_index_list), NULL, (void *)index)) {

                errmsg(&msgsym(RSCERR, RTSP),"Failed to init ses_index_list");
                list_destroy(&(conn->ses_index_list));
                return (FALSE);
	    }
        }

        /* initialize the ses_tbl to all zeros */
        memset(conn->ses_tbl, 0,sizeof(rtsp_session_t*)*RTSP_SESSION_TABLE_LEN);
    }

    return (TRUE);

}

/* the rtsp_conn init API */
rtsp_conn_t *rtsp_new_conn (ermi_c_conn_t *conn, conn_type_e type)
{
    rtsp_conn_t *rtsp_conn = NULL;

    rtsp_conn = (rtsp_conn_t *)conn->prot_info;

    if (rtsp_conn == NULL) {
        rtsp_conn = chunk_malloc(rtsp_conn_chunk);
    }

    if (rtsp_conn == NULL) {
        errmsg(&msgsym(RSCERR, RTSP), "rtsp_conn_chunk malloc failed");
        return (NULL);
    }
    
    /* remember each other */
    conn->prot_info = rtsp_conn;
    rtsp_conn->conn = conn;

    /* Init session tables on 1st successful creation of RTSP new connection */
    if (!ermi_c_init_session_tables(conn)) {
        chunk_free(rtsp_conn_chunk, rtsp_conn);
        return NULL;
    }

    rtsp_conn->send_cseq = RTSP_INITIAL_CSEQ;

    if (!rtsp_conn->req_table) {
        /* Initialize hash-table to hold pending requests. key is CSeq */
        rtsp_conn->req_table = vcp_create_hash_table(RTSPC_REQ_TABLE_SIZE,
                                                     VCP_HASH_TYPE_INT,
			                             vcp_int_hash_func,
                                                     ermi_c_rtspc_cseq_id_func,
                                                     vcp_int_lookup_func);
        if (!rtsp_conn->req_table) {
	    printf("\n RTSP Client failed to create Request table for fd %d",
                        conn->fd);
	    return NULL;
        }	
    }	
    
    /* initialize the read_pointer */
    rtsp_conn->read_pointer = rtsp_conn->in_buf;

    /* init send_list */
    queue_init(&rtsp_conn->send_list, 0);

    /* init the timer, context is rtsp_conn */
    mgd_timer_init_leaf(&rtsp_conn->conn_timer,
                        &ermi_c_parent_timer,
                        RTSP_CONNECT_TIMER, rtsp_conn, FALSE);

    mgd_timer_init_leaf(&rtsp_conn->keepalive_timer,
                        &ermi_c_parent_timer,
                        RTSP_CONN_KEEPALIVE_TIMER, rtsp_conn, FALSE);

    /* start the timer if the conn is in pending state */
    if (ERMI_C_GET_END_TYPE(conn->type) == CLIENT_CONN &&
        conn->state == SOCK_CONNECT_PENDING) {
        if (mgd_timer_running(&rtsp_conn->conn_timer)) {
            mgd_timer_stop(&rtsp_conn->conn_timer);
        }
        mgd_timer_start(&rtsp_conn->conn_timer, SOCK_CONN_TIMEOUT);
    }

    mgd_timer_start(&rtsp_conn->conn_timer, ERMI_C_RTSP_CONN_TIMEOUT);

    rtsp_conn->timer_retry_count = 0;

    rtsp_conn->remlen = 0;
    rtsp_conn->saved_rtsp_msg = NULL;

    return (rtsp_conn);
}

/* 
 * Cleanup transient sessions/pending_msg context.
 * Keep session table and indices as is.
 */
void rtsp_close_conn (rtsp_conn_t *rtsp_conn)
{
    ermi_c_conn_t *conn;
    rtsp_buf_t *rtsp_buf;

    conn = rtsp_conn->conn;

    if (conn->type == RTSP_SVR_CONN) {
        /* close the connection from a client */
        ERMI_C_RTSP_API_BUGINF("\nClosing connection %i:%d with %i",
			       conn->src_addr, conn->port, conn->dest_addr);
    }

    /* free send bufs */
    while (!QUEUEEMPTY(&rtsp_conn->send_list)) {
       rtsp_buf = dequeue(&rtsp_conn->send_list);
       free(rtsp_buf);        
    }

    /* destroy req_table */
    vcp_destroy_hash_table(rtsp_conn->req_table);

    /* stop the timer*/
    mgd_timer_stop(&rtsp_conn->conn_timer);

    /* stop the timer*/
    mgd_timer_stop(&rtsp_conn->keepalive_timer);
    
    if (rtsp_conn->saved_rtsp_msg) {
        rtsp_free_msg(rtsp_conn->saved_rtsp_msg);
    }

    /* free the chunk */
    chunk_free(rtsp_conn_chunk, rtsp_conn);
}

ermi_status rtsp_cleanup_outbuf(char *out_buf, uint len)
{
    memset(out_buf, 0, len);

    return ERMI_OK;
}

ermi_status rtsp_send_msg (ermi_c_conn_t *conn, rtsp_msg_t *rtsp_msg)
{
    ermi_status status;
    uint32 len;
    rtsp_conn_t *rtsp_conn;
   
    if (rtsp_msg->session_ptr) {
        /*
         * Use Session specific connection to send out the msg, as 1st priosity.
         * This is valid for per-session msgs whose session info is available
         */
        rtsp_conn = RTSP_GET_SESSION_CONN(rtsp_msg->session_ptr);
    } else {
        /*
         * For non-per-session type msgs like GET_PARAM, etc, use
         * RTSP connection over which the request msg was rcvd.
         */
        rtsp_conn = (rtsp_conn_t *)conn->prot_info;
    }

    /* encode the msg into text */
    status = ermi_c_parser_rtsp_encode(rtsp_msg, rtsp_conn->out_buf, 
                                       RTSP_OUT_BUF_SIZE, &len);
    if (status != ERMI_OK) {
        ermi_c_stats_incr_send_err(ERMI_C_GET_METHOD(rtsp_msg->msg_type));
        return (status);
    }

    /*
     * Adjust buf size, what does ermi_c_parser_rtsp_encode()
     * do to out_buf when encoding fails, or encoding never fails.
     */
    rtsp_conn->out_size += len;

    /* send the msg through the socket */
    status = rtsp_sock_send(rtsp_conn, rtsp_conn->out_buf, len);

    if (status != ERMI_OK) {
        ermi_c_stats_incr_send_err(ERMI_C_GET_METHOD(rtsp_msg->msg_type));
    } else {
        ermi_c_stats_incr_send_success(ERMI_C_GET_METHOD(rtsp_msg->msg_type));
    }

    rtsp_cleanup_outbuf(rtsp_conn->out_buf, len);
    return (status);
}

ermi_status rtsp_send_request (rtsp_session_t *ses, rtsp_msg_t *rtsp_msg)
{
    rtsp_conn_t *rtsp_conn;
    ermi_status ret;

    if (ses == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "null rtsp session");
        rtsp_free_msg(rtsp_msg);
        return (FALSE);        
    }

    if (rtsp_msg == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "null rtsp msg");
        return (FALSE);        
    }

    /* everytime we send out a request, we upate and set the send_cseq */
    rtsp_conn = RTSP_GET_SESSION_CONN(ses);

    rtsp_msg->cseq = rtsp_conn->send_cseq++;
    ses->send_cseq = rtsp_msg->cseq;
    ses->sent_method = rtsp_msg->msg_type;

    /* make sure session_id is copied to the rtsp_msg */
    if (rtsp_msg->msg_type != RTSP_SETUP && 
        RPM_TESTBIT(rtsp_msg->valid, RP_BIT_SESSIONID)) {
        rtsp_msg->sessionID = ses->session_id;
    }

    if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_CLIENTSESSIONID)) {
        strcpy(rtsp_msg->clientsessionID, ses->clientsession_id); 
    }

    rtsp_conn->cur_session = ses;
    ret = rtsp_send_msg(rtsp_conn->conn, rtsp_msg);
    
    if (ret == ERMI_OK || ret == ERMI_RTSP_SOCK_BLOCKED) {
	ses->request_pending = TRUE;

	/* Add this session to the table of 
	 * requests-that-are-waiting-for-response
	 */
	vcp_add_hash_obj(rtsp_conn->req_table, ses);
    } else {
        // Cleanup the msg context.
        ERMI_TRACE;
	return ERMI_FAIL;
    }

    /* start the response timer only if we send request */
    // ses = rtsp_conn->cur_session;
    if (mgd_timer_running(&ses->resp_timer)) {
        mgd_timer_stop(&ses->resp_timer);
    }

    mgd_timer_start(&ses->resp_timer, RTSP_SES_RESP_TIMEOUT);

#ifdef SUPPORT_RTSP_CLIENT_KEEPALIVE
	/* Restart keepalive timer for RTSP Client */
	if (rtsp_conn->conn->type == RTSP_CLIENT_CONN) {
	    mgd_timer_stop(&ses->keepalive_timer);
	    mgd_timer_start(&ses->keepalive_timer, ses->keepalive_duration);
	}
#endif

    return ret;
}

ermi_status rtsp_send_response (rtsp_session_t *ses, rtsp_msg_t *rtsp_msg)
{
    rtsp_conn_t *rtsp_conn;

    if (ses == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "null rtsp session");
        rtsp_free_msg(rtsp_msg);
        return (FALSE);        
    }

    if (rtsp_msg == NULL) {
        errmsg(&msgsym(SWERR, RTSP), "null rtsp msg");
        return (FALSE);        
    }

    /* send_cseq should be same as recv_cseq */
    rtsp_msg->cseq = ses->recv_cseq;

    /* make sure session_id is copied to the rtsp_msg */
    if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_SESSIONID)) {
         rtsp_msg->sessionID = ses->session_id;
    }

    /* make sure ondemandsession_id is copied to the rtsp_msg */
    if (RPM_TESTBIT(rtsp_msg->valid, RP_BIT_CLIENTSESSIONID)) {
        strcpy(rtsp_msg->clientsessionID, ses->clientsession_id); 
    }

    /* Prepare to send out the msg */
    ses->sent_method = rtsp_msg->msg_type;

    rtsp_conn = RTSP_GET_SESSION_CONN(ses);
    rtsp_conn->cur_session = ses;

    return (rtsp_send_msg(rtsp_conn->conn, rtsp_msg));
}

uint16 rtsp_get_prev_method (rtsp_session_t * ses) 
{
    return ses->sent_method;
}


const char *ermi_c_rtsp_msg_type_str(rp_msg_type_e msg_type)
{
    const char *work_str;

    switch (msg_type) {
        case RTSP_SETUP:
            work_str = "RTSP_SETUP";
            break;

        case RTSP_TEARDOWN:
            work_str = "RTSP_TEARDOWN";
            break;

        case RTSP_SETPARAMETER:
            work_str = "RTSP_SETPARAMETER";
            break;

        case RTSP_RESPONSE:
            work_str = "RTSP_RESPONSE";
            break;

        case RTSP_RESP_TIMEOUT:
            work_str = "RTSP_RESP_TIMEOUT";
            break;

        case RTSP_GETPARAMETER:
            work_str = "RTSP_GETPARAMETER";
            break;

        case RTSP_CONN_ERROR:
            work_str = "RTSP_CONN_ERROR";
            break;

        default:
            work_str = "Unknown RTSP message type";
            break;
    }

    return (work_str);
}


/*
 * ERMI-C subsystem header
 */
#define ERMI_C_RTSP_MAJVERSION 1
#define ERMI_C_RTSP_MINVERSION 0
#define ERMI_C_RTSP_EDITVERSION 1
SUBSYS_HEADER(ermi_c_rtsp, ERMI_C_RTSP_MAJVERSION, ERMI_C_RTSP_MINVERSION,
              ERMI_C_RTSP_EDITVERSION,
              ermi_c_rtsp_subsys_init, SUBSYS_CLASS_PROTOCOL,
              "seq: ermi_c rtsp");

