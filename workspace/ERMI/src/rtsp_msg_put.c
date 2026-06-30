/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: rtsp_msg_put.c
*    @brief RTSP message generation routines
*    @author Yi Tong Tse
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "util.h"
#include "rtsp.h"


#define RTSP_PUT_HEADER(buf, header, fmt, value...)     \
    put_format(buf, "%s:"fmt"\r\n", header, ##value);


int rtsp_put_transport_spec (char **buf, rtsp_trans_t *spec)
{
    char ip_txt[INET_ADDRSTRLEN];
    int len = put_format(buf, "%s", rtsp_trans_profiles[spec->profile_idx]);

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_UNICAST)) {
        len += put_format(buf, ";%s", RTSP_TRANS_PAR_UNICAST);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_MULTICAST)) {
        len += put_format(buf, ";%s", RTSP_TRANS_PAR_MULTICAST);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_BITRATE)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_BITRATE, spec->bitrate);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_DEPI_MODE)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_DEPI_MODE,
                          rtsp_trans_depi_modes[spec->depi_mode_idx]);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_SRC)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_SRC,
                          inet_ntop(AF_INET, &spec->src_ip, ip_txt,
                                    INET_ADDRSTRLEN));
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_SRC_PORT)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_SRC_PORT,
                          spec->src_udp);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_DST)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_DST,
                          inet_ntop(AF_INET, &spec->dst_ip, ip_txt,
                                    INET_ADDRSTRLEN));
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_DST_PORT)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_DST_PORT,
                          spec->dst_udp);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_MULTICAST_ADDR)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_MULTICAST_ADDR,
                          inet_ntop(AF_INET, &spec->mcast_ip, ip_txt,
                                    INET_ADDRSTRLEN));
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_RANK)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_RANK, spec->rank);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_MPTS_PROG)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_MPTS_PROG,
                          spec->mpts_prog);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_QAM_TSID)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_QAM_TSID, spec->tsid);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_FIBER_NODE)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_FIBER_NODE,
                          spec->fiber_nodes);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_FREQ_RANGE)) {
        len += put_format(buf, ";%s=%d-%d", RTSP_TRANS_PAR_FREQ_RANGE,
                          spec->freq_low, spec->freq_high);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_QAM_NAME)) {
        len += put_format(buf, ";%s=%s.%d", RTSP_TRANS_PAR_QAM_NAME,
                          spec->service_group, spec->tsid);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_QAM_DST)) {
        len += put_format(buf, ";%s=%d.%d", RTSP_TRANS_PAR_QAM_DST,
                          spec->freq, spec->prog_num);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_MODULATION)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_MODULATION,
                          spec->modulation);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_J83_ANNEX)) {
        len += put_format(buf, ";%s=%s", RTSP_TRANS_PAR_J83_ANNEX,
                          rtsp_trans_j83_annexes[spec->j83_annex_idx]);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_TAPS)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_TAPS, spec->taps);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_CHANNEL_WIDTH)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_CHANNEL_WIDTH,
                          spec->channel_width);
    }

    if ((spec->par_mask & RTSP_TRANS_PAR_MASK_SYMBOL_RATE)) {
        len += put_format(buf, ";%s=%d", RTSP_TRANS_PAR_SYMBOL_RATE,
                          spec->symbol_rate);
    }

    return len;
}


int rtsp_put_transport_header (char **buf, rtsp_trans_t *trans)
{
    int i;
    boolean first_spec = TRUE;

    int len = put_format(buf, "%s:", RTSP_HDR_TRANSPORT);
    for (i=0; i<RTSP_MAX_TRANSPORTS; i++, trans++) {
        if ((trans->par_mask)) {
            if (!first_spec)  len += put_byte(buf, ',');
            len += rtsp_put_transport_spec(buf, trans);
            first_spec = FALSE;
        }
    }
    len += put_format(buf, "\r\n");
    return len;
}


int rtsp_put_headers (char **buf, rtsp_msg_t *req)
{
    int len = 0;

    if ((req->hdr_mask & RTSP_HDR_MASK_CSEQ)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CSEQ, "%d", req->cseq);
    }
    
    if ((req->hdr_mask & RTSP_HDR_MASK_SESSION)) {
        len += put_format(buf, "%s:%s", RTSP_HDR_SESSION, req->ses_id);
        if (req->timeout > 0) {
            len += put_format(buf, ";timeout=%d", req->timeout);
        }
        len += put_format(buf, "\r\n");
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_REQUIRE)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_REQUIRE, "%s", RTSP_ERMI_REQUIRE);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_TRANSPORT)) {
        len += rtsp_put_transport_header(buf, req->transport);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_TYPE)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CONTENT_TYPE,
                               "%s", rtsp_content_types[req->content_type_idx]);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_LENGTH)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CONTENT_LENGTH,
               "%d", req->content_length);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_CLIENT_SESSION_ID,
                   "%012llx%08x", (long long)req->clab_cln_id,
                   req->clab_ses_id);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_NOTICE)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_NOTICE,
                   "%d \"%s\" event-date=%s npt",
                   req->notice_code, rtsp_clab_notice_txt(req->notice_code),
                   req->date_time);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_REASON)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_REASON, "%d %s",
                       req->reason_code, rtsp_reason_txt(req->reason_code));
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_SESSION_GROUP)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_SESSION_GROUP,
                               "%s", req->session_group);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_PRIORITY)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_PRIORITY,
                               "%d", req->priority);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_SETUP_TYPE)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_SETUP_TYPE,
                               "%d", req->setup_type);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_PID_REMAP)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_PID_REMAP,
                               "%d", req->pid_remap);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_MPTS_MODE)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_MPTS_MODE,
                               "%s", rtsp_clab_mpts_modes[req->mpts_mode_idx]);
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_STATMUX_GROUP)) {
        len += RTSP_PUT_HEADER(buf, RTSP_HDR_CLAB_STATMUX_GROUP,
                   "group_id=%s group_bitrate=%d",
                   req->statmux_group_id, req->statmux_group_bitrate);
    }

    len += put_format(buf, "\r\n");
    return len;
}


int rtsp_put_request (char **buf, rtsp_msg_t *req, char *method, char *uri)
{
    int len = put_format(buf, "%s %s %s\r\n", method, uri, RTSP_VERSION);

    len += rtsp_put_headers(buf, req);

    if (req->content_length > 0) {
        len += put_format(buf, "%s", req->msg_body);
    }

    return len;
}


int rtsp_put_response (char **buf, rtsp_msg_t *rsp)
{
    int len = put_format(buf, "%s %d %s\r\n",
                         RTSP_VERSION, rsp->rc, rtsp_resp_txt(rsp->rc));

    len += rtsp_put_headers(buf, rsp);

    if (rsp->content_length > 0) {
        len += put_format(buf, "%s", rsp->msg_body);
    }

    return len;
}

