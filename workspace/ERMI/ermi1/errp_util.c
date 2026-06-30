///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp_util.c
///  @brief     ERRP (ERMI-1) utilities
///  @author    Yi Tong Tse


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"
#include "util.h"
#include "errp.h"
#include "errp_fsm.h"


/// Initialize an ERRP node
void errp_node_init (errp_node_t *node,
                     uint16 hold_time, uint32 addr_domain, uint32 errp_id)
{
    memset(node, 0, sizeof(errp_node_t));

    node->version = ERRP_PROTOCOL_VERSION;
    errp_set_route_type(&node->route_type[node->route_type_cnt++],
                        ERRP_ADDR_FAMILY, ERRP_PROT_RTSP_DYNAMIC);
    node->errp_version = ERRP_VERSION;

    node->hold_time = hold_time;
    node->addr_domain = addr_domain;
    node->errp_id = errp_id;
}


/// Print ERRP node info
void errp_node_print (const errp_node_t *node)
{
    int i;

    assert(node);

    printf("  Version: %d\n", node->version);
    printf("  Hold time: %d\n", node->hold_time);
    printf("  Address domain: %d\n", node->addr_domain);
    printf("  ERRP ID: %d\n", node->errp_id);

    if (strlen(node->streaming_zone)) {
        printf("  Streaming zone: %s\n", node->streaming_zone);
    }
    if (strlen(node->component_name)) {
        printf("  Component name: %s\n", node->component_name);
    }
    if (strlen(node->vendor_string)) {
        printf("  Vendor string: %s\n", node->vendor_string);
    }

    printf("  Capabilities:\n");
    for (i=0; i<node->route_type_cnt; i++) {
        const errp_route_type_t *route_type = &node->route_type[i];
        printf("    Route type: Address family %d, App protocol %d\n",
               route_type->addr_family, route_type->app_protocol);
    }
    printf("    Send receive cap: %d\n", node->send_receive);
    printf("    ERRP version: %d\n", node->errp_version);

}


/// Initialize an ERRP connection
void errp_conn_init (errp_conn_t *conn, errp_node_t *node, errp_node_t *peer)
{
    int rc;
    struct sigevent se;

    conn->node = node;
    conn->peer = peer;
    conn->fsm_state = S_IDLE;

    // Set up default time
    conn->connect_time = ERRP_CONNECT_TIME_DFT;
    conn->keepalive_time = ERRP_KEEPALIVE_TIME_DFT;
    conn->hold_time = ERRP_HOLD_TIME_DFT;
    
    // Set up timer event
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_attributes = NULL;
    se.sigev_value.sival_ptr = conn;

    // Create connect timers
    se.sigev_notify_function = errp_connect_timer_event_handler;
    rc = timer_create(CLOCK_REALTIME, &se, &conn->connect_timer_id);
    assert(rc == EOK);

    // Create keepalive timers
    se.sigev_notify_function = errp_keepalive_timer_event_handler;
    rc = timer_create(CLOCK_REALTIME, &se, &conn->keepalive_timer_id);
    assert(rc == EOK);

    // Create hold timers
    se.sigev_notify_function = errp_hold_timer_event_handler;
    rc = timer_create(CLOCK_REALTIME, &se, &conn->hold_timer_id);
    assert(rc == EOK);

    // Create semaphore
    sem_init(&conn->conn_sema, 0, 0);   // shared between threads within process

    // Allocate incoming message buffer
    conn->avail_len = 0;
    conn->in_msg_offset = 0;
    conn->in_msg_buf = malloc(ERRP_MAX_MSG_BYTES);
    assert(conn->in_msg_buf);
}


/// Print ERRP connection info
void errp_conn_print (const errp_conn_t *conn)
{
    struct sockaddr_in cln_addr;
    socklen_t len = sizeof(cln_addr);
    char buf[INET_ADDRSTRLEN];

    assert(conn);

    getsockname(conn->fd, (struct sockaddr *)&cln_addr, &len);
    printf("  Local %s : %d\n",
           inet_ntop(AF_INET, &cln_addr.sin_addr, buf, sizeof(buf)),
           ntohs(cln_addr.sin_port));
    getpeername(conn->fd, (struct sockaddr *)&cln_addr, &len);
    printf("  Remote %s : %d\n",
           inet_ntop(AF_INET, &cln_addr.sin_addr, buf, sizeof(buf)),
           ntohs(cln_addr.sin_port));
    printf("  Timers: Hold %d, Keepalive %d, ConnectRetry %d\n",
           conn->hold_time, conn->keepalive_time, conn->connect_time);
    printf("  FSM state: %s\n", errp_fsm_state_txt(conn->fsm_state));
}


/// Print ERRP Route info
void errp_route_print (const errp_route_t *route)
{
    printf("  Family %d, Prot %d, Addr %s\n",
           route->type.addr_family, route->type.app_protocol, route->addr);
}


/// Print ERRP Server info
void errp_server_print (const errp_server_t *server)
{
    printf("  Domain %d, Comp %s, Zone %s\n",
           server->addr_domain, server->component_addr, server->streaming_zone);
}


/// Get QAM Modulation configuration label
char* errp_qam_cfg_modulation_txt (uint32 mod)
{
    switch (mod) {
    case ERRP_QAM_CFG_MODULATION_64_QAM:   return("64-QAM");
    case ERRP_QAM_CFG_MODULATION_256_QAM:  return("256-QAM");
    case ERRP_QAM_CFG_MODULATION_128_QAM:  return("128-QAM");
    case ERRP_QAM_CFG_MODULATION_512_QAM:  return("512-QAM");
    case ERRP_QAM_CFG_MODULATION_1024_QAM: return("1024-QAM");
    default: printf("[Value %d ", mod);                                  return("UNKNOWN");
    }
}


/// Get QAM Interleaver configuration label
char* errp_qam_cfg_interleaver_txt (uint32 fec)
{
    switch (fec) {
    case ERRP_QAM_CFG_FEC_I8_J16:  return("i=8, j=16");
    case ERRP_QAM_CFG_FEC_I16_J8:  return("i=16, j=8");
    case ERRP_QAM_CFG_FEC_I32_J4:  return("i=32, j=4");
    case ERRP_QAM_CFG_FEC_I64_J2:  return("i=64, j=2");
    case ERRP_QAM_CFG_FEC_I128_J1: return("i=128, j=1");
    case ERRP_QAM_CFG_FEC_I12_J7:  return("i=12, j=7");
    case ERRP_QAM_CFG_FEC_I128_J2: return("i=128, j=2");
    case ERRP_QAM_CFG_FEC_I128_J3: return("i=128, j=3");
    case ERRP_QAM_CFG_FEC_I128_J4: return("i=128, j=4");
    case ERRP_QAM_CFG_FEC_I128_J5: return("i=128, j=5");
    case ERRP_QAM_CFG_FEC_I128_J6: return("i=128, j=6");
    case ERRP_QAM_CFG_FEC_I128_J7: return("i=128, j=7");
    case ERRP_QAM_CFG_FEC_I128_J8: return("i=128, j=8");
    default:                       return("UNKNOWN");
    }
}


/// Get QAM Annex configuration label
char* errp_qam_cfg_annex_txt (uint32 annex)
{
    switch (annex) {
    case ERRP_QAM_CFG_ANNEX_A: return("A");
    case ERRP_QAM_CFG_ANNEX_B: return("B");
    case ERRP_QAM_CFG_ANNEX_C: return("C");
    default:                   return("UNKNOWN");
    }
}


/// Get QAM Channel Bandwidth configuration label
char* errp_qam_cfg_chan_bw_txt (uint32 annex)
{
    switch (annex) {
    case ERRP_QAM_CFG_CHAN_BW_6MHZ: return("6 MHz");
    case ERRP_QAM_CFG_CHAN_BW_7MHZ: return("7 MHz");
    case ERRP_QAM_CFG_CHAN_BW_8MHZ: return("8 MHz");
    default:                        return("UNKNOWN");
    }
}


/// Print ERRP QAM configuration
void errp_qam_cfg_print (const errp_qam_cfg_t *cfg)
{
    printf("  TSID: %d\n", cfg->tsid);
    printf("  Freq: %d Hz\n", cfg->freq);
    printf("  Annex: %s\n", errp_qam_cfg_annex_txt(cfg->annex));
    printf("  Channel bandwidth: %s\n", errp_qam_cfg_chan_bw_txt(cfg->chan_bw));
    printf("  Modulation: %s\n", errp_qam_cfg_modulation_txt(cfg->modulation));
    printf("  Interleaver: %s\n", errp_qam_cfg_interleaver_txt(cfg->interleaver));
}


/// Get CAS Encryption Type label
char* errp_cas_enc_type_txt (uint8 enc_type)
{
    switch (enc_type) {
    case ERRP_CAS_ENC_TYPE_NONE:        return("None");
    case ERRP_CAS_ENC_TYPE_NON_SESSION: return("Non-session");
    case ERRP_CAS_ENC_TYPE_SESSION:     return("Session");
    default:                            return("UNKNOWN");
    }
}


/// Get CAS Encryption Scheme label
char* errp_cas_enc_scheme_txt (uint8 enc_scheme)
{
    switch (enc_scheme) {
    case ERRP_CAS_ENC_SCHEME_DES:     return("DES");
    case ERRP_CAS_ENC_SCHEME_3DES:    return("Triple DES-session");
    case ERRP_CAS_ENC_SCHEME_AES:     return("AES");
    case ERRP_CAS_ENC_SCHEME_DVB_CSA: return("DVB-CSA");
    case ERRP_CAS_ENC_SCHEME_PKEY:    return("PowerKEY");
    case ERRP_CAS_ENC_SCHEME_MEDIAC:  return("MediaCipher");
    case ERRP_CAS_ENC_SCHEME_DVS042:  return("DVS-042");
    default:                          return("UNKNOWN");
    }
}


/// Print ERRP CAS capability
void errp_cas_cap_print (const errp_cas_cap_t *cap)
{
    printf("  Type: %s\n", errp_cas_enc_type_txt(cap->enc_type));
    printf("  Scheme: %s\n", errp_cas_enc_scheme_txt(cap->enc_scheme));
    printf("  Key length: %d\n", cap->key_len);
    printf("  CAS ID: %d\n", cap->cas_id);
}


/// Print ERRP Edge Input
void errp_edge_input_print (const errp_edge_input_t *input)
{
    printf("  Interface ID %d\n", input->if_id);
    printf("  Subnet mask: %08x\n", input->subnet_mask);
    printf("  Host: %s\n", input->host_name);
    printf("  Group: %s\n", input->group_name);
    printf("  Max BW: %d\n", input->max_bandwidth);
}


/// Print Static Port UDP Map
void errp_udp_map_static_port_print (const errp_udp_map_static_port_t *map)
{
    printf("  Static port: udp %d, prog %d\n", map->udp_port, map->prog_num);
}

/// Print Static Range UDP Map
void errp_udp_map_static_range_print (const errp_udp_map_static_range_t *map)
{
    printf("  Static range: start udp %d, start prog %d, count %d\n",
           map->start_udp_port, map->start_prog_num, map->count);
}

/// Print Dynamic Range UDP Map
void errp_udp_map_dynamic_range_print (const errp_udp_map_dynamic_range_t *map)
{
    printf("  Dynamic range: start udp %d, count %d\n",
           map->start_udp_port, map->count);
}

/// Print UDP Map
void errp_udp_map_print (const errp_udp_map_t *map)
{
    switch (map->type) {  
    case ERRP_UDP_MAP_TYPE_STATIC_PORT:
        errp_udp_map_static_port_print(&map->static_port);
        break;
    case ERRP_UDP_MAP_TYPE_STATIC_RANGE:
        errp_udp_map_static_range_print(&map->static_range);
        break;
    case ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE:
        errp_udp_map_dynamic_range_print(&map->dynamic_range);
        break;
    default:
        printf("Bad UDP map type %d", map->type);
    }
}


/// Get Service Status label
char* errp_service_status_txt (uint32 service_status)
{
    switch (service_status) {
    case ERRP_SERVICE_STATUS_OPERATIONAL:   return("Operational");
    case ERRP_SERVICE_STATUS_SHUTTING_DOWN: return("Shutting Down");
    case ERRP_SERVICE_STATUS_STANDBY:       return("Stadby");
    default:                                return("UNKNOWN");
    }
}


// returns a bit mask that indicates the rightmost set bit of the input
uint32 rightmost_set_bit (uint32 x)
{
    return (x & (-x));
}


/// Get ERRP attribute type code from a bit mask
/// @note  Only one bit in the mask should be set
errp_attr_t errp_get_attr_type_from_mask (uint32 attr_mask)
{
    switch (attr_mask) {
    case ERRP_ATTR_MASK_WITHDRAWN_ROUTE: return ERRP_ATTR_WITHDRAWN_ROUTE;
    case ERRP_ATTR_MASK_REACHABLE_ROUTE: return ERRP_ATTR_REACHABLE_ROUTE;
    case ERRP_ATTR_MASK_NEXT_HOP_SERVER: return ERRP_ATTR_NEXT_HOP_SERVER;
    case ERRP_ATTR_MASK_QAM_NAMES:       return ERRP_ATTR_QAM_NAMES;
    case ERRP_ATTR_MASK_CAS_CAP:         return ERRP_ATTR_CAS_CAP;
    case ERRP_ATTR_MASK_TOTAL_BANDWIDTH: return ERRP_ATTR_TOTAL_BANDWIDTH;
    case ERRP_ATTR_MASK_AVAIL_BANDWIDTH: return ERRP_ATTR_AVAIL_BANDWIDTH;
    case ERRP_ATTR_MASK_COST:            return ERRP_ATTR_COST;
    case ERRP_ATTR_MASK_EDGE_INPUT:      return ERRP_ATTR_EDGE_INPUT;
    case ERRP_ATTR_MASK_QAM_CFG:         return ERRP_ATTR_QAM_CFG;
    case ERRP_ATTR_MASK_UDP_MAP:         return ERRP_ATTR_UDP_MAP;
    case ERRP_ATTR_MASK_SERVICE_STATUS:  return ERRP_ATTR_SERVICE_STATUS;
    case ERRP_ATTR_MASK_MAX_MPEG_FLOWS:  return ERRP_ATTR_MAX_MPEG_FLOWS;
    case ERRP_ATTR_MASK_NEXT_HOP_ALT:    return ERRP_ATTR_NEXT_HOP_ALT;
    case ERRP_ATTR_MASK_PORT_ID:         return ERRP_ATTR_PORT_ID;
    case ERRP_ATTR_MASK_FIBER_NODE:      return ERRP_ATTR_FIBER_NODE;
    case ERRP_ATTR_MASK_QAM_CAP:         return ERRP_ATTR_QAM_CAP;
    case ERRP_ATTR_MASK_INPUT_MAP:       return ERRP_ATTR_INPUT_MAP;
    }
    return ERRP_ATTR_UNKNOWN;
}


/// Find missing ERRP attribute in UPDATE message
uint8 errp_find_missing_attr (uint32 attr_mask)
{
    uint32 miss_mask = 0;

    if ((attr_mask & ERRP_ATTR_MASK_REACHABLE_ROUTE)) {
        miss_mask = (attr_mask & ERRP_ATTR_MASK_REQUIRED_REACHABLE)
                    ^ ERRP_ATTR_MASK_REQUIRED_REACHABLE;

    } else if ((attr_mask & ERRP_ATTR_MASK_WITHDRAWN_ROUTE)) {
        miss_mask = (attr_mask & ERRP_ATTR_MASK_REQUIRED_WITHDRAWN)
                    ^ ERRP_ATTR_MASK_REQUIRED_WITHDRAWN;
    }

    if (miss_mask) {
        return errp_get_attr_type_from_mask(rightmost_set_bit(miss_mask));
    }
    return 0;
}


/// ERRP Attribute Type label
char* errp_attr_type_txt (uint8 attr_type)
{
    switch (attr_type) {
    case ERRP_ATTR_WITHDRAWN_ROUTE: return "Withdrawn Route";
    case ERRP_ATTR_REACHABLE_ROUTE: return "Reachable Route";
    case ERRP_ATTR_NEXT_HOP_SERVER: return "Next Hop Server";
    case ERRP_ATTR_QAM_NAMES:       return "QAM Names";
    case ERRP_ATTR_CAS_CAP:         return "CAS Capability";
    case ERRP_ATTR_TOTAL_BANDWIDTH: return "Total Bandwidth";
    case ERRP_ATTR_AVAIL_BANDWIDTH: return "Available Bandwidth";
    case ERRP_ATTR_COST:            return "Cost";
    case ERRP_ATTR_EDGE_INPUT:      return "Edge Input";
    case ERRP_ATTR_QAM_CFG:         return "QAM Configuration";
    case ERRP_ATTR_UDP_MAP:         return "UDP Map";
    case ERRP_ATTR_SERVICE_STATUS:  return "Service Status";
    case ERRP_ATTR_MAX_MPEG_FLOWS:  return "Max MPEG Flows";
    case ERRP_ATTR_NEXT_HOP_ALT:    return "Next Hop Alternate";
    case ERRP_ATTR_PORT_ID:         return "Port ID";
    case ERRP_ATTR_FIBER_NODE:      return "Fiber Node";
    case ERRP_ATTR_QAM_CAP:         return "QAM Capability";
    case ERRP_ATTR_INPUT_MAP:       return "Input Map";
    default:                        return "UNKNOWN";
    }
}


/// Print UDP Map
/// @todo Need better output format
void errp_qam_cap_print (const errp_qam_cap_t *cap)
{
    printf("  Channel BW: %04x", *(uint16*)&cap->chan_bw);
    printf("  J83: %04x", *(uint16*)&cap->j83);
    printf("  Interleaver: %08x", *(uint32*)&cap->interleaver);
    printf("  DOCSIS_Video: %08x", *(uint32*)&cap->docsis_video);
    printf("  Modulation: %04x", *(uint16*)&cap->modulation);
}


/// Print ERRP Resource info
void errp_resource_print (const errp_resource_t *rsc)
{
    int i;

    if ((rsc->attr_mask & ERRP_ATTR_WITHDRAWN_ROUTE)) {
        printf("Withdrawn route:\n");
        errp_route_print(&rsc->withdrawn_route);
    }

    if ((rsc->attr_mask & ERRP_ATTR_REACHABLE_ROUTE)) {
        printf("Reachable route:\n");
        errp_route_print(&rsc->reachable_route);
    }

    if ((rsc->attr_mask & ERRP_ATTR_NEXT_HOP_SERVER)) {
        printf("Next hop server:\n");
        errp_server_print(&rsc->next_hop_server);
    }

    if ((rsc->attr_mask & ERRP_ATTR_QAM_NAMES)) {
        printf("QAM names: %s\n", rsc->qam_name);
    }

    if ((rsc->attr_mask & ERRP_ATTR_CAS_CAP)) {
        printf("CAS capability:\n");
        errp_cas_cap_print(&rsc->cas_cap);
    }

    if ((rsc->attr_mask & ERRP_ATTR_TOTAL_BANDWIDTH)) {
        printf("Total bandwidth: %d\n", rsc->total_bandwidth);
    }

    if ((rsc->attr_mask & ERRP_ATTR_AVAIL_BANDWIDTH)) {
        printf("Avail bandwidth: %d\n", rsc->avail_bandwidth);
    }

    if ((rsc->attr_mask & ERRP_ATTR_COST)) {
        printf("Cost: %d\n", rsc->cost);
    }

    if ((rsc->attr_mask & ERRP_ATTR_EDGE_INPUT)) {
        printf("Edge Input: %d\n", rsc->edge_input_cnt);
        for (i=0; i<rsc->edge_input_cnt; i++) {
            errp_edge_input_print(&rsc->edge_input[i]);
        }
    }

    if ((rsc->attr_mask & ERRP_ATTR_QAM_CFG)) {
        printf("QAM Config:\n");
        errp_qam_cfg_print(&rsc->qam_cfg);
    }

    if ((rsc->attr_mask & ERRP_ATTR_UDP_MAP)) {
        printf("UDP MAP: %d\n", rsc->udp_map_cnt);
        for (i=0; i<rsc->udp_map_cnt;  i++) {
            errp_udp_map_print(&rsc->udp_map[i]); 
        }
    }

    if ((rsc->attr_mask & ERRP_ATTR_SERVICE_STATUS)) {
        printf("Service status: %s\n",
               errp_service_status_txt(rsc->service_status));
    }

    if ((rsc->attr_mask & ERRP_ATTR_MAX_MPEG_FLOWS)) {
        printf("Max MPEG flows: %d\n", rsc->max_flows);
    }

    if ((rsc->attr_mask & ERRP_ATTR_NEXT_HOP_ALT)) {
        printf("Next Hop Alternate: %d\n", rsc->alt_server_cnt);
        for (i=0; i<rsc->alt_server_cnt; i++) {
            errp_server_print(&rsc->next_hop_server_alt[i]);
        }
    }

    if ((rsc->attr_mask & ERRP_ATTR_PORT_ID)) {
        printf("Port ID: %d\n", rsc->port_id);
    }

    if ((rsc->attr_mask & ERRP_ATTR_FIBER_NODE)) {
        printf("Fiber Node: %d\n", rsc->fiber_node_cnt);
        for (i=0; i<rsc->fiber_node_cnt; i++) {
            printf("%s\n", rsc->fiber_node[i]);
        }
    }

    if ((rsc->attr_mask & ERRP_ATTR_QAM_CAP)) {
        printf("QAM Capability:\n");
        errp_qam_cap_print(&rsc->qam_cap);
    }

    if ((rsc->attr_mask & ERRP_ATTR_INPUT_MAP)) {
        printf("Input Map: %d\n", rsc->input_cnt);
        for (i=0; i<rsc->input_cnt; i++) {
            printf("%s\n", rsc->input_map[i]);
        }
    }
}


/// ERRP Error Code label
char* errp_err_txt (uint8 err)
{
    switch (err) {
    case ERRP_ERR_MSG_HEADER:         return("Message Header Error");
    case ERRP_ERR_OPEN_MSG:           return("OPEN Message Error");
    case ERRP_ERR_UPDATE_MSG:         return("UPDATE Message Error");
    case ERRP_ERR_HOLD_TIMER_EXPIRED: return("Hold Timer Expired");
    case ERRP_ERR_FSM:                return("Finite State Machine Error");
    case ERRP_ERR_CEASE:              return("Cease");
    default:                          return("UNKNOWN");
    }
}

/// ERRP Error Sub-Code label
char* errp_err_sub_txt (uint8 err, uint8 err_sub)
{
    switch (err) {
    case ERRP_ERR_MSG_HEADER:
        switch (err_sub) {
        case ERRP_ERR_SUB_UNSPECIFIED:
            return("Unspecified Error");
        case ERRP_ERR_SUB_BAD_MSG_LEN:
            return("Bad Message Length");
        case ERRP_ERR_SUB_BAD_MSG_TYPE:
            return("Bad Message Type");
        default:
            return("UNKNOWN");
        }

    case ERRP_ERR_OPEN_MSG:
        switch (err_sub) {
        case ERRP_ERR_SUB_UNSPECIFIED:
            return("Unspecified Error");
        case ERRP_ERR_SUB_UNSUP_VERSION:
            return("Unsupported Version Number");
        case ERRP_ERR_SUB_BAD_PEER_ADDR_DOMAIN:
            return("Bad Peer Address Domain");
        case ERRP_ERR_SUB_BAD_ERRP_ID:
            return("Bad ERRP Identifier");
        case ERRP_ERR_SUB_UNSUP_OPT_PARAM:
            return("Unsupported Optional Parameter");
        case ERRP_ERR_SUB_UNSUP_HOLD_TIME:
            return("Unacceptable Hold Time");
        case ERRP_ERR_SUB_UNSUP_CAP:
            return("Unsupported Capability");
        case ERRP_ERR_SUB_CAP_MISMATCH:
            return("Capability Mismatch");
        default:
            return("UNKNOWN");
        }

    case ERRP_ERR_UPDATE_MSG:
        switch (err_sub) {
        case ERRP_ERR_SUB_UNSPECIFIED:
            return("Unspecified Error");
        case ERRP_ERR_SUB_MALFORMED_ATTR_LIST:
            return("Malformed Attribute List");
        case ERRP_ERR_SUB_UNRECOG_ATTR:
            return("Unrecognized Well-known Attribute");
        case ERRP_ERR_SUB_MISSING_ATTR:
            return("Missing Well-known Mandatory Attribute");
        case ERRP_ERR_SUB_ATTR_FLAGS:
            return("Attribute Flags Error");
        case ERRP_ERR_SUB_ATTR_LEN:
            return("Attribute Length Error");
        case ERRP_ERR_SUB_INVALID_ATTR:
            return("Invalid Attribute");
        default:
            return("UNKNOWN");
        }

    case ERRP_ERR_HOLD_TIMER_EXPIRED:
    case ERRP_ERR_FSM:
    case ERRP_ERR_CEASE:
        return("N/A");
    }

    return ("UNKNOWN");
}


/// ERRP ConnectRetry timer timeout event handler
void errp_connect_timer_event_handler (union sigval value)
{
    errp_conn_t* conn = value.sival_ptr;
    errp_event(conn, E_CONN_RETRY_TIMER_EXP, NULL);
}


/// ERRP Keepalive timer timeout event handler
void errp_keepalive_timer_event_handler (union sigval value)
{
    errp_conn_t* conn = value.sival_ptr;
    errp_event(conn, E_KEEPALIVE_TIMER_EXP, NULL);
}


/// ERRP Hold timer timeout event handler
void errp_hold_timer_event_handler (union sigval value)
{
    errp_conn_t* conn = value.sival_ptr;
    errp_event(conn, E_HOLD_TIMER_EXP, NULL);
}


/// Set ERRP route type
void errp_set_route_type (errp_route_type_t *route_type,
                          uint16 addr_family, uint16 app_protocol)
{
    route_type->addr_family = addr_family;
    route_type->app_protocol = app_protocol;
}


/// Set up error report for bad message length error
void errp_set_err_bad_msg_len (errp_error_report_t *rpt, uint16 msg_len)
{
    rpt->err = ERRP_ERR_MSG_HEADER;
    rpt->err_sub = ERRP_ERR_SUB_BAD_MSG_LEN;
    rpt->len = 2;
    *((uint16*)rpt->data) = msg_len;
}


/// Set up error report for bad message type error
void errp_set_err_bad_msg_type (errp_error_report_t *rpt, uint8 msg_type)
{
    rpt->err = ERRP_ERR_MSG_HEADER;
    rpt->err_sub = ERRP_ERR_SUB_BAD_MSG_TYPE;
    rpt->len = 1;
    rpt->data[0] = msg_type;
}


void errp_set_err_mismatch_cap (errp_error_report_t *rpt)
{
    rpt->err_sub = ERRP_ERR_SUB_CAP_MISMATCH;
    rpt->len = 0;
}


void errp_add_err_mismatch_cap (errp_error_report_t *rpt, uint16 cap_code,
                                uint16 cap_len, void* cap_value)
{
    memcpy(rpt->data + rpt->len, cap_value, cap_len);
    rpt->len += ERRP_CAP_INFO_HDR_SIZE + cap_len;
}


/// ERRP Notification print
void errp_error_report_print (const errp_error_report_t *rpt)
{
    parser_t psr;
    psr.ptr = rpt->data;
    psr.len = rpt->len;

    printf("  Error: %s\n", errp_err_txt(rpt->err));
    printf("  Sub-error: %s\n", errp_err_sub_txt(rpt->err, rpt->err_sub));
    printf("  Data length: %d\n", rpt->len);

    switch (rpt->err) {
    case ERRP_ERR_MSG_HEADER:
        switch (rpt->err_sub) {
        case ERRP_ERR_SUB_BAD_MSG_LEN:
            printf("  Bad message length %d\n", *(uint16*)rpt->data);
            break;
        case ERRP_ERR_SUB_BAD_MSG_TYPE:
            printf("  Bad message type %d\n", *(uint8*)rpt->data);
            break;
        }
        break;

    case ERRP_ERR_OPEN_MSG:
        switch (rpt->err_sub) {
        case ERRP_ERR_SUB_UNSUP_VERSION:
            printf("  Largest supported version %d\n", *(uint8*)rpt->data);
            break;
        case ERRP_ERR_SUB_UNSUP_OPT_PARAM:
        {
            uint16 param_type, param_len;
            get_short(&psr, &param_type);
            get_short(&psr, &param_len);
            printf("  Parameter type %d, length %d\n", param_type, param_len);
            print_hex(rpt->data, rpt->len);
            break;
        }
        case ERRP_ERR_SUB_UNSUP_CAP:
        case ERRP_ERR_SUB_CAP_MISMATCH:
        {
            while (psr.len > 0) {
                uint16 cap_code, cap_len;
                get_short(&psr, &cap_code);
                get_short(&psr, &cap_len);
                printf("  Capability type %d, length %d\n", cap_code, cap_len);
                print_hex(psr.ptr - ERRP_CAP_INFO_HDR_SIZE,
                          cap_len + ERRP_CAP_INFO_HDR_SIZE);
                psr.ptr += cap_len;
                psr.len -= cap_len;
            }
            break;
        }
        }
        break;

    case ERRP_ERR_UPDATE_MSG:
        switch (rpt->err_sub) {
        case ERRP_ERR_SUB_ATTR_FLAGS:
        case ERRP_ERR_SUB_ATTR_LEN:
        case ERRP_ERR_SUB_UNRECOG_ATTR:
        case ERRP_ERR_SUB_INVALID_ATTR:
        case ERRP_ERR_SUB_MALFORMED_ATTR_LIST:
        {
            uint8 attr_flags, attr_type_code;
            uint16 attr_len;
            get_byte(&psr, &attr_flags);
            get_byte(&psr, &attr_type_code);
            get_short(&psr, &attr_len);
            printf("  Attribute flags %02x, type %d, length %d\n",
                   attr_flags, attr_type_code, attr_len);
            print_hex(rpt->data, rpt->len);
            break;
        }
        case ERRP_ERR_SUB_MISSING_ATTR:
        {
            uint8 attr_type = *((uint8*)rpt->data);
            printf("  Attribute type code %d: %s\n",
                   attr_type, errp_attr_type_txt(attr_type));
            break;
        }
        }
        break;
    }
}


/// This routine gets a complete message from the ERRP peer node
///
/// @param conn         ERRP connection
/// @param psr          parser
/// @param msg_type     pointer to the received message type (output)
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
int errp_get_peer_msg (errp_conn_t *conn, parser_t *psr, uint8 *msg_type,
                       uint16 *msg_len)
{
    int read_len;

check:
    if (conn->avail_len >= ERRP_MSG_HDR_SIZE) {
        psr->ptr = conn->in_msg_buf + conn->in_msg_offset;
        psr->len = conn->avail_len;
        errp_get_msg_hdr(psr, msg_type, msg_len);

        if (*msg_len > ERRP_MAX_MSG_BYTES) {
            printf("Message size too big: %d\n", *msg_len);
            return E2BIG;
        }

        if (*msg_len <= conn->avail_len) {
            // Complete message received
            conn->avail_len -= *msg_len;
            conn->in_msg_offset += *msg_len;
            psr->len = *msg_len - ERRP_MSG_HDR_SIZE;
            return EOK;
        }
    }

    if (conn->in_msg_offset > 0) {
        if (conn->avail_len > 0) {
            // Move partial message to the beginning of buffer
            memmove(conn->in_msg_buf, conn->in_msg_buf + conn->in_msg_offset,
                    conn->avail_len);
        }
        conn->in_msg_offset = 0;
    }

wait:
    // Need to get more data
    read_len = io_recv(conn->fd, conn->in_msg_buf + conn->avail_len,
                       ERRP_MAX_MSG_BYTES - conn->avail_len);

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

    conn->avail_len += read_len;
    goto check;
}

