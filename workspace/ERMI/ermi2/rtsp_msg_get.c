/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: rtsp_msg_get.c
*    @brief RTSP message parsing routines
*    @author Yi Tong Tse
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "util.h"
#include "rtsp.h"


/*
  All rtsp_get_xxx routines use the following conventions:
    - The first argument is the message buffer read pointer
      which will be updated under normal condition.
    - The last argument, rc, is the return code.  RTSP_RESP_OK means no error.
    - If an error occurs, rc should be updated.
    - If rc is already set to an error condition, the read routine should
      bypass all its operation. 
*/


// Skip all white spaces (SPACE or TAB)
//   *ptr will point to the first non-white space character
//
void skip_ws (char **ptr)
{
    char c;
    do {
        c = *(*ptr)++;
    } while (c == SPACE || c == TAB);
    (*ptr)--;
}


char get_char (char **ptr, int *rc)
{
    char c = 0;

    if (*rc == RTSP_RESP_OK) {
        if ((c = **ptr)) {
            (*ptr)++;
        }
    }
    return c;
}


/// Get a token
/// @note  The first input character matching one of the delimiter
///        will be replaced with NUL.
/// @param ptr          pointer to input buffer, which will be updated to point
///                     to the next character after the first found delimiter
/// @param delim        delimiter characters
/// @param token        output NUL terminated token
/// @param rc           response code
void rtsp_get_token (char **ptr, const char *delim, char **token, int *rc)
{
    if (*rc != RTSP_RESP_OK)  return;
    *token = strsep(ptr, delim);
}


/// Get a line
/// @param ptr          pointer to input buffer, which will be updted to point
///                     to right after the line.
/// @param line         output NUL terminated line
/// @param rc           response code
/// @note  A line is termined by CRLF
/// @note  If CRLF is not found before '\0' is reached, rc will be set.
void rtsp_get_line (char **ptr, char **line, int *rc)
{
    char *p;
    if (*rc != RTSP_RESP_OK)  return;

    *line = *ptr;
    p = strstr(*ptr, CRLF);
    if (p == NULL) {
        RTSP_LOG("CRLF not found");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }
    *p = *(p+1) = 0;
    *ptr = p + 2;
}


// Get decimal number
// - will stop at the first char not in '0'..'9'
// - returns the number of digits consumed
// - value of the number is returned through num
//
int rtsp_get_decimal (char **ptr, uint32 *num, int *rc)
{
    char c;
    int len = 0;

    if (*rc != RTSP_RESP_OK)  return 0;

    *num = 0;
    while (isdigit(c = **ptr)) {
        *num = *num * 10 + (int)(c - '0');
        (*ptr)++;
        len++;
    }
    return len;
}


// Get hexadecimal number
// - will stop at the first char not in '0'..'9' and 'A'..'F',
//   or after <max_len> xdigits
// - returns the number of xdigits consumed
// - value of the number is returned through num
//
int rtsp_get_hexadecimal (char **ptr, int max_len, uint64 *num, int *rc)
{
    int i;

    if (*rc != RTSP_RESP_OK)  return 0;

    *num = 0;
    for (i=0; i<max_len; i++) {
        int digit;
        char c = **ptr;
        if (!isxdigit(c)) {
            return i;
        }
        digit = isdigit(c) ? (int)(c - '0') : (int)(toupper(c) - 'A' + 10);
        *num = *num * 16 + digit;
        (*ptr)++;
    }
    return i;
}


int rtsp_parse_url (char *url_txt, char** protocol, uint32 *ip_addr,
                    uint16 *port)
{
    char** ptr = &url_txt;
    char *host;
    struct in_addr in;
    uint32 temp;
    int rc = RTSP_RESP_OK;

    rtsp_get_token(ptr, ":", protocol, &rc);
    *ptr += 2;    // skip over "//"
    rtsp_get_token(ptr, "/:\0", &host, &rc);
    inet_aton(host, &in);
    *ip_addr = in.s_addr;
    *port = 0;          // default
    if (*ptr) {
        rtsp_get_decimal(ptr, &temp, &rc);
        *port = temp;
    }
    return rc;
}


/// Get the request line
void rtsp_get_request_line (char **ptr, rtsp_msg_t *req, int *rc)
{
    char *line;
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_line(ptr, &line, rc);
    if (*rc != RTSP_RESP_OK)  return;
    
    rtsp_get_token(&line, " ", &req->method, rc);
    rtsp_get_token(&line, " ", &req->url, rc);
    req->version = line;
    if (!req->version) {
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }
}


/// Check RTSP SETUP Request
/// @return response code
int rtsp_check_request_line (rtsp_msg_t *req)
{
    char* prot;
    uint32 ip_addr;
    uint16 port;

    if (strcmp(req->version, RTSP_VERSION)) {
        return RTSP_RESP_VERSION_NOT_SUPPORTED;
    }
    rtsp_parse_url(req->url, &prot, &ip_addr, &port);
    return RTSP_RESP_OK;
}


// Get the next header type and value.
// If no more header is found, both type and value will be NULL.
//
void rtsp_get_header (char **ptr, char **type, char **value, int *rc)
{
    char *hdr;

    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_line(ptr, &hdr, rc);
    if (strlen(hdr) == 0) {
        *type = *value = NULL;
        *rc = RTSP_RESP_OK;
        return;
    }

    *type = strsep(&hdr, ":");
    *value = hdr;
    if (value == NULL) {
        RTSP_LOG("Can't find ':' in header");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }
    skip_ws(value);
}


void rtsp_get_transport_spec (char **ptr, rtsp_msg_t *req, int* rc)
{
    rtsp_trans_t* trans = req->transport;

    while (1) {
        if (*rc != RTSP_RESP_OK)  return;

        if (*ptr == NULL) {
            return;
        }
        if (req->transport_cnt > RTSP_MAX_TRANSPORTS) {
            RTSP_LOG("Too many transport spec in a request");
            *rc = RTSP_RESP_INTERNAL_SERVER_ERROR;
        }

        rtsp_get_token(ptr, ",", &trans->spec, rc);
        printf("Trans spec %d: %s\n", req->transport_cnt, trans->spec);

        rtsp_parse_transport_spec(trans, rc);

        trans++;
        req->transport_cnt++;

        if (req->transport_cnt >= 4)  break;
    }
}


void rtsp_parse_header_cseq (char *ptr, rtsp_msg_t *req, int* rc)
{
    int len = rtsp_get_decimal(&ptr, &req->cseq, rc);
    if (len == 0) {
        RTSP_LOG("Bad CSeq");
        *rc = RTSP_RESP_BAD_REQUEST;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CSEQ;
}


void rtsp_parse_header_session (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->ses_id = ptr;
    req->hdr_mask |= RTSP_HDR_MASK_SESSION;
}


void rtsp_parse_header_require (char *ptr, rtsp_msg_t *req, int* rc)
{
    int len = strlcpy(req->require, ptr, RTSP_MAX_TOKEN_LEN);
    if (len >= RTSP_MAX_TOKEN_LEN) {
        *rc = RTSP_RESP_BAD_REQUEST;
    }
    req->hdr_mask |= RTSP_HDR_MASK_REQUIRE;
}


void rtsp_parse_header_transport (char *ptr, rtsp_msg_t *req, int* rc)
{
    rtsp_get_transport_spec(&ptr, req, rc);
    req->hdr_mask |= RTSP_HDR_MASK_TRANSPORT;
}


void rtsp_parse_header_content_type (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->content_type_idx = str_lookup(ptr, rtsp_content_types);
    if (req->content_type_idx == -1) {
        printf("Content_type not understood: %s\n", ptr);
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
}


void rtsp_parse_header_content_length (char *ptr, rtsp_msg_t *req, int* rc)
{
    int len = rtsp_get_decimal(&ptr, &req->content_length, rc);
    if (len == 0) {
        RTSP_LOG("Bad Content-Length");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
}


void rtsp_parse_header_clab_client_session_id (char *ptr, rtsp_msg_t *req,
                                               int* rc)
{
    int len;
    uint64 temp;

    len = rtsp_get_hexadecimal(&ptr, 12, &req->clab_cln_id, rc);
    if (len != 12) {
        RTSP_LOG("Bad clab-ClientSessionId");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }

    len = rtsp_get_hexadecimal(&ptr, 8, &temp, rc);
    if (len != 8) {
        RTSP_LOG("Bad clab-ClientSessionId");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }
    req->clab_ses_id = temp;

    req->hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
}


void rtsp_parse_header_clab_notice (char *ptr, rtsp_msg_t *req, int* rc)
{
    int len;
    char *next;

    len = rtsp_get_decimal(&ptr, &req->notice_code, rc);
    if (len != 4) {
        RTSP_LOG("Bad clab-Reason code");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }

    skip_ws(&ptr);
    rtsp_get_token(&ptr, " ", &next, rc);
    if (next[0] == '"') {
        // Description field exists
        req->notice_desc = next;
    }

    req->hdr_mask |= RTSP_HDR_MASK_CLAB_NOTICE;
}


void rtsp_parse_header_clab_reason (char *ptr, rtsp_msg_t *req, int* rc)
{
    rtsp_get_decimal(&ptr, &req->reason_code, rc);
    req->reason_phrase = ptr;
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_REASON;
}


void rtsp_parse_header_clab_session_group (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->session_group = ptr;
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_SESSION_GROUP;
}


void rtsp_parse_header_clab_priority (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->priority = get_char(&ptr, rc) - '0';
    if (*rc != RTSP_RESP_OK)  return;

    if (req->priority > 9) {
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_PRIORITY;
}


void rtsp_parse_header_clab_setup_type (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->setup_type = get_char(&ptr, rc) - '0';
    if (*rc != RTSP_RESP_OK)  return;

    if (req->setup_type != 0) {
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_SETUP_TYPE;
}


void rtsp_parse_header_clab_pid_remap (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->pid_remap = get_char(&ptr, rc) - '0';
    if (*rc != RTSP_RESP_OK)  return;

    if (req->pid_remap > 1) {
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_PID_REMAP;
}


void rtsp_parse_header_clab_mpts_mode (char *ptr, rtsp_msg_t *req, int* rc)
{
    req->mpts_mode_idx = str_lookup(ptr, rtsp_clab_mpts_modes);
    if (req->mpts_mode_idx == -1) {
        printf("MPTS mode understood: %s\n", ptr);
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_MPTS_MODE;
}


void rtsp_parse_header_clab_statmux_group (char *ptr, rtsp_msg_t *req, int* rc)
{
    char *token;
    rtsp_get_token(&ptr, "=", &token, rc);
    if (strcmp(token, "group_id")) {
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
        return;
    }
    rtsp_get_token(&ptr, " ", &req->statmux_group_id, rc);
    rtsp_get_token(&ptr, "=", &token, rc);
    if (strcmp(token, "group_bitrate")) {
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
        return;
    }
    rtsp_get_decimal(&ptr, &req->statmux_group_bitrate, rc);
    req->hdr_mask |= RTSP_HDR_MASK_CLAB_STATMUX_GROUP;
}


void rtsp_parse_transport_spec (rtsp_trans_t *trans, int *rc)
{
    char *ptr = trans->spec;
    char *par, *key, *val, *val2;
    uint32 temp;

    rtsp_get_token(&ptr, ";", &par, rc);
    if (*rc != RTSP_RESP_OK)  return;

    trans->profile_idx = str_lookup(par, rtsp_trans_profiles);
    if (trans->profile_idx == -1) {
        printf("Transport profile not understood: %s\n", par);
        *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }

    while (1) {
        if (*rc != RTSP_RESP_OK)  return;

        rtsp_get_token(&ptr, ";", &par, rc);
        if (!par)  return;

        rtsp_get_token(&par, "=", &key, rc);
        val = par;
        if (*rc != RTSP_RESP_OK)  return;

        if (!strcmp(key, RTSP_TRANS_PAR_UNICAST)) {
            trans->par_mask |= RTSP_TRANS_PAR_MASK_UNICAST;

        } else if (!strcmp(key, RTSP_TRANS_PAR_MULTICAST)) {
            trans->par_mask |= RTSP_TRANS_PAR_MASK_MULTICAST;

        } else if (!strcmp(key, RTSP_TRANS_PAR_BITRATE)) {
            rtsp_get_decimal(&val, &trans->bitrate, rc);
            trans->par_mask |= RTSP_TRANS_PAR_MASK_BITRATE;

        } else if (!strcmp(key, RTSP_TRANS_PAR_DEPI_MODE)) {
            trans->depi_mode_idx = str_lookup(val, rtsp_trans_depi_modes);
            if (trans->depi_mode_idx == -1) {
                printf("DEPI mode not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_DEPI_MODE;

        } else if (!strcmp(key, RTSP_TRANS_PAR_SRC)) {
            if (!inet_pton(AF_INET, val, &trans->src_ip)) {
                printf("Source IP not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_SRC;

        } else if (!strcmp(key, RTSP_TRANS_PAR_SRC_PORT)) {
            rtsp_get_decimal(&val, &temp, rc);
            trans->src_udp = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_SRC_PORT;

        } else if (!strcmp(key, RTSP_TRANS_PAR_DST)) {
            if (!inet_pton(AF_INET, val, &trans->dst_ip)) {
                printf("Destinatio IP not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_DST;

        } else if (!strcmp(key, RTSP_TRANS_PAR_DST_PORT)) {
            rtsp_get_decimal(&val, &temp, rc);
            trans->dst_udp = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_DST_PORT;

        } else if (!strcmp(key, RTSP_TRANS_PAR_MULTICAST_ADDR)) {
            if (!inet_pton(AF_INET, val, &trans->mcast_ip)) {
                printf("Multicast IP not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_MULTICAST_ADDR;

        } else if (!strcmp(key, RTSP_TRANS_PAR_RANK)) {
            rtsp_get_decimal(&val, &trans->rank, rc);
            trans->par_mask |= RTSP_TRANS_PAR_MASK_RANK;

        } else if (!strcmp(key, RTSP_TRANS_PAR_MPTS_PROG)) {
            rtsp_get_decimal(&val, &temp, rc);
            trans->mpts_prog = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_MPTS_PROG;

        } else if (!strcmp(key, RTSP_TRANS_PAR_QAM_TSID)) {
            rtsp_get_decimal(&val, &temp, rc);
            trans->tsid = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_TSID;

        } else if (!strcmp(key, RTSP_TRANS_PAR_FIBER_NODE)) {
            trans->fiber_nodes = val;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_FIBER_NODE;

        } else if (!strcmp(key, RTSP_TRANS_PAR_FREQ_RANGE)) {
            rtsp_get_token(&val, "-", &val2, rc);
            rtsp_get_decimal(&val2, &trans->freq_low, rc);
            rtsp_get_decimal(&val, &trans->freq_high, rc);
            trans->par_mask |= RTSP_TRANS_PAR_MASK_FREQ_RANGE;

        } else if (!strcmp(key, RTSP_TRANS_PAR_QAM_NAME)) {
            // qam-name = service-group "." TSID
            char* ptr = strrchr(val, '.');
            if (!ptr) { 
                printf("QAM name not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
                continue;
            }
            *ptr = 0;
            trans->service_group = val;
            val = ptr + 1;
            rtsp_get_decimal(&val, &temp, rc);
            trans->tsid = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_NAME;

        } else if (!strcmp(key, RTSP_TRANS_PAR_QAM_DST)) {
            // qam-destination = frequency "." program-number
            rtsp_get_token(&val, ".", &val2, rc);
            rtsp_get_decimal(&val2, &trans->freq, rc);
            rtsp_get_decimal(&val, &temp, rc);
            trans->prog_num = temp;
            trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_DST;

        } else if (!strcmp(key, RTSP_TRANS_PAR_MODULATION)) {
            // modulation-value = "64" | "256"
            rtsp_get_decimal(&val, &trans->modulation, rc);
            if (trans->modulation != 64 && trans->modulation != 256) {
                printf("Modulation not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_MODULATION;

        } else if (!strcmp(key, RTSP_TRANS_PAR_J83_ANNEX)) {
            trans->j83_annex_idx = str_lookup(val, rtsp_trans_j83_annexes);
            if (trans->j83_annex_idx == -1) {
                printf("J83 Annex not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_J83_ANNEX;

        } else if (!strcmp(key, RTSP_TRANS_PAR_TAPS)) {
            rtsp_get_decimal(&val2, &trans->taps, rc);
            trans->par_mask |= RTSP_TRANS_PAR_MASK_TAPS;

        } else if (!strcmp(key, RTSP_TRANS_PAR_CHANNEL_WIDTH)) {
            // channel-width = "6" | "7" | "8"
            rtsp_get_decimal(&val2, &trans->channel_width, rc);
            if (trans->channel_width < 6 || trans->channel_width > 8) {
                printf("Channel width not understood: %s\n", val);
                *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            }
            trans->par_mask |= RTSP_TRANS_PAR_MASK_CHANNEL_WIDTH;

        } else if (!strcmp(key, RTSP_TRANS_PAR_SYMBOL_RATE)) {
            rtsp_get_decimal(&val2, &trans->symbol_rate, rc);
            trans->par_mask |= RTSP_TRANS_PAR_MASK_SYMBOL_RATE;

        } else {
            printf("Unknown transport parameter: %s\n", key);
        }
    }
}


void rtsp_get_headers (char **ptr, rtsp_msg_t *req, int *rc)
{
    char *type;
    char *value;

    if (*rc != RTSP_RESP_OK)  return;

    while (1) {
        rtsp_get_header(ptr, &type, &value, rc);

        if (*rc != RTSP_RESP_OK)  return;
        if (type == NULL)  break;

        if (!strcmp(type, RTSP_HDR_CSEQ)) {
            rtsp_parse_header_cseq(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_SESSION)) {
            rtsp_parse_header_session(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_REQUIRE)) {
            rtsp_parse_header_require(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_TRANSPORT)) {
            rtsp_parse_header_transport(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CONTENT_TYPE)) {
            rtsp_parse_header_content_type(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CONTENT_LENGTH)) {
            rtsp_parse_header_content_length(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_CLIENT_SESSION_ID)) {
            rtsp_parse_header_clab_client_session_id(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_NOTICE)) {
            rtsp_parse_header_clab_notice(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_REASON)) {
            rtsp_parse_header_clab_reason(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_SESSION_GROUP)) {
            rtsp_parse_header_clab_session_group(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_PRIORITY)) {
            rtsp_parse_header_clab_priority(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_SETUP_TYPE)) {
            rtsp_parse_header_clab_setup_type(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_PID_REMAP)) {
            rtsp_parse_header_clab_pid_remap(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_MPTS_MODE)) {
            rtsp_parse_header_clab_mpts_mode(value, req, rc);

        } else if (!strcmp(type, RTSP_HDR_CLAB_STATMUX_GROUP)) {
            rtsp_parse_header_clab_statmux_group(value, req, rc);

        } else {
            RTSP_LOG("Unknown header: %s", type);
            *rc = RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
            return;
        }
    }
}


void rtsp_check_header_cseq (rtsp_msg_t *req, rtsp_conn_t *conn,
                             boolean mandatory, int *rc)
{
    if (*rc != RTSP_RESP_OK)  return;

    // Check for CSeq
    if (!(req->hdr_mask & RTSP_HDR_MASK_CSEQ)) {
        if (mandatory) {
            printf("CSeq header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }

    if (conn->active_flag) {
        int diff = (int32)(req->cseq - conn->rx_cseq);
        if (diff == 1) {
            // Next message
            conn->rx_cseq = req->cseq;
        } else {
            printf("Bad CSeq %d, expected %d\n", req->cseq, conn->rx_cseq + 1);
            *rc = RTSP_RESP_BAD_REQUEST;  /// @todo Is this correct?
            return;
        }
    } else {
        // Initialize CSeq
        conn->rx_cseq = req->cseq;
        conn->active_flag = TRUE;
    }
}


void rtsp_check_header_session (rtsp_msg_t *req, rtsp_conn_t *conn,
                                boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_SESSION)) {
        if (mandatory) {
            printf("Session header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_require (rtsp_msg_t *req, rtsp_conn_t *conn,
                                boolean mandatory, int *rc)
{
    if (*rc != RTSP_RESP_OK)  return;

    if (!(req->hdr_mask & RTSP_HDR_MASK_REQUIRE)) {
        if (mandatory) {
            printf("Require header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
    if (strcmp(req->require, RTSP_ERMI_REQUIRE)) {
        printf("Require option not supported: %s\n", req->require);
        *rc = RTSP_RESP_OPTION_NOT_SUPPORTED;
        return;
    }
}


void rtsp_check_header_transport (rtsp_msg_t *req, rtsp_conn_t *conn,
                                  boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_TRANSPORT)) {
        if (mandatory) {
            printf("Transport header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_content_type (rtsp_msg_t *req, rtsp_conn_t *conn,
                                     boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CONTENT_TYPE)) {
        if (mandatory) {
            printf("Content-Type header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_content_length (rtsp_msg_t *req, rtsp_conn_t *conn,
                                       boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CONTENT_LENGTH)) {
        if (mandatory) {
            printf("Content-Length header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_client_session_id (rtsp_msg_t *req,
                        rtsp_conn_t *conn, boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID)) {
        if (mandatory) {
            printf("clab-ClientSessionId header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_notice (rtsp_msg_t *req, rtsp_conn_t *conn,
                                    boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_NOTICE)) {
        if (mandatory) {
            printf("clab-Notice header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_reason (rtsp_msg_t *req, rtsp_conn_t *conn,
                                    boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_REASON)) {
        if (mandatory) {
            printf("clab-Reason header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_session_group (rtsp_msg_t *req, rtsp_conn_t *conn,
                                    boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_REASON)) {
        if (mandatory) {
            printf("clab-Reason header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_priority (rtsp_msg_t *req, rtsp_conn_t *conn,
                                      boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_PRIORITY)) {
        if (mandatory) {
            printf("clab-Priority header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_setup_type (rtsp_msg_t *req, rtsp_conn_t *conn,
                                        boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_SETUP_TYPE)) {
        if (mandatory) {
            printf("clab-SetupType header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_pid_remap (rtsp_msg_t *req, rtsp_conn_t *conn,
                                       boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_PID_REMAP)) {
        if (mandatory) {
            printf("clab-PidRemap header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_mpts_mode (rtsp_msg_t *req, rtsp_conn_t *conn,
                                       boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_MPTS_MODE)) {
        if (mandatory) {
            printf("clab-MPTSMode header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_check_header_clab_statmux_group (rtsp_msg_t *req, rtsp_conn_t *conn,
                                           boolean mandatory, int *rc)
{
    if (!(req->hdr_mask & RTSP_HDR_MASK_CLAB_STATMUX_GROUP)) {
        if (mandatory) {
            printf("clab-StatmuxGroup header missing\n");
            *rc = RTSP_RESP_BAD_REQUEST;
        }
        return;
    }
}


void rtsp_get_request (char **ptr, rtsp_msg_t *req, int* rc)
{
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_request_line(ptr, req, rc);
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_headers(ptr, req, rc);

    req->msg_body = *ptr;
}


void rtsp_get_response_line (char **ptr, rtsp_msg_t *rsp, int *rc)
{
    char *line;
    if (*rc != RTSP_RESP_OK)  return;
    
    rtsp_get_line(ptr, &line, rc);
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_token(&line, " ", &rsp->version, rc);
    rtsp_get_decimal(&line, &rsp->rc, rc);
    rsp->rc_phrase = line;
}


void rtsp_get_response (char **ptr, rtsp_msg_t *rsp, int* rc)
{
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_response_line(ptr, rsp, rc);
    if (*rc != RTSP_RESP_OK)  return;

    rtsp_get_headers(ptr, rsp, rc);

    rsp->msg_body = *ptr;
}


/// Check RTSP SETUP Request
/// @return response code
int rtsp_check_request_setup (rtsp_msg_t *req, rtsp_conn_t *conn)
{
    boolean clab_client_session_id_req = FALSE;

    int rc = rtsp_check_request_line(req);
    rtsp_check_header_cseq(req, conn, TRUE, &rc);
    rtsp_check_header_require(req, conn, TRUE, &rc);
    rtsp_check_header_transport(req, conn, TRUE, &rc);
    if ((req->hdr_mask & RTSP_HDR_MASK_CONTENT_LENGTH)) {
        rtsp_check_header_content_type(req, conn, TRUE, &rc);
        rtsp_check_header_content_length(req, conn, TRUE, &rc);
    }
    if (rc == RTSP_RESP_OK && req->transport_cnt == 0) {
        return RTSP_RESP_BAD_REQUEST;
    }
    if (req->transport[0].profile_idx == RTSP_TRANS_PROFILE_IDX_MP2T_QAM ||
        req->transport[0].profile_idx == RTSP_TRANS_PROFILE_IDX_MP2T_UDP) {
        clab_client_session_id_req = TRUE;
    }
    rtsp_check_header_clab_client_session_id(req, conn,
                                             clab_client_session_id_req, &rc);
    rtsp_check_header_clab_mpts_mode(req, conn, FALSE, &rc);           // ?
    return rc;
}


int rtsp_check_request_teardown (rtsp_msg_t *req, rtsp_conn_t *conn)
{
    int rc = rtsp_check_request_line(req);
    rtsp_check_header_cseq(req, conn, TRUE, &rc);
    rtsp_check_header_require(req, conn, TRUE, &rc);
    rtsp_check_header_session(req, conn, TRUE, &rc);
    return rc;
}


int rtsp_check_request_get_parameter (rtsp_msg_t *req, rtsp_conn_t *conn)
{
    int rc = rtsp_check_request_line(req);
    rtsp_check_header_cseq(req, conn, TRUE, &rc);
    rtsp_check_header_require(req, conn, TRUE, &rc);
    rtsp_check_header_content_type(req, conn, TRUE, &rc);
    rtsp_check_header_content_length(req, conn, TRUE, &rc);
    return rc;
}


int rtsp_check_request_set_parameter (rtsp_msg_t *req, rtsp_conn_t *conn)
{
    int rc = rtsp_check_request_line(req);
    rtsp_check_header_cseq(req, conn, TRUE, &rc);
    rtsp_check_header_require(req, conn, TRUE, &rc);
    return rc;
}

