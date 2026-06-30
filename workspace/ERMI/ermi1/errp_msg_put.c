///  Copyright (c) 2011 by Cisco Systems, Inc.
///  All rights reserved.
///
///  @file      errp_msg_put.c
///  @brief     ERRP (ERMI-1) message generating functions
///  @author    Yi Tong Tse


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "util.h"
#include "errp.h"
#include "rfgw_errp.h"


#define ERRP_MSG_LOG(fmt, str...)        \
    printf("ERRP_MSG: "fmt"\n", ##str);


/// Write an ERRP-styled string (a string preceeded by its 2-byte length field)
/// @return Number of bytes written
int errp_put_string (char **buf, const char *string)
{
    int str_len = strlen(string);
    if (str_len > ERRP_MAX_NAME_LEN) {
        str_len = ERRP_MAX_NAME_LEN;
    }
    put_short(buf, str_len);
    put_string(buf, string);
    return 2 + str_len;
}


// ERRP message formatting
//

/// Skip over the ERRP message header
char* errp_skip_msg_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_MSG_HDR_SIZE;
    return buf2;
}


/// Write ERRP message header
int errp_put_msg_hdr (char **buf, int msg_type, int len)
{
    put_short(buf, len);
    put_byte(buf, msg_type);
    return ERRP_MSG_HDR_SIZE;
}


/// Skip over ERRP parameter header (in OPEN message)
char* errp_skip_param_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_PARAM_HDR_SIZE;
    return buf2;
}


/// Write ERRP parameter header (in OPEN message)
int errp_put_param_hdr (char **buf, errp_param_t param_type, int param_len)
{
    put_short(buf, param_type);
    put_short(buf, param_len);
    return ERRP_PARAM_HDR_SIZE;
}


/// Write ERRP Capability Info header
int errp_put_cap_info_hdr (char **buf, errp_cap_code_t cap_code, int cap_len)
{
    put_short(buf, cap_code);
    put_short(buf, cap_len);
    return ERRP_CAP_INFO_HDR_SIZE;
}


// Write ERRP Route Type
int errp_put_route_type (char **buf, const errp_route_type_t *route_type)
{
    int len = put_short(buf, route_type->addr_family);
    len += put_short(buf, route_type->app_protocol);
    return len;
}


/// Write ERRP Route Type Supported Capability
int errp_put_cap_route_type_supported (char **buf, const errp_node_t *node)
{
    int i;
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_ROUTE_TYPES_SUPPORTED,
                                    node->route_type_cnt * 4);
    for (i=0; i<node->route_type_cnt; i++) {
        len += errp_put_route_type(buf, &node->route_type[i]);
    }
    return len;
}


/// Write ERRP Send Receive Capability
int errp_put_cap_send_receive (char **buf, const errp_node_t *node)
{
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_SEND_RECEIVE, 4);
    len += put_long(buf, node->send_receive);
    return len;
}


/// Write ERRP Version Capability
int errp_put_cap_errp_version (char **buf, const errp_node_t *node)
{
    int len = errp_put_cap_info_hdr(buf, ERRP_CAP_ERRP_VERSION, 4);
    len += put_long(buf, node->errp_version);
    return len;
}


/// Write all ERRP Capabilities
int errp_put_param_cap_info (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = errp_put_cap_route_type_supported(buf, node);
    len += errp_put_cap_send_receive(buf, node);
    len += errp_put_cap_errp_version(buf, node);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_CAP_INFO, len);
    return len;
}


/// Write Streaming Zone parameter
int errp_put_param_streaming_zone (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->streaming_zone);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_STREAMING_ZONE, len);
    return len;
}


/// Write Component Name parameter
int errp_put_param_component_name (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->component_name);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_COMPONENT_NAME, len);
    return len;
}


/// Write Vendor Specific String parameter (optional)
int errp_put_param_vendor_string (char **buf, const errp_node_t *node)
{
    char* hdr_ptr = errp_skip_param_hdr(buf);
    int len = put_string(buf, node->vendor_string);
    len += errp_put_param_hdr(&hdr_ptr, ERRP_PARAM_VENDOR_STRING, len);
    return len;
}


/// Write ERRP OPEN message
int errp_put_open_msg (char **buf, const errp_node_t *node)
{
    char* param_len_ptr;
    int param_len;

    char* msg_hdr = errp_skip_msg_hdr(buf);
    int msg_len = put_byte(buf, node->version);
    msg_len += put_byte(buf, ERRP_RESERVED);
    msg_len += put_short(buf, node->hold_time);
    msg_len += put_long(buf, node->addr_domain);
    msg_len += put_long(buf, node->errp_id);

    param_len_ptr = skip_short(buf);
    param_len = errp_put_param_cap_info(buf, node);
    if (strlen(node->streaming_zone)) {
        param_len += errp_put_param_streaming_zone(buf, node);
    }
    if (strlen(node->component_name)) {
        param_len += errp_put_param_component_name(buf, node);
    }
    if (strlen(node->vendor_string)) {
        param_len += errp_put_param_vendor_string(buf, node);
    }
    msg_len += put_short(&param_len_ptr, param_len);
    msg_len += ERRP_MSG_HDR_SIZE + param_len;

    errp_put_msg_hdr(&msg_hdr, ERRP_OPEN, msg_len);
    return msg_len;
}


/// Skip over ERRP attribute header
static inline
char* errp_skip_attrib_hdr (char **buf)
{
    char* buf2 = *buf;
    *buf += ERRP_ATTR_HDR_SIZE;
    return buf2;
}


/// Write ERRP attribute header
int errp_put_attr_hdr (char **buf, uint8 attr_flags, uint8 attr_type_code,
                       uint16 attr_len)
{
    int len = put_byte(buf, attr_flags);
    len += put_byte(buf, attr_type_code);
    len += put_short(buf, attr_len);
    return len;
}


/// Write a route attribute
int errp_put_route (char **buf, const errp_route_t *route)
{
    int len = errp_put_route_type(buf, &route->type);
    len += errp_put_string(buf, route->addr);
    return len;
}


/// Write WithdrawnRoutes attribute
int errp_put_attr_withdrawn_routes (char **buf, int route_cnt,
                                    const errp_route_t *route)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; route_cnt > 0; route_cnt--) {
        len += errp_put_route(buf, route++);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_WITHDRAWN_ROUTE, len);
    return len;
}


/// Write ReachableRoutes attribute
int errp_put_attr_reachable_routes (char **buf, int route_cnt,
                                    const errp_route_t *route)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; route_cnt > 0; route_cnt--) {
        len += errp_put_route(buf, route++);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_REACHABLE_ROUTE, len);
    return len;
}


/// Write a next hop server
int errp_put_server (char **buf, const errp_server_t *server)
{
    int len = put_long(buf, server->addr_domain);
    len += errp_put_string(buf, server->component_addr);
    len += errp_put_string(buf, server->streaming_zone);
    return len;
}


/// Write NextHopServer attribute
int errp_put_attr_next_hop_server (char **buf, const errp_server_t *server)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = errp_put_server(buf, server);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_NEXT_HOP_SERVER, len);
    return len;
}


/// Write NextHopServerAlternate attribute
int errp_put_attr_next_hop_alt (char **buf, int server_cnt,
                                const errp_server_t *server)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_short(buf, server_cnt);

    for ( ; server_cnt > 0; server_cnt--) {
        char* server_hdr = skip_short(buf);
        int item_len = errp_put_server(buf, server++);
        len += put_short(&server_hdr, item_len);
        len += item_len;
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_NEXT_HOP_ALT, len);
    return len;
}


/// Write QAM Names attribute
/// Note: According to ERMI, only one QAM Name can be included in an
///       attribute in the UDPATE from EQAM to ERM.
int errp_put_attr_qam_names (char **buf, const char *qam_name)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = errp_put_string(buf, qam_name);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_NAMES, len);
    return len;
}


// Write CAS Capability attribute
int errp_put_attr_cas_cap (char **buf, const errp_cas_cap_t *cas_cap)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_byte(buf, cas_cap->enc_type);
    len += put_byte(buf, cas_cap->enc_scheme);
    len += put_short(buf, cas_cap->key_len);
    len += put_short(buf, cas_cap->cas_id);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_CAS_CAP, len);
    return len;
}


/// Write Total Bandwidth attribute
int errp_put_attr_total_bandwidth (char **buf, int bandwidth)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_TOTAL_BANDWIDTH, 4);
    len += put_long(buf, bandwidth);
    return len;
}


/// Write Available Bandwidth attribute
int errp_put_attr_avail_bandwidth (char **buf, int bandwidth)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_AVAIL_BANDWIDTH, 4);
    len += put_long(buf, bandwidth);
    return len;
}


/// Write Cost attribute
int errp_put_attr_cost (char **buf, uint8 cost)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_COST, 1);
    len += put_byte(buf, cost);
    return len;
}


/// Write an Edge Input
int errp_put_edge_input (char **buf, const errp_edge_input_t *input)
{
    int len = put_long(buf, input->subnet_mask);
    len += errp_put_string(buf, input->host_name);
    len += put_long(buf, input->if_id);
    len += put_long(buf, input->max_bandwidth);
    len += errp_put_string(buf, input->group_name);
    return len;
}


/// Write Edge Input attribute
int errp_put_attr_edge_input (char **buf, int edge_input_cnt,
                              const errp_edge_input_t *edge_input)
{
    int len = 0;
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    for ( ; edge_input_cnt > 0; edge_input_cnt--) {
        len += errp_put_edge_input(buf, edge_input++);
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_EDGE_INPUT, len);
    return len;
}


/// Write QAM Configuration attribute
int errp_put_attr_qam_cfg (char **buf, const errp_qam_cfg_t *cfg)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_long(buf, cfg->freq);
    len += put_byte(buf, cfg->modulation);
    len += put_byte(buf, cfg->interleaver);
    len += put_short(buf, cfg->tsid);
    len += put_byte(buf, cfg->annex);
    len += put_byte(buf, cfg->chan_bw);
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_CFG, len);
    return len;
}


/// Write UPD Map for a static port
int errp_put_udp_map_static_port (char **buf,
                                  const errp_udp_map_static_port_t *map)
{
    int len = put_short(buf, map->udp_port);
    len += put_short(buf, map->prog_num);
    return len;
}


/// Write UPD Map for a static port range
int errp_put_udp_map_static_range (char **buf,
                                   const errp_udp_map_static_range_t *map)
{
    int len = put_short(buf, map->start_udp_port);
    len += put_short(buf, map->start_prog_num);
    len += put_short(buf, map->count);
    return len;
}


/// Write UPD Map for a dynamic port range
int errp_put_udp_map_dynamic_range (char **buf,
                                    const errp_udp_map_dynamic_range_t *map)
{
    int len = put_short(buf, map->start_udp_port);
    len += put_short(buf, map->count);
    return len;
}


/// Write UDP Map attribute
int errp_put_attr_udp_map (char **buf, int num_maps, const errp_udp_map_t *map)
{
    int i;
    int len = 0;
    uint32 count;
    char *ptr;

    char* attr_hdr = errp_skip_attrib_hdr(buf);

    // Scan for static ports
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_STATIC_PORT) {
            count++;
            len += errp_put_udp_map_static_port(buf, &map->static_port);
        }
    }
    len += put_long(&ptr, count);

    // Scan for static ranges
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_STATIC_RANGE) {
            count++;
            len += errp_put_udp_map_static_range(buf, &map->static_range);
        }
    }
    len += put_long(&ptr, count);

    // Scan for dynamic ranges
    count = 0;
    ptr = skip_long(buf);
    for (i=0; i<num_maps; i++) {
        if (map->type == ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE) {
            count++;
            len += errp_put_udp_map_dynamic_range(buf, &map->dynamic_range);
        }
    }
    len += put_long(&ptr, count);

    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_UDP_MAP, len);
    return len;
}


/// Write Service Status attribute
int errp_put_attr_service_status (char **buf, uint32 service_status)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_SERVICE_STATUS, 4);
    len += put_long(buf, service_status);
    return len;
}


/// Write Max MPEG Flows attribute
int errp_put_attr_max_mpeg_flows (char **buf, uint32 max_flows)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_MAX_MPEG_FLOWS, 4);
    len += put_long(buf, max_flows);
    return len;
}


/// Write Interface ID attribute
int errp_put_attr_port_id (char **buf, uint32 port_id)
{
    int len = errp_put_attr_hdr(buf, 0, ERRP_ATTR_PORT_ID, 4);
    len += put_long(buf, port_id);
    return len;
}


/// Write Fiber Node attribute
/// @note  fiber_node is actually flatted 2D char array [][ERRP_MAX_NAME_LEN]
int errp_put_attr_fiber_node (char **buf, int fiber_node_cnt,
                              const char *fiber_node)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = 0;
    assert(fiber_node_cnt <= ERRP_MAX_FIBER_NODES_PER_ROUTE);
    for ( ; fiber_node_cnt > 0; fiber_node_cnt--) {
        len += errp_put_string(buf, fiber_node);
        fiber_node += ERRP_MAX_NAME_LEN;
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_FIBER_NODE, len);
    return len;
}


/// Write Input Map attribute
/// @note  input_map is actually flatted 2D char array [][ERRP_MAX_NAME_LEN]
int errp_put_attr_input_map (char **buf, int input_cnt, const char *input_map)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_long(buf, input_cnt);
    assert(input_cnt <= ERRP_MAX_INPUTS_PER_ROUTE);
    for ( ; input_cnt > 0; input_cnt--) {
        len += errp_put_string(buf, input_map);
        input_map += ERRP_MAX_NAME_LEN;
    }
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_INPUT_MAP, len);
    return len;
}


/// Write QAM Capability attribute
int errp_put_attr_qam_cap (char **buf, const errp_qam_cap_t *cap)
{
    char* attr_hdr = errp_skip_attrib_hdr(buf);
    int len = put_short(buf, *(uint16*)(&cap->chan_bw));
    len += put_short(buf, *(uint16*)(&cap->j83));
    len += put_long(buf, *(uint32*)(&cap->interleaver));
    len += put_long(buf, *(uint32*)(&cap->docsis_video));
    len += put_short(buf, *(uint16*)(&cap->modulation));
    len += errp_put_attr_hdr(&attr_hdr, 0, ERRP_ATTR_QAM_CAP, len);
    return len;
}


/// Write ERRP UPDATE message
int errp_put_update_msg (char **buf, const errp_resource_t *rsc)
{
    int i;
    int len = ERRP_MSG_HDR_SIZE;
    char* msg_hdr = errp_skip_msg_hdr(buf);

    ERRP_MSG_LOG("Attr mask: %08x\n", rsc->attr_mask);

    if (rsc->attr_mask & ERRP_ATTR_MASK_WITHDRAWN_ROUTE) {
        ERRP_MSG_LOG("< Withdrawn route:");
        ERRP_MSG_LOG("<   Addr %s, family %d, prot %d",
                     rsc->withdrawn_route.addr,
                     rsc->withdrawn_route.type.addr_family,
                     rsc->withdrawn_route.type.app_protocol);
        len += errp_put_attr_withdrawn_routes(buf, 1, &rsc->withdrawn_route);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_REACHABLE_ROUTE) {
        ERRP_MSG_LOG("< Reachable route:");
        ERRP_MSG_LOG("<   Addr %s, family %d, prot %d",
                     rsc->reachable_route.addr,
                     rsc->reachable_route.type.addr_family,
                     rsc->reachable_route.type.app_protocol);
        len += errp_put_attr_reachable_routes(buf, 1, &rsc->reachable_route);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_NEXT_HOP_SERVER) {
        ERRP_MSG_LOG("< Next Hop Server:");
        ERRP_MSG_LOG("<   Domain %d, Comp %s, Zone %s",
                     rsc->next_hop_server.addr_domain,
                     rsc->next_hop_server.component_addr,
                     rsc->next_hop_server.streaming_zone);
        len += errp_put_attr_next_hop_server(buf, &rsc->next_hop_server);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_QAM_NAMES) {
        ERRP_MSG_LOG("< Qam Names: %s", rsc->qam_name);
        len += errp_put_attr_qam_names(buf, rsc->qam_name);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_CAS_CAP) {
        ERRP_MSG_LOG("< CAS Cap: type %d, schemd %d, key sz %d, CAS ID %d",
                     rsc->cas_cap.enc_type, rsc->cas_cap.enc_scheme,
                     rsc->cas_cap.key_len, rsc->cas_cap.cas_id);
        len += errp_put_attr_cas_cap(buf, &rsc->cas_cap);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_TOTAL_BANDWIDTH) {
        ERRP_MSG_LOG("< Total Bandwidth: %d", rsc->total_bandwidth);
        len += errp_put_attr_total_bandwidth(buf, rsc->total_bandwidth);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_AVAIL_BANDWIDTH) {
        ERRP_MSG_LOG("< Avail Bandwidth: %d", rsc->avail_bandwidth);
        len += errp_put_attr_avail_bandwidth(buf, rsc->avail_bandwidth);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_COST) {
        ERRP_MSG_LOG("< Cost: %d", rsc->cost);
        len += errp_put_attr_cost(buf, rsc->cost);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_EDGE_INPUT) {
        errp_edge_input_t* ptr = rsc->edge_input;
        ERRP_MSG_LOG("< Edge Input: %d", rsc->edge_input_cnt);
        for (i=0; i<rsc->edge_input_cnt; i++, ptr++) {
            ERRP_MSG_LOG("<   IF %d, Mask %08x, BW %d, Host %s, Group %s",
                         ptr->if_id, ptr->subnet_mask, ptr->max_bandwidth,
                         ptr->host_name, ptr->group_name);
        }
        len += errp_put_attr_edge_input(buf, rsc->edge_input_cnt,
                                (const errp_edge_input_t*)(&rsc->edge_input));
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_QAM_CFG) {
        ERRP_MSG_LOG("< QAM config: TSID %d, Freq %d, Annex %d, Mod %d, " \
                     "Itlv %d, BW %d",
                     rsc->qam_cfg.tsid, rsc->qam_cfg.freq, rsc->qam_cfg.annex,
                     rsc->qam_cfg.modulation, rsc->qam_cfg.interleaver,
                     rsc->qam_cfg.chan_bw);
        len += errp_put_attr_qam_cfg(buf, &rsc->qam_cfg);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_UDP_MAP) {
        ERRP_MSG_LOG("< UDP Map: %d maps", rsc->udp_map_cnt);
        for (i=0; i<rsc->udp_map_cnt; i++) {
            const errp_udp_map_t *map = &rsc->udp_map[i];
            switch (map->type) {
            case ERRP_UDP_MAP_TYPE_STATIC_PORT:
                ERRP_MSG_LOG("<   Static: udp %d, prog %d",
                             map->static_port.udp_port,
                             map->static_port.prog_num);
                break;
            case ERRP_UDP_MAP_TYPE_STATIC_RANGE:
                ERRP_MSG_LOG("<   Static range: start udp %d, start prog %d, " \
                             "cnt %d", map->static_range.start_udp_port,
                             map->static_range.start_prog_num,
                             map->static_range.count);
                break;
            case ERRP_UDP_MAP_TYPE_DYNAMIC_RANGE:
                ERRP_MSG_LOG("<   Dynamic range: start udp %d, cnt %d",
                             map->dynamic_range.start_udp_port,
                             map->dynamic_range.count);
                break;
            default:
                ERRP_MSG_LOG("<   Bad UDP map type %d", map->type);
            }
        }
        len += errp_put_attr_udp_map(buf, rsc->udp_map_cnt, rsc->udp_map);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_SERVICE_STATUS) {
        ERRP_MSG_LOG("< Service Status: %d", rsc->service_status);
        len += errp_put_attr_service_status(buf, rsc->service_status);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_MAX_MPEG_FLOWS) {
        ERRP_MSG_LOG("< Max MPEG Flows: %d", rsc->max_flows);
        len += errp_put_attr_max_mpeg_flows(buf, rsc->max_flows);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_NEXT_HOP_ALT) {
        ERRP_MSG_LOG("< Next Hop Alternate: %d", rsc->alt_server_cnt);
        for (i=0; i<rsc->alt_server_cnt; i++) {
            ERRP_MSG_LOG("<   Domain %d, Comp %s, Zone %s",
                         rsc->next_hop_server_alt[i].addr_domain,
                         rsc->next_hop_server_alt[i].component_addr,
                         rsc->next_hop_server_alt[i].streaming_zone);
        }
        len += errp_put_attr_next_hop_alt(buf, rsc->alt_server_cnt,
                                          rsc->next_hop_server_alt);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_PORT_ID) {
        ERRP_MSG_LOG("< Port ID: %d", rsc->port_id);
        len += errp_put_attr_port_id(buf, rsc->port_id);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_FIBER_NODE) {
        ERRP_MSG_LOG("< Fiber Node:");
        ERRP_MSG_LOG("< Fiber node: %d", rsc->fiber_node_cnt);
        for (i=0; i<rsc->fiber_node_cnt; i++) {
            ERRP_MSG_LOG("<   %s", rsc->fiber_node[i]);
        }
        len += errp_put_attr_fiber_node(buf, rsc->fiber_node_cnt,
                                        (const char*)rsc->fiber_node);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_QAM_CAP) {
        ERRP_MSG_LOG("< QAM Cap:");
        ERRP_MSG_LOG("<   D/V %08x, Mod %04x, J83 %04x, FEC %08x, BW %04x",
                     *(uint32*)&rsc->qam_cap.docsis_video,
                     *(uint16*)&rsc->qam_cap.modulation,
                     *(uint16*)&rsc->qam_cap.j83,
                     *(uint32*)&rsc->qam_cap.interleaver,
                     *(uint16*)&rsc->qam_cap.chan_bw)
        len += errp_put_attr_qam_cap(buf, &rsc->qam_cap);
    }

    if (rsc->attr_mask & ERRP_ATTR_MASK_INPUT_MAP) {
        ERRP_MSG_LOG("< Input Map: %d", rsc->input_cnt);
        for (i=0; i<rsc->input_cnt; i++) {
            ERRP_MSG_LOG("<   %s", rsc->input_map[i]);
        }
        len += errp_put_attr_input_map(buf, rsc->input_cnt,
                                       (const char*)rsc->input_map);
    }

    errp_put_msg_hdr(&msg_hdr, ERRP_UPDATE, len);
    return len;
}


/// Write ERRP NOTIFICATION message
int errp_put_notification_msg (char **buf, const errp_error_report_t *err_rpt)
{
    int len = ERRP_MSG_HDR_SIZE;
    char* msg_hdr = errp_skip_msg_hdr(buf);
    len += put_byte(buf, err_rpt->err);
    len += put_byte(buf, err_rpt->err_sub);
    len += put_data(buf, err_rpt->data, err_rpt->len);
    errp_put_msg_hdr(&msg_hdr, ERRP_NOTIFICATION, len);
printf("NOT PUT: data_len %d, msg_len %d\n", err_rpt->len, len);
    return len;
}


/// Write ERRP KEEPALIVE message
int errp_put_keepalive_msg (char **buf)
{
    return errp_put_msg_hdr(buf, ERRP_KEEPALIVE, ERRP_MSG_HDR_SIZE);
}

