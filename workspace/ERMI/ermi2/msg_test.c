/**
*    Copyright (c) 2011 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file msg_test.c
*    @brief ERMI RTSP message test
*    @author Yi Tong Tse
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "util.h"
#include "os_util_lnx.h"
#include "os_util.h"
#include "rtsp.h"

#include "test_msg.h"


int main (int argc, char** argv)
{
    int rc = RTSP_RESP_OK;
    int i;
    int len;
    char req_msg[1024];
    char rsp_msg[1024];
    char *msg;
    rtsp_msg_t req;
    rtsp_msg_t rsp;
    rtsp_conn_t conn;
    int video_cnt, docsis_cnt, qam_cnt, ucast_cnt, mcast_cnt, mcast_rank;

    memset(&req, 0, sizeof(req));
    memset(&rsp, 0, sizeof(rsp));
    memset(&conn, 0, sizeof(conn));

    conn.ses_id = 1234;

    // Process request
    strcpy(req_msg, depi_setup_msg);
    msg = req_msg;
//    print_hex(req_msg, strlen(req_msg));

    rtsp_get_request(&msg, &req, &rc);
    if (rc == RTSP_RESP_OK) {
        int i;
        rc = rtsp_check_request_setup(&req, &conn);
        for (i=0; i<req.transport_cnt; i++) {
//            rtsp_parse_transport_spec(&req.transport[i].spec);
        }
    }

    printf("\n");
    rtsp_msg_print(&req);

    // Generate response
    rsp.rc = rc;

    rsp.cseq = req.cseq;
    rsp.hdr_mask |= RTSP_HDR_MASK_CSEQ;

    snprintf(rsp.ses_id, RTSP_MAX_TOKEN_LEN, "%08x", conn.ses_id++);
    rsp.hdr_mask |= RTSP_HDR_MASK_SESSION;

    // Try to pick multicast source with lowest rank
    video_cnt = 0;
    docsis_cnt = 0;
    qam_cnt = 0;
    ucast_cnt = 0;
    mcast_cnt = 0;
    mcast_rank = 10000;
    for (i=0; i<req.transport_cnt; i++) {
        rtsp_trans_t *trans = &req.transport[i];

        switch (trans->profile_idx) {

        case RTSP_TRANS_PROFILE_IDX_DOCSIS_QAM:
            docsis_cnt++;
            if (qam_cnt++ == 0) {
                // Just pick up the first QAM channel
                rsp.transport[0] = *trans;
            }
            break;

        case RTSP_TRANS_PROFILE_IDX_MP2T_QAM:
            rsp.transport[0] = *trans;  // always use 1st spec for QAM
            video_cnt++;
            qam_cnt++;
            break;

        case RTSP_TRANS_PROFILE_IDX_MP2T_UDP:
            if ((trans->par_mask & RTSP_TRANS_PAR_MASK_UNICAST)) {
                rsp.transport[1] = *trans;  // always use 2nd spec for UDP
                ucast_cnt++;

            } else if ((trans->par_mask & RTSP_TRANS_PAR_MASK_MULTICAST)) {
                if (trans->rank < mcast_rank) {
                    rsp.transport[1] = *trans;  // always use 2nd spec for UDP
                    mcast_rank = trans->rank;
                }
                mcast_cnt++;
            }
            video_cnt++;
            break;
        }
    }

    if (video_cnt * docsis_cnt != 0) {
        printf("Error: Mixed video and docsis transport spec\n");
        return -1;
    }

    if (docsis_cnt) {
        rtsp_trans_t* trans = &rsp.transport[0];
        rsp.transport_cnt = 1;
        trans->par_mask &= ~RTSP_TRANS_PAR_MASK_QAM_TSID;
        trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_NAME;
        strlcpy(trans->service_group, "SanJose.Hub", RTSP_MAX_TOKEN_LEN);
    }

    if (video_cnt) {
        if (qam_cnt != 1 || ucast_cnt > 1 || ucast_cnt * mcast_cnt != 0) {
            printf("Bad video transport spec combination\n");
            printf("  qam %d, ucast %d, mcast %d\n",
                   qam_cnt, ucast_cnt, mcast_cnt);
            return -1;
        }
        rsp.transport_cnt = 2;

        rsp.clab_cln_id = req.clab_cln_id;
        rsp.clab_ses_id = req.clab_ses_id;
        rsp.hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    }

    rsp.hdr_mask |= RTSP_HDR_MASK_TRANSPORT;

    msg = rsp_msg;
    len = rtsp_put_response(&msg, &rsp);
    printf("\nResponse (%d bytes):\n%s\n", len, rsp_msg);

//    print_hex(rsp_msg, strlen(rsp_msg));

    return 0;
}



