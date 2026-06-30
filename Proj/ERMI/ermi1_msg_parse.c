/**
*    Copyright (c) 2010 by Cisco Systems, Inc.
*    All rights reserved.
*
*    @file: ermi1_msg_parse.c
*    @brief ERRP message parsing functions
*    @author Yi Tong Tse
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"
#include "ermi1.h"
// #include "qam.h"


#define ERRP_DEBUG              1


static inline
int ERRP_ERROR (err_code, err_subcode)
{
    return (err_code << 8) | err_subcode;
}


// Generic message parsing tools
//

// Get a byte, and advance the read pointer
static inline
int get_byte (const char **buf, uint8 *data)
{
    *data = *(*buf)++;
    return 1;
}


// Get a 16-bit short, and advance the read pointer
static inline
int get_short (const char **buf, uint16 *data)
{
    *data = *(*buf)++ << 8;
    *data |= *(*buf)++;
    return 2;
}


// Get a 32-bit long, and advance the read pointer
static inline
int get_long (const char **buf, uint32 *data)
{
    *data = *(*buf)++ << 24;
    *data |= *(*buf)++ << 16;
    *data |= *(*buf)++ << 8;
    *data |= *(*buf)++;
    return 4;
}


// Get a string with specified length, and advance the read pointer
static inline
int get_string (const char **buf, int len, char *data)
{
    memcpy(data, *buf, len);
    *buf += len;
    return len;
}


//! Read an ERRP-styled string (a string preceeded by its 2-byte lenght field)
//! @return Number of bytes read
//
int errp_get_string (const char **buf, char *string)
{
    uint16 str_len;
    get_short(buf, &str_len);
    get_string(buf, str_len, string);
    return sizeof(short) + str_len;
}


//! Read ERRP message header
void errp_get_msg_hdr (const char **buf, uint8 *msg_type, uint16 *msg_len)
{
    get_short(buf, msg_len);
    get_byte(buf, msg_type);
#if ERRP_DEBUG
    printf("Msg type %d, len %d\n", *msg_type, *msg_len);
#endif
}


//! Read ERRP parameter header
void errp_get_param_hdr (const char **buf, uint16 *param_type,
                         uint16 *param_len)
{
    get_short(buf, param_type);
    get_short(buf, param_len);
#if ERRP_DEBUG
    printf("Param type %d, len %d\n", *param_type, *param_len);
#endif
}


//! Read ERRP Capability Info header
void errp_get_cap_info_hdr (const char **buf, uint16 *cap_code, uint16 *cap_len)
{
    get_short(buf, cap_code);
    get_short(buf, cap_len);
#if ERRP_DEBUG
    printf("CapInfo type %d, len %d\n", *cap_code, *cap_len);
#endif
}


//! Read Route Type Supported Capability
int errp_get_cap_route_type_supported (const char **buf, int len,
                                       errp_node_t *node)
{
    len -= get_short(buf, &node->addr_family);
    len -= get_short(buf, &node->app_protocol);
    return (len==0)?  ERRP_OK :
               ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSPECIFIED);
}


//! Read Send Receive Capability
int errp_get_cap_send_receive (const char **buf, int len, errp_node_t *node)
{
    len -= get_long(buf, &node->send_receive);
    return (len==0)?  ERRP_OK :
               ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSPECIFIED);
}


//! Read ERRP Version Capability
int errp_get_cap_errp_version (const char **buf, int len, errp_node_t *node)
{
    len -= get_long(buf, &node->errp_version);
    return (len==0)?  ERRP_OK :
               ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSPECIFIED);
}


//! Read all Capability parameters
int errp_get_param_cap_infos (const char **buf, int len, errp_node_t *node)
{
    int rc;
  
    while (len > 0) {
        uint16 cap_code;
        uint16 cap_len;

        errp_get_cap_info_hdr(buf, &cap_code, &cap_len);
        len -= ERRP_CAP_INFO_HDR_SIZE + cap_len;
        if (len < 0) {
            rc = ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSPECIFIED);
        }

        switch (cap_code) {
        case ERRP_CAP_ROUTE_TYPES_SUPPORTED:
            rc = errp_get_cap_route_type_supported(buf, cap_len, node);
            break;

        case ERRP_CAP_SEND_RECEIVE:
            rc = errp_get_cap_send_receive(buf, cap_len, node);
            break;

        case ERRP_CAP_ERRP_VERSION:
            rc = errp_get_cap_errp_version(buf, cap_len, node);
            break;

        default:
            printf("Unknown ERRP capability code %d\n", cap_code);
            rc = ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSUP_CAP);
        }

        if (rc != ERRP_OK) {
            return rc;
        }
    }

    return ERRP_OK;
}


//! Read Streaming Zone Parameter
int errp_get_param_streaming_zone(const char **buf, int len, errp_node_t *node)
{
    // TODO: verify string len
    assert(len <= ERRP_MAX_NAME_LEN);
    get_string(buf, len, node->streaming_zone);
    return ERRP_OK;
}


//! Read Component Name Parameter
int errp_get_param_component_name(const char **buf, int len, errp_node_t *node)
{
    // TODO: verify string len
    assert(len <= ERRP_MAX_NAME_LEN);
    get_string(buf, len, node->component_name);
    return ERRP_OK;
}


//! Read Vendor Specific String Parameter
int errp_get_param_vendor_string(const char **buf, int len, errp_node_t *node)
{
    // TODO: verify string len
    assert(len <= ERRP_MAX_NAME_LEN);
    get_string(buf, len, node->vendor_string);
    return ERRP_OK;
}


//! Read all parameters in OPEN message
int errp_get_params (const char **buf, int len, errp_node_t *node)
{
    int rc;

    while (len > 0) {
        uint16 param_type;
        uint16 param_len;

        errp_get_param_hdr(buf, &param_type, &param_len);
        len -= ERRP_PARAM_HDR_SIZE + param_len;
        if (len < 0) {
            return ERRP_ERROR(ERRP_ERR_MSG_HEADER, ERRP_SUB_ERR_BAD_MSG_LEN);
        }

        switch (param_type) {
        case ERRP_PARAM_CAP_INFO:
            rc = errp_get_param_cap_infos(buf, param_len, node);
            break;

        case ERRP_PARAM_STREAMING_ZONE:
            rc = errp_get_param_streaming_zone(buf, param_len, node);
            break;

        case ERRP_PARAM_COMPONENT_NAME:
            rc = errp_get_param_component_name(buf, param_len, node);
            break;

        case ERRP_PARAM_VENDOR_STRING:
            rc = errp_get_param_vendor_string(buf, param_len, node);
            break;

        default:
            rc = ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSUP_OPT_PARAM);
        }
    }

    return rc;
}



//! Read ERRP OPEN message
//! @return error code
//
int errp_get_open_msg (const char **buf, int len, errp_node_t *node)
{
    int rc;
    uint16 opt_param_len;
    uint8 open_msg_version;
    uint8 resv;

#if ERRP_DEBUG
    printf("OPEN msg: len %d\n", len);
#endif

    len -= get_byte(buf, &open_msg_version);
    len -= get_byte(buf, &resv);
    len -= get_short(buf, &node->hold_time);
    len -= get_long(buf, &node->addr_domain);
    len -= get_long(buf, &node->errp_id);
    len -= get_short(buf, &opt_param_len);
    assert(len == opt_param_len);
    if (len != opt_param_len) {
        return ERRP_ERROR(ERRP_ERR_OPEN_MSG, ERRP_SUB_ERR_UNSPECIFIED);
    }

    rc = errp_get_params(buf, len, node);
    return rc;
}


//! Read ERRP Attribute header
void errp_get_attr_hdr (const char **buf, uint8 *attr_flags,
                        uint8 *attr_type_code, uint16 *attr_len)
{
    get_byte(buf, attr_flags);
    get_byte(buf, attr_type_code);
    get_short(buf, attr_len);
#if ERRP_DEBUG
    printf("Attr type %d, len %d\n", *attr_type_code, *attr_len);
#endif
}   


//! Read an ERRP Route attribute
int errp_get_route (const char **buf, errp_route_t *route)
{
    int len = get_short(buf, &route->addr_family);
    len += get_short(buf, &route->app_protocol);
    len += errp_get_string(buf, route->addr);
    printf("  Addr [%s], family %d, protocol %d\n",
           route->addr, route->addr_family, route->app_protocol);
    return len;
}


//! Read WithdrawnRoutes attribute
int errp_get_attr_withdrawn_routes (const char **buf, int len, int *num_routes,
                                    errp_route_t *route, int max_routes)
{
    *num_routes = 0;
    printf("Withdrawn routes:\n");

    while (len > 0) {
        len -= errp_get_route(buf, route);
        errp_route_print(route);
        route++;
        if (++(*num_routes) > max_routes) {
            assert(1);
        }
    }
    assert(len == 0);
}


//! Read ReachableRoutes attribute
int errp_get_attr_reachable_routes (const char **buf, int len, int *num_routes,
                                    errp_route_t *route, int max_routes)
{
    *num_routes = 0;
    printf("Reachable routes:\n");

    while (len > 0) {
        len -= errp_get_route(buf, route);
        errp_route_print(route);
        route++;
        if (++(*num_routes) > max_routes) {
            assert(1);
        }
    }
    assert(len == 0);
}


//! Read NextHopServer attribute
int errp_get_attr_next_hop_server (const char **buf, int len,
                                   uint32 *addr_domain, char* component_addr,
                                   char* streaming_zone)
{   
    len -= get_long(buf, addr_domain);
    len -= errp_get_string(buf, component_addr);
    len -= errp_get_string(buf, streaming_zone);
    printf("Next Hop Server: domain %d, Comp [%s], Zone [%s]\n",
           *addr_domain, component_addr, streaming_zone);
    assert(len == 0);
    return ERRP_OK;
}   


//! Read QAM Name attribute
int errp_get_attr_qam_names (const char **buf, int len, char *qam_name)
{
    len -= errp_get_string(buf, qam_name);
    printf("QAM name: [%s]\n", qam_name);
    assert(len == 0);
    return ERRP_OK;
}


//! Read Total Bandwidth attribute
int errp_get_attr_total_bandwidth (const char **buf, int len, uint32 *bandwidth)
{
    len -= get_long(buf, bandwidth);
    printf("Total BW: %d\n", *bandwidth);
    assert(len == 0);
    return ERRP_OK;
}


//! Read an Edge Input
int errp_get_edge_input (const char **buf, errp_edge_input_t *input)
{
    int len = get_long(buf, &input->subnet_mask);
    len += errp_get_string(buf, input->host_name);
    len += get_long(buf, &input->if_id);
    len += get_long(buf, &input->max_bandwidth);
    len += errp_get_string(buf, input->group_name);
    printf("  Host [%s], mask %08x, inf %d, max BW %d, group [%s]\n",
           input->host_name, input->subnet_mask, input->if_id,
           input->max_bandwidth, input->group_name);
    return len;
}


//! Read Edge Input attribute
int errp_get_attr_edge_input (const char **buf, int len, int *num_inputs,
                              errp_edge_input_t *input, int max_inputs)
{
    *num_inputs = 0;
    printf("Edge Inputs:\n");
    while (len > 0) {
        len -= errp_get_edge_input(buf, input++);
        if (++(*num_inputs) >= max_inputs) {
            assert(0);
        }
    }
    assert(len == 0);
    return ERRP_OK;
}


//! Read QAM Configuration attribute
int errp_get_attr_qam_cfg (const char **buf, int len, errp_qam_cfg_t *cfg)
{
    len -= get_long(buf, &cfg->freq);
    len -= get_byte(buf, &cfg->modulation);
    len -= get_byte(buf, &cfg->interleaver);
    len -= get_short(buf, &cfg->tsid);
    len -= get_byte(buf, &cfg->annex);
    len -= get_byte(buf, &cfg->chan_bw);
    printf("QAM cfg: freq %d, mod %d, FEC %d, TSID %d, annex %d, BW %d\n",
           cfg->freq, cfg->modulation, cfg->interleaver, cfg->tsid,
           cfg->annex, cfg->chan_bw);
    assert(len == 0);
    return ERRP_OK;
}


//! Read UDP Map for a static port
int errp_get_udp_map_static_port (const char **buf,
                                  errp_udp_map_static_port_t *map)
{
    int len = get_short(buf, &map->udp_port);
    len += get_short(buf, &map->prog_num);
    printf("  Static port: UDP %d, prog %d\n", map->udp_port, map->prog_num);
    return len;
}


//! Read UDP Map for a static port range
int errp_get_udp_map_static_range (const char **buf,
                                   errp_udp_map_static_range_t *map)
{
    int len = get_short(buf, &map->start_udp_port);
    len += get_short(buf, &map->start_prog_num);
    len += get_short(buf, &map->count);
    printf("  Static range: start UDP %d, start prog %d, count %d\n",
           map->start_udp_port, map->start_prog_num, map->count);
    return len;
}


//! Read UDP Map for a dynamic port range
int errp_get_udp_map_dynamic_range (const char **buf,
                                    errp_udp_map_dynamic_range_t *map)
{
    int len = get_short(buf, &map->start_udp_port);
    len += get_short(buf, &map->count);
    printf("  Dynamic range: start UDP %d, count %d\n",
           map->start_udp_port, map->count);
    return len;
}


//! Read UDP Map attribute
int errp_get_attr_udp_map (const char **buf, int len, int *num_maps,
                           errp_udp_map_t *map, int max_maps)
{
    int i;
    int count;
    *num_maps = 0;

    // Parse all static ports
    len -= get_long(buf, &count);
    printf("%d static ports:\n", count);
    for (i=0; i<count; i++) {
        map->type = ERRP_UDP_MAP_TYPE_STATIC_PORT;
        len -= errp_get_udp_map_static_port(buf, &(map++)->static_port);
    }
    (*num_maps) += count;

    // Parse all static ranges
    len -= get_long(buf, &count);
    printf("%d static ranges:\n", count);
    for (i=0; i<count; i++) {
        map->type = ERRP_UDP_MAP_TYPE_STATIC_RANGE;
        len -= errp_get_udp_map_static_range(buf, &(map++)->static_range);
    }
    (*num_maps) += count;

    // Parse all dynamic ranges
    len -= get_long(buf, &count);
    printf("%d dynamic ranges:\n", count);
    for (i=0; i<count; i++) {
        map->type = ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE;
        len -= errp_get_udp_map_dynamic_range(buf, &(map++)->dynamic_range);
    }
    (*num_maps) += count;

    assert(len == 0);
    return ERRP_OK;
}


//! Read Interface ID attribute
int errp_get_attr_port_id (const char **buf, int len, uint32 *port_id)
{
    len -= get_long(buf, port_id);
    printf("Port ID: %d\n", *port_id);
    assert(len == 0);
    return ERRP_OK;
}


//! Read QAM Capability attribute
int errp_get_attr_qam_cap (const char **buf, int len, errp_qam_cap_t *cap)
{
    len -= get_short(buf, (uint16*)(&cap->chan_bw));
    len -= get_short(buf, (uint16*)(&cap->j83));
    len -= get_long(buf, (uint32*)(&cap->interleaver));
    len -= get_long(buf, (uint32*)(&cap->docsis_video));
    len -= get_short(buf, (uint16*)(&cap->modulation));
    printf("QAM cap: BW %04x, J83 %04x, FEC %08x, D/V %08x, mod %04x\n",
           *(uint16*)&cap->chan_bw, *(uint16*)&cap->j83,
           *(uint32*)&cap->interleaver, *(uint32*)&cap->docsis_video,
           *(uint16*)&cap->modulation);
    assert(len == 0);
    return ERRP_OK;
}


//! Read ERRP UPDATE message
int errp_get_update_msg (const char **buf, int len, errp_node_t *node)
{
    int rc;
    int num_routes;
    errp_route_t route[ERRP_MAX_ROUTES_PER_ATTR];
    char qam_name[ERRP_MAX_NAME_LEN];
    uint32 total_bw;
    uint32 num_inputs;
    errp_edge_input_t edge_input;
    int num_maps;
    errp_udp_map_t udp_map;
    uint32 port_id;
    errp_qam_cap_t qam_cap;
    errp_qam_cfg_t qam_cfg;

#if ERRP_DEBUG
    printf("UPDATE msg: len %d\n", len);
#endif

    while (len > 0) {
        uint8 attr_flags;
        uint8 attr_type_code;
        uint16 attr_len;

        errp_get_attr_hdr(buf, &attr_flags, &attr_type_code, &attr_len);
        len -= ERRP_ATTR_HDR_SIZE + attr_len;
        if (len < 0) {
            return ERRP_ERROR(ERRP_ERR_MSG_HEADER, ERRP_SUB_ERR_BAD_MSG_LEN);
        }

        switch (attr_type_code) {

        case ERRP_ATTR_WITHDRAWN_ROUTE:
            rc = errp_get_attr_withdrawn_routes(buf, attr_len, &num_routes,
                                 route, ERRP_MAX_ROUTES_PER_ATTR);
            break;

        case ERRP_ATTR_REACHABLE_ROUTE:
            rc = errp_get_attr_reachable_routes(buf, attr_len, &num_routes,
                                 route, ERRP_MAX_ROUTES_PER_ATTR);
            break;

        case ERRP_ATTR_NEXT_HOP_SERVER:
            errp_get_attr_next_hop_server(buf, attr_len,
                            &node->addr_domain, node->component_name,
                            node->streaming_zone);
            break;

        case ERRP_ATTR_QAM_NAMES:
            errp_get_attr_qam_names(buf, attr_len, qam_name);
            break;

        case ERRP_ATTR_CAS_CAP:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_TOTAL_BANDWIDTH:
            errp_get_attr_total_bandwidth(buf, attr_len, &total_bw);
            break;

        case ERRP_ATTR_AVAIL_BANDWIDTH:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_COST:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_EDGE_INPUT:
            errp_get_attr_edge_input(buf, attr_len, &num_inputs, &edge_input,
                                     ERRP_MAX_EDGE_INPUTS_PER_ATTR);
            break;

        case ERRP_ATTR_QAM_CFG:
            errp_get_attr_qam_cfg(buf, attr_len, &qam_cfg);
            break;

        case ERRP_ATTR_UDP_MAP:
            errp_get_attr_udp_map(buf, attr_len, &num_maps, &udp_map,
                                  ERRP_MAX_UDP_MAPS_PER_ATTR);
            break;

        case ERRP_ATTR_SERVICE_STATUS:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_MAX_MPEG_FLOWS:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_NEXT_HOP_ALT:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_PORT_ID:
            errp_get_attr_port_id(buf, attr_len, &port_id);
            break;

        case ERRP_ATTR_FIBER_NODE:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        case ERRP_ATTR_QAM_CAP:
            errp_get_attr_qam_cap(buf, attr_len, &qam_cap);
            break;

        case ERRP_ATTR_INPUT_MAP:
            printf("Attribute type %d is not yet supported\n", attr_type_code);
            break;

        default:
            rc = ERRP_ERROR(ERRP_ERR_UPDATE_MSG, ERRP_SUB_ERR_UNRECOG_ATTR);
        }
    }

    return rc;
}


//! Read ERRP NOTIFICATION message
int errp_get_notification_msg (const char **buf, int len, errp_node_t *node)
{
    return ERRP_OK;
}


//! Read ERRP KEEPALIVE message
int errp_get_keepalive_msg (const char **buf, int len, errp_node_t *node)
{
    // TODO: update hold timer
    assert(len == 0);
    return ERRP_OK;
}


//! Get an ERRP message
//! @return Error code
//
int errp_get_msg (const char **buf,  errp_node_t *node)
{
    int rc;
    int16 len;
    uint8 msg_type;

    errp_get_msg_hdr(buf, &msg_type, &len);
    len -= ERRP_MSG_HDR_SIZE;
    if (len < 0) {
        return ERRP_ERROR(ERRP_ERR_MSG_HEADER, ERRP_SUB_ERR_BAD_MSG_LEN);
    }

    switch (msg_type) {

    case ERRP_OPEN:
        rc = errp_get_open_msg(buf, len, node);
        break;

    case ERRP_UPDATE:
        rc = errp_get_update_msg(buf, len, node);
        break;

    case ERRP_NOTIFICATION:
        rc = errp_get_notification_msg(buf, len, node);
        break;

    case ERRP_KEEPALIVE:
        rc = errp_get_keepalive_msg(buf, len, node);
        break;

    default:
        rc = ERRP_ERROR(ERRP_ERR_MSG_HEADER, ERRP_SUB_ERR_BAD_MSG_TYPE);
    }

    return rc;
}


//! Print ERRP node info
void errp_node_print (const errp_node_t *node)
{
    printf("ERRP ID: %d\n", node->errp_id);
    printf("Hold time: %d\n", node->hold_time);
    printf("Address domain: %d\n", node->addr_domain);
    printf("Address family: %d\n", node->addr_family);
    printf("Application protocol: %d\n", node->hold_time);
    printf("Send receive cap: %d\n", node->send_receive);
    printf("ERRP version: %d\n", node->errp_version);
    if (strlen(node->streaming_zone)) {
        printf("Streaming zone: [%s]\n", node->streaming_zone);
    }
    if (strlen(node->component_name)) {
        printf("Component name: [%s]\n", node->component_name);
    }
    if (strlen(node->vendor_string)) {
        printf("Vendor string: [%s]\n", node->vendor_string);
    }
}

//! Print ERRP Route info
void errp_route_print (const errp_route_t *route)
{
    printf("Addr family %d, Protocol %d, Addr [%s]\n",
           route->addr_family, route->app_protocol, route->addr);
}

