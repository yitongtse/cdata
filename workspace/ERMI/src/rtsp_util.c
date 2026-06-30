/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: rtsp_util.c
*    @brief ERMI RTSP utilities
*    @author Yi Tong Tse
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "rtsp.h"


const char *rtsp_content_types[] = {
    RTSP_CONTENT_TYPE_TEXT_PARAM,
    RTSP_CONTENT_TYPE_TEXT_XML,
    ""
};


const char* rtsp_trans_profiles[] =
{
    RTSP_TRANS_PROFILE_DOCSIS_QAM,
    RTSP_TRANS_PROFILE_MP2T_QAM,
    RTSP_TRANS_PROFILE_MP2T_UDP,
    ""
};

const char *rtsp_trans_depi_modes[] = {
    RTSP_TRANS_DEPI_MODE_MPT,
    RTSP_TRANS_DEPI_MODE_PSP,
    ""
};

const char *rtsp_trans_j83_annexes[] = {
    RTSP_TRANS_J83_ANNEX_A,
    RTSP_TRANS_J83_ANNEX_B,
    RTSP_TRANS_J83_ANNEX_C,
    ""
};


const char *rtsp_clab_mpts_modes[] = {
    RTSP_CLAB_MPTS_MODE_PASSTHROUGH,
    RTSP_CLAB_MPTS_MODE_MULTIPLEX,
    ""
};


char *rtsp_resp_txt (uint32 resp_code)
{
    switch (resp_code) {
    case RTSP_RESP_OK:
        return "OK";
    case RTSP_RESP_BAD_REQUEST:
        return "Bad Request";
    case RTSP_RESP_FORBIDDEN:
        return "Forbidden";
    case RTSP_RESP_NOT_FOUND: 
        return "Not Found";
    case RTSP_RESP_METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case RTSP_RESP_REQUEST_TIMEOUT:
        return "Request Timeout";
    case RTSP_RESP_PRECONDITION_FAILED:
        return "Precondition Failed";
    case RTSP_RESP_PARAMETER_NOT_UNDERSTOOD:
        return "Parameter Not Understood";
    case RTSP_RESP_NOT_ENOUGH_BANDWIDTH:
        return "Not Enough Bandwidth";
    case RTSP_RESP_SESSION_NOT_FOUND:
        return "Session Not Found";
    case RTSP_RESP_HEADER_FIELD_NOT_VALID_FOR_RESOURCE:
        return "Header Field Not Valid for Resource";
    case RTSP_RESP_INVALID_RANGE:
        return "Invalid Range";
    case RTSP_RESP_UNSUPPORTED_TRANSPORT:
        return "Unsupported Transport";
    case RTSP_RESP_DESTINATION_UNREACHABLE:
        return "Destination Unreachable";
    case RTSP_RESP_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    case RTSP_RESP_NOT_IMPLEMENTED:
        return "Not Implemented";
    case RTSP_RESP_SERVICE_UNAVAILABLE:
        return "Service Unavailable";
    case RTSP_RESP_VERSION_NOT_SUPPORTED:
        return "RTSP Version Not Supported";
    case RTSP_RESP_OPTION_NOT_SUPPORTED:
        return "Option Not Supported";
    }
    return "UNKNOWN";
}


char *rtsp_reason_txt (uint32 reason_code)
{
    switch (reason_code) {
    case RTSP_REASON_USER_STOPPED:
        return "User stopped";
    case RTSP_REASON_NO_USER_ACTIVITY:
        return "No user activity";
    case RTSP_REASON_SETTOP_CAP_MISMATCH:
        return "Set-top capability mismatch";
    case RTSP_REASON_INSUFFICIENT_PRIORITY:
        return "Insufficient priority";
    case RTSP_REASON_NETWORK_DELIVERY_FAILURE:
        return "Network delivery failure";
    case RTSP_REASON_FAIL_TO_TUNE:
        return "Fail to tune";
    case RTSP_REASON_LOSS_OF_TUNE:
        return "Loss of tune";
    case RTSP_REASON_RTSP_FAILURE:
        return "RTSP failure";
    case RTSP_REASON_CHANNEL_FAILURE:
        return "Channel failure";
    case RTSP_REASON_NO_RTSP_SERVER:
        return "No RTSP server";
    case RTSP_REASON_UNKNOWN:
        return "Unknown";
    case RTSP_REASON_NETWORK_RESOURCE_FAILURE:
        return "Network Resource Failure";
    case RTSP_REASON_SETTOP_HEARTBEAT_TIMEOUT:
        return "Settop Heartbeat Timeout";
    case RTSP_REASON_SETTOP_INACTIVITY_TIMEOUT:
        return "Settop Inactivity Timeout";
    case RTSP_REASON_CONTENT_UNAVAILBLE:
        return "Content Unavailable";
    case RTSP_REASON_STREAMING_FAILURE:
        return "Streaming Failure";
    case RTSP_REASON_QAM_FAILURE:
        return "QAM Failure";
    case RTSP_REASON_VOLUME_FAILURE:
        return "Volume Failure";
    case RTSP_REASON_STREAM_CONTROL_ERROR:
        return "Stream Control Error";
    case RTSP_REASON_STREAM_CONTROL_TIMEOUT:
        return "Stream Control Timeout";
    case RTSP_REASON_SESSION_LIST_MISMATCH:
        return "Session List Mismatch";
    case RTSP_REASON_QAM_PARAMETER_MISMATCH:
        return "QAM parameter mismatch";
    case RTSP_REASON_SESSION_TIMEOUT:
        return "Session timeout";
    }
    return "UNKNOWN";
}


char *rtsp_clab_notice_txt (uint32 notice_code)
{
    switch (notice_code) {
    case RTSP_CLAB_NOTICE_DELIVERY_SUCCEED:
        return "Delivery succeeded (start of stream reached)";
    case RTSP_CLAB_NOTICE_PID_CONFLICT:
        return "Error Reading Content Data - PID Conflict";
    case RTSP_CLAB_NOTICE_INPUT_TS_INVALID:
        return "Input TS invalid";
    case RTSP_CLAB_NOTICE_PROGRAM_NUMBER_CONFLICT:
        return "Program number conflict";
    case RTSP_CLAB_NOTICE_DOWNSTREAM_FAILURE:
        return "Downstream Failure";
    case RTSP_CLAB_NOTICE_SERVER_RESOURCE_UNAVIL:
        return "Server Resource Unavailable";
    case RTSP_CLAB_NOTICE_UNABLE_TO_JOIN:
        return "Unable to Join";
    case RTSP_CLAB_NOTICE_INPUT_INTERFACE_FAILURE:
        return "Input Interface Failure";
    case RTSP_CLAB_NOTICE_REDUNDANT_SOURCE_FAILOVER:
        return "Failover to Redundant Source";
    case RTSP_CLAB_NOTICE_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    case RTSP_CLAB_NOTICE_BANDWIDTH_EXCEEDED_LIMIT:
        return "Bandwidth Exceeded Limit";
    case RTSP_CLAB_NOTICE_SESSION_IN_PROGRESS:
        return "Session in Progress";
    case RTSP_CLAB_NOTICE_RECLAIM_SESSION:
        return "Reclaim Session";
    }
    return "UNKNOWN";
}


void rtsp_msg_print (rtsp_msg_t *req)
{
    int i;
    if ((req->hdr_mask & RTSP_HDR_MASK_CSEQ)) {
        printf("CSeq: %d\n", req->cseq);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_SESSION)) {
        printf("Session: %s\n", req->ses_id);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_REQUIRE)) {
        printf("Require: %s\n", req->require);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_TRANSPORT)) {
        printf("Transport: %d specs\n", req->transport_cnt);
        for (i=0; i<req->transport_cnt; i++) {
            printf("  %s\n", req->transport[i].spec);
        }
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_TYPE)) {
        printf("Content-Type: %s\n", rtsp_content_types[req->content_type_idx]);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_LENGTH)) {
        printf("Content-Length: %d\n", req->content_length);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_LENGTH)) {
        printf("clab-ClientSessionId: %012lx, %08x\n",
               req->clab_cln_id, req->clab_ses_id);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_NOTICE)) {
        printf("clab-Notice: %d %s\n", req->notice_code, req->notice_desc);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_REASON)) {
        printf("clab-Reason: %d %s\n", req->reason_code, req->reason_phrase);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_SESSION_GROUP)) {
        printf("clab-SessionGroup: %s\n", req->session_group);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_PRIORITY)) {
        printf("clab-Priority: %d\n", req->priority);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_SETUP_TYPE)) {
        printf("clab-SetupType: %d\n", req->setup_type);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_PID_REMAP)) {
        printf("clab-PidRemap: %d\n", req->pid_remap);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_MPTS_MODE)) {
        printf("clab-MPTSMode: %s\n", rtsp_clab_mpts_modes[req->mpts_mode_idx]);
    }
    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_STATMUX_GROUP)) {
        printf("clab-StatmuxGroup: %s %d\n",
               req->statmux_group_id, req->statmux_group_bitrate);
    }
}


void rtsp_conn_init (rtsp_conn_t *conn)
{
//    memset(conn, 0, sizeof(*conn));
    conn->in_msg_buf = malloc(RTSP_MAX_MSG_BYTES);
}


/// This routine gets a complete message from the RTSP peer node
///
/// @param conn         RTSP connection
/// @param msg_len      pointer to the length of the received message (output)
///
/// @note
///   - blocks until the next message arrives completely
///   - uses the in_msg_buf pointed to by conn as a working area
///
/// @return
///     - EOK: operation successful
///     - EOF: connection closed
///     - some other values TBD
///
int rtsp_get_peer_msg (rtsp_conn_t *conn, uint16 *msg_len)
{
    int read_len;

wait:
    read_len = io_recv(conn->fd, conn->in_msg_buf, RTSP_MAX_MSG_BYTES-1);

    if (read_len == 0) {
        printf("Peer disconnected\n");
        return EOF;
    }

    if (read_len < 0) {
        errno = -read_len;
        if (errno == EINTR) {
            goto wait;
        }
        return errno;
    }

    *msg_len = read_len;

    // NUL-terminate the message
    conn->in_msg_buf[read_len] = 0;

    return EOK;
}


