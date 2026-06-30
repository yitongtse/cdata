/*-------------------------------------------------------------------
 * test_ermi_c_rtsp.c - ERMI Video ERMI-2 control plane test commands
 *
 * 02/09/04, Linda Hua
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */
#include <ttysrv.h>
#include COMP_INC(kernel/memory, chunk.h)
#include <assert.h>
#include IOS_INC(h/interface_private.h)

#include "ermi_c_global.h"
#include "ermi_video_main.h"
#include "parser_defs_ermi_c.h"
#include "ermi_c_global.h"
#include "ermi_video_hash.h"
#include "ermi_c_master.h"
#include "ermi_c_rtsp_api.h"

#include INTERNAL_INC(sup/src/vclient/session_main.h)

/* who is our remote server */
static ipaddrtype server_ip;
/* by default, we test the rtsp connection between 
   SM<-->ERM and ERM<-->SS */
static ushort tcp_port = DEFAULT_RTSP_PORT; 

/* the server prints the msg when it receives one */
static void test_handle_rtsp_server_msg (rtsp_session_t *ses, void *prot_msg)
{
    ermi_c_conn_t *conn;
    rtsp_msg_t *rtsp_msg = (rtsp_msg_t *)prot_msg;
    rtsp_msg_t *new_msg = rtsp_new_msg();

    buginf("\n RTSP server session %08X received a %s from %i",
           ses->session_id, 
           ermi_c_rtsp_method_str(ERMI_C_GET_METHOD(rtsp_msg->msg_type)), 
           ses->peer_erm->conn.dest_addr);
    if (rtsp_msg->msg_type == RTSP_SETUP) {
        /* for test only */
        new_msg->sessionID = ses->session_id;
	new_msg->valid = 0;
        RPM_SETBIT(new_msg->valid, RP_BIT_SESSIONID);
        new_msg->resp_retcode = 200; 
        RPM_SETBIT(new_msg->valid, RP_BIT_RESP_RETCODE);
        new_msg->msg_type = RTSP_RESPONSE;
        rtsp_send_response(ses, new_msg); 
    } else if (rtsp_msg->msg_type == RTSP_TEARDOWN) {
        new_msg->msg_type = RTSP_RESPONSE;
        new_msg->resp_retcode = 200; 
        RPM_SETBIT(new_msg->valid, RP_BIT_RESP_RETCODE);
        rtsp_send_response(ses, new_msg); 
        rtsp_server_close_session(ses);
    } else if (rtsp_msg->msg_type == RTSP_SETPARAMETER) {
        new_msg->sessionID = ses->session_id;
        new_msg->msg_type = RTSP_RESPONSE;
	new_msg->valid = 0;
        RPM_SETBIT(new_msg->valid, RP_BIT_SESSIONID);
        rtsp_send_response(ses, new_msg); 
    } else if (rtsp_msg->msg_type == RTSP_CONN_ERROR) {
        conn = &(ses->peer_erm->conn);
        /* it's a sever's conn, close the rtsp conn and conn */
        rtsp_server_close_session(ses);
        rtsp_free_msg(new_msg);
    } /* other wise, it is an response */
    rtsp_free_msg(rtsp_msg);
}


void test_ermi_c_rtsp (uint32 message, cli_test_msg_t *cli_test_msg)
{
    ermi_status status;
    ermi_c_conn_t *conn;
    rtsp_msg_t *rtsp_msg;
    rtsp_session_t *rtsp_ses;
    // rtsp_conn_t *svr_conn; 
    ermi_c_peer_t *peer;
    ipaddrtype addr;
    ushort port;
    int int_port;
    int index;
    int burst_size;

    addr = cli_test_msg->server_ip;
    port = cli_test_msg->port;
    index = cli_test_msg->ses_index; // <--- NK_TBD
    burst_size = cli_test_msg->burst_size;

    switch(message) {
    case ERMI_C_RTSP_SERVER_INIT:
	 status = rtsp_init_server(0, port, (void*)test_handle_rtsp_server_msg);
         if (status != ERMI_OK) {
            buginf("\nrtsp_server_init failed with error code . %d", 
                    status);
	 }

         break;

    case ERMI_C_RTSP_SERVER_SETPARAM:
        /* find the remote connection entry */
        int_port = port;
        peer = ermi_c_get_peer_erm_info(NULL);
        if (peer == NULL) {
            buginf("ermi_c connection to port %d doesn't exist.\n", port);
            return;
	}
        conn = &peer->conn;
        if (conn == NULL) {
            buginf("ermi_c connection to port %d doesn't exist.\n", port);
            return;
        }
 
        /* find one accepted connections */
        peer = ermi_c_get_peer_erm_info(NULL);
        conn = &(peer->conn);

        if (!conn || (conn->type != RTSP_SVR_CONN)) {
            break;
        }
        rtsp_ses = conn->ses_tbl[index];
        if(rtsp_ses == NULL) {
	    buginf("\nNo such session with index %d.", index);
            break;
	}
        rtsp_msg = rtsp_new_msg();
        if (rtsp_msg == NULL) {
            break;
	}
        /* fill in content */
	rtsp_msg->num_trans_hdrs = 1;
	rtsp_msg->transhdr[0].type = MPEG_QAM;
        rtsp_msg->transhdr[0].bit_rate = 3000000;
        if (message == ERMI_C_RTSP_CLIENT_TEARDOWN)
            rtsp_msg->msg_type = RTSP_TEARDOWN;
        else
            rtsp_msg->msg_type = RTSP_SETPARAMETER;
        status = rtsp_send_request(rtsp_ses, rtsp_msg);
        if (status != ERMI_OK) {
            buginf("\nsend setparam failed."); 
	}
        break;

    case ERMI_C_RTSP_SERVER_STOP:
	if (FALSE == ermi_c_stop_svr(0/*ipa*/, port)) {
            buginf("\nfailed to stop rtsp server.");
	} 
        break;

    case ERMI_C_TEST_RTSP_NO:
        test_rtsp = 0;
        break;

    default:  
        break;
    }
}

/* test_erm_command implement all rtsp test CLI */
void ermi_c_test_rtsp_cmds (parseinfo *csb)
{
    cli_test_msg_t *cli_test_msg;
    int **msg;

    if (RFGW_ERMI_INFO.req_proc_id == NO_PROCESS) {
        buginf("\nermi_c master process has not started yet.");
        return;
    }

    msg = (int **)malloc(sizeof(int *));
    cli_test_msg = malloc(sizeof(cli_test_msg_t));
    *msg = (int *)cli_test_msg;

    switch(csb->which) {
    case ERMI_C_RTSP_PORT:
         /* by default, we test the rtsp connection between 
            SM<-->ERM and ERM<-->SS using TCP_PORT_RTSP_VOD. 
            to test connection between ERM<-->EDA choose TCP_PORT_EDCP_EDA ??
            to test connection between ERM<-->EEA choose TCP_PORT_EDCP_EEA */
	 tcp_port = (ushort)GETOBJ(int, 1);
         break;

    case ERMI_C_RTSP_MEM:
         /* show all RTSP chunk utilization
	  */ 
	printf("rtsp_conn_chunk in use %d\n",
                                rtsp_conn_chunk->total_inuse);
	printf("rtsp_session_chunk in use %d\n",
                                rtsp_session_chunk->total_inuse);
	printf("rtsp_msg_chunk in use %d\n",
                                rtsp_msg_chunk->total_inuse);

         break;

    case ERMI_C_RTSP_SERVER_INIT:
         cli_test_msg->server_ip = 0;
         cli_test_msg->port = tcp_port;
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
                              csb->which, msg, 1); 
         break;

    case ERMI_C_RTSP_SERVER_STOP:
         cli_test_msg->server_ip = server_ip;
         cli_test_msg->port = tcp_port;
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
                              csb->which, msg, 1); 
         break;

    case ERMI_C_RTSP_CLIENT_CONNECT:
	 server_ip = GETOBJ(paddr,1)->ip_addr;
         cli_test_msg->server_ip = server_ip;
         cli_test_msg->port = tcp_port;
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
	                      csb->which, msg, 1); 
        break;

    case ERMI_C_RTSP_CLIENT_SETUP:
         cli_test_msg->server_ip = server_ip;
         cli_test_msg->port = tcp_port;
         cli_test_msg->ses_index = GETOBJ(int,1);
         cli_test_msg->burst_size = GETOBJ(int, 2);
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
	                      csb->which, msg, 1); 
         break;

    case ERMI_C_RTSP_CLIENT_TEARDOWN:
         cli_test_msg->server_ip = server_ip;
         cli_test_msg->port = tcp_port;
         cli_test_msg->ses_index = GETOBJ(int, 1);
         cli_test_msg->burst_size = GETOBJ(int, 2);
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
	                      csb->which, msg, 1); 
         break;

    case ERMI_C_RTSP_CLIENT_SETPARAM:
    case ERMI_C_RTSP_SERVER_SETPARAM:
         cli_test_msg->server_ip = server_ip;
         cli_test_msg->port = tcp_port;
         cli_test_msg->ses_index = GETOBJ(int, 1);

         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
	                      csb->which, msg, 1); 
         break;

    case ERMI_C_RTSP_CLIENT_STOP:
         cli_test_msg->server_ip = 0;
         cli_test_msg->port = tcp_port;
         process_send_message(RFGW_ERMI_INFO.req_proc_id,
                              RFGW_ERMI_INFO.req_proc_name, 
	                      csb->which, msg, 1); 
         break;


    case ERMI_C_TEST_RTSP_NO:
        test_rtsp = 0;

    default:
        break;
    }
}


