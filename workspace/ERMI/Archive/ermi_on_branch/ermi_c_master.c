/* 
 *-------------------------------------------------------------------
 * ermi_c_master.c - Provides major code structs for ERMI-Video ERMI-C
 * master process to handle socket events, timer events and message events
 * in a uniform way.
 *
 * January 2004, Linda Hua: July-2008, Neeraj Khurana
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#include COMP_INC(network, address.h)
#include <config.h>
#include <sched.h>
#include <logger.h>
#include COMP_INC(kernel, subsys.h)
#include <assert.h>
#include COMP_INC(ip, ip.h)

#include "ermi_video_debug.h"
#include "ermi_video_hash.h"
#include "ermi_c_global.h"
#include "ermi_c_master.h"
#include "ermi_c_rtsp_api.h"
#include "msg_ermi_c_vcp.c"
#include "parser_defs_ermi_c.h"
#include "ermi_video_main.h"

#include INTERNAL_INC(sup/src/vclient/session_main.h)

ermi_c_timer_t timer_table[LAST_TIMER_TYPE];  /* table of timer */

mgd_timer  ermi_c_parent_timer;      /* control plane parent timer */

ipaddrtype  local_ip;                /* master's local ip addr */

ermi_c_conn_t ermi_c_svr_conn;

extern void test_ermi_c_rtsp(uint32 message, cli_test_msg_t *cli_test_msg);

extern void rfgw_rsp_service_stop (rfgw_video_protocol_type video_protocol);

/* invoke RTSP server/client from CLI */

static void ermi_c_process_message (void)
{
    uint32 message;
    cli_test_msg_t *cli_test_msg;
    int *msg;
    uint32 value;

    process_get_message(&message, (void **)&msg, &value);

    /* the rtsp test code */
    if (message >= ERMI_C_RTSP_SERVER_INIT &&
        message <= ERMI_C_TEST_RTSP_NO) {
        cli_test_msg = (cli_test_msg_t *)(*msg);
        test_ermi_c_rtsp(message, cli_test_msg);
        free(cli_test_msg);
        free(msg);

        return; 
    }

    /* other messages from CLI */
    switch(message) {
    case ERMI_C_ENABLE:
        ermi_video_start();
	break;

    case ERMI_C_DISABLE:
        ermi_video_stop();
        break;

    default:
        buginf("\nunrecognized message %d.", message);
	break;
    }

}    

/* RTSP, VREP, ... should call this to add an entry in timer_table */
ermi_status ermi_c_timer_init(timer_type_e type,  /* the type of the timer */
                              timer_func_t timer_fn)  /* the handler for this
                                                         type of timer */
{
    if (timer_table[type].timer_fn != NULL) {
        errmsg(&msgsym(SWERR, VCP), "timer init for type", type, "failed");
        return (ERMI_TIMER_ERROR);
    }
    timer_table[type].type = type;
    timer_table[type].timer_fn = timer_fn;
    return(ERMI_OK);
}

/* RTSP, VREP, ... should call this to delete an entry from timer_table */
void ermi_c_timer_remove(timer_type_e type)  /* the type of the timer */
{
    if (timer_table[type].timer_fn == NULL) {
        errmsg(&msgsym(SWERR, VCP), "timer type", type, "doesn't exist");
        return;
    }
    timer_table[type].type = 0;
    timer_table[type].timer_fn = NULL;
    return;
}

ermi_protocol_info_t *
ermi_c_get_ermi_proto_info (rfgw_video_server_group *video_sg)
{
    rfgw_protocol_process_info *my_proto_info = NULL;

    if (!video_sg) {
        my_proto_info = &(RFGW_ERMI_INFO);
        video_sg = my_proto_info->rsp_sg;
        if (!video_sg) {
            return NULL;
        }
    }
    return ((ermi_protocol_info_t *)(video_sg->proto_specific_info));
}

ermi_c_peer_t *
ermi_c_get_peer_erm_info (rfgw_video_server_group *video_sg)
{
    ermi_protocol_info_t *ermi_proto_info;

    ermi_proto_info = ermi_c_get_ermi_proto_info(NULL);
    if (ermi_proto_info) {
        return (ermi_proto_info->ermi_c_info);
    }

    return NULL;
}

/*
 * control process dispatches socket events based on connection table
 */
static void ermi_c_process_socket_event (void)
{
    ulong serv_loop;
    ulong socket_events;
    int   fd, accept_fd;
    ermi_c_peer_t *peer;
    
    peer = ermi_c_get_peer_erm_info(NULL);
    if (!peer) {
        return;
    }

    accept_fd = INVALID_FD;
    serv_loop = MAX_SOCKET_SERVICE_LOOP;
    while (serv_loop-- && process_get_socket_event(&fd, &socket_events)) {

        /* is the event from a listening socket? */
        if ((ermi_c_svr_conn.fd == fd) && ermi_c_svr_conn.is_listen_fd) {

            if (ermi_c_svr_conn.type == RTSP_SVR_CONN) {
                accept_fd = rtsp_svr_sock_accept(fd);
            }

            /* save this new conn for the peer */
            if (accept_fd == INVALID_FD) {
                buginf("\nError: Accepting new connection on socket %d", fd);
                continue;
	    }
        } else {
           /* is the event from established socket,
           read the msg, parse it and then pass it to app */
           assert(peer->conn.prot_msg_func);
           peer->conn.prot_msg_func(&peer->conn, socket_events);
        }
    }
}

/* control process dispatches timer events based on timer table */
static void ermi_c_process_timer_event(void)
{
    mgd_timer *expired_timer;
    ushort type;
    
    while (mgd_timer_expired(&ermi_c_parent_timer)) {
        process_may_suspend();
        expired_timer = mgd_timer_first_expired(&ermi_c_parent_timer);
        mgd_timer_stop(expired_timer);

        type = mgd_timer_type(expired_timer);

        if (type < LAST_TIMER_TYPE) {
            ERMI_C_BUGINF("\ntimer type %d expired", type);
            assert(timer_table[type].timer_fn != NULL);
            timer_table[type].timer_fn(expired_timer,
                                       mgd_timer_context(expired_timer));
        } else {
            ERMI_C_BUGINF("unknown timer type %d", type);
            errmsg(&msgsym(SWERR, VCP), "bad type", type, "in timer");
        }
    }
}

/*
 * ermi_c_rtsp_resp_enque()
 *
 * Function to asynchronously handle RTSP response messages that are enqueued
 * to response message Queue.
 *
 * Parameters: rtsp_msg_t pointer to buffer to be enqueued to the Resp Queue.
 */
ermi_status
ermi_c_rtsp_resp_enque (rtsp_msg_t *rtsp_resp_msg)
{
    rfgw_protocol_process_info *rsp_proc_info = NULL;

    ERMI_TRACE;

    /*
     * TBD: When there would be one reply process per ERMI Server-group.
     * then Process ID would come on per-ERMI server-group basis. Like:
     * rsp_proc_info = (rtsp_sess_ctx..->..ermi_server_grp)->rsp_proc_info;
     */
    rsp_proc_info = &(RFGW_ERMI_INFO);

    if (!process_exists(rsp_proc_info->req_proc_id)) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: Process '%s' (%d) NOT found.",
                     rsp_proc_info->req_proc_name, rsp_proc_info->req_proc_id);
        rsp_proc_info->req_proc_id = NO_PROCESS;
        return (ERMI_FAIL);
    }

    if (rtsp_resp_msg == NULL) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: Invalid input");
        return (ERMI_FAIL);
    }

    if (rtsp_resp_msg->session_ptr == NULL) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: Invalid input");
        rtsp_free_msg(rtsp_resp_msg);
        return (ERMI_FAIL);
    }

    ERMI_TRACE;
    ERMI_C_RTSP_API_BUGINF("\nEnqueing into Q %#X; RTSP Msg: %#X",
                           rsp_proc_info->rsp_msgQ, rtsp_resp_msg);

    if (!process_enqueue(rsp_proc_info->rsp_msgQ, rtsp_resp_msg)) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: Unable to enqueue a session"
                      " response to rsp_msgQ "
                      "for server-group [%s : IP %i]. Max in queue = %d",
                      rsp_proc_info->rsp_sg->sg_name,
                      rsp_proc_info->rsp_sg->video_servers[0],
                      process_queue_size(rsp_proc_info->rsp_msgQ));
        rtsp_free_msg(rtsp_resp_msg);
        return (ERMI_FAIL);
    }

    return (ERMI_OK);
}


/*
 * ermi_c_process_resp_queue()
 *
 * Function to enqueue created RTSP response messages to the response msg Queue.
 *
 * Parameters: None
 */
void
ermi_c_process_resp_queue (void)
{
    rtsp_msg_t *rtsp_resp_msg;
    rtsp_conn_t *rtsp_conn;
    ermi_status sent;

    /*
     * TBD: When there would be one reply process per ERMI Server-group.
     * then Process ID would come on per-ERMI server-group basis. Like:
     * rsp_proc_info = (rtsp_sess_ctx->ermi_server_grp)->rsp_proc_info;
     */
    rfgw_protocol_process_info *rsp_proc_info = &(RFGW_ERMI_INFO);

    /* Now send as many response messages as possible */
    while ((rsp_proc_info->rsp_msgQ != NULL) &&
           (!process_is_queue_empty(rsp_proc_info->rsp_msgQ))) {

        rtsp_resp_msg= (rtsp_msg_t*)process_peek_queue(rsp_proc_info->rsp_msgQ);

        ERMI_TRACE;
        assert(rtsp_resp_msg);

        if (rtsp_resp_msg != NULL) {
            rtsp_conn = RTSP_GET_SESSION_CONN(rtsp_resp_msg->session_ptr);
            rtsp_conn->cur_session = rtsp_resp_msg->session_ptr;
            ERMI_TRACE;
            sent = rtsp_send_request(rtsp_resp_msg->session_ptr, rtsp_resp_msg);
            if (sent == ERMI_OK || sent == ERMI_RTSP_SOCK_BLOCKED) {

                /* Response sent successfully. We can pull msg off the queue */
                (void)process_dequeue(rsp_proc_info->rsp_msgQ);
                rtsp_free_msg(rtsp_resp_msg);
                ERMI_TRACE;

            } else {

                /* Failed to send msg */
                ERMI_C_RTSP_API_BUGINF("\n%s: Failed to send msg",__FUNCTION__);

                // Increment Msg Send Error Count
            }
        }

        process_may_suspend();
    }
}



/*
 * Generic OS objects initialization procedures for Reponse Process
 */
boolean
rfgw_ssp_proc_sys_init (rfgw_video_protocol_type video_protocol,
                        rfgw_protocol_process_info *my_proc_info)
{
    char q_name_txt[MAX_QUEUE_NAME_LEN];

    /* Establish a teardown handler */
    // signal_permanent(SIGEXIT, rfgw_rsp_process_stop); // Use ERMI handler

    process_wait_on_system_init();
    (void)process_set_crashblock(THIS_PROCESS, TRUE);

    /* Get the protocol type */
    (void)process_get_arg_num((ulong *)(&video_protocol));

    /* Created a watched queue for servicing incomming responses */
    snprintf(q_name_txt, MAX_QUEUE_NAME_LEN,
             "%s Proc Message Queue", rfgw_video_protocol_name[video_protocol]);

    my_proc_info->rsp_msgQ = create_watched_queue(q_name_txt,
                                                  RSP_MSGQ_MAX_LEN, 0);
    if (my_proc_info->rsp_msgQ == NULL) {
        SSP_ERRORS("\nRFGW %s(RSP): %%ERROR: Unable to create a watched queue.",
                   rfgw_video_protocol_name[video_protocol]);
        process_kill(THIS_PROCESS);
        return (FALSE);
    }

    /* Let the scheduler know what our interest is */
    process_watch_queue(my_proc_info->rsp_msgQ, ENABLE, RECURRING);

    /* Let watch the socket event */
    process_watch_socket_event(SOCKET_READ_EVENT, ENABLE, RECURRING);

    /* Enable the master timer */
    process_watch_mgd_timer(&my_proc_info->master_timer, ENABLE);
    mgd_timer_init_parent(&my_proc_info->master_timer, NULL);

    return (TRUE);
}

/*
 * Generic Request/Response Process Related Routines
 */
void ermi_c_rsp_svc_run (rfgw_protocol_process_info *my_proc_info)
{
    uint32 major, minor;

    while (TRUE) {
        process_wait_for_event();
        while (process_get_wakeup(&major, &minor)) {
            switch (major) {
            case QUEUE_EVENT:
                ERMI_TRACE;
                ERMI_C_BUGINF("\nERMI: Process [%d] Rcvd Queue Event.",
                              my_proc_info->rsp_proc_id);
                // ermi_c_process_resp_queue();
                break;

            case TIMER_EVENT:
                ERMI_C_BUGINF("\nERMI: Process [%d] Rcvd TIMER_EVENT.",
                              my_proc_info->rsp_proc_id);
                break;

            case SOCKET_EVENT:
            case DIRECT_EVENT:
            default:
                ERMI_C_BUGINF("\nERMI: %%ERROR: Process [%d] received"
                              " Unknown EVENT major = %d, minor = %d",
                              my_proc_info->rsp_proc_id,
                              major, minor);
                break;
            }
        }
    }
}


process
ermi_c_resp_process (void)
{
    rfgw_protocol_process_info  *my_proc_info;
    rfgw_video_protocol_type     video_protocol;

    video_protocol = RFGW_VIDEO_PROTOCOL_ERMI;
    my_proc_info   = &rfgw_session_global_info.proc_info[video_protocol];

    /* Let the scheduler know what our interest is */
    if (my_proc_info->rsp_msgQ) {
        process_watch_queue(my_proc_info->rsp_msgQ, DISABLE, RECURRING);
    }

    /* Let watch the socket event */
    process_watch_socket_event(SOCKET_READ_EVENT, DISABLE, RECURRING);

    ermi_c_rsp_svc_run(my_proc_info);

    process_kill(THIS_PROCESS);
}

extern void ermi_c_process_resp_queue(void);

void ermi_c_req_svc_run (void)
{
    uint32 major, minor;

    while (TRUE) {
        process_wait_for_event();
        while (process_get_wakeup(&major, &minor)) {

            switch (major) {

            case MESSAGE_EVENT:
                ermi_c_process_message( );
                break;

            case QUEUE_EVENT:
                ERMI_TRACE;
                ermi_c_process_resp_queue( );
                break;

            case TIMER_EVENT:
                ermi_c_process_timer_event( );
                break;

            case SOCKET_EVENT:
                ermi_c_process_socket_event( );
                break;

            case DIRECT_EVENT:
                break;

            default:
		ERMI_C_BUGINF("\nErmi-c receives an unknown event.");
                break;
            }   /* End of switch */

            process_may_suspend();
        }
    }
}

extern void rfgw_sg_server_failure_notify(rfgw_video_protocol_type video_proto);

/* the master control process */
process
ermi_c_master_proc (void)
{
    rfgw_protocol_process_info  *my_proc_info;
    rfgw_video_protocol_type     video_protocol;

    /* Set the protocol type */
    video_protocol = RFGW_VIDEO_PROTOCOL_ERMI;
    my_proc_info   = &(RFGW_ERMI_INFO);

    /* wait for the system to init */
    process_wait_on_system_init();

    if (!rfgw_ssp_proc_sys_init(video_protocol, my_proc_info)) {
        ERMI_C_RTSP_API_BUGINF("\nERMI: %%ERROR: Unable to Initialize "
                               " OS infra for %s VCP process.",
                               rfgw_video_protocol_name[video_protocol]);
    }

    /* Let the scheduler know what our interest is */
    process_watch_socket_event(SOCKET_READ_EVENT|SOCKET_WRITE_EVENT, 
                               ENABLE, RECURRING);

    if (my_proc_info->rsp_msgQ != NULL) {
        process_watch_queue(my_proc_info->rsp_msgQ, ENABLE, RECURRING);
    } else {
        ERMI_C_BUGINF("%%ERROR: Resp Msg Queue Not initialized");
    }
    process_watch_mgd_timer(&ermi_c_parent_timer, ENABLE);

    ERMI_TRACE;

    /* Forever Service loop */
    ermi_c_req_svc_run();

   /* Oops - svc_run() returned. Shouldn't happen */

    /* taking down all the clients since the server no longer active */
    ermi_c_stop_svr(local_ip, DEFAULT_RTSP_PORT);
    rfgw_sg_server_failure_notify(video_protocol);

    if (process_exists(my_proc_info->req_proc_id)) {
        process_kill(my_proc_info->req_proc_id);
        SSP_EVENTS("\nRFGW %s(REQ): Protocol Request Service [%d] terminated.",
                   rfgw_video_protocol_name[video_protocol],
                   my_proc_info->req_proc_id);
    }

    /* Do a graceful shutdown */
    my_proc_info->req_proc_id = NO_PROCESS;
}

/*
 * Another component (Config CLI) sends a message to master process to 
 * start/stop a component
 */
void ermi_c_notify_master (int message)
{
    if (!process_send_message(RFGW_ERMI_INFO.req_proc_id, 
                              RFGW_ERMI_INFO.req_proc_name, 
                             message, NULL, 0)) {
        ERMI_C_BUGINF("\nFailed to send message %d to master proc.", message);
    }
}

/*
 * Add the peer to the peer table when TCP based RTSP/VREP client 
 * is connected to a new peer or add a svr to the svr table, when a 
 * TCP based RTSP/VREP svr is inited
 */
ermi_c_peer_t *ermi_c_add_peer (rfgw_video_server_group *video_sg,
                                ipaddrtype server_addr,
                                ipaddrtype ipa, ushort port)
{
    ermi_c_peer_t *peer;
    ermi_protocol_info_t *ermi_proto_info;
 
    peer = malloc(sizeof(ermi_c_peer_t));
    if (peer == NULL) {
        errmsg(&msgsym(RSCERR, VCP), "Failed to create ERMI-C Peer");
        return (NULL);        
    }
    peer->addr = ipa;
    peer->port = port;

    /* Update ERMI-C peer entry pointer in ERMI SG Table */
    ermi_proto_info = ermi_c_get_ermi_proto_info(video_sg);
    if (ermi_proto_info) {
        /* Know each other */
        ermi_proto_info->ermi_c_info = peer;
        peer->erm_info = ermi_proto_info;
    }

    return (peer);
}

boolean rfgw_ermi_sg_add_server (rfgw_video_server_group *video_sg,
                                 ipaddrtype server_addr)
{
    ermi_c_peer_t  *peer;
    ermi_protocol_info_t *ermi_proto_info;

    if (!video_sg ||
        (video_sg->video_protocol != RFGW_VIDEO_PROTOCOL_ERMI)) {
        return (ERMI_RTSP_ERROR);
    }

    ermi_proto_info = ermi_c_get_ermi_proto_info(video_sg);
    if (!ermi_proto_info) {
        errmsg(&msgsym(SWERR, RTSP), "ERMI Protocol Struct = ",
               0, "not initialized");
        return (FALSE);
    }

    peer = ermi_proto_info->ermi_c_info;

    if (peer == NULL) {

        peer = ermi_c_add_peer(video_sg, server_addr, 0, DEFAULT_RTSP_PORT);
        if (!peer) {
            errmsg(&msgsym(SWERR, RTSP), "ermi_c_add_peer with listen port",
                   ermi_c_svr_conn.port, "failed");
            return (FALSE);
        }

        /* Update the ERMI-C info in the ERRP per server-group info */
        ermi_proto_info->ermi_c_info = peer;
    }

    return (TRUE);
}

boolean ermi_c_delete_ermi_conn_table (ermi_c_conn_t *conn)
{
    rtsp_buf_t *rtsp_sess_buf;

#ifdef NOT_NEEDED
    rtsp_session_t *ses;
    int k;
    rtsp_msg_t *rtsp_msg;

    for (k=0; k<RTSP_SESSION_TABLE_LEN; k++) {

        ses = conn->ses_tbl[k];
        if (ses) {
            /* For existing sessions, inform app, so that it can clean up */
            rtsp_msg = rtsp_new_msg();
            if (rtsp_msg == NULL) {
                errmsg(&msgsym(RSCERR, RTSP),
                            "rtsp_msg_chunk malloc failed");
                return;
            }
            rtsp_msg->msg_type = RTSP_CONN_ERROR;
            conn->app_msg_func(ses, rtsp_msg);
        }
    }
#endif // NOT_NEEDED

    /* free and destroy session index_list */
    while (!LIST_EMPTY(&conn->ses_index_list)) {
       rtsp_sess_buf = list_dequeue(&conn->ses_index_list);
       free(rtsp_sess_buf);
    }
    list_destroy(&conn->ses_index_list);

    return (TRUE);

}

#ifdef NK_NOT_USED
boolean ermi_c_del_peer (vcp_hash_table_t *table,
                         ipaddrtype ipa, ushort tcp_port)
{
    list_header *peer_list;
    list_element *runner;
    ermi_c_peer_t *found_peer;
    int        port;
    uint16     hash_value;

    port = tcp_port;
    hash_value = table->hash_func(table->len, table->type, &port);
    peer_list = &table->entry[hash_value];

    found_peer = NULL;
    /* search through list for a peer with this ipa and port */
    FOR_ALL_ELEMENTS_IN_LIST(peer_list, runner) {
        found_peer = (ermi_c_peer_t*) runner->data;
        if (found_peer->addr == ipa && found_peer->port == port) { 
            break;
        }
    }

    if (found_peer == NULL) {
        return (FALSE);
    } 

    ermi_c_delete_ermi_conn_table(found_peer->conn);

    list_remove(peer_list, NULL, found_peer);
    table->count--;
    chunk_free(ermi_c_peer_chunk, found_peer);
    return (TRUE);
}
#endif // NK_NOT_USED

/*  close the socket with well-known listening port 
    the connection can be both local and remote
    Notice that the arguments of this function are the same as the first
    three arguments of rtsp_init_server() 
    RETURN: TRUE if stopped, FALSE if not.
*/
boolean ermi_c_stop_svr (ipaddrtype local_ip, ushort listen_port)
{
    uint16 port;

    ermi_c_peer_t *peer; 
    // ermi_c_conn_t *accept_conn; /* all accepted conn */
    ermi_c_conn_t *conn; /* this connection should have no attached rtsp_conn */
    rtsp_conn_t *rtsp_conn;

    rfgw_video_server_group *video_sg;
    list_element *sg_elem;
 
    port = listen_port;

    /* Find amongst all active ERMI-Video Server group */
    FOR_ALL_DATA_IN_LIST(&rfgw_video_sg_list, sg_elem, video_sg) {

        if ((video_sg->video_protocol != RFGW_VIDEO_PROTOCOL_ERMI) ||
            (video_sg->state != RFGW_VIDEO_SERVER_GROUP_ACTIVE)) {
            continue;
        }

        /* find the remote connection ERM peer entry */

        peer = ermi_c_get_peer_erm_info(video_sg);
        conn = &(peer->conn);

        if (!conn || (conn->type != RTSP_SVR_CONN)) {
            continue;
        }

        /* close_rtsp conn, if applicable */
        rtsp_conn = (rtsp_conn_t *)conn->prot_info;
        assert(rtsp_conn != NULL); 
        rtsp_close_conn(rtsp_conn);
    
        /* close the socket and free ERMI-C conn */
        socket_close(conn->fd);
        memset(conn, 0, sizeof(ermi_c_conn_t));
    }    

    rtsps_app_func = NULL;

#if 0
    /* other apps could still be listening on this port.
     * So don't shut down this listen_port before verifying
     */
    chunk_free(ermi_c_peer_chunk, peer);

    /* close the socket and free ERMI-C conn */
    socket_close(ermi_c_svr_conn.fd);
    memset(&ermi_c_svr_conn, 0, sizeof(struct ermi_c_conn_));
#endif

    rfgw_rsp_service_stop(RFGW_VIDEO_PROTOCOL_ERMI);
    return (TRUE);
}


extern boolean rfgw_ermi_supported;

/* initialize the subsystem */
void ermi_c_master_init (subsystype* subsys)
{
    if (!rfgw_ermi_supported) {
        return;
    }

    ERMI_TRACE;

    /* initialize common global data structures,
     * such as connection table, timer table and
     * the watched queue
     */
    memset(timer_table, 0, LAST_TIMER_TYPE*sizeof(ermi_c_timer_t));

    mgd_timer_init_parent(&ermi_c_parent_timer, NULL);

    /* init local ip */
    local_ip = socket_inet_addr("2.37.33.20"); // NK_TBD: idb_get_ip_addrs(FEidb)

    /* RTSP service init */
    ermi_c_rtsp_init();
 
    /* CLI parser chain */
    ermi_video_parser_init();

    /* debug init */
    ermi_debug_init();

#ifdef NK_NOT_USED
    ermi_debug_ermi_video = TRUE;
    ermi_c_debug_ermi_c = TRUE;
    ermi_c_debug_rtsp_sock = TRUE;
    ermi_c_debug_rtsp_ses = TRUE;
    ermi_c_debug_rtsp_api = TRUE;
#endif // NK_NOT_USED

    ERMI_TRACE;
}

#define ERMI_C_MAJVERSION 1
#define ERMI_C_MINVERSION 0
#define ERMI_C_EDITVERSION 1

SUBSYS_HEADER(ermi_c, ERMI_C_MAJVERSION , ERMI_C_MINVERSION, ERMI_C_EDITVERSION,
              ermi_c_master_init, SUBSYS_CLASS_KERNEL, NULL);

