///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      eqam.c
///  @brief     EQAM simulaiton for ERMI-2
///  @author    Yi Tong Tse


#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "rtsp.h"
#include "cli.h"


#define EQAM_LOG(fmt, str...)    \
      printf("EQAM: "fmt"\n", ##str);
//    NULL


#define URI                     "rtsp://127.0.0.10:6070"
#define SERVICE_GROUP           "SanJoseHub"
#define LISTENQ                 1024
#define MAX_CONNS               4
#define MAX_SESSIONS            (MAX_CONNS * RTSP_MAX_SESSIONS_PER_CONN)
#define CSEQ_START              1000
#define RTSP_SESSION_TIMEOUT    60


rtsp_conn_t rtsp_conn[MAX_CONNS];
rtsp_session_t rtsp_ses[MAX_SESSIONS];
rtsp_msg_t req;
char msg_buf[RTSP_MAX_MSG_BYTES];
char date_time_buf[RTSP_MAX_TOKEN_LEN];


/******** CLI Syntax ********
announce ses-id notice-code
show session
exit
*****************************/

// Prototypes
int cli_announce(cli_control_block *ccb);
int cli_show_connections(cli_control_block *ccb);
int cli_show_sessions(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);

//// CLI Parser
CLI_ARR_START(cli_parser)

CLI_ARR_STR(0, announce, CLI_NULL(), 0, "Send ANNOUNCE")
CLI_ARR_GSTR(1, CLI_NULL(), 0, "Session ID")
CLI_ARR_NUM(2, 1000, 9999, CLI_FUN(cli_announce, 0), 0, "Notice Code")

CLI_ARR_STR(0, show, CLI_NULL(), 0, "Show commands")
CLI_ARR_STR(1, connection, CLI_FUN(cli_show_connections, 0), 0,
            "Show all connections")
CLI_ARR_STR(1, session, CLI_FUN(cli_show_sessions, 0), 0, "Show all sessions")

CLI_ARR_STR (0, exit, CLI_FUN(cli_exit, 0), 0, "Exit")
CLI_ARR_END
//// End of CLI parser

int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


void get_date_time (char *buf, int sz)
{
    time_t cur_time = time(NULL);
    struct tm* ltime = localtime(&cur_time);
    strftime(buf, sz, "%Y%m%dT%H%M%S.0Z", ltime);
}


/// Look up session using session_id
rtsp_session_t* lookup_session (const char *session_id, rtsp_conn_t *conn)
{
    int i;

    for (i=0; i<MAX_SESSIONS; i++) {
        if (!strncmp(session_id, rtsp_ses[i].ses_id, RTSP_MAX_TOKEN_LEN)) {
            return &rtsp_ses[i];
        }
    }
    return NULL;
}


int setup_socket (void)
{
    int listen_fd; 
    struct sockaddr_in srv_addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(-1);
    }

    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(RTSP_TCP_PORT_DFT);

    if (bind(listen_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

    if (listen(listen_fd, LISTENQ) == -1) {
        perror("listen");
        exit(-1);
    }

    return listen_fd;
}


void process_transports (rtsp_msg_t *req, rtsp_msg_t *rsp, int *rc)
{
    int i;
    int video_cnt = 0;
    int docsis_cnt = 0;
    int qam_cnt = 0;
    int ucast_cnt = 0;
    int mcast_cnt = 0;
    int mcast_rank = 10000;

    for (i=0; i<req->transport_cnt; i++) {
        rtsp_trans_t *trans = &req->transport[i];

        switch (trans->profile_idx) {

        case RTSP_TRANS_PROFILE_IDX_DOCSIS_QAM:
            docsis_cnt++;
            if (qam_cnt++ == 0) {
                // Just pick up the first QAM channel
                rsp->transport[0] = *trans;
            }
            break;

        case RTSP_TRANS_PROFILE_IDX_MP2T_QAM:
            rsp->transport[0] = *trans;  // always use 1st spec for QAM
            video_cnt++;
            qam_cnt++;
            break;

        case RTSP_TRANS_PROFILE_IDX_MP2T_UDP:
            if ((trans->par_mask & RTSP_TRANS_PAR_MASK_UNICAST)) {
                rsp->transport[1] = *trans;  // always use 2nd spec for UDP
                ucast_cnt++;

            } else if ((trans->par_mask & RTSP_TRANS_PAR_MASK_MULTICAST)) {
                if (trans->rank < mcast_rank) {
                    rsp->transport[1] = *trans;  // always use 2nd spec for UDP
                    mcast_rank = trans->rank;
                }
                mcast_cnt++;
            }
            video_cnt++;
            break;

        default:
            printf("Unknonw transport profile idx %d\n", trans->profile_idx);
            *rc = RTSP_RESP_UNSUPPORTED_TRANSPORT;
        }
    }

    printf("Summary: video %d, docsis %d, qam %d, ucast %d, mcast %d\n",
           video_cnt, docsis_cnt, qam_cnt, ucast_cnt, mcast_cnt);


    if (video_cnt * docsis_cnt != 0) {
        printf("Error: Mixed video and docsis transport spec\n");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }

    if (video_cnt + docsis_cnt == 0) {
        printf("Error: No video and docsis transport spec\n");
        *rc = RTSP_RESP_BAD_REQUEST;
        return;
    }

    if (docsis_cnt) {
        rtsp_trans_t* trans = &rsp->transport[0];
        rsp->transport_cnt = 1;
        trans->par_mask &= ~RTSP_TRANS_PAR_MASK_QAM_TSID;
        trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_NAME;
        trans->service_group = SERVICE_GROUP;
    }

    if (video_cnt) {
        if (qam_cnt != 1 || ucast_cnt > 1 || ucast_cnt * mcast_cnt != 0) {
            printf("Bad video transport spec combination\n");
            printf("  qam %d, ucast %d, mcast %d\n",
                   qam_cnt, ucast_cnt, mcast_cnt);
            *rc = RTSP_RESP_BAD_REQUEST;
            return;
        }
        rsp->transport_cnt = 2;

        rsp->clab_cln_id = req->clab_cln_id;
        rsp->clab_ses_id = req->clab_ses_id;
        rsp->hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    }
}


int cli_announce (cli_control_block *ccb)
{
    int i, j;
    rtsp_conn_t* conn = NULL;
    rtsp_session_t* ses = NULL;
    int len;
    char *msg_ptr = msg_buf;

    cli_prepare_cc(ccb);

    for (i=0; i<MAX_CONNS; i++) {
        conn = &rtsp_conn[i];
        if (!conn->connected)  continue;

        for (j=0; j<RTSP_MAX_SESSIONS_PER_CONN; j++) {
            if (!conn->session[j]) {
                printf("Error: Conn %d, ses %d is NULL\n", i, j);
                continue;
            }
            if (conn->session[j]->used &&
                !strncmp(ccb->tokens[1], conn->session[j]->ses_id,
                         RTSP_MAX_TOKEN_LEN)) {
                ses = conn->session[j];
                break;
            }
        }
        if (ses)  break;
    }
    if (!ses) {
        printf("Error: session ID not found: %s\n", ccb->tokens[1]);
        return 0;
    }

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn->tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_NOTICE;
    req.notice_code = ccb->var[2];
    req.date_time = date_time_buf;
    get_date_time(req.date_time, RTSP_MAX_TOKEN_LEN);

    req.hdr_mask |= RTSP_HDR_MASK_SESSION;
    req.ses_id = ses->ses_id;

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    req.clab_cln_id = ses->clab_cln_id;
    req.clab_ses_id = ses->clab_ses_id;

    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_ANNOUNCE, URI);

    printf("\nS -> C:\n");
    printf("%s\n", msg_buf);
    io_send(conn->fd, msg_buf, len);

    return 0;    
}


int cli_show_connections (cli_control_block *ccb)
{
    int i, j;
    cli_prepare_cc(ccb);

    for (i=0; i<MAX_CONNS; i++) {
        rtsp_conn_t *conn = &rtsp_conn[i];
        if (conn->connected) {
            printf("Connection %d:", i);
            for (j=0; j<RTSP_MAX_SESSION_GROUPS_PER_CONN; j++) {
                if (strlen(conn->session_group[j]) > 0) {
                    printf(" %s", conn->session_group[j]);
                }
            }
            printf("\n");
        }
    }
    return 0;
}


int cli_show_sessions (cli_control_block *ccb)
{
    int i;
    cli_prepare_cc(ccb);

    for (i=0; i<MAX_SESSIONS; i++) {
        rtsp_session_t *ses = &rtsp_ses[i];
        if (ses->used) {
            printf("Session %s", ses->ses_id);
            if (strlen(ses->ses_grp) > 0) {
                printf(": %s", ses->ses_grp);
            }
            printf("\n");
        }
    }
    return 0;
}


static int process_setup (rtsp_msg_t *req, rtsp_msg_t *rsp, rtsp_conn_t *conn)
{
    int i;
    rtsp_session_t *ses = NULL;

    int rc = rtsp_check_request_setup(req, conn);
    if (rc != RTSP_RESP_OK)  return rc;

    // Find an available session to use
    for (i=0; i<RTSP_MAX_SESSIONS_PER_CONN; i++) {
        if (!conn->session[i]->used) {
            ses = conn->session[i];
            break;
        }
    }   
    if (!ses) {
        return RTSP_RESP_SERVICE_UNAVAILABLE;
    }

    // Set up the session
    memset(ses, 0, sizeof(*ses));
    ses->used = TRUE;
    snprintf(ses->ses_id, RTSP_MAX_TOKEN_LEN, "%08x", conn->ses_id++);

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID)) {
        ses->clab_cln_id = req->clab_cln_id;
        ses->clab_ses_id = req->clab_ses_id;
    }   

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_SESSION_GROUP)) {
        int len = strlcpy(ses->ses_grp, req->session_group, RTSP_MAX_TOKEN_LEN);
        if (len >= RTSP_MAX_TOKEN_LEN) {
            return RTSP_RESP_INTERNAL_SERVER_ERROR;
        }   
    }   

    rsp->ses_id = ses->ses_id;
    rsp->timeout = RTSP_SESSION_TIMEOUT;
    rsp->hdr_mask |= RTSP_HDR_MASK_SESSION;

    process_transports(req, rsp, &rc);
    rsp->hdr_mask |= RTSP_HDR_MASK_TRANSPORT;

    return rc;
}


static int process_teardown (rtsp_msg_t *req, rtsp_msg_t *rsp, 
                             rtsp_conn_t *conn)
{
    rtsp_session_t *ses;

    int rc = rtsp_check_request_teardown(req, conn);
    if (rc != RTSP_RESP_OK)  return rc;

    ses = lookup_session(req->ses_id, conn);
    if (!ses) {
        return RTSP_RESP_SESSION_NOT_FOUND;
    }

    if ((req->hdr_mask & RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID)) {
        if (ses->clab_cln_id != req->clab_cln_id ||
            ses->clab_ses_id != req->clab_ses_id) {
            return RTSP_RESP_SESSION_NOT_FOUND;
        }
    }

    ses->used = FALSE;

    return rc;
}


static int process_get_parameter (rtsp_msg_t *req, rtsp_msg_t *rsp,
                                  rtsp_conn_t *conn)
{
    char* ptr = rsp->msg_body;

    int rc = rtsp_check_request_get_parameter(req, conn);
    if (rc != RTSP_RESP_OK)  return rc;

    skip_ws(&req->msg_body);

    if (!strcmp(req->msg_body, RTSP_PARAM_CLAB_SESSION_LIST)) {
        int i;
        boolean first_session = TRUE;
        rsp->content_length = put_format(&ptr, "%s:",
                                         RTSP_PARAM_CLAB_SESSION_LIST);
        for (i=0; i<MAX_SESSIONS; i++) {
            rtsp_session_t* ses = &rtsp_ses[i];
            if (ses->used && (!req->session_group ||
                !strcmp(ses->ses_grp, req->session_group))) {
                if (!first_session) {
                    rsp->content_length += put_format(&ptr, ";");
                }
                rsp->content_length +=
                    put_format(&ptr, "%s:%012llx%08x", ses->ses_id,
                               (long long)ses->clab_cln_id, ses->clab_ses_id);
                first_session = FALSE;
            }
        }
        if (rsp->content_length > 0) {
            rsp->hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
            rsp->hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
            rsp->content_type_idx = RTSP_CONTENT_TYPE_IDX_TEXT_PARAM;
        }

    } else if (!strcmp(req->msg_body, RTSP_PARAM_CLAB_CONNECTION_TIMEOUT)) {
        rsp->content_length = put_format(&ptr, "%s:%d",
                      RTSP_PARAM_CLAB_CONNECTION_TIMEOUT, RTSP_SESSION_TIMEOUT);
        rsp->hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
        rsp->hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
        rsp->content_type_idx = RTSP_CONTENT_TYPE_IDX_TEXT_PARAM;

    } else {
        return RTSP_RESP_BAD_REQUEST;
    }

    return rc;
}


static int process_set_parameter (rtsp_msg_t *req, rtsp_msg_t *rsp,
                                  rtsp_conn_t *conn)
{
    char *param;
    char *ses_grp;
    int rc = rtsp_check_request_set_parameter(req, conn);

    rtsp_get_token(&req->msg_body, ":", &param, &rc);
    if (rc != RTSP_RESP_OK)  return rc;

    if (!strcmp(param, RTSP_PARAM_CLAB_SESSION_GROUP_LIST)) {
        int i;
        for (i=0; i<4; i++) {
            int len;
            rtsp_get_token(&req->msg_body, " ", &ses_grp, &rc);
            if (!ses_grp || *ses_grp == 0)  break;
            len = strlcpy(conn->session_group[i], ses_grp, RTSP_MAX_TOKEN_LEN);
            printf("Session group %d: %s\n", i, conn->session_group[i]);
            if (len >= RTSP_MAX_TOKEN_LEN)  return RTSP_RESP_BAD_REQUEST;
        }
        for ( ; i<4; i++) {
            *conn->session_group[i] = 0;
        }

    } else {
        printf("Unknown set parameter %s\n", param);
        return RTSP_RESP_PARAMETER_NOT_UNDERSTOOD;
    }
    return rc;
}


/// ERM Server main function (for single RTSP connection)
void* server_main (void* arg)
{
    int sock_fd = (size_t)arg;
    struct sockaddr_in cln_addr;
    socklen_t len = sizeof(cln_addr);
    rtsp_conn_t *conn = NULL;
    char addr_buf[INET_ADDRSTRLEN];
    uint16 msg_len;
    char *msg;
    char *msg_body;
    rtsp_msg_t req;
    rtsp_msg_t rsp;
    int rc;
    int i;

    for (i=0; i<MAX_CONNS; i++) {
        if (!rtsp_conn[i].connected) {
            conn = &rtsp_conn[i];
            break;
        }
    }
    if (!conn) {
        EQAM_LOG("REJECT EQAM connection from %s : %d\n",
            inet_ntop(AF_INET, &cln_addr.sin_addr, addr_buf, sizeof(addr_buf)),
            ntohs(cln_addr.sin_port));
        close(sock_fd);
        return (void*)0;
    }

    rtsp_conn_init(conn);
    conn->fd = sock_fd;
    conn->connected = TRUE;
    conn->tx_cseq = CSEQ_START;
    conn->ses_id = 0xABCD0001;
    msg_body = malloc(RTSP_MAX_MSG_BYTES);

    getpeername(sock_fd, (struct sockaddr *)&cln_addr, &len);
    EQAM_LOG("Connection from %s : %d\n",
            inet_ntop(AF_INET, &cln_addr.sin_addr, addr_buf, sizeof(addr_buf)),
            ntohs(cln_addr.sin_port));

    // Wait for incoming message
    while (conn->connected) {
        rc = rtsp_get_peer_msg(conn, &msg_len);

        if (rc == EOF || rc == ECONNRESET) {
            printf("EQAM: ERM disconnected\n");
            return (void*)0;
        }
        if (rc == E2BIG) {
            return (void*)0;
        }
        if (rc != EOK) {
            printf("rtsp_get_peer_msg: %s\n", strerror(rc));
            return (void*)0;
        }

        printf("\nC -> S:\n");
        printf("%s\n", conn->in_msg_buf);

        // Process request
        memset(&req, 0, sizeof(req));

        msg = conn->in_msg_buf;
        rc = RTSP_RESP_OK;
        rtsp_get_request(&msg, &req, &rc);

        // Generate response
        memset(&rsp, 0, sizeof(rsp));
        memset(msg_body, 0, RTSP_MAX_MSG_BYTES);
        rsp.msg_body = msg_body;
 
        rsp.cseq = req.cseq;
        rsp.hdr_mask |= RTSP_HDR_MASK_CSEQ;
        rsp.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

        if (rc != RTSP_RESP_OK) {
            goto send;
        }

        if (!strcmp(req.method, RTSP_METHOD_SETUP)) {
            rc = process_setup(&req, &rsp, conn);

        } else if (!strcmp(req.method, RTSP_METHOD_TEARDOWN)) {
            rc = process_teardown(&req, &rsp, conn);

        } else if (!strcmp(req.method, RTSP_METHOD_GET_PARAMETER)) {
            rc = process_get_parameter(&req, &rsp, conn);

        } else if (!strcmp(req.method, RTSP_METHOD_SET_PARAMETER)) {
            rc = process_set_parameter(&req, &rsp, conn);

        } else {
            rc = RTSP_RESP_BAD_REQUEST;
        }

send:
        // Send response
        rsp.rc = rc;
        msg = conn->in_msg_buf;
        msg_len = rtsp_put_response(&msg, &rsp);

        printf("\nS -> C:\n");
        printf("%s\n", conn->in_msg_buf);
        io_send(conn->fd, conn->in_msg_buf, msg_len);
    }

    return (void*)0;
}


void* listen_main (void *arg)
{
    int listen_fd;
    int conn_fd;
    struct sockaddr_in cln_addr;
    pthread_t server_tid;
    int rc;
    
    // Open socket to listen for EQAM
    listen_fd = setup_socket();
    
    for ( ; ; ) {
        socklen_t len = sizeof(cln_addr);
        conn_fd = accept(listen_fd, (struct sockaddr *)&cln_addr, &len);
        if (conn_fd < 0) {
            perror("accept");
            exit(-1);
        }

        // Create thread for concurrent server
        rc = pthread_create(&server_tid, NULL, server_main,
                            (void*)(size_t)conn_fd);
        if (rc != EOK) {
            printf("Failed to create server thread\n");
            exit(-1);
        }
    }

    return (void*)0;
}


int main (int argc, char** argv)
{
    int i, j;
    int rc;
    pthread_t listen_tid;
    rtsp_session_t *ses;

    // Initialize all RTSP sessions
    memset(rtsp_ses, 0, sizeof(rtsp_ses));

    // Setup connections
    memset(rtsp_conn, 0, sizeof(rtsp_conn));
    ses = rtsp_ses;
    for (i=0; i<MAX_CONNS; i++) {
        rtsp_conn[i].id = i;
        for (j=0; j<RTSP_MAX_SESSIONS_PER_CONN; j++) {
            rtsp_conn[i].session[j] = ses++;
        }
    }

    // Create a thread to listen for connection
    rc = pthread_create(&listen_tid, NULL, listen_main, NULL);
    if (rc != EOK) {
        printf("Failed to create listen thread\n");
        exit(-1);
    }

    // CLI loop
    cli_init();
    cli_set_prompt("EQAM> ");
    cli_add_elems(cli_parser);
    rc = cli_main_loop();
    kill(0, SIGKILL);

    return rc;
}
