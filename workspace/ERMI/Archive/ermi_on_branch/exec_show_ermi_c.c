/*
 *-------------------------------------------------------------------
 * exec_show_rtsp.c - rtsp show command 
 *
 * 03/27/03, Xiaomei Liu
 *
 * Copyright (c) 2004-2009 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 */

#include <ttysrv.h>
#include <../os/list_private.h>
#include <assert.h>
#include <../os/time.h>
#include "parser_defs_ermi_c.h"

#include "ermi_video_hash.h"
#include "ermi_c_master.h"

#include "ermi_c_global.h"
#include "ermi_c_rtsp_api.h"

/* display all session info */
static void show_rtsp_sessions (ermi_c_conn_t *conn, int num_ses)
{
    int k;
    rtsp_session_t *ses;

    if (num_ses == 0) {
        return;
    }
   
    printf(" Index\tSessionId\tState\tGroup\tClientSession\n");
    printf(" -----\t---------\t-----\t-----\t-------------\n");

    /* Show all ERMI-C / RTSP sessions */
    for (k=0; k<RTSP_SESSION_TABLE_LEN;k++) {
        ses = conn->ses_tbl[k];
        if (ses) {
            printf(" %d\t%08X\t%d\t%s\t%s\n",
		   ses->index, ses->session_id, ses->state,
                   ses->session_group, ses->clientsession_id);
	}
    }
}

static void show_rtsp_server_msg_stats (ermi_c_conn_t *conn)
{
    prot_msg_event_e msg_type;

    if (conn == NULL) {
        return;
    }
   
    printf("\nMsgType          Rcvd       RcvdFail   RcvdSucc"
           "   Sent       SentFail   SentSucc"
           "\n-----------------------------------------------"
           "---------------------------------\n");

    /* Show all ERMI-C / RTSP Message Stats */
    for (msg_type=METHOD_SETUP; msg_type < MSG_TYPE_LAST; msg_type++) {
        printf("%16s %10d %10d %10d %10d %10d %d\n",
               ermi_c_rtsp_method_str(msg_type),
               ERMI_STATS_TABLE[msg_type].recv_req,
               ERMI_STATS_TABLE[msg_type].fail_req,
               ERMI_STATS_TABLE[msg_type].succ_req,
               ERMI_STATS_TABLE[msg_type].send_rsp,
               ERMI_STATS_TABLE[msg_type].fail_rsp,
               ERMI_STATS_TABLE[msg_type].succ_rsp);
    }

    printf("\n-----------------------------------------------"
           "---------------------------------\n");
}

/* erm show command implementation */
void rtsp_show_command (parseinfo *csb)
{
    uint16 port, num_ses;
    ermi_c_peer_t *peer;
    ermi_c_conn_t *conn;
    rtsp_conn_t *rtsp_conn;
    rfgw_video_server_group *video_sg;
    list_element *sg_elem;

    num_ses = 0;


    ERMI_TRACE;

    /* Show all connections with all active ERMI-Video Server group */
    FOR_ALL_DATA_IN_LIST(&rfgw_video_sg_list, sg_elem, video_sg) {

        if ((video_sg->video_protocol != RFGW_VIDEO_PROTOCOL_ERMI) ||
            (video_sg->state != RFGW_VIDEO_SERVER_GROUP_ACTIVE)) {
            continue;
        }

        peer = ermi_c_get_peer_erm_info(video_sg);
        conn = &(peer->conn);

        if ((ERMI_C_GET_PROT(conn->type) != PROT_RTSP) ||
            (conn->type != RTSP_SVR_CONN)) {
            continue;
        }

        rtsp_conn = (rtsp_conn_t *)conn->prot_info;
        assert(rtsp_conn != NULL);

        switch(csb->which) {

        case RTSP_SHOW_SERVER_MSG_STATS:
            show_rtsp_server_msg_stats(conn);
            break;

        case RTSP_SHOW_SERVER:
            port = GETOBJ(int, 1);

            num_ses = (RTSP_SESSION_TABLE_LEN - conn->ses_index_list.count);
            printf("connection fd=%d from %i:%d has %d sessions.\n",
                   conn->fd, conn->dest_addr, conn->port, num_ses);
            show_rtsp_sessions(conn, num_ses);
            break;

        case RTSP_SHOW_CLIENT:
            break;

        default:
            break;
        }
    }
}
