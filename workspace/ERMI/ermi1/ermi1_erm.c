///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      erm.c
///  @brief     ERM simulaiton for ERMI-1
///  @author    Yi Tong Tse


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
#include "errp.h"
#include "errp_fsm.h"
#include "rfgw_errp.h"
#include "cli.h"


#define ERM_LOG(fmt, str...)    \
      printf("ERM: "fmt"\n", ##str);
//    NULL


#define STREAMING_ZONE          "SanJose"
#define COMPONENT_NAME          "SanJose.ERM-Sim"
#define VENDOR_STRING           "Cisco"

#define HOLD_TIME               5

#define ADDR_DOMAIN             1
#define ERRP_ID                 1

#define LISTENQ                 1024
#define MAX_CONNS               4
#define MAX_ROUTES_PER_CONN     64

#define INVALID_TSID    0xFFFF


errp_node_t erm_node;
errp_node_t eqam_node[MAX_CONNS];
errp_conn_t eqam_conn[MAX_CONNS];
errp_resource_t eqam_resource[MAX_CONNS][MAX_ROUTES_PER_CONN];


/******** CLI Syntax ********
show erm
show eqam
show eqam <qam_id>
show eqam <qam_id> <route_id>
close <idx>
exit
*****************************/

// Prototypes
int cli_show_erm(cli_control_block *ccb);
int cli_show_eqam_list(cli_control_block *ccb);
int cli_show_eqam(cli_control_block *ccb);
int cli_show_eqam_route(cli_control_block *ccb);
int cli_close(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);


//// CLI Parser
CLI_ARR_START(cli_parser)
CLI_ARR_STR (0, show, CLI_NULL(), 0, "Show command")
CLI_ARR_STR (1, erm, CLI_FUN(cli_show_erm, 0), 0, "Show ERM node info")
CLI_ARR_STR (1, eqam, CLI_FUN(cli_show_eqam_list, 0), 0, "Show EQAM connection info")
CLI_ARR_NUM (2, 0, MAX_CONNS-1, CLI_FUN(cli_show_eqam, 0), 0, "EQAM connection id")
CLI_ARR_NUM (3, 0, MAX_ROUTES_PER_CONN-1, CLI_FUN(cli_show_eqam_route, 0),
             0, "Route id")
CLI_ARR_STR (0, close, CLI_FUN(cli_close, 0), 0, "Close EQAM connection")
CLI_ARR_STR (0, exit, CLI_FUN(cli_exit, 0), 0, "Exit")
CLI_ARR_END
//// End of CLI parser


int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


int cli_show_erm (cli_control_block *ccb)
{
    printf("ERM node info:\n");
    errp_node_print(&erm_node);
    return 0;
}


int cli_show_eqam_list (cli_control_block *ccb)
{
    int i;
    int eqam_cnt = 0;

    printf("EQAM connection info:\n");
    for (i=0; i<MAX_CONNS; i++) {
        if (eqam_conn[i].connected) {
            printf("\nConnection %d:\n", i);
            errp_conn_print(&eqam_conn[i]);
            eqam_cnt++;
        }
    }
    printf("\nTotal %d EQAM connections\n", eqam_cnt);
    return 0;
}


int cli_show_eqam (cli_control_block *ccb)
{
    int i;
    int idx = ccb->var[2];

    printf("EQAM connection %d info:\n", idx);
    if (!eqam_conn[idx].connected) {
        printf("Connection not used\n");
        return 0;
    }
    errp_conn_print(&eqam_conn[idx]);
    printf("\nPeer:\n");
    errp_node_print(eqam_conn[idx].peer);

    printf("\nRoutes:\n");
    for (i=0; i<MAX_ROUTES_PER_CONN; i++) {
        errp_resource_t* rsc = &eqam_resource[idx][i];
        if (rsc->attr_mask) {
            printf("%d: %s\n", i, rsc->reachable_route.addr);
        }
    }
    return 0;
}


int cli_show_eqam_route (cli_control_block *ccb)
{
    int eqam_idx = ccb->var[2];
    int rsc_idx = ccb->var[3];
    errp_resource_t* rsc = &eqam_resource[eqam_idx][rsc_idx];

    if (!eqam_conn[eqam_idx].connected) {
        printf("Connection not used\n");
        return 0;
    }
    if (!rsc->attr_mask) {
        printf("Route not used\n");
        return 0;
    }
    errp_resource_print(rsc);
    return 0;
}


int cli_close (cli_control_block *ccb)
{
    close(eqam_conn[0].fd);
    return 0;
}


// Dummy ERRP callback functions
//

boolean errp_check_cap_param (errp_conn_t *conn, errp_error_report_t *rpt)
{
/// @todo  How to get the list of attributes??

    // May set rpt->err_sub to:
    //   - ERRP_ERR_SUB_CAP_MISMATCH, or
    //   - ERRP_ERR_SUB_UNSUP_CAP
    //
    // Sample code:
    //     errp_set_err_mismatch_cap(rpt);
    //     for ( ; ; ) {
    //         errp_add_err_mismatch_cap(rpt, cap_code, cap_len, cap_value);
    //     }
    //     return FALSE;

    return TRUE;
}

boolean errp_is_addr_domain_valid (uint32 addr_domain)
{
    return TRUE;
}

boolean errp_is_errp_id_present (uint32 addr_domain, uint32 errp_id)
{
    return FALSE;
}


/// Find available connection
errp_conn_t* find_avail_conn (void)
{
    int i;
    for (i=0; i<MAX_CONNS; i++) {
        if (!eqam_conn[i].connected) {
            return &eqam_conn[i];
        }
    }
    return NULL;
}


/// Lookup resource by TSID
errp_resource_t* find_avail_resource (errp_resource_t *rsc)
{
    int i;
    for (i=0; i<MAX_ROUTES_PER_CONN; i++, rsc++) {
        if (!rsc->attr_mask) {
            return rsc;
        }
    }
    return NULL;
}


/// Add QAM resource
int qam_add (int conn_id, errp_resource_t *resource)
{
    // Find available resource
    errp_resource_t* rsc = find_avail_resource(eqam_resource[conn_id]);
    if (rsc == NULL) {
        printf("Resource table already full\n");
    } else {
        *rsc = *resource;
    }
    return 0;
}


/// Delete QAM resource
int qam_delete (int conn_id, errp_resource_t *resource)
{
    int i;
    errp_resource_t* rsc = eqam_resource[conn_id];

    // Find QAM resource with matching route address
    for (i=0; i<MAX_ROUTES_PER_CONN; i++, rsc++) {
        if (!strcmp(rsc->reachable_route.addr,
                    resource->withdrawn_route.addr)) {
            break;
        }
    }
    if (i == MAX_ROUTES_PER_CONN) {
        printf("Matching resource not found\n");
        return 0;
    }

    /// @todo  Should also verify other attributes
    rsc->attr_mask = 0;                 // mark the resource unavailable

    return 0;
}


/// Resource update
int resource_update (int conn_id, errp_resource_t *resource)
{
    if ((resource->attr_mask & ERRP_ATTR_MASK_REACHABLE_ROUTE)) {
        qam_add(conn_id, resource);

    } else if ((resource->attr_mask & ERRP_ATTR_MASK_WITHDRAWN_ROUTE)) {
        qam_delete(conn_id, resource);
    }
    return 0;
}


/// ERM Server main function (for single ERRP connection)
void* server_main (void* arg)
{
    int sock_fd = (size_t)arg;
    struct sockaddr_in cln_addr;
    socklen_t len = sizeof(cln_addr);
    errp_conn_t *conn;
    int conn_id;
    char addr_buf[INET_ADDRSTRLEN];
    uint8 msg_type;
    uint16 msg_len;
    parser_t psr;
    char err_rpt_buf[ERRP_MAX_ERR_LEN];
    errp_error_report_t* err_rpt = (errp_error_report_t*)err_rpt_buf;
    int rc;

    conn = find_avail_conn();
    if (!conn) {
        ERM_LOG("REJECT EQAM connection from %s : %d\n",
            inet_ntop(AF_INET, &cln_addr.sin_addr, addr_buf, sizeof(addr_buf)),
            ntohs(cln_addr.sin_port));
        close(sock_fd);
        return (void*)0;
    }

    conn_id = conn - eqam_conn;

    if (conn->fsm_state != S_IDLE) {
        printf("Err: ERRP connection is already used: %s (%d)\n",
               errp_fsm_state_txt(conn->fsm_state), conn->fsm_state);
        return (void*)0;
    }

    memset(eqam_resource[conn_id], 0, sizeof(eqam_resource[conn_id]));
    memset(err_rpt_buf, 0, sizeof(err_rpt_buf));

    conn->connected = TRUE;

    getpeername(sock_fd, (struct sockaddr *)&cln_addr, &len);
    ERM_LOG("Connection from %s : %d\n",
            inet_ntop(AF_INET, &cln_addr.sin_addr, addr_buf, sizeof(addr_buf)),
            ntohs(cln_addr.sin_port));

    errp_conn_init(conn, &erm_node, conn->peer);
    conn->fd = sock_fd;
    conn->fsm_state = S_CONNECT;

    // Trigger TCP connection opened event
    errp_event(conn, E_CONN_OPENED, NULL);

    psr.err = PARSER_OK;

    // Wait for incoming message
    while (conn->connected) {
        rc = errp_get_peer_msg(conn, &psr, &msg_type, &msg_len);
        if (rc == EOF || rc == ECONNRESET) {
            printf("ERM: EQAM disconnected\n");
            errp_event(conn, E_CONN_CLOSED, NULL);
            return (void*)0;
        }
        if (rc == E2BIG) {
            errp_set_err_bad_msg_len(err_rpt, msg_len);
            errp_event(conn, E_MSG_ERROR, err_rpt);
            return (void*)0;
        }
        if (rc != EOK) {
            printf("errp_get_peer_msg: %s\n", strerror(rc));
            err_rpt->err = ERRP_ERR_MSG_HEADER;
            err_rpt->err_sub = ERRP_ERR_SUB_BAD_MSG_LEN;
            errp_event(conn, E_MSG_ERROR, err_rpt);
            return (void*)0;
        }

        ERM_LOG("Recv msg: type %d, len %d\n", msg_type, msg_len);

        switch (msg_type) {

        case ERRP_OPEN:
            errp_get_open_msg(&psr, conn, err_rpt);
            if (err_rpt->err == ERRP_OK) {
                // Client message handling code here (report error thru err_rpt)
                // ...
            }

            if (err_rpt->err == ERRP_OK) {
                errp_event(conn, E_OPEN_RCVD, NULL);
            }
            break;

        case ERRP_UPDATE:
        {
            errp_resource_t tmp_rsc;
            errp_get_update_msg(&psr, &tmp_rsc, err_rpt);

            if (err_rpt->err == ERRP_OK) {
                // Client message handling code here (report error thru err_rpt)
                resource_update(conn_id, &tmp_rsc);
            }

            if (err_rpt->err == ERRP_OK) {
                errp_event(conn, E_UPDATE_RCVD, NULL);
            }
            break;
        }

        case ERRP_NOTIFICATION:
            errp_get_notification_msg(&psr, err_rpt);
            if (psr.err == ERRP_OK) {
                // Client message handling code here
                printf("NOTIFICATION received:\n");
                errp_error_report_print(err_rpt);
                err_rpt->err = ERRP_OK;          // reset error code

                errp_event(conn, E_NOTIF_RCVD, NULL);
            }
            break;

        case ERRP_KEEPALIVE:
            errp_get_keepalive_msg(&psr);
            if (psr.err == ERRP_OK) {
                errp_event(conn, E_KEEPALIVE_RCVD, NULL);
            }
            break;

        default:
            errp_set_err_bad_msg_type(err_rpt, msg_type);
            printf("ERM: Unknown msg received: type %d\n", msg_type);
        }

        if (err_rpt->err != ERRP_OK) {
            printf("Message handling error:\n");
            errp_error_report_print(err_rpt);
            /// @todo  Other resource clean up needed??

            errp_event(conn, E_MSG_ERROR, err_rpt);
        }
    }

    return (void*)0;
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
    srv_addr.sin_port = htons(ERRP_TCP_PORT_DFT);

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
    int i;
    int rc;
    pthread_t listen_tid;

    // Setup EQAM nodes, connections, and resources
    memset(eqam_node, 0, sizeof(eqam_node));
    memset(eqam_conn, 0, sizeof(eqam_conn));
    for (i=0; i<MAX_CONNS; i++) {
        eqam_conn[i].fsm_state = S_IDLE;
        eqam_conn[i].peer = &eqam_node[i];
    }

    // Setup ERM node
    errp_node_init(&erm_node, HOLD_TIME, ADDR_DOMAIN, ERRP_ID);
    erm_node.send_receive = ERRP_SEND_AND_RECEIVE;
    strcpy(erm_node.streaming_zone, STREAMING_ZONE);
    strcpy(erm_node.component_name, COMPONENT_NAME);
    strcpy(erm_node.vendor_string, VENDOR_STRING);

    // Create a thread to listen for connection
    rc = pthread_create(&listen_tid, NULL, listen_main, NULL);
    if (rc != EOK) {
        printf("Failed to create listen thread\n");
        exit(-1);
    }

    // CLI loop
    cli_init();
    cli_set_prompt("ERM> ");
    cli_add_elems(cli_parser);
    rc = cli_main_loop();
    kill(0, SIGKILL);
    return rc;
}
