///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      eqam.c
///  @brief     EQAM simulaiton for ERMI-1
///  @author    Yi Tong Tse


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "errp.h"
#include "errp_fsm.h"
#include "cli.h"


#define EQAM_LOG(fmt, str...)    \
    printf("EQAM: "fmt"\n", ##str);
//    NULL


#define STREAMING_ZONE  "SanJose"
#define COMPONENT_NAME  "SanJose.RFGW-10-Sim"
#define VENDOR_STRING   "Cisco"


#define HOLD_TIME       10

#define ADDR_DOMAIN     0x12345678
#define ERRP_ID         2

#define ERM_IP_ADDR     "127.0.0.1"
#define MAX_QAMS        64
#define INVALID_TSID    0xFFFF


errp_node_t eqam_node;          ///< EQAM node info
errp_node_t erm_node;           ///< ERM node info
errp_conn_t erm_conn;           ///< ERM connection info
errp_resource_t erm_resource;   ///< ERM resource received

errp_resource_t resource_template;              ///< EQAM resource template
errp_resource_t eqam_resource[MAX_QAMS];        ///< EQAM resources


#if 0
#define NUM_ROUTES              1
errp_route_t route[NUM_ROUTES] =
    { ERRP_ADDR_FAMILY, ERRP_PROT_DYNAMIC, "rtsp://192.16.0.1/" };
#endif


errp_qam_cfg_t qam_cfg;
errp_qam_cap_t qam_cap;


/******** CLI Syntax ********
show eqam
show erm
open erm {<ip_addr>}
close erm

set qam config annex <code>
set qam config bandwidth <code>
set qam config interleaver <code>
set qam config modulation <code>

send udp-map

show qam parameter

add qam <tsid> <freq>
delete qam <tsid>

show qam
show qam <tsid>

exit
*****************************/

// Prototypes
int cli_set_qam_cfg_annex(cli_control_block *ccb);
int cli_set_qam_cfg_bw(cli_control_block *ccb);
int cli_set_qam_cfg_fec(cli_control_block *ccb);
int cli_set_qam_cfg_mod(cli_control_block *ccb);

int cli_show_eqam(cli_control_block *ccb);
int cli_show_erm(cli_control_block *ccb);
int cli_show_qam_param(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);
int cli_open_erm(cli_control_block *ccb);
int cli_close_erm(cli_control_block *ccb);
int cli_send_edge_input(cli_control_block *ccb);
int cli_add_qam(cli_control_block *ccb);
int cli_delete_qam(cli_control_block *ccb);

//// CLI Parser
CLI_ARR_START(cli_parser)
CLI_ARR_STR (0, set, CLI_NULL(), 0, "Set commands")
CLI_ARR_STR (1, qam, CLI_NULL(), 0, "Set QAM commands")
CLI_ARR_STR (2, config, CLI_NULL(), 0, "Set QAM config commands")

CLI_ARR_STR (3, annex, CLI_NULL(), 0, "Set QAM Annex config")
CLI_ARR_STR (4, A, CLI_FUN(cli_set_qam_cfg_annex, ERRP_QAM_CFG_ANNEX_A), 0,
             "Annex A")
CLI_ARR_STR (4, B, CLI_FUN(cli_set_qam_cfg_annex, ERRP_QAM_CFG_ANNEX_B), 0,
             "Annex B")
CLI_ARR_STR (4, C, CLI_FUN(cli_set_qam_cfg_annex, ERRP_QAM_CFG_ANNEX_C), 0,
             "Annex C")

CLI_ARR_STR (3, bandwidth, CLI_NULL(), 0, "Set QAM channel bandwidth config")
CLI_ARR_STR (4, 6, CLI_FUN(cli_set_qam_cfg_bw, ERRP_QAM_CFG_CHAN_BW_6MHZ), 0,
             "6 MHz")
CLI_ARR_STR (4, 7, CLI_FUN(cli_set_qam_cfg_bw, ERRP_QAM_CFG_CHAN_BW_7MHZ), 0,
             "7 MHz")
CLI_ARR_STR (4, 8, CLI_FUN(cli_set_qam_cfg_bw, ERRP_QAM_CFG_CHAN_BW_8MHZ), 0,
             "8 MHz")

CLI_ARR_STR (3, interleaver, CLI_NULL(), 0, "Set QAM interleaver config")
CLI_ARR_STR (4, I8J16, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I8_J16),
             0, "I=8, J=16")
CLI_ARR_STR (4, I16J8, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I16_J8),
             0, "I=16, J=8")
CLI_ARR_STR (4, I32J4, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I32_J4),
             0, "I=32, J=4")
CLI_ARR_STR (4, I64J2, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I64_J2),
             0, "I=64, J=2")
CLI_ARR_STR (4, I128J1, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J1),
             0, "I=128, J=1")
CLI_ARR_STR (4, I12J7, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I12_J7),
             0, "I=12, J=7")
CLI_ARR_STR (4, I128J2, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J2),
             0, "I=128, J=2")
CLI_ARR_STR (4, I128J3, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J3),
             0, "I=128, J=3")
CLI_ARR_STR (4, I128J4, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J4),
             0, "I=128, J=4")
CLI_ARR_STR (4, I128J5, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J5),
             0, "I=128, J=5")
CLI_ARR_STR (4, I128J6, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J6),
             0, "I=128, J=6")
CLI_ARR_STR (4, I128J7, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J7),
             0, "I=128, J=7")
CLI_ARR_STR (4, I128J8, CLI_FUN(cli_set_qam_cfg_fec, ERRP_QAM_CFG_FEC_I128_J8),
             0, "I=128, J=8")

CLI_ARR_STR (3, modulation, CLI_NULL(), 0, "Set QAM modulation config")
CLI_ARR_STR (4, 64, CLI_FUN(cli_set_qam_cfg_mod, ERRP_QAM_CFG_MODULATION_64_QAM),
             0, "64-QAM")
CLI_ARR_STR (4, 256, CLI_FUN(cli_set_qam_cfg_mod, ERRP_QAM_CFG_MODULATION_256_QAM),
             0, "256-QAM")
CLI_ARR_STR (4, 128, CLI_FUN(cli_set_qam_cfg_mod, ERRP_QAM_CFG_MODULATION_128_QAM),
             0, "128-QAM")
CLI_ARR_STR (4, 512, CLI_FUN(cli_set_qam_cfg_mod, ERRP_QAM_CFG_MODULATION_512_QAM),
             0, "512-QAM")
CLI_ARR_STR (4, 1024, CLI_FUN(cli_set_qam_cfg_mod, ERRP_QAM_CFG_MODULATION_1024_QAM),
             0, "1024-QAM")

CLI_ARR_STR (0, show, CLI_NULL(), 0, "Show command")
CLI_ARR_STR (1, erm, CLI_FUN(cli_show_erm, 0), 0, "Show ERM connection info")
CLI_ARR_STR (1, eqam, CLI_FUN(cli_show_eqam, 0), 0, "Show EQAM node info")
CLI_ARR_STR (1, qam, CLI_NULL(), 0, "Show qam")
CLI_ARR_STR (2, param, CLI_FUN(cli_show_qam_param, 0), 0, "Show qam parameters")

CLI_ARR_STR (0, open, CLI_NULL(), 0, "Open command")
CLI_ARR_STR (1, erm, CLI_NULL(), 0, "Open connection to ERM")
CLI_ARR_GSTR (2, CLI_FUN(cli_open_erm, 0), 0, "ERM IP address")

CLI_ARR_STR (0, close, CLI_NULL(), 0, "Close command")
CLI_ARR_STR (1, erm, CLI_FUN(cli_close_erm, 0), 0, "Close connection to ERM")

CLI_ARR_STR (0, send, CLI_NULL(), 0, "Send command")
CLI_ARR_STR (1, udp-map, CLI_FUN(cli_send_edge_input, 0), 0, "Send Edge Input")

CLI_ARR_STR (0, add, CLI_NULL(), 0, "Add command")
CLI_ARR_STR (1, qam, CLI_NULL(), 0, "Add QAM resource")
CLI_ARR_NUM (2, 0, 8191, CLI_NULL(), 0, "TSID")
CLI_ARR_NUM (3, 1, 1000000000, CLI_FUN(cli_add_qam, 0), 0, "Frequency in Hz")

CLI_ARR_STR (0, delete, CLI_NULL(), 0, "Delete command")
CLI_ARR_STR (1, qam, CLI_NULL(), 0, "Delete QAM resource")
CLI_ARR_NUM (2, 0, 65535, CLI_FUN(cli_delete_qam, 0), 0, "TSID")

CLI_ARR_STR (0, exit, CLI_FUN(cli_exit, 0), 0, "Exit")
CLI_ARR_END
//// End of CLI parser


// Prototypes
int open_erm(const char *ip_addr, errp_conn_t *eqam_conn, errp_node_t *erm_node,
             errp_node_t *eqam_node);
void* eqam_server_main(void* arg);
errp_resource_t* lookup_resource(errp_resource_t *rsc, uint16 tsid);


int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


/// Set QAM config of Annex
int cli_set_qam_cfg_annex (cli_control_block *ccb)
{
    resource_template.qam_cfg.annex = ccb->fooid;
    return 0;
}

/// Set QAM config of Channel Bandwidth
int cli_set_qam_cfg_bw (cli_control_block *ccb)
{
    resource_template.qam_cfg.chan_bw = ccb->fooid;
    return 0;
}

/// Set QAM config of Interleaver
int cli_set_qam_cfg_fec (cli_control_block *ccb)
{
    resource_template.qam_cfg.interleaver = ccb->fooid;
    return 0;
}

/// Set QAM config of Modulation
int cli_set_qam_cfg_mod (cli_control_block *ccb)
{
    resource_template.qam_cfg.modulation = ccb->fooid;
    return 0;
}


int cli_show_erm (cli_control_block *ccb)
{
    if (!erm_conn.connected) {
        printf("The ERM connection is not up\n");
        return 0;
    }
    printf("ERM connection info:\n");
    errp_conn_print(&erm_conn);
    printf("\nERM node info:\n");
    errp_node_print(&erm_node);
    return 0;
}


int cli_show_eqam (cli_control_block *ccb)
{
    printf("EQAM node info:\n");
    errp_node_print(&eqam_node);
    return 0;
}


int cli_show_qam_param (cli_control_block *ccb)
{
    printf("QAM parameters:\n");
    errp_resource_print(&resource_template);
    return 0;
}


int cli_open_erm (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    return open_erm(ccb->tokens[2], &erm_conn, &eqam_node, &erm_node);
}


int cli_close_erm (cli_control_block *ccb)
{
    errp_error_report_t err_rpt;

    printf("Close ERM connection:\n");

    if (erm_conn.fsm_state == S_IDLE) {
        printf("Error: ERM connection is currently IDLE\n");
        return 0;
    }

    // Set up error report
    err_rpt.err = ERRP_ERR_CEASE;
    err_rpt.err_sub = ERRP_ERR_SUB_UNSPECIFIED;

    errp_event(&erm_conn, E_STOP, &err_rpt);
    return 0;
}


int cli_send_edge_input (cli_control_block *ccb)
{
    errp_resource_t rsc;

    memset(&rsc, 0, sizeof(rsc));

    rsc.attr_mask |= ERRP_ATTR_MASK_EDGE_INPUT;
    rsc.edge_input[0].if_id = 0;
    rsc.edge_input[0].subnet_mask = 0xFFFFFF00;
    rsc.edge_input[0].max_bandwidth = 1000000000;
    sprintf(rsc.edge_input[0].host_name, "10.0.0.1");
    sprintf(rsc.edge_input[0].group_name, "SanJose");

    errp_event(&erm_conn, E_SEND_UPDATE, &rsc);
    return 0;
}


int cli_add_qam (cli_control_block *ccb)
{
    errp_resource_t* rsc;
    printf("Add QAM:\n");

    // Find available QAM resource
    rsc = lookup_resource(eqam_resource, INVALID_TSID);
    if (!rsc) {
        printf("All QAM resources are used up\n");
        return 0;
    }

    *rsc = resource_template;
    rsc->qam_cfg.tsid = ccb->var[2];
    rsc->qam_cfg.freq = ccb->var[3];

    // QAM name: <component_name>.<tsid>
    sprintf(rsc->qam_name, "%s.%d", COMPONENT_NAME, rsc->qam_cfg.tsid);

    // Route name: <streaming_zone>.ERM1.<tsid>
    rsc->attr_mask |= ERRP_ATTR_MASK_REACHABLE_ROUTE;
    rsc->reachable_route.type.addr_family = ERRP_ADDR_FAMILY;
    rsc->reachable_route.type.app_protocol = ERRP_PROT_RTSP_DYNAMIC;
    sprintf(rsc->reachable_route.addr, "%s.%d",
            COMPONENT_NAME, rsc->qam_cfg.tsid);

    errp_event(&erm_conn, E_SEND_UPDATE, rsc);
    return 0;
}


int cli_delete_qam (cli_control_block *ccb)
{
    errp_resource_t *rsc;
    int tsid = ccb->var[2];
    printf("Delete QAM: TSID %d\n", tsid);

    // Find matching QAM resource
    rsc = lookup_resource(eqam_resource, INVALID_TSID);
    if (!rsc) {
        printf("Matching QAM resource not found\n");
        return 0;
    }

    *rsc = resource_template;
    rsc->attr_mask = ERRP_ATTR_MASK_WITHDRAWN_ROUTE
                     | ERRP_ATTR_MASK_REQUIRED_WITHDRAWN;
    rsc->withdrawn_route.type.addr_family = ERRP_ADDR_FAMILY;
    rsc->withdrawn_route.type.app_protocol = ERRP_PROT_RTSP_DYNAMIC;
    sprintf(rsc->withdrawn_route.addr, "%s.%d", COMPONENT_NAME, tsid);

    errp_event(&erm_conn, E_SEND_UPDATE, rsc);

    rsc->qam_cfg.tsid = INVALID_TSID;    // mark the qam unused
    return 0;
}


// Dummy ERRP callback functions
//

boolean errp_check_cap_param (errp_conn_t *conn, errp_error_report_t *rpt)
{
    // May set rpt->err_sub to:
    //   - ERRP_ERR_SUB_CAP_MISMATCH, or
    //   - ERRP_ERR_SUB_UNSUP_CAP
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


/// Find QAM with matching TSID
errp_resource_t* lookup_resource (errp_resource_t *rsc, uint16 tsid)
{
    int i;
    for (i=0; i<MAX_QAMS; i++, rsc++) {
        if (rsc->qam_cfg.tsid == tsid) {
            return rsc;
        }
    }
    return NULL;
}


int sockaddr_setup(struct sockaddr_in *addr, const char* ip_addr, uint16 port)
{
    bzero(addr, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    return inet_pton(AF_INET, ip_addr, &addr->sin_addr);
}


int open_erm (const char *erm_ip_addr, errp_conn_t *erm_conn,
              errp_node_t *eqam_node, errp_node_t *erm_node)
{
    int rc;
    pthread_t server_tid;

    printf("Open ERM connection:\n");
    if (erm_conn->fsm_state != S_IDLE) {
        printf("Error: ERM connection is currently not IDLE: %d\n",
               erm_conn->fsm_state);
        return 0;
    }

    // Set up destination socket address
    rc = sockaddr_setup(&erm_conn->addr, erm_ip_addr, ERRP_TCP_PORT_DFT);
    if (rc == 0) {
        printf("Error: Bad ERM IP address: %s\n", erm_ip_addr);
        return 0;
    }
    if (rc < 0) {
        perror("open_erm");
        return 0;
    }

    errp_conn_init(erm_conn, eqam_node, erm_node);

    // Create a thread to do the server work
    rc = pthread_create(&server_tid, NULL, eqam_server_main, (void*)erm_conn);
    if (rc != EOK) {
        printf("EQAM: Failed to create server thread\n");
        exit(-1);
    }

    return 0;
}


void *eqam_server_main (void* arg)
{
    int rc = ERRP_OK;
    uint8 msg_type;
    uint16 msg_len;
    errp_conn_t* conn = (errp_conn_t*)arg;
    parser_t psr;
    char err_rpt_buf[ERRP_MAX_ERR_LEN];
    errp_error_report_t* err_rpt = (errp_error_report_t*)err_rpt_buf;

    memset(err_rpt_buf, 0, sizeof(err_rpt_buf));

    // Trigger Start event for EQAM
    errp_event(conn, E_START, NULL);

    while (1) {
        sema_wait(&conn->conn_sema);

        if (conn->connected) {
            break;
        }
        errp_event(conn, E_CONN_OPEN_FAILED, NULL);
    }

    // Trigger the connection opened event
    errp_event(conn, E_CONN_OPENED, NULL);

    EQAM_LOG("Entering message loop...\n");

    psr.err = ERRP_OK;

    // Wait for incoming message
    while (conn->connected) {
        rc = errp_get_peer_msg(conn, &psr, &msg_type, &msg_len);
        if (rc == EOF) {
            printf("EQAM: ERM disconnected\n");
            errp_event(conn, E_CONN_CLOSED, NULL);
            return (void*)0;
        }
        if (rc == E2BIG) {
            printf("EQAM: Peer message too big: %d bytes\n", msg_len);
            err_rpt->err = ERRP_ERR_MSG_HEADER;
            err_rpt->err_sub = ERRP_ERR_SUB_BAD_MSG_LEN;
            errp_event(conn, E_MSG_ERROR, err_rpt);
            return (void*)0;
        }
        if (rc != EOK) {
            printf("errp_get_peer_msg: %s\n", strerror(rc));
            err_rpt->err = ERRP_ERR_MSG_HEADER;
            err_rpt->err_sub = ERRP_ERR_SUB_UNSPECIFIED;
            errp_event(conn, E_MSG_ERROR, err_rpt);
            return (void*)0;
        }

        EQAM_LOG("Recv msg: type %d, len %d\n", msg_type, msg_len);

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
            errp_get_update_msg(&psr, &erm_resource, err_rpt);

            if (err_rpt->err == ERRP_OK) {
                // Client message handling code here (report error thru err_rpt)
                // ...
            }

            if (err_rpt->err == ERRP_OK) {
                errp_event(conn, E_UPDATE_RCVD, NULL);
            }
            break;

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
            /// @todo  Need to copy psr.err to err_rpt->err!!
            break;

        default:
            err_rpt->err = ERRP_ERR_MSG_HEADER;
            err_rpt->err_sub = ERRP_ERR_SUB_BAD_MSG_TYPE;
            printf("EQAM: Unknown msg received: type %d\n", msg_type);
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


/// Set default QAM parameters
void set_default_qam_params (errp_resource_t *rsc)
{
    memset(rsc, 0, sizeof(*rsc));

//    rsc->attr_mask |= ERRP_ATTR_MASK_WITHDRAWN_ROUTE;

//    rsc->attr_mask |= ERRP_ATTR_MASK_REACHABLE_ROUTE;

    rsc->attr_mask |= ERRP_ATTR_MASK_NEXT_HOP_SERVER;
    rsc->next_hop_server.addr_domain = 1;
    strcpy(rsc->next_hop_server.component_addr, "comp_addr");
    strcpy(rsc->next_hop_server.streaming_zone, "192.0.2.2");

    rsc->attr_mask |= ERRP_ATTR_MASK_QAM_NAMES;
    strcpy(rsc->qam_name, "QAM 3/1.1");

    rsc->attr_mask |= ERRP_ATTR_MASK_CAS_CAP;
    rsc->cas_cap.enc_type = ERRP_CAS_ENC_TYPE_SESSION;
    rsc->cas_cap.enc_scheme = ERRP_CAS_ENC_SCHEME_PKEY;
    rsc->cas_cap.key_len = 24;
    rsc->cas_cap.cas_id = 1234;

    rsc->attr_mask |= ERRP_ATTR_MASK_TOTAL_BANDWIDTH;
    rsc->total_bandwidth = 38800000;

    rsc->attr_mask |= ERRP_ATTR_MASK_AVAIL_BANDWIDTH;
    rsc->avail_bandwidth = 38800000;

    rsc->attr_mask |= ERRP_ATTR_MASK_MAX_MPEG_FLOWS;
    rsc->max_flows = 30;

    rsc->attr_mask |= ERRP_ATTR_MASK_COST;
    rsc->cost = 10;

//    rsc->attr_mask |= ERRP_ATTR_MASK_EDGE_INPUT;

    rsc->attr_mask |= ERRP_ATTR_MASK_QAM_CFG;
    rsc->qam_cfg.annex = ERRP_QAM_CFG_ANNEX_B;
    rsc->qam_cfg.chan_bw = ERRP_QAM_CFG_CHAN_BW_6MHZ;
    rsc->qam_cfg.modulation = ERRP_QAM_CFG_MODULATION_256_QAM;
    rsc->qam_cfg.interleaver = ERRP_QAM_CFG_FEC_I8_J16;

    rsc->attr_mask |= ERRP_ATTR_MASK_UDP_MAP;
    rsc->udp_map_cnt = 2;
    rsc->udp_map[0].type = ERRP_UDP_MAP_TYPE_STATIC_RANGE;
    rsc->udp_map[0].static_range.start_udp_port = 49152;
    rsc->udp_map[0].static_range.start_prog_num = 2;
    rsc->udp_map[0].static_range.count = 20;
    rsc->udp_map[1].type = ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE;
    rsc->udp_map[1].dynamic_range.start_udp_port = 5000;
    rsc->udp_map[1].dynamic_range.count = 20;

    rsc->attr_mask |= ERRP_ATTR_MASK_SERVICE_STATUS;
    rsc->service_status = ERRP_SERVICE_STATUS_OPERATIONAL;

    rsc->attr_mask |= ERRP_ATTR_MASK_MAX_MPEG_FLOWS;
    rsc->max_flows = 20;

    rsc->attr_mask |= ERRP_ATTR_MASK_NEXT_HOP_ALT;
    rsc->next_hop_server_alt[0].addr_domain = 2;
    strcpy(rsc->next_hop_server_alt[0].component_addr, "comp_addr2");
    strcpy(rsc->next_hop_server_alt[0].streaming_zone, "192.0.2.3");
    rsc->alt_server_cnt = 1;

    rsc->attr_mask |= ERRP_ATTR_MASK_PORT_ID;
    rsc->port_id = 1;

    rsc->attr_mask |= ERRP_ATTR_MASK_FIBER_NODE;
    rsc->fiber_node_cnt = 2;
    strncpy(rsc->fiber_node[0], "SanJoseA", ERRP_MAX_NAME_LEN);
    strncpy(rsc->fiber_node[1], "SanJoseB", ERRP_MAX_NAME_LEN);

    rsc->attr_mask |= ERRP_ATTR_MASK_QAM_CAP;
    rsc->qam_cap.chan_bw._6mhz = 1;
    rsc->qam_cap.chan_bw._8mhz = 1;
    rsc->qam_cap.j83.annex_A = 1;
    rsc->qam_cap.j83.annex_B = 1;
    rsc->qam_cap.j83.annex_C = 1;
    rsc->qam_cap.interleaver.i12_j7 = 1;
    rsc->qam_cap.interleaver.i128_j8 = 1;
    rsc->qam_cap.docsis_video.docsis_MPT = 1;
    rsc->qam_cap.docsis_video.video = 1;
    rsc->qam_cap.modulation._64_qam = 1;
    rsc->qam_cap.modulation._256_qam = 1;

    rsc->attr_mask |= ERRP_ATTR_MASK_INPUT_MAP;
    rsc->input_cnt = 2;
    strncpy(rsc->input_map[0], "10.0.0.1", ERRP_MAX_NAME_LEN);
    strncpy(rsc->input_map[1], "10.0.0.2", ERRP_MAX_NAME_LEN);
}


int main (int argc, char** argv)
{
    int i;
    int rc;

    // Setup EQAM node
    errp_node_init(&eqam_node, HOLD_TIME, ADDR_DOMAIN, ERRP_ID);
    eqam_node.send_receive = ERRP_SEND_ONLY;
    strcpy(eqam_node.streaming_zone, STREAMING_ZONE);
    strcpy(eqam_node.component_name, COMPONENT_NAME);
    strcpy(eqam_node.vendor_string, VENDOR_STRING);

    set_default_qam_params(&resource_template);

    // Initialize QAM resources
    memset(eqam_resource, 0, sizeof(eqam_resource));
    for (i=0; i<MAX_QAMS; i++) {
        eqam_resource[i].qam_cfg.tsid = INVALID_TSID;
    }

    erm_conn.fsm_state = S_IDLE;

    // Connect with ERM
    open_erm(ERM_IP_ADDR, &erm_conn, &eqam_node, &erm_node);

    // CLI loop
    cli_init();
    cli_set_prompt("EQAM> ");
    cli_add_elems(cli_parser);
    rc = cli_main_loop();
    kill(0, SIGKILL);
    return rc;
}

