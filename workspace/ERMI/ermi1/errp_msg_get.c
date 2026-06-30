///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp_msg_get.c
///  @brief     ERRP (ERMI-1) message parsing functions
///  @author    Yi Tong Tse


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"
#include "util.h"
#include "errp.h"


#define ERRP_MSG_LOG(fmt, str...)        \
    printf("ERRP_MSG: "fmt"\n", ##str);
//    NULL


/// Read an ERRP-styled string (a string preceeded by its 2-byte lenght field)
/// @return Number of bytes read
void errp_get_string (parser_t *psr, char *string)
{
    uint16 str_len = 0;
    get_short(psr, &str_len);
    if (str_len > ERRP_MAX_NAME_LEN) {
        ERRP_MSG_LOG("> String too long: %d", str_len);
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
    }
    get_string(psr, str_len, string);
}


/// Read ERRP message header
void errp_get_msg_hdr (parser_t *psr, uint8 *msg_type, uint16 *msg_len)
{
    get_short(psr, msg_len);
    get_byte(psr, msg_type);
}


/// Read ERRP parameter header
void errp_get_param_hdr (parser_t *psr,
                         uint16 *param_type, uint16 *param_len)
{
    get_short(psr, param_type);
    get_short(psr, param_len);
    if (!psr->err) {
        ERRP_MSG_LOG("> Param type %d, len %d", *param_type, *param_len);
    }
}


/// Read ERRP Capability Info header
void errp_get_cap_info_hdr (parser_t *psr, uint16 *cap_code, uint16 *cap_len)
{
    get_short(psr, cap_code);
    get_short(psr, cap_len);
    if (!psr->err) {
        ERRP_MSG_LOG("> CapInfo type %d, len %d", *cap_code, *cap_len);
    }
}


/// Read ERRP Route Type
void errp_get_route_type (parser_t *psr, errp_route_type_t *route_type)
{
    get_short(psr, &route_type->addr_family);
    get_short(psr, &route_type->app_protocol);
}


/// Read Route Type Supported Capability
void errp_get_cap_route_type_supported (parser_t *psr, int len,
                                        int *route_type_cnt,
                                        errp_route_type_t *route_type)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    ERRP_MSG_LOG("> Route type supported: len %d", len);

    *route_type_cnt = 0;
    while (!psr->err && psr->len > 0) {
        if (++(*route_type_cnt) > ERRP_MAX_ROUTE_TYPES) {
            psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
            break;
        }
        errp_get_route_type(psr, route_type++);
        ERRP_MSG_LOG("> Route type: family %d, prot %d",
                     (route_type-1)->addr_family, (route_type-1)->app_protocol);
    }
    psr->len = save_len;
}


/// Read Send Receive Capability
void errp_get_cap_send_receive (parser_t *psr, int len, uint32 *send_receive)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, send_receive);
}


/// Read ERRP Version Capability
void errp_get_cap_errp_version (parser_t *psr, int len, uint32 *errp_version)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, errp_version);
}


/// Read all Capability parameters
/// @todo Support of vendor specific capability codes (32769-65535)
void errp_get_param_cap_info (parser_t *psr, int len, errp_node_t *node,
                              errp_error_report_t *err_rpt)
{
    int save_len;
    PARSER_CHECK(psr, len);  
    save_len = psr->len;
    psr->len = len;

    while (!psr->err && psr->len > 0) {
        const char* cap_ptr = psr->ptr;
        uint16 cap_code;
        uint16 cap_len;

        errp_get_cap_info_hdr(psr, &cap_code, &cap_len);

        switch (cap_code) {
        case ERRP_CAP_ROUTE_TYPES_SUPPORTED:
            errp_get_cap_route_type_supported(psr, cap_len,
                            &node->route_type_cnt, node->route_type);
            break;

        case ERRP_CAP_SEND_RECEIVE:
            errp_get_cap_send_receive(psr, cap_len, &node->send_receive);
            break;

        case ERRP_CAP_ERRP_VERSION:
            errp_get_cap_errp_version(psr, cap_len, &node->errp_version);
            break;

        default:
            if (!psr->err) {
                psr->err = PARSER_ERR_UNKNOWN_SYNTAX;
                printf("Unknown ERRP capability code %d\n", cap_code);
                err_rpt->err_sub = ERRP_ERR_SUB_UNSUP_CAP;
                err_rpt->len = ERRP_CAP_INFO_HDR_SIZE + cap_len;
                memcpy(err_rpt->data, cap_ptr, err_rpt->len);
            }
        }
    }

    psr->len = save_len;
}


/// Read Streaming Zone Parameter
void errp_get_param_streaming_zone(parser_t *psr, int len, char *streaming_zone)
{
    if (!psr->err && len > ERRP_MAX_NAME_LEN) {
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
    }
    get_string(psr, len, streaming_zone);
}


/// Read Component Name Parameter
void errp_get_param_component_name(parser_t *psr, int len, char* component_name)
{
    if (!psr->err && len > ERRP_MAX_NAME_LEN) {
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
    }
    get_string(psr, len, component_name);
}


/// Read Vendor Specific String Parameter
void errp_get_param_vendor_string(parser_t *psr, int len, char *vendor_string)
{
    if (!psr->err && len > ERRP_MAX_NAME_LEN) {
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
    }
    get_string(psr, len, vendor_string);
}


/// Read all parameters in OPEN message
void errp_get_params (parser_t *psr, errp_node_t *node,
                      errp_error_report_t *err_rpt)
{
    while (!psr->err && psr->len > 0) {
        const char* param_ptr = psr->ptr;
        uint16 param_type;
        uint16 param_len;

        errp_get_param_hdr(psr, &param_type, &param_len);

        switch (param_type) {
        case ERRP_PARAM_CAP_INFO:
            errp_get_param_cap_info(psr, param_len, node, err_rpt);
            break;

        case ERRP_PARAM_STREAMING_ZONE:
            errp_get_param_streaming_zone(psr, param_len, node->streaming_zone);
            break;

        case ERRP_PARAM_COMPONENT_NAME:
            errp_get_param_component_name(psr, param_len, node->component_name);
            break;

        case ERRP_PARAM_VENDOR_STRING:
            errp_get_param_vendor_string(psr, param_len, node->vendor_string);
            break;

        default:
            if (!psr->err) {
                psr->err = PARSER_ERR_UNKNOWN_SYNTAX;
                printf("Unknown ERRP optional parameter type %d\n", param_type);
                err_rpt->err_sub = ERRP_ERR_SUB_UNSUP_OPT_PARAM;
                err_rpt->len = ERRP_PARAM_HDR_SIZE + param_len;
                memcpy(err_rpt->data, param_ptr, err_rpt->len);
            }
        }
    }
}


/// Check Open Message
/// @return  TRUE if no error is detected
boolean errp_check_open_msg (errp_conn_t *conn, errp_error_report_t *rpt)
{
    errp_node_t *peer = conn->peer;
    errp_node_t *node = conn->node;

    /// Check version
    if (peer->version > node->version) { 
        *(uint8*)(rpt->data) = node->version;
        rpt->err_sub = ERRP_ERR_SUB_UNSUP_VERSION;
        rpt->len = 1;
        return FALSE;
    }

    /// Check address domain
    /// @note  Need to define errp_is_addr_domain_valid()
    if (!errp_is_addr_domain_valid(peer->addr_domain)) {
        rpt->err_sub = ERRP_ERR_SUB_BAD_PEER_ADDR_DOMAIN;
        return FALSE;
    }

    /// Check Hold time
    if (peer->hold_time > 0 && peer->hold_time < ERRP_HOLD_TIME_MIN) {
        rpt->err_sub = ERRP_ERR_SUB_UNSUP_HOLD_TIME;
        return FALSE;
    }

    /// Check ERRP ID 
    /// @note  Need to implement errp_is_errp_id_present()
    if (errp_is_errp_id_present(peer->addr_domain,
                                peer->errp_id)) { 
        rpt->err_sub = ERRP_ERR_SUB_BAD_ERRP_ID;
        return FALSE;
    }

    return TRUE;
}


/// Read ERRP OPEN message
void errp_get_open_msg (parser_t *psr, errp_conn_t *conn,
                        errp_error_report_t *err_rpt)
{
    errp_node_t *peer = conn->peer;
    uint16 opt_param_len;
    uint8 resv;
    uint16 msg_len = psr->len + ERRP_MSG_HDR_SIZE;

    ERRP_MSG_LOG("> OPEN msg:");

    err_rpt->err_sub = 0;

    if (msg_len <= 16) {
        errp_set_err_bad_msg_len(err_rpt, msg_len);
        return;
    }

    // Parse fixed header
    get_byte(psr, &peer->version);
    get_byte(psr, &resv);
    get_short(psr, &peer->hold_time);
    get_long(psr, &peer->addr_domain);
    get_long(psr, &peer->errp_id);
    get_short(psr, &opt_param_len);
    if (psr->err) {
        return;
    }

    err_rpt->err = ERRP_ERR_OPEN_MSG;

    // Check header fields
    if (!errp_check_open_msg(conn, err_rpt)) {
        return;
    }

    if (psr->len != opt_param_len) {
        return;
    }

    // Parse optional parameters
    errp_get_params(psr, peer, err_rpt);
    if (psr->err) {
        return;
    }

    // Check capability parameters
    if (!errp_check_cap_param(conn, err_rpt)) {
        return;
    }

    err_rpt->err = ERRP_OK;
}


/// Read ERRP Attribute header
void errp_get_attr_hdr (parser_t *psr, uint8 *attr_flags,
                        uint8 *attr_type_code, uint16 *attr_len)
{
    get_byte(psr, attr_flags);
    get_byte(psr, attr_type_code);
    get_short(psr, attr_len);
//    ERRP_MSG_LOG("> Attr type %d, len %d", *attr_type_code, *attr_len);
}   


/// Read an ERRP Route attribute
void errp_get_route (parser_t *psr, errp_route_t *route)
{
    get_short(psr, &route->type.addr_family);
    get_short(psr, &route->type.app_protocol);
    errp_get_string(psr, route->addr);
}


/// Read WithdrawnRoutes attribute
/// @note Support only one route per UPDATE
void errp_get_attr_withdrawn_routes (parser_t *psr, int len,
                                     errp_route_t *route)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    errp_get_route(psr, route);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read ReachableRoutes attribute
void errp_get_attr_reachable_routes (parser_t *psr, int len,
                                     errp_route_t *route)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    errp_get_route(psr, route);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


void errp_get_server (parser_t *psr, int len, errp_server_t *server)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_long(psr, &server->addr_domain);
    errp_get_string(psr, server->component_addr);
    errp_get_string(psr, server->streaming_zone);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read NextHopServer attribute
void errp_get_attr_next_hop_server (parser_t *psr, int len,
                                    errp_server_t *server)
{   
    errp_get_server(psr, len, server);
}   


/// Read NextHopServerAlternate attribute
void errp_get_attr_next_hop_alt (parser_t *psr, int len,
                                 uint16 *server_cnt, errp_server_t *server)
{
    uint16 cnt;

    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_short(psr, &cnt);
    *server_cnt = cnt;

    if (cnt > ERRP_MAX_ALT_SERVERS) {
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
        goto skip;
    }

    for ( ; cnt>0; cnt--) {
        uint16 server_len;
        get_short(psr, &server_len);
        errp_get_server(psr, server_len, server++);
    }

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
skip:
    psr->len = save_len;
}


/// Read QAM Name attribute
void errp_get_attr_qam_names (parser_t *psr, int len, char *qam_name)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    errp_get_string(psr, qam_name);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read CAS Capability attribute
void errp_get_attr_cas_cap (parser_t *psr, int len, errp_cas_cap_t *cas_cap)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_byte(psr, &cas_cap->enc_type);
    get_byte(psr, &cas_cap->enc_scheme);
    get_short(psr, &cas_cap->key_len);
    get_short(psr, &cas_cap->cas_id);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read Total Bandwidth attribute
void errp_get_attr_total_bandwidth (parser_t *psr, int len,
                                    uint32 *bandwidth)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, bandwidth);
}


/// Read Available Bandwidth attribute
void errp_get_attr_avail_bandwidth (parser_t *psr, int len, uint32 *bandwidth)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, bandwidth);
}


/// Read Cost attribute
void errp_get_attr_cost (parser_t *psr, int len, uint8 *cost)
{
    if (!psr->err && len != 1) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_byte(psr, cost);
}


/// Read an Edge Input
void errp_get_edge_input (parser_t *psr, errp_edge_input_t *input)
{
    get_long(psr, &input->subnet_mask);
    errp_get_string(psr, input->host_name);
    get_long(psr, &input->if_id);
    get_long(psr, &input->max_bandwidth);
    errp_get_string(psr, input->group_name);
}


/// Read Edge Input attribute
void errp_get_attr_edge_input (parser_t *psr, int len, int *num_inputs,
                              errp_edge_input_t *input)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    *num_inputs = 0;
    while (!psr->err && psr->len > 0) {
        if (++(*num_inputs) > ERRP_MAX_EDGE_INPUTS_PER_ATTR) {
            psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
            break;
        }

        errp_get_edge_input(psr, input++);
    }

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read QAM Configuration attribute
void errp_get_attr_qam_cfg (parser_t *psr, int len, errp_qam_cfg_t *cfg)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_long(psr, &cfg->freq);
    get_byte(psr, &cfg->modulation);
    get_byte(psr, &cfg->interleaver);
    get_short(psr, &cfg->tsid);
    get_byte(psr, &cfg->annex);
    get_byte(psr, &cfg->chan_bw);

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read UDP Map for a static port
void errp_get_udp_map_static_port (parser_t *psr,
                                  errp_udp_map_static_port_t *map)
{
    get_short(psr, &map->udp_port);
    get_short(psr, &map->prog_num);
}


/// Read UDP Map for a static port range
void errp_get_udp_map_static_range (parser_t *psr,
                                    errp_udp_map_static_range_t *map)
{
    get_short(psr, &map->start_udp_port);
    get_short(psr, &map->start_prog_num);
    get_short(psr, &map->count);
}


/// Read UDP Map for a dynamic port range
void errp_get_udp_map_dynamic_range (parser_t *psr,
                                     errp_udp_map_dynamic_range_t *map)
{
    get_short(psr, &map->start_udp_port);
    get_short(psr, &map->count);
}


/// Read UDP Map attribute
void errp_get_attr_udp_map (parser_t *psr, int len, int *num_maps,
                            errp_udp_map_t *map)
{
    int cnt;

    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    // Parse all static ports
    get_long(psr, &cnt);
    if (psr->err)  return;
    (*num_maps) = cnt;
    for ( ; cnt>0; cnt--) {
        map->type = ERRP_UDP_MAP_TYPE_STATIC_PORT;
        errp_get_udp_map_static_port(psr, &(map++)->static_port);
    }

    // Parse all static ranges
    get_long(psr, &cnt);
    if (psr->err)  return;
    (*num_maps) += cnt;
    for ( ; cnt>0; cnt--) {
        map->type = ERRP_UDP_MAP_TYPE_STATIC_RANGE;
        errp_get_udp_map_static_range(psr, &(map++)->static_range);
    }

    // Parse all dynamic ranges
    get_long(psr, &cnt);
    if (psr->err)  return;
    (*num_maps) += cnt;
    for ( ; cnt>0; cnt--) {
        map->type = ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE;
        errp_get_udp_map_dynamic_range(psr, &(map++)->dynamic_range);
    }
    (*num_maps) += cnt;

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read Service Status attribute
void errp_get_attr_service_status (parser_t *psr, int len,
                                   uint32 *service_status)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, service_status);
}


/// Read Max MPEG Flows attribute
void errp_get_attr_max_mpeg_flows (parser_t *psr, int len,
                                   uint32 *max_flows)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, max_flows);
}


/// Read Interface ID attribute
void errp_get_attr_port_id (parser_t *psr, int len, uint32 *port_id)
{
    if (!psr->err && len != 4) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
        return;
    }
    get_long(psr, port_id);
}


/// Read Fiber Node attribute
/// @note fiber_node is actually 2D char array [][ERRP_MAX_NAME_LEN]
void errp_get_attr_fiber_node (parser_t *psr, int len, int *fiber_node_cnt,
                               char *fiber_node)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    *fiber_node_cnt = 0;
    while (!psr->err && psr->len > 0) {
        if (++(*fiber_node_cnt) > ERRP_MAX_FIBER_NODES_PER_ROUTE) {
            psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
            break;
        }
        errp_get_string(psr, fiber_node);
        fiber_node += ERRP_MAX_NAME_LEN;
    }

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read Input Map attribute
/// @note input_map is actually 2D char array [][ERRP_MAX_NAME_LEN]
void errp_get_attr_input_map (parser_t *psr, int len, int *input_cnt,
                              char *input_map)
{
    int i;
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_long(psr, input_cnt);
    if (!psr->err && *input_cnt > ERRP_MAX_INPUTS_PER_ROUTE) {
        psr->err = PARSER_ERR_SYNTAX_TOO_LONG;
    }

    for (i=0; i<*input_cnt; i++) {
        errp_get_string(psr, input_map);
        input_map += ERRP_MAX_NAME_LEN;
    }

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read QAM Capability attribute
void errp_get_attr_qam_cap (parser_t *psr, int len, errp_qam_cap_t *cap)
{
    int save_len;
    PARSER_CHECK(psr, len);
    save_len = psr->len;
    psr->len = len;

    get_short(psr, (uint16*)(&cap->chan_bw));
    get_short(psr, (uint16*)(&cap->j83));
    get_long(psr, (uint32*)(&cap->interleaver));
    get_long(psr, (uint32*)(&cap->docsis_video));
    get_short(psr, (uint16*)(&cap->modulation));

    if (!psr->err && psr->len != 0) {
        psr->err = PARSER_ERR_BAD_SYNTAX_LEN;
    }
    psr->len = save_len;
}


/// Read ERRP UPDATE message
// Note: to simply things, we assume each route attribute contain
//       at most one route!
//
void errp_get_update_msg (parser_t *psr, errp_resource_t *rsc,
                          errp_error_report_t *err_rpt)
{
    int i;
    uint8 missing_attr_type;

    ERRP_MSG_LOG("> UPDATE msg: len %d", psr->len);

    err_rpt->err = ERRP_ERR_UPDATE_MSG;
    err_rpt->err_sub = ERRP_ERR_SUB_UNSPECIFIED;

    rsc->attr_mask = 0;            // clear attribute mask first

    while (!psr->err && psr->len > 0) {
        const char* attr_ptr = psr->ptr;
        uint8 attr_flags;
        uint8 attr_type_code;
        uint16 attr_len;

        errp_get_attr_hdr(psr, &attr_flags, &attr_type_code, &attr_len);

//        printf("Psr: len %d, err %d;  Attr: len %d, type %d\n",
//               psr->len, psr->err, attr_len, attr_type_code);

        switch (attr_type_code) {

        case ERRP_ATTR_WITHDRAWN_ROUTE:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_WITHDRAWN_ROUTE)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_withdrawn_routes(psr, attr_len,
                                           &rsc->withdrawn_route);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_WITHDRAWN_ROUTE;
                ERRP_MSG_LOG("> Withdrawn route:");
                ERRP_MSG_LOG(">   Addr %s, family %d, prot %d",
                             rsc->withdrawn_route.addr,
                             rsc->withdrawn_route.type.addr_family,
                             rsc->withdrawn_route.type.app_protocol);
            }
            break;

        case ERRP_ATTR_REACHABLE_ROUTE:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_REACHABLE_ROUTE)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_reachable_routes(psr, attr_len,
                                           &rsc->reachable_route);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_REACHABLE_ROUTE;
                ERRP_MSG_LOG("> Reachable route:");
                ERRP_MSG_LOG(">   Addr %s, family %d, prot %d",
                             rsc->reachable_route.addr,
                             rsc->reachable_route.type.addr_family,
                             rsc->reachable_route.type.app_protocol);
            }
            break;

        case ERRP_ATTR_NEXT_HOP_SERVER:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_NEXT_HOP_SERVER)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_next_hop_server(psr, attr_len,
                                          &rsc->next_hop_server);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_NEXT_HOP_SERVER;
                ERRP_MSG_LOG("> Next Hop Server:");
                ERRP_MSG_LOG(">   Domain %d, Comp %s, Zone %s",
                             rsc->next_hop_server.addr_domain,
                             rsc->next_hop_server.component_addr,
                             rsc->next_hop_server.streaming_zone);
            }
            break;

        case ERRP_ATTR_QAM_NAMES:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_QAM_NAMES)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_qam_names(psr, attr_len, rsc->qam_name);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_QAM_NAMES;
                ERRP_MSG_LOG("> QAM names: %s", rsc->qam_name);
            }
            break;

        case ERRP_ATTR_CAS_CAP:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_CAS_CAP)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_cas_cap(psr, attr_len, &rsc->cas_cap);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_CAS_CAP;
                ERRP_MSG_LOG("> CAS Cap: type %d, schemd %d, key sz %d, "\
                             "CAS ID %d",
                             rsc->cas_cap.enc_type, rsc->cas_cap.enc_scheme,
                             rsc->cas_cap.key_len, rsc->cas_cap.cas_id);

            }
            break;

        case ERRP_ATTR_TOTAL_BANDWIDTH:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_TOTAL_BANDWIDTH)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_total_bandwidth(psr, attr_len, &rsc->total_bandwidth);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_TOTAL_BANDWIDTH;
                ERRP_MSG_LOG("> Total Bandwidth: %d", rsc->total_bandwidth);
            }
            break;

        case ERRP_ATTR_AVAIL_BANDWIDTH:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_AVAIL_BANDWIDTH)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_avail_bandwidth(psr, attr_len, &rsc->avail_bandwidth);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_AVAIL_BANDWIDTH;
                ERRP_MSG_LOG("> Avail Bandwidth: %d", rsc->avail_bandwidth);
            }
            break;

        case ERRP_ATTR_COST:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_COST)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_cost(psr, attr_len, &rsc->cost);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_COST;
                ERRP_MSG_LOG("> Cost: %d", rsc->cost);
            }
            break;

        case ERRP_ATTR_EDGE_INPUT:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_EDGE_INPUT)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_edge_input(psr, attr_len, &rsc->edge_input_cnt,
                                     rsc->edge_input);
            if (!psr->err) {
                errp_edge_input_t* ptr = rsc->edge_input;
                rsc->attr_mask |= ERRP_ATTR_MASK_EDGE_INPUT;
                ERRP_MSG_LOG("> Edge input: %d", rsc->edge_input_cnt);
                for (i=0; i<rsc->edge_input_cnt; i++, ptr++) {
                    ERRP_MSG_LOG(">   IF %d, Mask %08x, BW %d, Host %s, "\
                                 "Group %s", ptr->if_id, ptr->subnet_mask,
                                 ptr->max_bandwidth, ptr->host_name,
                                 ptr->group_name);
                }
            }
            break;

        case ERRP_ATTR_QAM_CFG:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_QAM_CFG)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_qam_cfg(psr, attr_len, &rsc->qam_cfg);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_QAM_CFG;
                ERRP_MSG_LOG("> QAM config: TSID %d, Freq %d, Annex %d, " \
                             "Mod %d, Itlv %d, BW %d",
                             rsc->qam_cfg.tsid, rsc->qam_cfg.freq,
                             rsc->qam_cfg.annex, rsc->qam_cfg.modulation,
                             rsc->qam_cfg.interleaver, rsc->qam_cfg.chan_bw);
            }
            break;

        case ERRP_ATTR_UDP_MAP:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_UDP_MAP)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_udp_map(psr, attr_len, &rsc->udp_map_cnt,
                                  rsc->udp_map);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_UDP_MAP;
                ERRP_MSG_LOG("> UDP map:");
                for (i=0; i<rsc->udp_map_cnt; i++) {
                    errp_udp_map_t *map = &rsc->udp_map[i];
                    switch (map->type) {
                    case ERRP_UDP_MAP_TYPE_STATIC_PORT:
                        ERRP_MSG_LOG(">   Static: udp %d, prog %d",
                                     map->static_port.udp_port,
                                     map->static_port.prog_num);
                        break;
                    case ERRP_UDP_MAP_TYPE_STATIC_RANGE:
                        ERRP_MSG_LOG(">   Static range: start udp %d, " \
                                     "start prog %d, cnt %d",
                                     map->static_range.start_udp_port,
                                     map->static_range.start_prog_num,
                                     map->static_range.count);
                        break;
                    case ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE:
                        ERRP_MSG_LOG(">   Dynamic range: start udp %d, cnt %d",
                                     map->dynamic_range.start_udp_port,
                                     map->dynamic_range.count);
                        break;
                    default:
                        ERRP_MSG_LOG(">   Bad UDP map type %d", map->type);
                    }
                }
            }
            break;

        case ERRP_ATTR_SERVICE_STATUS:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_SERVICE_STATUS)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_service_status(psr, attr_len, &rsc->service_status);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_SERVICE_STATUS;
                ERRP_MSG_LOG("> Service Status: %d", rsc->service_status);
            }
            break;

        case ERRP_ATTR_MAX_MPEG_FLOWS:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_MAX_MPEG_FLOWS)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_service_status(psr, attr_len, &rsc->max_flows);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_MAX_MPEG_FLOWS;
                ERRP_MSG_LOG("> Max MPEG Flows: %d", rsc->max_flows);
            }
            break;

        case ERRP_ATTR_NEXT_HOP_ALT:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_NEXT_HOP_ALT)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_next_hop_alt(psr, attr_len, &rsc->alt_server_cnt,
                                       rsc->next_hop_server_alt);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_NEXT_HOP_ALT;

                ERRP_MSG_LOG("> Next Hop Alt: %d", rsc->alt_server_cnt);
                for (i=0; i<rsc->alt_server_cnt; i++) {
                    ERRP_MSG_LOG(">   Domain %d, Comp %s, Zone %s",
                                 rsc->next_hop_server_alt[i].addr_domain,
                                 rsc->next_hop_server_alt[i].component_addr,
                                 rsc->next_hop_server_alt[i].streaming_zone);
                }
            }
            break;

        case ERRP_ATTR_PORT_ID:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_PORT_ID)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_port_id(psr, attr_len, &rsc->port_id);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_PORT_ID;
                ERRP_MSG_LOG("> Port ID: %d", rsc->port_id);
            }
            break;

        case ERRP_ATTR_FIBER_NODE:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_FIBER_NODE)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_fiber_node(psr, attr_len, &rsc->fiber_node_cnt,
                                     (char*)rsc->fiber_node);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_FIBER_NODE;
                ERRP_MSG_LOG("> Fiber node: %d", rsc->fiber_node_cnt);
                for (i=0; i<rsc->fiber_node_cnt; i++) {
                    ERRP_MSG_LOG(">   %s", rsc->fiber_node[i]);
                }
            }
            break;

        case ERRP_ATTR_QAM_CAP:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_QAM_CAP)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_qam_cap(psr, attr_len, &rsc->qam_cap);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_QAM_CAP;
                ERRP_MSG_LOG("> QAM Cap:");
                ERRP_MSG_LOG(">   D/V %08x, Mod %04x, J83 %04x, FEC %08x, "\
                             "BW %04x", *(uint32*)&rsc->qam_cap.docsis_video,
                             *(uint16*)&rsc->qam_cap.modulation,
                             *(uint16*)&rsc->qam_cap.j83,
                             *(uint32*)&rsc->qam_cap.interleaver,
                             *(uint16*)&rsc->qam_cap.chan_bw);
            }
            break;

        case ERRP_ATTR_INPUT_MAP:
            if (!psr->err && !errp_is_attr_well_known(attr_flags)) {
                err_rpt->err_sub = ERRP_ERR_SUB_ATTR_FLAGS;
                break;
            }
            if ((rsc->attr_mask & ERRP_ATTR_MASK_INPUT_MAP)) {
                err_rpt->err_sub = ERRP_ERR_SUB_MALFORMED_ATTR_LIST;
                break;
            }
            errp_get_attr_input_map(psr, attr_len, &rsc->input_cnt,
                                    (char*)rsc->input_map);
            if (!psr->err) {
                rsc->attr_mask |= ERRP_ATTR_MASK_INPUT_MAP;
                ERRP_MSG_LOG("> Input Map: %d", rsc->input_cnt);
                for (i=0; i<rsc->input_cnt; i++) {
                    ERRP_MSG_LOG(">   %s", rsc->input_map[i]);
                }
            }
            break;

        default:
            if (!(attr_flags & ERRP_ATTR_FLAG_NOT_WELL_KNOWN)) {
                // Well-known attribute
                err_rpt->err_sub = ERRP_ERR_SUB_UNRECOG_ATTR;
                err_rpt->len = attr_len + ERRP_ATTR_HDR_SIZE;
                memcpy(err_rpt->data, attr_ptr, err_rpt->len);
                return;

            } else {
                // Not well-known attribute.  Just skip it.
                ERRP_MSG_LOG("Unknown ATTR TC %d", attr_type_code);
                PARSER_CHECK(psr, attr_len);
                psr->ptr += attr_len;
            }
        }

        if (psr->err == PARSER_ERR_BAD_SYNTAX_LEN) {
            err_rpt->err_sub = ERRP_ERR_SUB_ATTR_LEN;
        }

        if (err_rpt->err_sub == ERRP_ERR_SUB_ATTR_FLAGS ||
            err_rpt->err_sub == ERRP_ERR_SUB_MALFORMED_ATTR_LIST ||
            err_rpt->err_sub == ERRP_ERR_SUB_ATTR_LEN) {
            err_rpt->len = attr_len + ERRP_ATTR_HDR_SIZE;
            memcpy(err_rpt->data, attr_ptr, err_rpt->len);
            return;
        }
    }

    ERRP_MSG_LOG("Attr mask: %08x", rsc->attr_mask);
    missing_attr_type = errp_find_missing_attr(rsc->attr_mask);
    if (missing_attr_type) {
        err_rpt->err_sub = ERRP_ERR_SUB_MISSING_ATTR;
        err_rpt->len = 1;
        *((uint8*)err_rpt->data) = missing_attr_type;
        return;
    }

    err_rpt->err = ERRP_OK;
}


/// Read ERRP NOTIFICATION message
void errp_get_notification_msg (parser_t *psr, errp_error_report_t *err_rpt)
{
    get_byte(psr, &err_rpt->err);
    get_byte(psr, &err_rpt->err_sub);
    err_rpt->len = psr->len;
    /// @todo  Check error data size to avoid overflow!
    memcpy(err_rpt->data, psr->ptr, psr->len);
}


/// Read ERRP KEEPALIVE message
void errp_get_keepalive_msg (parser_t *psr)
{
    /// @todo  Update hold timer
}

