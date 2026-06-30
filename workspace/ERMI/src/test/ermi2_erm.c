/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file erm.c
*    @brief ERM simulator for ERMI-2
*    @author Yi Tong Tse
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "util.h"
#include "os_util_lnx.h"
#include "os_util.h"
#include "rtsp.h"
#include "cli.h"


#define CSEQ_START              100
#define SERVICE_GROUP           "SanJose.Hub"
#define MAX_RTSP_SESSIONS       10
#define CLAB_CLIENT_ID          0x1234ABCD0000
#define CLAB_SESSION_ID         0x1000
#define URI                     "rtsp://127.0.0.10:6070"

char msg_buf[RTSP_MAX_MSG_BYTES];
char msg_body[RTSP_MAX_MSG_BYTES];

rtsp_conn_t conn;
rtsp_msg_t req;
rtsp_msg_t rsp;

uint64 clab_client_id = CLAB_CLIENT_ID;
uint32 clab_session_id = CLAB_SESSION_ID;


/******** CLI Syntax ********
add docsis num_qam bitrate tsid
add video unicast dst_ip dst_udp bitrate prog_num tsid freq [ses_grp]
add video multicast num_src src_ip grp_ip bitrate prog_num tsid freq [ses_grp]
delete ses_id reason_code
get session-list [session_group]
get connection-timeout
set session-group "session_group_list"
exit
*****************************/

// Prototypes
int cli_setup_docsis_session(cli_control_block *ccb);
int cli_setup_ucast_video_session(cli_control_block *ccb);
int cli_setup_mcast_video_session(cli_control_block *ccb);
int cli_teardown_session(cli_control_block *ccb);
int cli_get_param_session_list(cli_control_block *ccb);
int cli_get_param_connection_timeout(cli_control_block *ccb);
int cli_set_param_session_group_list(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);

//// CLI Parser
CLI_ARR_START(cli_parser)
CLI_ARR_STR(0, add, CLI_NULL(), 0, "Set up session")

CLI_ARR_STR(1, docsis, CLI_NULL(), 0, "Set up DOCSIS session")
CLI_ARR_NUM(2, 1, 4, CLI_NULL(), 0, "Number of QAM channels")
CLI_ARR_NUM(3, 0, 52000000, CLI_NULL(), 0, "Bitrate in bps")
CLI_ARR_NUM(4, 0, 65535, CLI_FUN(cli_setup_docsis_session, 0), 0, "TSID")

CLI_ARR_STR(1, video, CLI_NULL(), 0, "Set up video session")

CLI_ARR_STR(2, unicast, CLI_NULL(), 0, "Set up unicast video session")
CLI_ARR_GSTR(3, CLI_NULL(), 0, "Destination IP")
CLI_ARR_NUM(4, 0, 65535, CLI_NULL(), 0, "Destination UDP")
CLI_ARR_NUM(5, 0, 52000000, CLI_NULL(), 0, "Bitrate in bps")
CLI_ARR_NUM(6, 0, 65535, CLI_NULL(), 0, "Program number")
CLI_ARR_NUM(7, 0, 65535, CLI_NULL(), 0, "TSID")
CLI_ARR_NUM(8, 0, 1000000000, CLI_FUN(cli_setup_ucast_video_session, 0), 0,
            "Frequency")
CLI_ARR_GSTR(9, CLI_FUN(cli_setup_ucast_video_session, 0), 0, "Session Group")

CLI_ARR_STR(2, multicast, CLI_NULL(), 0, "Set up multicast video session")
CLI_ARR_NUM(3, 1, 3, CLI_NULL(), 0, "Number of sources")
CLI_ARR_GSTR(4, CLI_NULL(), 0, "Source IP")
CLI_ARR_GSTR(5, CLI_NULL(), 0, "Group IP")
CLI_ARR_NUM(6, 0, 52000000, CLI_NULL(), 0, "Bitrate in bps")
CLI_ARR_NUM(7, 0, 65535, CLI_NULL(), 0, "Program number")
CLI_ARR_NUM(8, 0, 65535, CLI_NULL(), 0, "TSID")
CLI_ARR_NUM(9, 0, 1000000000, CLI_FUN(cli_setup_mcast_video_session, 0), 0,
            "Frequency")
CLI_ARR_GSTR(10, CLI_FUN(cli_setup_mcast_video_session, 0), 0, "Session Group")

CLI_ARR_STR(0, delete, CLI_NULL(), 0, "Teardown session")
CLI_ARR_GSTR(1, CLI_NULL(), 0, "Session ID")
CLI_ARR_NUM(2, 0, 600, CLI_FUN(cli_teardown_session, 0), 0, "Reason Code")

CLI_ARR_STR(0, get, CLI_NULL(), 0, "Get parameter")

CLI_ARR_STR(1, session-list, CLI_FUN(cli_get_param_session_list, 0),
            0, "Get session-list")
CLI_ARR_GSTR(2, CLI_FUN(cli_get_param_session_list, 0), 0, "Session Group")

CLI_ARR_STR(1, connection-timeout, CLI_FUN(cli_get_param_connection_timeout, 0),
            0, "Get connection-timeout")

CLI_ARR_STR(0, set, CLI_NULL(), 0, "Set parameter")
CLI_ARR_STR(1, session-group-list, CLI_NULL(), 0, "Set session group list")
CLI_ARR_GSTR(2, CLI_FUN(cli_set_param_session_group_list, 0), 0,
             "Session group list (comma-separated list without space)")

CLI_ARR_STR(0, exit, CLI_FUN(cli_exit, 0), 0, "Exit")
CLI_ARR_END
//// End of CLI parser


int sockaddr_setup(struct sockaddr_in *addr, const char* ip_addr, uint16 port)
{
    bzero(addr, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    return inet_pton(AF_INET, ip_addr, &addr->sin_addr);
}


// Set up a transport for DOCSIS QAM channel
void erm_setup_docsis_qam_transport (rtsp_trans_t *trans, uint32 bitrate,
                                     uint16 tsid)
{
    trans->par_mask = 0;
    trans->profile_idx = RTSP_TRANS_PROFILE_IDX_DOCSIS_QAM;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_UNICAST;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_BITRATE;
    trans->bitrate = bitrate;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_DEPI_MODE;
    trans->depi_mode_idx = RTSP_TRANS_DEPI_MODE_IDX_MPT;   // hard-coded value

    trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_TSID;
    trans->tsid = tsid;
}


// Set up a transport for video QAM channel
void erm_setup_video_qam_transport (rtsp_trans_t *trans, char *service_grp,
                                    uint16 tsid, uint32 freq, uint16 prog_num)
{
    trans->par_mask = 0;
    trans->profile_idx = RTSP_TRANS_PROFILE_IDX_MP2T_QAM;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_NAME;
    trans->service_group = service_grp;
    trans->tsid = tsid;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_QAM_DST;
    trans->freq = freq;
    trans->prog_num = prog_num;
}


// Set up a transport for unicast video source
void erm_setup_ucast_transport (rtsp_trans_t *trans, uint32 dst_ip, 
                                uint16 dst_udp, uint32 bitrate)
{
    trans->par_mask = 0;
    trans->profile_idx = RTSP_TRANS_PROFILE_IDX_MP2T_UDP;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_UNICAST;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_DST;
    trans->dst_ip = dst_ip;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_DST_PORT;
    trans->dst_udp = dst_udp;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_BITRATE;
    trans->bitrate = bitrate;
}


// Set up a transport for multicast video source
void erm_setup_mcast_transport (rtsp_trans_t *trans, uint32 src_ip, 
                                uint32 grp_ip, uint32 bitrate, uint16 rank)
{
    trans->par_mask = 0;
    trans->profile_idx = RTSP_TRANS_PROFILE_IDX_MP2T_UDP;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_MULTICAST;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_SRC;
    trans->src_ip = src_ip;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_MULTICAST_ADDR;
    trans->mcast_ip = grp_ip;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_BITRATE;
    trans->bitrate = bitrate;

    trans->par_mask |= RTSP_TRANS_PAR_MASK_RANK;
    trans->rank = rank;
}


// Set up Keepalive
void erm_setup_keepalive (rtsp_msg_t *req, rtsp_session_t *ses)
{
    int len;
    char *msg_ptr = msg_buf;

    memset(&req, 0, sizeof(req));

    req->hdr_mask = RTSP_HDR_MASK_CSEQ;
    req->cseq = conn.tx_cseq++;

    req->hdr_mask = RTSP_HDR_MASK_REQUIRE;

    req->hdr_mask = RTSP_HDR_MASK_SESSION;
    req->ses_id = ses->ses_id;

    len = rtsp_put_request(&msg_ptr, req, RTSP_METHOD_SET_PARAMETER, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
}


int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


// CLI: add docsis num_qam bitrate tsid
int cli_setup_docsis_session (cli_control_block *ccb)
{
    int i;
    int len;
    char *msg_ptr = msg_buf;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;
    req.hdr_mask |= RTSP_HDR_MASK_TRANSPORT;

    // Set up transports
    for (i=0; i<ccb->var[2]; i++) { 
        erm_setup_docsis_qam_transport(&req.transport[i], ccb->var[3],
                                      ccb->var[4] + i);
    }

    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_SETUP, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


// CLI: add video unicast dst_ip dst_udp bitrate prog_num tsid freq
int cli_setup_ucast_video_session (cli_control_block *ccb)
{
    int len;
    char *msg_ptr = msg_buf;
    uint32 dst_ip;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    req.clab_cln_id = clab_client_id;
    req.clab_ses_id = clab_session_id;

    if (ccb->num_tokens > 9) {
        req.hdr_mask |= RTSP_HDR_MASK_CLAB_SESSION_GROUP;
        req.session_group = ccb->tokens[9];
    }

    if (!inet_pton(AF_INET, ccb->tokens[3], &dst_ip)) {
        perror("inet_pton");
        return 0;
    }

    // Set up transports
    req.hdr_mask |= RTSP_HDR_MASK_TRANSPORT;
    erm_setup_video_qam_transport(&req.transport[0], SERVICE_GROUP, ccb->var[7],
                                  ccb->var[8], ccb->var[6]);
    erm_setup_ucast_transport(&req.transport[1], dst_ip, ccb->var[4],
                              ccb->var[5]);

    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_SETUP, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


// CLI: add video multicast num_src src_ip grp_ip bitrate prog_num tsid freq
int cli_setup_mcast_video_session (cli_control_block *ccb)
{
    int len;
    char *msg_ptr = msg_buf;
    uint32 src_ip, grp_ip;
    int i;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    req.clab_cln_id = clab_client_id;
    req.clab_ses_id = clab_session_id;

    if (!inet_pton(AF_INET, ccb->tokens[4], &src_ip) ||
        !inet_pton(AF_INET, ccb->tokens[5], &grp_ip)) {
        perror("inet_pton");
        return 0;
    }

    if (ccb->num_tokens > 10) {
        req.hdr_mask |= RTSP_HDR_MASK_CLAB_SESSION_GROUP;
        req.session_group = ccb->tokens[10];
    }

    // Set up transports
    req.hdr_mask |= RTSP_HDR_MASK_TRANSPORT;
    erm_setup_video_qam_transport(&req.transport[0], SERVICE_GROUP, ccb->var[8],
                                  ccb->var[9], ccb->var[7]);
    for (i=0; i<ccb->var[3]; i++) {
        erm_setup_mcast_transport(&req.transport[1+i], htonl(ntohl(src_ip) + i),
                                  grp_ip, ccb->var[6], i);
    }

    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_SETUP, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


int cli_teardown_session (cli_control_block *ccb)
{
    int len;
    char *msg_ptr = msg_buf;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_REASON;
    req.reason_code = ccb->var[2];

    req.hdr_mask |= RTSP_HDR_MASK_SESSION;
    req.ses_id = ccb->tokens[1];

    req.hdr_mask |= RTSP_HDR_MASK_CLAB_CLIENT_SESSION_ID;
    req.clab_cln_id = clab_client_id;
    req.clab_ses_id = clab_session_id;

    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_TEARDOWN, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


int cli_get_param_session_list (cli_control_block *ccb)
{
    int len;
    char *msg_ptr;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
    req.content_type_idx = RTSP_CONTENT_TYPE_IDX_TEXT_PARAM;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
    msg_ptr = msg_body;
    req.content_length = put_format(&msg_ptr, "%s",
                                    RTSP_PARAM_CLAB_SESSION_LIST);
    req.msg_body = msg_body;

    if (ccb->num_tokens > 2) {
        req.hdr_mask |= RTSP_HDR_MASK_CLAB_SESSION_GROUP;
        req.session_group = ccb->tokens[2];
    }

    msg_ptr = msg_buf;
    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_GET_PARAMETER, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


int cli_get_param_connection_timeout (cli_control_block *ccb)
{
    int len;
    char *msg_ptr = msg_buf;

    cli_prepare_cc(ccb);

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
    req.content_type_idx = RTSP_CONTENT_TYPE_IDX_TEXT_PARAM;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
    msg_ptr = msg_body;
    req.content_length = put_format(&msg_ptr, "%s", 
                                    RTSP_PARAM_CLAB_CONNECTION_TIMEOUT);
    req.msg_body = msg_body;

    msg_ptr = msg_buf;
    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_GET_PARAMETER, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


int cli_set_param_session_group_list (cli_control_block *ccb)
{
    int len;
    char *msg_ptr = msg_buf;
    char *ptr;

    cli_prepare_cc(ccb);

    for (ptr=ccb->tokens[2]; *ptr; ptr++) {
        if (*ptr == ',')  *ptr = ' ';
    }

    memset(&req, 0, sizeof(req));

    req.hdr_mask = RTSP_HDR_MASK_CSEQ;
    req.cseq = conn.tx_cseq++;

    req.hdr_mask |= RTSP_HDR_MASK_REQUIRE;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_TYPE;
    req.content_type_idx = RTSP_CONTENT_TYPE_IDX_TEXT_PARAM;

    req.hdr_mask |= RTSP_HDR_MASK_CONTENT_LENGTH;
    msg_ptr = msg_body;
    req.content_length = put_format(&msg_ptr, "%s:%s",
                                    RTSP_PARAM_CLAB_SESSION_GROUP_LIST,
                                    ccb->tokens[2]);
    req.msg_body = msg_body;

    msg_ptr = msg_buf;
    len = rtsp_put_request(&msg_ptr, &req, RTSP_METHOD_SET_PARAMETER, URI);

    printf("\nC -> S:\n");
    printf("%s\n", msg_buf);
    io_send(conn.fd, msg_buf, len);
    return 0;
}


void *erm_server_main (void* arg)
{
    int sock_fd = (size_t)arg;
    char *msg_ptr = msg_buf;
    uint16 len;
    int rc;

    rtsp_conn_init(&conn);
    conn.fd = sock_fd;
    conn.connected = TRUE;
    conn.tx_cseq = CSEQ_START;

    while (conn.connected) {
        rc = rtsp_get_peer_msg(&conn, &len);

        if (rc == EOF || rc == ECONNRESET) {
            printf("ERM: EQAM disconnected\n");
            return (void*)0;
        }
        if (rc == E2BIG) {
            return (void*)0;
        }
        if (rc != EOK) {
            printf("rtsp_get_peer_msg: %s\n", strerror(rc));
            return (void*)0;
        }

        printf("\nS -> C:\n");
        printf("%s\n", conn.in_msg_buf);

        // Process request
        rc = RTSP_RESP_OK;
        msg_ptr = conn.in_msg_buf;
        rtsp_get_response(&msg_ptr, &rsp, &rc);
        printf("Result of response parsing: %d\n", rc);
    }

    printf("ERM server thread quitting...\n");
    return (void*)0;
}


int open_eqam (const char *eqam_ip_addr)
{
    int rc;
    int sock_fd;
    pthread_t server_tid;
    struct sockaddr_in addr;

    printf("Open EQAM connection:\n");

    // Set up destination socket address
    rc = sockaddr_setup(&addr, eqam_ip_addr, RTSP_TCP_PORT_DFT);
    if (rc == 0) {
        printf("Error: Bad EQAM IP address: %s\n", eqam_ip_addr);
        return 0;
    }
    if (rc < 0) {
        perror("open_eqam");
        return 0;
    }

    sock_fd = io_connect((struct sockaddr *)&addr);
    if (sock_fd == -1) {
        return -1;
    }

    printf("Connet successful\n");

    // Create a thread to do the server work
    rc = pthread_create(&server_tid, NULL, erm_server_main,
                        (void*)(size_t)sock_fd);
    if (rc != EOK) {
        printf("EQAM: Failed to create server thread\n");
        exit(-1);
    }

    return 0;
}


int main (int argc, char** argv)
{   
    int rc = RTSP_RESP_OK;

    open_eqam("127.0.0.1");

    // CLI loop
    cli_init();
    cli_set_prompt("ERM> ");
    cli_add_elems(cli_parser);
    rc = cli_main_loop();
    kill(0, SIGKILL);
    return rc;

    return 0;
}

